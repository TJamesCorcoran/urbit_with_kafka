/* v/kafk.c
**
** This file is in the public domain.
*/

// there are two methods of storing events:
//    * egz.hope log file
//    * kafka log servers
// this code implementes function around the latter.


//  Design goals:
//  1) bc logging takes time, we want to log events as soon as we receive them
//  2) ...and process them IN PARALLEL w kafka servers chugging along
//  3) bc processing can either succeed or fail, we want to 
//       log a second time (a "commit" of sorts) when processing succeeds.
//       That way later playback won't assume that failed things actually changed state.
//  
//  INVARIANT: What is absolutely certain is that we *can't emit a response until the
//  event is logged*.  Ideally, we are trying to log it at the same time as
//  we're trying to compute it.
//
//  THOUGHT (unconfirmed by Curtis): egz.hope logging does not suffer
//  under this constraint logging to local disk is effectively
//  foolproof, and thus can use a simpler architecture.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "all.h"
#include "v/vere.h"
#include "v/kafk.h"

#include <errno.h>
#include <time.h>   

clock_t before;

#define WRITE_PARTITION RD_KAFKA_PARTITION_UA
#define READ_PARTITION  0  
#define READ_TIMEOUT    3000
static void _kafka_msg_delivered_cb (rd_kafka_t *rk, 
                                     const rd_kafka_message_t *rkmessage, 
                                     void *opaque);

//--------------------
//  init functions
//--------------------

void u2_kafk_init()
{
  

  if (! u2_Host.ops_u.kaf_c) { return ; }

  // set global vars
  //----------------
  u2K->largest_offset_seen_ds = 0;
  u2K->largest_offset_seen_precom_ds = 0;

  // Create Kafka handles
  //----------------------
  //
  char errstr[512];

  // 1) build configuration objects
  rd_kafka_conf_t *prod_conf_u  = rd_kafka_conf_new();
  rd_kafka_conf_t *cons_conf_u  = rd_kafka_conf_new();

  // 2) tweak conf objects to set up delivery callback (producer only)
  rd_kafka_conf_set_dr_msg_cb(prod_conf_u, & _kafka_msg_delivered_cb);

  // 3) use conf objects to create kafka handles; now we're done w the conf objects
  u2K->kafka_prod_handle_u = rd_kafka_new(RD_KAFKA_PRODUCER, prod_conf_u, errstr, sizeof(errstr));
  if (! u2K->kafka_prod_handle_u ) {
    fprintf(stderr, "%% Failed to create producer: %s\n", errstr);
    exit(1);
  }
  u2K->kafka_cons_handle_u = rd_kafka_new(RD_KAFKA_CONSUMER, cons_conf_u, errstr, sizeof(errstr));
  if (! u2K->kafka_cons_handle_u ) {
    fprintf(stderr, "%% Failed to create consumer: %s\n", errstr);
    exit(1);
  }

  // 4) Add brokers.  These were specified on the command line.
  if (rd_kafka_brokers_add(u2K->kafka_prod_handle_u, u2_Host.ops_u.kaf_c) == 0) {
    fprintf(stderr, "%% No valid brokers specified\n");
    exit(1);
  }
  if (rd_kafka_brokers_add(u2K->kafka_cons_handle_u, u2_Host.ops_u.kaf_c) == 0) {
    fprintf(stderr, "%% No valid brokers specified\n");
    exit(1);
  }



  // create topic handles
  //---------------------
  //

  //   1) create configuration
  rd_kafka_topic_conf_t * topic_prod_conf_u = rd_kafka_topic_conf_new();
  rd_kafka_topic_conf_t * topic_cons_conf_u = rd_kafka_topic_conf_new();

  //    2) customize configuration
  rd_kafka_conf_res_t prod_topic_u = rd_kafka_topic_conf_set(topic_prod_conf_u, "produce.offset.report", "true", errstr, sizeof(errstr));
  if (prod_topic_u != RD_KAFKA_CONF_OK){
    exit(-1);
  }

  //    3) actually create topic handles
  u2K->topic_prod_handle_u = rd_kafka_topic_new(u2K->kafka_prod_handle_u, u2_Host.cpu_c + 1, topic_prod_conf_u);
  u2K->topic_cons_handle_u = rd_kafka_topic_new(u2K->kafka_cons_handle_u, u2_Host.cpu_c + 1, topic_cons_conf_u);
 

  // note in the global datastructure that all systems are green
  u2K->inited_t = c3_true;
}

//--------------------
//  write functions
//--------------------

void _kafka_finalize_and_emit(uv_async_t* async_u, int status_w)
{
  clog_thread_baton * baton_u = (clog_thread_baton *)  async_u->data;

  if (baton_u->msg_type_y != LOG_MSG_PRECOMMIT){
    fprintf(stderr, "egzh: ERROR: emit stage for event %lli which is not in PRECOMMIT - state machine panic\n", (long long int) baton_u->seq_d);
    exit(-1);
  }

  // emit here
  // NOTFORCHECKIN

  // after success we can log a second time
  
  // this is a big of a hack: we're operating on data that sort of claims to be opaque to us.
  // ...but it's cool because We Know The Truth (tm)
  u2_clpr * header_u = (u2_clpr *) baton_u->bob_y;

  header_u -> msg_type_y = LOG_MSG_POSTCOMMIT;
  baton_u  -> msg_type_y = LOG_MSG_POSTCOMMIT;

  u2_kafk_push(baton_u->bob_y, 
               baton_u->len_w,
               baton_u);

}

// Message push callback
//
//    * gets invoked when our pushed message has been committed to the kafka server
//    * gets invoked in one of kafka's threads, which means we can't touch  loom memory
//
// What we do: 
//        * sanity check
//        * store the kafka offset in u2K.  u2K will get checkpointed as part of the running system,
//          so that on boot later we'll know what the last kafka offset the checkpoint file reflects,
//          and then we can start querying kafka for log entries >> there, to build further state
//
//       
//        
// 
static void _kafka_msg_delivered_cb (rd_kafka_t *rk, 
                                     const rd_kafka_message_t *rkmessage, 
                                     void *opaque) 
{
  // 1) sanity check
  //
  if (rkmessage->err){
    fprintf(stderr, "kafk: CB delivery failure: %s\n", rd_kafka_message_errstr(rkmessage));
    exit(-1);
  }

  clog_thread_baton * baton_u = (clog_thread_baton * ) opaque;


  // 2) store this kafka offset in u2k global
  //
  if (rkmessage->offset > u2K->largest_offset_seen_ds){
    fprintf(stderr, "kafk: OK     CB in order. Old: %lli ; New: %lli \n", (long long int) u2K->largest_offset_seen_ds,  (long long int) rkmessage->offset );      
  } else {
    fprintf(stderr, "kafk: WARN!! CB out of order. Old: %lli ; New: %lli \n", (long long int) u2K->largest_offset_seen_ds, (long long int)  rkmessage->offset );      
  }

  u2K->largest_offset_seen_ds = rkmessage->offset;

  if (LOG_MSG_PRECOMMIT == baton_u -> msg_type_y){
    u2K->largest_offset_seen_precom_ds = rkmessage->offset;
  }

  // 3) bail if done 
  if (LOG_MSG_POSTCOMMIT == baton_u -> msg_type_y){
    printf("kafk: 2nd logging complete for event %lli\n", (long long int) baton_u->seq_d); // NOTFORCHECKIN
    free(baton_u->bob_y);
    free(baton_u);
    return;
  }

  // 4) re-package the data, hand off to libuv worker thread 

  uv_async_t * async_u = malloc(sizeof(uv_async_t));

  c3_w ret_w = uv_async_init(u2_Host.lup_u, 
                             async_u, 
                             &_kafka_finalize_and_emit );
  if (ret_w < 0){
    fprintf(stderr, "kafk: unable to inject event into uv\n");
    exit(-1);
  }
  async_u->data = (void *) baton_u;

  uv_async_send(async_u);
}


// Note:
//    * does NOT write a u2_clpr event header: this just writes bytes
//
// input:
//     * data    -
//     * datalen - 
//     * baton_u - ptr to data block that we'll hand from thread to thread so that we can 
//                 pick up data and emit, finalize later.
// return:
//     * sequence #
//
void u2_kafk_push(c3_y * msg_y, c3_w len_w, clog_thread_baton * baton_u)
{
  if (u2K->inited_t != c3_true){ 
    fprintf(stderr, "kafk: must init first\n"); 
    exit(-1);
  }

  // send the message
  //
  if (rd_kafka_produce(u2K->topic_prod_handle_u,   // topic
                       WRITE_PARTITION,            // partition
                       RD_KAFKA_MSG_F_COPY,        // msg flags
                       msg_y, len_w,               // payload
                       NULL, 0,                    // OPTIONAL: message key
                       (void*) baton_u)            // opaque payload for callback
      == -1) {
    fprintf(stderr,
            "%% Failed to produce to topic %s partition %i: %s\n",
            rd_kafka_topic_name(u2K->topic_prod_handle_u), WRITE_PARTITION,
            rd_kafka_err2str(
                             rd_kafka_errno2err(errno)));
    rd_kafka_poll(u2K->kafka_prod_handle_u, 0);
  }

  // NOTFORCHECKIN - do we want this here?  I think we want to rip it out
  while (rd_kafka_outq_len(u2K->kafka_prod_handle_u) > 0) {
    rd_kafka_poll(u2K->kafka_prod_handle_u, 100);
  }
  

}

// copy-and-paste programming; see also u2_egz_push_ova()
//
// input:
//    * rec_u
//    * ovo
// return:
//    * id of log msg
c3_d
u2_kafk_push_ova(u2_reck* rec_u, u2_noun ovo, c3_y msg_type_y)
{

  c3_w   malloc_w;   // space for header AND payload
  c3_w   len_w;      // length of payload
  c3_y * data_y;     

  // convert into bytes, w some padding up front for the header
  u2_clog_o2b(ovo, & malloc_w, & len_w, & data_y);

  // write the header in place
  c3_d seq_d = u2A->ent_d++;
  u2_clog_write_prefix((u2_clpr *) data_y, seq_d, msg_type_y, len_w, data_y + sizeof(u2_clpr));

  // write a data baton for use in callback, final emit
  clog_thread_baton * baton_u = (clog_thread_baton *) malloc(sizeof(clog_thread_baton));
  baton_u->bob_y         = data_y;
  baton_u->len_w         = len_w + sizeof(u2_clpr);
  baton_u->seq_d         = seq_d;
  baton_u->msg_type_y    = msg_type_y;
  baton_u->ovo           = ovo;
  memset(& baton_u->push_thread_u, 0 , sizeof(uv_thread_t));

  u2_kafk_push( data_y, malloc_w, baton_u);

  return(seq_d);
}

//--------------------
//  read functions
//--------------------


// Prepare for reading.   
// 
//    Call this once, specifying the offset - the KAFKA offset, not the ovum message number!
//
void u2_kafk_pre_read(c3_d offset_d)
{
  if (u2K->inited_t != c3_true){ 
    fprintf(stderr, "kafk: must init first\n"); 
    exit(-1);
  }

  if (rd_kafka_consume_start(u2K->topic_cons_handle_u, 
                             READ_PARTITION,
                             offset_d ) == -1){
    fprintf(stderr, "%% Failed to start consuming: %s\n", rd_kafka_err2str(rd_kafka_errno2err(errno)));
    exit(-1);
  }
}


// Read one kafka message and return the payload and some header info from it.
//
//
// Which one?  You don't get to specify - that falls out from where
// you started the consumption sequence via u2_kafk_pre_read()
//
c3_t u2_kafk_pull_one(c3_d * ent_d,            // return arg
                      c3_y * msg_type_y,       // return arg
                      c3_w * len_w,            // return arg
                      c3_y ** buf_y)           // return arg
            
{
  if (u2K->inited_t != c3_true){ 
    fprintf(stderr, "kafk: must init first\n"); 
    exit(-1);
  }

  rd_kafka_message_t *rkmessage;

  // (1) Consume single message.
  //     See rdkafka_performance.c for high speed consuming of messages. 
  rkmessage = rd_kafka_consume(u2K->topic_cons_handle_u, 
                               READ_PARTITION, 
                               READ_TIMEOUT
                               );
  if (NULL == rkmessage){
    fprintf(stderr, "kafk_read() failed: %s\n", strerror(errno));
    exit(1);
  }

  // (2) check error messages
  if (rkmessage->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    fprintf(stderr, "kafk error: %s\n", rd_kafka_err2str(rkmessage->err));
    return(c3_false);
  }

  // (3) pull out our event prefix
  //
  if (rkmessage->len < sizeof(u2_clpr)){
    fprintf(stderr, "kafk error: too small for event prefix\n");
    return(c3_false);
  }

  u2_clpr * clog_u = (u2_clpr *) rkmessage->payload;
  c3_t ret_t = u2_clog_check_prefix(clog_u);
  if (c3_true != ret_t){
    return(c3_false);
  }

  // (4) pull out our message
  *len_w = clog_u->len_w;
  *buf_y = (c3_y *) malloc(*len_w);
  memcpy(* buf_y, rkmessage->payload + sizeof(u2_clpr), *len_w);
  

  // success: return payload and metadata
  *ent_d            = clog_u->ent_d;
  *msg_type_y       = clog_u->msg_type_y;

  rd_kafka_message_destroy(rkmessage);

  return(c3_true);
}

// Read ova from kafka.
//
// input args:
//    * rec_u -
//    * ohh   - 
// output args:
//    * ohh   - ???
//
// return:
//    * first read noun, which is a cell structure, which means that later we can iterate over it by using
//      u2h() and u2t().  All reading from kafka should be done in this func!
//
c3_t u2_kafk_pull_one_ova(c3_d    * ent_d,  
                          c3_y    * msg_type_y,
                          u2_noun * ovo)
{
  if (u2K->inited_t != c3_true){ 
    fprintf(stderr, "kafk: must init first\n"); 
    exit(-1);
  }

  // 4) pull from log
  c3_y * payload_y;
  c3_w payload_len_w;

  c3_t success=  u2_kafk_pull_one(ent_d,            
                                  msg_type_y, 
                                  & payload_len_w,
                                  & payload_y); 
  if (c3_false == success){
    return(c3_false);
  }

  u2_clog_b2o(payload_len_w, payload_y, ovo);

  return(c3_true);
}

u2_noun  u2_kafk_pull_all(u2_reck* rec_u,  u2_bean *  ohh)
{
  // NOTFORCHECKIN - unimplemented
}

#define U2_KAFK_VERSION 1

void u2_kafk_commit()
{
// NOTFORCHECKIN
}

void u2_kafk_decommit()
{
  // NOTFORCHECKIN
}

//--------------------
//  shutdown functions
//--------------------


void u2_kafka_down()
{
  if (! u2_Host.ops_u.kaf_c) { return ; }

  // Destroy the handles
  rd_kafka_destroy(u2K->kafka_prod_handle_u);
  rd_kafka_destroy(u2K->kafka_cons_handle_u);


  /* Let background threads clean up and terminate cleanly. */
  rd_kafka_wait_destroyed(2000);


}

// admin:
//   convert egz to kafka
void u2_kafka_admin_egz_to_kafka()
{
  fprintf(stderr, "u2_kafka_admin_egz_to_kafka() unimplemented");
  exit(1);
}

// admin:
//   convert kafka to egz
void u2_kafka_admin_kafka_to_egz()
{
  fprintf(stderr, "u2_kafka_admin_kafka_to_egz() unimplemented");
  exit(1);
}

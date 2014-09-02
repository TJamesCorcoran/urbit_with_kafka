// v/egzh.c
//
// This file is in the public domain.
//

// there are two methods of storing events:
//    * egz.hope log file
//    * kafka log servers
// this code implementes function around the former.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int syncfs(int fd);
#include <uv.h>
#include <dirent.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

#include "all.h"
#include "v/vere.h"
#include "v/sist.h"
#include "v/egzh.h"

#if defined(U2_OS_linux)
#include <stdio_ext.h>
#define fpurge(fd) __fpurge(fd)
#define DEVRANDOM "/dev/urandom"
#else
#define DEVRANDOM "/dev/random"
#endif

#define CONSOLIDATOR_SLEEP_SECONDS  10

// forward prototypes
void _egz_push_in_thread(c3_y* bob_y, c3_w len_w, c3_d seq_d, u2_noun ovo, c3_y msg_type_y);

//----------
// linked list to hold names of unconsolidated minifiles
//----------

struct _minilog_node_i;
typedef struct _minilog_node_i { 
  c3_d name_d;
  c3_y msgtype_y;
  struct _minilog_node_i * nex_u;
} _minilog_node;

_minilog_node * head_u;
_minilog_node * tail_u;

uv_mutex_t _queue_mutex_u;

// enque at tail
//
void _enqueue(c3_d new_d, c3_y msgtype_y)
{
  uv_mutex_lock(& _queue_mutex_u);

  // do work inside mutex
  {
    _minilog_node * new_u = (_minilog_node *) malloc(sizeof(_minilog_node));
    new_u->name_d = new_d;
    new_u->msgtype_y = msgtype_y;
    new_u->nex_u = NULL;

    if (tail_u){
      tail_u->nex_u = new_u;
    }
    tail_u = new_u;

    if (NULL == head_u) {
      head_u = new_u;
    }
  }

  uv_mutex_unlock(& _queue_mutex_u);
}

// deque at head
//
c3_t _dequeue(c3_d * ret_d, c3_y * msgtype_y)
{
  c3_t ret_t = c3_false;

  uv_mutex_lock(& _queue_mutex_u);

  // do work inside mutex
  {

    if (NULL == tail_u){
      ret_t = c3_false;
    } else {
      _minilog_node * tmp_u = head_u;
      head_u = head_u->nex_u;
      *ret_d = tmp_u->name_d;
      *msgtype_y = tmp_u->msgtype_y;
      free(tmp_u);
      ret_t = c3_true;

      if (NULL == head_u){
        tail_u = NULL;
      }

    }
  }

  uv_mutex_unlock(& _queue_mutex_u);
  return(ret_t);
}

//----------
// header utilities
//----------

// this should only be used in testing
//
void u2_egz_rm()
{
  // delete egz.hope
  c3_c    ful_c[2048];  
  u2_sist_get_egz_filestr(ful_c, 2048);
  unlink(ful_c);

  // delete all quickfiles
  u2_sist_get_egz_quick_dirstr(ful_c, 2048);
  c3_c    exec_c[2048];
  sprintf(exec_c, "exec rm -rf %s/*", ful_c);
  if (system(exec_c) < 0){
    fprintf(stderr, "u2_egz_rm() failed\n");
    exit(-1);
  }
}

// Create a new egz file w default header
//
// args: 
//   * rec_u - arvo runtine. Prob you want to pass in u2_Host.arv_u
//   * sal_l  - 
//
void u2_egz_write_header(u2_reck* rec_u, c3_l sal_l)
{
  c3_i pig_i = O_CREAT | O_WRONLY | O_EXCL;
#ifdef O_DSYNC
  pig_i |= O_DSYNC;
#endif

  c3_c    ful_c[2048];  
  u2_sist_get_egz_filestr(ful_c, 2048);
  struct stat buf_b;
  c3_i        fid_i;

  if ( ((fid_i = open(ful_c, pig_i, 0600)) < 0) ||
       (fstat(fid_i, &buf_b) < 0) )
    {
      uL(fprintf(uH, "can't create record (%s)\n", ful_c));
      u2_lo_bail(rec_u);
    }
#ifdef F_NOCACHE
  if ( -1 == fcntl(fid_i, F_NOCACHE, 1) ) {
    uL(fprintf(uH, "zest: can't uncache %s: %s\n", ful_c, strerror(errno)));
    u2_lo_bail(rec_u);
  }
#endif

  u2R->lug_u.w_fid_i = fid_i;
  
  u2_eghd led_u;

  led_u.mag_l = u2_mug('h');
  led_u.kno_w = rec_u->kno_w;

  if ( 0 == rec_u->key ) {
    led_u.key_l = 0;
  } else {
    led_u.key_l = u2_mug(rec_u->key);

    c3_assert(!(led_u.key_l >> 31));
  }
  led_u.sal_l = sal_l;
  led_u.sev_l = rec_u->sev_l;
  led_u.tno_l = 1;  // egg count init to 1

  c3_w        size_w =  write(fid_i, &led_u, sizeof(led_u)); 

  if ( sizeof(led_u) != size_w ) {
    uL(fprintf(uH, "can't write record (%s)\n", ful_c));
    u2_lo_bail(rec_u);
  }

  u2R->lug_u.len_d = c3_wiseof(led_u);

  close(fid_i); 
  syncfs(fid_i); 
}

// * update in memory structure. 
// * NO NOT REALLY // rewrite egz file header
// 
void _u2_egz_note_larger_size(c3_d new_bytes_d)
{
  // modify memory structure
  u2R->lug_u.len_d += new_bytes_d;

//  // read, modify, write existing log header
//  //
//  u2_eghd new_header_u;
//
//  if  (lseek64(fid_i, 0, SEEK_SET) < 0){
//    uL(fprintf(uH, "egzh: tno lseek fail\n"));
//    exit(-1);
//  }
//
//  if (sizeof(u2_eghd) != read(fid_i, & new_header_u, sizeof(u2_eghd))){
//    uL(fprintf(uH, "egzh: tno read fail\n"));
//    exit(-1);
//  }
//
//  new_header_u.tno_l = tno_l;
//
//  if (sizeof(u2_eghd) != write(fid_i, & new_header_u, sizeof(u2_eghd))){
//    uL(fprintf(uH, "egzh: tno write fail\n"));
//    exit(-1);
//  }

}

// void u2_egz_rewrite_header(u2_reck* rec_u, c3_i fid_i, u2_bean ohh, u2_eghd * led_u)
// {
//   //  Increment sequence numbers. New logs start at 1.
//   if ( u2_yes == ohh ) {
//     uL(fprintf(uH, "egzh: bumping ent_d, don't panic.\n"));
//     u2_clpr lar_u;
//     c3_d    end_d;
//     c3_d    tar_d;
// 
//     rec_u->ent_d++;
//     end_d = u2R->lug_u.len_d;
//     while ( end_d != c3_wiseof(u2_eghd) ) {
//       tar_d = end_d - c3_wiseof(u2_clpr);
//       if ( -1 == lseek64(fid_i, 4ULL * tar_d, SEEK_SET) ) {
//         uL(fprintf(uH, "bumping sequence numbers failed (a)\n"));
//         u2_lo_bail(rec_u);
//       }
//       if ( sizeof(lar_u) != read(fid_i, &lar_u, sizeof(lar_u)) ) {
//         uL(fprintf(uH, "bumping sequence numbers failed (b)\n"));
//         u2_lo_bail(rec_u);
//       }
//       lar_u.ent_d++;
//       if ( -1 == lseek64(fid_i, 4ULL * tar_d, SEEK_SET) ) {
//         uL(fprintf(uH, "bumping sequence numbers failed (c)\n"));
//         u2_lo_bail(rec_u);
//       }
//       if ( sizeof(lar_u) != write(fid_i, &lar_u, sizeof(lar_u)) ) {
//         uL(fprintf(uH, "bumping sequence numbers failed (d)\n"));
//         u2_lo_bail(rec_u);
//       }
//       end_d = tar_d - lar_u.len_w;
//     }
//   }
// 
//   led_u->mag_l = u2_mug('h');
//   // led_u->sal_l stays the same
//   led_u->sev_l = rec_u->sev_l;
//   led_u->key_l = rec_u->key ? u2_mug(rec_u->key) : 0;
//   led_u->kno_w = rec_u->kno_w;         //  may need actual translation!
//   led_u->tno_l = 1;                    // why sequence # 1 ?
// 
//   if ( (-1 == lseek64(fid_i, 0, SEEK_SET)) ||
//        (sizeof(led_u) != write(fid_i, led_u, sizeof(u2_eghd))) )
//     {
//       uL(fprintf(uH, "record failed to rewrite\n"));
//       u2_lo_bail(rec_u);
//     }
// }


//----------
// read
//----------

// open the egz.hope file
//   * sanity check:
//        * egz exists
//        * is not corrupt,
//        * arvo kernel versions match
//        * etc
//   * store details
//        * egz file handle in u2R->lug_u
//        * tno_l (msg number) in 
c3_t u2_egz_open(u2_reck* rec_u, c3_i * fid_i, u2_eghd * led_u)
{
  struct stat buf_b;
  u2_noun     sev_l; // , sal_l, key_l, tno_l;

  c3_i pig_i = O_RDWR;
#ifdef O_DSYNC
  pig_i |= O_DSYNC;
#endif

  c3_c        ful_c[2048];
  u2_sist_get_egz_filestr(ful_c, 2048);

  if ( ((*fid_i = open(ful_c, pig_i)) < 0) || (fstat(*fid_i, &buf_b) < 0) ) {
    uL(fprintf(uH, "egzh: can't open record (%s)\n", ful_c));
    u2_lo_bail(rec_u);

    return c3_false;
  }
#ifdef F_NOCACHE
  if ( -1 == fcntl(*fid_i, F_NOCACHE, 1) ) {
    uL(fprintf(uH, "egzh: can't uncache %s: %s\n", ful_c, strerror(errno)));
    u2_lo_bail(rec_u);

    return c3_false;
  }
#endif
  u2R->lug_u.w_fid_i = *fid_i;
  u2R->lug_u.len_d = ((buf_b.st_size + 3ULL) >> 2ULL);

  c3_w        size_w;

  size_w = read(*fid_i, led_u, sizeof(*led_u));
  if ( sizeof(*led_u) != size_w)  {
    fprintf(stderr, "egzh: record is corrupt (a)\n");
    u2_lo_bail(rec_u);
  }

  if (u2_mug('h') != led_u->mag_l ) {
    fprintf(stderr, "egzh: record is obsolete or corrupt\n");
    u2_lo_bail(rec_u);
  }

  if ( led_u->kno_w != rec_u->kno_w ) {
    //  XX perhaps we should actually do something here
    //
    fprintf(stderr, "egzh: (not) translating events (old %d, now %d)\n",
               led_u->kno_w,
               rec_u->kno_w);
  }

  sev_l = led_u->sev_l;
  /* sal_l = led_u->sal_l; */
  /* key_l = led_u->key_l; */
  /* tno_l = led_u->tno_l; */

  {
    u2_noun old = u2_dc("scot", c3__uv, sev_l);
    u2_noun nuu = u2_dc("scot", c3__uv, rec_u->sev_l);
    c3_c* old_c = u2_cr_string(old);
    c3_c* nuu_c = u2_cr_string(nuu);

    uL(fprintf(uH, "egzh: old %s, new %s\n", old_c, nuu_c));
    free(old_c); free(nuu_c);

    u2z(old); u2z(nuu);
  }
  c3_assert(sev_l != rec_u->sev_l);   //  1 in 2 billion, just retry


  fprintf(stderr, "egzh: opened without error\n");

  //  check passcode
  // NOTFORCHECKIN  _sist_passcode(rec_u, sal_l);

  return c3_true;
}

// 
//
void u2_egz_pull_start()
{
  if (u2R->lug_u.r_fid_i){
    if (0 != close(u2R->lug_u.r_fid_i)){
      uL(fprintf(uH, "u2_egz_pull_start() failed - close\n"));
      exit(1);
    }
    u2R->lug_u.r_fid_i = 0;
  }

  c3_c    egz_c[2048];  
  u2_sist_get_egz_filestr(egz_c, 2048);

  u2R->lug_u.r_fid_i = open(egz_c, O_RDONLY, 0600);
  if (u2R->lug_u.r_fid_i < 0){
    fprintf(stderr, "u2_egz_pull_start() failed - open - %s\n", strerror(errno));
    exit(1);
  }

  u2_eghd fileheader_u;
  if ( sizeof(u2_eghd) != read(u2R->lug_u.r_fid_i, &fileheader_u, sizeof(u2_eghd)) ) {
    fprintf(stderr, "u2_egz_pull_start() failed - read header\n");
    exit(1);
  }

  if (fileheader_u.mag_l != u2_mug('h')){
    fprintf(stderr, "u2_egz_pull_start() failed - expecting log format 'h' \n");
    exit(1);
  }

}


//
//
c3_t u2_egz_pull_one(c3_d *  ent_d,                         //  event sequence number
                     c3_y *  msg_type_y,                    //  0 = precommit; 1 = commit
                     c3_w *  len_w,                         //  word length of this event
                     c3_y ** data_y)                         //  data
{
  u2_clpr eventprefix_u;

  // read the per-event prefix
  if ( sizeof(u2_clpr) != read(u2R->lug_u.r_fid_i, &eventprefix_u, sizeof(u2_clpr)) ) {  
    fprintf(stderr, "u2_egz_pull_one() failed - read prefix\n");
    return(c3_false);
  }

  *msg_type_y = eventprefix_u.msg_type_y;
  *ent_d      = eventprefix_u.ent_d;
  *len_w      = eventprefix_u.len_w;

  // now that we know the size, read the actual log msg
  *data_y = (c3_y *) malloc(eventprefix_u.len_w);
  
  if ( *len_w != read(u2R->lug_u.r_fid_i, *data_y, *len_w) ) {  
    fprintf(stderr, "u2_egz_pull_one() failed - read message\n");
    return(c3_false);
  }

  return(c3_true);
}

c3_t u2_egz_pull_one_ova(c3_d *  ent_d,
                         c3_y *  msg_type_y,
                         u2_noun * ovo)
{

  c3_w   len_w;
  c3_y * data_y;

  c3_t ret_t = u2_egz_pull_one(ent_d, msg_type_y, & len_w,  & data_y );
  if (c3_true != ret_t){ return(c3_false);   }
  u2_clog_b2o(len_w, data_y, ovo);

  return(c3_true);
}

u2_noun u2_egz_pull_all(    u2_reck* rec_u,  c3_i fid_i,  u2_bean *  ohh)
{
  // NOTFORCHECKIN - unimplemented
}


// Read from the file. Return one noun which is event number of first item.
//
// input args:
//    * rec_u -
//    * fid_i - file handle of already open egz.hope
//    * ohh   - ??
// output args:
//    * ohh   - ???
//
// return:
//    * first read noun, which is a cell structure, which means that later we can iterate over it by using
//      u2h() and u2t().  All reading from  disk should be done in this func!

u2_noun u2_egz_read_all(u2_reck* rec_u, c3_i fid_i,   u2_bean *  ohh)
{

  c3_d    end_d;
  c3_d    las_d = 0;
  c3_d    old_d = rec_u->ent_d;



  end_d = u2R->lug_u.len_d;

  // u2R->lug_u.len_d points us to the end of the file THAT WE HAVE PROCESSED.
  //
  // Seek to there; new data is beyond that point.
  //
  if ( -1 == lseek64(fid_i, 4ULL * end_d, SEEK_SET) ) {
    fprintf(stderr, "end_d %llu\n", (long long int) end_d);
    perror("lseek");
    uL(fprintf(uH, "record is corrupt (c)\n"));
    u2_lo_bail(rec_u);
  }

  while ( end_d != c3_wiseof(u2_eghd) ) {

    // NOTFORCHECKIN - NOT FOR CHECKIN - RIPPED OUT GUTS - BROKEN

  }
  rec_u->ent_d = c3_max(las_d + 1ULL, old_d);

  exit(-1); // NOTFORCHECKIN - unimplemented!
  u2_noun uglyhack_u;
  return( uglyhack_u);
}

//----------
// write
//----------

// Quickly write bytes to an egz mini file.
//
// Note:
//    * we will consolidate these mini files later in the consolidator thread.
//    * does NOT write a u2_clpr event header: this just writes bytes
// 
// inputs:
//   * raf_u - pointer to raft state structure (pass in global 'u2A')
//   * data_y - msg
//   * len_y - msg len
//   * seq_d - sequence number of this msg (so we can store the minifile to disk & collect it later)
//   * _d - suffix of msg (clog.h precommit / postcommit flag) for same reasons
void
u2_egz_push(c3_y* data_y, c3_w len_w, c3_d seq_d, c3_y msgtype_y)
{
  c3_assert(0 != data_y && 0 < len_w);
  
  // get file name
  c3_c bas_c[2048];
  u2_sist_get_egz_quick_filestr(bas_c, 2048, seq_d, msgtype_y);

  // open, write, close file
  c3_i fid_i = open(bas_c, O_CREAT | O_TRUNC | O_WRONLY, 0600);
  if (fid_i < 0){
    fprintf(stderr, "egz_push() failed - fopen - %s\n", strerror(errno));
    exit(1);
  }
  if (write(fid_i, data_y, len_w) < len_w){
    fprintf(stderr, "egz_push() failed - write - %s\n", strerror(errno));
    exit(1);
  }
  if (syncfs(fid_i) < 0){
    uL(fprintf(uH, "egz_push() failed - sync\n"));
    exit(1);
  }
  if (0 != close(fid_i)){
    uL(fprintf(uH, "egz_push() failed - close\n"));
    exit(1);
  }

  // make note to consolidate this file later
  _enqueue(seq_d, msgtype_y);



}



// Q: what happens when we've written the minifile?
//
// A: we end up back here, in the main libuv thread.  And have access to loom.
//
void _egz_finalize_and_emit(uv_async_t* async_u, int status_w)
{
  clog_thread_baton *  clog_baton_u = (clog_thread_baton *) async_u->data;
  

  if (clog_baton_u->msg_type_y != LOG_MSG_PRECOMMIT){
    fprintf(stderr, "egzh: ERROR: emit stage for event %lli which is not in PRECOMMIT - state machine panic\n", (long long int) clog_baton_u->seq_d);
    exit(-1);
  }

  // emit here
  // NOTFORCHECKIN

  // after success we can log a second time
  
  // this is a big of a hack: we're operating on data that sort of claims to be opaque to us.
  // ...but it's cool because We Know The Truth (tm)
  u2_clpr * header_u = (u2_clpr *) clog_baton_u->bob_y;
  header_u -> msg_type_y = LOG_MSG_POSTCOMMIT;

  _egz_push_in_thread(clog_baton_u->bob_y, 
                      clog_baton_u->len_w,
                      clog_baton_u->seq_d,
                      clog_baton_u->ovo,
                      LOG_MSG_POSTCOMMIT);

  free(clog_baton_u); // NOTFORCHECKIN is this correct?
  free(async_u);
}



// we are now in a short lived special purpose thread. 
//
void
_egz_push_in_thread_inner(void * raw_u)
{
  clog_thread_baton * clog_baton_u = (clog_thread_baton *) raw_u;

  // push data. Might take a long time bc of flush()
  //
  u2_egz_push(clog_baton_u->bob_y, 
              clog_baton_u->len_w, 
              clog_baton_u->seq_d,
              clog_baton_u->msg_type_y);

  // if this is a precommit, there's more to be done (emit side effects and log again)
  // if it's a post commit, there's not
  if (LOG_MSG_POSTCOMMIT == clog_baton_u -> msg_type_y){
    printf("egzh: 2nd logging complete for event %lli\n", (long long int) clog_baton_u->seq_d); // NOTFORCHECKIN

    // this is the second and final time we log this message, therefore we're done w the space
    //
    free(clog_baton_u->bob_y);

  } else {

    //
    // We're in the child thread, but we want to get back into the
    // libuv worker thread where we have loom access.
    // How do we inject work into libuv worker?
    //
    // (1) set up an async handler, which means that ANY TIME someone
    //     sends something to that handler, the CB gets called in the main
    //     thread.
    // (2) send to that main thread.
    //
    // Q: surely you want a GLOBAL async handler, right?
    // A: that sounds good, but, no.  UV is crazy.  Multiple calls to
    //    uv_async_send() are not guaranteed to result in multiple calls to the callback.
    //    Also, data we stash in the handle can be overwritten. So we either do 
    //    elaborate mutex / linked list work, or we just go ghetto and create a whole new async 
    //    setup for each and every ova.  We do the latter.

    uv_async_t * async_u = malloc(sizeof(uv_async_t));

    c3_w ret_w = uv_async_init(u2_Host.lup_u, 
                               async_u, 
                               &_egz_finalize_and_emit );
    if (ret_w < 0){
      fprintf(stderr, "kafk: unable to inject event into uv\n");
      exit(-1);
    }


    // the data pack we created in libuv thread made it here to child
    // thread...and now will go back to libuv thread.
    async_u->data = (void *) clog_baton_u;

    // pull the trigger
    uv_async_send(async_u);
  }
  // nothing more to be done in this thread
}

void
_egz_push_in_thread(c3_y* bob_y, c3_w len_w, c3_d seq_d, u2_noun ovo, c3_y msg_type_y)
{
  
  clog_thread_baton *  clog_baton_u = (clog_thread_baton *) malloc(sizeof(clog_thread_baton));
  clog_baton_u->bob_y = bob_y;
  clog_baton_u->len_w = len_w;
  clog_baton_u->seq_d = seq_d;
  clog_baton_u->ovo   = ovo;
  clog_baton_u->msg_type_y = msg_type_y;

  if (uv_thread_create(& clog_baton_u->push_thread_u,
                       & _egz_push_in_thread_inner,
                       clog_baton_u) < 0){
    fprintf(stderr, "egzh: unable to spawn thread for push\n");
    exit(-1);
  }

}


// copy-and-paste programming; see also u2_kafka_push_ova()
//
// input:
//    * rec_u
//    * ovo
//    * msg type - { LOG_MSG_PRECOMMIT | LOG_MSG_POSTCOMMIT }
// return:
//    * id of log msg
c3_d
u2_egz_push_ova(u2_reck* rec_u, u2_noun ovo, c3_y msg_type_y)
{
  c3_w len_w;
  c3_w malloc_w;
  c3_y * data_y;

  // 1) convert noun into bytes - pad begining for header
  //
  u2_clog_o2b(ovo, &malloc_w, & len_w, & data_y);

  // 2) write prefix into padding
  u2_clpr * eventprefix_u = (u2_clpr *) data_y;
  c3_d seq_d = u2A->ent_d++;
  eventprefix_u->ent_d = seq_d;
  eventprefix_u->len_w = len_w;
  eventprefix_u->msg_type_y = msg_type_y;

  // 3) log
  _egz_push_in_thread(data_y, malloc_w, seq_d, ovo, msg_type_y );

  return(seq_d);
}


// run forever:
//     1) sleep
//     2) wake
//     3) find 0+ tasks in the egz work queue. 
//         3.1) open specified minifile
//         3.2) read it
//         3.3) append its contents to the egz file
//     4) rewrite egz.hope header to reflect new length
//     5) repeat
//
// inputs:
//    none - (ignore *arg; it's supplied by the general purpose thread fork)
// outputs:
//    none - should never return
//
void _egz_consolidator(void *arg)
{
  fprintf(stdout, "egzh: consolidator live\n");

  while(1) {
    sleep(CONSOLIDATOR_SLEEP_SECONDS);

    fprintf(stdout, "egzh: consolidator awake\n");
    c3_d count_d = 0;

    c3_d eventnum_d;
    c3_y msgtype_y;

    c3_i egzfid_i = 0;
    c3_c egzfile_c[2048];

    c3_d new_bytes_d = 0;

    // deque() will return true right up until there's no more work to do
    while (c3_true == _dequeue(& eventnum_d, &msgtype_y)){
      count_d ++;

      // 0) open egz file just once per waking cycle 
      //    (this is inside the loop but happens just once)
      if (0 == egzfid_i){
        u2_sist_get_egz_filestr(egzfile_c, 2048);
        egzfid_i = open(egzfile_c, O_WRONLY | O_APPEND, 0600);
        if (-1 ==egzfid_i){
          fprintf(stderr, "FATAL: consolidator couldn't read egz %s\n", egzfile_c);
          exit(-1);
        }

        //         if (off_ds < 0){
        //           fprintf(stderr, "FATAL: consolidator couldn't lseek egz %s\n", egzfile_c);
        //           exit(-1);
        //         }
        //         if (off_ds < sizeof(u2_eghd)){
        //           fprintf(stderr, "FATAL: consolidator lseek suggests no header %s ; %lli\n", egzfile_c, (long long int) off_ds);
        //           exit(-1);
        //         }
        //         printf("consolidator egz.hope size = %lli\n", (long long int) off_ds); // NOTFORCHECKIN
      }

      // 1) open minifile
      c3_c minifile_c[2048];
      u2_sist_get_egz_quick_filestr(minifile_c, 2048, eventnum_d, msgtype_y);
      c3_i minifid_i = open(minifile_c, O_RDONLY, 0400);
      if (-1 == minifid_i){
        fprintf(stderr, "FATAL: consolidator couldn't open minifile %s\n", minifile_c);
        exit(-1);
      }
      
      // 2) read minifile

      // 2.1) header first
      u2_clpr eventheader_u;
      c3_w eventheadersize_w = read(minifid_i, &eventheader_u, sizeof(eventheader_u));
      if (eventheadersize_w < sizeof(u2_clpr)){
        fprintf(stderr, "FATAL: consolidator couldn't read minifile eventheader %s\n", minifile_c);
        exit(-1);
      }
      c3_w totalsize_w = eventheader_u.len_w + sizeof(u2_clpr);

      // 2.2) seek back a few bytes then read for real
      if  (lseek64(minifid_i, -1 * sizeof(u2_clpr) , SEEK_CUR) < 0){
        fprintf(stderr, "FATAL: consolidator couldn't backup\n");
        exit(-1);
      }
      c3_y*   fullentry_y = (c3_y*) c3_malloc(totalsize_w);
      if (read(minifid_i, fullentry_y, totalsize_w) < totalsize_w) {
        fprintf(stderr, "FATAL: consolidator read insufficient bytes from minifile %s\n", minifile_c);
        exit(-1);
      }

      // 3.0) append data to egz
      //      note: we do not build a per-entry event prefix, bc it already exists
      c3_w ret_w = write(egzfid_i, fullentry_y, totalsize_w); 
      if (ret_w < 0){
        fprintf(stderr, "FATAL: consolidator couldn't append to egz %s\n", egzfile_c);
        exit(-1);
      }

      new_bytes_d += totalsize_w;

      // 4) close minifile
      ret_w = close(minifid_i); 
      if (ret_w < 0){
        fprintf(stderr, "FATAL: consolidator couldn't close minifile %s\n", minifile_c);
        exit(-1);
      }

      // 5) delete minifile
      ret_w =  unlink(minifile_c);
      if (ret_w < 0){
        fprintf(stderr, "FATAL: consolidator couldn't delete minifile %s\n", minifile_c);
        exit(-1);
      }

    } // while _dequeue

    // 5) close egz file just once per waking cycle
    if (0 != syncfs(egzfid_i)){
      fprintf(stderr, "FATAL: consolidator couldn't syncfs egzfile: %s\n", strerror(errno));
      exit(-1);
    }
    if (0 != egzfid_i){
      c3_w ret_w = close(egzfid_i); 
      if (ret_w < 0){
        fprintf(stderr, "FATAL: consolidator couldn't close egzfile\n");
        exit(-1);
      }
    }

    // 6) note that egz has grown
    _u2_egz_note_larger_size(new_bytes_d);


    fprintf(stdout, "egzh: consolidator sleeping after doing %lli tasks\n", (long long int) count_d);
  } // while(1)
}

void u2_egz_init()
{
  fprintf(stdout, "egzh: egz.hope logging begin\n");


  uv_mutex_init(& _queue_mutex_u);
  head_u = NULL;
  tail_u = NULL;

  // note that this new thread is not the libuv event loop and has nothing to do w the libuv event loop
  uv_thread_create(& u2R->lug_u.egz_consolidator_thread_u,
                   & _egz_consolidator,
                   NULL);

}

void u2_egz_shutdown()
{
  fprintf(stdout, "egzh: egz.hope shutdown\n");

  uv_thread_join(& u2R->lug_u.egz_consolidator_thread_u);
}


u2_reck *  u2_egz_util_get_u2a()
{
  return(u2A);
}

u2_kafk *  u2_egz_util_get_u2k()
{
  return(u2K);
}

u2_raft *  u2_egz_util_get_u2r()
{
  return(u2R);
}

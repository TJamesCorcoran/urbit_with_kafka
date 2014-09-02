// u2_kafk: kafka / egz.hope state
//

#include "rdkafka.h"  /* for Kafka driver */
#include "clog.h"

// init
void u2_kafk_init(void);

// push (write)
void u2_kafk_push(c3_y *   raw_y, c3_w kafk_len_w, clog_thread_baton * baton_u);
c3_d u2_kafk_push_ova(u2_reck* rec_u, u2_noun ovo, c3_y msg_type_y);


// pull (read)
void u2_kafk_pre_read(    c3_d  start_offset_c);               
c3_t u2_kafk_pull_one(    c3_d * ent_d, c3_y * msg_type_y, c3_w * len_w, c3_y ** buf_y); 
c3_t u2_kafk_pull_one_ova(c3_d * ent_d, c3_y * msg_type_y, u2_noun * ovo);
u2_noun  u2_kafk_pull_all(u2_reck* rec_u,  u2_bean *  ohh);


// shutdown
void u2_kafka_down(void);

// admin
void u2_kafka_admin_kafka_to_egz();
void u2_kafka_admin_egz_to_kafka();

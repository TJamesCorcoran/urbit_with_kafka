#include <sys/stat.h>
#include <sys/types.h>
#include "clog.h"

// u2_eghd: logfile header (1 at head of file)
//
typedef struct {
  c3_l mag_l;                         //  mug of log format, 'a', 'b'...
  c3_w kno_w;                         //  kernel number validated with
  c3_l sal_l;                         //  salt for passcode
  c3_l key_l;                         //  mug of crypto key, or 0
  c3_l sev_l;                         //  host process identity
  c3_l tno_l;                         //  terminal count in host
} u2_eghd;


// init, admin
void     u2_egz_init();
void     u2_egz_rm();   // for testing only!
void     u2_egz_write_header(u2_reck* rec_u, c3_l sal_l);
void     u2_egz_rewrite_header(u2_reck* rec_u,  c3_i fid_i, u2_bean ohh, u2_eghd * led_u);
c3_t     u2_egz_open(u2_reck* rec_u, c3_i * fid_i, u2_eghd * led_u);



// push (write)
void     u2_egz_push(c3_y* bob_y, c3_w len_w, c3_d seq_d, c3_y msgtype_y);
c3_d     u2_egz_push_ova(u2_reck* rec_u, u2_noun ovo, c3_y msg_type_y);

// pull (read)
void    u2_egz_pull_start();
c3_t    u2_egz_pull_one(    c3_d * ent_d, c3_y *  msg_type_y, c3_w *  len_w, c3_y ** bob_y );
c3_t    u2_egz_pull_one_ova(c3_d * ent_d, c3_y *  msg_type_y, u2_noun * ovo);
u2_noun u2_egz_pull_all(    u2_reck* rec_u,  c3_i fid_i,  u2_bean *  ohh);

// shutdown
void u2_egz_shutdown();

// util
u2_reck *  u2_egz_util_get_u2a();
u2_kafk *  u2_egz_util_get_u2k();

// Common Log tools

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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


// ovum -> bytes
//
// input:
//    * ovo
// output:
//    * len
//    * data - note that there's space at the front for 
void u2_clog_o2b(u2_noun ovo,
                 c3_w *  malloc_w, 
                 c3_w *  len_w, 
                 c3_y ** data_y)
{
  // 1) serialize - MAIN THREAD ONLY
  u2_noun ron = u2_cke_jam(ovo);

  // 2) encrypt  - MAIN THREAD ONLY
  c3_assert(u2_Host.arv_u->key);
  // NOTFORCHECKIN ron = u2_dc("en:crua", u2_Host.arv_u->key, ron);

  // 3) copy noun -> buffer 
  *len_w  = u2_cr_met(5, ron) * 4;
  *malloc_w  = (u2_cr_met(5, ron) * 4) + sizeof(u2_clpr);

  *data_y  = c3_malloc(*malloc_w);
  if (data_y == NULL){
    fprintf(stderr, "malloc failure: %s\n", strerror(errno));
    exit(1);
  }
  memset(*data_y, 0, *malloc_w); // debug

  u2_cr_bytes(0, *len_w, *data_y + sizeof(u2_clpr), ron);  // copy len_w bytes of ron into bob_w

  // release storage
  u2z(ron);  

}

// bytes -> ovum
//
// input:
//    * len  - len of raw ovum data ; DO NOT INCLUDE EVENT HEADER
//    * data -        raw ovum data ; DO NOT INCLUDE EVENT HEADER
// output:
//    * ovo
void u2_clog_b2o(c3_w   len_w, 
                 c3_y * data_y,
                 u2_noun * ovo)
{

  // 3) turn bytes ->noun
  u2_noun ron;

  ron = u2_ci_bytes(len_w, data_y);  

  // 2) decrypt
  c3_assert(u2_Host.arv_u->key);
  // NOTFORCHECKIN ron = u2_dc("de:crua", u2_Host.arv_u->key, ron);  

  // 1) deserialize
  *ovo = u2_cke_cue(ron);
}

void u2_clog_write_prefix(u2_clpr * prefix_u, c3_d ent_d, c3_y msg_type_y, c3_w len_w, c3_y * data_y)
{
  memset(prefix_u, 0, sizeof(u2_clpr));

  // prefix_u->syn_w = ???
  prefix_u->ent_d      = ent_d;
  prefix_u->len_w      = len_w;
  // prefix_u->mug_w   = ???;                         //  mug of entry
  prefix_u->msg_type_y = msg_type_y;
  prefix_u->ver_c      = 'h';
}

c3_t u2_clog_check_prefix(u2_clpr * prefix_u)
{
  if (prefix_u->ver_c != 'h'){
    fprintf(stderr, "kafk: prefix version != h");
    return(c3_false);
  }
  if ((prefix_u->msg_type_y != LOG_MSG_PRECOMMIT) &&
      (prefix_u->msg_type_y != LOG_MSG_POSTCOMMIT)) {
    fprintf(stderr, "kafk: illegal precommit / postcommit type");
    return(c3_false);
  }
  return(c3_true);
}

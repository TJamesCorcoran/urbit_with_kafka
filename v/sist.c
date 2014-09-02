/* v/sist.c
**
** This file is in the public domain.
*/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

#include "all.h"
#include "v/vere.h"
#include "v/sist.h"
#include "v/egzh.h"
#include "v/kafk.h"

#if defined(U2_OS_linux)
#include <stdio_ext.h>
#define fpurge(fd) __fpurge(fd)
#define DEVRANDOM "/dev/urandom"
#else
#define DEVRANDOM "/dev/random"
#endif



/* u2_sist_put(): moronic key-value store put.
*/
void
u2_sist_put(const c3_c* key_c, const c3_y* val_y, size_t siz_i)
{
  c3_c ful_c[2048];
  c3_i ret_i;
  c3_i fid_i;

  c3_c bas_c[2048];
  u2_sist_get_pier_dirstr(bas_c, 2048);
  ret_i = snprintf(ful_c, 2048, "%s/.urb/sis/_%s", bas_c, key_c);
  c3_assert(ret_i < 2048);

  if ( (fid_i = open(ful_c, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0 ) {
    uL(fprintf(uH, "sist: could not put %s\n", key_c));
    perror("open");
    u2_lo_bail(u2A);
  }
  if ( (ret_i = write(fid_i, val_y, siz_i)) != siz_i ) {
    uL(fprintf(uH, "sist: could not write %s\n", key_c));
    if ( ret_i < 0 ) {
      perror("write");
    }
    u2_lo_bail(u2A);
  }
  ret_i = c3_sync(fid_i);
  if ( ret_i < 0 ) {
    perror("sync");
  }
  ret_i = close(fid_i);
  c3_assert(0 == ret_i);
}

/* u2_sist_has(): moronic key-value store existence check.
*/
ssize_t
u2_sist_has(const c3_c* key_c)
{
  c3_c        ful_c[2048];
  c3_i        ret_i;
  struct stat sat_u;

  c3_c bas_c[2048];
  u2_sist_get_pier_dirstr(bas_c, 2048);
  ret_i = snprintf(ful_c, 2048, "%s/.urb/sis/_%s", bas_c, key_c);
  c3_assert(ret_i < 2048);

  if ( (ret_i = stat(ful_c, &sat_u)) < 0 ) {
    if ( errno == ENOENT ) {
      return -1;
    }
    else {
      uL(fprintf(uH, "sist: could not stat %s\n", key_c));
      perror("stat");
      u2_lo_bail(u2A);
    }
  }
  else {
    return sat_u.st_size;
  }
  c3_assert(!"not reached");
}

/* u2_sist_get(): moronic key-value store get.
*/
void
u2_sist_get(const c3_c* key_c, c3_y* val_y)
{
  c3_c        ful_c[2048];
  c3_i        ret_i;
  c3_i        fid_i;
  struct stat sat_u;

  c3_c bas_c[2048];
  u2_sist_get_pier_dirstr(bas_c, 2048);
  ret_i = snprintf(ful_c, 2048, "%s/.urb/sis/_%s", bas_c, key_c);
  c3_assert(ret_i < 2048);

  if ( (fid_i = open(ful_c, O_RDONLY)) < 0 ) {
    uL(fprintf(uH, "sist: could not get %s\n", key_c));
    perror("open");
    u2_lo_bail(u2A);
  }
  if ( (ret_i = fstat(fid_i, &sat_u)) < 0 ) {
    uL(fprintf(uH, "sist: could not stat %s\n", key_c));
    perror("fstat");
    u2_lo_bail(u2A);
  }
  if ( (ret_i = read(fid_i, val_y, sat_u.st_size)) != sat_u.st_size ) {
    uL(fprintf(uH, "sist: could not read %s\n", key_c));
    if ( ret_i < 0 ) {
      perror("read");
    }
    u2_lo_bail(u2A);
  }
  ret_i = close(fid_i);
  c3_assert(0 == ret_i);
}

/* u2_sist_nil(): moronic key-value store rm.
*/
void
u2_sist_nil(const c3_c* key_c)
{
  c3_c ful_c[2048];
  c3_i ret_i;

  c3_c bas_c[2048];
  u2_sist_get_pier_dirstr(bas_c, 2048);
  ret_i = snprintf(ful_c, 2048, "%s/.urb/sis/_%s", bas_c, key_c);
  c3_assert(ret_i < 2048);

  if ( (ret_i = unlink(ful_c)) < 0 ) {
    if ( errno == ENOENT ) {
      return;
    }
    else {
      uL(fprintf(uH, "sist: could not unlink %s\n", key_c));
      perror("unlink");
      u2_lo_bail(u2A);
    }
  }
}

/* _sist_suck(): past failure.
*/
static void
_sist_suck(u2_reck* rec_u, u2_noun ovo, u2_noun gon)
{
  uL(fprintf(uH, "sing: ovum failed!\n"));
  {
    c3_c* hed_c = u2_cr_string(u2h(u2t(ovo)));

    uL(fprintf(uH, "fail %s\n", hed_c));
    free(hed_c);
  }

  u2_lo_punt(2, u2_ckb_flop(u2k(u2t(gon))));
  u2_loom_exit();
  u2_lo_exit();

  exit(1);
}

/* _sist_sing(): replay ovum from the past, time already set.
*/
static void
_sist_sing(u2_reck* rec_u, u2_noun ovo)
{
  u2_noun gon = u2_lo_soft(rec_u, 0, u2_reck_poke, u2k(ovo));

  if ( u2_blip != u2h(gon) ) {
    _sist_suck(rec_u, ovo, gon);
  }
  else {
    u2_noun vir = u2k(u2h(u2t(gon)));
    u2_noun cor = u2k(u2t(u2t(gon)));
    u2_noun nug;

    u2z(gon);
    nug = u2_reck_nick(rec_u, vir, cor);

    if ( u2_blip != u2h(nug) ) {
      _sist_suck(rec_u, ovo, nug);
    }
    else {
      vir = u2h(u2t(nug));
      cor = u2k(u2t(u2t(nug)));

      while ( u2_nul != vir ) {
        u2_noun fex = u2h(vir);
        u2_noun fav = u2t(fex);

        if ( (c3__init == u2h(fav)) || (c3__inuk == u2h(fav)) ) {
          rec_u->own = u2nc(u2k(u2t(fav)), rec_u->own);
        }
        vir = u2t(vir);
      }
      u2z(nug);
      u2z(rec_u->roc);
      rec_u->roc = cor;
    }
    u2z(ovo);
  }
}


void u2_sist_get_pill_filestr(c3_c * str_w, int len_w)
{
  if ( u2_yes == u2_Host.ops_u.nuu ) {
    // creating a new pier; no existing pill; grab prepackaged freeze-dried one

    snprintf(str_w, len_w, "%s/urbit.pill", U2_LIB);
  } else {
    // existing pier; use it
    c3_c bas_c[2048];
    u2_sist_get_pier_dirstr(bas_c, 2048);
    snprintf(str_w, len_w, "%s/.urb/urbit.pill", bas_c);
  }
}

// NOTFORCHECKIN - we call getpwuid() over and over - centralize it
// in one place and store it so we don't do OS calls.

void u2_sist_get_pier_dirstr(c3_c * str_w, int len_w)
{
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;

    // chop off tilda
    snprintf(str_w, len_w, "%s/urb/%s", homedir, u2_Host.cpu_c + 1);  
}

void u2_sist_get_lockfile_filestr(c3_c * str_w, int len_w)
{
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;

    // chop off tilda
    snprintf(str_w, len_w, "%s/urb/%s/.vere.lock", homedir, u2_Host.cpu_c + 1);  
}


void u2_sist_mkdir_chkpt_dir()
{
  c3_c    base_c[2048];
  c3_c    ship_c[2048];
  c3_c    full_c[2048];
  c3_c    chk_c[2048];
  
  {
    // in all mkdir()s below, ignore errors - OK if dir already exists


    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;

    // create ~/urb/   -> write to base_c
    snprintf(base_c, 2048, "%s/urb", homedir);  
    mkdir(base_c, 0700); 

    // create ~/urb/<shipname> -> write to ship_c
    snprintf(ship_c, 2048, "%s/%s", base_c, u2_Host.cpu_c + 1);  
    mkdir(ship_c, 0700);

    // create ~/urb/<shipname>/.urb -> write to full_c
    snprintf(full_c, 2048, "%s/.urb", ship_c);
    mkdir(full_c, 0700);

    // create ~/urb/<shipname>/.urb/ckh -> write to ckh_c
    snprintf(chk_c, 2048, "%s/chk", full_c);
    mkdir(chk_c, 0700);
  }

}

void u2_sist_get_egz_filestr(c3_c* buf_c, // return arg
                              int len_c) // input arg
{
  c3_c    base_c[2048];  
  u2_sist_get_pier_dirstr(base_c, 2048);

  snprintf(buf_c, len_c, "%s/.urb/egz.hope", base_c);
}

void u2_sist_get_doturb_dirstr(c3_c * str_w, int len_w)
{
  c3_c    pier_c[2048];  
  u2_sist_get_pier_dirstr(pier_c, 2048);

  snprintf(str_w, len_w, "%s/.urb", pier_c);  
}

void u2_sist_get_egz_quick_dirstr(c3_c * str_w, int len_w)
{
  c3_c    pier_c[2048];  
  u2_sist_get_pier_dirstr(pier_c, 2048);

  snprintf(str_w, len_w, "%s/.urb/egz_quick", pier_c);  
}


void u2_sist_get_egz_quick_filestr(c3_c * str_w, int len_w, c3_d sequence_d, c3_y msgtype_y)
{
  c3_c    pier_c[2048];  
  u2_sist_get_pier_dirstr(pier_c, 2048);

  snprintf(str_w, len_w, "%s/.urb/egz_quick/%lli.%i", pier_c, (long long int) sequence_d, msgtype_y);  
}



void u2_sist_get_chkpt_filestr(c3_c* fil_c, c3_c* suf_c, c3_c* buf_c, int len_c)
{
  c3_c    base_c[2048];  
  u2_sist_get_pier_dirstr(base_c, 2048);

  snprintf(buf_c, len_c, "%s/.urb/chk/%s.%s", base_c, fil_c, suf_c);
}

void u2_sist_get_code_filestr(c3_c* buf_c, // return arg
                              int len_c,    
                              c3_c* gum_c) // input arg
{
  c3_c    base_c[2048];  
  u2_sist_get_pier_dirstr(base_c, 2048);

  snprintf(buf_c, len_c, "%s/.urb/code.%s", base_c, gum_c);
}


// The unix sync directory (I think).  
// Formerly at ./urbit/mypier/wanrec-pilsut-laslut-pasmex--posryg-migmyr-hosref-ribrum (or what have you)
// Now at      ~/urb/<piername>/<shipname>
//
void u2_sist_get_ship_dirstr(c3_c* buf_c, // return arg
                              int len_c) // input arg
{
  c3_c    base_c[2048];  
  u2_sist_get_pier_dirstr(base_c, 2048);

  snprintf(buf_c, len_c, "%s/%s", base_c, u2_Host.cpu_c + 1);
}

/* _sist_home(): create pier directory.

   Note that piers are named after the ship that will primarily be parked there.
   
   ~/home/<user>/urb/<piername>/                     - note that piername = primary ship name
                               /<shipname>/          - unix sync dir (as in old world order)
                               /.urb/                - misc files    (as in old world order)            
                                    /chk
                                    /egz.hope
                                    /egz_quick
                                    /get
                                    /put
                                    /sis

                                    /urbit.pill
                                    /zod
 */
void
_sist_home(u2_reck* rec_u)
{
  c3_c    base_c[2048];
  c3_c    pier_c[2048];
  c3_c    full_c[2048];
  
  {
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;

    // create ~/urb/   -> write to base_c
    snprintf(base_c, 2048, "%s/urb", homedir);  
    mkdir(base_c, 0700);

    // create ~/urb/<pier> -> write to pier_c
    // note that we strip off the leading tilda
    snprintf(pier_c, 2048, "%s/%s", base_c, u2_Host.cpu_c + 1);  
    int ret = 0;
    ret = mkdir(pier_c, 0700);
    if ( 0 != ret && EEXIST != errno  ) {
      perror(pier_c);
      u2_lo_bail(rec_u);
    }

    //  Create .urb subdirectories
    //
    snprintf(full_c, 2048, "%s/.urb", pier_c);
    ret = mkdir(full_c, 0700);
    if ( 0 != ret && EEXIST != errno  ) {
      perror(full_c);
      u2_lo_bail(rec_u);
    }

    snprintf(full_c, 2048, "%s/.urb/get", pier_c);
    ret = mkdir(full_c, 0700);
    if ( 0 != ret && EEXIST != errno  ) {
      perror(full_c);
      u2_lo_bail(rec_u);
    }

    snprintf(full_c, 2048, "%s/.urb/egz_quick", pier_c);
    ret = mkdir(full_c, 0700);
    if ( 0 != ret && EEXIST != errno  ) {
      perror(full_c);
      u2_lo_bail(rec_u);
    }

    snprintf(full_c, 2048, "%s/.urb/put", pier_c);
    ret = mkdir(full_c, 0700);
    if ( 0 != ret && EEXIST != errno  ) {
      perror(full_c);
      u2_lo_bail(rec_u);
    }

    snprintf(full_c, 2048, "%s/.urb/sis", pier_c);
    ret = mkdir(full_c, 0700);
    if ( 0 != ret && EEXIST != errno  ) {
      perror(full_c);
      u2_lo_bail(rec_u);
    }
  }

  //  Copy urbit.pill.
  //
  {
    c3_c    old_c[2048];
    c3_c    new_c[2048];
    c3_c    doit_c[2048];
    u2_sist_get_pill_filestr(old_c, 2048);
    snprintf(new_c, 2048, "%s/.urb", pier_c);

    snprintf(doit_c, 2048, "cp %s %s", old_c, new_c);
    if ( 0 != system(doit_c) ) {
      uL(fprintf(uH, "could not %s\n", full_c));
      u2_lo_bail(rec_u);
    }
  }

  //  Copy zod files, if we're generating a carrier.
  //
  if ( u2_Host.ops_u.imp_c ) {

    // OLD
    snprintf(full_c, 2048, "cp -r %s/zod %s/%s",
             U2_LIB, u2_Host.cpu_c, u2_Host.ops_u.imp_c+1);

    // NEW
    snprintf(full_c, 2048, "cp -r %s/zod %s/%s",
             U2_LIB, u2_Host.cpu_c, u2_Host.ops_u.imp_c+1);
    if ( 0 != system(full_c) ) {
      uL(fprintf(uH, "could not %s\n", full_c));
      u2_lo_bail(rec_u);
    }
  }
}

  /* _sist_cask(): ask for a passcode.
   */
  static u2_noun
    _sist_cask(u2_reck* rec_u, c3_c* dir_c, u2_bean nun)
  {
    c3_c   paw_c[60];
    u2_noun key;

    uH;
    while ( 1 ) {
      printf("passcode for %s%s? ~", dir_c, (u2_yes == nun) ? " [none]" : "");

      paw_c[0] = 0;
      c3_fpurge(stdin);
      fgets(paw_c, 59, stdin);

      if ( '\n' == paw_c[0] ) {
        if ( u2_yes == nun ) {
          key = 0; break;
        }
        else {
          continue;
        }
      }
      else {
        c3_c* say_c = c3_malloc(strlen(paw_c) + 2);
        u2_noun say;

        say_c[0] = '~';
        say_c[1] = 0;
        strncat(say_c, paw_c, strlen(paw_c) - 1);

        say = u2_do("slay", u2_ci_string(say_c));
        if ( (u2_nul == say) ||
             (u2_blip != u2h(u2t(say))) ||
             ('p' != u2h(u2t(u2t(say)))) )
          {
            printf("invalid passcode\n");
            continue;
          }
        key = u2k(u2t(u2t(u2t(say))));

        u2z(say);
        break;
      }
    }
    uL(0);
    return key;
  }

  /* _sist_text(): ask for a name string.
   */
  static u2_noun
    _sist_text(u2_reck* rec_u, c3_c* pom_c)
  {
    c3_c   paw_c[60];
    u2_noun say;

    uH;
    while ( 1 ) {
      printf("%s: ", pom_c);

      paw_c[0] = 0;
      fpurge(stdin);
      fgets(paw_c, 59, stdin);

      if ( '\n' == paw_c[0] ) {
        continue;
      }
      else {
        c3_w len_w = strlen(paw_c);

        if ( paw_c[len_w - 1] == '\n' ) {
          paw_c[len_w-1] = 0;
        }
        say = u2_ci_string(paw_c);
        break;
      }
    }
    uL(0);
    return say;
  }

#if 0
  /* _sist_bask(): ask a yes or no question.
   */
  static u2_bean
    _sist_bask(c3_c* pop_c, u2_bean may)
  {
    u2_bean yam;

    uH;
    while ( 1 ) {
      c3_c ans_c[3];

      printf("%s [y/n]? ", pop_c);
      ans_c[0] = 0;

      c3_fpurge(stdin);
      fgets(ans_c, 2, stdin);

      if ( (ans_c[0] != 'y') && (ans_c[0] != 'n') ) {
        continue;
      } else {
        yam = (ans_c[0] != 'n') ? u2_yes : u2_no;
        break;
      }
    }
    uL(0);
    return yam;
  }
#endif

  /* _sist_rand(): fill a 256-bit (8-word) buffer.
   */
  static void
    _sist_rand(u2_reck* rec_u, c3_w* rad_w)
  {
    c3_i fid_i = open(DEVRANDOM, O_RDONLY);

    if ( 32 != read(fid_i, (c3_y*) rad_w, 32) ) {
      c3_assert(!"lo_rand");
    }
    close(fid_i);
  }

  /* _sist_fast(): offer to save passcode by mug in home directory.
   */
  static void
    _sist_fast(u2_reck* rec_u, u2_noun pas, c3_l key_l)
  {
    c3_c    ful_c[2048];
    //    c3_c*   hom_c = u2_Host.cpu_c;
    u2_noun gum   = u2_dc("scot", 'p', key_l);
    c3_c*   gum_c = u2_cr_string(gum);
    u2_noun yek   = u2_dc("scot", 'p', pas);
    c3_c*   yek_c = u2_cr_string(yek);

    u2_sist_get_code_filestr(ful_c,  2048, gum_c);

    printf("saving passcode in %s \r\n", ful_c);
    printf("(for real security, write it down and delete the file...)\r\n");
    {
      c3_i fid_i;


      if ( (fid_i = open(ful_c, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0 ) {
        uL(fprintf(uH, "fast: could not save %s\n", ful_c));
        u2_lo_bail(rec_u);
      }
      write(fid_i, yek_c, strlen(yek_c));
      close(fid_i);
    }
    free(gum_c);
    u2z(gum);

    free(yek_c);
    u2z(yek);
  }

  /* _sist_staf(): try to load passcode by mug from home directory.
   */
  static u2_noun
    _sist_staf(u2_reck* rec_u, c3_l key_l)
  {
    c3_c    ful_c[2048];
    u2_noun gum   = u2_dc("scot", 'p', key_l);
    c3_c*   gum_c = u2_cr_string(gum);
    u2_noun txt;

    c3_c bas_c[2048];
    u2_sist_get_pier_dirstr(bas_c, 2048);
    snprintf(ful_c, 2048, "%s/.urb/code.%s", bas_c, gum_c);
    free(gum_c);
    u2z(gum);
    txt = u2_walk_safe(ful_c);

    if ( 0 == txt ) {
      uL(fprintf(uH, "staf: no passcode %s\n", ful_c));
      return 0;
    }
    else {
      // c3_c* txt_c = u2_cr_string(txt);
      u2_noun say = u2_do("slay", txt);
      u2_noun pas;


      if ( (u2_nul == say) ||
           (u2_blip != u2h(u2t(say))) ||
           ('p' != u2h(u2t(u2t(say)))) )
        {
          uL(fprintf(uH, "staf: %s is corrupt\n", ful_c));
          u2z(say);
          return 0;
        }
      uL(fprintf(uH, "loaded passcode from %s\n", ful_c));
      pas = u2k(u2t(u2t(u2t(say))));

      u2z(say);
      return pas;
    }
  }



  /* _sist_fatt(): stretch a 64-bit passcode to make a 128-bit key.
   */
  static u2_noun
    _sist_fatt(c3_l sal_l, u2_noun pas)
  {
    c3_w i_w;
    u2_noun key = pas;

    //  XX use scrypt() - this is a stupid iterated hash
    //
    for ( i_w = 0; i_w < 32768; i_w++ ) {
      key = u2_dc("shaf", sal_l, key);
    }
    return key;
  }

  /* _sist_zest(): create a new, empty record.
   */
//  static
 void
    _sist_zest(u2_reck* rec_u)
  {
    // struct stat buf_b;
    // c3_c        ful_c[8193];
    c3_l        sal_l;

    //  Create the ship directory.
    //
    _sist_home(rec_u);

    //  Generate a 31-bit salt.
    //
    {
      c3_w rad_w[8];

      _sist_rand(rec_u, rad_w);
      sal_l = (0x7fffffff & rad_w[0]);
    }

    //  Create and save a passcode (to passcode file)
    //
    {
      c3_w rad_w[8];
      u2_noun pas;

      _sist_rand(rec_u, rad_w);
      pas = u2_ci_words(2, rad_w);

      rec_u->key = _sist_fatt(sal_l, u2k(pas));
      _sist_fast(rec_u, pas, u2_mug(rec_u->key));
    }

    // Create a new egz file w default header
    //   Do we really want to do that in all cases, even if using kafka?  Unclear.
    //   But what if they restart later w egz logging? Then we've got it. <shrug>
    u2_egz_write_header(rec_u, sal_l);

    //  Work through the boot events.
    u2_raft_work(rec_u);
  }

  /* _sist_make(): boot from scratch.
   */
static void
    _sist_make(u2_reck* rec_u, u2_noun fav)
{
  //  Initialize ames
  u2_ames_ef_bake();

  //  Authenticate and initialize terminal.
  //
  u2_term_ef_bake(fav);

  //  Create the ship directory.
  //
  _sist_zest(rec_u);
}






  /* _sist_rest(): restore from record, or exit.
   */
 static void
_sist_rest(u2_reck* rec_u)
{
  c3_i        fid_i;
  c3_d        old_d = rec_u->ent_d;
  c3_d        kaf_d = rec_u->kaf_d;
  c3_d        las_d = 0;
  u2_bean     ohh = u2_no;
  u2_eghd     led_u;

  c3_t kafk_b = (u2_Host.ops_u.kaf_c != NULL);

  // (1) diagnostic print out for humans
  //
  if ( 0 != rec_u->ent_d ) {
    u2_noun ent;
    c3_c*   ent_c;

    ent = u2_ci_chubs(1, &rec_u->ent_d);
    ent = u2_dc("scot", c3__ud, ent);
    ent_c = u2_cr_string(ent);
    uL(fprintf(uH, "rest: checkpoint to event %s\n", ent_c));
    free(ent_c);
    u2z(ent);
  }


  // (2) setup
  //

  if (kafk_b) {
    // get ready to read: note use of kaf_d
    u2_kafk_pre_read(kaf_d);
  } else {
    //  Open the file, check the header, store details in fid_i, led_u
    c3_t success = u2_egz_open(rec_u, & fid_i, & led_u);
    if (success != c3_true) { exit(-1); }
  }

  // (3) Read in first event from log.  See if there's anything newer
  //     than what we already have in memory.
  //
  u2_noun     roe = u2_nul;

  if (kafk_b) {
    roe = u2_kafk_pull_all(rec_u, & ohh);
  } else {
    roe = u2_egz_pull_all(rec_u, fid_i, & ohh);
  }

  if ( u2_nul == roe ) {
    //  Nothing in the log that was not also in the checkpoint.
    //
    c3_assert(rec_u->ent_d == old_d);
    if ( las_d + 1 != old_d ) {
      uL(fprintf(uH, "checkpoint and log disagree! las:%llu old:%llu\n",
                 las_d + 1, old_d));
      uL(fprintf(uH, "Some events appear to be missing from the log.\n"
                 "Please contact the authorities, "
                 "and do not delete your pier!\n"));
      u2_lo_bail(rec_u);
    }
    u2_term_ef_boil();
    return;
  }


  // (4) the log contains info that is new to us. Read it! Use it!
  //

  exit(-1); // the next few lines are UNIMPLEMENTED  // NOTFORCHECKIN
  // we need to:
  //   * read through all events
  //   * replay the committed ones
  //   * keep track of the uncommitted ones and run these under a longer timer

  u2_noun rou = roe;
  c3_w    xno_w;

  uL(fprintf(uH, "rest: replaying through event %llu\n", las_d));
  fprintf(uH, "---------------- playback starting----------------\n");

  xno_w = 0;
  while ( u2_nul != roe ) {
    u2_noun i_roe = u2h(roe);
    u2_noun t_roe = u2t(roe);
    u2_noun now = u2h(i_roe);
    u2_noun ovo = u2t(i_roe);

    // set timestamp
    u2_reck_wind(rec_u, u2k(now));
    if ( (u2_yes == u2_Host.ops_u.vno) &&
         ( (c3__veer == u2h(u2t(ovo)) ||
            (c3__vega == u2h(u2t(ovo)))) ) )
      {
        fprintf(stderr, "replay: skipped veer\n");
      }
    else if ( u2_yes == u2_Host.ops_u.fog &&
              u2_nul == t_roe ) {
      fprintf(stderr, "replay: -Xwtf, skipped last event\n");
    }
    else {
      _sist_sing(rec_u, u2k(ovo));
      fputc('.', stderr);
    }

    // fprintf(stderr, "playback: sing: %d\n", xno_w));

    roe = t_roe;
    xno_w++;

    if ( 0 == (xno_w % 1000) ) {
      uL(fprintf(uH, "{%d}\n", xno_w));
      u2_lo_grab("rest", rou, u2_none);
    }
  }
  u2z(rou);

  uL(fprintf(stderr, "\n---------------- playback complete----------------\n"));

  //  (5) Rewrite the log header
  //
  if (kafk_b) {
    // do nothing; not relevant to kafka
  } else {
    u2_egz_rewrite_header(rec_u, fid_i, ohh, & led_u);
  }

  // success
  u2_term_ef_boil();
}

/* _sist_zen(): get OS entropy.
*/
static u2_noun
_sist_zen(u2_reck* rec_u)
{
  c3_w rad_w[8];

  _sist_rand(rec_u, rad_w);
  return u2_ci_words(8, rad_w);
}

/* u2_sist_boot(): restore or create.
*/
void
u2_sist_boot(void)
{
  uL(fprintf(uH, "sist: booting\n"));
  if ( u2_yes == u2_Host.ops_u.nuu ) {
    u2_noun pig = u2_none;

    if ( 0 == u2_Host.ops_u.imp_c ) {
      c3_c bas_c[2048];
      c3_c get_c[2048];
      u2_sist_get_pier_dirstr(bas_c, 2048);
      snprintf(get_c, 2048, "%s/.urb/get", bas_c);
      if ( 0 == access(get_c, 0) ) {
          uL(fprintf(uH, "pier: already built\n"));
          u2_lo_bail(u2A);
      }
      u2_noun ten = _sist_zen(u2A);
      uL(fprintf(uH, "generating 2048-bit RSA pair...\n"));

      pig = u2nq(c3__make, u2_nul, 11, u2nc(ten, u2_Host.ops_u.fak));
    }
    else {
      u2_noun imp = u2_ci_string(u2_Host.ops_u.imp_c);
      u2_noun whu = u2_dc("slaw", 'p', u2k(imp));

      if ( (u2_nul == whu) ) {
        fprintf(stderr, "czar: incorrect format\r\n");
        u2_lo_bail(u2A);
      }
      else {
        u2_noun gen = u2_nul;
        u2_noun gun = u2_nul;
        if (u2_no == u2_Host.ops_u.fak) {
          gen = _sist_text(u2A, "generator");
          gun = u2_dc("slaw", c3__uw, gen);

          if ( u2_nul == gun ) {
            fprintf(stderr, "czar: incorrect format\r\n");
            u2_lo_bail(u2A);
          }
        }
        else {
          gun = u2nc(u2_nul, u2_nul);
        }
        pig = u2nq(c3__sith,
                   u2k(u2t(whu)),
                   u2k(u2t(gun)),
                   u2_Host.ops_u.fak);

        u2z(whu); u2z(gun);
      }
      u2z(imp);
    }
    _sist_make(u2A, pig);
  }
  else {
    _sist_rest(u2A);
  }
}

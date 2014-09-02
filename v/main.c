/* v/main.c
**
** This file is in the public domain.
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <gmp.h>
#include <stdint.h>
#include <uv.h>
#include <sigsegv.h>
#include <curses.h>
#include <termios.h>
#include <term.h>
#include <dirent.h>
#include <pmmintrin.h>
#include <xmmintrin.h>

#define U2_GLOBAL
#define C3_GLOBAL
#include "all.h"
#include "v/vere.h"
#include "v/kafk.h"


/**  Legacy fixed jet linkage.  Destroy me please.
**/
    /* External drivers.
    */
      extern u2_ho_driver j2_da(k_164);

    /* Built-in battery drivers.   Null `cos` terminates.
    */
      u2_ho_driver *HostDriverBase[] = {
        &j2_k_164_d,
        0
      };

/* _main_readw(): parse a word from a string.
*/
static u2_bean
_main_readw(const c3_c* str_c, c3_w max_w, c3_w* out_w)
{
  c3_c* end_c;
  c3_w  par_w = strtoul(str_c, &end_c, 0);

  if ( *str_c != '\0' && *end_c == '\0' && par_w < max_w ) {
    *out_w = par_w;
    return u2_yes;
  }
  else return u2_no;
}

/* _main_getopt(): extract option map from command line.
*/
static u2_bean
_main_getopt(c3_i argc, c3_c** argv)
{
  c3_i ch_i;
  c3_w arg_w;

  u2_Host.ops_u.abo = u2_no;
  u2_Host.ops_u.bat = u2_no;
  u2_Host.ops_u.gab = u2_no;
  u2_Host.ops_u.loh = u2_no;
  u2_Host.ops_u.dem = u2_no;
  u2_Host.ops_u.fog = u2_no;
  u2_Host.ops_u.fak = u2_no;
  u2_Host.ops_u.pro = u2_no;
  u2_Host.ops_u.veb = u2_yes;
  u2_Host.ops_u.nuu = u2_no;
  u2_Host.ops_u.mem = u2_no;
  u2_Host.ops_u.kno_w = DefaultKernel;
  u2_Host.ops_u.kaf_c = (c3_c*) NULL;

  while ( (ch_i = getopt(argc, argv, "I:A:K:X:f:k:l:n:p:r:LabcdgqvFM")) != -1 ) {
    switch ( ch_i ) {
      case 'A': {
        u2_Host.ops_u.adm_c = strdup(optarg);
        if ((strcmp(optarg, "ltok") != 0) && (strcmp(optarg, "ktol"))) {
          fprintf(stderr, "illegal -A subcommand: %s\n",  optarg);
          return u2_no;

        }
        break;
      }
      case 'M': {
        u2_Host.ops_u.mem = u2_yes;
        break;
      }
      case 'I': {
        u2_Host.ops_u.imp_c = strdup(optarg);
        break;
      }
      case 'X': {
        if ( 0 != strcmp("wtf", optarg) ) {
          return u2_no;
        } else u2_Host.ops_u.fog = u2_yes;
        break;
      }
      case 'f': {
        if ( u2_no == _main_readw(optarg, 100, &u2_Host.ops_u.fuz_w) ) {
          return u2_no;
        }
        break;
      }
      case 'k': {
        if ( u2_no == _main_readw(optarg, 256, &u2_Host.ops_u.kno_w) ) {
          return u2_no;
        }
        break;
      }
      case 'l': {
        if ( u2_no == _main_readw(optarg, 65536, &arg_w) ) {
          return u2_no;
        } else u2_Host.ops_u.rop_s = arg_w;
        break;
      }
      case 'n': {
        u2_Host.ops_u.nam_c = strdup(optarg);
        break;
      }
      case 'p': {
        if ( u2_no == _main_readw(optarg, 65536, &arg_w) ) {
          return u2_no;
        } else u2_Host.ops_u.por_s = arg_w;
        break;
      }
      case 'r': {
        u2_Host.ops_u.raf_c = strdup(optarg);
        break;
      }
      case 'L': { u2_Host.ops_u.loh = u2_yes; break; }
      case 'F': {
        u2_Host.ops_u.loh = u2_yes;
        u2_Host.ops_u.fak = u2_yes;
        break;
      }
      case 'K': {
        u2_Host.ops_u.kaf_c  = strdup(optarg);
        break;
      }

      case 'a': { u2_Host.ops_u.abo = u2_yes; break; }
      case 'b': { u2_Host.ops_u.bat = u2_yes; break; }
      case 'c': { u2_Host.ops_u.nuu = u2_yes; break; }
      case 'd': { u2_Host.ops_u.dem = u2_yes; break; }
      case 'g': { u2_Host.ops_u.gab = u2_yes; break; }
      case 'q': { u2_Host.ops_u.veb = u2_no; break; }
      case 'v': { u2_Host.ops_u.veb = u2_yes; break; }
      case '?': default: {
        return u2_no;
      }
    }
  }

  if ( u2_Host.ops_u.rop_s == 0 && u2_Host.ops_u.raf_c != 0 ) {
    fprintf(stderr, "The -r flag requires -l.\n");
    return u2_no;
  }

  if ( u2_yes == u2_Host.ops_u.bat ) {
    u2_Host.ops_u.dem = u2_yes;
    u2_Host.ops_u.nuu = u2_yes;
  }


  if ( u2_Host.ops_u.nam_c == 0 ) {
    u2_Host.ops_u.nam_c = getenv("HOSTNAME");
    if ( u2_Host.ops_u.nam_c == 0 ) {
      c3_w len_w = sysconf(_SC_HOST_NAME_MAX) + 1;

      u2_Host.ops_u.nam_c = c3_malloc(len_w);
      if ( 0 != gethostname(u2_Host.ops_u.nam_c, len_w) ) {
        perror("gethostname");
        exit(1);
      }
    }
  }

  // N.B. this used to be something like 'mypier', which specified computer name
  // As of July 2014, computer name = ship name = name of directory in ~/urb where pier lives
  //
  // find shipname
  //
  if ( argc != (optind + 1) ) {
    fprintf(stderr, "You must specify your ship name on the command line.\n");
    exit(1);
  } 
  if (argv[optind][0] != '~'){
    fprintf(stderr, "You must specify your ship name (not pier). Names begin with '~'.\n");
    exit(1);
  }

  u2_Host.cpu_c = strdup(argv[optind]);
  return u2_yes;

}

/* u2_ve_usage(): print usage and exit.
*/
static void
u2_ve_usage(c3_i argc, c3_c** argv)
{
  fprintf(stderr, "%s: usage: { options } shipname\n",                  argv[0]);
  fprintf(stderr, "       [-I emperor ]   // specify emperor e.g. ~zod \n");
  fprintf(stderr, "       [-K brokerlist] // kafka logging. Specify 1+ brokers e.g. localhost:9092,... \n");
  fprintf(stderr, "       [-X ]           // skip last event \n");
  fprintf(stderr, "       [-a ]           // ? \n");
  fprintf(stderr, "       [-b ]           // batch create \n");
  fprintf(stderr, "       [-c ]           // create new pier (to be named after ship spec) \n");
  fprintf(stderr, "       [-d ]           // ? \n");
  fprintf(stderr, "       [-f ]           // fuzz testing\n");
  fprintf(stderr, "       [-g ]           // ? \n");
  fprintf(stderr, "       [-k stage]      // kernel version \n");
  fprintf(stderr, "       [-l ]           // raft port\n");
  fprintf(stderr, "       [-n ]           // unix hostname \n");
  fprintf(stderr, "       [-p ames_port]  // ames port\n");
  fprintf(stderr, "       [-q ]           // quiet\n");
  fprintf(stderr, "       [-r host ]      // raft flotilla \n");
  fprintf(stderr, "       [-v ]           // verbose\n");
  fprintf(stderr, "       [-F ]           // fake carrier \n");
  fprintf(stderr, "       [-L ]           // local-only networking \n");
  fprintf(stderr, "       [-A command ]   // admin tool \n");
  fprintf(stderr, "                       // * ltok - read log, write it kafka \n");
  fprintf(stderr, "                       // * ktol - read kafka, write it log \n");

  exit(1);
}

/* u2_ve_panic(): panic and exit.
*/
static void
u2_ve_panic(c3_i argc, c3_c** argv)
{
  fprintf(stderr, "%s: gross system failure\n", argv[0]);
  exit(1);
}

/* u2_ve_sysopt(): apply option map to system state.
*/
static void
u2_ve_sysopt()
{
  u2_Local = strdup(u2_Host.cpu_c);
  u2_System = U2_LIB;
  u2_Flag_Abort = u2_Host.ops_u.abo;
  u2_Flag_Garbage = u2_Host.ops_u.gab;
  u2_Flag_Profile = u2_Host.ops_u.pro;
  u2_Flag_Verbose = u2_Host.ops_u.veb;
}

static jmp_buf Signal_buf;
#ifndef SIGSTKSZ
# define SIGSTKSZ 16384
#endif
static uint8_t Sigstk[SIGSTKSZ];

volatile enum { sig_none, sig_overflow, sig_interrupt } Sigcause;

static void _main_cont(void *arg1, void *arg2, void *arg3)
{
  (void)(arg1);
  (void)(arg2);
  (void)(arg3);
  siglongjmp(Signal_buf, 1);
}

static void
overflow_handler(int emergency, stackoverflow_context_t scp)
{
  if ( 1 == emergency ) {
    write(2, "stack emergency\n", strlen("stack emergency" + 2));
    exit(1);
  } else {
    Sigcause = sig_overflow;
    sigsegv_leave_handler(_main_cont, NULL, NULL, NULL);
  }
}

static void
interrupt_handler(int x)
{
  Sigcause = sig_interrupt;
  longjmp(Signal_buf, 1);
}

c3_i
main(c3_i   argc,
     c3_c** argv)
{
  // set both logging systems to unit-ed
  //
  u2K->inited_t = c3_false;

  c3_w kno_w;

  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  //  Parse options.
  //
  if ( u2_no == _main_getopt(argc, argv) ) {
    u2_ve_usage(argc, argv);
    return 1;
  }

  u2_ve_sysopt();

  printf("~\n");
  printf("welcome.\n");
  printf("vere: urbit home is %s\n", u2_Host.cpu_c);
  printf("vere: hostname is %s\n", u2_Host.ops_u.nam_c);

  if ( u2_yes == u2_Host.ops_u.dem && u2_no == u2_Host.ops_u.bat ) {
    printf("Starting daemon\n");
  }

  //  Seed prng. Don't panic -- just for fuzz testing and election timeouts.
  //
  srand(getpid());

  //  Instantiate process globals.
  {
    u2_wr_check_init(u2_Host.cpu_c);
    u2_Host.xit_i = 0;
    if ( (u2_no == u2_Host.ops_u.nuu) &&
          (u2_yes == u2_loom_load()) )
    {
      u2_Host.wir_r = u2_ray_of(0, 0);
      u2_Wire = u2_Host.wir_r;

      u2_Host.arv_u = u2_Arv;

      u2_Arv->ova.egg_u = u2_Arv->ova.geg_u = 0;

      u2_lo_grab("init", u2_none);

      //  Horrible ancient stuff.
      //
      kno_w = u2_Host.arv_u->kno_w;
      u2_Host.kno_w = kno_w;

      u2_ho_push();
    }
    else {
      u2_loom_boot();
      u2_Host.wir_r = u2_wr_init(c3__rock, u2_ray_of(0, 0), u2_ray_of(1, 0));
      u2_Wire = u2_Host.wir_r;

      u2_Host.arv_u = u2_Arv;
    }
  }

  //  If we have not loaded from checkpoint, build kernel.
  //
  if ( 0 != u2_Host.arv_u->ent_d ) {
    u2_reck_time(u2_Host.arv_u);
    u2_reck_numb(u2_Host.arv_u);
    {
      c3_c* dyt_c = u2_cr_string(u2_Host.arv_u->wen);

      printf("time: %s\n", dyt_c);
      free(dyt_c);
    }
  }
  else {
    //  Set outside bail trap.  Should not be used, but you never know...
    //
    if ( 0 != u2_cm_trap() ) {
      u2_ve_panic(argc, argv);
    }
    else {
      //  Set boot and goal stages.
      {
        if ( (0 == u2_Host.ops_u.kno_w) || (u2_Host.ops_u.kno_w > 255) ) {
          kno_w = DefaultKernel;
        } else {
          kno_w = u2_Host.ops_u.kno_w;
        }
      }

      //  Load the system.
      //
      {
        u2_Host.kno_w = u2_Host.ops_u.kno_w;

        u2_reck_boot(u2_Host.arv_u);
      }
      u2_cm_done();
    }
  }

  //  Install signal handlers and set buffers.
  //
  //  Note that we use the sigmask-restoring variant.  Essentially, when
  //  we get a signal, we force the system back into the just-booted state.
  //  If anything goes wrong during boot (above), it's curtains.
  {
    if ( 0 != sigsetjmp(Signal_buf, 1) ) {
      switch ( Sigcause ) {
        case sig_overflow: printf("[stack overflow]\r\n"); break;
        case sig_interrupt: printf("[interrupt]\r\n"); break;
        default: printf("[signal error!]\r\n"); break;
      }
      Sigcause = sig_none;

      signal(SIGINT, SIG_DFL);
      stackoverflow_deinstall_handler();

      //  Print the trace, do a GC, etc.
      //
      //  This is half-assed at present, so we exit.
      //
      u2_lo_sway(0, u2k(u2_wire_tax(u2_Wire)));

      u2_lo_bail(u2_Host.arv_u);

      exit(1);
    }
#if 1
    if ( -1 == stackoverflow_install_handler
        (overflow_handler, Sigstk, SIGSTKSZ) )
    {
      fprintf(stderr, "overflow_handler: install failed\n");
      exit(1);
    }
    signal(SIGINT, interrupt_handler);
    signal(SIGIO, SIG_IGN);
#endif
  }

  u2_lo_grab("main", u2_none);

  // booted in admin mode: do a task, then exit
  // booted in user mode: do command loop
  if (u2_Host.ops_u.adm_c != 0){
    if (strcmp(u2_Host.ops_u.adm_c, "ltok") ==0) { u2_kafka_admin_egz_to_kafka(); }
    else if (strcmp(u2_Host.ops_u.adm_c, "ktol") ==0) { u2_kafka_admin_kafka_to_egz(); }
    else  { fprintf(stderr, "unsupported admin mode command %s\n", u2_Host.ops_u.adm_c); exit(1); }
  } else {
    u2_lo_loop();
  }

  return 0;
}

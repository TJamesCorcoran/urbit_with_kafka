/* test/test.c
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
#include "v/egzh.h"

void _sist_home(u2_reck* rec_u);
void _sist_zest(u2_reck* rec_u);
void _lo_init();




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
  if (0){
    if ( argc != (optind + 1) ) {
      fprintf(stderr, "You must specify your ship name on the command line.\n");
      exit(1);
    } 
    if (argv[optind][0] != '~'){
      fprintf(stderr, "You must specify your ship name (not pier). Names begin with '~'.\n");
      exit(1);
    }
    u2_Host.cpu_c = strdup(argv[optind]);
  }
  u2_Host.cpu_c = strdup("~test");  
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

void run_tests();

c3_i
main(c3_i   argc, c3_c** argv)
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

  // hack
  u2_Host.ops_u.nuu = u2_yes;


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

  run_tests();

  return 0;
}

uv_loop_t* lup_u;

void setup_loop()
{
  lup_u = uv_default_loop();

  u2_Host.lup_u = lup_u;

  signal(SIGPIPE, SIG_IGN);     //  pipe, schmipe

  // set up libuv watchers
  _lo_init();

  // set up arvo processing (timeout clock, etc.)
  // SKIPPING FOR TESTING
  //
  // u2_raft_init();

}

void util_run_inside_loop(void (*func_ptr)(), void * data)
{

  uv_async_t * async_u = malloc(sizeof(uv_async_t));
  async_u->data = data;

  uv_async_init(u2_Host.lup_u, 
                async_u, 
                func_ptr );

  uv_async_send(async_u);
}

// funct_ptr - function to run
// data      -
// first     - ms to first invocation
// thereafter - ms after first invocation
void util_run_after_timer(void (*func_ptr)(uv_timer_t* handle, int status), void * data, c3_d first_d, c3_d thereafter_d)
{
  uv_timer_t * timer_u = (uv_timer_t *) malloc (sizeof(uv_timer_t));
  timer_u->data = data;
  
  int ret = uv_timer_init(lup_u, timer_u);
  if (ret <0){
    fprintf(stderr, "error init timer\n");
    exit(-1);
  }

  ret = uv_timer_start(timer_u,
                       func_ptr,
                       first_d,
                       thereafter_d
                       );

  if (ret <0){
    fprintf(stderr, "error init start\n");
    exit(-1);
  }

}

void util_run_loop()
{
  // head into event loop
  if ( u2_no == u2_Host.ops_u.bat ) {
    uv_run(u2L, UV_RUN_DEFAULT);
  }
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// tests begin here
//----------------------------------------------------------------------
//----------------------------------------------------------------------

int write_done = 0;
#define READ_INTERVAL_SECONDS (5 * 1000)


void util_end_test()
{
  printf("ENDING TESTS\n");
  u2_reck*   rec_u = u2_Host.arv_u;
  u2_lo_bail(rec_u);
}

void util_read_gate(uv_timer_t* handle, int status)
{
  printf("READ GATE: READY\n");
  write_done = 1;
  uv_timer_stop(handle);
}


//--------------------
// egzh minifile work queue test
//
//--------------------
void test_egz_queue()
{
  void _enqueue(c3_d new_d,  c3_y msgtype_y);
  c3_t _dequeue(c3_d * ret_d,  c3_y * msgtype_y);

  // this inits mutex...and also starts consolidator thread (which we do NOT want)
  // what's our solution to test just consolidator?  ignore for now.
  u2_egz_init();

  _enqueue(10, 0);
  _enqueue(20, 0);
  _enqueue(30, 0);
  c3_t ret;
  c3_d number;
  c3_y msgtype_y;

  ret = _dequeue(&number, & msgtype_y);
  if (! (ret == c3_true && number == 10)){ printf("FAIL - egz_queue 1\n"); exit(-1);}

  ret = _dequeue(&number, & msgtype_y);
  if (! (ret == c3_true && number == 20)){ printf("FAIL - egz_queue 2\n"); exit(-1);}

  ret = _dequeue(&number, & msgtype_y);
  if (! (ret == c3_true && number == 30)){ printf("FAIL - egz_queue 3\n"); exit(-1);}

  // expect failure here
  ret = _dequeue(&number, & msgtype_y);
  if (! (ret == c3_false)){ printf("FAIL - egz_queue 4\n"); exit(-1);}

  printf("PASS - egz_queue\n");
}

void test_egz_queue_setup()
{
  _sist_home(u2A);
  u2_Host.arv_u->key = 1;

  setup_loop();
  util_run_inside_loop( & test_egz_queue, NULL );
  util_run_after_timer( & util_end_test, NULL, 4 * 1000, 0);
  util_run_loop();
}


void _test_clog_unclog()
{
  bool verbose = false;
  u2_Host.arv_u->key = 1;
  _sist_home(u2A);


  char * payload_str = "hello nouns";
  if (verbose) { printf("input was: %s\n", payload_str); }
  u2_noun aaa = u2_ci_string(payload_str);  

  c3_w malloc_w;
  c3_w len_w;
  c3_y * complete_y;
  u2_clog_o2b(aaa, &malloc_w, & len_w, & complete_y);

  //  c3_y * header_y = complete_y;
  c3_y * data_y   = complete_y + sizeof(u2_clpr);
  
  u2_noun bbb;
  u2_clog_b2o(len_w, data_y, & bbb);

  c3_c* output_c = u2_cr_string(bbb);

  if (verbose) { printf("output is: %s\n", output_c); }

  if (0 == strcmp(payload_str, output_c)){
    printf("PASS - test_clog_unclog\n");
  } else {
    printf("FAIL - test_clog_unclog\n");
    exit(-1);
  }

  // release storage
  free(complete_y);

}

void test_clog_unclog_setup()
{
  setup_loop();
  util_run_inside_loop( & _test_clog_unclog, NULL );
  util_run_after_timer( & util_end_test, NULL, 2 * 1000, 0);
  util_run_loop();
}


void test_kafka_logging_bytes()
{
  u2_Host.ops_u.kaf_c = strdup("localhost:9092");
  u2_kafk_init();

  c3_y * msg_1_c= (c3_y * ) "bytes one";
  u2_kafk_push(  msg_1_c, strlen( (char * )msg_1_c), NULL);

  c3_y * msg_2_c= (c3_y *) "bytes two";
  u2_kafk_push( msg_2_c, strlen( (char * )msg_2_c), NULL);

  c3_ds start_kafka_offset = u2K->largest_offset_seen_ds;

  u2_kafk_pre_read(start_kafka_offset);

  c3_y * msg_c;
  c3_w len_w;
  c3_d ent_d;           
  c3_y msg_type_y;

  c3_t success = u2_kafk_pull_one(& ent_d, & msg_type_y, &len_w, & msg_c);
  if (success != c3_true){
    printf("error read 1");
  } else {
    printf("ent out:  %lli\n", (long long int) ent_d);
    printf("kfk type: %i\n",  msg_type_y);
    printf("msg len:  %i\n", len_w);
    printf("msg:      %s\n",  msg_c);
  }

  success = u2_kafk_pull_one(& ent_d, & msg_type_y, &len_w, & msg_c);
  if (success != c3_true){
    printf("error");
  } else {
    printf("ent out:  %lli\n", (long long int) ent_d);
    printf("kfk type: %i\n",  msg_type_y);
    printf("msg len:  %i\n", len_w);
    printf("msg:      %s\n",  msg_c);
  }

  exit(0);

}

c3_ds test_kafka_logging_ova_rw_start = 0;

void test_kafka_logging_ova_w()
{
  bool verbose = true;

  // write
  char * input_str = "double yolk";
  u2_noun aaa = u2_ci_string(input_str);  
  c3_d ent_1_d = u2_kafk_push_ova(u2_Host.arv_u,  aaa, LOG_MSG_PRECOMMIT);
  if (verbose){ printf("ent 1 #: %lli\n", (long long int) ent_1_d); }

  // what's the biggest offset seen before we start running this test? 
  // store it as a global var; we'll use it later
  test_kafka_logging_ova_rw_start = u2K->largest_offset_seen_precom_ds;


}

void test_kafka_logging_ova_r(uv_timer_t* handle, int status)
{
  bool verbose = false;

  //  printf("test_kafka_logging_ova_r - read gate will open, but human can set write_done = 1 to proceed\n");
  if (0 == write_done){    return;  }
  uv_timer_stop(handle);    // we got through once; that's all that's needed
  

  // prepare to read
  //
  u2_kafk_pre_read(test_kafka_logging_ova_rw_start);

  // precommit msg
  //
  c3_d    ent_d;
  c3_y    msg_type_y;
  u2_noun ova;
  c3_c*   output_str = NULL;

  c3_t success = u2_kafk_pull_one_ova(& ent_d, & msg_type_y, &ova);
  if (success != c3_true){
    if (verbose) { printf("error read 1"); }
  } else if (verbose) {
    printf("ent: %lli\n", (long long int) ent_d);
    printf("kfk type: %i\n",  msg_type_y);
    output_str = u2_cr_string(ova);
    printf("ova: %s\n",  output_str);
  }

  if (! (0 == strcmp("double yolk", output_str) && msg_type_y == LOG_MSG_PRECOMMIT)){
    printf("FAIL - kafka_logging_ova_r #1 \n");
    exit(-1);
  }

  // postcommit msg
  //
  output_str = NULL;
  success = u2_kafk_pull_one_ova(& ent_d, & msg_type_y, &ova);
  if (success != c3_true){
    if (verbose) { printf("error read 2"); }
  } else if (verbose) {
    printf("ent: %lli\n", (long long int) ent_d);
    printf("kfk type: %i\n",  msg_type_y);
    output_str = u2_cr_string(ova);
    printf("ova: %s\n",  output_str);
  }

  if (0 == strcmp("double yolk", output_str) && msg_type_y == LOG_MSG_POSTCOMMIT){
    printf("FAIL - kafka_logging_ova #2\n");
    exit(-1);
  }

  printf("PASS - kafka_logging_ova \n");

}

void test_kafka_logging_ova_setup()
{
  u2_Host.ops_u.kaf_c = strdup("localhost:9092");
  u2_Host.arv_u->key = 1;
  u2_kafk_init();

  setup_loop();
  util_run_inside_loop( & test_kafka_logging_ova_w , NULL );
  util_run_after_timer( & test_kafka_logging_ova_r , NULL, READ_INTERVAL_SECONDS, READ_INTERVAL_SECONDS );
  util_run_after_timer( & util_read_gate           , NULL, 20 * 1000, 1 );
  util_run_after_timer( & util_end_test            , NULL, 30 * 1000, 0);
  util_run_loop();
}


// test that minilog files get written.
// run the consolidator by hand.
// test just raw bytes
//
void _egz_consolidator(void *arg);
void test_egz_bytes()
{

  // delete any old files
  c3_w ret_w = system("rm -rf ~/urb/test");
  if (ret_w < 0) { fprintf(stderr, "failure to setup\n"); exit(-1); }


  // create the directory for the minifiles
  _sist_zest(u2A);

  // start the consolidator
  u2_egz_init();

  // write
  //
  c3_y * msg_1_y= (c3_y * ) "hello world 1";
  c3_w   len_1_w = strlen( (char * )msg_1_y);
  u2_egz_push(msg_1_y, len_1_w, 1, 0);

  c3_y * msg_2_y= (c3_y * ) "hello world 2";
  c3_w   len_2_w = strlen( (char * )msg_2_y);
  u2_egz_push(msg_2_y, len_2_w, 2, 0);

  c3_y * msg_3_y= (c3_y * ) "hello world 3";
  c3_w   len_3_w = strlen( (char * )msg_3_y);
  u2_egz_push(msg_3_y, len_3_w, 3, 0);

  // read
  sleep(15);
  
  u2_egz_pull_start();

  c3_d ent_d;
  c3_w  len_w;
  c3_y* bob_y;
  c3_y  msg_type_y;

  printf("BROKEN - egz_queue - design change: we log raw bytes w/o header, thus can't read them back :-( \n");
  return;

  c3_t ret_t =  u2_egz_pull_one(& ent_d,
                                & msg_type_y,
                                & len_w,
                                & bob_y);
  printf("ret_t = %i\n", ret_t);
  printf("ent_d = %lli\n", (long long int) ent_d);
  printf("len_w = %i\n", len_w);
  printf("msg_type_y = %i\n", msg_type_y);
  printf("msg = %s\n", bob_y);

  ret_t =  u2_egz_pull_one(& ent_d,
                           & msg_type_y,
                           & len_w,
                           & bob_y);

  printf("ret_t = %i\n", ret_t);
  printf("ent_d = %lli\n", (long long int) ent_d);
  printf("len_w = %i\n", len_w);
  printf("msg_type_y = %i\n", msg_type_y);
  printf("msg = %s\n", bob_y);

  ret_t =  u2_egz_pull_one(& ent_d,
                           & msg_type_y,
                           & len_w,
                           & bob_y);

  printf("ret_t = %i\n", ret_t);
  printf("ent_d = %lli\n", (long long int) ent_d);
  printf("len_w = %i\n", len_w);
  printf("msg_type_y = %i\n", msg_type_y);
  printf("msg = %s\n", bob_y);

}

void test_egz_bytes_setup()
{
  setup_loop();
  util_run_inside_loop( & test_egz_bytes, NULL );
  util_run_after_timer( & util_end_test, NULL, 10 * 1000, 0);
  util_run_loop();

}

//--------------------
// egz write/read test
//
// architecture:
//    * we write minifiles quickly
//    * we have a full running system w a consolidator thread that runs every 10 seconds
//    * ...but single stepping means that it could take 10 minutes to get egz.hope written
//    * so we have a variable 'write_done' that will be set by util_read_gate() after 15s
//    * ...but a human in the debugger can also set write_done

void test_egz_ovo_w()
{
  u2_Host.arv_u->key = 1;

  u2_egz_write_header(u2_Host.arv_u, 0);

  char * payload_str = "egz ovo";
  printf("input was: %s\n", payload_str);

  u2_noun aaa = u2_ci_string(payload_str);  

  c3_d num_d = u2_egz_push_ova(u2A, aaa, LOG_MSG_PRECOMMIT);
  printf("ovo w: num_d = %llu\n", (unsigned long long int) num_d);
}


void test_egz_ovo_r(uv_timer_t* handle, int status)
{
  bool verbose = false;

  // no need for consolidator in this test
  //
  //  u2_egz_init();

  printf("test_egz_ova_r - read gate will open, but human can set write_done = 1 to proceed\n");
  if (0 == write_done){    return;  }

  printf("******** OVO R **********\n");
  
  u2_egz_pull_start();

  c3_d    ent_d;
  c3_y    msg_type_y;
  u2_noun ovo;

  c3_t ret_t =  u2_egz_pull_one_ova(& ent_d, & msg_type_y, & ovo);

  if (verbose){
    printf("ret_t = %i\n", ret_t);
    printf("ent_d = %lli\n", (long long int) ent_d);
    printf("msg_type_y = %i\n", msg_type_y);
    printf("payload = %s\n", u2_cr_string(ovo));
  }
  
  c3_c * ovo_str = u2_cr_string(ovo);

  printf("payload = %s\n", ovo_str);
  if (0 == strcmp((char *) ovo_str, "egz ovo")){
    printf("PASS - egz_ovo\n");
  } else {
    printf("FAIL - egz_ovo\n");
  }
}


void test_egz_ova_setup()
{

  _sist_home(u2A);
  u2_Host.arv_u->key = 1;


  u2_egz_rm();
  u2_egz_init();

  setup_loop();
  util_run_inside_loop( & test_egz_ovo_w , NULL );
  util_run_after_timer( & test_egz_ovo_r , NULL, READ_INTERVAL_SECONDS, READ_INTERVAL_SECONDS );
  util_run_after_timer( & util_read_gate , NULL, 5 * 1000, 1 );
  util_run_after_timer( & util_end_test  , NULL, 12 * 1000, 0);
  util_run_loop();

}
//----------
// these are your two entry points.
//
// call tests here.
//----------


// these tests will be run inside a full running vere 
//
void run_tests()
{
  // WORKS test_clog_unclog_setup();

  // WORKS test_egz_queue_setup();
  // WORKS test_egz_bytes_setup();
  // WORKS test_egz_ova_setup();

  // WORKS test_kafka_logging_bytes();
  test_kafka_logging_ova_setup();

  exit(1);
}


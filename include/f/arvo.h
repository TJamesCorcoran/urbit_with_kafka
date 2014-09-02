/* include/f/arvo.h
**
** This file is in the public domain.
*/
  /**   Data structures.
  **/
    /* u2_cart: ovum carton.
    */
      struct _u2_reck;

      typedef struct _u2_cart {
        u2_noun vir;                      //  effects of ovum

		// NOTE: tasks associated with 'pre' and 'done' are done in
		//       seperate threads.  Completion order is unspecified,
		//       but once BOTH are done we can proceed to emitting
		//       side effects, updating state.  See raft.c foobar()
		//       and foobar_completion() for details, mutex, etc.
		//
        u2_bean log;                      //  message successfully logged?
        u2_bean done;                     //  arvo processing done?
        u2_bean emit;                     //  ?

        c3_d    ent_d;                    //  entry in raft queue?
        void (*clr_f)                     //  ovum processing failed
            (struct _u2_reck *rec_u,      //  system
             u2_noun,                     //  egg itself
             u2_noun,                     //  failure mode
             u2_noun);                    //  trace if any
        struct _u2_cart* nex_u;           //  next in queue
      } u2_cart;

    /* u2_reck: modern arvo structure.
    */
      typedef struct _u2_reck {
        c3_w    kno_w;                    //  kernel stage
        c3_w    rno_w;                    //  rotor index (always 0)
        c3_d    ent_d;                    //  event counter
        c3_d    kaf_d;                    //  kafka number (loaded from checkpt at boot, then incrementing)

        u2_noun yot;                      //  new toy system
        u2_noun now;                      //  current time, as noun
        u2_noun wen;                      //  current time, as text
        u2_noun sev_l;                    //  instance number
        u2_noun sen;                      //  instance string
        u2_noun own;                      //  owner list

        u2_noun roe;                      //  temporary unsaved events
        u2_noun key;                      //  log key, or 0

        u2_noun ken;                      //  kernel formula (for now)
        u2_noun roc;                      //  rotor core

        union {
          struct { uint64_t a; uint64_t b; };
          struct {
            struct _u2_cart* egg_u;         //  exit of ovum queue
            struct _u2_cart* geg_u;         //  entry of ovum queue
          } ova;
        };
      } u2_reck;

  /**   Global variables.
  **/
#define  u2_Arv  ((u2_reck*)u2_at_cord(u2_wire_arv_r(u2_Wire), sizeof(u2_reck)))
#define  u2A      u2_Arv

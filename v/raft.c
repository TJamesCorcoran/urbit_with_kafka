/* v/raft.c
**
** This file is in the public domain.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "all.h"
#include "v/vere.h"
#include "v/sist.h"
#include "v/egzh.h"
#include "v/kafk.h"


/* u2_rent: Log entry wire format.
*/
typedef struct {
  c3_w             tem_w;                               //  Log entry term
  c3_w             typ_w;                               //  Entry type, %ra|%ov
  c3_w             len_w;                               //  Word length of blob
  c3_w*            bob_w;                               //  Blob
} u2_rent;

/* u2_rmsg: Raft RPC wire format.
*/
typedef struct _u2_rmsg {
  c3_w             ver_w;                               //  version, mug('a')...
  c3_d             len_d;                               //  Words in message
  c3_w             tem_w;                               //  Current term
  c3_w             typ_w;                               //  %apen|%revo|%rasp
  union {
    struct {
      c3_w         suc_w;                               //  Request successful
    } rasp;
    struct {
      c3_d         lai_d;                               //  Last log index
      c3_w         lat_w;                               //  Last log term
      c3_w         nam_w;                               //  Name word length
      c3_c*        nam_c;                               //  Requestor name
      union {
        struct {
          c3_d     cit_d;                               //  Leader commitIndex
          c3_d     ent_d;                               //  Number of entries
          u2_rent* ent_u;                               //  Entries
        } apen;
      };
    } rest;
  };
} u2_rmsg;


static ssize_t _raft_rmsg_read(const u2_rbuf* buf_u, u2_rmsg* msg_u);
static void _raft_rmsg_send(u2_rcon* ron_u, const u2_rmsg* msg_u);
static void _raft_rmsg_free(u2_rmsg* msg_u);
static void _raft_conn_dead(u2_rcon* ron_u);
static u2_bean _raft_remove_run(u2_rcon* ron_u);
static void _raft_send_rasp(u2_rcon* ron_u, c3_t suc_t);
static void _raft_rreq_free(u2_rreq* req_u);
static void _raft_time_cb(uv_timer_t* tim_u, c3_i sas_i);
static void _raft_sure_guard(u2_reck* rec_u, u2_noun ovo, u2_noun vir, u2_noun cor, c3_t force_delay);

static void
_raft_rnam_free(u2_rnam* nam_u)
{
  if ( nam_u ) {
    c3_assert(0 == nam_u->ron_u);
    c3_assert(0 == nam_u->nex_u);
    free(nam_u->str_c);
    free(nam_u->nam_c);
    free(nam_u->por_c);
  }
  free(nam_u);
}

/* _raft_readname(): parse a raft host:port peer name.
*/
static u2_rnam*
_raft_readname(const c3_c* str_c, c3_w siz_w)
{
  u2_rnam* nam_u = calloc(1, sizeof(*nam_u));
  c3_c*    col_c;
  c3_w     nam_w;

  nam_u->str_c = c3_malloc(siz_w + 1);
  strncpy(nam_u->str_c, str_c, siz_w);
  nam_u->str_c[siz_w] = '\0';

  if ( 0 == (col_c = strchr(nam_u->str_c, ':')) ) {
    uL(fprintf(uH, "raft: invalid name %s\n", nam_u->str_c));
    _raft_rnam_free(nam_u);
    nam_u = 0;
  }
  else {
    nam_w = col_c - nam_u->str_c + 1;
    nam_u->nam_c = c3_malloc(nam_w);
    uv_strlcpy(nam_u->nam_c, nam_u->str_c, nam_w);
    nam_u->por_c = strdup(col_c + 1);
  }
  return nam_u;
}

/* u2_raft_readopt(): parse a string into a list of raft peers.
*/
u2_rnam*
u2_raft_readopt(const c3_c* arg_c, c3_c* our_c, c3_s oup_s)
{
  u2_rnam* nam_u;
  u2_rnam* nex_u;
  c3_c*    com_c;

  if ( 0 == (com_c = strchr(arg_c, ',')) ) {
    nam_u = _raft_readname(arg_c, strlen(arg_c));
    nex_u = 0;
  }
  else {
    nam_u = _raft_readname(arg_c, com_c - arg_c);
    nex_u = u2_raft_readopt(com_c + 1, our_c, oup_s);
  }

  if ( nam_u ) {
    c3_c* end_c;
    c3_w  por_w = strtoul(nam_u->por_c, &end_c, 10);

    if ( '\0' == *nam_u->por_c || '\0' != *end_c || por_w >= 65536 ) {
      uL(fprintf(uH, "raft: invalid port %s\n", nam_u->por_c));
      _raft_rnam_free(nam_u);
      _raft_rnam_free(nex_u);
      nam_u = 0;
    }
    else {
      if ( oup_s == por_w && 0 == strcmp(our_c, nam_u->nam_c) ) {
        _raft_rnam_free(nam_u);
        nam_u = nex_u;
      }
      else nam_u->nex_u = nex_u;
    }
  }
  else _raft_rnam_free(nex_u);
  return nam_u;
}

/* _raft_alloc(): libuv-style allocator for raft.
*/
static uv_buf_t
_raft_alloc(uv_handle_t* had_u, size_t siz_i)
{
  uv_buf_t buf_u = { .base = c3_malloc(siz_i), .len = siz_i };
  return buf_u;
}

/* _raft_election_rand(): election timeout.
*/
static c3_w
_raft_election_rand()
{
  c3_w ret = (1.0 + (float) rand() / RAND_MAX) * 150;
  //uL(fprintf(uH, "raft: timeout %d\n", ret));
  return ret;
}

/* _raft_promote(): actions on raft leader election.
*/
static void
_raft_promote(u2_raft* raf_u)
{
  if ( u2_raty_lead == raf_u->typ_e ) {
    uL(fprintf(uH, "raft: double promote; ignoring\n"));
  }
  else {
    c3_i sas_i;

    if ( 1 == raf_u->pop_w ) {
      uL(fprintf(uH, "raft:      -> lead\n"));
      raf_u->typ_e = u2_raty_lead;
      //  TODO boot in multiuser mode
      u2_sist_boot();
      if ( u2_no == u2_Host.ops_u.bat ) {
        u2_lo_lead(u2A);
      }
    }
    else {
      c3_assert(u2_raty_cand == raf_u->typ_e);
      uL(fprintf(uH, "raft: cand -> lead\n"));
      raf_u->typ_e = u2_raty_lead;

      sas_i = uv_timer_stop(&raf_u->tim_u);
      c3_assert(0 == sas_i);
      sas_i = uv_timer_start(&raf_u->tim_u, _raft_time_cb, 50, 50);
      c3_assert(0 == sas_i);
    }
  }
}

/* _raft_demote(): demote to follower.
*/
static void
_raft_demote(u2_raft* raf_u)
{
  u2_raty typ_e = raf_u->typ_e;

  raf_u->vog_c = 0;
  u2_sist_nil("vote");
  raf_u->vot_w = 0;
  raf_u->typ_e = u2_raty_foll;

  if ( u2_raty_lead == typ_e ) {
    c3_i sas_i;

    uL(fprintf(uH, "raft: lead -> foll\n"));
    sas_i = uv_timer_stop(&raf_u->tim_u);
    c3_assert(0 == sas_i);
    sas_i = uv_timer_start(&raf_u->tim_u, _raft_time_cb,
                           _raft_election_rand(), 0);
    c3_assert(0 == sas_i);
    //  TODO dump not-yet-committed events
  }
  else {
    c3_assert(u2_raty_cand == typ_e);
    uL(fprintf(uH, "raft: cand -> foll\n"));
  }
}

/* _raft_note_term(): note a term from the network, demoting if it is newer.
*/
static void
_raft_note_term(u2_raft* raf_u, c3_w tem_w)
{
  if ( raf_u->tem_w < tem_w ) {
    uL(fprintf(uH, "raft: got term from network: %d\n", tem_w));
    raf_u->tem_w = tem_w;
    u2_sist_put("term", (c3_y*)&raf_u->tem_w, sizeof(c3_w));
    c3_assert(raf_u->typ_e != u2_raty_none);
    if ( raf_u->typ_e == u2_raty_foll ) {
      c3_assert(0 == raf_u->vot_w);
    } else _raft_demote(raf_u);
  }
}

/* _raft_rest_name(): update conn name from incoming request.
**
** If this connection already has a name, make sure the passed name
** matches.  Otherwise, try to associate it with a name, killing old
** connections to that name.
*/
static void  //  TODO indicate whether conn died
_raft_rest_name(u2_rcon* ron_u, const c3_c* nam_c)
{
  if ( 0 != ron_u->nam_u ) {
    if ( 0 != strcmp(ron_u->nam_u->str_c, nam_c) ) {
      uL(fprintf(uH, "raft: names disagree o:%s n:%s\n",
                     ron_u->nam_u->str_c, nam_c));
      _raft_conn_dead(ron_u);
    }
  }
  else {
    u2_raft* raf_u = ron_u->raf_u;
    u2_rnam* nam_u = raf_u->nam_u;

    while ( nam_u ) {
      if ( 0 == strcmp(nam_u->str_c, nam_c) ) {
        if ( nam_u->ron_u ) {
          c3_assert(nam_u->ron_u != ron_u);
          //uL(fprintf(uH, "raft: closing old conn %p to %s (%p)\n",
          //               nam_u->ron_u, nam_u->str_c, ron_u));
          _raft_conn_dead(nam_u->ron_u);
        }
        uL(fprintf(uH, "raft: incoming conn from %s\n", nam_u->str_c));
        nam_u->ron_u = ron_u;
        ron_u->nam_u = nam_u;
        _raft_remove_run(ron_u);
        break;
      }
      else nam_u = nam_u->nex_u;
    }
    if ( 0 == ron_u->nam_u ) {
      uL(fprintf(uH, "connection from unkown peer %s\n", nam_c));
      _raft_conn_dead(ron_u);
    }
  }
}

/* _raft_do_rest(): effects of an incoming request.
*/
static void
_raft_do_rest(u2_rcon* ron_u, const u2_rmsg* msg_u)
{
  u2_raft* raf_u = ron_u->raf_u;

  if ( u2_raty_cand == raf_u->typ_e || u2_raty_foll == raf_u->typ_e ) {
    c3_i sas_i;

    sas_i = uv_timer_stop(&raf_u->tim_u);
    c3_assert(0 == sas_i);
    sas_i = uv_timer_start(&raf_u->tim_u, _raft_time_cb,
                           _raft_election_rand(), 0);
    c3_assert(0 == sas_i);
  }

  _raft_rest_name(ron_u, msg_u->rest.nam_c);
  _raft_note_term(raf_u, msg_u->tem_w);
}

/* _raft_do_apen(): Handle incoming AppendEntries.
*/
static void
_raft_do_apen(u2_rcon* ron_u, const u2_rmsg* msg_u)
{
  c3_assert(c3__apen == msg_u->typ_w);
  _raft_do_rest(ron_u, msg_u);
  /* TODO respond */
}

/* _raft_apen_done(): process AppendEntries response.
*/
static void
_raft_apen_done(u2_rreq* req_u, c3_w suc_w)
{
  c3_assert(c3__apen == req_u->msg_u->typ_w);
  /* TODO */
}

/* _raft_do_revo(): Handle incoming RequestVote.
*/
static void
_raft_do_revo(u2_rcon* ron_u, const u2_rmsg* msg_u)
{
  u2_raft* raf_u = ron_u->raf_u;

  c3_assert(c3__revo == msg_u->typ_w);
  _raft_do_rest(ron_u, msg_u);

  c3_assert(0 != ron_u->nam_u);
  if ( msg_u->tem_w >= raf_u->tem_w                     &&
       (0 == raf_u->vog_c                             ||
        0 == strcmp(raf_u->vog_c, ron_u->nam_u->str_c)) &&
       (raf_u->lat_w < msg_u->rest.lat_w              ||
        (raf_u->lat_w == msg_u->rest.lat_w          &&
         raf_u->ent_d <= msg_u->rest.lai_d)) )
  {
    raf_u->vog_c = ron_u->nam_u->str_c;
    u2_sist_put("vote", (c3_y*)raf_u->vog_c, strlen(raf_u->vog_c));
    uL(fprintf(uH, "raft: granting vote to %s\n", raf_u->vog_c));
    _raft_send_rasp(ron_u, 1);
  }
  else _raft_send_rasp(ron_u, 0);
}

/* _raft_revo_done(): process RequestVote response.
*/
static void
_raft_revo_done(u2_rreq* req_u, c3_w suc_w)
{
  u2_rcon* ron_u = req_u->ron_u;
  u2_raft* raf_u = ron_u->raf_u;

  c3_assert(c3__revo == req_u->msg_u->typ_w);
  if ( suc_w && req_u->msg_u->tem_w == raf_u->tem_w ) {
    if ( u2_no == ron_u->nam_u->vog ) {
      ron_u->nam_u->vog = u2_yes;
      raf_u->vot_w++;
    }
    else {
      uL(fprintf(uH, "XX raft: duplicate response for %s [tem:%d]\n",
                     ron_u->nam_u->str_c, raf_u->tem_w));
    }
  }
  if ( raf_u->vot_w > raf_u->pop_w / 2 ) {
    uL(fprintf(uH, "raft: got majority of %d for term %d\n",
                   raf_u->vot_w, raf_u->tem_w));
    _raft_promote(raf_u);
  }
}

/* _raft_do_rasp(): act on an incoming raft RPC response.
*/
static void
_raft_do_rasp(u2_rcon* ron_u, u2_rmsg* msg_u)
{
  u2_raft* raf_u = ron_u->raf_u;

  c3_assert(c3__rasp == msg_u->typ_w);
  if ( 0 == ron_u->nam_u ) {
    uL(fprintf(uH, "raft: invalid connection from unknown host\n"));
    _raft_conn_dead(ron_u);
  }
  else {
    u2_rreq* req_u = ron_u->out_u;

    if ( !req_u ) {
      uL(fprintf(uH, "raft: response with no request from %s\n",
                     ron_u->nam_u->str_c));
      _raft_conn_dead(ron_u);
    }
    else {
      switch ( req_u->msg_u->typ_w ) {
        default: {
          uL(fprintf(uH, "raft: bogus request type %x?!\n",
                         req_u->msg_u->typ_w));
          c3_assert(0);
        }
        case c3__apen: {
          _raft_apen_done(req_u, msg_u->rasp.suc_w);
          break;
        }
        case c3__revo: {
          _raft_revo_done(req_u, msg_u->rasp.suc_w);
          break;
        }
      }

      _raft_note_term(raf_u, msg_u->tem_w);

      ron_u->out_u = req_u->nex_u;
      if ( 0 == req_u->nex_u ) {
        c3_assert(req_u == ron_u->tou_u);
        ron_u->tou_u = 0;
      }
      _raft_rreq_free(req_u);
    }
  }
}

/* _raft_rmsg_read(): read a u2_rmsg from a buffer.
**
** Returns <0 on parse failure.
** Returns 0 on partial data.
** Returns bytes read on successful read.
**
** If successful, caller must eventually call _raft_free_rmsg() on msg_u.
*/
static ssize_t
_raft_rmsg_read(const u2_rbuf* buf_u, u2_rmsg* msg_u)
{
  ssize_t red_i = 0;
  c3_d    ben_d;

  if ( buf_u->len_w < sizeof(c3_w) + sizeof(c3_d) ) {
    return 0;
  }
  memcpy(&msg_u->ver_w, buf_u->buf_y + red_i, sizeof(c3_w));
  red_i += sizeof(c3_w);
  if ( msg_u->ver_w != u2_cr_mug('a') ) {
    uL(fprintf(uH, "raft: versions don't match: %x %x\n",
                   msg_u->ver_w, u2_cr_mug('a')));
    return -1;
  }

  memcpy(&msg_u->len_d, buf_u->buf_y + red_i, sizeof(c3_d));
  red_i += sizeof(c3_d);

  if ( msg_u->len_d < 4 ) {
    uL(fprintf(uH, "raft: length too short (a) %llu\n", ( unsigned long long int) msg_u->len_d));
    return -1;
  }

  ben_d = 4ULL * msg_u->len_d;

  if ( buf_u->len_w < ben_d ) {
    return 0;
  }

  if ( ben_d < red_i + 2 * sizeof(c3_w) ) {
    uL(fprintf(uH, "raft: length too short (b) %llu\n", (unsigned long long int) msg_u->len_d));
    return -1;
  }
  memcpy(&msg_u->tem_w, buf_u->buf_y + red_i, sizeof(c3_w));
  red_i += sizeof(c3_w);
  memcpy(&msg_u->typ_w, buf_u->buf_y + red_i, sizeof(c3_w));
  red_i += sizeof(c3_w);

  switch ( msg_u->typ_w ) {
    default: {
      uL(fprintf(uH, "raft: unknown msg type %x\n", msg_u->typ_w));
      return -1;
    }
    case c3__rasp: {
      if ( ben_d < red_i + sizeof(c3_w) ) {
        uL(fprintf(uH, "raft: length too short (c) %llu\n", ( unsigned long long int)msg_u->len_d));
        return -1;
      }
      memcpy(&msg_u->rasp.suc_w, buf_u->buf_y + red_i, sizeof(c3_w));
      red_i += sizeof(c3_w);
      break;
    }
    case c3__apen: case c3__revo: {
      if ( ben_d < red_i + sizeof(c3_d) + 2 * sizeof(c3_w) ) {
        uL(fprintf(uH, "raft: length too short (d) %llu\n", ( unsigned long long int)msg_u->len_d));
        return -1;
      }
      memcpy(&msg_u->rest.lai_d, buf_u->buf_y + red_i, sizeof(c3_d));
      red_i += sizeof(c3_d);
      memcpy(&msg_u->rest.lat_w, buf_u->buf_y + red_i, sizeof(c3_w));
      red_i += sizeof(c3_w);
      memcpy(&msg_u->rest.nam_w, buf_u->buf_y + red_i, sizeof(c3_w));
      red_i += sizeof(c3_w);

      if ( ben_d < red_i + 4 * msg_u->rest.nam_w ) {
        uL(fprintf(uH, "raft: length too short (e) %llu\n", ( unsigned long long int)msg_u->len_d));
        return -1;
      }
      msg_u->rest.nam_c = c3_malloc(4 * msg_u->rest.nam_w);
      uv_strlcpy(msg_u->rest.nam_c, (const char*)(buf_u->buf_y + red_i),
                 4 * msg_u->rest.nam_w);
      red_i += 4 * msg_u->rest.nam_w;
      break;
    }
  }

  if ( c3__apen == msg_u->typ_w ) {
    if ( ben_d < red_i + 2 * sizeof(c3_d) ) {
      uL(fprintf(uH, "raft: length too short (f) %llu\n", ( unsigned long long int)msg_u->len_d));
      red_i = -1;
      goto fail;
    }
    memcpy(&msg_u->rest.apen.cit_d, buf_u->buf_y + red_i, sizeof(c3_d));
    red_i += sizeof(c3_d);
    memcpy(&msg_u->rest.apen.ent_d, buf_u->buf_y + red_i, sizeof(c3_d));
    red_i += sizeof(c3_d);

    msg_u->rest.apen.ent_u = calloc(
        1, msg_u->rest.apen.ent_d * sizeof(u2_rent));
    {
      c3_d     i_d;
      u2_rent* ent_u = msg_u->rest.apen.ent_u;

      for ( i_d = 0; i_d < msg_u->rest.apen.ent_d; i_d++ ) {
        if ( ben_d < red_i + 3 * sizeof(c3_w) ) {
          uL(fprintf(uH, "raft: length too short (g) %llu\n", ( unsigned long long int)msg_u->len_d));
          red_i = -1;
          goto fail;
        }
        memcpy(&ent_u[i_d].tem_w, buf_u->buf_y + red_i, sizeof(c3_w));
        red_i += sizeof(c3_w);
        memcpy(&ent_u[i_d].typ_w, buf_u->buf_y + red_i, sizeof(c3_w));
        red_i += sizeof(c3_w);
        memcpy(&ent_u[i_d].len_w, buf_u->buf_y + red_i, sizeof(c3_w));
        red_i += sizeof(c3_w);
        if ( ben_d < red_i + 4 * ent_u[i_d].len_w ) {
          uL(fprintf(uH, "raft: length too short (h) %llu\n", ( unsigned long long int)msg_u->len_d));
          red_i = -1;
          goto fail;
        }
        ent_u[i_d].bob_w = c3_malloc(4 * ent_u[i_d].len_w);
        memcpy(ent_u[i_d].bob_w, buf_u->buf_y + red_i, 4 * ent_u[i_d].len_w);
        red_i += 4 * ent_u[i_d].len_w;
      }
    }
  }

  if ( red_i != ben_d ) {
    uL(fprintf(uH, "raft: sizes don't match r:%ld w:%llu\n", (long int) red_i, ( unsigned long long int)ben_d));
    red_i = -1;
    goto fail;
  }

out:
  return red_i;
fail:
  _raft_rmsg_free(msg_u);
  goto out;
}

/* _raft_rbuf_grow(): append data to the buffer, reallocating if needed.
**
** Returns new buffer location, as realloc.
*/
static u2_rbuf*
_raft_rbuf_grow(u2_rbuf* buf_u, const c3_y* buf_y, size_t siz_i)
{
  if ( 0 == buf_u ) {
    buf_u = c3_malloc(sizeof(*buf_u) + siz_i);
    buf_u->len_w = 0;
    buf_u->cap_w = siz_i;
  }

  if ( buf_u->cap_w < buf_u->len_w + siz_i ) {
    c3_w cap_w = c3_max(2 * buf_u->cap_w, buf_u->len_w + siz_i);

    buf_u = realloc(buf_u, sizeof(*buf_u) + cap_w);
    buf_u->cap_w = cap_w;
  }

  memcpy(buf_u->buf_y + buf_u->len_w, buf_y, siz_i);
  buf_u->len_w += siz_i;
  return buf_u;
}

/* _raft_bytes_send():
*/
static void
_raft_bytes_send(u2_rcon* ron_u, const void* ptr_v, size_t siz_i)
{
  ron_u->wri_u = _raft_rbuf_grow(ron_u->wri_u, ptr_v, siz_i);
}

/* _raft_rmsg_send(): send a u2_rmsg over the wire.
*/
static void
_raft_rmsg_send(u2_rcon* ron_u, const u2_rmsg* msg_u)
{
  c3_d len_d = sizeof(c3_d) + 3 * sizeof(c3_w);

  _raft_bytes_send(ron_u, &msg_u->ver_w, sizeof(c3_w));
  _raft_bytes_send(ron_u, &msg_u->len_d, sizeof(c3_d));
  _raft_bytes_send(ron_u, &msg_u->tem_w, sizeof(c3_w));
  _raft_bytes_send(ron_u, &msg_u->typ_w, sizeof(c3_w));
  switch ( msg_u->typ_w ) {
    default: {
      uL(fprintf(uH, "raft: send: unknown message type\n"));
      c3_assert(0);
    }
    case c3__rasp: {
      len_d += sizeof(c3_w);
      _raft_bytes_send(ron_u, &msg_u->rasp.suc_w, sizeof(c3_w));
      break;
    }
    case c3__apen: case c3__revo: {
      len_d += sizeof(c3_d) + 2 * sizeof(c3_w) + 4 * msg_u->rest.nam_w;
      _raft_bytes_send(ron_u, &msg_u->rest.lai_d, sizeof(c3_d));
      _raft_bytes_send(ron_u, &msg_u->rest.lat_w, sizeof(c3_w));
      _raft_bytes_send(ron_u, &msg_u->rest.nam_w, sizeof(c3_w));
      _raft_bytes_send(ron_u, msg_u->rest.nam_c, 4 * msg_u->rest.nam_w);
      break;
    }
  }
  if ( c3__apen == msg_u->typ_w ) {
    c3_d     i_d;
    u2_rent* ent_u = msg_u->rest.apen.ent_u;

    len_d += 2 * sizeof(c3_d);
    _raft_bytes_send(ron_u, &msg_u->rest.apen.cit_d, sizeof(c3_d));
    _raft_bytes_send(ron_u, &msg_u->rest.apen.ent_d, sizeof(c3_d));
    for ( i_d = 0; i_d < msg_u->rest.apen.ent_d; i_d++ ) {
      len_d += 3 * sizeof(c3_w) + ent_u[i_d].len_w;
      _raft_bytes_send(ron_u, &ent_u[i_d].tem_w, sizeof(c3_w));
      _raft_bytes_send(ron_u, &ent_u[i_d].typ_w, sizeof(c3_w));
      _raft_bytes_send(ron_u, &ent_u[i_d].len_w, sizeof(c3_w));
      _raft_bytes_send(ron_u, ent_u[i_d].bob_w, ent_u[i_d].len_w);
    }
  }

  //uL(fprintf(uH, "raft: sent %llu (%llu) [%x]\n",
  //               len_d, msg_u->len_d, msg_u->typ_w));
  c3_assert(len_d == 4 * msg_u->len_d);
}

/* _raft_rmsg_free(): free a u2_rmsg's resources (but not the msg itself).
*/
static void
_raft_rmsg_free(u2_rmsg* msg_u) {
  if ( c3__apen == msg_u->typ_w && msg_u->rest.apen.ent_u ) {
    c3_d i_d;

    for ( i_d = 0; i_d < msg_u->rest.apen.ent_d; i_d++ ) {
      free(msg_u->rest.apen.ent_u[i_d].bob_w);
    }
    free(msg_u->rest.apen.ent_u);
    msg_u->rest.apen.ent_u = 0;
  }
  if ( c3__apen == msg_u->typ_w || c3__revo == msg_u->typ_w ) {
    free(msg_u->rest.nam_c);
    msg_u->rest.nam_c = 0;
  }
}

/* An unusual lameness in libuv.
*/
struct _u2_write_t {
  uv_write_t wri_u;
  c3_y*      buf_y;
};

/* _raft_write_cb(): generic write callback.
*/
static void
_raft_write_cb(uv_write_t* wri_u, c3_i sas_i)
{
  struct _u2_write_t* req_u = (struct _u2_write_t*)wri_u;

  if ( 0 != sas_i ) {
    uL(fprintf(uH, "raft: write_cb: %s\n",
                   uv_strerror(uv_last_error(u2L))));
    _raft_conn_dead((u2_rcon*)wri_u->handle);
  }
  free(req_u->buf_y);
  free(req_u);
}

/* _raft_conn_work(): read and write requests and responses.
*/
static void
_raft_conn_work(u2_rcon* ron_u)
{
  c3_assert(u2_yes == ron_u->liv);
  if ( u2_yes == ron_u->red ) {
    c3_assert(ron_u->red_u);
    ron_u->red = u2_no;
    while (1) {
      u2_rmsg msg_u;
      ssize_t ret_i = _raft_rmsg_read(ron_u->red_u, &msg_u);

      if ( ret_i < 0 ) {
        if ( ron_u->nam_u ) {
          uL(fprintf(uH, "raft: conn_work: error reading from %s\n",
                         ron_u->nam_u->str_c));
        }
        else {
          uL(fprintf(uH, "raft: conn_work: error reading\n"));
        }
        _raft_conn_dead(ron_u);
        break;
      }
      else if ( ret_i == 0 ) {
        break;
      }
      else {
        if ( 4 * msg_u.len_d != ret_i ) {
          uL(fprintf(uH, "raft: conn_work: lengths don't match\n"));
          c3_assert(0);
        }
        else {
          c3_assert(ron_u->red_u->len_w >= ret_i);
          memmove(ron_u->red_u->buf_y,
                  ron_u->red_u->buf_y + ret_i,
                  ron_u->red_u->len_w - ret_i);
          ron_u->red_u->len_w -= ret_i;

          switch ( msg_u.typ_w ) {
            default: {
              uL(fprintf(uH, "raft: work: unknown message type %x\n",
                             msg_u.typ_w));
              break;
            }
            case c3__apen: {
              _raft_do_apen(ron_u, &msg_u);
              break;
            }
            case c3__revo: {
              _raft_do_revo(ron_u, &msg_u);
              break;
            }
            case c3__rasp: {
              _raft_do_rasp(ron_u, &msg_u);
              break;
            }
          }
          _raft_rmsg_free(&msg_u);
        }
      }
    }
  }

  if ( ron_u->wri_u && ron_u->wri_u->len_w > 0 ) {
    uv_buf_t            buf_u;
    struct _u2_write_t* req_u = c3_malloc(sizeof(*req_u));


    req_u->buf_y = c3_malloc(ron_u->wri_u->len_w);
    memcpy(req_u->buf_y, ron_u->wri_u->buf_y, ron_u->wri_u->len_w);
    buf_u.base = (char*)req_u->buf_y;
    buf_u.len = ron_u->wri_u->len_w;

    if ( 0 != uv_write((uv_write_t*)req_u,
                       (uv_stream_t*)&ron_u->wax_u,
                       &buf_u,
                       1,
                       _raft_write_cb) )
    {
      uL(fprintf(uH, "raft: conn_work (write): %s\n",
                     uv_strerror(uv_last_error(u2L))));
      free(req_u->buf_y);
      free(req_u);
    }
    else {
      ron_u->wri_u->len_w = 0;
    }
  }
}

/* _raft_conn_read_cb(): generic connection read callback.
*/
static void
_raft_conn_read_cb(uv_stream_t* tcp_u,
                   ssize_t      siz_i,
                   uv_buf_t     buf_u)
{
  u2_rcon* ron_u = (u2_rcon*)tcp_u;

  u2_lo_open();
  {
    if ( siz_i < 0 ) {
      uv_err_t las_u = uv_last_error(u2L);

      if ( UV_EOF != las_u.code ) {
        uL(fprintf(uH, "raft: read: %s\n", uv_strerror(las_u)));
      }
      _raft_conn_dead(ron_u);
    }
    else if ( siz_i == 0 ) {
      //  do nothing
    }
    else {
      if ( u2_yes == ron_u->liv ) {
        ron_u->red_u = _raft_rbuf_grow(ron_u->red_u, (c3_y*)buf_u.base, siz_i);
        ron_u->red = u2_yes;
        _raft_conn_work(ron_u);
      }
      else uL(fprintf(uH, "XX raft: read on dead conn %p\n", ron_u));
    }
  }
  free(buf_u.base);
  u2_lo_shut(u2_no);
}

/* _raft_conn_new(): allocate a new raft connection.
*/
static u2_rcon*
_raft_conn_new(u2_raft* raf_u)
{
  u2_rcon* ron_u = c3_malloc(sizeof(*ron_u));

  uv_tcp_init(u2L, &ron_u->wax_u);
  ron_u->red_u = 0;
  ron_u->out_u = ron_u->tou_u = 0;
  ron_u->red_u = 0;
  ron_u->red = u2_no;
  ron_u->wri_u = 0;
  ron_u->nam_u = 0;
  ron_u->raf_u = raf_u;
  ron_u->nex_u = 0;
  ron_u->liv = u2_no;

  return ron_u;
}

/* _raft_remove_run(): remove a connection from the list of unknowns.
*/
static u2_bean
_raft_remove_run(u2_rcon* ron_u)
{
  u2_raft* raf_u = ron_u->raf_u;
  u2_bean  suc = u2_no;

  if ( raf_u->run_u == ron_u ) {
    raf_u->run_u = ron_u->nex_u;
    suc = u2_yes;
  }
  else {
    u2_rcon* pre_u = raf_u->run_u;

    while ( pre_u ) {
      if ( pre_u->nex_u == ron_u ) {
        pre_u->nex_u = ron_u->nex_u;
        suc = u2_yes;
        break;
      }
      else pre_u = pre_u->nex_u;
    }
  }

  return suc;
}

static u2_rreq*
_raft_rreq_new(u2_rcon* ron_u)
{
  u2_rreq* req_u = c3_malloc(sizeof(*req_u));

  req_u->msg_u = c3_malloc(sizeof(*req_u->msg_u));
  req_u->nex_u = 0;
  req_u->ron_u = ron_u;
  if ( ron_u->tou_u ) {
    c3_assert(ron_u->out_u);
    ron_u->tou_u->nex_u = req_u;
    ron_u->tou_u = req_u;
  }
  else {
    c3_assert(0 == ron_u->out_u);
    ron_u->tou_u = ron_u->out_u = req_u;
  }
  return req_u;
}

static void
_raft_rreq_free(u2_rreq* req_u)
{
  _raft_rmsg_free(req_u->msg_u);
  free(req_u->msg_u);   //  XX
  free(req_u);
}

/* _raft_conn_free(): unlink a connection and free its resources.
*/
static void
_raft_conn_free(uv_handle_t* had_u)
{
  u2_rcon* ron_u = (void*)had_u;
  u2_raft* raf_u = ron_u->raf_u;

  //uL(fprintf(uH, "raft: conn_free %p\n", ron_u));

  //  Unlink references.
  if ( ron_u->nam_u ) {
    c3_assert(u2_no == _raft_remove_run(ron_u));
    if ( ron_u->nam_u->ron_u == ron_u ) {
      ron_u->nam_u->ron_u = 0;
    }
  }
  else {
    u2_bean suc = _raft_remove_run(ron_u);
    c3_assert(u2_yes == suc);
    //  Slow, expensive debug assert.
    {
      u2_rnam* nam_u = raf_u->nam_u;

      while ( nam_u ) {
        c3_assert(nam_u->ron_u != ron_u);
        nam_u = nam_u->nex_u;
      }
    }
  }

  //  Free requests.
  {
    u2_rreq* req_u = ron_u->out_u;

    if ( 0 == req_u ) {
      c3_assert(0 == ron_u->tou_u);
    }
    else {
      while ( req_u ) {
        if ( 0 == req_u->nex_u ) {
          c3_assert(req_u == ron_u->tou_u);
        }
        ron_u->out_u = req_u->nex_u;
        _raft_rreq_free(req_u);
        req_u = ron_u->out_u;
      }
    }
  }
  free(ron_u->red_u);
  free(ron_u->wri_u);
  free(ron_u);
}

/* _raft_conn_dead(): kill a connection.
*/
static void
_raft_conn_dead(u2_rcon* ron_u)
{
  if ( u2_no == ron_u->liv ) {
    //uL(fprintf(uH, "raft: conn already dead %p\n", ron_u));
    return;
  }
  else {
    uL(fprintf(uH, "raft: conn_dead %p\n", ron_u));
    ron_u->liv = u2_no;
  }

  uv_read_stop((uv_stream_t*)&ron_u->wax_u);
  uv_close((uv_handle_t*)&ron_u->wax_u, _raft_conn_free);
}

/* _raft_listen_cb(): generic listen callback.
*/
static void
_raft_listen_cb(uv_stream_t* str_u, c3_i sas_i)
{
  u2_raft* raf_u = (u2_raft*)str_u;

  if ( 0 != sas_i ) {
    uL(fprintf(uH, "raft: listen_cb: %s\n",
                   uv_strerror(uv_last_error(u2L))));
  }
  else {
    u2_rcon* ron_u = _raft_conn_new(raf_u);

    if ( 0 != uv_accept((uv_stream_t*)&raf_u->wax_u,
                        (uv_stream_t*)&ron_u->wax_u) )
    {
      uL(fprintf(uH, "raft: accept: %s\n",
                     uv_strerror(uv_last_error(u2L))));

      uv_close((uv_handle_t*)&ron_u->wax_u, 0);
      free(ron_u);
    }
    else {
      ron_u->liv = u2_yes;

      uv_read_start((uv_stream_t*)&ron_u->wax_u,
                    _raft_alloc,
                    _raft_conn_read_cb);

      ron_u->nex_u = raf_u->run_u;
      raf_u->run_u = ron_u;
    }
  }
}

/* _raft_connect_cb(): generic connection callback.
*/
static void
_raft_connect_cb(uv_connect_t* con_u, c3_i sas_i)
{
  u2_rcon* ron_u = con_u->data;
  free(con_u);

  if ( 0 != sas_i ) {
    uL(fprintf(uH, "raft: connect_cb: %s\n",
                   uv_strerror(uv_last_error(u2L))));
    uv_close((uv_handle_t*)&ron_u->wax_u, _raft_conn_free);
  }
  else {
    c3_assert(ron_u->nam_u);
    uL(fprintf(uH, "raft: connected to %s\n", ron_u->nam_u->str_c));
    ron_u->liv = u2_yes;

    uv_read_start((uv_stream_t*)&ron_u->wax_u,
                  _raft_alloc,
                  _raft_conn_read_cb);

    _raft_conn_work(ron_u);
  }
}

/* _raft_getaddrinfo_cb(): generic getaddrinfo callback.
*/
static void
_raft_getaddrinfo_cb(uv_getaddrinfo_t* raq_u,
                     c3_i              sas_i,
                     struct addrinfo*  add_u)
{
  struct addrinfo* res_u;
  uv_connect_t*    con_u = c3_malloc(sizeof(*con_u));
  u2_rcon*         ron_u = raq_u->data;

  //uL(fprintf(uH, "getaddrinfo_cb %s\n", ron_u->nam_u->nam_c));

  con_u->data = ron_u;
  for ( res_u = add_u; res_u; res_u = res_u->ai_next ) {
    if ( 0 != uv_tcp_connect(con_u,
                             &ron_u->wax_u,
                             *(struct sockaddr_in*)res_u->ai_addr,
                             _raft_connect_cb) )
    {
      uL(fprintf(uH, "raft: getaddrinfo_cb: %s\n",
                     uv_strerror(uv_last_error(u2L))));
      uv_close((uv_handle_t*)&ron_u->wax_u, 0);
      continue;
    }
    else {
#if 0
      c3_c  add_c[17] = {'\0'};

      uv_ip4_name((struct sockaddr_in*)res_u->ai_addr, add_c, 16);
      uL(fprintf(uH, "raft: conn %s\n", add_c));
#endif
      break;                                            //  Found one
    }
  }
  if ( !res_u ) {
    uL(fprintf(uH, "raft: getaddrinfo_cb: no address matched\n"));
    _raft_conn_free((uv_handle_t*)&ron_u->wax_u);
    free(con_u);
  }
  uv_freeaddrinfo(add_u);
  free(raq_u);
}

/* _raft_conn_all(): ensure that we are connected to each peer.
*/
static void
_raft_conn_all(u2_raft* raf_u, void (*con_f)(u2_rcon* ron_u))
{
  u2_rnam* nam_u = raf_u->nam_u;
  u2_rcon* ron_u;

  while ( nam_u ) {
    if ( 0 == nam_u->ron_u || u2_no == nam_u->ron_u->liv ) {
      struct addrinfo   hit_u;
      uv_getaddrinfo_t* raq_u = c3_malloc(sizeof(*raq_u));

      ron_u = _raft_conn_new(raf_u);

      //uL(fprintf(uH, "raft: new conn to %s:%s %p\n",
      //               nam_u->nam_c, nam_u->por_c, ron_u));

      memset(&hit_u, 0, sizeof(hit_u));
      hit_u.ai_family = AF_INET;
      hit_u.ai_socktype = SOCK_STREAM;
      hit_u.ai_protocol = IPPROTO_TCP;

      raq_u->data = ron_u;

      if ( 0 != uv_getaddrinfo(u2L,
                               raq_u,
                               _raft_getaddrinfo_cb,
                               nam_u->nam_c,
                               nam_u->por_c,
                               &hit_u) )
      {
        uL(fprintf(uH, "raft: getaddrinfo: %s\n",
                       uv_strerror(uv_last_error(u2L))));

        uv_close((uv_handle_t*)&ron_u->wax_u, 0);
        free(raq_u);
        free(ron_u);
        c3_assert(0);
      }
      else {
        ron_u->nam_u = nam_u;
        nam_u->ron_u = ron_u;
      }

      con_f(nam_u->ron_u);
    }
    else {
      //uL(fprintf(uH, "raft: existing connection %p for %s\n",
      //               nam_u->ron_u, nam_u->str_c));
      con_f(nam_u->ron_u);
      if ( u2_yes == nam_u->ron_u->liv ) {
        _raft_conn_work(nam_u->ron_u);
      }
    }
    nam_u = nam_u->nex_u;
  }
}

/* _raft_write_base(): Populate the base fields of a u2_rmsg.
**
** Should not be called directly.
*/
static void
_raft_write_base(u2_rcon* ron_u, u2_rmsg* msg_u)
{
  u2_raft* raf_u = ron_u->raf_u;

  msg_u->ver_w = u2_cr_mug('a');
  msg_u->tem_w = raf_u->tem_w;
  msg_u->len_d = 5;
}

/* _raft_write_rest(): Write fields for an RPC request to msg_u.
**
** Should not be called directly.
*/
static void
_raft_write_rest(u2_rcon* ron_u, c3_d lai_d, c3_w lat_w, u2_rmsg* msg_u)
{
  u2_raft* raf_u = ron_u->raf_u;

  c3_assert(ron_u->nam_u);
  _raft_write_base(ron_u, msg_u);
  msg_u->rest.lai_d = lai_d;
  msg_u->rest.lat_w = lat_w;
  msg_u->rest.nam_w = 1 + strlen(raf_u->str_c) / 4;
  msg_u->rest.nam_c = calloc(1, 4 * msg_u->rest.nam_w);
  uv_strlcpy(msg_u->rest.nam_c, raf_u->str_c, 4 * msg_u->rest.nam_w);
  msg_u->len_d += 4 + msg_u->rest.nam_w;
}

/* _raft_write_apen(): Write fields for an AppendEntries request.
*/
static void
_raft_write_apen(u2_rcon* ron_u,
                 c3_d lai_d, c3_w lat_w,
                 c3_d cit_d, c3_d ent_d, u2_rent* ent_u,
                 u2_rmsg* msg_u)
{
  _raft_write_rest(ron_u, lai_d, lat_w, msg_u);
  msg_u->typ_w = c3__apen;
  msg_u->rest.apen.cit_d = cit_d;
  msg_u->rest.apen.ent_d = ent_d;
  msg_u->len_d += 4;

  msg_u->rest.apen.ent_u = ent_u;
  {
    c3_d i_d;

    for ( i_d = 0; i_d < ent_d; i_d++ ) {
      msg_u->len_d += 3 + ent_u[i_d].len_w;
    }
  }
}

/* _raft_write_revo(): Write fields for a RequestVote request.
*/
static void
_raft_write_revo(u2_rcon* ron_u, u2_rmsg* msg_u)
{
  u2_raft* raf_u = ron_u->raf_u;

  _raft_write_rest(ron_u, raf_u->ent_d, raf_u->lat_w, msg_u);
  msg_u->typ_w = c3__revo;
}

/* _raft_send_rasp(): Send a rasp (raft response) to a peer.
*/
static void
_raft_send_rasp(u2_rcon* ron_u, c3_t suc_t)
{
  u2_rmsg msg_u;

  _raft_write_base(ron_u, &msg_u);
  msg_u.typ_w = c3__rasp;
  msg_u.rasp.suc_w = suc_t;
  msg_u.len_d += 1;
  _raft_rmsg_send(ron_u, &msg_u);
}

/* _raft_send_beat(): send a heartbeat (empty AppendEntries) to a peer.
**
** Creates a new request.
*/
static void
_raft_send_beat(u2_rcon* ron_u)
{
  u2_rreq*    req_u = _raft_rreq_new(ron_u);
  u2_rmsg*    msg_u = req_u->msg_u;

  c3_log_every(50, "raft: beat 50\n");

  _raft_write_apen(ron_u, 0, 0, 0, 0, 0, msg_u);
  _raft_rmsg_send(ron_u, msg_u);
}

/* _raft_send_revo(): send a RequestVote to a peer.
**
** Creates a new request.
*/
static void
_raft_send_revo(u2_rcon* ron_u)
{
  u2_rreq* req_u = _raft_rreq_new(ron_u);
  u2_rmsg* msg_u = req_u->msg_u;

  _raft_write_revo(ron_u, msg_u);
  _raft_rmsg_send(ron_u, msg_u);
}

/* _raft_start_election(): bump term, vote for self, solicit votes from peers.
*/
static void
_raft_start_election(u2_raft* raf_u)
{
  c3_i sas_i;

  c3_assert(0 == uv_is_active((uv_handle_t*)&raf_u->tim_u));
  sas_i = uv_timer_start(&raf_u->tim_u, _raft_time_cb,
                         _raft_election_rand(), 0);
  c3_assert(sas_i == 0);

  raf_u->tem_w++;
  u2_sist_put("term", (c3_y*)&raf_u->tem_w, sizeof(c3_w));
  uL(fprintf(uH, "raft: starting election [tem:%d]\n", raf_u->tem_w));

  {
    u2_rnam* nam_u;

    for ( nam_u = raf_u->nam_u; nam_u; nam_u = nam_u->nex_u ) {
      nam_u->vog = u2_no;
    }
  }
  raf_u->vot_w = 1;
  raf_u->vog_c = raf_u->str_c;
  u2_sist_put("vote", (c3_y*)raf_u->vog_c, strlen(raf_u->vog_c));

  _raft_conn_all(raf_u, _raft_send_revo);
}

/* _raft_heartbeat(): send a heartbeat to all peers.
*/
static void
_raft_heartbeat(u2_raft* raf_u)
{
  _raft_conn_all(raf_u, _raft_send_beat);
}

/* _raft_time_cb(): generic timer callback.
**
** Called on election timeouts for non-leaders, and at heartbeat interval for
** leaders.
*/
static void
_raft_time_cb(uv_timer_t* tim_u, c3_i sas_i)
{
  u2_raft* raf_u = tim_u->data;
  //uL(fprintf(uH, "raft: time\n"));

  c3_assert(sas_i == 0);
  switch ( raf_u->typ_e ) {
    default: {
      uL(fprintf(uH, "raft: time_cb: unknown server state\n"));
      c3_assert(0);
    }
    case u2_raty_foll: {
      uL(fprintf(uH, "raft: foll -> cand\n"));
      raf_u->typ_e = u2_raty_cand;
      // continue to cand
    }
    case u2_raty_cand: {
      _raft_start_election(raf_u);
      break;
    }
    case u2_raty_lead: {
      _raft_heartbeat(raf_u);
      break;
    }
  }
}

/* _raft_foll_init(): begin, follower mode.
*/
static void
_raft_foll_init(u2_raft* raf_u)
{
  uL(fprintf(uH, "raft: none -> foll\n"));
  raf_u->typ_e = u2_raty_foll;

  //  Initialize and count peers.
  {
    u2_rnam* nam_u = u2_raft_readopt(u2_Host.ops_u.raf_c,
                                     u2_Host.ops_u.nam_c,
                                     u2_Host.ops_u.rop_s);

    if ( 0 == nam_u ) {
      uL(fprintf(uH, "raft: couldn't parse arg '%s'\n", u2_Host.ops_u.raf_c));
      u2_lo_bail(u2A);
    }

    raf_u->pop_w = 1; raf_u->nam_u = nam_u;
    while ( nam_u ) {
      raf_u->pop_w++; nam_u = nam_u->nex_u;
    }
  }

  //  Set our name.
  {
    c3_i wri_i, siz_i;

    siz_i = strlen(u2_Host.ops_u.nam_c) + strlen(":65536") + 1;
    raf_u->str_c = c3_malloc(siz_i);
    wri_i = snprintf(raf_u->str_c, siz_i, "%s:%d",
                     u2_Host.ops_u.nam_c, u2_Host.ops_u.rop_s);
    c3_assert(wri_i < siz_i);
  }

  //  Load persisted settings.
  {
    c3_w  tem_w = 0;
    c3_c* vog_c = 0;
    c3_i  ret_i;

    if ( (ret_i = u2_sist_has("term")) >= 0 ) {
      c3_assert(sizeof(c3_w) == ret_i);
      u2_sist_get("term", (c3_y*)&tem_w);
      uL(fprintf(uH, "raft: term from sist: %u\n", tem_w));
    }
    if ( (ret_i = u2_sist_has("vote")) >= 0 ) {
      c3_assert(ret_i > 0);
      vog_c = c3_malloc(ret_i);
      u2_sist_get("vote", (c3_y*)vog_c);
      uL(fprintf(uH, "raft: vote from sist: %s\n", vog_c));
    }

    raf_u->tem_w = tem_w;
    if ( vog_c ) {
      if ( 0 == strcmp(vog_c, raf_u->str_c) ) {
        raf_u->vog_c = raf_u->str_c;
        raf_u->vot_w = 1;
        raf_u->typ_e = u2_raty_cand;
      }
      else {
        u2_rnam* nam_u;

        for ( nam_u = raf_u->nam_u; nam_u; nam_u = nam_u->nex_u ) {
          if ( 0 == strcmp(vog_c, nam_u->str_c) ) {
            raf_u->vog_c = nam_u->str_c;
            break;
          }
        }
        if ( 0 == nam_u ) {
          uL(fprintf(uH, "raft: discarding unknown vote %s\n", vog_c));
        }
      }
      free(vog_c);
    }
  }

  //  Bind the listener.
  {
    struct sockaddr_in add_u = uv_ip4_addr("0.0.0.0", u2_Host.ops_u.rop_s);

    if ( 0 != uv_tcp_init(u2L, &raf_u->wax_u) ) {
      uL(fprintf(uH, "raft: init: %s\n", uv_strerror(uv_last_error(u2L))));
      c3_assert(0);
    }
    if ( 0 != uv_tcp_bind(&raf_u->wax_u, add_u) ) {
      uL(fprintf(uH, "raft: bind: %s\n", uv_strerror(uv_last_error(u2L))));
      c3_assert(0);
    }
    if ( 0 != uv_listen((uv_stream_t*)&raf_u->wax_u, 16, _raft_listen_cb) ) {
      uL(fprintf(uH, "raft: listen: %s\n", uv_strerror(uv_last_error(u2L))));
      c3_assert(0);
    }
    else {
      uL(fprintf(uH, "raft: on TCP %d\n", u2_Host.ops_u.rop_s));
    }
  }

  //  Start the initial election timeout.
  uv_timer_start(&raf_u->tim_u, _raft_time_cb, _raft_election_rand(), 0);
}

/* _raft_lone_init(): begin, single-instance mode.
*/
static void
_raft_lone_init(u2_raft* raf_u)
{
  uL(fprintf(uH, "raft: single-instance mode\n"));
  raf_u->pop_w = 1;
  _raft_promote(raf_u);
}

/* u2_raft_init(): start Raft process.
*/
void
u2_raft_init()
{
  u2_raft* raf_u = u2R;

  //  Initialize timer -- used in both single and multi-instance mode,
  //  for different things.
  uv_timer_init(u2L, &raf_u->tim_u);
  raf_u->tim_u.data = raf_u;

  if ( 0 == u2_Host.ops_u.raf_c ) {
    _raft_lone_init(raf_u);
  }
  else {
    _raft_foll_init(raf_u);
  }
}

/* _raft_sure(): apply and save an input ovum and its result.
*/
static void
_raft_sure(u2_reck* rec_u, u2_noun ovo, u2_noun vir, u2_noun cor)
{
  //  Whatever worked, save it.  (XX - should be concurrent with execute.)
  //  We'd like more events that don't change the state but need work here.
  {
    u2_mug(cor);
    u2_mug(rec_u->roc);

    // we've got a new candidate world state.  Perhaps it's identical
    // to where we are now. Perhaps its new.
    //
    // * if new: update world state
    // * in either case: push side effects event into queue
    //
    if ( u2_no == u2_sing(cor, rec_u->roc) ) {
      rec_u->roe = u2nc(u2nc(vir, ovo), rec_u->roe);

      u2z(rec_u->roc);

      // update universe
      rec_u->roc = cor;
    }
    else {
      u2z(ovo);

      // push a new event into queue
      rec_u->roe = u2nc(u2nc(vir, u2_nul), rec_u->roe);

      u2z(cor);
    }
  }
}

/* _raft_lame(): handle an application failure.
*/
static void
_raft_lame(u2_reck* rec_u, u2_noun ovo, u2_noun why, u2_noun tan)
{
  u2_noun bov, gon;

#if 0
  {
    c3_c* oik_c = u2_cr_string(u2h(u2t(ovo)));

    // uL(fprintf(uH, "lame: %s\n", oik_c));
    free(oik_c);
  }
#endif

  //  Formal error in a network packet generates a hole card.
  //
  //  There should be a separate path for crypto failures,
  //  to prevent timing attacks, but isn't right now.  To deal
  //  with a crypto failure, just drop the packet.
  //
  if ( (c3__exit == why) && (c3__hear == u2h(u2t(ovo))) ) {
    u2_lo_punt(2, u2_ckb_flop(u2k(tan)));

    bov = u2nc(u2k(u2h(ovo)), u2nc(c3__hole, u2k(u2t(u2t(ovo)))));
    u2z(why);
  }
  else {
    bov = u2nc(u2k(u2h(ovo)), u2nt(c3__crud, why, u2k(tan)));
    u2_hevn_at(lad) = u2_nul;
  }
  // u2_lo_show("data", u2k(u2t(u2t(ovo))));

  u2z(ovo);

  gon = u2_lo_soft(rec_u, 0, u2_reck_poke, u2k(bov));
  if ( u2_blip == u2h(gon) ) {
    _raft_sure_guard(rec_u, bov, u2k(u2h(u2t(gon))), u2k(u2t(u2t(gon))), c3_false);

    u2z(gon);
  }
  else {
    u2z(gon);
    {
      u2_noun vab = u2nc(u2k(u2h(bov)),
                         u2nc(c3__warn, u2_ci_tape("crude crash!")));
      u2_noun nog = u2_lo_soft(rec_u, 0, u2_reck_poke, u2k(vab));

      if ( u2_blip == u2h(nog) ) {
        _raft_sure_guard(rec_u, vab, u2k(u2h(u2t(nog))), u2k(u2t(u2t(nog))), c3_false);
        u2z(nog);
      }
      else {
        u2z(nog);
        u2z(vab);

        uL(fprintf(uH, "crude: all delivery failed!\n"));
        u2_lo_punt(2, u2_ckb_flop(u2k(tan)));
        c3_assert(!"crud");
      }
    }
  }
}

/* _raft_punk(): insert and apply an input ovum (unprotected).
*/
static void
_raft_punk(u2_reck* rec_u, u2_noun ovo)
{
#ifdef GHETTO
  c3_c* txt_c = u2_cr_string(u2h(u2t(ovo)));
#endif
  c3_w sec_w;
  //  static c3_w num_w;
  u2_noun gon;

  //  uL(fprintf(uH, "punk: %s: %d\n", u2_cr_string(u2h(u2t(ovo))), num_w++));

  //  XX this is wrong - the timer should be on the original hose.
  //
  if ( (c3__term == u2h(u2t(u2h(ovo)))) ||
       (c3__batz == u2h(u2t(u2h(ovo)))) ) {
    sec_w = 0;
  } else sec_w = 60;

  //  Control alarm loops.
  //
  if ( c3__wake != u2h(u2t(ovo)) ) {
    u2_Host.beh_u.run_w = 0;
  }

#ifdef GHETTO
  struct timeval b4, f2, d0;
  gettimeofday(&b4, 0);
  uL(fprintf(uH, "%%soft %s\n", txt_c));
#endif

  // TJIC: this is what calls into arvo proper
  // does NOT modify global arvo state.  gon is a list of ova which are
  // EFFECTS ** AND ** the new kernel state

  gon = u2_lo_soft(rec_u, sec_w, u2_reck_poke, u2k(ovo));  
#ifdef GHETTO
  c3_w ms_w;

  gettimeofday(&f2, 0);
  timersub(&f2, &b4, &d0);
  ms_w = (d0.tv_sec * 1000) + (d0.tv_usec / 1000);
  uL(fprintf(uH, "%%punk %s %d.%dms\n", txt_c, ms_w, (d0.tv_usec % 1000) / 10));
  free(txt_c);
#endif

  // error case
  if ( u2_blip != u2h(gon) ) {
    u2_noun why = u2k(u2h(gon));
    u2_noun tan = u2k(u2t(gon));

    u2z(gon);
    _raft_lame(rec_u, ovo, why, tan);
  }
  // TJIC success; operate on gon
  else {
    u2_noun vir = u2k(u2h(u2t(gon)));  // TJIC  vir = side effects  // c3_hear packets  "I want to hear that again"
    u2_noun cor = u2k(u2t(u2t(gon)));  // TJIC  cor = new rock (new value of system)
    u2_noun nug;

    u2z(gon);
    nug = u2_reck_nick(rec_u, vir, cor);  // <---- TJIC event transformations can happen here

    if ( u2_blip != u2h(nug) ) {
      u2_noun why = u2k(u2h(nug));
      u2_noun tan = u2k(u2t(nug));

      u2z(nug);
      _raft_lame(rec_u, ovo, why, tan);  // <---- TJIC event transformations can happen here
    }
    else {
      vir = u2k(u2h(u2t(nug)));
      cor = u2k(u2t(u2t(nug)));

      u2z(nug);
      _raft_sure_guard(rec_u, ovo, vir, cor, c3_false);
    }
  }
  //  uL(fprintf(uH, "punk oot %s\n", txt_c));
  //  free(txt_c);
}

static void
_raft_comm(u2_reck* rec_u, c3_d bid_d)
{
  u2_cart* egg_u;

  u2_lo_open();

  egg_u = rec_u->ova.egg_u;
  while ( egg_u ) {
    if ( egg_u->ent_d <= bid_d ) {
      egg_u->log = u2_yes;
    } else break;
    egg_u = egg_u->nex_u;
  }
  u2_lo_shut(u2_yes);
}

// When we send something to raft w _raft_push(), we ask libuv to call us back later.
// This is later.
static void
_raft_comm_cb(uv_timer_t* tim_u, c3_i sas_i)
{
  u2_raft* raf_u = tim_u->data;

  _raft_comm(u2A, raf_u->ent_d);
}




/* _raft_kick_all(): kick a list of events, transferring.
*/
static void
_raft_kick_all(u2_reck* rec_u, u2_noun vir)
{
  while ( u2_nul != vir ) {
    u2_noun ovo = u2k(u2h(vir));
    u2_noun nex = u2k(u2t(vir));
    u2z(vir); vir = nex;

    u2_reck_kick(rec_u, ovo);
  }
}



/* u2_raft_work(): work in rec_u.
*/
void
u2_raft_work(u2_reck* rec_u)
{
  if ( u2R->typ_e != u2_raty_lead ) {
    c3_assert(rec_u->ova.egg_u == 0);
    if ( u2_nul != rec_u->roe ) {
      uL(fprintf(uH, "raft: dropping roe!!\n"));
      u2z(rec_u->roe);
      rec_u->roe = u2_nul;
    }
  }
  else {
    u2_cart* egg_u;
    u2_noun  ova;
    u2_noun  vir;
    u2_noun  nex;

    //  Delete finished events.
    //
    while ( rec_u->ova.egg_u ) {

      egg_u = rec_u->ova.egg_u;

      if ( u2_yes == egg_u->done ) {
        vir = egg_u->vir;

        if ( egg_u == rec_u->ova.geg_u ) {
          c3_assert(egg_u->nex_u == 0);
          rec_u->ova.geg_u = rec_u->ova.egg_u = 0;
        }
        else {
          c3_assert(egg_u->nex_u != 0);
          rec_u->ova.egg_u = egg_u->nex_u;
        }

        egg_u->log = u2_yes;
        free(egg_u);
      }
      else break;
    }

    //  Poke pending events, leaving the poked events and errors on rec_u->roe.
    //
    {
      if ( 0 == u2R->lug_u.len_d ) {
        return;
      }
      ova = u2_ckb_flop(rec_u->roe);     /// call multiple things off queue and punk them to arvo
      rec_u->roe = u2_nul;

      while ( u2_nul != ova ) {
        _raft_punk(rec_u, u2k(u2t(u2h(ova))));   // arvo has got it ; side effects are queued up SOMEWHERE
        c3_assert(u2_nul == u2h(u2h(ova)));

        nex = u2k(u2t(ova));
        u2z(ova); ova = nex;
      }
    }

    //  Cartify, jam, encrypt, log this batch of events. 
    //
    {
      c3_d    bid_d;

      u2_noun ovo;

      ova = u2_ckb_flop(rec_u->roe);
      rec_u->roe = u2_nul;

      while ( u2_nul != ova ) {
        ovo = u2k(u2t(u2h(ova)));
        vir = u2k(u2h(u2h(ova)));
        nex = u2k(u2t(ova));
        u2z(ova); ova = nex;

        if ( u2_nul != ovo ) {

          // we're making a new egg here, that's LIKE ovo
          //
          egg_u = c3_malloc(sizeof(*egg_u));
          egg_u->nex_u = 0;
          egg_u->log = u2_no;
          egg_u->done = u2_no;
          egg_u->vir = vir;

          if(u2_Host.ops_u.kaf_c){
            bid_d = u2_kafk_push_ova(rec_u, ovo, LOG_MSG_PRECOMMIT);  
          } else {
            bid_d = u2_egz_push_ova(rec_u, ovo, LOG_MSG_PRECOMMIT);
          }

          // ...and  here we're putting the log ID # in it
          // but why?
          //
          egg_u->ent_d = bid_d;

          // then we enqueu this NEW egg
          // again...why?
          if ( 0 == rec_u->ova.geg_u ) {
            c3_assert(0 == rec_u->ova.egg_u);
            rec_u->ova.geg_u = rec_u->ova.egg_u = egg_u;
          }
          else {
            c3_assert(0 == rec_u->ova.geg_u->nex_u);
            rec_u->ova.geg_u->nex_u = egg_u;
            rec_u->ova.geg_u = egg_u;
          }
          _raft_kick_all(rec_u, vir);
          egg_u->done = u2_yes;
        }
      }
    }
  }
}

//  This is the final step of some tricky coordination.
//
//  We must (a) log each ova, (b) process each ova.  We may do those things in parallel, but 
//  both must be done before we can update the global state and emit side effects.
//
//  _raft_sure is that final step.  
//
//  This function guards _raft_sure.  Either you are ready to call raft_sure right now, 
//
//  force_delay
static void 
_raft_sure_guard(u2_reck* rec_u, u2_noun ovo, u2_noun vir, u2_noun cor, c3_t force_delay)
{
  //  if ( ( rec_u->ova.egg_u->log) && (rec_u->ova.egg_u->done) && force_delay  ) {
    _raft_sure(rec_u, ovo, vir, cor);
    //  } else {
    // NOTFORCHECKIN - here we have to enqueue a task for later
    //  }
}



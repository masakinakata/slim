/*
 * mc_worker.h
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group
 *                                          <lmntal@ueda.info.waseda.ac.jp>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *    3. Neither the name of the Ueda Laboratory LMNtal Group nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef LMN_MC_WORKER_H
#define LMN_MC_WORKER_H

/**
 * @ingroup  Verifier
 * @defgroup Worker
 * @{
 */

#include "../lmntal.h"
#include "element/element.h"
#include "automata.h"

struct StateSpace;

#include <memory>

#if defined(HAVE_ATOMIC_SUB) && defined(HAVE_BUILTIN_MBARRIER)
#//define OPT_WORKERS_SYNC /* ????????????buggy?????????comment out */
#endif

typedef struct LmnWorkerGroup LmnWorkerGroup;
typedef struct LmnMCObj LmnMCObj;
typedef struct LmnWorker LmnWorker;

/**
 *  Worker Group
 */

class LmnWorkerGroup {
  unsigned int worker_num; /* ??????Worker??? */
  /* 4bytes alignment (64-bit processor) */
  LmnWorker **workers; /* ??????Worker */

  BOOL terminated;    /* ???????????????????????? */
  volatile BOOL stop; /* ???????????????????????? */
  BOOL do_search;     /* ???????????????????????????????????? */
  BOOL
      do_exhaustive; /* ?????????1?????????????????????????????????!!????????????????????????!!?????? */
  BOOL do_para_algo; /* ??????????????????????????????????????????????????? */
  volatile BOOL mc_exit; /* ????????????????????????????????????????????????????????? */
  BOOL error_exist; /* ????????????????????????????????? */

  State *opt_end_state; /* the state has optimized cost */
  EWLock *ewlock;       /* elock: ????????????????????????
                         * wlock: ?????????????????????????????????????????? */

  FILE *out; /* ????????? */
 
  LmnWorkerGroup(const LmnWorkerGroup &lwg);
  LmnWorker *workers_get_entry(unsigned int i);
  void workers_set_entry(unsigned int i, LmnWorker *w);
  void workers_free(unsigned int w_num);
  void workers_gen(unsigned int w_num, AutomataRef a, Vector *psyms, BOOL flags);//private?
  BOOL flags_init(AutomataRef property_a);
  void ring_alignment();

public:
#ifdef OPT_WORKERS_SYNC
  volatile unsigned int synchronizer;
  volatile unsigned int workers_synchronizer();
  void workers_set_synchronizer(unsigned int i);
#else
  lmn_barrier_t synchronizer; /* ???????????????????????????????????? */
  lmn_barrier_t *workers_synchronizer(); //koredato private ni dekinai by sumiya
  void workers_set_synchronizer(lmn_barrier_t);
#endif

  LmnWorkerGroup();
  LmnWorkerGroup(AutomataRef a, Vector *psyms, int thread_num);
  ~LmnWorkerGroup();
  volatile BOOL workers_are_exit();
  void workers_set_exit();
  void workers_unset_exit();
  BOOL workers_have_error();
  void workers_found_error();
  void workers_unfound_error();
  BOOL workers_get_do_palgorithm();
  void workers_set_do_palgorithm();
  void workers_unset_do_palgorithm();
  FILE workers_out();

  State *workers_get_opt_end_state();
  void workers_set_opt_end_state(State *s);
  EWLock *workers_get_ewlock();
  void workers_set_ewlock(EWLock *e);
  void workers_opt_end_lock();
  void workers_opt_end_unlock();
  void workers_state_lock(mtx_data_t id);
  void workers_state_unlock(mtx_data_t id);

  BOOL workers_are_terminated();
  void workers_set_terminated();
  void workers_unset_terminated();

  volatile BOOL workers_are_stop();
  void workers_set_stop();
  void workers_unset_stop();

  BOOL workers_are_do_search();
  void workers_set_do_search();
  void workers_unset_do_search();

  BOOL workers_are_do_exhaustive();
  void workers_set_do_exhaustive();
  void workers_unset_do_exhaustive();

  unsigned int workers_get_entried_num();
  void workers_set_entried_num(unsigned int i);

  LmnWorker *get_worker(unsigned long id);
  LmnWorker *workers_get_my_worker(); 
  void launch_lmn_workers();
  LmnCost opt_cost();
  void update_opt_cost(State *new_s, BOOL f);
  BOOL termination_detection(int id);
  void lmn_workers_synchronization(unsigned long id, void (*func)(LmnWorker *w));

};
/**
 *  Objects for Model Checking
 */

struct LmnMCObj {
  LmnWorker *owner;
  BYTE type;
  void *obj;                     /* ?????????????????? */
  void (*init)(LmnWorker *);     /* obj?????????????????? */
  void (*finalize)(LmnWorker *); /* obj?????????????????? */
};

#define mc_obj(MC) ((MC)->obj)
#define mc_obj_set(MC, O) ((MC)->obj = (O))
#define mc_type(MC) ((MC)->type)
#define mc_type_set(MC, T) ((MC)->type = (T))
#define mc_init_f(MC)                                                          \
  if ((MC)->init) {                                                            \
    (*(MC)->init)((MC)->owner);                                                \
  }
#define mc_init_f_set(MC, FP) ((MC)->init = (FP))
#define mc_finalize_f_set(MC, FP) ((MC)->finalize = (FP))
#define mc_finalize_f(MC)                                                      \
  if ((MC)->finalize) {                                                        \
    (*(MC)->finalize)((MC)->owner);                                            \
  }

/**
 *  Worker
 */

struct LmnWorker {
  lmn_thread_t pth;         /* ?????????????????????(pthread_t) */
  volatile unsigned int id; /* Natural integer id (lmn_thread_id) */

  BOOL f_end; /* Worker?????????????????????????????????. ?????????Worker??????????????? */
  BOOL f_end2; /* Worker?????????????????????????????????. ?????????Worker??????????????? */
  BYTE f_safe; /* Worker??????????????????????????????????????????Writable???????????? */
  BYTE f_exec; /* ???????????????????????????????????????????????? */

  BOOL wait;
  /* ?????????3BYTE */

  LmnMCObj generator;
  LmnMCObj explorer;
  BOOL is_explorer;

  void (*start)(struct LmnWorker *); /* ???????????? */
  BOOL (*check)(struct LmnWorker *); /* ?????????????????? */

  StateSpaceRef states; /* Pointer to StateSpace */
  std::unique_ptr<MCReactContext> cxt;   /* ReactContext Object */
  LmnWorker *next;      /* Pointer to Neighbor Worker */
  LmnWorkerGroup *group;

  Vector *invalid_seeds;
  Vector *cycles;

  int expand; // for debug
  int red;
};

/**
 * Macros for data access
 */
#define worker_id(W) ((W)->id)
#define worker_pid(W) ((W)->pth)
#define worker_flags(W) ((W)->f_exec)
#define worker_flags_set(W, F) ((W)->f_exec |= (F))
#define worker_states(W) ((W)->states)
#define worker_next(W) ((W)->next)
#define worker_rc(W) ((W)->cxt)
#define worker_generator(W) ((W)->generator)
#define worker_generator_obj_set(W, O) mc_obj_set(&worker_generator(W), (O))
#define worker_generator_obj(W) mc_obj(&worker_generator(W))
#define worker_generator_type(W) mc_type(&worker_generator(W))
#define worker_generator_type_set(W, T) mc_type_set(&worker_generator(W), (T))
#define worker_explorer(W) ((W)->explorer)
#define worker_explorer_obj_set(W, O) mc_obj_set(&worker_explorer(W), (O))
#define worker_explorer_obj(W) mc_obj(&worker_explorer(W))
#define worker_explorer_type(W) mc_type(&worker_explorer(W))
#define worker_explorer_type_set(W, T) mc_type_set(&worker_explorer(W), (T))
#define worker_group(W) ((W)->group)
#define worker_STOP(W) ((W)->wait = TRUE)
#define worker_RESTART(W) ((W)->wait = FALSE)
#define worker_is_WAIT(W) ((W)->wait)
#define worker_invalid_seeds(W) ((W)->invalid_seeds)
#define worker_cycles(W) ((W)->cycles)
#define worker_is_explorer(W) ((W)->is_explorer)
#define worker_explorer_set(W) ((W)->is_explorer = TRUE)
#define worker_is_generator(W) (!worker_is_explorer(W))
#define worker_generator_set(W) ((W)->is_explorer = FALSE)

#define worker_init(W)                                                         \
  do {                                                                         \
    mc_init_f(&worker_generator(W));                                           \
    mc_init_f(&worker_explorer(W));                                            \
  } while (0)

#define worker_finalize(W)                                                     \
  do {                                                                         \
    mc_finalize_f(&worker_generator(W));                                       \
    mc_finalize_f(&worker_explorer(W));                                        \
  } while (0)

#define worker_start(W)                                                        \
  if ((W)->start) {                                                            \
    (*(W)->start)(W);                                                          \
  }

static inline BOOL worker_check(LmnWorker *w) {
  if (w->check) {
    return (*(w)->check)(w);
  } else {
    return TRUE;
  }
}

#define worker_generator_init_f_set(W, F) mc_init_f_set(&worker_generator(W), F)
#define worker_generator_finalize_f_set(W, F)                                  \
  mc_finalize_f_set(&worker_generator(W), F)
#define worker_explorer_init_f_set(W, F) mc_init_f_set(&worker_explorer(W), F)
#define worker_explorer_finalize_f_set(W, F)                                   \
  mc_finalize_f_set(&worker_explorer(W), F)

/** Macros for f_safe
 */
#define WORKER_ACTIVE_MASK (0x01U)
#define WORKER_WAITING_MASK (0x01U << 1)

#define worker_unset_active(W) ((W)->f_safe &= (~WORKER_ACTIVE_MASK))
#define worker_set_idle(W) (worker_unset_active(W))
#define worker_set_active(W) ((W)->f_safe |= WORKER_ACTIVE_MASK)
#define worker_is_active(W) ((W)->f_safe & WORKER_ACTIVE_MASK)
#define worker_is_idle(W) (!(worker_is_active(W)))

/** Macros for f_end
 */
#define worker_set_black(W) ((W)->f_end = FALSE)
#define worker_set_white(W) ((W)->f_end = TRUE)
#define worker_is_white(W) ((W)->f_end)

/** Macros for f_end2
 */
#define worker_set_stealer(W) ((W)->f_end2 = TRUE)
#define worker_unset_stealer(W) ((W)->f_end2 = FALSE)
#define worker_is_stealer(W) ((W)->f_end2)

/** Macros for f_exec
 *
 * ---- ---1  incremental state dumper
 * ---- --1-  partial order statespace generator
 * ---- -1--  asynchronous with property automata
 * ---- 1---  use Transition Object
 * ---1 ----  use membrane2binaryString compresser
 * --1- ----  use delta-membrane generator
 * -1-- ----  use membrane2canonical ID generator
 * 1--- ----  EMPTY
 *
 * ???????????????, ???????????????????????????8bit??????uint16_t??????????????????,
 * ??????1????????????????????????.
 */
#define WORKER_F0_MC_DUMP_MASK (0x01U)
#define WORKER_F0_MC_POR_MASK (0x01U << 1)
#define WORKER_F0_MC_PROP_MASK (0x01U << 2)
#define WORKER_F0_MC_TRANS_MASK (0x01U << 3)
#define WORKER_F0_MC_COMPRESS_MASK (0x01U << 4)
#define WORKER_F0_MC_DELTA_MASK (0x01U << 5)
#define WORKER_F0_MC_CANONICAL_MASK (0x01U << 6)

#define mc_is_dump(F) ((F)&WORKER_F0_MC_DUMP_MASK)
#define mc_set_dump(F) ((F) |= WORKER_F0_MC_DUMP_MASK)
#define mc_unset_dump(F) ((F) &= (~WORKER_F0_MC_DUMP_MASK))
#define mc_enable_por(F) ((F)&WORKER_F0_MC_POR_MASK)
#define mc_set_por(F) ((F) |= WORKER_F0_MC_POR_MASK)
#define mc_unset_por(F) ((F) &= (~WORKER_F0_MC_POR_MASK))
#define mc_has_property(F) ((F)&WORKER_F0_MC_PROP_MASK)
#define mc_set_property(F) ((F) |= WORKER_F0_MC_PROP_MASK)
#define mc_has_trans(F) ((F)&WORKER_F0_MC_TRANS_MASK)
#define mc_set_trans(F) ((F) |= WORKER_F0_MC_TRANS_MASK)
#define mc_use_delta(F) ((F)&WORKER_F0_MC_DELTA_MASK)
#define mc_set_delta(F) ((F) |= WORKER_F0_MC_DELTA_MASK)
#define mc_use_compress(F) ((F)&WORKER_F0_MC_COMPRESS_MASK)
#define mc_set_compress(F) ((F) |= WORKER_F0_MC_COMPRESS_MASK)
#define mc_use_canonical(F) ((F)&WORKER_F0_MC_CANONICAL_MASK)
#define mc_set_canonical(F) ((F) |= WORKER_F0_MC_CANONICAL_MASK)
#define mc_unset_canonical(F) ((F) &= (~WORKER_F0_MC_CANONICAL_MASK))

/** Macros for MCObj of generator
 */

#define WORKER_F1_PARALLEL_MASK (0x01U)
#define WORKER_F1_DYNAMIC_LB_MASK (0x01U << 1)
#define WORKER_F1_MC_DFS_MASK (0x01U << 2)
#define WORKER_F1_MC_BFS_MASK (0x01U << 3)
#define WORKER_F1_MC_BFS_LSYNC_MASK (0x01U << 4)
#define WORKER_F1_MC_OPT_SCC_MASK (0x01U << 5)

#define mc_on_parallel(F) ((F)&WORKER_F1_PARALLEL_MASK)
#define mc_set_parallel(F) ((F) |= WORKER_F1_PARALLEL_MASK)
#define mc_on_dynamic_lb(F) ((F)&WORKER_F1_DYNAMIC_LB_MASK)
#define mc_set_dynamic_lb(F) ((F) |= WORKER_F1_DYNAMIC_LB_MASK)
#define mc_on_dfs(F) ((F)&WORKER_F1_MC_DFS_MASK)
#define mc_set_dfs(F) ((F) |= WORKER_F1_MC_DFS_MASK)
#define mc_on_bfs(F) ((F)&WORKER_F1_MC_BFS_MASK)
#define mc_set_bfs(F) ((F) |= WORKER_F1_MC_BFS_MASK)
#define mc_use_lsync(F) ((F)&WORKER_F1_MC_BFS_LSYNC_MASK)
#define mc_set_lsync(F) ((F) |= WORKER_F1_MC_BFS_LSYNC_MASK)
#define mc_use_opt_scc(F) ((F)&WORKER_F1_MC_OPT_SCC_MASK)
#define mc_set_opt_scc(F) ((F) |= WORKER_F1_MC_OPT_SCC_MASK)

#define worker_on_parallel(W) (mc_on_parallel(worker_generator_type(W)))
#define worker_set_parallel(W) (mc_set_parallel(worker_generator_type(W)))
#define worker_on_dynamic_lb(W) (mc_on_dynamic_lb(worker_generator_type(W)))
#define worker_set_dynamic_lb(W) (mc_set_dynamic_lb(worker_generator_type(W)))
#define worker_on_mc_dfs(W) (mc_on_dfs(worker_generator_type(W)))
#define worker_set_mc_dfs(W) (mc_set_dfs(worker_generator_type(W)))
#define worker_on_mc_bfs(W) (mc_on_bfs(worker_generator_type(W)))
#define worker_set_mc_bfs(W) (mc_set_bfs(worker_generator_type(W)))
#define worker_use_lsync(W) (mc_use_lsync(worker_generator_type(W)))
#define worker_set_lsync(W) (mc_set_lsync(worker_generator_type(W)))
#define worker_use_opt_scc(W) (mc_use_opt_scc(worker_generator_type(W)))
#define worker_set_opt_scc(W) (mc_set_opt_scc(worker_generator_type(W)))

#define WORKER_F2_MC_NDFS_MASK (0x01U)
#define WORKER_F2_MC_OWCTY_MASK (0x01U << 1)
#define WORKER_F2_MC_MAP_MASK (0x01U << 2)
#define WORKER_F2_MC_MAP_WEAK_MASK (0x01U << 3)
#define WORKER_F2_MC_BLE_MASK (0x01U << 4)
#define WORKER_F2_MC_MAPNDFS_MASK (0x01U << 5)
#define WORKER_F2_MC_MAPNDFS_WEAK_MASK (0x01U << 6)
#define WORKER_F2_MC_MCNDFS_MASK (0x01U << 7)

#define mc_ltl_none(F) (F == 0x00U)
#define mc_use_ndfs(F) ((F)&WORKER_F2_MC_NDFS_MASK)
#define mc_set_ndfs(F) ((F) |= WORKER_F2_MC_NDFS_MASK)
#define mc_use_owcty(F) ((F)&WORKER_F2_MC_OWCTY_MASK)
#define mc_set_owcty(F) ((F) |= WORKER_F2_MC_OWCTY_MASK)
#define mc_use_map(F) ((F)&WORKER_F2_MC_MAP_MASK)
#define mc_set_map(F) ((F) |= WORKER_F2_MC_MAP_MASK)
#define mc_use_weak_map(F) ((F)&WORKER_F2_MC_MAP_WEAK_MASK)
#define mc_set_weak_map(F) ((F) |= WORKER_F2_MC_MAP_WEAK_MASK)
#define mc_use_ble(F) ((F)&WORKER_F2_MC_BLE_MASK)
#define mc_set_ble(F) ((F) |= WORKER_F2_MC_BLE_MASK)
#define mc_use_mapndfs(F) ((F)&WORKER_F2_MC_MAPNDFS_MASK)
#define mc_set_mapndfs(F) ((F) |= WORKER_F2_MC_MAPNDFS_MASK)
#define mc_use_mapndfs_weak(F) ((F)&WORKER_F2_MC_MAPNDFS_WEAK_MASK)
#define mc_set_mapndfs_weak(F) ((F) |= WORKER_F2_MC_MAPNDFS_WEAK_MASK)
#define mc_use_mcndfs(F) ((F)&WORKER_F2_MC_MCNDFS_MASK)
#define mc_set_mcndfs(F) ((F) |= WORKER_F2_MC_MCNDFS_MASK)

#define worker_ltl_none(W) (mc_ltl_none(worker_explorer_type(W)))
#define worker_use_ndfs(W) (mc_use_ndfs(worker_explorer_type(W)))
#define worker_set_ndfs(W) (mc_set_ndfs(worker_explorer_type(W)))
#define worker_use_owcty(W) (mc_use_owcty(worker_explorer_type(W)))
#define worker_set_owcty(W) (mc_set_owcty(worker_explorer_type(W)))
#define worker_use_map(W) (mc_use_map(worker_explorer_type(W)))
#define worker_set_map(W) (mc_set_map(worker_explorer_type(W)))
#define worker_use_weak_map(W) (mc_use_weak_map(worker_explorer_type(W)))
#define worker_set_weak_map(W) (mc_set_weak_map(worker_explorer_type(W)))
#define worker_use_ble(W) (mc_use_ble(worker_explorer_type(W)))
#define worker_set_ble(W) (mc_set_ble(worker_explorer_type(W)))
#define worker_use_mapndfs(W) (mc_use_mapndfs(worker_explorer_type(W)))
#define worker_set_mapndfs(W) (mc_set_mapndfs(worker_explorer_type(W)))
#define worker_use_mapndfs_weak(W)                                             \
  (mc_use_mapndfs_weak(worker_explorer_type(W)))
#define worker_set_mapndfs_weak(W)                                             \
  (mc_set_mapndfs_weak(worker_explorer_type(W)))
#define worker_use_mcndfs(W) (mc_use_mcndfs(worker_explorer_type(W)))
#define worker_set_mcndfs(W) (mc_set_mcndfs(worker_explorer_type(W)))

/** Macros for MAPNDFS
 */
/*#define worker_is_generator(W)        (worker_id(W) > 0)
#define worker_is_explorer(W)         (woeker_id(W) == 0)*/

/** ProtoTypes
 */
BOOL lmn_workers_termination_detection_for_rings(LmnWorker *root);
void lmn_workers_synchronization(LmnWorker *root, void (*func)(LmnWorker *w));
LmnWorker *lmn_worker_make_minimal(void);
LmnWorker *lmn_worker_make(StateSpaceRef ss, unsigned long id, BOOL flags);
void lmn_worker_free(LmnWorker *w);

LmnWorker *worker_next_generator(LmnWorker *w);

/* @} */

#endif

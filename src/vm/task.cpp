/*
 * task.cpp
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group
 * <lmntal@ueda.info.waseda.ac.jp> All rights reserved.
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
 * $Id: task.c,v 1.37 2008/10/21 11:31:58 riki Exp $
 */

#include "task.h"
#include "ccallback.h"
#include "dumper.h"
#include "interpret/false_driven_enumerator.hpp"
#include "interpret/interpreter.hpp"
#include "normal_thread.h"
#include "special_atom.h"
#include "symbol.h"
#include "verifier/runtime_status.h"
#include "verifier/verifier.h"

#ifdef USE_FIRSTCLASS_RULE
#include "firstclass_rule.h"
#endif

#include <algorithm>
#include <iostream> 
typedef void (*callback_0)(LmnReactCxtRef, LmnMembraneRef);
typedef void (*callback_1)(LmnReactCxtRef, LmnMembraneRef, LmnAtomRef,
                           LmnLinkAttr);
typedef void (*callback_2)(LmnReactCxtRef, LmnMembraneRef, LmnAtomRef,
                           LmnLinkAttr, LmnAtomRef, LmnLinkAttr);
typedef void (*callback_3)(LmnReactCxtRef, LmnMembraneRef, LmnAtomRef,
                           LmnLinkAttr, LmnAtomRef, LmnLinkAttr, LmnAtomRef,
                           LmnLinkAttr);
typedef void (*callback_4)(LmnReactCxtRef, LmnMembraneRef, LmnAtomRef,
                           LmnLinkAttr, LmnAtomRef, LmnLinkAttr, LmnAtomRef,
                           LmnLinkAttr, LmnAtomRef, LmnLinkAttr);

typedef void (*callback_5)(LmnReactCxtRef, LmnMembraneRef, LmnAtomRef,
                           LmnLinkAttr, LmnAtomRef, LmnLinkAttr, LmnAtomRef,
                           LmnLinkAttr, LmnAtomRef, LmnLinkAttr, LmnAtomRef,
                           LmnLinkAttr);

struct Vector user_system_rulesets; /* system ruleset defined by user */

/**
  Java????????????????????????????????????????????????????????????????????????????????????SLIM??????
  ?????????????????????????????????????????????????????????????????????????????????????????????????????????
  ??????????????????SLIM????????????????????????a(L,L).  a(X,Y) :- b(X), b(Y). ?????????
  ???2.??????????????????????????????????????????????????????GETLINK???????????????????????????
  INHERIT_LINK ????????????????????????????????????????????????????????????????????????????????????
  ??????3.??????????????????GETLINK ???????????????????????????????????????????????????????????????
  ???????????????????????????????????????????????????

  ?????????3.????????????????????????????????????????????? wt???at????????????????????????????????????
  ??????????????????????????????????????????????????????????????????????????????????????????????????????
  ??????????????????????????????wt??????????????????1????????????1????????????????????????????????????
  ??????????????????????????????????????????????????????????????????????????????????????????????????????
  ????????????????????????????????????????????????????????????

  1. ??????????????????????????????????????????(Java??????????????????)

    0--+       0----b       0 +--b       +--b
    a  |  =>   a       =>   a |      =>  |
    1--+       1            1 +--b       +--b

  2. ??????????????????????????????????????????(SLIM)
     A,B???GETLINK??????????????????????????????????????????

                  (B???????????????)    (A???????????????)    (???????????????)
      A                A                  A               A
      |                |             b--> |          b--> |
  +---0<---+       +---0              <---0
  |   a    |  =>   |   a      =>          a      =>
  +-->1----+       +-->1--->              1--->
      |                |<-- b             |<-- b          |<-- b
      B                B                  B               B


  3. ??????????????????????????????????????????????????????????????????????????????(SLIM)
      A,B???GETLINK???????????????????????????????????????????????????????????????????????????
      ???????????????????????????????????????????????????

         (B????????????????????????)        (A????????????????????????)    a?????????
                          +---------+    +---------+    +---------+-+    +----+
                          |         |    |         |    |         | |    |    |
  B             B         v     B   |    v     B   |    v     B   | |    v    |
  |         b-->|         b     |   |    b     |   |    b     |   | |    b    |
+---0<--+    +---0<--+    | +---0   |    | +---0   |    | +---0   | |    |    |
|   a   | => |   a   | => | |   a   | => | |   a   | => | |   a   | | => |    |
+-->1---+    +-->1---+    +-+-->1---+    +-+-->1---+    | +-->1---+ |    |    |
      |            |              |              |<--b  +------|--->b    +--->b
      A            A              A              A             A
*/

static inline BOOL react_ruleset(LmnReactCxtRef rc, LmnMembraneRef mem,
                                 LmnRuleSetRef ruleset);
static inline void react_initial_rulesets(LmnReactCxtRef rc,
                                          LmnMembraneRef mem);
static inline BOOL react_ruleset_in_all_mem(LmnReactCxtRef rc, LmnRuleSetRef rs,
                                            LmnMembraneRef mem);
static BOOL dmem_interpret(LmnReactCxtRef rc, LmnRuleRef rule,
                           LmnRuleInstr instr);

static void mem_oriented_loop(MemReactContext *ctx, LmnMembraneRef mem);

void lmn_dmem_interpret(LmnReactCxtRef rc, LmnRuleRef rule,
                        LmnRuleInstr instr) {
  dmem_interpret(rc, rule, instr);
}

namespace c14 = slim::element;
namespace c17 = slim::element;

/** ????????????????????????.
 *  ????????????????????????????????????????????????????????????????????????[yueno]
 *    1. ?????????     normal_cleaning??????????????????????????????
 *    2. ?????????     !(normal_remain???????????? && normal_remaining=ON) ???
 *    3. ??????       ????????????
 *    4. ?????????     ??????????????????
 *    5. ??????????????? ?????????????????????ON???normal_remain???????????????OFF??????????????????
 */
void lmn_run(Vector *start_rulesets) {
  static LmnMembraneRef mem;
  static std::unique_ptr<MemReactContext> mrc = nullptr;

  if (!mrc)
    mrc = c14::make_unique<MemReactContext>(nullptr);

#ifdef USE_FIRSTCLASS_RULE
  first_class_rule_tbl_init();
#endif

  /* ????????????????????????????????????????????????Process ID???
   * 1?????????????????????????????????(?????????????????????)??????????????????.
   * ????????????Process???ID??????????????????????????????.
   * ???????????????????????????????????????????????????ID???????????????????????????MT-Safe??????????????????
   */
  if (!env_proc_id_pool()) {
    env_set_proc_id_pool(new Vector(64));
  }

  /* interactive: normal_cleaning????????????ON????????????????????? */
  if (lmn_env.normal_cleaning) {
    mem->drop();
    delete mem;
    mrc = nullptr;
    lmn_env.normal_cleaning = FALSE;
  }

  /* interactive : (normal_remain??????normal_remaining=ON)??????????????????????????? */
  if (!lmn_env.normal_remain && !lmn_env.normal_remaining) {
    mem = new LmnMembrane();
    mrc = c14::make_unique<MemReactContext>(mem);
  }
  mrc->memstack_push(mem);

  // normal parallel mode init
  if (lmn_env.enable_parallel && !lmn_env.nd) {
    normal_parallel_init();
  }

  /** PROFILE START */
  if (lmn_env.profile_level >= 1) {
    profile_start_exec();
    profile_start_exec_thread();
  }

  react_start_rulesets(mem, start_rulesets);
  mrc->memstack_reconstruct(mem);

  if (lmn_env.trace) {
    if (lmn_env.output_format != JSON) {
      mrc->increment_reaction_count();
      fprintf(stdout, "%d: ", mrc->get_reaction_count());
    }
    lmn_dump_cell_stdout(mem);
    if (lmn_env.show_hyperlink)
      lmn_hyperlink_print(mem);
  }

  mem_oriented_loop(mrc.get(), mem);

  /** PROFILE FINISH */
  if (lmn_env.profile_level >= 1) {
    profile_finish_exec_thread();
    profile_finish_exec();
  }

  if (lmn_env
          .dump) { /* lmntal??????io??????????????????????????????????????????????????????????????????????????????????????????,
                      ??????????? */
    if (lmn_env.sp_dump_format == LMN_SYNTAX) {
      fprintf(stdout, "finish.\n");
    } else {
      lmn_dump_cell_stdout(mem);
    }
  }
  if (lmn_env.show_hyperlink)
    lmn_hyperlink_print(mem);

  // normal parallel mode free
  if (lmn_env.enable_parallel && !lmn_env.nd) {
    if (lmn_env.profile_level == 3)
      normal_parallel_prof_dump(stderr);
    normal_parallel_free();
  }
  /* ????????? */
  if (lmn_env.normal_remain) {
    lmn_env.normal_remaining = TRUE;
  } else {
    lmn_env.normal_remaining = FALSE;
    mem->drop();
    delete mem;
    mrc = nullptr;
  }
  if (env_proc_id_pool()) {
    delete env_proc_id_pool();
  }
}

/** ?????????????????????????????????????????? */
static void mem_oriented_loop(MemReactContext *ctx, LmnMembraneRef mem) {
  while (!ctx->memstack_isempty()) {
    LmnMembraneRef mem = ctx->memstack_peek();
    if (!react_all_rulesets(ctx, mem)) {
      /* ???????????????????????????????????????????????????????????????????????????????????? */
      ctx->memstack_pop();
    }
  }
}

/**
 * @brief ?????????0step??????????????????????????????????????????????????????
 */
bool react_zerostep_rulesets(LmnReactCxtRef rc, LmnMembraneRef cur_mem) {
  auto &rulesets = cur_mem->get_rulesets();
  BOOL reacted = FALSE;
  bool reacted_any = false;

  rc->is_zerostep = true;
  do {
    reacted = FALSE;
    for (int i = 0; i < rulesets.size(); i++) {
      LmnRuleSetRef rs = rulesets[i];
      if (!rs->is_zerostep())
        continue;
      reacted |= react_ruleset(rc, cur_mem, rs);
    }
    reacted_any |= reacted;
  } while (reacted);
  rc->is_zerostep = false;

  return reacted_any;
}

/**
 * @brief ??????????????????????????????0step?????????????????????????????????
 * @sa react_zerostep_rulesetsm
 */
void react_zerostep_recursive(LmnReactCxtRef rc, LmnMembraneRef cur_mem) {
  for (; cur_mem; cur_mem = cur_mem->mem_next()) {
    react_zerostep_recursive(rc, cur_mem->mem_child_head());
    react_zerostep_rulesets(rc, cur_mem);
  }
}

/** cur_mem???????????????????????????????????????????????????????????????
 * @see mem_oriented_loop (task.c)
 * @see expand_inner      (nd.c) */
BOOL react_all_rulesets(LmnReactCxtRef rc, LmnMembraneRef cur_mem) {
  unsigned int i;
  auto &rulesets = cur_mem->get_rulesets(); /* ???????????????????????????????????? */
  BOOL ok = FALSE;

  /* ??????????????????????????? */
  for (i = 0; i < rulesets.size(); i++) {
    if (react_ruleset(rc, cur_mem, rulesets[i])) {
      /* nd????????????????????????????????????????????????????????????????????????????????????FALSE??????????????????
       */
      ok = TRUE;
      break;
    }
  }

#ifdef USE_FIRSTCLASS_RULE
  for (i = 0; i < (cur_mem->firstclass_rulesets())->get_num(); i++) {
    if (react_ruleset(
            rc, cur_mem,
            (LmnRuleSetRef)(cur_mem->firstclass_rulesets())->get(i))) {
      ok = TRUE;
      break;
    }
  }
#endif

  /* ??????????????????, ??????????????????????????????????????????????????????????????????????????????
   * nd??????ok???FALSE?????????, system_ruleset??????????????????. */
  ok = ok || react_ruleset(rc, cur_mem, system_ruleset);

#ifdef USE_FIRSTCLASS_RULE
  lmn_rc_execute_insertion_events(rc);
#endif

  return ok;
}

/** ???mem??????????????????????????????rs????????????????????????????????????.
 *  ?????????:
 *   ??????????????????, ?????????????????????????????????TRUE,
 * ???????????????????????????????????????FALSE?????????.
 *   ???????????????????????????FALSE?????????(?????????????????????????????????????????????????????????????????????).
 */
static inline BOOL react_ruleset(LmnReactCxtRef rc, LmnMembraneRef mem,
                                 LmnRuleSetRef rs) {
  for (auto r : *rs) {
#ifdef PROFILE
    if (!lmn_env.nd && lmn_env.profile_level >= 2)
      profile_rule_obj_set(rs, r);
#endif
    if (react_rule(rc, mem, r))
      return true;
  }
  return false;
}

/** ???mem?????????????????????rule?????????????????????.
 *  ?????????:
 *   ??????????????????, ?????????????????????????????????TRUE,
 * ???????????????????????????????????????FALSE?????????. ?????????????????????,
 * ????????????????????????????????????????????????????????????????????????????????????FALSE?????????. */
BOOL react_rule(LmnReactCxtRef rc, LmnMembraneRef mem, LmnRuleRef rule) {
  LmnTranslated translated;
  BYTE *inst_seq;
  BOOL result;

  translated = rule->translated;
  inst_seq = rule->inst_seq;

  rc->resize(1);
  rc->wt(0) = (LmnWord)mem;
  rc->tt(0) = TT_MEM;

  profile_start_trial();

  if (lmn_env.enable_parallel && !lmn_env.nd)
    rule_wall_time_start();

  /* ????????????????????????????????????????????????????????????
   * ????????????????????????????????????interpret??????????????? */
  slim::vm::interpreter in(rc, rule, inst_seq);
  result = (translated && translated(rc, mem, rule)) || (inst_seq && in.run());

  if (lmn_env.enable_parallel && !lmn_env.nd && normal_parallel_flag)
    rule_wall_time_finish();

  /* ????????????????????????0step????????????????????????????????????????????????????????? */
  if (result && !rc->is_zerostep) {
    bool reacted = react_zerostep_rulesets(rc, mem);
    if (reacted && rc->has_mode(REACT_MEM_ORIENTED)) {
      // zerostep????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
      // ????????????????????????mem???????????????????????????????????????????????????mem??????????????????????????????????????????
      ((MemReactContext *)rc)->memstack_reconstruct(rc->get_global_root());
    }
  }

  profile_finish_trial();

  if (rc->has_mode(REACT_MEM_ORIENTED) && !rc->is_zerostep) {
    if (lmn_env.trace && result) {
      if (lmn_env.sp_dump_format == LMN_SYNTAX) {
        lmn_dump_mem_stdout(rc->get_global_root());
        fprintf(stdout, ".\n");
        lmn_dump_mem_stdout(rc->get_global_root());
        fprintf(stdout, ":- ");
        rc->increment_reaction_count();
      } else if (lmn_env.output_format == JSON) {
        lmn_dump_cell_stdout(rc->get_global_root());
      } else {
        fprintf(stdout, "---->%s\n", lmn_id_to_name(rule->name));
        rc->increment_reaction_count();
        fprintf(stdout, "%d: ", rc->get_reaction_count());
        lmn_dump_cell_stdout(rc->get_global_root());
        if (lmn_env.show_hyperlink)
          lmn_hyperlink_print(rc->get_global_root());
      }
    }
  }

  if (rc->get_hl_sameproccxt()) {
    rc->clear_hl_spc(); /* ?????????????????????????????? */
    // normal parallel destroy
    if (lmn_env.enable_parallel && !lmn_env.nd) {
      int i;
      for (i = 0; i < lmn_env.core_num; i++) {
        thread_info[i]->rc->clear_hl_spc();
      }
    }
  }

  return result;
}

/* ???mem???rulesets??????????????????????????????.
 * ??????????????????????????? */
void react_start_rulesets(LmnMembraneRef mem, Vector *rulesets) {
  LmnReactCxt rc(mem, REACT_STAND_ALONE);
  int i;

  for (i = 0; i < rulesets->get_num(); i++) {
    react_ruleset(&rc, mem, (LmnRuleSetRef)rulesets->get(i));
  }
  react_initial_rulesets(&rc, mem);
  react_zerostep_recursive(&rc, mem);

#ifdef USE_FIRSTCLASS_RULE
  // register first-class rulesets produced by the initial process.
  lmn_rc_execute_insertion_events(&rc);
#endif
}

inline static void react_initial_rulesets(LmnReactCxtRef rc,
                                          LmnMembraneRef mem) {
  BOOL reacted;

  do {
    reacted = FALSE;
    if (react_ruleset_in_all_mem(rc, initial_system_ruleset, mem)) {
      reacted = TRUE;
      continue;
    }
    for (auto r : *initial_ruleset) {
      if (react_rule(rc, mem, r)) {
        reacted = TRUE;
        break;
      }
    }
  } while (reacted);
}

/* ??????????????????rs???mem?????????????????????????????????????????? */
static BOOL react_ruleset_in_all_mem(LmnReactCxtRef rc, LmnRuleSetRef rs,
                                     LmnMembraneRef mem) {
  LmnMembraneRef m;

  for (m = mem->mem_child_head(); m; m = m->mem_next()) {
    if (react_ruleset_in_all_mem(rc, rs, m))
      return TRUE;
  }

  return react_ruleset(rc, mem, rs);
}

/* Utility for reading data */

#define READ_DATA_ATOM(dest, attr)                                             \
  do {                                                                         \
    switch (attr) {                                                            \
    case LMN_INT_ATTR: {                                                       \
      long n;                                                                  \
      READ_VAL(long, instr, n);                                                \
      (dest) = (LmnAtomRef)n;                                                  \
      break;                                                                   \
    }                                                                          \
    case LMN_DBL_ATTR: {                                                       \
      double x;                                                                \
      READ_VAL(double, instr, x);                                              \
      (dest) = (LmnAtomRef)lmn_create_double_atom(x);                          \
      break;                                                                   \
    }                                                                          \
    case LMN_STRING_ATTR: {                                                    \
      lmn_interned_str s;                                                      \
      READ_VAL(lmn_interned_str, instr, s);                                    \
      (dest) = (LmnAtomRef) new LmnString(lmn_id_to_name(s));                  \
      break;                                                                   \
    }                                                                          \
    default:                                                                   \
      lmn_fatal("Implementation error 1");                                     \
    }                                                                          \
  } while (0)

/* attr????????????????????????????????????????????????dest????????????????????????????????????
 * attr????????????????????????????????????????????? */
#define READ_CONST_DATA_ATOM(dest, attr, type)                                 \
  do {                                                                         \
    switch (attr) {                                                            \
    case LMN_INT_ATTR:                                                         \
      READ_VAL(long, instr, (dest));                                           \
      break;                                                                   \
    case LMN_DBL_ATTR: {                                                       \
      double x;                                                                \
      READ_VAL(double, instr, x);                                              \
      (dest) = (LmnWord)lmn_create_double_atom(x);                             \
      break;                                                                   \
    }                                                                          \
    case LMN_STRING_ATTR: {                                                    \
      lmn_interned_str s;                                                      \
      READ_VAL(lmn_interned_str, instr, s);                                    \
      (dest) = s;                                                              \
      (attr) = LMN_CONST_STR_ATTR;                                             \
      break;                                                                   \
    }                                                                          \
    default:                                                                   \
      lmn_fatal("Implementation error 2");                                     \
    }                                                                          \
    (type) = TT_OTHER;                                                         \
  } while (0)

#define READ_CMP_DATA_ATOM(attr, x, result, type)                              \
  do {                                                                         \
    switch (attr) {                                                            \
    case LMN_INT_ATTR: {                                                       \
      long t;                                                                  \
      READ_VAL(long, instr, t);                                                \
      (result) = ((long)(x) == t);                                             \
      break;                                                                   \
    }                                                                          \
    case LMN_DBL_ATTR: {                                                       \
      double t;                                                                \
      READ_VAL(double, instr, t);                                              \
      (result) = (lmn_get_double(x) == t);                                     \
      break;                                                                   \
    }                                                                          \
    case LMN_STRING_ATTR: {                                                    \
      lmn_interned_str s;                                                      \
      LmnStringRef str1;                                                       \
      READ_VAL(lmn_interned_str, instr, s);                                    \
      str1 = new LmnString(lmn_id_to_name(s));                                 \
      (result) = *str1 == *(LmnStringRef)(x);                                  \
      delete (str1);                                                           \
      break;                                                                   \
    }                                                                          \
    default:                                                                   \
      lmn_fatal("Implementation error 3");                                     \
    }                                                                          \
    (type) = TT_ATOM;                                                          \
  } while (0)

#define SKIP_DATA_ATOM(attr)                                                   \
  do {                                                                         \
    switch (attr) {                                                            \
    case LMN_INT_ATTR: {                                                       \
      SKIP_VAL(long, instr);                                                   \
      break;                                                                   \
    }                                                                          \
    case LMN_DBL_ATTR: {                                                       \
      SKIP_VAL(double, instr);                                                 \
      break;                                                                   \
    }                                                                          \
    case LMN_STRING_ATTR: {                                                    \
      SKIP_VAL(lmn_interned_str, instr);                                       \
      break;                                                                   \
    }                                                                          \
    default:                                                                   \
      lmn_fatal("Implementation error 4");                                     \
    }                                                                          \
  } while (0)

/* DEBUG: */
/* static void print_wt(void); */

/* mem != NULL ????????? mem???UNIFY?????????????????????????????????UNIFY???????????????????????? */
HashSet *insertconnectors(slim::vm::RuleContext *rc, LmnMembraneRef mem,
                          const Vector *links) {
  unsigned int i, j;
  HashSet *retset;
  /* EFFICIENCY: retset???Hash Set????????????????????????????????????????????????????
   * ???????????????????????????????????????????????????????????? */

  retset = new HashSet(8);
  for (i = 0; i < links->get_num(); i++) {
    LmnWord linkid1 = links->get(i);
    if (LMN_ATTR_IS_DATA(rc->at(linkid1)))
      continue;
    for (j = i + 1; j < links->get_num(); j++) {
      LmnWord linkid2 = links->get(j);
      if (LMN_ATTR_IS_DATA(rc->at(linkid2)))
        continue;
      /* is buddy? */
      if ((LmnAtomRef)rc->wt(linkid2) == ((LmnSymbolAtomRef)rc->wt(linkid1))
                                      ->get_link(rc->at(linkid1)) &&
          rc->at(linkid2) == ((LmnSymbolAtomRef)rc->wt(linkid1))
                                      ->get_attr(rc->at(linkid1))) {
        /* '='????????????????????? */
        LmnSymbolAtomRef eq;
        LmnSymbolAtomRef a1, a2;
        LmnLinkAttr t1, t2;

        if (mem)
          eq = lmn_mem_newatom(mem, LMN_UNIFY_FUNCTOR);
        else {
          eq = lmn_new_atom(LMN_UNIFY_FUNCTOR);
        }

        if (eq->get_id() == 0) {
          eq->set_id(env_gen_next_id());
        }

        /* ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
         * ???????????????new_link???????????????????????????????????????*/

        a1 = (LmnSymbolAtomRef)rc->wt(linkid1);
        a2 = (LmnSymbolAtomRef)rc->wt(linkid2);
        t1 = rc->at(linkid1);
        t2 = rc->at(linkid2);

        lmn_newlink_in_symbols(a1, t1, eq, 0);
        lmn_newlink_in_symbols(a2, t2, eq, 1);
        retset->add((HashKeyType)eq);
      }
    }
  }

  return retset;
}

void slim::vm::interpreter::findatom(LmnReactCxtRef rc, LmnRuleRef rule,
                                     LmnRuleInstr instr, LmnMembrane *mem,
                                     LmnFunctor f, size_t reg) {
  auto atomlist_ent = mem->get_atomlist(f);

  if (!atomlist_ent)
    return;

  auto iter = std::begin(*atomlist_ent);
  auto end = std::end(*atomlist_ent);
  if (iter == end)
    return;

  auto v = std::vector<LmnRegister>();
  std::transform(iter, end, std::back_inserter(v), [](LmnSymbolAtomRef atom) {
    return LmnRegister({(LmnWord)atom, LMN_ATTR_MAKE_LINK(0), TT_ATOM});
  });

  this->false_driven_enumerate(reg, std::move(v));
}

/* ??????, ???????????????????????????????????????????????????(nakata) */
int rule_number;
bool slimopt_test_flag = false;
bool successflag = false;
bool debug_mode = false;
RecordList record_list(0);

void slim::vm::interpreter::findatomopt(LmnRuleRef rule, LmnMembrane *mem, LmnFunctor f, size_t reg) {
  slimopt_test_flag = true;
  AtomListEntryRef atomlist_ent = mem->get_atomlist(f);
  if(!atomlist_ent || atomlist_ent->n == atomlist_ent->n_record) {
    return;
  }

  // if the head of atomlist isn't an atom for record. make an atom and insert.
  if(!atomlist_ent->head->record_flag) {
    atomlist_ent->make_head_record();
  }
  // rule is applied findatomopt first time, rule_number will be change -1 to new number
  if(rule->rule_number == -1) {
    rule->rule_number = record_list.atoms.size();
    record_list.atoms.resize(record_list.atoms.size()+1);
    record_list.set_delete_flag(false);
    record_list.loop_flag.resize(record_list.loop_flag.size()+1);
    record_list.rule_reset.resize(record_list.rule_reset.size()+1);
    record_list.start_flag.resize(record_list.start_flag.size()+1);
  }
  
  if(record_list.rule_reset[rule->rule_number].size() < f+1) record_list.rule_reset[rule->rule_number].resize(f+1);

  record_list.rule_reset[rule->rule_number][f] = true;
  record_list.set_rule_number(rule->rule_number);

  if(record_list.atoms[rule->rule_number].size() < reg) {
    record_list.push_back_newatom(atomlist_ent, rule->rule_number, reg);
  }

  auto head = atomlist_ent->head;
  
  record_list.loop(atomlist_ent, rule->rule_number, reg);
  
  AtomListEntry::const_iterator begin;
  AtomListEntry::const_iterator end;

  begin = atomlist_ent->make_iterator(record_list.atoms[rule->rule_number][reg-1]->get_record()->next);
  end = std::end(*atomlist_ent);

  if(begin == end) return;

  auto v = std::vector<LmnRegister>();

  std::transform(begin, end, std::back_inserter(v), [](LmnSymbolAtomRef atom) {
						      return LmnRegister({(LmnWord)atom, LMN_ATTR_MAKE_LINK(0), TT_ATOM});
						    });
  this->false_driven_enumerate(reg, std::move(v));
  
}

/* ????????????(nakata)*/

/** find atom with a hyperlink occurred in the current rule for the first time.
 */
void slim::vm::interpreter::findatom_original_hyperlink(
    LmnReactCxtRef rc, LmnRuleRef rule, LmnRuleInstr instr, SameProcCxt *spc,
    LmnMembrane *mem, LmnFunctor f, size_t reg) {
  auto atomlist_ent = mem->get_atomlist(f);
  if (!atomlist_ent)
    return;

  auto filtered = slim::element::make_range_remove_if(
      std::begin(*atomlist_ent), std::end(*atomlist_ent),
      [=](LmnSymbolAtomRef atom) { return !spc->is_consistent_with(atom); });
  auto v = std::vector<std::function<LmnRegister(void)>>();
  std::transform(
      filtered.begin(), filtered.end(), std::back_inserter(v),
      [=](LmnSymbolAtomRef atom) {
        return [=]() {
          spc->match(atom);
          return LmnRegister({(LmnWord)atom, LMN_ATTR_MAKE_LINK(0), TT_ATOM});
        };
      });

  this->false_driven_enumerate(reg, std::move(v));
}

void lmn_hyperlink_get_elements(std::vector<HyperLink *> &tree,
                                HyperLink *start_hl) {
  Vector vec;
  vec.init(1);
  start_hl->get_elements(&vec);
  for (int i = 0; i < vec.get_num(); i++)
    tree.push_back((HyperLink *)vec.get(i));
  vec.destroy();
}

/** find atom with a hyperlink occurred again in the current rule. */
void slim::vm::interpreter::findatom_clone_hyperlink(
    LmnReactCxtRef rc, LmnRuleRef rule, LmnRuleInstr instr, SameProcCxt *spc,
    LmnMembrane *mem, LmnFunctor f, size_t reg) {
  /* ????????????????????????hyperlink??????????????????????????????hyperlink??????
   * (Vector*)LMN_SPC_TREE(spc)?????????.
   * (Vector*)LMN_SPC_TREE(spc)?????????????????????????????????HyperLink???
   * ???????????????????????????????????????-1?????? */
  HyperLink *h = spc->start();
  lmn_hyperlink_get_elements(spc->tree, h);
  auto element_num = spc->tree.size() - 1;
  if (element_num <= 0)
    return;

  std::vector<HyperLink *> children;
  for (int i = 0; i < element_num; i++)
    children.push_back(spc->tree[i]);

  /* ----------------------------------------------------------
   * ???????????????????????????????????????????????????????????????????????????spc???????????????????????????
   * ---------------------------------------------------------- */

  if (!mem->get_atomlist(LMN_HL_FUNC))
    return;

  auto filtered = slim::element::make_range_remove_if(
      children.begin(), children.end(), [=](HyperLink *h) {
        auto linked_pc = h->atom;
        auto hl_atom = (LmnSymbolAtomRef)linked_pc->get_link(0);
        /*    ??????findatom????????????????????????????????????????????????????????????
         * || hyperlink?????????????????????????????????????????????
         * || ??????findatom??????????????????????????????????????????????????????????????????
         */
        return hl_atom->get_functor() != f ||
               spc->start_attr != linked_pc->get_attr(0) ||
               LMN_HL_MEM(lmn_hyperlink_at_to_hl(linked_pc)) != mem ||
               !spc->is_consistent_with(hl_atom);
      });
  std::vector<std::function<LmnRegister(void)>> v;
  std::transform(
      filtered.begin(), filtered.end(), std::back_inserter(v),
      [=](HyperLink *h) {
        return [=]() {
          auto atom = (LmnSymbolAtomRef)h->atom->get_link(0);
          spc->match(atom);
          return LmnRegister({(LmnWord)atom, LMN_ATTR_MAKE_LINK(0), TT_ATOM});
        };
      });

  this->false_driven_enumerate(reg, std::move(v));
}

/** hyperlink?????????????????????findatom */
void slim::vm::interpreter::findatom_through_hyperlink(
    LmnReactCxtRef rc, LmnRuleRef rule, LmnRuleInstr instr, SameProcCxt *spc,
    LmnMembrane *mem, LmnFunctor f, size_t reg) {
  auto atom_arity = LMN_FUNCTOR_ARITY(lmn_functor_table, f);

  /* ???????????????????????????atomi???original/clone?????????????????????????????? */
  if (spc->is_clone(atom_arity)) {
    findatom_clone_hyperlink(rc, rule, instr, spc, mem, f, reg);
  } else {
    findatom_original_hyperlink(rc, rule, instr, spc, mem, f, reg);
  }
}

BOOL ground_atoms(Vector *srcvec, Vector *avovec,
                  std::unique_ptr<ProcessTbl> &atoms, unsigned long *natoms,
                  std::unique_ptr<ProcessTbl> &hlinks,
                  const std::vector<LmnFunctor> &attr_functors,
                  const std::vector<LmnWord> &attr_dataAtoms,
                  const std::vector<LmnLinkAttr> &attr_dataAtom_attrs) {
  auto v1 = new Vector(attr_dataAtoms.size());
  auto v2 = new Vector(attr_dataAtom_attrs.size());
  auto p1 = new ProcessTbl(attr_functors.size());

  for (auto &v : attr_dataAtoms)
    v1->push(v);
  for (auto &v : attr_dataAtom_attrs)
    v2->push(v);
  for (auto &p : attr_functors)
    p1->proc_tbl_put(p, p);

  auto a = atoms.get();
  auto h = hlinks.get();
  auto result = ground_atoms(srcvec, avovec, &a, natoms, &h, &p1, v1, v2);
  atoms = std::unique_ptr<ProcessTbl>(a);
  hlinks = std::unique_ptr<ProcessTbl>(h);
  delete v1;
  delete v2;
  delete p1;
  return result;
}

BOOL ground_atoms(Vector *srcvec, Vector *avovec,
                  std::unique_ptr<ProcessTbl> &atoms, unsigned long *natoms) {
  auto a = atoms.get();
  auto result = ground_atoms(srcvec, avovec, &a, natoms, nullptr, nullptr,
                             nullptr, nullptr);
  atoms = std::unique_ptr<ProcessTbl>(a);
  return result;
}

std::vector<c17::variant<std::pair<LmnLinkAttr, LmnAtomRef>, LmnFunctor>>
read_unary_atoms(LmnReactCxt *rc, LmnRuleInstr &instr) {
  std::vector<c17::variant<std::pair<LmnLinkAttr, LmnAtomRef>, LmnFunctor>>
      args;
  LmnInstrVar n;
  READ_VAL(LmnInstrVar, instr, n);

  for (int i = 0; i < n; i++) {
    LmnLinkAttr attr;
    READ_VAL(LmnLinkAttr, instr, attr);
    if (LMN_ATTR_IS_DATA(attr)) {
      LmnAtomRef at;
      READ_DATA_ATOM(at, attr);
      args.push_back(std::make_pair(attr, at));
    } else {
      LmnFunctor f;
      READ_VAL(LmnFunctor, instr, f);
      args.push_back(f);
    }
  }
  return args;
}

std::vector<c17::variant<std::pair<LmnLinkAttr, LmnAtomRef>, LmnFunctor>>
read_unary_atoms_indirect(LmnReactCxt *rc, LmnRuleInstr &instr) {
  std::vector<c17::variant<std::pair<LmnLinkAttr, LmnAtomRef>, LmnFunctor>>
      args;
  LmnInstrVar n;
  READ_VAL(LmnInstrVar, instr, n);

  for (int i = 0; i < n; i++) {
    LmnInstrVar ai;
    READ_VAL(LmnInstrVar, instr, ai);
    if (LMN_ATTR_IS_DATA(rc->at(ai))) {
      args.push_back(std::make_pair(rc->at(ai), (LmnAtomRef)rc->wt(ai)));
    } else {
      args.push_back(((LmnSymbolAtomRef)rc->wt(ai))->get_functor());
    }
  }
  return args;
}

std::vector<HyperLink *> lmn_hyperlink_get_elements(HyperLink *start_hl) {
  std::vector<HyperLink *> vec;
  Vector tree;
  tree.init(32);
  start_hl->get_elements(&tree);
  for (int i = 0; i < tree.get_num() - 1; i++)
    vec.push_back((HyperLink *)tree.get(i));
  tree.destroy();
  return vec;
}

struct exec_subinstructions_while {
  bool executed; // ??????while????????????????????????true
  LmnRuleInstr body;
  LmnRuleInstr next;

  exec_subinstructions_while(LmnRuleInstr body, LmnRuleInstr next)
      : executed(false), body(body), next(next) {}

  slim::vm::interpreter::command_result
  operator()(slim::vm::interpreter &interpreter, bool result) {
    if (executed) // ???????????????????????????????????????????????????
      return result ? slim::vm::interpreter::command_result::Success
                    : slim::vm::interpreter::command_result::Failure;

    if (result) {
      interpreter.instr = body;
    } else {
      interpreter.instr = next;
      executed = true;
    }
    return slim::vm::interpreter::command_result::Trial;
  }
};

struct exec_subinstructions_not {
  bool executed;
  LmnRuleInstr next_instr;

  exec_subinstructions_not(LmnRuleInstr next_instr)
      : executed(false), next_instr(next_instr) {}

  slim::vm::interpreter::command_result
  operator()(slim::vm::interpreter &interpreter, bool result) {
    if (!executed) {
      // ?????????????????????????????????????????????
      if (result)
        return slim::vm::interpreter::command_result::Failure;

      // ?????????????????????????????????????????????
      interpreter.instr = next_instr;
      executed = true;
      return slim::vm::interpreter::command_result::Trial;
    } else {
      return result ? slim::vm::interpreter::command_result::Success
                    : slim::vm::interpreter::command_result::Failure;
    }
  }
};

struct exec_subinstructions_group {
  bool executed;
  LmnRuleInstr next_instr;

  exec_subinstructions_group(LmnRuleInstr next_instr)
      : executed(false), next_instr(next_instr) {}

  slim::vm::interpreter::command_result
  operator()(slim::vm::interpreter &interpreter, bool result) {
    if (!executed) {
      // ?????????????????????????????????????????????
      if (!result)
        return slim::vm::interpreter::command_result::Failure;

      // ?????????????????????????????????????????????
      interpreter.instr = next_instr;
      executed = true;
      return slim::vm::interpreter::command_result::Trial;
    } else {
      return result ? slim::vm::interpreter::command_result::Success
                    : slim::vm::interpreter::command_result::Failure;
    }
  }
};

struct exec_subinstructions_branch {
  bool executed;
  LmnRuleInstr next_instr;

  exec_subinstructions_branch(LmnRuleInstr next_instr)
      : executed(false), next_instr(next_instr) {}

  slim::vm::interpreter::command_result
  operator()(slim::vm::interpreter &interpreter, bool result) {
    if (!executed) {
      // ?????????????????????????????????????????????
      if (result)
        return slim::vm::interpreter::command_result::Success;

      // ?????????????????????????????????????????????
      interpreter.instr = next_instr;
      executed = true;
      return slim::vm::interpreter::command_result::Trial;
    } else {
      return result ? slim::vm::interpreter::command_result::Success
                    : slim::vm::interpreter::command_result::Failure;
    }
  }
};

/**
 *  execute a command at instr.
 *  instr is incremented by the size of operation.
 *  returns true if execution finished sucessfully.
 *  stop becomes true only if executien should be aborted.
 */
bool slim::vm::interpreter::exec_command(LmnReactCxt *rc, LmnRuleRef rule,
                                         bool &stop) {
  LmnInstrOp op;
  READ_VAL(LmnInstrOp, instr, op);
  stop = true;

  if (lmn_env.find_atom_parallel)
    return FALSE;

  switch (op) {
  case INSTR_SPEC: {
    LmnInstrVar s0;

    SKIP_VAL(LmnInstrVar, instr);
    READ_VAL(LmnInstrVar, instr, s0);

    rc->resize(s0);
    break;
  }
  case INSTR_INSERTCONNECTORSINNULL: {
    LmnInstrVar seti, list_num;
    Vector links;
    unsigned int i;

    READ_VAL(LmnInstrVar, instr, seti);
    READ_VAL(LmnInstrVar, instr, list_num);

    links.init(list_num + 1);
    for (i = 0; i < list_num; i++) {
      LmnInstrVar t;
      READ_VAL(LmnInstrVar, instr, t);
      links.push((LmnWord)t);
    }

    auto hashset = insertconnectors(rc, NULL, &links);
    rc->reg(seti) = {(LmnWord)hashset, 0, TT_OTHER};

    links.destroy();

    /* EFFICIENCY: ???????????????????????? */
    this->push_stackframe([=](interpreter &itr, bool result) {
      delete hashset;
      return result ? command_result::Success : command_result::Failure;
    });

    break;
  }
  case INSTR_INSERTCONNECTORS: {
    LmnInstrVar seti, list_num, memi, enti;
    Vector links; /* src list */
    unsigned int i;

    READ_VAL(LmnInstrVar, instr, seti);
    READ_VAL(LmnInstrVar, instr, list_num);

    links.init(list_num + 1);

    for (i = 0; i < list_num; i++) {
      READ_VAL(LmnInstrVar, instr, enti);
      links.push((LmnWord)enti);
    }

    READ_VAL(LmnInstrVar, instr, memi);
    auto hashset = insertconnectors(rc, (LmnMembraneRef)rc->wt(memi), &links);
    rc->reg(seti) = {(LmnWord)hashset, 0, TT_OTHER};

    links.destroy();

    /* EFFICIENCY: ???????????????????????? */
    this->push_stackframe([=](interpreter &itr, bool result) {
      delete hashset;
      return result ? command_result::Success : command_result::Failure;
    });

    break;
  }
  case INSTR_JUMP: {
    /* EFFICIENCY: ????????????????????????malloc?????????????????????????????????
                   -O3 ?????????????????????????????????JUMP????????????????????????
                   ?????????????????? */
    LmnRuleInstr next;
    LmnInstrVar num, i, n;
    LmnJumpOffset offset;

    auto v = LmnRegisterArray(rc->capacity());

    READ_VAL(LmnJumpOffset, instr, offset);
    next = instr + offset;

    i = 0;
    /* atom */
    READ_VAL(LmnInstrVar, instr, num);
    for (; num--; i++) {
      READ_VAL(LmnInstrVar, instr, n);
      v.at(i) = rc->reg(n);
    }
    /* mem */
    READ_VAL(LmnInstrVar, instr, num);
    for (; num--; i++) {
      READ_VAL(LmnInstrVar, instr, n);
      v.at(i) = rc->reg(n);
    }
    /* vars */
    READ_VAL(LmnInstrVar, instr, num);
    for (; num--; i++) {
      READ_VAL(LmnInstrVar, instr, n);
      v.at(i) = rc->reg(n);
    }

    instr = next;

    // use pointer to avoid copy capture.
    auto tmp = new std::vector<LmnRegister>(std::move(rc->work_array));
    rc->warray_set(std::move(v));

    this->push_stackframe([=](interpreter &itr, bool result) {
      rc->warray_set(std::move(*tmp));
      delete tmp;
      return result ? command_result::Success : command_result::Failure;
    });
    break;
  }
  case INSTR_RESETVARS: {
    LmnInstrVar num, i, n, t;

    auto v = LmnRegisterArray(rc->capacity());

    i = 0;
    /* atom */
    READ_VAL(LmnInstrVar, instr, num);
    for (; num--; i++) {
      READ_VAL(LmnInstrVar, instr, n);
      v.at(i) = rc->reg(n);
    }

    /* mem */
    READ_VAL(LmnInstrVar, instr, num);
    for (; num--; i++) {
      READ_VAL(LmnInstrVar, instr, n);
      v.at(i) = rc->reg(n);
    }

    /* vars */
    READ_VAL(LmnInstrVar, instr, num);
    for (; num--; i++) {
      READ_VAL(LmnInstrVar, instr, n);
      v.at(i) = rc->reg(n);
    }

    for (t = 0; t <= i; t++) {
      rc->reg(t) = v.at(t);
    }

    break;
  }
  case INSTR_COMMIT: {
    lmn_interned_str rule_name;

    READ_VAL(lmn_interned_str, instr, rule_name);
    SKIP_VAL(LmnLineNum, instr);

    if (lmn_env.findatom_parallel_mode) {
      lmn_fatal("Couldn't find sync instruction!!");
    }

#ifdef KWBT_OPT
    {
      LmnInstrVar cost;
      READ_VAL(LmnInstrVar, instr, cost);
      rule->cost = cost;
    }
#endif

    rule->name = rule_name;

    profile_apply();

    /*
     * MC mode
     *
     * ?????????????????????global_root?????????????????????????????????????????????????????????????????????
     * ????????????????????????????????????????????????????????????
     * ????????????????????????????????????????????????????????????????????????????????????????????????????????????
     *
     * CONTRACT: COMMIT???????????????????????????????????????????????????????????????????????????
     */
    if (rc->has_mode(REACT_ND) && !rc->is_zerostep) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      ProcessID org_next_id = env_next_id();

      if (mcrc->has_optmode(DeltaMembrane)) {
        /** >>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<< **/
        /** >>>>>>>> enable delta-membrane <<<<<<< **/
        /** >>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<< **/
        struct MemDeltaRoot *d =
            new MemDeltaRoot(rc->get_global_root(), rule, env_next_id());
        RC_ND_SET_MEM_DELTA_ROOT(rc, d);

        /* dmem_commit/revert??????????????????????????????,
         * uniq??????????????????????????????????????? */
        rule->undo_history();

        if (mcrc->has_optmode(DynamicPartialOrderReduction)) {
          dpor_transition_gen_LHS(RC_POR_DATA(rc), d, rc);
        }

        dmem_interpret(rc, rule, instr);
        dmem_root_finish(d);

        if (mcrc->has_optmode(DynamicPartialOrderReduction)) {
          if (!dpor_transition_gen_RHS(RC_POR_DATA(rc), d, mcrc)) {
            delete d;
          } else {
            mc_react_cxt_add_mem_delta(mcrc, d, rule);
          }

          /* ???????????????????????????????????????????????????????????????????????????????????????,
           * ????????????????????????????????????????????????????????????ID????????????????????????????????????.
           */
          RC_ND_SET_MEM_DELTA_ROOT(rc, NULL);
          return FALSE;
        }

        mc_react_cxt_add_mem_delta(mcrc, d, rule);
        RC_ND_SET_MEM_DELTA_ROOT(rc, NULL);
      } else {
        /** >>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<< **/
        /** >>>>>>> disable delta-membrane <<<<<<< **/
        /** >>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<< **/
        ProcessTableRef copymap;
        LmnMembraneRef tmp_global_root;
        unsigned int i, n;

#ifdef PROFILE
        if (lmn_env.profile_level >= 3) {
          profile_start_timer(PROFILE_TIME__STATE_COPY_IN_COMMIT);
        }
#endif

        tmp_global_root = lmn_mem_copy_with_map_ex(rc->get_global_root(), &copymap);

        /** ????????????????????????????????????????????? */
        auto v = LmnRegisterArray(rc->capacity());

        /** copymap????????????????????????????????????????????? */
        for (i = 0; i < rc->capacity(); i++) {
          LmnWord t;
          LmnRegisterRef r = &v.at(i);
          r->register_set_at(rc->at(i));
          r->register_set_tt(rc->tt(i));

          if (r->register_tt() == TT_ATOM) {
            if (LMN_ATTR_IS_DATA(r->register_at())) {
              /* data-atom */
              if (r->register_at() == LMN_HL_ATTR) {
                if (proc_tbl_get_by_hlink(
                        copymap,
                        lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)rc->wt(i)),
                        &t)) {
                  r->register_set_wt((LmnWord)((HyperLink *)t)->hl_to_at());
                } else {
                  r->register_set_wt(
                      (LmnWord)rc->wt(i)); /* new_hlink?????????????????? */
                }
              } else {
                r->register_set_wt((LmnWord)lmn_copy_data_atom(
                    (LmnAtom)rc->wt(i), r->register_at()));
              }
            } else if (proc_tbl_get_by_atom(copymap,
                                            (LmnSymbolAtomRef)rc->wt(i), &t)) {
              /* symbol-atom */
              r->register_set_wt((LmnWord)t);
            } else {
              t = 0;
            }
          } else if (r->register_tt() == TT_MEM) {
            if (rc->wt(i) ==
                (LmnWord)rc->get_global_root()) { /* ??????????????????????????? */
              r->register_set_wt((LmnWord)tmp_global_root);
            } else if (proc_tbl_get_by_mem(copymap, (LmnMembraneRef)rc->wt(i),
                                           &t)) {
              r->register_set_wt((LmnWord)t);
            } else {
              t = 0;
              //              v[i].wt = wt(rc, i); //
              //              allocmem??????????????????TT_OTHER??????????????????(2014-05-08
              //              ueda)
            }
          } else { /* TT_OTHER */
            r->register_set_wt(rc->wt(i));
          }
        }
        delete copymap;

        /** ????????????????????????????????????????????????????????????, ?????????????????????????????? */
        auto tmp = new std::vector<LmnRegister>(std::move(rc->work_array));
        rc->warray_set(std::move(v));

#ifdef PROFILE
        if (lmn_env.profile_level >= 3) {
          profile_finish_timer(PROFILE_TIME__STATE_COPY_IN_COMMIT);
        }
#endif

        this->push_stackframe([=](interpreter &itr, bool result) {
          react_zerostep_recursive(
              rc, tmp_global_root); /**< 0step???????????????????????? */
          mc_react_cxt_add_expanded(mcrc, tmp_global_root, rule);

          rule->undo_history();

          rc->warray_set(std::move(*tmp));
          if (!rc->keep_process_id_in_nd_mode)
            env_set_next_id(org_next_id);
          delete tmp;
          return command_result::Failure;
        });
        break;
      }

      return FALSE; /* matching backtrack! */
    } else if (rc->has_mode(REACT_PROPERTY)) {
      return TRUE; /* property???matching?????? */
    }

    break;
  }
  case INSTR_FINDATOM: {
    LmnInstrVar atomi, memi;
    LmnLinkAttr attr;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnLinkAttr, instr, attr);

    if (LMN_ATTR_IS_DATA(attr))
      throw std::runtime_error("cannot find data atoms.");

    if (lmn_env.find_atom_parallel)
      return false;

    LmnFunctor f;
    READ_VAL(LmnFunctor, instr, f);
    auto mem = (LmnMembraneRef)rc->wt(memi);

    if (rc_hlink_opt(atomi, rc)) {
      /* hyperlink ??????????????????????????????????????????????????????????????? */
      if (!rc->get_hl_sameproccxt())
        rc->prepare_hl_spc();
      auto spc =
          (SameProcCxt *)hashtbl_get(rc->get_hl_sameproccxt(), (HashKeyType)atomi);
      findatom_through_hyperlink(rc, rule, instr, spc, mem, f, atomi);
    } else {
      if(slimopt_test_flag) findatomopt(rule, mem, f, atomi);
      else findatom(rc, rule, instr, mem, f, atomi);
    }

    return false; // false driven loop
  }
  case INSTR_FINDATOM2: {
    LmnInstrVar atomi, memi, findatomid;
    LmnLinkAttr attr;

    if (rc->has_mode(REACT_ND) && !rc->is_zerostep) {
      lmn_fatal("This mode:exhaustive search can't use instruction:FindAtom2");
    }

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, findatomid);
    READ_VAL(LmnLinkAttr, instr, attr);
    if (LMN_ATTR_IS_DATA(attr)) {
      lmn_fatal("I can not find data atoms.\n");
    } else { /* symbol atom */
      LmnFunctor f;
      AtomListEntryRef atomlist_ent;
      AtomListEntry::const_iterator start_atom(nullptr, nullptr);

      READ_VAL(LmnFunctor, instr, f);
      atomlist_ent = ((LmnMembraneRef)rc->wt(memi))->get_atomlist(f);
      if (!atomlist_ent)
        return false;

      auto record = atomlist_ent->find_record(findatomid);
      if (record == atomlist_ent->end()) {
        start_atom = std::begin(*atomlist_ent);
        record =
            atomlist_ent->insert(findatomid, lmn_new_atom(LMN_RESUME_FUNCTOR));
      } else {
        start_atom = std::next(record, 1);
      }

      std::vector<AtomListEntry::const_iterator> its;
      for (auto it = start_atom; it != std::end(*atomlist_ent); ++it)
        its.push_back(it);
      auto filtered = slim::element::make_range_remove_if(
          its.begin(), its.end(),
          [=](const AtomListEntry::const_iterator &atom) {
            return (*atom)->get_functor() == LMN_RESUME_FUNCTOR;
          });
      std::vector<std::function<LmnRegister(void)>> candidates;
      std::transform(
          filtered.begin(), filtered.end(), std::back_inserter(candidates),
          [=](const AtomListEntry::const_iterator &it) {
            return [=]() {
              atomlist_ent->splice(std::prev(it, 1), *atomlist_ent, record);
              return LmnRegister(
                  {(LmnWord)*it, LMN_ATTR_MAKE_LINK(0), TT_ATOM});
            };
          });

      this->false_driven_enumerate(atomi, std::move(candidates));

      /* ?????????findatom2?????????????????????????????????cf.
       * ?????????Wikifindatom2????????????
       * ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
       * ???????????????????????????????????????????????????????????????????????? */
      auto v = std::vector<LmnRegister>();
      std::transform(
          atomlist_ent->begin(), start_atom, std::back_inserter(v),
          [=](LmnSymbolAtomRef atom) {
            return LmnRegister({(LmnWord)atom, LMN_ATTR_MAKE_LINK(0), TT_ATOM});
          });

      this->false_driven_enumerate(atomi, std::move(v));

      return false;
    }
    break;
  }
  case INSTR_FINDATOMP: { // TODO: refactoring
    if (!lmn_env.enable_parallel || lmn_env.nd) {
      REWRITE_VAL(LmnInstrOp, instr, INSTR_FINDATOM);
      break;
    }
    LmnInstrVar atomi, memi;
    LmnLinkAttr attr;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnLinkAttr, instr, attr);

    if (LMN_ATTR_IS_DATA(attr)) {
      lmn_fatal("I can not find data atoms.\n");
    } else { /* symbol atom */
      LmnFunctor f;

      READ_VAL(LmnFunctor, instr, f);

      auto atom_arity = LMN_FUNCTOR_ARITY(lmn_functor_table, f);

      if (rc_hlink_opt(atomi, rc)) {
        SameProcCxt *spc;

        if (!rc->get_hl_sameproccxt()) {
          rc->prepare_hl_spc();
        }

        /* ???????????????????????????atomi???original/clone?????????????????????????????? */
        spc = (SameProcCxt *)hashtbl_get(rc->get_hl_sameproccxt(), (HashKeyType)atomi);
        if (spc->is_clone(atom_arity)) {
          lmn_fatal(
              "Can't use hyperlink searching in parallel-runtime mode.\n");
        }
      }
      auto atomlist_ent = ((LmnMembraneRef)rc->wt(memi))->get_atomlist(f);
      if (atomlist_ent)
        return false;
      ///
      int ip;
      LmnInstrVar i;
      BOOL judge;
      LmnSymbolAtomRef atom;

      normal_parallel_flag = TRUE;

      while (!temp->is_empty()) {
        ip = (int)(temp->pop_head());
        atom = (LmnSymbolAtomRef)thread_info[ip]->rc->wt(atomi);
        if (check_exist(atom, f)) {
          rc->reg(atomi) = {(LmnWord)atom, LMN_ATTR_MAKE_LINK(0), TT_ATOM};
          if (rc_hlink_opt(atomi, rc)) {
            auto spc = (SameProcCxt *)hashtbl_get(rc->get_hl_sameproccxt(),
                                                  (HashKeyType)atomi);

            if (spc->is_consistent_with((LmnSymbolAtomRef)rc->wt(atomi))) {
              spc->match((LmnSymbolAtomRef)rc->wt(atomi));
              if (interpret(rc, rule, instr)) {
                success_temp_check++;
                return TRUE;
              }
            }
          } else {
            if (interpret(rc, rule, instr)) {
              success_temp_check++;
              return TRUE;
            }
          }
        }
        fail_temp_check++;
      }

      active_thread = std::min((unsigned int)atomlist_ent->n, lmn_env.core_num);

      lmn_env.findatom_parallel_mode = TRUE;
      for (ip = 0, atom = atomlist_head(atomlist_ent); ip < active_thread;
           atom = atom->get_next(), ip++) {
        // pthread create
        if (lmn_env.find_atom_parallel)
          break;
        if (!check_exist((LmnSymbolAtomRef)thread_info[ip]->next_atom, f) ||
            atom == thread_info[ip]->next_atom ||
            lmn_env.findatom_parallel_inde)
          thread_info[ip]->next_atom = NULL;
        threadinfo_init(ip, atomi, rule, rc, instr, atomlist_ent, atom_arity);
        //
        pthread_mutex_unlock(thread_info[ip]->exec);
      }
      for (auto ip2 = 0; ip2 < ip; ip2++) {
        // lmn_thread_join(findthread[ip2]);
        op_lock(ip2, 0);
        profile_backtrack_add(thread_info[ip2]->backtrack);
        thread_info[ip2]->profile->backtrack_num += thread_info[ip2]->backtrack;
      }
      lmn_env.findatom_parallel_mode = FALSE;

      // copy register
      judge = TRUE;
      for (auto ip2 = 0; ip2 < ip; ip2++) {
        if (thread_info[ip2]->judge && judge) {
          rc->work_array = thread_info[ip2]->rc->work_array;
          if (lmn_env.trace)
            fprintf(stdout, "( Thread id : %d )", thread_info[ip2]->id);
          instr = instr_parallel;
          judge = FALSE;
          continue;
        }
        if (thread_info[ip2]->judge) {
          temp->push_head(ip2);
        }
      }

      if (!lmn_env.find_atom_parallel)
        return FALSE; // Can't find atom
      lmn_env.find_atom_parallel = FALSE;
      break; // Find atom!!
    }
    break;
  }
  case INSTR_SYNC: {
    if (lmn_env.findatom_parallel_mode) {
      lmn_env.find_atom_parallel = TRUE;
      instr_parallel = instr;
      return TRUE;
    }
    break;
  }

  case INSTR_LOCKMEM: {
    LmnInstrVar memi, atomi, memn;
    LmnMembraneRef m;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(lmn_interned_str, instr, memn);

    LMN_ASSERT(!LMN_ATTR_IS_DATA(rc->at(atomi)));
    LMN_ASSERT(LMN_IS_PROXY_FUNCTOR(
        ((LmnSymbolAtomRef)(rc->wt(atomi)))->get_functor()));
    //      LMN_ASSERT(((LmnMembraneRef)wt(rc, memi))->parent);

    m = LMN_PROXY_GET_MEM((LmnSymbolAtomRef)rc->wt(atomi));
    if (m->NAME_ID() != memn)
      return FALSE;
    rc->reg(memi) = {(LmnWord)m, 0, TT_MEM};
    break;
  }
  case INSTR_ANYMEM: {
    LmnInstrVar mem1, mem2, memn; /* dst, parent, type, name */

    READ_VAL(LmnInstrVar, instr, mem1);
    READ_VAL(LmnInstrVar, instr, mem2);
    SKIP_VAL(LmnInstrVar, instr);
    READ_VAL(lmn_interned_str, instr, memn);

    rc->at(mem1) = 0;
    rc->tt(mem1) = TT_MEM;
    auto children = slim::vm::membrane_children((LmnMembraneRef)rc->wt(mem2));
    auto filtered = slim::element::make_range_remove_if(
        children.begin(), children.end(),
        [=](LmnMembrane &m) { return (&m)->NAME_ID() != memn; });
    std::vector<LmnRegister> v;
    for (auto &m : filtered)
      v.push_back(LmnRegister({(LmnWord)&m, 0, TT_MEM}));

    this->false_driven_enumerate(mem1, std::move(v));
    return false;
  }
  case INSTR_NMEMS: {
    LmnInstrVar memi, nmems;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, nmems);

    if (!((LmnMembraneRef)rc->wt(memi))->nmems(nmems)) {
      return FALSE;
    }

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        LmnMembraneRef m = (LmnMembraneRef)rc->wt(memi);
        dpor_LHS_flag_add(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NMEMS);
        this->push_stackframe([=](interpreter &itr, bool result) {
          dpor_LHS_flag_remove(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NMEMS);
          return command_result::
              Failure; /* ?????????????????????????????????ND?????????FALSE???????????????
                        */
        });
      }
    }

    break;
  }
  case INSTR_NORULES: {
    LmnInstrVar memi;

    READ_VAL(LmnInstrVar, instr, memi);
    if (!((LmnMembraneRef)rc->wt(memi))->get_rulesets().empty())
      return FALSE;

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        LmnMembraneRef m = (LmnMembraneRef)rc->wt(memi);
        dpor_LHS_flag_add(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NORULES);
        this->push_stackframe([=](interpreter &itr, bool result) {
          dpor_LHS_flag_remove(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NORULES);
          return command_result::
              Failure; /* ?????????????????????????????????ND?????????FALSE???????????????
                        */
        });
      }
    }

    break;
  }
  case INSTR_NEWATOM: {
    LmnInstrVar atomi, memi;
    LmnAtomRef ap;
    LmnLinkAttr attr;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnLinkAttr, instr, attr);
    if (LMN_ATTR_IS_DATA(attr)) {
      READ_DATA_ATOM(ap, attr);
    } else { /* symbol atom */
      LmnFunctor f;

      READ_VAL(LmnFunctor, instr, f);
      ap = lmn_new_atom(f);
      ((LmnSymbolAtomRef)ap)->record_flag = false;
#ifdef USE_FIRSTCLASS_RULE
      if (f == LMN_COLON_MINUS_FUNCTOR) {
        lmn_rc_push_insertion(rc, (LmnSymbolAtomRef)ap,
                              (LmnMembraneRef)wt(rc, memi));
      }
#endif
    }
    lmn_mem_push_atom((LmnMembraneRef)rc->wt(memi), (LmnAtomRef)ap, attr);
    rc->reg(atomi) = {(LmnWord)ap, attr, TT_ATOM};
    break;
  }
  case INSTR_NATOMS: {
    LmnInstrVar memi, natoms;
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, natoms);

    if (!((LmnMembraneRef)rc->wt(memi))->natoms(natoms)) {
      return FALSE;
    }

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        LmnMembraneRef m = (LmnMembraneRef)rc->wt(memi);
        dpor_LHS_flag_add(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NATOMS);
        this->push_stackframe([=](interpreter &itr, bool result) {
          dpor_LHS_flag_remove(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NATOMS);
          return command_result::
              Failure; /* ?????????????????????????????????ND?????????FALSE???????????????
                        */
        });
      }
    }

    break;
  }
  case INSTR_NATOMSINDIRECT: {
    LmnInstrVar memi, natomsi;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, natomsi);

    if (!((LmnMembraneRef)rc->wt(memi))->natoms(rc->wt(natomsi))) {
      return FALSE;
    }

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        LmnMembraneRef m = (LmnMembraneRef)rc->wt(memi);
        dpor_LHS_flag_add(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NATOMS);
        this->push_stackframe([=](interpreter &itr, bool result) {
          dpor_LHS_flag_remove(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NATOMS);
          return command_result::
              Failure; /* ?????????????????????????????????ND?????????FALSE???????????????
                        */
        });
      }
    }

    break;
  }
  case INSTR_ALLOCLINK: {
    LmnInstrVar link, atom, n;

    READ_VAL(LmnInstrVar, instr, link);
    READ_VAL(LmnInstrVar, instr, atom);
    READ_VAL(LmnInstrVar, instr, n);

    if (LMN_ATTR_IS_DATA(rc->at(atom))) {
      rc->reg(link) = {rc->wt(atom), rc->at(atom), TT_ATOM};
    } else { /* link to atom */
      rc->reg(link) = {(LmnWord)LMN_SATOM(rc->wt(atom)), LMN_ATTR_MAKE_LINK(n),
                       TT_ATOM};
    }
    break;
  }
  case INSTR_UNIFYLINKS: {
    LmnInstrVar link1, link2, mem;
    LmnLinkAttr attr1, attr2;

    READ_VAL(LmnInstrVar, instr, link1);
    READ_VAL(LmnInstrVar, instr, link2);
    READ_VAL(LmnInstrVar, instr, mem);

    attr1 = rc->at(link1);
    attr2 = rc->at(link2);

    if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr1)) {
      if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) { /* 1, 2 are data */
        lmn_mem_link_data_atoms((LmnMembraneRef)rc->wt(mem),
                                (LmnAtomRef)rc->wt(link1), rc->at(link1),
                                (LmnAtomRef)rc->wt(link2), attr2);
      } else { /* 1 is data */
        ((LmnSymbolAtomRef)rc->wt(link2))
            ->set_link(LMN_ATTR_GET_VALUE(attr2), (LmnAtomRef)rc->wt(link1));
        ((LmnSymbolAtomRef)rc->wt(link2))
            ->set_attr(LMN_ATTR_GET_VALUE(attr2), attr1);
      }
    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) { /* 2 is data */
      ((LmnSymbolAtomRef)rc->wt(link1))
          ->set_link(LMN_ATTR_GET_VALUE(attr1), (LmnAtomRef)rc->wt(link2));
      ((LmnSymbolAtomRef)rc->wt(link1))
          ->set_attr(LMN_ATTR_GET_VALUE(attr1), attr2);
    } else { /* 1, 2 are symbol atom */

      if (LMN_ATTR_IS_EX(attr1)) {
        if (LMN_ATTR_IS_EX(attr2)) { /* 1, 2 are ex */
          lmn_newlink_with_ex((LmnMembraneRef)rc->wt(mem),
                              (LmnSymbolAtomRef)rc->wt(link1), attr1,
                              0, // ex atom ??? unary atom
                              (LmnSymbolAtomRef)rc->wt(link2), attr2, 0);
        } else { /* 1 is ex */
          lmn_newlink_with_ex(
              (LmnMembraneRef)rc->wt(mem), (LmnSymbolAtomRef)rc->wt(link1),
              attr1, 0, (LmnSymbolAtomRef)rc->wt(link2), attr2, attr2);
        }
      } else if (LMN_ATTR_IS_EX(attr2)) { /* 2 is ex */
        lmn_newlink_with_ex((LmnMembraneRef)rc->wt(mem),
                            (LmnSymbolAtomRef)rc->wt(link1), attr1, attr1,
                            (LmnSymbolAtomRef)rc->wt(link2), attr2, 0);
      } else {
        ((LmnSymbolAtomRef)rc->wt(link1))
            ->set_link(LMN_ATTR_GET_VALUE(attr1), (LmnAtomRef)rc->wt(link2));
        ((LmnSymbolAtomRef)rc->wt(link2))
            ->set_link(LMN_ATTR_GET_VALUE(attr2), (LmnAtomRef)rc->wt(link1));
        ((LmnSymbolAtomRef)rc->wt(link1))
            ->set_attr(LMN_ATTR_GET_VALUE(attr1), attr2);
        ((LmnSymbolAtomRef)rc->wt(link2))
            ->set_attr(LMN_ATTR_GET_VALUE(attr2), attr1);
      }
    }
    break;
  }
  case INSTR_NEWLINK: {
    LmnInstrVar atom1, atom2, pos1, pos2, memi;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, pos1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, pos2);
    READ_VAL(LmnInstrVar, instr, memi);

    lmn_mem_newlink((LmnMembraneRef)rc->wt(memi), (LmnAtomRef)rc->wt(atom1),
                    rc->at(atom1), pos1, (LmnAtomRef)rc->wt(atom2),
                    rc->at(atom2), pos2);
    break;
  }
  case INSTR_RELINK: {
    LmnInstrVar atom1, atom2, pos1, pos2, memi;
    LmnSymbolAtomRef ap;
    LmnByte attr;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, pos1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, pos2);
    READ_VAL(LmnInstrVar, instr, memi);

    ap = (LmnSymbolAtomRef)((LmnSymbolAtomRef)rc->wt(atom2))->get_link(pos2);
    attr = ((LmnSymbolAtomRef)rc->wt(atom2))->get_attr(pos2);

    if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1)) &&
        LMN_ATTR_IS_DATA_WITHOUT_EX(attr)) {
      /* hlink???????????????????????????????????????????????????????????? */
#ifdef DEBUG
      fprintf(stderr, "Two data atoms are connected each other.\n");
#endif
    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1))) {
      /* hlink?????????????????????????????????????????????atom1????????????????????????atom2?????????.
       */
      ap->set_link(attr, (LmnAtomRef)rc->wt(atom1));
      ap->set_attr(attr, rc->at(atom1));
    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr)) {
      /* hlink?????????????????????????????????????????????atom2????????????????????????atom1?????????
       */
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, ap);
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, attr);
    } else if (!LMN_ATTR_IS_EX(rc->at(atom1)) && !LMN_ATTR_IS_EX(attr)) {
      /* ???????????????????????????????????? */
      ap->set_link(attr, (LmnAtomRef)rc->wt(atom1));
      ap->set_attr(attr, pos1);
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, ap);
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, attr);
    } else if (LMN_ATTR_IS_EX(rc->at(atom1))) {
      lmn_newlink_with_ex((LmnMembraneRef)rc->wt(memi),
                          (LmnSymbolAtomRef)rc->wt(atom1), rc->at(atom1), pos1,
                          ap,
                          // 0,
                          attr, /* this arg should be attr because
                                   atom2 may be a hyperlink. */
                          attr);
    } else {
      lmn_newlink_with_ex((LmnMembraneRef)rc->wt(memi),
                          (LmnSymbolAtomRef)rc->wt(atom1), rc->at(atom1), pos1,
                          ap, attr, 0);
    }
    break;
  }
  case INSTR_SWAPLINK: {
    LmnInstrVar atom1, atom2, pos1, pos2;
    LmnSymbolAtomRef ap1, ap2;
    LmnByte attr1, attr2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, pos1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, pos2);
    if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1)) &&
        LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom2))) {
      //(D,D)
#ifdef DEBUG
      fprintf(stderr, "Two data atoms are specified in the arg of the "
                      "swaplink instruction.\n");
#endif
    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1))) {
      //(D,S)
      ap2 = (LmnSymbolAtomRef)((LmnSymbolAtomRef)rc->wt(atom2))->get_link(pos2);
      attr2 = ((LmnSymbolAtomRef)rc->wt(atom2))->get_attr(pos2);
      ap2->set_link(attr2, (LmnAtomRef)rc->wt(atom1));
      ap2->set_attr(attr2, rc->at(atom1));
      break;
    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom2))) {
      //(S,D)
      ap1 = (LmnSymbolAtomRef)((LmnSymbolAtomRef)rc->wt(atom1))->get_link(pos1);
      attr1 = ((LmnSymbolAtomRef)rc->wt(atom1))->get_attr(pos1);
      ap1->set_link(attr1, (LmnAtomRef)rc->wt(atom2));
      ap1->set_attr(attr1, rc->at(atom2));
      break;
    }
    //(S,S)
    ap1 = (LmnSymbolAtomRef)((LmnSymbolAtomRef)rc->wt(atom1))->get_link(pos1);
    ap2 = (LmnSymbolAtomRef)((LmnSymbolAtomRef)rc->wt(atom2))->get_link(pos2);
    attr1 = ((LmnSymbolAtomRef)rc->wt(atom1))->get_attr(pos1);
    attr2 = ((LmnSymbolAtomRef)rc->wt(atom2))->get_attr(pos2);

    if ((LmnSymbolAtomRef)rc->wt(atom1) == ap2 &&
        (LmnSymbolAtomRef)rc->wt(atom2) == ap1 && attr1 == pos2 &&
        attr2 == pos1) {
      // use same link

    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr1) &&
               LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) {
      //(-D,-D)

      /* ??????????????????ap2????????????????????????atom1 */
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, ap2);
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, attr2);

      /* ??????????????????ap1????????????????????????atom2 */
      ((LmnSymbolAtomRef)rc->wt(atom2))->set_link(pos2, ap1);
      ((LmnSymbolAtomRef)rc->wt(atom2))->set_attr(pos2, attr1);

    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr1)) {
      //(-D,-S)

      /* ??????????????????ap1????????????????????????atom2 */
      ((LmnSymbolAtomRef)rc->wt(atom2))->set_link(pos2, ap1);
      ((LmnSymbolAtomRef)rc->wt(atom2))->set_attr(pos2, attr1);

      /* ?????????????????????atom1????????????????????????ap2 */
      if (ap2 != NULL) {
        ap2->set_link(attr2, (LmnAtomRef)rc->wt(atom1));
        ap2->set_attr(attr2, pos1);
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, ap2);
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, attr2);
      } else {
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, 0);
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, 0);
      }

    } else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) {
      //(-S,-D)

      /* ??????????????????ap2????????????????????????atom1 */
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, ap2);
      ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, attr2);

      /* ?????????????????????atom2????????????????????????ap1 */
      if (ap1 != NULL) {
        ((LmnSymbolAtomRef)rc->wt(atom2))->set_link(pos2, ap1);
        ((LmnSymbolAtomRef)rc->wt(atom2))
            ->set_attr(pos2, LMN_ATTR_GET_VALUE(attr1));
        ap1->set_link(attr1, (LmnAtomRef)rc->wt(atom2));
        ap1->set_attr(attr1, pos2);
      } else {
        ((LmnSymbolAtomRef)rc->wt(atom2))->set_link(pos2, 0);
        ((LmnSymbolAtomRef)rc->wt(atom2))->set_attr(pos2, 0);
      }

    } else {
      //(-S,-S)

      /* ?????????????????????atom2????????????????????????ap1 */
      if (ap1 != NULL) {
        ((LmnSymbolAtomRef)rc->wt(atom2))->set_link(pos2, ap1);
        ((LmnSymbolAtomRef)rc->wt(atom2))
            ->set_attr(pos2, LMN_ATTR_GET_VALUE(attr1));
        ap1->set_link(attr1, (LmnAtomRef)rc->wt(atom2));
        ap1->set_attr(attr1, pos2);
      } else {
        ((LmnSymbolAtomRef)rc->wt(atom2))->set_link(pos2, 0);
        ((LmnSymbolAtomRef)rc->wt(atom2))->set_attr(pos2, 0);
      }

      /* ?????????????????????atom1????????????????????????ap2 */
      if (ap2 != NULL) {
        ap2->set_link(attr2, (LmnAtomRef)rc->wt(atom1));
        ap2->set_attr(attr2, pos1);
        ((LmnSymbolAtomRef)(LmnSymbolAtomRef)rc->wt(atom1))
            ->set_link(pos1, ap2);
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, attr2);
      } else {
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_link(pos1, 0);
        ((LmnSymbolAtomRef)rc->wt(atom1))->set_attr(pos1, 0);
      }
    }

    break;
  }

    /*     case INSTR_SWAPLINK: */
    /*     { */
    /*       LmnInstrVar atom1, atom2, pos1, pos2; */
    /*       LmnSAtom ap1,ap2; */
    /*       LmnLinkAttr attr1, attr2; */
    /*       READ_VAL(LmnInstrVar, instr, atom1); */
    /*       READ_VAL(LmnInstrVar, instr, pos1); */
    /*       READ_VAL(LmnInstrVar, instr, atom2); */
    /*       READ_VAL(LmnInstrVar, instr, pos2); */
    /*       ap1 = LMN_SATOM(wt(rc, atom1)->get_link(pos1)); */
    /*       ap2 = LMN_SATOM(wt(rc, atom2)->get_link(pos2)); */
    /*       attr1 = (wt(rc, atom1))->get_attr(pos1); */
    /*       attr2 = (wt(rc, atom2))->get_attr(pos2); */
    /*       if ((LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1)) &&
     * LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) */
    /*           || (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom2)) &&
     * LMN_ATTR_IS_DATA_WITHOUT_EX(attr1))) { */
    /*         /\* atom1???ap2??????????????????????????? or
     * atom2???ap1??????????????????????????? *\/ */
    /* #ifdef DEBUG */
    /*         fprintf(stderr, "Two data atoms are connected each other.\n");
     */
    /* #endif */
    /*       }else if(LMN_SATOM(wt(rc,atom1)) == ap2 &&
     * LMN_SATOM(wt(rc,atom2)) == ap1 && attr1 == pos2 && attr2 ==pos1){ */
    /*       }else if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom2))){ */
    /*         /\* ??????????????????atom2????????????????????????ap1 *\/ */
    /*      if(ap1 != NULL){ */
    /*        ap1->set_link(attr1, wt(rc, atom2)); */
    /*        ap1->set_attr(attr1, pos2); */
    /*      } */
    /*         if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1))){ */
    /*           /\* ??????????????????atom1????????????????????????ap2 *\/ */
    /*        if(ap2 != NULL){ */
    /*          ap2->set_link(attr2, wt(rc, atom1)); */
    /*          ap2->set_attr(attr2, pos1); */
    /*        } */
    /*         }else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)){ */
    /*           /\* ??????????????????ap2????????????????????????atom1 *\/ */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, attr2); */
    /*         }else { */
    /*           /\* ?????????????????????atom1????????????????????????ap2 *\/ */
    /*      ////// */
    /*              if(ap2 != NULL){ */
    /*                      ap2->set_link(attr2, wt(rc, atom1)); */
    /*                      ap2->set_attr(attr2, pos1); */
    /*                      (LMN_SATOM(wt(rc, atom1)))->set_link(pos1,
     * ap2); */
    /*                      (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * attr2); */
    /*              }else{ */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, 0);
     */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, 0);
     */
    /*              } */
    /*      ////// */
    /*           /\*ap2->set_link(attr2, wt(rc, atom1)); */
    /*           ap2->set_attr(attr2, rc->at(atom1)); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, attr2);*\/
     */
    /*         } */
    /*       } */
    /*       else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr1)){ */
    /*         /\* ??????????????????ap1????????????????????????atom2 *\/ */
    /*         (LMN_SATOM(wt(rc, atom2)))->set_link(pos2, ap1); */
    /*         (LMN_SATOM(wt(rc, atom2)))->set_attr(pos2, attr1); */
    /*         if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1))){ */
    /*           /\* ??????????????????atom1????????????????????????ap2 *\/ */
    /*           ap2->set_link(attr2, wt(rc, atom1)); */
    /*           ap2->set_attr(attr2, pos1); */
    /*         }else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)){ */
    /*           /\* ??????????????????ap2????????????????????????atom1 *\/ */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, attr2); */
    /*         }else if (!LMN_ATTR_IS_EX(rc->at(atom1)) &&
     * !LMN_ATTR_IS_EX(attr2)){ */
    /*           /\* ?????????????????????atom1????????????????????????ap2 *\/ */
    /*      ////// */
    /*              if(ap2 != NULL){ */
    /*                      ap2->set_link(attr2, wt(rc, atom1)); */
    /*                      ap2->set_attr(attr2, pos1); */
    /*                      (LMN_SATOM(wt(rc, atom1)))->set_link(pos1,
     * ap2); */
    /*                      (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * attr2); */
    /*              }else{ */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, 0);
     */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, 0);
     */
    /*              } */
    /*      ////// */
    /*           /\*ap2->set_link(attr2, wt(rc, atom1)); */
    /*           ap2->set_attr(attr2, rc->at(atom1)); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, attr2);*\/
     */
    /*         } */
    /*       } */
    /*       else if (!LMN_ATTR_IS_EX(rc->at(atom1)) && !LMN_ATTR_IS_EX(at(rc,
     * atom2)) */
    /*                && !LMN_ATTR_IS_EX(attr1) && !LMN_ATTR_IS_EX(attr2)){ */
    /*         /\* ?????????????????????atom2????????????????????????ap1 *\/ */

    /*         if(ap1 != NULL){ */
    /*           (LMN_SATOM(wt(rc, atom2)))->set_link(pos2, ap1); */
    /*           (LMN_SATOM(wt(rc, atom2)))->set_attr(pos2,
     * LMN_ATTR_GET_VALUE(attr1)); */
    /*           ap1->set_link(attr1, wt(rc, atom2)); */
    /*           ap1->set_attr(attr1, pos2); */
    /*         }else{ */
    /*           (LMN_SATOM(wt(rc, atom2)))->set_link(pos2, 0); */
    /*           (LMN_SATOM(wt(rc, atom2)))->set_attr(pos2, 0); */
    /*         } */
    /*      if (LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1))){ */
    /*           /\* ??????????????????atom1????????????????????????ap2 *\/ */
    /*           ap2->set_link(attr2, wt(rc, atom1)); */
    /*           ap2->set_attr(attr2, pos1); */
    /*         }else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)){ */
    /*           /\* ??????????????????ap2????????????????????????atom1 *\/ */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * LMN_ATTR_GET_VALUE(attr2)); */
    /*         }else { */
    /*           /\* ?????????????????????atom1????????????????????????ap2 *\/ */
    /*      ////// */
    /*              if(ap2 != NULL){ */
    /*                      ap2->set_link(LMN_ATTR_GET_VALUE(attr2),
     * wt(rc, atom1)); */
    /*                      ap2->set_attr(LMN_ATTR_GET_VALUE(attr2),
     * pos1); */
    /*                      (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * ap2); */
    /*                      (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * LMN_ATTR_GET_VALUE(attr2)); */
    /*              }else{ */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, 0);
     */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, 0);
     */
    /*              } */
    /*      ////// */
    /*      /\* */
    /*         if(ap2){ */
    /*              if(LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atom1))){ */
    /*                // ??????????????????atom1????????????????????????ap2  */
    /*                ap2->set_link(attr2, wt(rc, atom1)); */
    /*                ap2->set_attr(attr2, rc->at(atom1)); */
    /*              }else if(LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)){ */
    /*                // ??????????????????ap2????????????????????????atom1  */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2);
     */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * attr2); */
    /*              }else{ */
    /*                // ?????????????????????atom1????????????????????????ap2  */
    /*                ap2->set_link(attr2, wt(rc, atom1)); */
    /*                ap2->set_attr(attr2, pos1); */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, ap2);
     */
    /*                (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1,
     * attr2); */
    /*              } */
    /*         }else{ */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_link(pos1, 0); */
    /*           (LMN_SATOM(wt(rc, atom1)))->set_attr(pos1, 0); */
    /*         }*\/ */
    /*       } */
    /*       } */
    /*       break; */
    /*     } */
  case INSTR_INHERITLINK: {
    LmnInstrVar atomi, posi, linki;
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, posi);
    READ_VAL(LmnInstrVar, instr, linki);
    SKIP_VAL(LmnInstrVar, instr);

    if (LMN_ATTR_IS_DATA(rc->at(atomi)) &&
        LMN_ATTR_IS_DATA(rc->at(linki))) {
#ifdef DEBUG
      fprintf(stderr, "Two data atoms are connected each other.\n");
#endif
    } else if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
      ((LmnSymbolAtomRef)rc->wt(linki))
          ->set_link(rc->at(linki), (LmnAtomRef)rc->wt(atomi));
      ((LmnSymbolAtomRef)rc->wt(linki))
          ->set_attr(rc->at(linki), rc->at(atomi));
    } else if (LMN_ATTR_IS_DATA(rc->at(linki))) {
      ((LmnSymbolAtomRef)rc->wt(atomi))->set_link(posi, (LmnAtomRef)rc->wt(linki));
      ((LmnSymbolAtomRef)rc->wt(atomi))->set_attr(posi, rc->at(linki));
    } else {
      ((LmnSymbolAtomRef)rc->wt(atomi))->set_link(posi, (LmnAtomRef)rc->wt(linki));
      ((LmnSymbolAtomRef)rc->wt(atomi))->set_attr(posi, rc->at(linki));
      ((LmnSymbolAtomRef)rc->wt(linki))
          ->set_link(rc->at(linki), (LmnAtomRef)rc->wt(atomi));
      ((LmnSymbolAtomRef)rc->wt(linki))
          ->set_attr(rc->at(linki), posi);
    }

    break;
  }
  case INSTR_GETLINK: {
    LmnInstrVar linki, atomi, posi;
    READ_VAL(LmnInstrVar, instr, linki);
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, posi);

    /* ??????????????????????????????????????????????????????????????????????????????
     * ?????????????????????????????????????????????????????????????????????????????????????????? */

    rc->reg(linki) = {
        (LmnWord)((LmnSymbolAtomRef)rc->wt(atomi))->get_link(posi),
        ((LmnSymbolAtomRef)rc->wt(atomi))->get_attr(posi), TT_ATOM};

    break;
  }
  case INSTR_HYPERGETLINK:
    // head????????????
    // hyperlink?????????????????????????????????????????????????????????????????????????????????????????????
    {
      LmnInstrVar linki, atomi, posi;
      READ_VAL(LmnInstrVar, instr, linki);
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, posi);

      LmnAtomRef hlAtom = ((LmnSymbolAtomRef)rc->wt(atomi))->get_link(posi);
      LmnLinkAttr attr = ((LmnSymbolAtomRef)rc->wt(atomi))->get_attr(posi);
      if (attr != LMN_HL_ATTR) {
        return FALSE;
      } else {
        HyperLink *hl = lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)hlAtom);
        auto v = lmn_hyperlink_get_elements(hl);
        auto regs = std::vector<LmnRegister>();
        std::transform(
            v.begin(), v.end(), std::back_inserter(regs),
            [](HyperLink *h) -> LmnRegister {
              auto child_hlAtom = h->atom;
              auto linked_atom = child_hlAtom->get_link(0);
              return {(LmnWord)linked_atom, child_hlAtom->get_attr(0), TT_ATOM};
            });

        this->false_driven_enumerate(linki, std::move(regs));
        return false;
      }
      break;
    }
  case INSTR_UNIFY: {
    LmnInstrVar atom1, pos1, atom2, pos2, memi;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, pos1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, pos2);
    READ_VAL(LmnInstrVar, instr, memi);

    lmn_mem_unify_atom_args((LmnMembraneRef)rc->wt(memi),
                            (LmnSymbolAtomRef)rc->wt(atom1), pos1,
                            (LmnSymbolAtomRef)rc->wt(atom2), pos2);
    break;
  }
  case INSTR_PROCEED:
    return TRUE;
  case INSTR_STOP:
    return FALSE;
  case INSTR_NOT: {
    LmnSubInstrSize subinstr_size;
    READ_VAL(LmnSubInstrSize, instr, subinstr_size);
    this->push_stackframe(exec_subinstructions_not(instr + subinstr_size));
    break;
  }
  case INSTR_ENQUEUEATOM: {
    SKIP_VAL(LmnInstrVar, instr);
    /* do nothing */
    break;
  }
  case INSTR_DEQUEUEATOM: {
    SKIP_VAL(LmnInstrVar, instr);
    break;
  }
  case INSTR_TAILATOM: {
    LmnInstrVar atomi, memi;
    LmnMembraneRef mem = (LmnMembraneRef)rc->wt(memi);
    LmnSymbolAtomRef sa = (LmnSymbolAtomRef)rc->wt(atomi);
    LmnFunctor f = sa->get_functor();
    AtomListEntry *ent = mem->get_atomlist(f);
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    ent->move_atom_to_atomlist_tail(sa);
    break;
  }

  case INSTR_HEADATOM: {
    LmnInstrVar atomi, memi;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    move_atom_to_atomlist_head((LmnSymbolAtomRef)rc->wt(atomi),
                               (LmnMembraneRef)rc->wt(memi));
    break;
  }
  case INSTR_TAILATOMLIST: {
    LmnInstrVar atomi, memi;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);
    move_atomlist_to_atomlist_tail((LmnSymbolAtomRef)rc->wt(atomi),
                                   (LmnMembraneRef)rc->wt(memi));
    break;
  }
  case INSTR_ATOMTAILATOM: {
    LmnInstrVar atomi, atomi2, memi;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, atomi2);
    READ_VAL(LmnInstrVar, instr, memi);
    move_atom_to_atom_tail((LmnSymbolAtomRef)rc->wt(atomi),
                           (LmnSymbolAtomRef)rc->wt(atomi2),
                           (LmnMembraneRef)rc->wt(memi));
    break;
  }
  case INSTR_CLEARLINK: {
    LmnInstrVar atomi, link;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, link);

    if (!LMN_ATTR_IS_DATA_WITHOUT_EX(rc->at(atomi))) {
      ((LmnSymbolAtomRef)rc->wt(atomi))->set_link(link, NULL);
    }

    break;
  }
  case INSTR_NEWMEM: {
    LmnInstrVar newmemi, parentmemi;
    LmnMembraneRef mp;

    READ_VAL(LmnInstrVar, instr, newmemi);
    READ_VAL(LmnInstrVar, instr, parentmemi);
    SKIP_VAL(LmnInstrVar, instr);

    mp = new LmnMembrane(); /*lmn_new_mem(memf);*/
    ((LmnMembraneRef)rc->wt(parentmemi))->add_child_mem(mp);
    rc->wt(newmemi) = (LmnWord)mp;
    rc->tt(newmemi) = TT_MEM;
    mp->set_active(TRUE);
    if (rc->has_mode(REACT_MEM_ORIENTED)) {
      ((MemReactContext *)rc)->memstack_push(mp);
    }
    break;
  }
  case INSTR_ALLOCMEM: {
    LmnInstrVar dstmemi;
    READ_VAL(LmnInstrVar, instr, dstmemi);
    rc->wt(dstmemi) = (LmnWord) new LmnMembrane();
    rc->tt(dstmemi) = TT_OTHER; /* 2014-05-08, ueda */
    break;
  }
  case INSTR_REMOVEATOM: {
    LmnInstrVar atomi, memi;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, memi);

#ifdef USE_FIRSTCLASS_RULE
    LmnSymbolAtomRef atom = (LmnSymbolAtomRef)rc->wt(atomi);
    LmnLinkAttr attr = rc->at(atomi);
    if (LMN_HAS_FUNCTOR(atom, attr, LMN_COLON_MINUS_FUNCTOR)) {
      LmnMembraneRef mem = (LmnMembraneRef)rc->wt(memi);
      lmn_mem_remove_firstclass_ruleset(mem, firstclass_ruleset_lookup(atom));
      firstclass_ruleset_release(atom);
    }
#endif
    lmn_mem_remove_atom((LmnMembraneRef)rc->wt(memi), (LmnAtomRef)rc->wt(atomi),
                        rc->at(atomi));

    break;
  }
  case INSTR_FREEATOM: {
    LmnInstrVar atomi;

    READ_VAL(LmnInstrVar, instr, atomi);
    lmn_free_atom((LmnAtomRef)rc->wt(atomi), rc->at(atomi));
    break;
  }
  case INSTR_REMOVEMEM: {
    LmnInstrVar memi, parenti;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, parenti);

    ((LmnMembraneRef)rc->wt(parenti))->remove_mem((LmnMembraneRef)rc->wt(memi));
    break;
  }
  case INSTR_FREEMEM: {
    LmnInstrVar memi;
    LmnMembraneRef mp;

    READ_VAL(LmnInstrVar, instr, memi);

    mp = (LmnMembraneRef)rc->wt(memi);
    delete mp;
    break;
  }
  case INSTR_ADDMEM: {
    LmnInstrVar dstmem, srcmem;

    READ_VAL(LmnInstrVar, instr, dstmem);
    READ_VAL(LmnInstrVar, instr, srcmem);

    //      LMN_ASSERT(!((LmnMembraneRef)rc->wt( srcmem))->parent);

    ((LmnMembraneRef)rc->wt(dstmem))
        ->add_child_mem((LmnMembraneRef)rc->wt(srcmem));
    break;
  }
  case INSTR_ENQUEUEMEM: {
    LmnInstrVar memi;
    READ_VAL(LmnInstrVar, instr, memi);

    if (rc->has_mode(REACT_MEM_ORIENTED)) {
      ((MemReactContext *)rc)->memstack_push((LmnMembraneRef)rc->wt(memi));
    }
    break;
  }
  case INSTR_UNLOCKMEM: { /* do nothing */
    SKIP_VAL(LmnInstrVar, instr);
    break;
  }
  case INSTR_LOADRULESET: {
    LmnInstrVar memi;
    LmnRulesetId id;
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnRulesetId, instr, id);

    lmn_mem_add_ruleset((LmnMembraneRef)rc->wt(memi), LmnRuleSetTable::at(id));
    break;
  }
  case INSTR_LOADMODULE: {
    LmnInstrVar memi;
    lmn_interned_str module_name_id;
    LmnRuleSetRef ruleset;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(lmn_interned_str, instr, module_name_id);

    if ((ruleset = lmn_get_module_ruleset(module_name_id))) {
      /* ??????????????????????????????????????????????????? */
      lmn_mem_add_ruleset((LmnMembraneRef)rc->wt(memi), ruleset);
    } else {
      /* ??????????????????????????????????????????????????? */
      fprintf(stderr, "Undefined module %s\n", lmn_id_to_name(module_name_id));
    }
    break;
  }
  case INSTR_RECURSIVELOCK: {
    SKIP_VAL(LmnInstrVar, instr);
    /* do notiong */
    break;
  }
  case INSTR_RECURSIVEUNLOCK: {
    SKIP_VAL(LmnInstrVar, instr);
    /* do notiong */
    break;
  }
  case INSTR_DEREFATOM: {
    LmnInstrVar atom1, atom2, posi;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, posi);

    rc->reg(atom1) = {
        (LmnWord)LMN_SATOM(((LmnSymbolAtomRef)rc->wt(atom2))->get_link(posi)),
        ((LmnSymbolAtomRef)rc->wt(atom2))->get_attr(posi), TT_ATOM};
    break;
  }
  case INSTR_DEREF: {
    LmnInstrVar atom1, atom2, pos1, pos2;
    LmnByte attr;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, pos1);
    READ_VAL(LmnInstrVar, instr, pos2);

    attr = ((LmnSymbolAtomRef)rc->wt(atom2))->get_attr(pos1);
    LMN_ASSERT(!LMN_ATTR_IS_DATA(rc->at(atom2)));
    if (LMN_ATTR_IS_DATA(attr)) {
      if (pos2 != 0)
        return FALSE;
    } else {
      if (attr != pos2)
        return FALSE;
    }
    rc->reg(atom1) = {
        (LmnWord)((LmnSymbolAtomRef)rc->wt(atom2))->get_link(pos1), attr,
        TT_ATOM};
    break;
  }
  case INSTR_FUNC: {
    LmnInstrVar atomi;
    LmnFunctor f;
    LmnLinkAttr attr;
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnLinkAttr, instr, attr);

    if (LMN_ATTR_IS_DATA(rc->at(atomi)) == LMN_ATTR_IS_DATA(attr)) {
      if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
        BOOL eq;
        if (rc->at(atomi) != attr)
          return FALSE; /* comp attr */
        LmnByte type;
        READ_CMP_DATA_ATOM(attr, rc->wt(atomi), eq, type);
        rc->tt(atomi) = type;
        if (!eq)
          return FALSE;
      } else { /* symbol atom */
        READ_VAL(LmnFunctor, instr, f);
        if (((LmnSymbolAtomRef)rc->wt(atomi))->get_functor() != f) {
          return FALSE;
        }
        if (rc_hlink_opt(atomi, rc)) {
          auto spc = ((SameProcCxt *)hashtbl_get(rc->get_hl_sameproccxt(),
                                                 (HashKeyType)atomi));

          auto atom = (LmnSymbolAtomRef)rc->wt(atomi);
          for (int i = 0; i < spc->proccxts.size(); i++) {
            if (spc->proccxts[i]) {
              if (spc->proccxts[i]->is_argument_of(atom, i)) {
                auto linked_hl =
                    lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)atom->get_link(i));
                spc->proccxts[i]->start = linked_hl;
              } else {
                return false;
              }
            }
          }
        }
      }
    } else { /* LMN_ATTR_IS_DATA(rc->at(atomi)) != LMN_ATTR_IS_DATA(attr) */
      return FALSE;
    }
    break;
  }
  case INSTR_NOTFUNC: {
    LmnInstrVar atomi;
    LmnFunctor f;
    LmnLinkAttr attr;
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnLinkAttr, instr, attr);

    if (LMN_ATTR_IS_DATA(rc->at(atomi)) == LMN_ATTR_IS_DATA(attr)) {
      if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
        if (rc->at(atomi) == attr) {
          BOOL eq;
          LmnByte type;
          READ_CMP_DATA_ATOM(attr, rc->wt(atomi), eq, type);
          rc->tt(atomi) = type;
          if (eq)
            return FALSE;
        } else {
          goto label_skip_data_atom;
        }
      } else { /* symbol atom */
        READ_VAL(LmnFunctor, instr, f);
        if (((LmnSymbolAtomRef)rc->wt(atomi))->get_functor() == f)
          return FALSE;
      }
    } else if (LMN_ATTR_IS_DATA(attr)) {
      goto label_skip_data_atom;
    }
    break;
  label_skip_data_atom:
    SKIP_DATA_ATOM(attr);
    break;
  }
  case INSTR_ISGROUND:
  case INSTR_ISHLGROUND:
  case INSTR_ISHLGROUNDINDIRECT: {
    LmnInstrVar funci, srclisti, avolisti;
    Vector *srcvec, *avovec;
    unsigned long natoms;
    BOOL b;

    READ_VAL(LmnInstrVar, instr, funci);
    READ_VAL(LmnInstrVar, instr, srclisti);
    READ_VAL(LmnInstrVar, instr, avolisti);

    /* ???????????????????????????????????????????????? */
    srcvec = links_from_idxs((Vector *)rc->wt(srclisti), rc);
    avovec = links_from_idxs((Vector *)rc->wt(avolisti), rc);

    std::unique_ptr<ProcessTbl> atoms = nullptr;
    std::unique_ptr<ProcessTbl> hlinks = nullptr;

    switch (op) {
    case INSTR_ISHLGROUND:
    case INSTR_ISHLGROUNDINDIRECT: {
      std::vector<LmnFunctor> attr_functors(16);
      std::vector<LmnWord> attr_dataAtoms(16);
      std::vector<LmnLinkAttr> attr_dataAtom_attrs(16);

      auto args = (op == INSTR_ISHLGROUNDINDIRECT)
                      ? read_unary_atoms_indirect(rc, instr)
                      : read_unary_atoms(rc, instr);

      for (auto &v : args) {
        if (c17::holds_alternative<LmnFunctor>(v)) {
          attr_functors.push_back(c17::get<LmnFunctor>(v));
        } else {
          auto &p = c17::get<std::pair<LmnLinkAttr, LmnDataAtomRef>>(v);
          attr_dataAtom_attrs.push_back(p.first);
          attr_dataAtoms.push_back(p.second);
        }
      }
      std::sort(std::begin(attr_functors), std::end(attr_functors));

      b = ground_atoms(srcvec, avovec, atoms, &natoms, hlinks, attr_functors,
                       attr_dataAtoms, attr_dataAtom_attrs);
      break;
    }
    case INSTR_ISGROUND: {
      b = ground_atoms(srcvec, avovec, atoms, &natoms);
      break;
    }
    }
    free_links(srcvec);
    free_links(avovec);

    if (!b)
      return false;

    rc->reg(funci) = {natoms, LMN_INT_ATTR, TT_OTHER};

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        auto addr = atoms.get();
        atoms.release();
        dpor_LHS_add_ground_atoms(RC_POR_DATA(rc), addr);

        this->push_stackframe([=](interpreter &itr, bool result) {
          dpor_LHS_remove_ground_atoms(RC_POR_DATA(rc), addr);
          delete addr;
          return command_result::Failure;
        });
      }
    }

    break;
  }
  case INSTR_UNIQ: {
    /*
     * uniq ????????????
     * "??????????????????????????????????????????????????????" ??????
     * "???????????????????????????????????????????????????????????????" ???
     * ??????????????????????????????????????????????????????????????????????????????
     */

    LmnInstrVar llist, n;
    LmnPortRef port;
    lmn_interned_str id;
    unsigned int i;
    BOOL sh;
    LmnLinkAttr attr;

    port = (LmnPortRef)lmn_make_output_string_port();
    READ_VAL(LmnInstrVar, instr, llist);

    if (lmn_env.show_hyperlink) {
      sh = TRUE;
      /* MT-UNSAFE!!
       *  --show_hl???????????????????????????lmn_dump_atom?????????????????????
       *  ????????????????????????????????????????????????????????????????????????
       *
       *  TODO:
       *    ???????????????????????????????????????????????????????????????,
       *    ????????????ReactCxt?????????????????????????????????????????????,
       *    ????????????????????????????????? */
      lmn_env.show_hyperlink = FALSE;
    } else {
      sh = FALSE;
    }

    for (i = 0; i < (int)llist; i++) {
      Vector *srcvec;

      READ_VAL(LmnInstrVar, instr, n);
      srcvec = (Vector *)rc->wt(n);
      attr = (LmnLinkAttr)rc->at(srcvec->get(0));

      /** ?????????????????? **/
      /* ??????????????????????????????????????????????????????????????? */
      if (LMN_ATTR_IS_DATA(attr)) {
        switch (attr) {
        case LMN_INT_ATTR: {
          char *s = int_to_str(rc->wt(srcvec->get(0)));
          port_put_raw_s(port, s);
          LMN_FREE(s);
          break;
        }
        case LMN_DBL_ATTR: {
          char buf[64];
          sprintf(buf, "%f", lmn_get_double(rc->wt(srcvec->get(0))));
          port_put_raw_s(port, buf);
          break;
        }
        case LMN_HL_ATTR: {
          char buf[16];
          port_put_raw_s(port, EXCLAMATION_NAME);
          sprintf(buf, "%lx",
                  LMN_HL_ID(LMN_HL_ATOM_ROOT_HL(
                      (LmnSymbolAtomRef)rc->wt(srcvec->get(0)))));
          port_put_raw_s(port, buf);
          break;
        }
        default: /* int, double, hlink ??????????????????????????????????????? */
          lmn_dump_atom(port, (LmnAtomRef)rc->wt(srcvec->get(0)),
                        (LmnLinkAttr)rc->at(srcvec->get(0)));
        }
      } else { /* symbol atom */
        lmn_dump_atom(port, (LmnAtomRef)rc->wt(srcvec->get(0)),
                      (LmnLinkAttr)rc->at(srcvec->get(0)));
      }
      port_put_raw_s(port, ":");
    }

    id = lmn_intern(((LmnStringRef)port->data)->c_str());
    lmn_port_free(port);

    if (sh)
      lmn_env.show_hyperlink = TRUE;

    /* ?????????????????? */
    if (rule->has_history(id))
      return FALSE;

    /* ??????????????? */
    rule->add_history(id);

    break;
  }
  case INSTR_NEWHLINKWITHATTR:
  case INSTR_NEWHLINKWITHATTRINDIRECT:
  case INSTR_NEWHLINK: {
    /* ?????????????????????????????????????????????????????????????????????????????????
     * ???????????????????????????????????????????????????
     */

    LmnInstrVar atomi;
    READ_VAL(LmnInstrVar, instr, atomi);

    switch (op) {
    case INSTR_NEWHLINKWITHATTR: {
      LmnAtomRef ap;
      LmnLinkAttr attr;
      READ_VAL(LmnLinkAttr, instr, attr);
      if (LMN_ATTR_IS_DATA(attr)) {
        READ_DATA_ATOM(ap, attr);
      } else {
        LmnFunctor f;
        READ_VAL(LmnFunctor, instr, f);
        ap = lmn_new_atom(f);
        attr = 0; //????????????????????????????????????????????????????????????????????????
        if (((LmnSymbolAtomRef)ap)->get_arity() > 1) {
          lmn_fatal("hyperlink's attribute takes only an unary atom");
        }
      }

      rc->reg(atomi) = {
          (LmnWord)lmn_hyperlink_new_with_attr((LmnSymbolAtomRef)ap, attr),
          LMN_HL_ATTR, TT_ATOM};
      break;
    }
    case INSTR_NEWHLINKWITHATTRINDIRECT: {
      LmnAtomRef ap;
      LmnLinkAttr attr;
      LmnInstrVar atomi2; //?????????????????????????????????
      READ_VAL(LmnInstrVar, instr, atomi2);
      ap = lmn_copy_atom((LmnAtomRef)rc->wt(atomi2), rc->at(atomi2));
      attr = rc->at(atomi2);
      if (!LMN_ATTR_IS_DATA(rc->at(atomi2)) &&
          ((LmnSymbolAtomRef)ap)->get_arity() > 1) {
        lmn_fatal("hyperlink's attribute takes only an unary atom");
      }
      rc->reg(atomi) = {(LmnWord)lmn_hyperlink_new_with_attr(ap, attr),
                        LMN_HL_ATTR, TT_ATOM};
      break;
    }
    case INSTR_NEWHLINK:
      rc->reg(atomi) = {(LmnWord)lmn_hyperlink_new(), LMN_HL_ATTR, TT_ATOM};
      break;
    }
    break;
  }
  case INSTR_MAKEHLINK: {
    /* // ?????????
     *
     * i(N) :- make(N, $x), N1 = N-1 | i(N1), hoge($x).
     * ?????????????????????int(N)?????????ID?????????hyperlink?????????????????????????????????????????????
     * ????????????????????????????????????
     * (??????(?????????)????????????????????????????????????hyperlink??????????????????????????????????????????????????????)
     */
    break;
  }
  case INSTR_ISHLINK: {
    LmnInstrVar atomi;
    READ_VAL(LmnInstrVar, instr, atomi);

    if (!LMN_ATTR_IS_HL(rc->at(atomi)))
      return FALSE;

    break;
  }
  case INSTR_GETATTRATOM: {
    LmnInstrVar dstatomi, atomi;
    READ_VAL(LmnInstrVar, instr, dstatomi);
    READ_VAL(LmnInstrVar, instr, atomi);

    rc->reg(dstatomi) = {(LmnWord)LMN_HL_ATTRATOM(lmn_hyperlink_at_to_hl(
                             (LmnSymbolAtomRef)rc->wt(atomi))),
                         LMN_HL_ATTRATOM_ATTR(lmn_hyperlink_at_to_hl(
                             (LmnSymbolAtomRef)rc->wt(atomi))),
                         TT_OTHER};
    break;
  }
  case INSTR_GETNUM: {
    LmnInstrVar dstatomi, atomi;

    /* ISHLINK?????????????????? */
    READ_VAL(LmnInstrVar, instr, dstatomi);
    READ_VAL(LmnInstrVar, instr, atomi);

    rc->reg(dstatomi) = {
        (LmnWord)(lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)rc->wt(atomi)))
            ->element_num(),
        LMN_INT_ATTR, TT_OTHER};
    break;
  }
  case INSTR_UNIFYHLINKS: {
    LmnSymbolAtomRef atom;
    LmnInstrVar memi, atomi;
    LmnLinkAttr attr1, attr2;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, atomi);

    atom = (LmnSymbolAtomRef)rc->wt(atomi);

    attr1 = atom->get_attr(0);
    attr2 = atom->get_attr(1);

    /* >< ??????????????????????????????????????????????????????????????? */
    if (LMN_ATTR_IS_HL(attr1) && LMN_ATTR_IS_HL(attr2)) {
      LmnMembraneRef m;
      LmnSymbolAtomRef atom1, atom2;
      HyperLink *hl1, *hl2;

      m = (LmnMembraneRef)rc->wt(memi);
      atom1 = (LmnSymbolAtomRef)atom->get_link(0);
      atom2 = (LmnSymbolAtomRef)atom->get_link(1);

      hl1 = lmn_hyperlink_at_to_hl(atom1);
      hl2 = lmn_hyperlink_at_to_hl(atom2);

      if (atom->get_arity() ==
          2) { //??????????????????????????????????????????????????????????????????????????????
        hl1->lmn_unify(hl2, LMN_HL_ATTRATOM(hl1), LMN_HL_ATTRATOM_ATTR(hl1));
      } else if (atom->get_arity() ==
                 3) { //???????????????????????????????????????????????????????????????
        LmnAtom attrAtom;
        attrAtom = LMN_ATOM(atom->get_link(2));
        hl1->lmn_unify(hl2, (LmnAtomRef)attrAtom, atom->get_attr(2));
      } else {
        lmn_fatal("too many arguments to >< atom");
      }

      lmn_mem_delete_atom(m, (LmnAtomRef)rc->wt(atomi), rc->at(atomi));
      lmn_mem_delete_atom(m, atom1, (LmnWord)attr1);
      lmn_mem_delete_atom(m, atom2, (LmnWord)attr2);
    }
    break;
  }
  case INSTR_FINDPROCCXT: {
    /**
     * ????????????????????????????????????????????????????????????????????????????????????????????????????????????
     * hyperlink??????(2010/10/10??????)
     *
     * Java????????????????????????--hl-opt?????????????????????????????????????????????findatom????????????????????????
     * SLIM??????--hl?????????????????????????????????????????????????????????????????????????????????warning???
     *
     * cf. ????????????????????????????????????????????????
     *     a($x), b($x) :- ...
     *   ??? a($x), a($x0) :- hlink($x), $x = $x0 | ...
     *   ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
     *   ????????????????????????????????????????????????????????????????????????????????????????????????
     *   ??????????????????????????????????????????$x?????????????????????????????????????????????????????????????????????$x0????????????????????????????????????
     */
    LmnInstrVar atom1, length1, arg1, atom2, length2, arg2;
    SameProcCxt *spc1, *spc2;
    int i;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, length1);
    READ_VAL(LmnInstrVar, instr, arg1);
    READ_VAL(LmnInstrVar, instr, atom2);
    READ_VAL(LmnInstrVar, instr, length2);
    READ_VAL(LmnInstrVar, instr, arg2);

    if (!rc->get_hl_sameproccxt()) {
      rc->prepare_hl_spc();
    }

    if (!hashtbl_contains(rc->get_hl_sameproccxt(), (HashKeyType)atom1)) {
      spc1 = new SameProcCxt(length1);
      hashtbl_put(rc->get_hl_sameproccxt(), (HashKeyType)atom1, (HashValueType)spc1);
    } else {
      spc1 = (SameProcCxt *)hashtbl_get(rc->get_hl_sameproccxt(), (HashKeyType)atom1);
    }

    if (!hashtbl_contains(rc->get_hl_sameproccxt(), (HashKeyType)atom2)) {
      spc2 = new SameProcCxt(length2);
      hashtbl_put(rc->get_hl_sameproccxt(), (HashKeyType)atom2, (HashValueType)spc2);
    } else {
      spc2 = (SameProcCxt *)hashtbl_get(rc->get_hl_sameproccxt(), (HashKeyType)atom2);
    }

    spc1->add_proccxt_if_absent(atom1, arg1);
    spc2->add_proccxt_if_absent(atom2, arg2, *spc1, arg1);

    ////normal parallel init
    if (lmn_env.enable_parallel && !lmn_env.nd) {
      for (i = 0; i < lmn_env.core_num; i++) {
        if (!thread_info[i]->rc->get_hl_sameproccxt()) {
          thread_info[i]->rc->prepare_hl_spc();
        }

        if (!hashtbl_contains(thread_info[i]->rc->get_hl_sameproccxt(),
                              (HashKeyType)atom1)) {
          spc1 = new SameProcCxt(length1);
          hashtbl_put(thread_info[i]->rc->get_hl_sameproccxt(), (HashKeyType)atom1,
                      (HashValueType)spc1);
        } else {
          spc1 = (SameProcCxt *)hashtbl_get(thread_info[i]->rc->get_hl_sameproccxt(),
                                            (HashKeyType)atom1);
        }

        if (!hashtbl_contains(thread_info[i]->rc->get_hl_sameproccxt(),
                              (HashKeyType)atom2)) {
          spc2 = new SameProcCxt(length2);
          hashtbl_put(thread_info[i]->rc->get_hl_sameproccxt(), (HashKeyType)atom2,
                      (HashValueType)spc2);
        } else {
          spc2 = (SameProcCxt *)hashtbl_get(thread_info[i]->rc->get_hl_sameproccxt(),
                                            (HashKeyType)atom2);
        }

        spc1->add_proccxt_if_absent(atom1, arg1);
        spc2->add_proccxt_if_absent(atom2, arg2, *spc1, arg1);
      }
    }
    break;
  }
  case INSTR_EQGROUND:
  case INSTR_NEQGROUND: {
    LmnInstrVar srci, dsti;
    Vector *srcvec, *dstvec;
    BOOL ret_flag;

    READ_VAL(LmnInstrVar, instr, srci);
    READ_VAL(LmnInstrVar, instr, dsti);

    srcvec = links_from_idxs((Vector *)rc->wt(srci), rc);
    dstvec = links_from_idxs((Vector *)rc->wt(dsti), rc);

    ret_flag = lmn_mem_cmp_ground(srcvec, dstvec);

    free_links(srcvec);
    free_links(dstvec);

    if ((!ret_flag && INSTR_EQGROUND == op) ||
        (ret_flag && INSTR_NEQGROUND == op)) {
      return FALSE;
    }
    break;
  }
  case INSTR_COPYHLGROUND:
  case INSTR_COPYHLGROUNDINDIRECT:
  case INSTR_COPYGROUND: {
    LmnInstrVar dstlist, srclist, memi;
    Vector *srcvec, *dstlovec, *retvec; /* ???????????????????????? */
    ProcessTableRef atommap;
    ProcessTableRef hlinkmap;

    READ_VAL(LmnInstrVar, instr, dstlist);
    READ_VAL(LmnInstrVar, instr, srclist);
    READ_VAL(LmnInstrVar, instr, memi);

    /* ???????????????????????????????????????????????? */
    srcvec = links_from_idxs((Vector *)rc->wt(srclist), rc);

    switch (op) {
    case INSTR_COPYHLGROUND:
    case INSTR_COPYHLGROUNDINDIRECT: {
      ProcessTableRef attr_functors;
      Vector attr_dataAtoms;
      Vector attr_dataAtom_attrs;
      attr_dataAtoms.init(16);
      attr_dataAtom_attrs.init(16);
      attr_functors = new ProcessTbl(16);
      LmnInstrVar i = 0, n;

      READ_VAL(LmnInstrVar, instr, n);

      switch (op) {
      case INSTR_COPYHLGROUNDINDIRECT: {
        LmnInstrVar ai;
        for (; n--; i++) {
          READ_VAL(LmnInstrVar, instr, ai);
          if (LMN_ATTR_IS_DATA(rc->at(ai))) {
            attr_dataAtom_attrs.push(rc->at(ai));
            attr_dataAtoms.push(rc->wt(ai));
          } else {
            LmnFunctor f;
            f = ((LmnSymbolAtomRef)rc->wt(ai))->get_functor();
            attr_functors->proc_tbl_put(f, f);
          }
        }
        break;
      }
      case INSTR_COPYHLGROUND: {
        for (; n--; i++) {
          LmnLinkAttr attr;
          READ_VAL(LmnLinkAttr, instr, attr);
          if (LMN_ATTR_IS_DATA(attr)) {
            LmnAtomRef at;
            attr_dataAtom_attrs.push(attr);
            READ_DATA_ATOM(at, attr);
            attr_dataAtoms.push((LmnWord)at);
          } else {
            LmnFunctor f;
            READ_VAL(LmnFunctor, instr, f);
            attr_functors->proc_tbl_put(f, f);
          }
        }
        break;
      }
      }
      lmn_mem_copy_hlground((LmnMembraneRef)rc->wt(memi), srcvec, &dstlovec,
                            &atommap, &hlinkmap, &attr_functors,
                            &attr_dataAtoms, &attr_dataAtom_attrs);

      break;
    }
    case INSTR_COPYGROUND:
      lmn_mem_copy_ground((LmnMembraneRef)rc->wt(memi), srcvec, &dstlovec,
                          &atommap);
      break;
    }
    free_links(srcvec);

    /* ?????????????????? */
    retvec = new Vector(2);
    retvec->push((LmnWord)dstlovec);
    retvec->push((LmnWord)atommap);
    rc->reg(dstlist) = {(LmnWord)retvec, LIST_AND_MAP, TT_OTHER};

    this->push_stackframe([=](interpreter &itr, bool result) {
      free_links(dstlovec);
      delete retvec;
      LMN_ASSERT(result);
      return result ? command_result::Success : command_result::Failure;
    });

    break;
  }
  case INSTR_REMOVEHLGROUND:
  case INSTR_REMOVEHLGROUNDINDIRECT:
  case INSTR_FREEHLGROUND:
  case INSTR_FREEHLGROUNDINDIRECT:
  case INSTR_REMOVEGROUND:
  case INSTR_FREEGROUND: {
    LmnInstrVar listi, memi;
    Vector *srcvec; /* ???????????????????????? */

    READ_VAL(LmnInstrVar, instr, listi);
    if (INSTR_REMOVEGROUND == op || INSTR_REMOVEHLGROUND == op ||
        INSTR_REMOVEHLGROUNDINDIRECT == op) {
      READ_VAL(LmnInstrVar, instr, memi);
    } else {
      memi = 0;
    }
    srcvec = links_from_idxs((Vector *)rc->wt(listi), rc);

    switch (op) {
    case INSTR_REMOVEHLGROUND:
    case INSTR_REMOVEHLGROUNDINDIRECT:
    case INSTR_FREEHLGROUND:
    case INSTR_FREEHLGROUNDINDIRECT: {
      ProcessTableRef attr_functors;
      Vector attr_dataAtoms;
      Vector attr_dataAtom_attrs;
      attr_dataAtoms.init(16);
      attr_dataAtom_attrs.init(16);
      attr_functors = new ProcessTbl(16);
      LmnInstrVar i = 0, n;

      READ_VAL(LmnInstrVar, instr, n);

      switch (op) {
      case INSTR_REMOVEHLGROUNDINDIRECT:
      case INSTR_FREEHLGROUNDINDIRECT: {
        LmnInstrVar ai;
        for (; n--; i++) {
          READ_VAL(LmnInstrVar, instr, ai);
          if (LMN_ATTR_IS_DATA(rc->at(ai))) {
            attr_dataAtom_attrs.push(rc->at(ai));
            attr_dataAtoms.push(rc->wt(ai));
          } else {
            LmnFunctor f;
            f = ((LmnSymbolAtomRef)rc->wt(ai))->get_functor();
            attr_functors->proc_tbl_put(f, f);
          }
        }
        break;
      }
      case INSTR_REMOVEHLGROUND:
      case INSTR_FREEHLGROUND: {
        for (; n--; i++) {
          LmnLinkAttr attr;
          READ_VAL(LmnLinkAttr, instr, attr);
          if (LMN_ATTR_IS_DATA(attr)) {
            LmnAtomRef at;
            attr_dataAtom_attrs.push(attr);
            READ_DATA_ATOM(at, attr);
            attr_dataAtoms.push((LmnWord)at);
          } else {
            LmnFunctor f;
            READ_VAL(LmnFunctor, instr, f);
            attr_functors->proc_tbl_put(f, f);
          }
        }
        break;
      }
      }
      switch (op) {
      case INSTR_REMOVEHLGROUND:
      case INSTR_REMOVEHLGROUNDINDIRECT:
        lmn_mem_remove_hlground((LmnMembraneRef)rc->wt(memi), srcvec,
                                &attr_functors, &attr_dataAtoms,
                                &attr_dataAtom_attrs);
        break;
      case INSTR_FREEHLGROUND:
      case INSTR_FREEHLGROUNDINDIRECT:
        lmn_mem_free_hlground(
            srcvec, // this may also cause a bug, see 15 lines below
            &attr_functors, &attr_dataAtoms, &attr_dataAtom_attrs);
        break;
      }
      delete attr_functors;
      attr_dataAtoms.destroy();
      attr_dataAtom_attrs.destroy();
      break;
    }
    case INSTR_REMOVEGROUND:
      ((LmnMembraneRef)rc->wt(memi))->remove_ground(srcvec);
      break;
    case INSTR_FREEGROUND:
      lmn_mem_free_ground(srcvec);
      break;
    }

    free_links(srcvec);

    break;
  }
  case INSTR_ISUNARY: {
    LmnInstrVar atomi;
    READ_VAL(LmnInstrVar, instr, atomi);

    if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
      switch (rc->at(atomi)) {
      case LMN_SP_ATOM_ATTR:
        /* ???????????????????????????ground????????????unary?????????????????? */
        if (!SP_ATOM_IS_GROUND(rc->wt(atomi))) {
          return FALSE;
        }
        break;
      default:
        break;
      }
    } else if (((LmnSymbolAtomRef)rc->wt(atomi))->get_arity() != 1)
      return FALSE;
    break;
  }
  case INSTR_ISINT: {
    LmnInstrVar atomi;
    READ_VAL(LmnInstrVar, instr, atomi);

    if (rc->at(atomi) != LMN_INT_ATTR)
      return FALSE;
    break;
  }
  case INSTR_ISFLOAT: {
    LmnInstrVar atomi;
    READ_VAL(LmnInstrVar, instr, atomi);

    if (rc->at(atomi) != LMN_DBL_ATTR)
      return FALSE;
    break;
  }
  case INSTR_ISSTRING: {
    LmnInstrVar atomi;

    READ_VAL(LmnInstrVar, instr, atomi);

    if (!lmn_is_string((LmnAtomRef)rc->wt(atomi), rc->at(atomi)))
      return FALSE;
    break;
  }
  case INSTR_ISINTFUNC: {
    LmnInstrVar funci;
    READ_VAL(LmnInstrVar, instr, funci);

    if (rc->at(funci) != LMN_INT_ATTR)
      return FALSE;
    break;
  }
  case INSTR_ISFLOATFUNC: {
    LmnInstrVar funci;
    READ_VAL(LmnInstrVar, instr, funci);

    if (rc->at(funci) != LMN_DBL_ATTR)
      return FALSE;
    break;
  }
  case INSTR_COPYATOM: {
    LmnInstrVar atom1, memi, atom2;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(atom1) = {
        (LmnWord)lmn_copy_atom((LmnAtomRef)rc->wt(atom2), rc->at(atom2)),
        rc->at(atom2), TT_OTHER};
    lmn_mem_push_atom((LmnMembraneRef)rc->wt(memi), (LmnAtomRef)rc->wt(atom1),
                      rc->at(atom1));
    break;
  }
  case INSTR_EQATOM: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    /* ???????????????????????????????????????,?????????????????????????????????
       ????????????FALSE????????? */
    if (LMN_ATTR_IS_DATA(rc->at(atom1)) || LMN_ATTR_IS_DATA(rc->at(atom2)) ||
        LMN_SATOM(rc->wt(atom1)) != LMN_SATOM(rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_NEQATOM: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(LMN_ATTR_IS_DATA(rc->at(atom1)) || LMN_ATTR_IS_DATA(rc->at(atom2)) ||
          LMN_SATOM(rc->wt(atom1)) != LMN_SATOM(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_EQMEM: {
    LmnInstrVar mem1, mem2;

    READ_VAL(LmnInstrVar, instr, mem1);
    READ_VAL(LmnInstrVar, instr, mem2);
    if (rc->wt(mem1) != rc->wt(mem2))
      return FALSE;
    break;
  }
  case INSTR_NEQMEM: {
    LmnInstrVar mem1, mem2;
    READ_VAL(LmnInstrVar, instr, mem1);
    READ_VAL(LmnInstrVar, instr, mem2);

    if (rc->wt(mem1) == rc->wt(mem2))
      return FALSE;
    break;
  }
  case INSTR_STABLE: {
    LmnInstrVar memi;
    READ_VAL(LmnInstrVar, instr, memi);

    if (((LmnMembraneRef)rc->wt(memi))->is_active()) {
      return FALSE;
    }

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        LmnMembraneRef m = (LmnMembraneRef)rc->wt(memi);
        dpor_LHS_flag_add(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_STABLE);
        this->push_stackframe([=](interpreter &itr, bool result) {
          dpor_LHS_flag_remove(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_STABLE);
          return command_result::
              Failure; /* ?????????????????????????????????ND?????????FALSE???????????????
                        */
        });
      }
    }

    break;
  }
  case INSTR_NEWLIST: {
    LmnInstrVar listi;
    Vector *listvec = new Vector(16);
    READ_VAL(LmnInstrVar, instr, listi);
    rc->reg(listi) = {(LmnWord)listvec, 0, TT_OTHER};

    /* ???????????????????????? */
    this->push_stackframe([=](interpreter &itr, bool result) {
      delete listvec;
      return result ? command_result::Success : command_result::Failure;
    });
    break;
  }
  case INSTR_ADDTOLIST: {
    LmnInstrVar listi, linki;
    READ_VAL(LmnInstrVar, instr, listi);
    READ_VAL(LmnInstrVar, instr, linki);
    ((Vector *)rc->wt(listi))->push(linki);

    break;
  }
  case INSTR_GETFROMLIST: {
    LmnInstrVar dsti, listi, posi;
    READ_VAL(LmnInstrVar, instr, dsti);
    READ_VAL(LmnInstrVar, instr, listi);
    READ_VAL(LmnInstrVar, instr, posi);

    switch (rc->at(listi)) {
    case LIST_AND_MAP:
      if (posi == 0) {
        rc->reg(dsti) = {((Vector *)rc->wt(listi))->get((unsigned int)posi),
                         LINK_LIST, TT_OTHER};
      } else if (posi == 1) {
        rc->reg(dsti) = {((Vector *)rc->wt(listi))->get((unsigned int)posi),
                         MAP, TT_OTHER};
      } else {
        lmn_fatal("unexpected attribute @instr_getfromlist");
      }
      break;
    case LINK_LIST: /* LinkObj???free????????????????????? */
    {
      LinkObjRef lo =
          (LinkObjRef)((Vector *)rc->wt(listi))->get((unsigned int)posi);
      rc->reg(dsti) = {(LmnWord)LinkObjGetAtom(lo), LinkObjGetPos(lo), TT_ATOM};
      break;
    }
    }
    break;
  }
  case INSTR_IADD: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);
    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) + (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_ISUB: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) - (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IMUL: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) * (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IDIV: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) / (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};

    break;
  }
  case INSTR_INEG: {
    LmnInstrVar dstatom, atomi;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atomi);
    rc->reg(dstatom) = {static_cast<LmnWord>((-(long)rc->wt(atomi))),
                        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IMOD: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) % (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_INOT: {
    LmnInstrVar dstatom, atomi;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atomi);
    rc->reg(dstatom) = {static_cast<LmnWord>((~(int)rc->wt(atomi))),
                        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IAND: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) & (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IOR: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) | (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};

    break;
  }
  case INSTR_IXOR: {
    LmnInstrVar dstatom, atom1, atom2;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    rc->reg(dstatom) = {
        static_cast<LmnWord>(((long)rc->wt(atom1) ^ (long)rc->wt(atom2))),
        LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_ILT: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!((long)rc->wt(atom1) < (long)rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_ILE: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!((long)rc->wt(atom1) <= (long)rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_IGT: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!((long)rc->wt(atom1) > (long)rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_IGE: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!((long)rc->wt(atom1) >= (long)rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_IEQ: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!((long)rc->wt(atom1) == (long)rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_INE: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!((long)rc->wt(atom1) != (long)rc->wt(atom2)))
      return FALSE;
    break;
  }
  case INSTR_ILTFUNC: {
    LmnInstrVar func1, func2;
    READ_VAL(LmnInstrVar, instr, func1);
    READ_VAL(LmnInstrVar, instr, func2);

    if (!((long)rc->wt(func1) < (long)rc->wt(func2)))
      return FALSE;
    break;
  }
  case INSTR_ILEFUNC: {
    LmnInstrVar func1, func2;
    READ_VAL(LmnInstrVar, instr, func1);
    READ_VAL(LmnInstrVar, instr, func2);

    if (!((long)rc->wt(func1) <= (long)rc->wt(func2)))
      return FALSE;
    break;
  }
  case INSTR_IGTFUNC: {
    LmnInstrVar func1, func2;
    READ_VAL(LmnInstrVar, instr, func1);
    READ_VAL(LmnInstrVar, instr, func2);

    if (!((long)rc->wt(func1) > (long)rc->wt(func2)))
      return FALSE;
    break;
  }
  case INSTR_IGEFUNC: {
    LmnInstrVar func1, func2;
    READ_VAL(LmnInstrVar, instr, func1);
    READ_VAL(LmnInstrVar, instr, func2);

    if (!((long)rc->wt(func1) >= (long)rc->wt(func2)))
      return FALSE;
    break;
  }
  case INSTR_FADD: {
    LmnInstrVar dstatom, atom1, atom2;
    LmnAtom d;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    d = lmn_create_double_atom(lmn_get_double(rc->wt(atom1)) +
                               lmn_get_double(rc->wt(atom2)));
    rc->reg(dstatom) = {d, LMN_DBL_ATTR, TT_ATOM};
    break;
  }
  case INSTR_FSUB: {
    LmnInstrVar dstatom, atom1, atom2;
    LmnAtom d;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    d = lmn_create_double_atom(lmn_get_double(rc->wt(atom1)) -
                               lmn_get_double(rc->wt(atom2)));
    rc->reg(dstatom) = {d, LMN_DBL_ATTR, TT_ATOM};
    break;
  }
  case INSTR_FMUL: {
    LmnInstrVar dstatom, atom1, atom2;
    LmnAtom d;

    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    d = lmn_create_double_atom(lmn_get_double(rc->wt(atom1)) *
                               lmn_get_double(rc->wt(atom2)));
    rc->reg(dstatom) = {d, LMN_DBL_ATTR, TT_ATOM};
    break;
  }
  case INSTR_FDIV: {
    LmnInstrVar dstatom, atom1, atom2;
    LmnAtom d;

    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    d = lmn_create_double_atom(lmn_get_double(rc->wt(atom1)) /
                               lmn_get_double(rc->wt(atom2)));
    rc->reg(dstatom) = {d, LMN_DBL_ATTR, TT_ATOM};
    break;
  }
  case INSTR_FNEG: {
    LmnInstrVar dstatom, atomi;
    LmnAtom d;
    READ_VAL(LmnInstrVar, instr, dstatom);
    READ_VAL(LmnInstrVar, instr, atomi);

    d = lmn_create_double_atom(-lmn_get_double(rc->wt(atomi)));
    rc->reg(dstatom) = {d, LMN_DBL_ATTR, TT_ATOM};
    break;
  }
  case INSTR_FLT: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(lmn_get_double(rc->wt(atom1)) < lmn_get_double(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_FLE: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(lmn_get_double(rc->wt(atom1)) <= lmn_get_double(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_FGT: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(lmn_get_double(rc->wt(atom1)) > lmn_get_double(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_FGE: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(lmn_get_double(rc->wt(atom1)) >= lmn_get_double(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_FEQ: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(lmn_get_double(rc->wt(atom1)) == lmn_get_double(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_FNE: {
    LmnInstrVar atom1, atom2;
    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!(lmn_get_double(rc->wt(atom1)) != lmn_get_double(rc->wt(atom2))))
      return FALSE;
    break;
  }
  case INSTR_ALLOCATOM: {
    LmnInstrVar atomi;
    LmnLinkAttr attr;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnLinkAttr, instr, attr);
    rc->at(atomi) = attr;
    if (LMN_ATTR_IS_DATA(attr)) {
      LmnWord w;
      LmnByte a = rc->at(atomi), t;
      READ_CONST_DATA_ATOM(w, a, t);
      rc->reg(atomi) = {w, a, t};
    } else { /* symbol atom */
      LmnFunctor f;
      /*         fprintf(stderr, "symbol atom can't be created in GUARD\n");
       */
      /*         exit(EXIT_FAILURE); */
      READ_VAL(LmnFunctor, instr, f);

      /* ?????????allocatom?????????????????????????????????????????????????????????????????????????????????????????????
       */
      rc->wt(atomi) = f;
    }
    rc->tt(atomi) = TT_OTHER; /* ??????????????????????????????????????????????????? */
    break;
  }
  case INSTR_ALLOCATOMINDIRECT: {
    LmnInstrVar atomi;
    LmnInstrVar srcatomi;

    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnInstrVar, instr, srcatomi);

    if (LMN_ATTR_IS_DATA(rc->at(srcatomi))) {
      if (LMN_ATTR_IS_EX(rc->at(srcatomi))) {
        rc->wt(atomi) = rc->wt(srcatomi);
      } else {
        rc->wt(atomi) = lmn_copy_data_atom(rc->wt(srcatomi), rc->at(srcatomi));
      }
      rc->at(atomi) = rc->at(srcatomi);
      rc->tt(atomi) = TT_OTHER;
    } else { /* symbol atom */
      fprintf(stderr, "symbol atom can't be created in GUARD\n");
      exit(EXIT_FAILURE);
    }
    break;
  }
  case INSTR_SAMEFUNC: {
    LmnInstrVar atom1, atom2;

    READ_VAL(LmnInstrVar, instr, atom1);
    READ_VAL(LmnInstrVar, instr, atom2);

    if (!lmn_eq_func((LmnAtomRef)rc->wt(atom1), rc->at(atom1),
                     (LmnAtomRef)rc->wt(atom2), rc->at(atom2)))
      return FALSE;
    break;
  }
  case INSTR_GETFUNC: {
    LmnInstrVar funci, atomi;

    READ_VAL(LmnInstrVar, instr, funci);
    READ_VAL(LmnInstrVar, instr, atomi);

    if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
      /* ?????????????????????????????????????????????????????????????????????????????????????????????
         double ????????????????????????????????????????????? */
      rc->reg(funci) = {rc->wt(atomi), rc->at(atomi), TT_OTHER};
    } else {

      rc->reg(funci) = {
          (LmnWord)((LmnSymbolAtomRef)rc->wt(atomi))->get_functor(),
          rc->at(atomi), TT_OTHER};
    }
    break;
  }
  case INSTR_PRINTINSTR: {
    char c;

    while (TRUE) {
      READ_VAL(char, instr, c);
      if (!c)
        break;
      fprintf(stderr, "%c", c);
    }
    break;
  }
  case INSTR_SETMEMNAME: {
    LmnInstrVar memi;
    lmn_interned_str name;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(lmn_interned_str, instr, name);
    ((LmnMembraneRef)rc->wt(memi))->set_name(name);
    break;
  }
  case INSTR_COPYRULES: {
    LmnInstrVar destmemi, srcmemi;
    unsigned int i;

    READ_VAL(LmnInstrVar, instr, destmemi);
    READ_VAL(LmnInstrVar, instr, srcmemi);
    auto &v = ((LmnMembraneRef)rc->wt(srcmemi))->get_rulesets();
    for (auto &rs : v) {
      auto cp = new LmnRuleSet(*rs);
      lmn_mem_add_ruleset((LmnMembraneRef)rc->wt(destmemi), cp);
    }
    break;
  }
  case INSTR_REMOVEPROXIES: {
    LmnInstrVar memi;

    READ_VAL(LmnInstrVar, instr, memi);
    ((LmnMembraneRef)rc->wt(memi))->remove_proxies();
    break;
  }
  case INSTR_INSERTPROXIES: {
    LmnInstrVar parentmemi, childmemi;

    READ_VAL(LmnInstrVar, instr, parentmemi);
    READ_VAL(LmnInstrVar, instr, childmemi);
    ((LmnMembraneRef)rc->wt(parentmemi))
        ->insert_proxies((LmnMembraneRef)rc->wt(childmemi));
    break;
  }
  case INSTR_DELETECONNECTORS: {
    LmnInstrVar srcset, srcmap;
    HashSet *delset;
    ProcessTableRef delmap;
    HashSetIterator it;
    READ_VAL(LmnInstrVar, instr, srcset);
    READ_VAL(LmnInstrVar, instr, srcmap);

    delset = (HashSet *)rc->wt(srcset);
    delmap = (ProcessTableRef)rc->wt(srcmap);

    for (it = hashset_iterator(delset); !hashsetiter_isend(&it);
         hashsetiter_next(&it)) {
      LmnSymbolAtomRef orig, copy;
      LmnWord t;

      orig = (LmnSymbolAtomRef)hashsetiter_entry(&it);
      t = 0;
      proc_tbl_get_by_atom(delmap, orig, &t);
      copy = (LmnSymbolAtomRef)t;

      lmn_mem_unify_symbol_atom_args(copy, 0, copy, 1);
      /* mem ????????????????????????????????????????????????????????????????????????.
       * UNIFY????????????natom????????????????????????????????? */
      copy->get_next()->set_prev(copy->get_prev());
      copy->get_prev()->set_next(copy->get_next());

      lmn_delete_atom(copy);
    }

    delete delmap;
    break;
  }
  case INSTR_REMOVETOPLEVELPROXIES: {
    LmnInstrVar memi;

    READ_VAL(LmnInstrVar, instr, memi);
    ((LmnMembraneRef)rc->wt(memi))->remove_toplevel_proxies();
    break;
  }
  case INSTR_DEREFFUNC: {
    LmnInstrVar funci, atomi, pos;
    LmnLinkAttr attr;

    READ_VAL(LmnInstrVar, instr, funci);
    READ_VAL(LmnInstrVar, instr, atomi);
    READ_VAL(LmnLinkAttr, instr, pos);

    attr = ((LmnSymbolAtomRef)rc->wt(atomi))->get_attr(pos);
    if (LMN_ATTR_IS_DATA(attr)) {
      rc->reg(funci) = {
          (LmnWord)((LmnSymbolAtomRef)rc->wt(atomi))->get_link(pos), attr,
          TT_OTHER};
    } else { /* symbol atom */
      rc->reg(funci) = {
          ((LmnSymbolAtomRef)((LmnSymbolAtomRef)rc->wt(atomi))->get_link(pos))
              ->get_functor(),
          attr, TT_OTHER};
    }
    break;
  }
  case INSTR_LOADFUNC: {
    LmnInstrVar funci;
    LmnLinkAttr attr;

    READ_VAL(LmnInstrVar, instr, funci);
    READ_VAL(LmnLinkAttr, instr, attr);
    rc->at(funci) = attr;
    rc->tt(funci) = TT_OTHER;
    if (LMN_ATTR_IS_DATA(attr)) {
      LmnWord w;
      LmnByte a = rc->at(funci), t;
      READ_CONST_DATA_ATOM(w, a, t);
      rc->reg(funci) = {w, a, t};
    } else {
      LmnFunctor f;

      READ_VAL(LmnFunctor, instr, f);
      rc->wt(funci) = f;
      rc->tt(funci) = TT_OTHER;
    }
    break;
  }
  case INSTR_EQFUNC: {
    LmnInstrVar func0;
    LmnInstrVar func1;

    READ_VAL(LmnFunctor, instr, func0);
    READ_VAL(LmnFunctor, instr, func1);

    if (rc->at(func0) != rc->at(func1))
      return FALSE;
    switch (rc->at(func0)) {
    case LMN_INT_ATTR:
      if ((long)rc->wt(func0) != (long)rc->wt(func1))
        return FALSE;
      break;
    case LMN_DBL_ATTR:
      if (lmn_get_double(rc->wt(func0)) != lmn_get_double(rc->wt(func1)))
        return FALSE;
      break;
    case LMN_HL_ATTR:
      if (!(lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)rc->wt(func0)))
               ->eq_hl(lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)rc->wt(func1))))
        return FALSE;
      break;
    case LMN_SP_ATOM_ATTR:
      if (!SP_ATOM_EQ(rc->wt(func0), rc->wt(func1)))
        return FALSE;
    default:
      if (rc->wt(func0) != rc->wt(func1))
        return FALSE;
      break;
    }
    break;
  }
  case INSTR_NEQFUNC: {
    LmnInstrVar func0;
    LmnInstrVar func1;

    READ_VAL(LmnFunctor, instr, func0);
    READ_VAL(LmnFunctor, instr, func1);

    if (rc->at(func0) == rc->at(func1)) {
      switch (rc->at(func0)) {
      case LMN_INT_ATTR:
        if ((long)rc->wt(func0) == (long)rc->wt(func1))
          return FALSE;
        break;
      case LMN_DBL_ATTR:
        if (lmn_get_double(rc->wt(func0)) == lmn_get_double(rc->wt(func1)))
          return FALSE;
        break;
      case LMN_HL_ATTR:
        if ((lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)rc->wt(func0)))
                ->eq_hl(
                    lmn_hyperlink_at_to_hl((LmnSymbolAtomRef)rc->wt(func1))))
          return FALSE;
        break;
      case LMN_SP_ATOM_ATTR:
        if (SP_ATOM_EQ(rc->wt(func0), rc->wt(func1)))
          return FALSE;
      default:
        if (rc->wt(func0) == rc->wt(func1))
          return FALSE;
        break;
      }
    }
    break;
  }
  case INSTR_ADDATOM: {
    LmnInstrVar memi, atomi;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, atomi);
    lmn_mem_push_atom((LmnMembraneRef)rc->wt(memi), (LmnAtomRef)rc->wt(atomi),
                      rc->at(atomi));
    break;
  }
  case INSTR_MOVECELLS: {
    LmnInstrVar destmemi, srcmemi;

    READ_VAL(LmnInstrVar, instr, destmemi);
    READ_VAL(LmnInstrVar, instr, srcmemi);
    LMN_ASSERT(rc->wt(destmemi) != rc->wt(srcmemi));
    ((LmnMembraneRef)rc->wt(destmemi))
        ->move_cells((LmnMembraneRef)rc->wt(srcmemi));
    break;
  }
  case INSTR_REMOVETEMPORARYPROXIES: {
    LmnInstrVar memi;

    READ_VAL(LmnInstrVar, instr, memi);
    ((LmnMembraneRef)rc->wt(memi))->remove_temporary_proxies();
    break;
  }
  case INSTR_NFREELINKS: {
    LmnInstrVar memi, count;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, count);

    if (!((LmnMembraneRef)rc->wt(memi))->nfreelinks(count))
      return FALSE;

    if (rc->has_mode(REACT_ND)) {
      auto mcrc = dynamic_cast<MCReactContext *>(rc);
      if (mcrc->has_optmode(DynamicPartialOrderReduction) && !rc->is_zerostep) {
        LmnMembraneRef m = (LmnMembraneRef)rc->wt(memi);
        dpor_LHS_flag_add(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NFLINKS);
        this->push_stackframe([=](interpreter &itr, bool result) {
          LMN_ASSERT(!result);
          dpor_LHS_flag_remove(RC_POR_DATA(rc), m->mem_id(), LHS_MEM_NFLINKS);
          return command_result::
              Failure; /* ?????????????????????????????????ND?????????FALSE???????????????
                        */
        });
      }
    }

    break;
  }
  case INSTR_COPYCELLS: {
    LmnInstrVar mapi, destmemi, srcmemi;

    READ_VAL(LmnInstrVar, instr, mapi);
    READ_VAL(LmnInstrVar, instr, destmemi);
    READ_VAL(LmnInstrVar, instr, srcmemi);
    rc->wt(mapi) = (LmnWord)lmn_mem_copy_cells((LmnMembraneRef)rc->wt(destmemi),
                                               (LmnMembraneRef)rc->wt(srcmemi));
    rc->tt(mapi) = TT_OTHER;
    break;
  }
  case INSTR_LOOKUPLINK: {
    LmnInstrVar destlinki, tbli, srclinki;

    READ_VAL(LmnInstrVar, instr, destlinki);
    READ_VAL(LmnInstrVar, instr, tbli);
    READ_VAL(LmnInstrVar, instr, srclinki);

    rc->at(destlinki) = rc->at(srclinki);
    rc->tt(destlinki) = TT_ATOM;
    if (LMN_ATTR_IS_DATA(rc->at(srclinki))) {
      rc->wt(destlinki) = (LmnWord)rc->wt(srclinki);
    } else { /* symbol atom */
      ProcessTableRef ht = (ProcessTableRef)rc->wt(tbli);
      LmnWord w = rc->wt(destlinki);
      proc_tbl_get_by_atom(ht, (LmnSymbolAtomRef)rc->wt(srclinki), &w);
      rc->wt(destlinki) = w;
    }
    break;
  }
  case INSTR_CLEARRULES: {
    LmnInstrVar memi;

    READ_VAL(LmnInstrVar, instr, memi);
    ((LmnMembraneRef)rc->wt(memi))->clearrules();
    break;
  }
  case INSTR_DROPMEM: {
    LmnInstrVar memi;

    READ_VAL(LmnInstrVar, instr, memi);
    ((LmnMembraneRef)rc->wt(memi))->drop();
    break;
  }
  case INSTR_TESTMEM: {
    LmnInstrVar memi, atomi;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, atomi);
    LMN_ASSERT(!LMN_ATTR_IS_DATA(rc->at(atomi)));
    LMN_ASSERT(
        LMN_IS_PROXY_FUNCTOR(((LmnSymbolAtomRef)rc->wt(atomi))->get_functor()));

    if (LMN_PROXY_GET_MEM((LmnSymbolAtomRef)rc->wt(atomi)) !=
        (LmnMembraneRef)rc->wt(memi))
      return FALSE;
    break;
  }
  case INSTR_IADDFUNC: {
    LmnInstrVar desti, i0, i1;

    READ_VAL(LmnInstrVar, instr, desti);
    READ_VAL(LmnInstrVar, instr, i0);
    READ_VAL(LmnInstrVar, instr, i1);
    LMN_ASSERT(rc->at(i0) == LMN_INT_ATTR);
    LMN_ASSERT(rc->at(i1) == LMN_INT_ATTR);
    rc->reg(desti) = {rc->wt(i0) + rc->wt(i1), LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_ISUBFUNC: {
    LmnInstrVar desti, i0, i1;

    READ_VAL(LmnInstrVar, instr, desti);
    READ_VAL(LmnInstrVar, instr, i0);
    READ_VAL(LmnInstrVar, instr, i1);
    LMN_ASSERT(rc->at(i0) == LMN_INT_ATTR);
    LMN_ASSERT(rc->at(i1) == LMN_INT_ATTR);
    rc->reg(desti) = {rc->wt(i0) - rc->wt(i1), LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IMULFUNC: {
    LmnInstrVar desti, i0, i1;

    READ_VAL(LmnInstrVar, instr, desti);
    READ_VAL(LmnInstrVar, instr, i0);
    READ_VAL(LmnInstrVar, instr, i1);
    LMN_ASSERT(rc->at(i0) == LMN_INT_ATTR);
    LMN_ASSERT(rc->at(i1) == LMN_INT_ATTR);
    rc->reg(desti) = {rc->wt(i0) * rc->wt(i1), LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IDIVFUNC: {
    LmnInstrVar desti, i0, i1;

    READ_VAL(LmnInstrVar, instr, desti);
    READ_VAL(LmnInstrVar, instr, i0);
    READ_VAL(LmnInstrVar, instr, i1);
    LMN_ASSERT(rc->at(i0) == LMN_INT_ATTR);
    LMN_ASSERT(rc->at(i1) == LMN_INT_ATTR);
    rc->reg(desti) = {rc->wt(i0) / rc->wt(i1), LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_IMODFUNC: {
    LmnInstrVar desti, i0, i1;

    READ_VAL(LmnInstrVar, instr, desti);
    READ_VAL(LmnInstrVar, instr, i0);
    READ_VAL(LmnInstrVar, instr, i1);
    LMN_ASSERT(rc->at(i0) == LMN_INT_ATTR);
    LMN_ASSERT(rc->at(i1) == LMN_INT_ATTR);
    rc->reg(desti) = {rc->wt(i0) % rc->wt(i1), LMN_INT_ATTR, TT_ATOM};
    break;
  }
  case INSTR_GROUP: {
    LmnSubInstrSize subinstr_size;
    READ_VAL(LmnSubInstrSize, instr, subinstr_size);

    auto next = instr + subinstr_size;
    this->push_stackframe(exec_subinstructions_group(next));
    break;
  }
  case INSTR_BRANCH: {
    LmnSubInstrSize subinstr_size;
    READ_VAL(LmnSubInstrSize, instr, subinstr_size);

    if (rc->get_hl_sameproccxt()) {
      /*branch???hyperlink????????????????????????????????????????????? */
      rc->clear_hl_spc();
    }

    auto next = instr + subinstr_size;
    this->push_stackframe(exec_subinstructions_branch(next));
    break;
  }
  case INSTR_LOOP: {
    LmnSubInstrSize subinstr_size;
    READ_VAL(LmnSubInstrSize, instr, subinstr_size);

    this->push_stackframe(
        exec_subinstructions_while(instr, instr + subinstr_size));
    break;
  }
  case INSTR_CALLBACK: {
    LmnInstrVar memi, atomi;
    LmnSymbolAtomRef atom;
    const struct CCallback *c;

    READ_VAL(LmnInstrVar, instr, memi);
    READ_VAL(LmnInstrVar, instr, atomi);

    atom = (LmnSymbolAtomRef)rc->wt(atomi);

    if (!LMN_ATTR_IS_DATA(atom->get_attr(0))) {
      LmnSymbolAtomRef f_name = (LmnSymbolAtomRef)atom->get_link(0);
      lmn_interned_str name =
          LMN_FUNCTOR_NAME_ID(lmn_functor_table, f_name->get_functor());
      int arity = LMN_FUNCTOR_ARITY(lmn_functor_table, atom->get_functor());

      c = CCallback::get_ccallback(name);
      if (!c)
        break;

      if (arity - 1 != c->get_arity()) {
        fprintf(stderr, "EXTERNAL FUNC: invalid arity - %s\n",
                LMN_SYMBOL_STR(name));
        break;
      }

      /* (2015-07-30) moved to the end so that lmn_dump_mem can safely
         be called in callback functions
      lmn_mem_delete_atom((LmnMembraneRef)rc->wt( memi), rc->wt( atomi),
      rc->at(atomi)); lmn_mem_delete_atom((LmnMembraneRef)rc->wt( memi),
                          atom->get_link(0),
                          atom->get_attr(0));
      */

      switch (arity) {
      case 1:
        ((callback_0)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi));
        break;
      case 2:
        ((callback_1)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                 atom->get_link(1), atom->get_attr(1));
        break;
      case 3:
        ((callback_2)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                 atom->get_link(1), atom->get_attr(1),
                                 atom->get_link(2), atom->get_attr(2));
        break;
      case 4:
        ((callback_3)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                 atom->get_link(1), atom->get_attr(1),
                                 atom->get_link(2), atom->get_attr(2),
                                 atom->get_link(3), atom->get_attr(3));
        break;
      case 5:
        ((callback_4)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                 atom->get_link(1), atom->get_attr(1),
                                 atom->get_link(2), atom->get_attr(2),
                                 atom->get_link(3), atom->get_attr(3),
                                 atom->get_link(4), atom->get_attr(4));
        break;
      case 6:
        ((callback_5)c->get_f())(
            rc, (LmnMembraneRef)rc->wt(memi), atom->get_link(1),
            atom->get_attr(1), atom->get_link(2), atom->get_attr(2),
            atom->get_link(3), atom->get_attr(3), atom->get_link(4),
            atom->get_attr(4), atom->get_link(5), atom->get_attr(5));
        break;
      default:
        printf("EXTERNAL FUNCTION: too many arguments\n");
        break;
      }

      lmn_mem_delete_atom((LmnMembraneRef)rc->wt(memi),
                          (LmnAtomRef)rc->wt(atomi), rc->at(atomi));
      lmn_mem_delete_atom((LmnMembraneRef)rc->wt(memi),
                          ((LmnSymbolAtomRef)atom)->get_link(0),
                          ((LmnSymbolAtomRef)atom)->get_attr(0));
    }

    break;
  }
  case INSTR_GETCLASS: {
    LmnInstrVar reti, atomi;

    READ_VAL(LmnInstrVar, instr, reti);
    READ_VAL(LmnInstrVar, instr, atomi);

    rc->tt(reti) = TT_OTHER;
    if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
      switch (rc->at(atomi)) {
      case LMN_INT_ATTR:
        rc->wt(reti) = lmn_intern("int");
        break;
      case LMN_DBL_ATTR:
        rc->wt(reti) = lmn_intern("float");
        break;
      case LMN_SP_ATOM_ATTR:
        rc->wt(reti) = SP_ATOM_NAME(rc->wt(atomi));
        break;
      default:
        rc->wt(reti) = lmn_intern("unknown");
        break;
      }
    } else { /* symbol atom */
      rc->wt(reti) = lmn_intern("symbol");
    }
    break;
  }
  case INSTR_SUBCLASS: {
    LmnInstrVar subi, superi;

    READ_VAL(LmnInstrVar, instr, subi);
    READ_VAL(LmnInstrVar, instr, superi);

    /* ?????????????????????????????????????????????????????????????????????????????????????????? */
    if (rc->wt(subi) != rc->wt(superi))
      return FALSE;
    break;
  }
  case INSTR_CELLDUMP: {
    printf("CELL DUMP:\n");
    lmn_dump_cell_stdout(rc->get_global_root());
    lmn_hyperlink_print(rc->get_global_root());
    break;
  }
  default:
    fprintf(stderr, "interpret: Unknown operation %d\n", op);
    exit(1);
  }

  stop = false;
  return false;
}

bool slim::vm::interpreter::run() {
  bool result;
  do {
    // ??????????????????????????????????????????????????????
    bool stop = false;
    do {
      result = exec_command(this->rc, this->rule, stop);
    } while (!stop);

    // ???????????????????????????stack frame?????????????????????callback???????????????
    while (!this->callstack.empty()) {
      auto &s = this->callstack.back();
      auto r = s.callback(*this, result);

      if (r == command_result::Success) {
        result = true;
        this->callstack.pop_back();
      } else if (r == command_result::Failure) {
        result = false;
        this->callstack.pop_back();
      } else if (r == command_result::Trial) {
        // ????????????????????????????????????????????????????????????????????????callback???pop?????????
        // (c.f. false_driven_enumerator)
        break;
      }
    }
  } while (!this->callstack.empty());

  return result;
}

bool slim::vm::interpreter::interpret(LmnReactCxt *rc, LmnRuleRef rule,
                                      LmnRuleInstr instr) {
  auto trc = this->rc;
  auto trule = this->rule;
  auto tinstr = this->instr;
  std::vector<stack_frame> tstack;
  this->rc = rc;
  this->rule = rule;
  this->instr = instr;
  tstack.swap(this->callstack);
  bool result = run();
  this->rc = trc;
  this->rule = trule;
  this->instr = tinstr;
  this->callstack = tstack;
  return result;
}

static BOOL dmem_interpret(LmnReactCxtRef rc, LmnRuleRef rule,
                           LmnRuleInstr instr) {
  /*   LmnRuleInstr start = instr; */
  LmnInstrOp op;

  while (TRUE) {
    READ_VAL(LmnInstrOp, instr, op);
    /*     fprintf(stdout, "op: %d %d\n", op, (instr - start)); */
    /*     lmn_dump_mem(LmnMembraneRef)wt(rc, 0)); */
    switch (op) {
    case INSTR_SPEC: {
      LmnInstrVar s0;

      SKIP_VAL(LmnInstrVar, instr);
      READ_VAL(LmnInstrVar, instr, s0);

      rc->resize(s0);
      break;
    }
    case INSTR_INSERTCONNECTORSINNULL: {
      LmnInstrVar seti, list_num;
      Vector links;
      unsigned int i;

      READ_VAL(LmnInstrVar, instr, seti);
      READ_VAL(LmnInstrVar, instr, list_num);

      links.init(list_num + 1);
      for (i = 0; i < list_num; i++) {
        LmnInstrVar t;
        READ_VAL(LmnInstrVar, instr, t);
        links.push((LmnWord)t);
      }

      rc->reg(seti) = {(LmnWord)insertconnectors(rc, NULL, &links), 0,
                       TT_OTHER};
      links.destroy();

      /* EFFICIENCY: ???????????????????????? */
      if (dmem_interpret(rc, rule, instr)) {
        delete (HashSet *)rc->wt(seti);
        return TRUE;
      } else {
        LMN_ASSERT(0);
      }
      break;
    }
    case INSTR_INSERTCONNECTORS: {
      LmnInstrVar seti, list_num, memi, enti;
      Vector links; /* src list */
      unsigned int i;

      READ_VAL(LmnInstrVar, instr, seti);
      READ_VAL(LmnInstrVar, instr, list_num);

      links.init(list_num + 1);

      for (i = 0; i < list_num; i++) {
        READ_VAL(LmnInstrVar, instr, enti);
        links.push((LmnWord)enti);
      }

      READ_VAL(LmnInstrVar, instr, memi);

      rc->reg(seti) = {
          (LmnWord)insertconnectors(rc, (LmnMembraneRef)rc->wt(memi), &links),
          0, TT_OTHER};
      links.destroy();

      /* EFFICIENCY: ???????????????????????? */
      if (dmem_interpret(rc, rule, instr)) {
        delete (HashSet *)rc->wt(seti);
        return TRUE;
      } else {
        LMN_ASSERT(0);
      }
      break;
    }
    case INSTR_NEWATOM: {
      LmnInstrVar atomi, memi;
      LmnAtomRef ap;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnLinkAttr, instr, attr);
      if (LMN_ATTR_IS_DATA(attr)) {
        READ_DATA_ATOM(ap, attr);
      } else { /* symbol atom */
        LmnFunctor f;

        READ_VAL(LmnFunctor, instr, f);
        ap = (LmnAtomRef)dmem_root_new_atom(RC_ND_MEM_DELTA_ROOT(rc), f);
      }

      RC_ND_MEM_DELTA_ROOT(rc)->push_atom((LmnMembraneRef)rc->wt(memi),
                                          (LmnAtomRef)ap, attr);

      rc->reg(atomi) = {
          (LmnWord)ap, attr,
          TT_OTHER}; /* BODY??????????????????????????????????????????????????????->TT_OTHER */
      break;
    }
    case INSTR_COPYATOM: {
      LmnInstrVar atom1, memi, atom2;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, atom2);

      rc->reg(atom1) = {(LmnWord)dmem_root_copy_atom(RC_ND_MEM_DELTA_ROOT(rc),
                                                     (LmnAtomRef)rc->wt(atom2),
                                                     rc->at(atom2)),
                        rc->at(atom2), TT_OTHER};
      RC_ND_MEM_DELTA_ROOT(rc)->push_atom((LmnMembraneRef)rc->wt(memi),
                                          (LmnAtomRef)rc->wt(atom1),
                                          rc->at(atom1));
      break;
    }
    case INSTR_ALLOCLINK: {
      LmnInstrVar link, atom, n;

      READ_VAL(LmnInstrVar, instr, link);
      READ_VAL(LmnInstrVar, instr, atom);
      READ_VAL(LmnInstrVar, instr, n);

      if (LMN_ATTR_IS_DATA(rc->at(atom))) {
        rc->wt(link) = rc->wt(atom);
        rc->at(link) = rc->at(atom);
      } else { /* link to atom */
        rc->wt(link) = (LmnWord)LMN_SATOM(rc->wt(atom));
        rc->at(link) = LMN_ATTR_MAKE_LINK(n);
      }
      rc->tt(link) = TT_OTHER;
      break;
    }
    case INSTR_UNIFYLINKS: {
      LmnInstrVar link1, link2, mem;

      READ_VAL(LmnInstrVar, instr, link1);
      READ_VAL(LmnInstrVar, instr, link2);
      READ_VAL(LmnInstrVar, instr, mem);

      if (LMN_ATTR_IS_DATA(rc->at(link1))) {
        if (LMN_ATTR_IS_DATA(rc->at(link2))) { /* 1, 2 are data */
          RC_ND_MEM_DELTA_ROOT(rc)->link_data_atoms(
              (LmnMembraneRef)rc->wt(mem), (LmnDataAtomRef)rc->wt(link1),
              rc->at(link1), (LmnDataAtomRef)rc->wt(link2),
              rc->at(link2));
        } else { /* 1 is data */
          RC_ND_MEM_DELTA_ROOT(rc)->unify_links(
              (LmnMembraneRef)rc->wt(mem), (LmnAtomRef)rc->wt(link2),
              rc->at(link2), (LmnAtomRef)rc->wt(link1), rc->at(link1));
        }
      } else { /* 2 is data or 1, 2 are symbol atom */
        RC_ND_MEM_DELTA_ROOT(rc)->unify_links(
            (LmnMembraneRef)rc->wt(mem), (LmnAtomRef)rc->wt(link1), rc->at(link1),
            (LmnAtomRef)rc->wt(link2), rc->at(link2));
      }
      break;
    }
    case INSTR_NEWLINK: {
      LmnInstrVar atom1, atom2, pos1, pos2, memi;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos2);
      READ_VAL(LmnInstrVar, instr, memi);

      RC_ND_MEM_DELTA_ROOT(rc)->newlink(
          (LmnMembraneRef)rc->wt(memi), (LmnAtomRef)rc->wt(atom1),
          rc->at(atom1), pos1, (LmnAtomRef)rc->wt(atom2), rc->at(atom2), pos2);
      break;
    }
    case INSTR_RELINK: {
      LmnInstrVar atom1, atom2, pos1, pos2, memi;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos2);
      READ_VAL(LmnInstrVar, instr, memi);

      RC_ND_MEM_DELTA_ROOT(rc)->relink(
          (LmnMembraneRef)rc->wt(memi), (LmnAtomRef)rc->wt(atom1),
          rc->at(atom1), pos1, (LmnAtomRef)rc->wt(atom2), rc->at(atom2), pos2);
      break;
    }
    case INSTR_GETLINK: {
      LmnInstrVar linki, atomi, posi;
      READ_VAL(LmnInstrVar, instr, linki);
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, posi);

      rc->wt(linki) = (LmnWord)dmem_root_get_link(
          RC_ND_MEM_DELTA_ROOT(rc), (LmnSymbolAtomRef)rc->wt(atomi), posi);
      rc->at(linki) =
          (LmnWord)((LmnSymbolAtomRef)rc->wt(atomi))->get_attr(posi);
      rc->tt(linki) = TT_OTHER;
      break;
    }
    case INSTR_UNIFY: {
      LmnInstrVar atom1, pos1, atom2, pos2, memi;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos2);
      READ_VAL(LmnInstrVar, instr, memi);

      RC_ND_MEM_DELTA_ROOT(rc)->unify_atom_args(
          (LmnMembraneRef)rc->wt(memi), (LmnSymbolAtomRef)rc->wt(atom1), pos1,
          (LmnSymbolAtomRef)rc->wt(atom2), pos2);
      break;
    }
    case INSTR_PROCEED:
      return TRUE;
    case INSTR_STOP:
      return FALSE;
    case INSTR_ENQUEUEATOM: {
      SKIP_VAL(LmnInstrVar, instr);
      /* do nothing */
      break;
    }
    case INSTR_DEQUEUEATOM: {
      SKIP_VAL(LmnInstrVar, instr);
      /* do nothing */
      break;
    }
    case INSTR_NEWMEM: {
      LmnInstrVar newmemi, parentmemi;
      LmnMembraneRef mp;

      READ_VAL(LmnInstrVar, instr, newmemi);
      READ_VAL(LmnInstrVar, instr, parentmemi);
      SKIP_VAL(LmnInstrVar, instr);

      mp = dmem_root_new_mem(RC_ND_MEM_DELTA_ROOT(rc)); /*lmn_new_mem(memf);*/
      dmem_root_add_child_mem(RC_ND_MEM_DELTA_ROOT(rc),
                              (LmnMembraneRef)rc->wt(parentmemi), mp);
      rc->wt(newmemi) = (LmnWord)mp;
      rc->tt(newmemi) = TT_OTHER;
      mp->set_active(TRUE);
      if (rc->has_mode(REACT_MEM_ORIENTED)) {
        ((MemReactContext *)rc)->memstack_push(mp);
      }
      break;
    }
    case INSTR_ALLOCMEM: {
      LmnInstrVar dstmemi;

      READ_VAL(LmnInstrVar, instr, dstmemi);

      rc->wt(dstmemi) = (LmnWord)dmem_root_new_mem(RC_ND_MEM_DELTA_ROOT(rc));
      rc->tt(dstmemi) = TT_OTHER;
      break;
    }
    case INSTR_REMOVEATOM: {
      LmnInstrVar atomi, memi;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, memi);

      RC_ND_MEM_DELTA_ROOT(rc)->remove_atom((LmnMembraneRef)rc->wt(memi),
                                            (LmnAtomRef)rc->wt(atomi),
                                            rc->at(atomi));
      break;
    }
    case INSTR_FREEATOM: {
      LmnInstrVar atomi;

      READ_VAL(LmnInstrVar, instr, atomi);

      dmem_root_free_atom(RC_ND_MEM_DELTA_ROOT(rc), (LmnAtomRef)rc->wt(atomi),
                          rc->at(atomi));
      break;
    }
    case INSTR_REMOVEMEM: {
      LmnInstrVar memi, parenti;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, parenti);

      dmem_root_remove_mem(RC_ND_MEM_DELTA_ROOT(rc),
                           (LmnMembraneRef)rc->wt(parenti),
                           (LmnMembraneRef)rc->wt(memi));
      break;
    }
    case INSTR_FREEMEM: {
      LmnInstrVar memi;
      LmnMembraneRef mp;

      READ_VAL(LmnInstrVar, instr, memi);

      mp = (LmnMembraneRef)rc->wt(memi);
      /*       lmn_mem_free(mp); */
      break;
    }
    case INSTR_ADDMEM: {
      LmnInstrVar dstmem, srcmem;

      READ_VAL(LmnInstrVar, instr, dstmem);
      READ_VAL(LmnInstrVar, instr, srcmem);

      //      LMN_ASSERT(!((LmnMembraneRef)rc->wt( srcmem))->parent);

      //      lmn_mem_add_child_mem((LmnMembraneRef)rc->wt( dstmem),
      //      (LmnMembraneRef)rc->wt( srcmem));
      dmem_root_add_child_mem(RC_ND_MEM_DELTA_ROOT(rc),
                              (LmnMembraneRef)rc->wt(dstmem),
                              (LmnMembraneRef)rc->wt(srcmem));
      break;

      break;
    }
    case INSTR_ENQUEUEMEM: {
      SKIP_VAL(LmnInstrVar, instr);
      //      if (rc->has_mode(REACT_ND) && !rc->is_zerostep)
      //      {
      //        lmn_mem_activate_ancestors((LmnMembraneRef)rc->wt( memi)); /* MC
      //        */
      //      }
      /* ??????????????????dmem_interpret???????????????????????????????????????????????????.
       * ??????,
       * ???????????????dmem??????????????????interactive???????????????????????????????????????????????????
       */
      //      if (rc->has_mode(REACT_MEM_ORIENTED)) {
      //        ((MemReactContext *)rc)->memstack_push(
      //        (LmnMembraneRef)rc->wt( memi)); /* ??????????????? */
      //      }
      break;
    }
    case INSTR_UNLOCKMEM: {
      SKIP_VAL(LmnInstrVar, instr);
      /* do nothing */
      break;
    }
    case INSTR_LOADRULESET: {
      LmnInstrVar memi;
      LmnRulesetId id;
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnRulesetId, instr, id);

      lmn_mem_add_ruleset((LmnMembraneRef)rc->wt(memi),
                          LmnRuleSetTable::at(id));
      break;
    }
    case INSTR_LOADMODULE: {
      LmnInstrVar memi;
      lmn_interned_str module_name_id;
      LmnRuleSetRef ruleset;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(lmn_interned_str, instr, module_name_id);

      if ((ruleset = lmn_get_module_ruleset(module_name_id))) {
        /* ??????????????????????????????????????????????????? */
        lmn_mem_add_ruleset((LmnMembraneRef)rc->wt(memi), ruleset);
      } else {
        /* ??????????????????????????????????????????????????? */
        fprintf(stderr, "Undefined module %s\n",
                lmn_id_to_name(module_name_id));
      }
      break;
    }
    case INSTR_RECURSIVELOCK: {
      SKIP_VAL(LmnInstrVar, instr);
      /* do nothing */
      break;
    }
    case INSTR_RECURSIVEUNLOCK: {
      SKIP_VAL(LmnInstrVar, instr);
      /* do nothing */
      break;
    }
    case INSTR_COPYGROUND: {
      LmnInstrVar dstlist, srclist, memi;
      Vector *srcvec, *dstlovec, *retvec; /* ???????????????????????? */
      ProcessTableRef atommap;

      READ_VAL(LmnInstrVar, instr, dstlist);
      READ_VAL(LmnInstrVar, instr, srclist);
      READ_VAL(LmnInstrVar, instr, memi);

      /* ???????????????????????????????????????????????? */
      srcvec = links_from_idxs((Vector *)rc->wt(srclist), rc);

      dmem_root_copy_ground(RC_ND_MEM_DELTA_ROOT(rc),
                            (LmnMembraneRef)rc->wt(memi), srcvec, &dstlovec,
                            &atommap);
      free_links(srcvec);

      /* ?????????????????? */
      retvec = new Vector(2);
      retvec->push((LmnWord)dstlovec);
      retvec->push((LmnWord)atommap);
      rc->reg(dstlist) = {(LmnWord)retvec, LIST_AND_MAP, TT_OTHER};

      /* ???????????????????????????????????????????????????????????????????????????????????? */
      dmem_interpret(rc, rule, instr);

      free_links(dstlovec);
      delete retvec;

      return TRUE; /* COPYGROUND??????????????????????????? */
    }
    case INSTR_REMOVEGROUND:
    case INSTR_FREEGROUND: {
      LmnInstrVar listi, memi;
      Vector *srcvec; /* ???????????????????????? */

      memi = 0; /* warning???????????? */
      READ_VAL(LmnInstrVar, instr, listi);
      if (INSTR_REMOVEGROUND == op) {
        READ_VAL(LmnInstrVar, instr, memi);
      }

      srcvec = links_from_idxs((Vector *)rc->wt(listi), rc);

      switch (op) {
      case INSTR_REMOVEGROUND:
        dmem_root_remove_ground(RC_ND_MEM_DELTA_ROOT(rc),
                                (LmnMembraneRef)rc->wt(memi), srcvec);
        break;
      case INSTR_FREEGROUND:
        /* mem????????????????????????free?????????????????? */
        //         dmem_root_free_ground(RC_ND_MEM_DELTA_ROOT(rc), srcvec);
        break;
      }

      free_links(srcvec);

      break;
    }
    case INSTR_NEWLIST: {
      LmnInstrVar listi;
      Vector *listvec = new Vector(16);
      READ_VAL(LmnInstrVar, instr, listi);
      rc->reg(listi) = {(LmnWord)listvec, 0, TT_OTHER};

      if (dmem_interpret(rc, rule, instr)) {
        delete listvec;
        return TRUE;
      } else {
        delete listvec;
        return FALSE;
      }
      break;
    }
    case INSTR_ADDTOLIST: {
      LmnInstrVar listi, linki;
      READ_VAL(LmnInstrVar, instr, listi);
      READ_VAL(LmnInstrVar, instr, linki);
      ((Vector *)rc->wt(listi))->push(linki);
      break;
    }
    case INSTR_GETFROMLIST: {
      LmnInstrVar dsti, listi, posi;
      READ_VAL(LmnInstrVar, instr, dsti);
      READ_VAL(LmnInstrVar, instr, listi);
      READ_VAL(LmnInstrVar, instr, posi);

      switch (rc->at(listi)) {
      case LIST_AND_MAP:

        if (posi == 0) {
          rc->reg(dsti) = {((Vector *)rc->wt(listi))->get((unsigned int)posi),
                           LINK_LIST, TT_OTHER};
        } else if (posi == 1) {
          rc->reg(dsti) = {((Vector *)rc->wt(listi))->get((unsigned int)posi),
                           MAP, TT_OTHER};
        } else {
          LMN_ASSERT(0);
        }
        break;
      case LINK_LIST: /* LinkObj???free????????????????????? */
      {
        LinkObjRef lo =
            (LinkObjRef)((Vector *)rc->wt(listi))->get((unsigned int)posi);
        rc->wt(dsti) = (LmnWord)LinkObjGetAtom(lo);
        rc->at(dsti) = LinkObjGetPos(lo);
        break;
      }
      default:
        lmn_fatal("unexpected.");
        break;
      }
      break;
    }
    case INSTR_ALLOCATOM: {
      LmnInstrVar atomi;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnLinkAttr, instr, attr);

      rc->at(atomi) = attr;
      if (LMN_ATTR_IS_DATA(attr)) {
        LmnWord w;
        LmnByte a = rc->at(atomi), t;
        READ_CONST_DATA_ATOM(w, a, t);
        rc->reg(atomi) = {w, a, t};
      } else { /* symbol atom */
        LmnFunctor f;
        /*         fprintf(stderr, "symbol atom can't be created in GUARD\n");
         */
        /*         exit(EXIT_FAILURE); */
        READ_VAL(LmnFunctor, instr, f);

        /* ?????????allocatom?????????????????????????????????????????????????????????????????????????????????????????????
         */
        rc->wt(atomi) = f;
      }
      rc->tt(atomi) = TT_OTHER;
      break;
    }
    case INSTR_ALLOCATOMINDIRECT: {
      LmnInstrVar atomi;
      LmnInstrVar srcatomi;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, srcatomi);

      if (LMN_ATTR_IS_DATA(rc->at(srcatomi))) {
        rc->reg(atomi) = {
            lmn_copy_data_atom(rc->wt(srcatomi), rc->at(srcatomi)),
            rc->at(srcatomi), TT_OTHER};
      } else { /* symbol atom */
        fprintf(stderr, "symbol atom can't be created in GUARD\n");
        exit(EXIT_FAILURE);
      }
      break;
    }
    case INSTR_GETFUNC: {
      LmnInstrVar funci, atomi;

      READ_VAL(LmnInstrVar, instr, funci);
      READ_VAL(LmnInstrVar, instr, atomi);

      if (LMN_ATTR_IS_DATA(rc->at(atomi))) {
        /* ?????????????????????????????????????????????????????????????????????????????????????????????
           double ????????????????????????????????????????????? */
        rc->wt(funci) = rc->wt(atomi);
      } else {
        rc->wt(funci) = ((LmnSymbolAtomRef)rc->wt(atomi))->get_functor();
      }
      rc->at(funci) = rc->at(atomi);
      rc->tt(funci) = TT_OTHER;
      break;
    }
    case INSTR_SETMEMNAME: {
      LmnInstrVar memi;
      lmn_interned_str name;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(lmn_interned_str, instr, name);
      dmem_root_set_mem_name(RC_ND_MEM_DELTA_ROOT(rc),
                             (LmnMembraneRef)rc->wt(memi), name);
      break;
    }
    case INSTR_COPYRULES: {
      LmnInstrVar destmemi, srcmemi;

      READ_VAL(LmnInstrVar, instr, destmemi);
      READ_VAL(LmnInstrVar, instr, srcmemi);

      dmem_root_copy_rules(RC_ND_MEM_DELTA_ROOT(rc),
                           (LmnMembraneRef)rc->wt(destmemi),
                           (LmnMembraneRef)rc->wt(srcmemi));
      break;
    }
    case INSTR_REMOVEPROXIES: {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      dmem_root_remove_proxies(RC_ND_MEM_DELTA_ROOT(rc),
                               (LmnMembraneRef)rc->wt(memi));
      break;
    }
    case INSTR_INSERTPROXIES: {
      LmnInstrVar parentmemi, childmemi;

      READ_VAL(LmnInstrVar, instr, parentmemi);
      READ_VAL(LmnInstrVar, instr, childmemi);
      dmem_root_insert_proxies(RC_ND_MEM_DELTA_ROOT(rc),
                               (LmnMembraneRef)rc->wt(parentmemi),
                               (LmnMembraneRef)rc->wt(childmemi));
      break;
    }
    case INSTR_DELETECONNECTORS: {
      LmnInstrVar srcset, srcmap;
      HashSet *delset;
      ProcessTableRef delmap;
      HashSetIterator it;
      READ_VAL(LmnInstrVar, instr, srcset);
      READ_VAL(LmnInstrVar, instr, srcmap);

      delset = (HashSet *)rc->wt(srcset);
      delmap = (ProcessTableRef)rc->wt(srcmap);

      for (it = hashset_iterator(delset); !hashsetiter_isend(&it);
           hashsetiter_next(&it)) {
        LmnSymbolAtomRef orig, copy;
        LmnWord t;

        orig = (LmnSymbolAtomRef)hashsetiter_entry(&it);
        t = 0; /* warning???????????? */
        proc_tbl_get_by_atom(delmap, orig, &t);
        copy = (LmnSymbolAtomRef)t;
        lmn_mem_unify_symbol_atom_args(orig, 0, orig, 1);
        lmn_mem_unify_symbol_atom_args(copy, 0, copy, 1);

        lmn_delete_atom(orig);
        lmn_delete_atom(copy);
      }

      if (delmap)
        delete delmap;
      break;
    }
    case INSTR_REMOVETOPLEVELPROXIES: {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      dmem_root_remove_toplevel_proxies(RC_ND_MEM_DELTA_ROOT(rc),
                                        (LmnMembraneRef)rc->wt(memi));
      break;
    }
    case INSTR_ADDATOM: {
      LmnInstrVar memi, atomi;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, atomi);
      RC_ND_MEM_DELTA_ROOT(rc)->push_atom((LmnMembraneRef)rc->wt(memi),
                                          (LmnAtomRef)rc->wt(atomi),
                                          rc->at(atomi));
      break;
    }
    case INSTR_MOVECELLS: {
      LmnInstrVar destmemi, srcmemi;

      READ_VAL(LmnInstrVar, instr, destmemi);
      READ_VAL(LmnInstrVar, instr, srcmemi);
      LMN_ASSERT(rc->wt(destmemi) != rc->wt(srcmemi));
      RC_ND_MEM_DELTA_ROOT(rc)->move_cells((LmnMembraneRef)rc->wt(destmemi),
                                           (LmnMembraneRef)rc->wt(srcmemi));
      break;
    }
    case INSTR_REMOVETEMPORARYPROXIES: {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      dmem_root_remove_temporary_proxies(RC_ND_MEM_DELTA_ROOT(rc),
                                         (LmnMembraneRef)rc->wt(memi));
      break;
    }
    case INSTR_COPYCELLS: {
      LmnInstrVar mapi, destmemi, srcmemi;

      READ_VAL(LmnInstrVar, instr, mapi);
      READ_VAL(LmnInstrVar, instr, destmemi);
      READ_VAL(LmnInstrVar, instr, srcmemi);
      RC_ND_MEM_DELTA_ROOT(rc)->copy_cells((LmnMembraneRef)rc->wt(destmemi),
                                           (LmnMembraneRef)rc->wt(srcmemi));
      rc->tt(mapi) = TT_OTHER;
      break;
    }
    case INSTR_LOOKUPLINK: {
      LmnInstrVar destlinki, tbli, srclinki;

      READ_VAL(LmnInstrVar, instr, destlinki);
      READ_VAL(LmnInstrVar, instr, tbli);
      READ_VAL(LmnInstrVar, instr, srclinki);

      rc->at(destlinki) = rc->at(srclinki);
      if (LMN_ATTR_IS_DATA(rc->at(srclinki))) {
        rc->wt(destlinki) = (LmnWord)rc->wt(srclinki);
      } else { /* symbol atom */
        ProcessTableRef ht = (ProcessTableRef)rc->wt(tbli);
        LmnWord w = rc->wt(destlinki);
        proc_tbl_get_by_atom(ht, (LmnSymbolAtomRef)rc->wt(srclinki), &w);
        rc->wt(destlinki) = w;
      }
      break;
    }
    case INSTR_CLEARRULES: {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      dmem_root_clear_ruleset(RC_ND_MEM_DELTA_ROOT(rc),
                              (LmnMembraneRef)rc->wt(memi));
      ((LmnMembraneRef)rc->wt(memi))->clearrules();

      break;
    }
    case INSTR_DROPMEM: {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      dmem_root_drop(RC_ND_MEM_DELTA_ROOT(rc), (LmnMembraneRef)rc->wt(memi));
      break;
    }
    case INSTR_LOOP: {
      LmnSubInstrSize subinstr_size;
      READ_VAL(LmnSubInstrSize, instr, subinstr_size);

      while (dmem_interpret(rc, rule, instr))
        ;
      instr += subinstr_size;
      break;
    }
    case INSTR_CALLBACK: {
      LmnInstrVar memi, atomi;
      LmnSymbolAtomRef atom;
      const struct CCallback *c;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, atomi);

      atom = (LmnSymbolAtomRef)rc->wt(atomi);

      if (!LMN_ATTR_IS_DATA(atom->get_attr(0))) {
        LmnSymbolAtomRef f_name = (LmnSymbolAtomRef)atom->get_link(0);
        lmn_interned_str name = LMN_FUNCTOR_NAME_ID(lmn_functor_table, f_name->get_functor());
        int arity = LMN_FUNCTOR_ARITY(lmn_functor_table, atom->get_functor());

        c = CCallback::get_ccallback(name);
        if (!c)
          break;

        if (arity - 1 != c->get_arity()) {
          fprintf(stderr, "EXTERNAL FUNC: invalid arity - %s\n",
                  LMN_SYMBOL_STR(name));
          break;
        }

        lmn_mem_delete_atom((LmnMembraneRef)rc->wt(memi),
                            (LmnAtomRef)rc->wt(atomi), rc->at(atomi));
        lmn_mem_delete_atom((LmnMembraneRef)rc->wt(memi), atom->get_link(0),
                            atom->get_attr(0));

        switch (arity) {
        case 1:
          ((callback_0)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi));
          break;
        case 2:
          ((callback_1)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                   atom->get_link(1), atom->get_attr(1));
          break;
        case 3:
          ((callback_2)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                   atom->get_link(1), atom->get_attr(1),
                                   atom->get_link(2), atom->get_attr(2));
          break;
        case 4:
          ((callback_3)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                   atom->get_link(1), atom->get_attr(1),
                                   atom->get_link(2), atom->get_attr(2),
                                   atom->get_link(3), atom->get_attr(3));
          break;
        case 5:
          ((callback_4)c->get_f())(rc, (LmnMembraneRef)rc->wt(memi),
                                   atom->get_link(1), atom->get_attr(1),
                                   atom->get_link(2), atom->get_attr(2),
                                   atom->get_link(3), atom->get_attr(3),
                                   atom->get_link(4), atom->get_attr(4));
          break;
        default:
          printf("EXTERNAL FUNCTION: too many arguments\n");
          break;
        }
      }

      break;
    }
    default:
      fprintf(stderr, "interpret: Unknown operation %d\n", op);
      exit(1);
    }
    /*     lmn_dump_mem((LmnMembraneRef)rc->wt( 0)); */
    /*     print_wt(); */

#ifdef DEBUG
    /*     print_wt(); */
#endif
  }
}

Vector *links_from_idxs(const Vector *link_idxs, LmnReactCxtRef rc) {
  unsigned long i;
  Vector *vec = new Vector(16);

  /* ???????????????????????????????????????????????? */
  for (i = 0; i < link_idxs->get_num(); i++) {
    vec_data_t t = link_idxs->get(i);
    LinkObjRef l = LinkObj_make((LmnAtomRef)rc->wt(t), rc->at(t));
    vec->push((LmnWord)l);
  }
  return vec;
}

void free_links(Vector *links) {
  unsigned long i;

  for (i = 0; i < links->get_num(); i++) {
    LMN_FREE(links->get(i));
  }
  delete links;
}

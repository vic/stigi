#include "kanren.h"

#include <stdlib.h>
#include <string.h>

/* ---- Normalized uKanren Terms (Byrd & Friedman + Bezem 1993 termination) ---- */

typedef enum {
  KTERM_VAR = 0,   /* existential logic variable from a given's type params */
  KTERM_RIGID,     /* rigid type parameter of the caller (never unifies with CON) */
  KTERM_CON        /* canonical type/trait constructor with arity args */
} KTermKind;

typedef struct KTerm KTerm;
struct KTerm {
  KTermKind kind;
  uint32_t var_id;
  StigiStr name;
  KTerm **args;
  uint32_t argc;
};

typedef struct {
  uint32_t var_id;
  KTerm *val;
} KBind;

typedef struct {
  KBind *binds;
  uint32_t count;
  uint32_t cap;
} KSubst;

static bool kstr_eq(StigiStr a, StigiStr b) {
  return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

static KTerm *kalloc_term(Arena *arena, KTermKind kind, uint32_t var_id, StigiStr name,
                          uint32_t argc) {
  KTerm *t = arena_alloc(arena, sizeof *t);
  if (t == NULL) return NULL;
  t->kind = kind;
  t->var_id = var_id;
  t->name = name;
  t->argc = argc;
  t->args = argc > 0 ? arena_alloc(arena, (size_t)argc * sizeof *t->args) : NULL;
  if (argc > 0 && t->args == NULL) return NULL;
  return t;
}

static KTerm *kwalk(const KSubst *s, KTerm *t) {
  uint32_t hops = 0;
  while (t != NULL && t->kind == KTERM_VAR && hops < 256U) {
    bool found = false;
    for (uint32_t i = s->count; i > 0; i--) {
      if (s->binds[i - 1].var_id == t->var_id) {
        t = s->binds[i - 1].val;
        found = true;
        break;
      }
    }
    if (!found) break;
    hops++;
  }
  return t;
}

static KTerm *kwalk_deep(const KSubst *s, KTerm *t, Arena *arena, uint32_t depth) {
  if (t == NULL || depth > 32U) return t;
  t = kwalk(s, t);
  if (t == NULL || t->kind != KTERM_CON || t->argc == 0) return t;
  KTerm *nt = kalloc_term(arena, KTERM_CON, 0, t->name, t->argc);
  if (nt == NULL) return t;
  for (uint32_t i = 0; i < t->argc; i++) {
    nt->args[i] = kwalk_deep(s, t->args[i], arena, depth + 1U);
  }
  return nt;
}

static bool koccurs(const KSubst *s, uint32_t var_id, KTerm *t, uint32_t depth) {
  if (t == NULL || depth > 32U) return false;
  t = kwalk(s, t);
  if (t == NULL) return false;
  if (t->kind == KTERM_VAR) return t->var_id == var_id;
  if (t->kind == KTERM_CON) {
    for (uint32_t i = 0; i < t->argc; i++) {
      if (koccurs(s, var_id, t->args[i], depth + 1U)) return true;
    }
  }
  return false;
}

static bool ksubst_push(KSubst *s, uint32_t var_id, KTerm *val) {
  if (s->count == s->cap) {
    uint32_t ncap = s->cap == 0 ? 16U : s->cap * 2U;
    KBind *nb = realloc(s->binds, (size_t)ncap * sizeof *nb);
    if (nb == NULL) return false;
    s->binds = nb;
    s->cap = ncap;
  }
  s->binds[s->count].var_id = var_id;
  s->binds[s->count].val = val;
  s->count++;
  return true;
}

static bool kunify(KSubst *s, KTerm *a, KTerm *b, uint32_t depth) {
  if (depth > 32U) return false;
  a = kwalk(s, a);
  b = kwalk(s, b);
  if (a == NULL || b == NULL) return false;
  if (a->kind == KTERM_VAR && b->kind == KTERM_VAR && a->var_id == b->var_id) return true;
  if (a->kind == KTERM_VAR) {
    if (koccurs(s, a->var_id, b, 0)) return false;
    return ksubst_push(s, a->var_id, b);
  }
  if (b->kind == KTERM_VAR) {
    if (koccurs(s, b->var_id, a, 0)) return false;
    return ksubst_push(s, b->var_id, a);
  }
  if (a->kind == KTERM_RIGID && b->kind == KTERM_RIGID) {
    return kstr_eq(a->name, b->name);
  }
  if (a->kind == KTERM_CON && b->kind == KTERM_CON) {
    if (a->argc != b->argc || !kstr_eq(a->name, b->name)) return false;
    for (uint32_t i = 0; i < a->argc; i++) {
      if (!kunify(s, a->args[i], b->args[i], depth + 1U)) return false;
    }
    return true;
  }
  return false;
}

static bool kterm_equal(const KTerm *a, const KTerm *b, uint32_t depth) {
  if (a == NULL || b == NULL || depth > 32U) return false;
  if (a->kind != b->kind) return false;
  if (a->kind == KTERM_VAR) return a->var_id == b->var_id;
  if (a->kind == KTERM_RIGID) return kstr_eq(a->name, b->name);
  if (a->argc != b->argc || !kstr_eq(a->name, b->name)) return false;
  for (uint32_t i = 0; i < a->argc; i++) {
    if (!kterm_equal(a->args[i], b->args[i], depth + 1U)) return false;
  }
  return true;
}

static uint32_t kterm_size(const KTerm *t, uint32_t depth) {
  if (t == NULL || depth > 32U) return 1U;
  if (t->kind != KTERM_CON) return 1U;
  uint32_t sz = 1U;
  for (uint32_t i = 0; i < t->argc; i++) {
    sz += kterm_size(t->args[i], depth + 1U);
  }
  return sz;
}

/* ---- Convert AST StigiType to Canonical KTerm ---- */

typedef struct {
  StigiStr name;
  uint32_t var_id;
  bool is_rigid;
} KVarEntry;

static KTerm *ast_type_to_kterm(const StigiImage *img, uint32_t from_idx, const StigiType *ty,
                                const KVarEntry *vars, uint32_t var_count,
                                Arena *arena, uint32_t depth) {
  if (ty == NULL || depth > 16U) return NULL;
  if (ty->kind == STIGI_TYPE_NAMED && ty->type_arg_count == 0) {
    for (uint32_t i = 0; i < var_count; i++) {
      if (kstr_eq(vars[i].name, ty->name)) {
        return kalloc_term(arena, vars[i].is_rigid ? KTERM_RIGID : KTERM_VAR,
                           vars[i].var_id, vars[i].name, 0);
      }
    }
    int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
    if (tgt >= 0) {
      const StigiDecl *td = img->units[tgt].ast->decl;
      /* Transparent (fact-free, zero-param) type alias expands to RHS */
      if (td->kind == STIGI_DECL_TYPE && !td->u.type_decl.has_facts &&
          td->u.type_decl.type_param_count == 0 && td->u.type_decl.val_param_count == 0 &&
          td->u.type_decl.cap_param_count == 0 && td->u.type_decl.rhs != NULL) {
        return ast_type_to_kterm(img, (uint32_t)tgt, td->u.type_decl.rhs, NULL, 0, arena, depth + 1U);
      }
      StigiStr cname = { img->units[tgt].name, (uint32_t)img->units[tgt].name_len };
      return kalloc_term(arena, KTERM_CON, 0, cname, 0);
    }
    return kalloc_term(arena, KTERM_CON, 0, ty->name, 0);
  }

  StigiStr cname = ty->name;
  if (ty->kind == STIGI_TYPE_ARRAY) {
    cname = (StigiStr){ "Array", 5 };
  } else if (ty->kind == STIGI_TYPE_CELL) {
    cname = (StigiStr){ "Cell", 4 };
  } else if (ty->kind == STIGI_TYPE_PTR) {
    cname = (StigiStr){ "Ptr", 3 };
  } else {
    int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
    if (tgt >= 0) {
      cname = (StigiStr){ img->units[tgt].name, (uint32_t)img->units[tgt].name_len };
    }
  }
  KTerm *t = kalloc_term(arena, KTERM_CON, 0, cname, ty->type_arg_count);
  if (t == NULL) return NULL;
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    t->args[i] = ast_type_to_kterm(img, from_idx, ty->type_args[i], vars, var_count, arena, depth + 1U);
    if (t->args[i] == NULL) return NULL;
  }
  return t;
}

/* ---- Bounded Relational Search Engine ---- */

typedef struct {
  const StigiImage *img;
  uint32_t from_idx;
  const KTerm **local_goals;
  uint32_t local_count;
  Arena *arena;
  uint32_t next_var_id;
  uint32_t fuel;
  bool hit_fuel;
  const KTerm *stack[STIGI_KANREN_MAX_DEPTH + 1U];
  uint32_t depth;
} KSolver;

static StigiKanrenProof ksolve_goal(KSolver *ks, const KTerm *goal) {
  StigiKanrenProof fail_res;
  memset(&fail_res, 0, sizeof fail_res);
  fail_res.status = STIGI_KANREN_MISSING;
  fail_res.given_uidx = -1;

  if (goal == NULL) return fail_res;
  if (ks->fuel == 0U || ks->depth >= STIGI_KANREN_MAX_DEPTH) {
    ks->hit_fuel = true;
    fail_res.status = STIGI_KANREN_FUEL_EXCEEDED;
    return fail_res;
  }
  ks->fuel--;

  /* Cycle detection on active goal stack (Bezem strong-termination guard) */
  for (uint32_t d = 0; d < ks->depth; d++) {
    if (kterm_equal(ks->stack[d], goal, 0)) {
      ks->hit_fuel = true;
      fail_res.status = STIGI_KANREN_FUEL_EXCEEDED;
      return fail_res;
    }
  }

  /* 1. Check local scope assumptions ({Trait[A]} / {ev: Trait[A]}) first */
  for (uint32_t i = 0; i < ks->local_count; i++) {
    if (ks->local_goals[i] != NULL && kterm_equal(ks->local_goals[i], goal, 0)) {
      StigiKanrenProof ok;
      memset(&ok, 0, sizeof ok);
      ok.status = STIGI_KANREN_LOCAL_EV;
      ok.given_uidx = -1;
      ok.local_ev_idx = i;
      return ok;
    }
  }

  /* 2. Relational search across all visible `given` units in image (check up to 2 solutions) */
  ks->stack[ks->depth++] = goal;
  uint32_t goal_sz = kterm_size(goal, 0);

  uint32_t sol_count = 0;
  StigiKanrenProof best;
  memset(&best, 0, sizeof best);
  best.status = STIGI_KANREN_MISSING;
  best.given_uidx = -1;

  for (uint32_t u = 0; u < ks->img->unit_count && sol_count < 2U; u++) {
    const StigiDecl *gd = ks->img->units[u].ast->decl;
    if (gd->kind != STIGI_DECL_GIVEN) continue;
    if (!stigi_image_visible_from(ks->img, u, ks->from_idx)) continue;
    if (gd->u.given_decl.trait_type == NULL) continue;

    /* Instantiate given's existential type parameters with fresh logic variables */
    uint32_t tpc = gd->u.given_decl.type_param_count;
    KVarEntry *gvars = tpc > 0 ? malloc((size_t)tpc * sizeof *gvars) : NULL;
    if (tpc > 0 && gvars == NULL) continue;
    for (uint32_t p = 0; p < tpc; p++) {
      gvars[p].name = gd->u.given_decl.type_params[p].name;
      gvars[p].var_id = ks->next_var_id++;
      gvars[p].is_rigid = false;
    }

    KTerm *head = ast_type_to_kterm(ks->img, u, gd->u.given_decl.trait_type,
                                    gvars, tpc, ks->arena, 0);
    KSubst subst = {NULL, 0, 0};
    bool unified = (head != NULL) && kunify(&subst, head, (KTerm *)goal, 0);
    if (!unified) {
      free(subst.binds);
      free(gvars);
      continue;
    }

    /* Resolve all trait subgoals of this given's `{...}` clause */
    uint32_t sub_cnt = gd->u.given_decl.cap_param_count;
    StigiKanrenProof *subs = sub_cnt > 0
                                 ? arena_alloc(ks->arena, (size_t)sub_cnt * sizeof *subs)
                                 : NULL;
    bool all_ok = true;
    bool sub_ambig = false;
    bool sub_fuel = false;
    uint32_t sub_w = 0;

    for (uint32_t c = 0; c < sub_cnt && all_ok; c++) {
      const StigiType *ct = gd->u.given_decl.cap_params[c].cap_type;
      if (ct == NULL) continue;
      int32_t ct_uidx = stigi_image_resolve_name(ks->img, u, ct->name.ptr, ct->name.len);
      if (ct_uidx < 0 || ks->img->units[ct_uidx].ast->decl->kind != STIGI_DECL_TRAIT) {
        /* Non-trait capability in given row is rejected by Datalog `given-row`; skip here */
        continue;
      }
      KTerm *raw_sub = ast_type_to_kterm(ks->img, u, ct, gvars, tpc, ks->arena, 0);
      KTerm *sub_goal = kwalk_deep(&subst, raw_sub, ks->arena, 0);
      if (sub_goal == NULL) {
        all_ok = false;
        break;
      }
      /* Bezem level-mapping guard: non-decreasing subgoal burns extra fuel */
      uint32_t sub_sz = kterm_size(sub_goal, 0);
      if (sub_sz >= goal_sz) {
        if (ks->fuel <= 16U) {
          ks->fuel = 0U;
          ks->hit_fuel = true;
          sub_fuel = true;
          all_ok = false;
          break;
        }
        ks->fuel -= 16U;
      }
      StigiKanrenProof sp = ksolve_goal(ks, sub_goal);
      if (sp.status == STIGI_KANREN_FOUND || sp.status == STIGI_KANREN_LOCAL_EV) {
        if (subs != NULL) subs[sub_w++] = sp;
      } else {
        if (sp.status == STIGI_KANREN_AMBIGUOUS) sub_ambig = true;
        if (sp.status == STIGI_KANREN_FUEL_EXCEEDED) sub_fuel = true;
        all_ok = false;
      }
    }

    free(subst.binds);
    free(gvars);

    if (all_ok) {
      sol_count++;
      if (sol_count == 1U) {
        best.status = STIGI_KANREN_FOUND;
        best.given_uidx = (int32_t)u;
        best.subproofs = subs;
        best.subproof_count = sub_w;
      }
    } else if (sub_ambig) {
      ks->depth--;
      fail_res.status = STIGI_KANREN_AMBIGUOUS;
      return fail_res;
    } else if (sub_fuel) {
      ks->hit_fuel = true;
    }
  }

  ks->depth--;

  if (sol_count == 1U) return best;
  if (sol_count >= 2U) {
    fail_res.status = STIGI_KANREN_AMBIGUOUS;
    return fail_res;
  }
  if (ks->hit_fuel) {
    fail_res.status = STIGI_KANREN_FUEL_EXCEEDED;
    return fail_res;
  }
  return fail_res;
}

StigiKanrenProof stigi_kanren_solve(const StigiImage *img, uint32_t from_idx,
                                    const StigiType *goal_type,
                                    const StigiParam *rigid_tparams, uint32_t rigid_count,
                                    const StigiCapParam *local_assumps, uint32_t local_count,
                                    Arena *arena) {
  StigiKanrenProof res;
  memset(&res, 0, sizeof res);
  res.status = STIGI_KANREN_MISSING;
  res.given_uidx = -1;
  if (img == NULL || goal_type == NULL || arena == NULL) return res;

  KVarEntry *rvars = rigid_count > 0 ? malloc((size_t)rigid_count * sizeof *rvars) : NULL;
  if (rigid_count > 0 && rvars == NULL) return res;
  for (uint32_t i = 0; i < rigid_count; i++) {
    rvars[i].name = rigid_tparams[i].name;
    rvars[i].var_id = i + 1U;
    rvars[i].is_rigid = true;
  }

  KTerm *goal = ast_type_to_kterm(img, from_idx, goal_type, rvars, rigid_count, arena, 0);
  const KTerm **lgoals = local_count > 0 ? malloc((size_t)local_count * sizeof *lgoals) : NULL;
  uint32_t lcnt = 0;
  if (lgoals != NULL) {
    for (uint32_t i = 0; i < local_count; i++) {
      if (local_assumps[i].cap_type == NULL) {
        lgoals[lcnt++] = NULL;
        continue;
      }
      int32_t tidx = stigi_image_resolve_name(img, from_idx, local_assumps[i].cap_type->name.ptr,
                                              local_assumps[i].cap_type->name.len);
      if (tidx >= 0 && img->units[tidx].ast->decl->kind == STIGI_DECL_TRAIT) {
        lgoals[lcnt++] = ast_type_to_kterm(img, from_idx, local_assumps[i].cap_type,
                                           rvars, rigid_count, arena, 0);
      } else {
        lgoals[lcnt++] = NULL;
      }
    }
  }

  KSolver ks;
  memset(&ks, 0, sizeof ks);
  ks.img = img;
  ks.from_idx = from_idx;
  ks.local_goals = lgoals;
  ks.local_count = lcnt;
  ks.arena = arena;
  ks.next_var_id = rigid_count + 16U;
  ks.fuel = STIGI_KANREN_FUEL;

  res = ksolve_goal(&ks, goal);

  free(lgoals);
  free(rvars);
  return res;
}

bool stigi_kanren_given_is_effectful(const StigiImage *img, uint32_t given_uidx) {
  if (img == NULL || given_uidx >= img->unit_count) return false;
  const StigiDecl *d = img->units[given_uidx].ast->decl;
  if (d->kind != STIGI_DECL_GIVEN) return false;
  for (uint32_t i = 0; i < d->u.given_decl.cap_param_count; i++) {
    const StigiType *ct = d->u.given_decl.cap_params[i].cap_type;
    if (ct == NULL) continue;
    int32_t tgt = stigi_image_resolve_name(img, given_uidx, ct->name.ptr, ct->name.len);
    if (tgt >= 0 && img->units[tgt].ast->decl->kind == STIGI_DECL_TRAIT) {
      continue;
    }
    /* Anything in a given's {...} that is NOT a trait (e.g. Stigi.Mut, Stigi.Region, Stigi.File,
     * or any user capability) is an effectful capability -> given-row! */
    return true;
  }
  return false;
}

bool stigi_kanren_is_concrete_protocol(const StigiImage *img, uint32_t from_idx,
                                       const StigiType *ty) {
  if (img == NULL || ty == NULL) return false;
  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt < 0) return false;
  StigiDeclKind dk = img->units[tgt].ast->decl->kind;
  return dk == STIGI_DECL_HANDLER || dk == STIGI_DECL_GIVEN;
}

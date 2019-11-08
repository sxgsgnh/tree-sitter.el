/*
 * Copyright (C) 2018, 2019 Karl Otness
 *
 * This file is part of tree-sitter.el.
 *
 * tree-sitter.el is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * tree-sitter.el is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tree-sitter.el. If not, see
 * <https://www.gnu.org/licenses/>.
 */
#include "qcursor.h"
#include "common.h"
#include "language.h"
#include "query.h"
#include "node.h"
#include "point.h"
#include <emacs-module.h>
#include <stdint.h>

static void tsel_qcursor_fin(void *ptr) {
  TSElQueryCursor *cursor = ptr;
  ts_query_cursor_delete(cursor->cursor);
  free(cursor);
}

static const char *tsel_query_cursor_new_doc =
  "Create a new cursor for executing a given query\n"
  "\n"
  "(fn )";
static emacs_value tsel_query_cursor_new(emacs_env *env,
					 __attribute__((unused)) ptrdiff_t nargs,
					 __attribute__((unused)) emacs_value *args,
					 __attribute__((unused)) void *data) {
  TSElQueryCursor *wrapper = malloc(sizeof(TSElQuery));
  TSQueryCursor* qcursor = ts_query_cursor_new();
  if (!wrapper || !qcursor) {
    if (wrapper) {
      free(wrapper);
    }
    if (qcursor) {
      ts_query_cursor_delete(qcursor);
    }
    
    tsel_signal_error(env, "Initialization failed!");
    return tsel_Qnil;
  }
  wrapper->cursor = qcursor;
  emacs_value new_querycursor = env->make_user_ptr(env, &tsel_qcursor_fin, wrapper);
  emacs_value Qts_query_cursor_create = env->intern(env, "tree-sitter-query-cursor--create");
  emacs_value funargs[1] = {new_querycursor};
  emacs_value res = env->funcall(env, Qts_query_cursor_create, 1, funargs);
  if (tsel_pending_nonlocal_exit(env)) {
    tsel_qcursor_fin(wrapper);
    tsel_signal_error(env, "Initialization failed");
    return tsel_Qnil;
  }
  return res;

}

static const char *tsel_query_cursor_exec_doc =
  "Start running a given query on a given node.\n"
  "\n"
  "(fn QCURSOR QUERY NODE)";
static emacs_value tsel_query_cursor_exec(emacs_env *env,
					    __attribute__((unused)) ptrdiff_t nargs,
					    emacs_value *args,
					    __attribute__((unused)) void *data) {
  TSElQueryCursor* qcursor;
  TSElQuery* query;
  TSElNode* node;
  TSEL_SUBR_EXTRACT(qcursor,env,args[0],&qcursor);
  TSEL_SUBR_EXTRACT(query,env,args[1],&query);
  TSEL_SUBR_EXTRACT(node,env,args[2],&node);
  qcursor->node = node;
  ts_query_cursor_exec(qcursor->cursor,query->query,node->node);
  return tsel_Qnil;
}

static const char *tsel_query_cursor_next_capture_doc = "Advance to the next capture of the currently running query.\n"
  "\n"
  "(fn QCURSOR)";
static emacs_value tsel_query_cursor_next_capture(emacs_env *env,
						  __attribute__((unused)) ptrdiff_t nargs,
						  emacs_value *args,
						  __attribute__((unused)) void *data) {
  TSElQueryCursor* qcursor;
  TSEL_SUBR_EXTRACT(qcursor,env,args[0],&qcursor);
  TSQueryMatch match;
  uint32_t index;
  bool result = ts_query_cursor_next_capture(qcursor->cursor,&match,&index);
  if(!result){
    return tsel_Qnil;
  }
  emacs_value node = tsel_node_emacs_move(env,match.captures->node,qcursor->node->tree);
  emacs_value func_args[] = {env->make_integer(env,match.capture_count),
    node,env->make_integer(env,match.id),env->make_integer(env,match.pattern_index)};
  
  emacs_value Qtree_sitter_query_match_create = env->intern(env,"tree-sitter-query-match--create");
  return env->funcall(env,Qtree_sitter_query_match_create,4,func_args);
}

static const char *tsel_query_cursor_next_match_doc = "Get the number of string literals in the query.\n"
  "\n"
  "(fn QCURSOR)";
static emacs_value tsel_query_cursor_next_match(emacs_env *env,
						__attribute__((unused)) ptrdiff_t nargs,
						emacs_value *args,
						__attribute__((unused)) void *data) {
  TSElQueryCursor* qcursor;
  TSEL_SUBR_EXTRACT(qcursor,env,args[0],&qcursor);
  TSQueryMatch match;
  bool result = ts_query_cursor_next_match(qcursor->cursor,&match);
  if(!result){
    return tsel_Qnil;
  }
  emacs_value node = tsel_node_emacs_move(env,match.captures->node,qcursor->node->tree);
  emacs_value func_args[] = {env->make_integer(env,match.capture_count),
    node,env->make_integer(env,match.id),env->make_integer(env,match.pattern_index)};
  
  emacs_value Qtree_sitter_query_match_create = env->intern(env,"tree-sitter-query-match--create");
  return env->funcall(env,Qtree_sitter_query_match_create,4,func_args);
}

static const char *tsel_query_cursor_remove_match_doc = "remove match.\n"
  "\n"
  "(fn QCURSOR ID)";
static emacs_value tsel_query_cursor_remove_match(emacs_env *env,
						  __attribute__((unused)) ptrdiff_t nargs,
						  emacs_value *args,
						  __attribute__((unused)) void *data) {
  TSElQueryCursor* qcursor;
  intmax_t id;
  TSEL_SUBR_EXTRACT(qcursor,env,args[0],&qcursor);
  TSEL_SUBR_EXTRACT(integer,env,args[1],&id);
  ts_query_cursor_remove_match(qcursor->cursor,id);
  return tsel_Qnil;
}

static const char *tsel_query_cursor_set_byte_range_doc = "Set the range of bytes or (row, column) positions in which the query will be executed.\n"
  "\n"
  "(fn QCURSOR START-BYTE END-BYTE)";
static emacs_value tsel_query_cursor_set_byte_range(emacs_env *env,
					    __attribute__((unused)) ptrdiff_t nargs,
					    emacs_value *args,
					    __attribute__((unused)) void *data) {
  TSElQueryCursor* qcursor;
  intmax_t start,end;
  TSEL_SUBR_EXTRACT(qcursor,env,args[0],&qcursor);
  TSEL_SUBR_EXTRACT(integer,env,args[1],&start);
  TSEL_SUBR_EXTRACT(integer,env,args[2],&end);
  ts_query_cursor_set_byte_range(qcursor->cursor,start-1,end-1);
  return tsel_Qnil;
}

static const char *tsel_query_cursor_set_point_range_doc = "Set the range of bytes or (row, column) positions in which the query will be executed.\n"
  "\n"
  "(fn QCURSOR START-POINT END-POINT)";
static emacs_value tsel_query_cursor_set_point_range(emacs_env *env,
						     __attribute__((unused)) ptrdiff_t nargs,
						     emacs_value *args,
						     __attribute__((unused)) void *data) {
  TSElQueryCursor* qcursor;
  TSPoint pt1,pt2;
  TSEL_SUBR_EXTRACT(qcursor,env,args[0],&qcursor);
  TSEL_SUBR_EXTRACT(point,env,args[1],&pt1);
  TSEL_SUBR_EXTRACT(point,env,args[1],&pt2);
  ts_query_cursor_set_point_range(qcursor->cursor,pt1,pt2);
  return tsel_Qnil;
}

static const char *tsel_query_cursor_p_doc = "Return t if OBJECT is a tree-sitter-query-cursor.\n"
  "\n"
  "(fn QCURSOR)";
static emacs_value tsel_query_cursor_p(emacs_env *env,
				       __attribute__((unused)) ptrdiff_t nargs,
				       emacs_value *args,
				       __attribute__((unused)) void *data) {
  if(tsel_qcursor_p(env, args[0])) {
    return tsel_Qt;
  }
  return tsel_Qnil;
}

bool tsel_qcursor_init(emacs_env *env){
  bool function_result = tsel_define_function(env, "tree-sitter-query-cursor-new",
                                              &tsel_query_cursor_new, 0, 0,
                                              tsel_query_cursor_new_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-p",
                                          &tsel_query_cursor_p, 1, 1,
                                          tsel_query_cursor_p_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-next-capture",
                                          &tsel_query_cursor_next_capture, 1, 1,
                                          tsel_query_cursor_next_capture_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-next-match",
                                          &tsel_query_cursor_next_match, 1, 1,
                                          tsel_query_cursor_next_match_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-remove-match",
                                          &tsel_query_cursor_remove_match, 2, 2,
                                          tsel_query_cursor_remove_match_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-exec",
                                          &tsel_query_cursor_exec, 3, 3,
                                          tsel_query_cursor_exec_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-set-byte-range",
                                          &tsel_query_cursor_set_byte_range, 3, 3,
                                          tsel_query_cursor_set_byte_range_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-cursor-set-point-range",
                                          &tsel_query_cursor_set_point_range, 3, 3,
                                          tsel_query_cursor_set_point_range_doc, NULL);
  return function_result;
}

bool tsel_qcursor_p(emacs_env *env, emacs_value obj){
  if(!tsel_check_record_type(env, "tree-sitter-query-cursor", obj, 1)) {
    return false;
  }
  // Get the ptr field
  emacs_value user_ptr;
  if(!tsel_record_get_field(env, obj, 1, &user_ptr)) {
    return false;
  }
  // Make sure it's a user pointer
  emacs_value Quser_ptrp = env->intern(env, "user-ptrp");
  emacs_value args[1] = { user_ptr };
  if(!env->eq(env, env->funcall(env, Quser_ptrp, 1, args), tsel_Qt) ||
     tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  // Check the finalizer
  emacs_finalizer *fin = env->get_user_finalizer(env, user_ptr);
  return !tsel_pending_nonlocal_exit(env) && fin == &tsel_qcursor_fin;
  return true;
}
bool tsel_extract_qcursor(emacs_env *env, emacs_value obj,TSElQueryCursor** cursor){
    if(!tsel_qcursor_p(env, obj)) {
    tsel_signal_wrong_type(env, "tree-sitter-query-cursor-p", obj);
    return false;
  }
  // Get the ptr field
  emacs_value user_ptr;
  if(!tsel_record_get_field(env, obj, 1, &user_ptr)) {
    return false;
  }
  // Get the raw pointer
  TSElQueryCursor *ptr = env->get_user_ptr(env, user_ptr);
  if(tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  *cursor = ptr;
  return true;
}

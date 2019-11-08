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

#include <emacs-module.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "query.h"
#include "common.h"
#include "language.h"
#include <bits/stdint-uintn.h>

static void tsel_query_fin(void *ptr) {
  TSElQuery *query = ptr;
  ts_query_delete(query->query);
  free(query);
}

static const char *tsel_query_new_doc =
    "Create a new QUERY\n"
    "LANG is a `tree-sitter-language-p' object.\n"
    "\n"
    "(fn LANG SOURCE)";
static emacs_value tsel_query_new(emacs_env *env,
                                  __attribute__((unused)) ptrdiff_t nargs,
				  emacs_value *args,
                                  __attribute__((unused)) void *data) {
  TSElLanguage* lang = NULL;
  char* source = NULL;
  TSEL_SUBR_EXTRACT(language,env,args[0],&lang);
  TSEL_SUBR_EXTRACT(string,env,args[1],&source);

  TSQueryError err;
  uint32_t error_offset;
  TSQuery *query = ts_query_new(lang->ptr,source,strlen(source),&error_offset,&err);
  
  TSElQuery *wrapper = malloc(sizeof(TSElQuery));
  if (!wrapper || !query) {
    if (wrapper) {
      free(wrapper);
    }
    
    char error[125] = "";
    if (query) {
      ts_query_delete(query);
      char* errtype[] = {"None","Syntax","NodeType","Field","Capture"};
      sprintf(error," TSQueryError:%s,ErrorOffset:%d.",errtype[err],error_offset);
    }
    char errinfo[512];
    sprintf(errinfo,"Initialization failed!%s",error);
    tsel_signal_error(env, errinfo);
    return tsel_Qnil;
  }
  wrapper->query = query;
  emacs_value new_query = env->make_user_ptr(env, &tsel_query_fin, wrapper);
  emacs_value Qts_query_create = env->intern(env, "tree-sitter-query--create");
  emacs_value funargs[1] = {new_query};
  emacs_value res = env->funcall(env, Qts_query_create, 1, funargs);
  if (tsel_pending_nonlocal_exit(env)) {
    tsel_query_fin(wrapper);
    tsel_signal_error(env, "Initialization failed");
    return tsel_Qnil;
  }
  return res;
}

static const char *tsel_query_capture_count_doc =
    "Get the number of captures in the query.\n"
    "\n"
    "(fn QUERY)";
static emacs_value
tsel_query_capture_count(emacs_env *env,
                         __attribute__((unused)) ptrdiff_t nargs,
                          emacs_value *args,
                         __attribute__((unused)) void *data) {
  TSElQuery* q;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  return env->make_integer(env,ts_query_capture_count(q->query));
}

static const char *tsel_query_capture_name_for_id_doc =
    "Get the name and length of one of the QUERY's captures.\n"
    "\n"
    "(fn QUERY PATTERN-ID)";
static emacs_value
tsel_query_capture_name_for_id(emacs_env *env,
                               __attribute__((unused)) ptrdiff_t nargs,
                               emacs_value *args,
                               __attribute__((unused)) void *data) {
  TSElQuery* q;
  intmax_t patternid;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  TSEL_SUBR_EXTRACT(integer,env,args[1],&patternid);
  uint32_t len;
  const char* capture_name = ts_query_capture_name_for_id(q->query,patternid,&len);
  return env->make_string(env,capture_name,len);
}

static const char *tsel_query_string_count_doc =
    "Get the number of string literals in the query.\n"
    "\n"
    "(fn QUERY)";
static emacs_value
tsel_query_string_count(emacs_env *env,
			__attribute__((unused)) ptrdiff_t nargs,
			emacs_value *args,
                        __attribute__((unused)) void *data) {
  TSElQuery* q;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  return env->make_integer(env,ts_query_string_count(q->query));
}

static const char *tsel_query_string_value_for_id_doc =
    "Get the name and length of one of the query's string literals.\n"
    "\n"
    "(fn QUERY ID)";
static emacs_value
tsel_query_string_value_for_id(emacs_env *env,
                               __attribute__((unused)) ptrdiff_t nargs,
			       emacs_value *args,
                               __attribute__((unused)) void *data) {
  TSElQuery* q;
  intmax_t id;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  TSEL_SUBR_EXTRACT(integer,env,args[1],&id);
  uint32_t len;

  const char* string_name = ts_query_string_value_for_id(q->query,id,&len);
  return env->make_string(env,string_name,len);
}

static const char *tsel_query_disable_capture_doc =
    "Disable a certain capture within a query.\n"
    "\n"
    "(fn QUERY STRING)";
static emacs_value
tsel_query_disable_capture(emacs_env *env,
                         __attribute__((unused)) ptrdiff_t nargs,
                         emacs_value *args,
                         __attribute__((unused)) void *data) {
  TSElQuery* q;
  char* str;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  TSEL_SUBR_EXTRACT(string,env,args[1],&str);
  ts_query_disable_capture(q->query,str,strlen(str));
  return tsel_Qnil;
}

static const char *tsel_query_pattern_count_doc =
    "Get the number of patterns in the query\n"
    "\n"
    "(fn QUERY)";
static emacs_value
tsel_query_pattern_count(emacs_env *env,
                         __attribute__((unused)) ptrdiff_t nargs,
                         emacs_value *args,
                         __attribute__((unused)) void *data) {
  TSElQuery* q;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  return env->make_integer(env,ts_query_pattern_count(q->query));
}

static const char *tsel_query_start_byte_for_pattern_doc =
    "Get the byte offset where the given pattern starts in the query's "
    "source.\n"
    "\n"
    "(fn QUERY ID)";
static emacs_value
tsel_query_start_byte_for_pattern(emacs_env *env,
                                  __attribute__((unused)) ptrdiff_t nargs,
                                  emacs_value *args,
                                  __attribute__((unused)) void *data) {
  TSElQuery* q;
  intmax_t id;
  TSEL_SUBR_EXTRACT(query,env,args[0],&q);
  TSEL_SUBR_EXTRACT(integer,env,args[1],&id);
  uint32_t byte = ts_query_start_byte_for_pattern(q->query,id)
  return env->make_integer(env,byte+1);
}

static const char *tsel_query_p_wrapped_doc = "Return t if OBJECT is a tree-sitter-query.\n"
  "\n"
  "(fn QUERY)";
static emacs_value tsel_query_p_wrapped(emacs_env *env,
					__attribute__((unused)) ptrdiff_t nargs,
					emacs_value *args,
					__attribute__((unused)) void *data) {
  if(tsel_query_p(env, args[0])) {
    return tsel_Qt;
  }

  return tsel_Qnil;
}

bool tsel_query_init(emacs_env *env) {
  bool function_result =
      tsel_define_function(env, "tree-sitter-query-new", &tsel_query_new, 2, 2,
                           tsel_query_new_doc, NULL);
  function_result &=
      tsel_define_function(env, "tree-sitter-query-p", &tsel_query_p_wrapped, 1, 1,
                           tsel_query_p_wrapped_doc, NULL);
  function_result &= tsel_define_function(
      env, "tree-sitter-query-capture-count", &tsel_query_capture_count, 1, 1,
      tsel_query_capture_count_doc, NULL);
  function_result &=
      tsel_define_function(env, "tree-sitter-query-capture-name-for-id",
                           &tsel_query_capture_name_for_id, 2, 2,
                           tsel_query_capture_name_for_id_doc, NULL);
  function_result &=
      tsel_define_function(env, "tree-sitter-query-string-value-for-id",
                           &tsel_query_string_value_for_id, 2, 2,
                           tsel_query_string_value_for_id_doc, NULL);
  function_result &= tsel_define_function(env, "tree-sitter-query-string-count",
                                          &tsel_query_string_count, 1, 1,
                                          tsel_query_string_count_doc, NULL);
  function_result &= tsel_define_function(
      env, "tree-sitter-query-pattern-count", &tsel_query_pattern_count, 1, 1,
      tsel_query_pattern_count_doc, NULL);
  function_result &=
      tsel_define_function(env, "tree-sitter-query-start-byte-for-pattern",
                           &tsel_query_start_byte_for_pattern, 2, 2,
                           tsel_query_start_byte_for_pattern_doc, NULL);
    function_result &=
      tsel_define_function(env, "tree-sitter-disable-capture",
                           &tsel_query_disable_capture, 2, 2,
                           tsel_query_disable_capture_doc, NULL);
  return function_result;
}

bool tsel_query_p(emacs_env *env, emacs_value obj) {
  if (!tsel_check_record_type(env, "tree-sitter-query", obj, 1)) {
    return false;
  }
  // Get the ptr field
  emacs_value user_ptr;
  if (!tsel_record_get_field(env, obj, 1, &user_ptr)) {
    return false;
  }
  // Make sure it's a user pointer
  emacs_value Quser_ptrp = env->intern(env, "user-ptrp");
  emacs_value args[1] = {user_ptr};
  if (!env->eq(env, env->funcall(env, Quser_ptrp, 1, args), tsel_Qt) ||
      tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  // Check the finalizer
  emacs_finalizer *fin = env->get_user_finalizer(env, user_ptr);
  return !tsel_pending_nonlocal_exit(env) && fin == &tsel_query_fin;
  return true;
}

bool tsel_extract_query(emacs_env *env, emacs_value obj, TSElQuery **query) {
  if (!tsel_query_p(env, obj)) {
    tsel_signal_wrong_type(env, "tree-sitter-query-p", obj);
    return false;
  }
  // Get the ptr field
  emacs_value user_ptr;
  if (!tsel_record_get_field(env, obj, 1, &user_ptr)) {
    return false;
  }
  // Get the raw pointer
  TSElQuery *ptr = env->get_user_ptr(env, user_ptr);
  if (tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  *query = ptr;
  return true;
}

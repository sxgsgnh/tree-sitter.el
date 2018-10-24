/*
 * Copyright (C) 2018 Karl Otness
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
#include <string.h>
#include "language.h"
#include "common.h"

bool tsel_language_p(emacs_env *env, emacs_value obj) {
  // Ensure the pointer is wrapped in the proper Emacs structure type
  emacs_value Qts_lang_p = env->intern(env, "tree-sitter-language-p");
  emacs_value args[1] = { obj };
  if(!env->eq(env, env->funcall(env, Qts_lang_p, 1, args), tsel_Qt) ||
     tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  // Extract the "ptr" field and make sure it's a user pointer
  emacs_value Qts_lang_ptr = env->intern(env, "tree-sitter-language-ptr");
  emacs_value user_ptr = env->funcall(env, Qts_lang_ptr, 1, args);
  emacs_value Quser_ptrp = env->intern(env, "user-ptrp");
  args[0] = user_ptr;
  if(!env->eq(env, env->funcall(env, Quser_ptrp, 1, args), tsel_Qt) ||
     tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  // Get the raw pointer
  TSElLanguage *ptr = env->get_user_ptr(env, user_ptr);
  if(ptr == NULL || tsel_pending_nonlocal_exit(env)) {
    return false;
  }
  // Check the type tag in the struct
  return strncmp(ptr->tag, "TSLanguage", 11) == 0;
}

TSElLanguage *tsel_language_get_ptr(emacs_env *env, emacs_value obj) {
  if(!tsel_language_p(env, obj)) {
    return NULL;
  }
  emacs_value Qts_lang_ptr = env->intern(env, "tree-sitter-language-ptr");
  emacs_value args[1] = { obj };
  emacs_value user_ptr = env->funcall(env, Qts_lang_ptr, 1, args);
  TSElLanguage *ptr = env->get_user_ptr(env, user_ptr);
  if(tsel_pending_nonlocal_exit(env)) {
    return NULL;
  }
  return ptr;
}
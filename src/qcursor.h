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
#pragma once
#ifndef TSEL_QCURSOR_H
#define TSEL_QCURSOR_H
#include <stdbool.h>
#include <emacs-module.h>
#include "tree_sitter/api.h"
#include "node.h"

typedef struct TSElQueryCursor{
  TSQueryCursor * cursor;
  TSElNode* node;
}TSElQueryCursor;

bool tsel_qcursor_init(emacs_env *env);
bool tsel_qcursor_p(emacs_env *env, emacs_value obj);
bool tsel_extract_qcursor(emacs_env *env, emacs_value obj,TSElQueryCursor** cursor);

#endif //TSEL_QCURSOR_H

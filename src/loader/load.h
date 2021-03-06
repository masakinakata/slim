/*
 * load.h
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
 * $Id: load.h,v 1.4 2008/09/29 04:47:03 taisuke Exp $
 */

#ifndef LMN_LOAD_H
#define LMN_LOAD_H

/**
 * @ingroup  Loader
 * @defgroup Load
 * @{
 */

#include "syntax.hpp"
#include "vm/vm.h"

#include <memory>
#include <string>
#include <cstdio>

LmnRuleSetRef load(std::unique_ptr<FILE, decltype(&fclose)> in);
std::unique_ptr<LmnRule> load_rule(const Rule &rule);
LmnRuleSetRef load_file(const std::string &file_name);
void load_il_files(const char *path);
std::unique_ptr<Rule> il_parse_rule(std::unique_ptr<FILE, decltype(&fclose)> in);
void init_so_handles();
void finalize_so_handles();
/* path???so??????????????????,??????????????????????????????????????????????????? */
/* ??????????????????(_???)O(???????????????,???????????????????????????)??????????????? */
std::string create_formatted_basename(const std::string &path);

/* ?????????????????????????????? */
#define OPTIMIZE_LEVEL_MAX 3

/* @} */

#endif /* LMN_MEMBRANE_H */

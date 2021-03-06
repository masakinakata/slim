/*
 * util.c - common utility functions and macros
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

#include "util.h"
#include "../lmntal.h"
#include "error.h"

char *int_to_str(long n) {
  char *s;
  int keta = 0;

  if (n == 0)
    keta = 1;
  else {
    int m = n;
    keta = 0;
    if (m < 0) {
      m = -m, keta = 1;
    }
    while (m > 0) {
      m /= 10;
      keta++;
    }
  }

  s = LMN_NALLOC(char, keta + 1);
  sprintf(s, "%ld", n);

  return s;
}

/* ???????????????int???????????????????????????*/
int comp_int_f(const void *a_, const void *b_) {
  int a = *(int *)a_;
  int b = *(int *)b_;
  return a > b ? 1 : (a == b ? 0 : -1);
}

/* ???????????????int???????????????????????????*/
int comp_int_greater_f(const void *a_, const void *b_) {
  int a = *(int *)a_;
  int b = *(int *)b_;
  return a > b ? -1 : (a == b ? 0 : 1);
}

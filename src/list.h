/*
    check_mutti - aggregate Nagios checks
    Copyright (C) 2009  Mark A. Meyer

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _list_h_
#define _list_h_

#include "error.h"

#define MAX_STR 320

typedef struct check check_t;

typedef struct str_cons {
  char *elem;
  struct str_cons *rest;
} str_list_t;

typedef struct chk_cons {
  check_t *check;
  struct chk_cons *rest;
} chk_list_t;

/*@null@*/ char*
strl_head(str_list_t *l);

check_t*
chkl_head(chk_list_t *l);

/*@null@*/ str_list_t*
strl_rest(str_list_t *l);

check_t*
chkl_rest(chk_list_t *l);

str_list_t*
strl_cons(str_list_t /*@null@*/ *l, char* elem);

chk_list_t*
chkl_cons(chk_list_t *l, check_t* elem);

void
strl_free(str_list_t *l);

void
chkl_free(chk_list_t *l);

#endif /* _list_h_ */

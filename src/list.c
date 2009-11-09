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

#include "list.h"
#include "error.h"

#include <string.h>
#include <stdlib.h>

/*@null@*/ char*
strl_head(str_list_t *l)
{
  if(l != (str_list_t*)NULL)
    return l->elem;

  return NULL;
}

/*@null@*/ str_list_t*
strl_rest(str_list_t *l)
{
  if(l != (str_list_t*)NULL)
    return l->rest;

  return NULL;
}

str_list_t*
strl_cons(str_list_t /*@null@*/ *l, char* elem)
{
  str_list_t *n = (str_list_t*)malloc(sizeof(str_list_t));
  char *nelem = (char*)malloc(sizeof(char) * MAX_STR);

  if(n != NULL && nelem != NULL){
    strncpy(nelem, elem, MAX_STR -1);
    nelem[MAX_STR -1] = '\0';
    n->elem = nelem;
    n->rest = l;
  } else {
    print_error_and_die();
  }

  return n;
}

void
strl_free(str_list_t *l)
{
  str_list_t *i = l;
  str_list_t *tmp = NULL;

  while(i != NULL){
    free(i->elem);
    tmp = i->rest;
    free(i);
    i = tmp;
  }
}

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

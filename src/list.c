#include "list.h"

#include <string.h>

void
print_error_and_die()
{
  perror("check_mutti");
  fprintf(stderr, "Error is fatal, bailing out.\n");
  exit(EXIT_FAILURE);
}

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

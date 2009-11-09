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

#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#include <stdio.h>
#include <errno.h>

#include "error.h"
#include "list.h"

#define MAX_ARGS 125
#define MAX_OUT 4048
#define MAX_LINE 256

#define CHECK_OK 0
#define CHECK_WARNING 1
#define CHECK_CRITICAL 2
#define CHECK_UNKNOWN 3

static regex_t line_re; /* regex for commands */
static int verbosity = 0;

struct check {
  char *tag;          /* tag of check */
  char *path;         /* path to check */
  int val;            /* return value */
  int elapsed;        /* usecs elapsed */
  char *out;          /* check stdout */
  char *err;          /* check stderr */
  struct check *next;
  char **argv;
};

static struct option longopts[] = {
  { "filename",   required_argument,    NULL,    'f'},
  { "name",       required_argument,    NULL,    'n'},
  { "timeout",    required_argument,    NULL,    't'},
  { "TIMEOUT",    required_argument,    NULL,    'T'},
  { "libexec",    required_argument,    NULL,    'l'},
  { "report",     required_argument,    NULL,    'r'},
  { "set",        required_argument,    NULL,    's'},
  { "execute",    required_argument,    NULL,    'x'},
  { "warning",    required_argument,    NULL,    'w'},
  { "critical",   required_argument,    NULL,    'c'},
  { "unknown",    required_argument,    NULL,    'u'},
  { "ok",         required_argument,    NULL,    'o'},
  { "help",       no_argument,          NULL,    'h'},
  { "verbose",    no_argument,          NULL,    'v'},
  { "version",    no_argument,          NULL,    'V'},
  { NULL,         0,                    NULL,    0}
};

static void
version()
{
  printf("check_mutti version X, Copyright 2009, Mark A. Meyer\n");
}

static void
usage()
{
  printf("check_mutti version X, Copyright 2009, Mark A. Meyer\n");
  printf("Usage here\n");
}

int sigchld_pipe[2]; /* magic pipe */

void sigchld_sigh(int n)
{
  write(sigchld_pipe[1], "c", 1);
}

static void
setup_sigchld()
{
  /* fixme -- we don't want to call this multiple times (we do, however) */

  static struct sigaction act;

  if(pipe(sigchld_pipe))
    print_error_and_die();

  fcntl(sigchld_pipe[0], F_SETFL,
	fcntl(sigchld_pipe[0], F_GETFL) | O_NONBLOCK);
  fcntl(sigchld_pipe[1], F_SETFL,
	fcntl(sigchld_pipe[1], F_GETFL) | O_NONBLOCK);

  memset(&act, 0, sizeof(act));
  act.sa_handler = sigchld_sigh;

  sigaction(SIGCHLD, &act, NULL);
}

int
run_check(check_t *check, int usecs)
{
  int outp[2];
  int errp[2];
  int res = -1;
  char buf[1];
  char iobuf[MAX_OUT + 1];
  char *outbuf = NULL;
  fd_set fds;
  struct timeval tv, tv_check, tv_end;
  int status;
  static char *intmsg = "Check timed out.";
  static char *emptymsg = "";
  int sz = 0;

  if(pipe(outp))
    print_error_and_die();
    fcntl(outp[0], F_SETFL, fcntl(outp[0], F_GETFL) | O_NONBLOCK);
    fcntl(outp[1], F_SETFL, fcntl(outp[1], F_GETFL) | O_NONBLOCK);

  if(pipe(errp))
    print_error_and_die();
    fcntl(errp[0], F_SETFL, fcntl(errp[0], F_GETFL) | O_NONBLOCK);
    fcntl(errp[1], F_SETFL, fcntl(errp[1], F_GETFL) | O_NONBLOCK);

  setup_sigchld();

  tv.tv_sec = usecs / 1000;
  tv.tv_usec = usecs % 1000;

  FD_ZERO(&fds);
  FD_SET(sigchld_pipe[0], &fds);

  /* take time before fork */

  gettimeofday(&tv_check, 0);

  res = fork();
  if( res == 0 ){
    close(outp[0]);
    close(errp[0]);

    dup2(outp[1], STDOUT_FILENO);
    dup2(errp[1], STDERR_FILENO);

    execv(check->path, check->argv);
    
    perror("check_mutti");
    fflush(stdout);
    fflush(stderr);
    exit(EXIT_FAILURE);
  } else if (res > 0) {

    close(outp[1]);
    close(errp[1]);

    if(select(sigchld_pipe[0], &fds, NULL, NULL, &tv) != 0){

      read(sigchld_pipe[0],buf,sizeof(buf));
      waitpid(res, &status, WNOHANG);
      
      check->val = status;
    } else {
      kill(res, SIGKILL);
      check->val = 23;
      check->out = intmsg;
      check->err = emptymsg;
    }

    sz = read(outp[0], iobuf, MAX_OUT);
    while(sz == -1 && errno == EINTR)
      sz = read(outp[0], iobuf, MAX_OUT);

    iobuf[sz] = '\0';
    if(sz > 0){
      outbuf = malloc(sizeof(char) * sz + 1);
      if(outbuf == NULL)
	print_error_and_die();

      strncpy(outbuf, iobuf, sz);
      outbuf[sz] = '\0';
      check->out = outbuf;
    } else {
      check->out = emptymsg;
    }

    sz = read(errp[0], iobuf, MAX_OUT);
    iobuf[sz] = '\0';
    if(sz > 0){
      outbuf = malloc(sizeof(char) * sz + 1);
      if(outbuf == NULL)
	print_error_and_die();

      strncpy(outbuf, iobuf, sz);
      outbuf[sz] = '\0';
      check->err = outbuf;      
    } else {
      check->err = emptymsg;
    }

  } else {
    close(outp[0]);
    close(outp[1]);
    close(errp[0]);
    close(errp[1]);

    print_error_and_die();
  }

  gettimeofday(&tv_end, 0);

  check->elapsed = (tv_end.tv_sec - tv_check.tv_sec) * 1000;
  check->elapsed += (tv_end.tv_usec - tv_check.tv_usec) / 1000;

  return 0;
}

int
parse_line(char *line, char **tag, char **cmd)
{
  regmatch_t matchptr[3];
  char *ntag, *ncmd;
  int len;
  char regerr[MAX_LINE];
  int ec;

  ec = regexec(&line_re, line, 3, matchptr, 0);
  if(ec){
    if(verbosity > 2){
      if(regerror(ec, &line_re, regerr, MAX_LINE) < MAX_LINE)
	printf("Error in regex: %s\n", regerr);
      printf("Regex doesn't match.\n");
    }
    return 1;
  }

  // per definition of the regex, matchptr[n].eo > matchptr[n].so

  len = matchptr[1].rm_eo - matchptr[1].rm_so;
  ntag = malloc((len+1)*sizeof(char));
  if(ntag == 0)
    print_error_and_die();
  strncpy(ntag, line + matchptr[1].rm_so, len);
  ntag[len] = '\0';

  len = matchptr[2].rm_eo - matchptr[2].rm_so;
  ncmd = malloc((len+1)*sizeof(char));
  if(ncmd == 0)
    print_error_and_die();
  strncpy(ncmd, line + matchptr[2].rm_so, len);
  ncmd[len] = '\0';

  *tag = ntag;
  *cmd = ncmd;
  
  return 0;
}

int
execute_xlines(str_list_t *xlines, int usecs_total, int usecs_check,
	       check_t** reslist)
{
  str_list_t *rest;
  char *head;
  check_t check;
  char *realcmd;
  char **argv;
  int remaining;
  check_t* prev = 0;
  check_t* current = 0;
  int maxres = 0;

  remaining = usecs_total;

  head = strl_head(xlines);
  rest = strl_rest(xlines);

  if(*reslist != NULL){
    prev = *reslist;
  }

  while(head != NULL){
    memset(&check, 0, sizeof(check));

    if(parse_line(head, &check.tag, &realcmd)){
      if(verbosity > 2)
	fprintf(stderr, "Malfrmed check: %s\n", head);
      if(maxres < 3)
	maxres = 3;

      head = strl_head(rest);
      rest = strl_rest(rest);
      continue;
    }
    check.path = "/bin/sh";
    argv = malloc(sizeof(char*) * 4);
    if(argv == NULL)
      print_error_and_die();
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = realcmd;
    argv[3] = NULL;
    check.argv = argv;

    if(verbosity > 2)
      printf("Running check: %s with tag: %s, remaining %i.\n", realcmd, check.tag, remaining);

    run_check(&check, remaining > usecs_check ? usecs_check : remaining);

    if(verbosity > 2){
      printf("Status: %i\n", check.val);
    }

    if(check.val > maxres)
      maxres = check.val;

    remaining -= check.elapsed;
    if(remaining < 0){
      break;
    }

    current = malloc(sizeof(check_t));
    if(current == 0)
      print_error_and_die();
    memcpy(current, &check, sizeof(check_t));

    if(prev != 0){
      current->next = prev;
    }
    prev = current;

    head = strl_head(rest);
    rest = strl_rest(rest);

  }

  if(current != NULL)
    *reslist = current;

  return maxres;
}

int
execute_files(str_list_t *files, int usecs_total, int usecs_check,
	      check_t** reslist)
{
  char* head;
  str_list_t *rest;
  FILE* file;
  char line[MAX_LINE];
  char* tmp;
  str_list_t* lines = 0;
  int maxres = 0;
  int tmpres;

  head = strl_head(files);
  rest = strl_rest(files);

  while(head != NULL){
    lines = 0;
    file = fopen(head, "r");
    if(file == NULL){
      perror("check_mutti");
      if(maxres < 1)
	maxres = 1;

      goto el1;
    }

    tmp = fgets(line, MAX_LINE, file);
    if(tmp == NULL){
      if(ferror(file)){
	if(maxres < 1)
	  maxres = 1;
	perror("check_mutti");
      }

      goto el1;
    }
    for(;;){
      lines = strl_cons(lines, tmp);

      tmp = fgets(line, MAX_LINE, file);
      if(tmp == NULL){
	if(ferror(file)){
	  if(maxres < 1)
	    maxres = 1;
	  perror("check_mutti");
	}
	break;
      }
    }

    tmpres = execute_xlines(lines, usecs_total, usecs_check, reslist);
    if(tmpres > maxres)
      maxres = tmpres;

    strl_free(lines);

    fclose(file);

  el1:
    head = strl_head(rest);
    rest = strl_rest(rest);
  }

  return maxres;
}

void
report(check_t* res, int result)
{
  check_t* i;
  int checks_crit = 0, checks_warn = 0, checks_unknown =0, checks_ok =0;
  char tmp[MAX_OUT];
  char tmp2[MAX_OUT];
  char* tmpline;
  char* tmppipe;
  char* lastline;
  int linelen, outlen, len;
  str_list_t* perf_list = 0;
  str_list_t* rest = 0;
  char* head;
  char* status;
  int needscomma =0;
  
  if(res == NULL)
    return;

  for(i = res; i != NULL; i = i->next){
    if(i->val == CHECK_OK){
      checks_ok++;
    } else if (i->val == CHECK_WARNING){
      checks_warn++;
    } else if (i->val == CHECK_CRITICAL){
      checks_crit++;
    } else {
      checks_unknown++;
    }

    lastline = i->out;
    outlen = strlen(i->out);
    for(tmpline = strchr(i->out, '\n');
	tmpline != NULL;
	tmpline = strchr(tmpline + 1, '\n')){
      linelen = (tmpline - lastline);
      memcpy(tmp, lastline, linelen);
      lastline = tmpline + 1;
      tmp[linelen] = '\0';

      tmppipe = strchr(tmp, '|');

      if(tmppipe != NULL)
	len = tmppipe - tmp;
      else
	len = linelen;

/*
      strncpy(tmp2, tmp, len);
      tmp2[len] = '\0';
      res_list = strl_cons(res_list, tmp2);
*/

      if(tmppipe != 0){
	len = linelen - len - 1;
	strncpy(tmp2, tmppipe + 1, len);
	tmp2[len] = '\0';
	perf_list = strl_cons(perf_list, tmp2);
      }
    }
  }

  if(result == CHECK_OK)
    status = "OK";
  else if(result == CHECK_WARNING)
    status = "WARNING";
  else if(result == CHECK_CRITICAL)
    status = "CRITICAL";
  else
    status = "UNKNOWN";

  printf("check_mutti %s - (", status);
  if(checks_ok > 0){
    printf("ok: %i", checks_ok);
    needscomma = 1;
  }
  if(checks_warn > 0){
    if(needscomma)
      printf(",");
    printf("warn: %i", checks_warn);
    needscomma = 1;
  }
  if(checks_crit > 0){
    if(needscomma)
      printf(",");
    printf("crit: %i", checks_crit);
    needscomma = 1;
  }
  if(checks_unknown > 0){
    if(needscomma)
      printf(",");
    printf("uknown: %i", checks_unknown);
  }
  printf(") | ");

  head = strl_head(perf_list);
  rest = strl_rest(perf_list);

  while(head != NULL){
    printf("%s ", head);

    head = strl_head(rest);
    rest = strl_rest(rest);
  }
}

int
main(int argc, char **argv)
{
  int ch;
  char *tmp;

  str_list_t *files = NULL;
  str_list_t *xlines = NULL;
  char *name = NULL;
  char *libexec = NULL;
  int time_each, time_total, tmp_int;
  check_t* resx = 0;

  time_each = 10000;
  time_total = 10000;

  /* parse options */

  while((ch = getopt_long(argc, argv, "f:n:t:T:l:r:s:x:w:c:u:o:hvV", longopts, NULL)) != -1)
    switch (ch){
    case 'f':
      files = strl_cons(files, optarg);
      break;
    case 'n':
      tmp = malloc(MAX_STR * sizeof(char));
      if(tmp != NULL){
	strncpy(tmp, optarg, MAX_STR-1);
	tmp[MAX_STR-1] = '\0';
	name = tmp;
      }
      name = optarg;
      break;
    case 't':
      tmp_int = (int)strtol(optarg, 0, 10);
      if(tmp_int < 1){
	fprintf(stderr, "argument to -t not valid\n");
	usage();
	exit(CHECK_UNKNOWN);
      }
      time_each = 1000 * tmp_int;
      break;
    case 'T':
      tmp_int = (int)strtol(optarg, 0, 10);
      if(tmp_int < 1){
	fprintf(stderr, "argument to -T not valid\n");
	usage();
	exit(CHECK_UNKNOWN);
      }
      time_total = 1000 * tmp_int;
      break;
    case 'l':
      tmp = malloc(MAX_STR * sizeof(char));
      if(tmp != NULL){
	strncpy(tmp, optarg, MAX_STR -1);
	tmp[MAX_STR-1] = '\0';
	libexec = tmp;
      } else {
	print_error_and_die();
      }
      break;
    case 'r':
      break;
    case 's':
      tmp_int = putenv(optarg);
      if(tmp_int)
	print_error_and_die();
      break;
    case 'x':
      xlines = strl_cons(xlines, optarg);
      break;
    case 'w':
      break;
    case 'c':
      break;
    case 'o':
      break;
    case 'u':
      break;
    case 'h':
      usage();
      exit(EXIT_SUCCESS);
      break;
    case 'v':
      verbosity += 1;
      break;
    case 'V':
      version();
      exit(EXIT_SUCCESS);
      break;
    default:
      usage();
      exit(CHECK_UNKNOWN);
    }

  regcomp(&line_re, "[[:space:]]*command[[:space:]]*\\[[[:space:]]*([^[:space:]]+)[[:space:]]*\\][[:space:]]*=[[:space:]]*(.+)", REG_EXTENDED);

  int x = execute_xlines(xlines, time_total, time_each, &resx);

  x = execute_files(files, time_total, time_each, &resx);

  report(resx, x);

  return x;
}

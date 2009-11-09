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

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void
print_error_and_die()
{
  perror("check_mutti");
  fprintf(stderr, "Error is fatal, bailing out.\n");
  exit(3);
}

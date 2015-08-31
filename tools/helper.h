/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Surfer */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#ifndef HELPER_HELPER_H
# define HELPER_HELPER_H


/* Helper */
/* public */
/* types */
typedef struct _Surfer Helper;


/* functions */
Helper * helper_new(void);
void helper_delete(Helper * helper);

/* useful */
int helper_error(Helper * helper, char const * message, int ret);

int helper_open(Helper * helper, char const * url);
int helper_open_dialog(Helper * helper);

#endif /* !HELPER_HELPER_H */

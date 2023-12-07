/*
 * Copyright (C) 2016  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* kyulib.h
 *	Tom Trebisky 12/1/2001
 *	Tom Trebisky 9/23/2015
 *	Tom Trebisky 12/1/2023
 */

/* We provide these (at least thus far */
int printf(const char *, ...);
// int sprintf(char *, const char *, ...);

void * memcpy ( void *, char *, int );

/* XXX - the following fixed size should really
 * be an argument to cq_init() and be dynamically
 * allocated.
 */

#define MAX_CQ_SIZE	2048

struct cqueue {
	char	buf[MAX_CQ_SIZE];
	char	*bp;
	char	*ip;
	char	*op;
	char	*limit;
	int	size;
	int	count;
	int	toss;
};

// struct cqueue * cq_init ( int );
struct cqueue * cq_init ( struct cqueue * );
void cq_add ( struct cqueue *, int );
int cq_remove ( struct cqueue * );
int cq_count ( struct cqueue * );

/* THE END */

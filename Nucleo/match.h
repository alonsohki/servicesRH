/*
 * match.h
 *
 * $Id: match.h,v 1.1.1.1 2006/12/19 12:56:34 zipbreake Exp $
 */
#pragma once

/*
 * Prototypes
 */

/*
 * XXX - match returns 0 if a match is found. Smelly interface
 * needs to be fixed. --Bleep
 */
extern int mmatch(const char *old_mask, const char *new_mask);
extern int match(const char *ma, const char *na);
extern char *collapse(char *pattern);

extern int matchcomp(char *cmask, int *minlen, int *charset, const char *mask);
extern int matchexec(const char *string, const char *cmask, int minlen);
extern int matchdecomp(char *mask, const char *cmask);
extern int mmexec(const char *wcm, int wminlen, const char *rcm, int rminlen);


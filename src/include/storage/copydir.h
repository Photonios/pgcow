/*-------------------------------------------------------------------------
 *
 * copydir.h
 *	  Copy a directory.
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/copydir.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef COPYDIR_H
#define COPYDIR_H

/* Hook for plugins to get control in copydir() */
typedef void (*copydir_hook_type) (char *fromdir, char *todir, bool recurse);
extern PGDLLIMPORT copydir_hook_type copydir_hook;

extern void copydir(char *fromdir, char *todir, bool recurse);
extern void standard_copydir(char *fromdir, char *todir, bool recurse);
extern void copy_file(char *fromfile, char *tofile);

#endif							/* COPYDIR_H */

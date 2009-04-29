/*
 * IRC - Internet Relay Chat, ircd/hash.c
 * Copyright (C) 1998 Andrea Cocito, completely rewritten version.
 * Previous version was Copyright (C) 1991 Darren Reed, the concept
 * of linked lists for each hash bucket and the move-to-head
 * optimization has been borrowed from there.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define HASHSIZE 32000

extern void init_hash ( void );
extern int strhash ( const char *n );

#define HASH_STRING_DELETED (const char *)0xFABADA00
#define HASH_STRING_EMPTY (const char *)0x00000000

static inline int CompareStrings ( const char* s1, const char* s2 )
{
    while ( *s1 != '\0' && *s2 != '\0' && ToLower ( *s1 ) == ToLower ( *s2 ) )
    {
        ++s1;
        ++s2;
    }

    return ( *s1 - *s2 );
}

struct SStringHasher
{
    size_t operator ( ) ( const char* str ) const
    {
        return strhash ( str );
    }
};

struct SStringEquals
{
    bool operator ( ) ( const char* s1, const char* s2 ) const
    {
        if ( s1 == s2 )
            return true;
        if ( s1 == HASH_STRING_EMPTY || s2 == HASH_STRING_EMPTY || s2 == HASH_STRING_DELETED )
            return false;
        return !CompareStrings ( s1, s2 );
    }
};
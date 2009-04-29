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

#include "stdafx.h"

/************************* Nemesi's hash alghoritm ***********************/

/* This hash function returns *exactly* N%HASHSIZE, where 'N'
 * is the string itself (included the trailing '\0') seen as
 * a baseHASHSHIFT number whose "digits" are the bytes of the
 * number mapped through a "weight" transformation that gives
 * the same "weight" to caseless-equal chars, example:
 *
 * Hashing the string "Nick\0" the result will be:
 * N  i  c  k \0
 * |  |  |  |  `--->  ( (hash_weight('\0') * (HASHSHIFT**0) +
 * |  |  |  `------>    (hash_weight('k')  * (HASHSHIFT**1) +
 * |  |  `--------->    (hash_weight('c')  * (HASHSHIFT**2) +
 * |  `------------>    (hash_weight('i')  * (HASHSHIFT**3) +
 * `--------------->    (hash_weight('N')  * (HASHSHIFT**4)   ) % HASHSIZE
 *
 * It's actually a lot similar to a base transformation of the
 * text representation of an integer.
 * Looking at it this way seems slow and requiring unlimited integer
 * precision, but we actually do it with a *very* fast loop, using only
 * short integer arithmetic and by means of two memory accesses and
 * 3 additions per each byte processed.. and nothing else, as a side
 * note the distribution of real nicks over the hash table of this
 * function is about 3 times better than the previous one, and the
 * hash function itself is about 25% faster with a "normal" HASHSIZE
 * (it gets slower with larger ones and faster for smallest ones
 * because the hash table size affect the size of some maps and thus
 * the effectiveness of RAM caches while accesing them).
 * These two pages of macros are here to make the following code
 * _more_ understandeable... I hope ;)
 */

/* Internal stuff, think well before changing this, it's how
   much the weights of two lexicograhically contiguous chars
   differ, i.e. (hash_weight('b')-hash_weight('a')) == HASHSTEP
   One seems to be fine but the alghoritm doesn't depend on it */
#define HASHSTEP 1

/* The smallest _prime_ int beeing HASHSTEP times bigger than a byte,
   that is the first prime bigger than the maximum hash_weight
   (since the maximum hash weight is gonne be the "biggest-byte * HASHSTEP")
 */
#define HASHSHIFT 257

/* Are we sure that HASHSHIFT is big enough ? */
#if !(HASHSHIFT > (HASHSTEP*(CHAR_MAX-CHAR_MIN)))
#error "No no, I cannot, please make HASHSHIFT a bigger prime !"
#endif

/* Now HASHSIZE doesn't need to be a prime, but we really don't want it
   to be an exact multiple of HASHSHIFT, that would make the distribution
   a LOT worse, once is not multiple of HASHSHIFT it can be anything */
#if ((HASHSIZE%HASHSHIFT)==0)
#error "Please set HASHSIZE to something not multiple of HASHSHIFT"
#endif

/* What type of integer do we need in our computations ? the largest
   value we need to work on is (HASHSIZE+HASHSHIFT+1), for memory
   operations we want to keep the tables compact (the cache will work
   better and we will run faster) while for work variables we prefer
   to roundup to 'int' if it is the case: on platforms where int!=short
   int arithmetic is often faster than short arithmetic, we prefer signed
   types if they are big enough since on some architectures they are faster
   than unsigned, but we always keep signedness of mem and regs the same,
   to avoid sign conversions that sometimes require time, the following
   precompile stuff will set HASHMEMS to an appropriate integer type for
   the tables stored in memory and HASHREGS to an appropriate int type
   for the work registers/variables/return types. Everything of type
   HASH???S will remain internal to this source file so I placed this stuff
   here and not in the header file. */

#undef HASHMEMS
#undef HASHREGS

#if ((!defined(HASHMEMS)) && (HASHSIZE < (SHRT_MAX-HASHSHIFT)))
#define HASHMEMS short
#define HASHREGS int
#endif

#if ((!defined(HASHMEMS)) && (HASHSIZE < (USHRT_MAX-HASHSHIFT)))
#define HASHMEMS unsigned short
#define HASHREGS unsigned int
#endif

#if ((!defined(HASHMEMS)) && (HASHSIZE < (INT_MAX-HASHSHIFT)))
#define HASHMEMS int
#define HASHREGS int
#endif

#if ((!defined(HASHMEMS)) && (HASHSIZE < (UINT_MAX-HASHSHIFT)))
#define HASHMEMS unsigned int
#define HASHREGS unsigned int
#endif

#if ((!defined(HASHMEMS)) && (HASHSIZE < (LONG_MAX-HASHSHIFT)))
#define HASHMEMS long
#define HASHREGS long
#endif

#if ((!defined(HASHMEMS)) && (HASHSIZE < (ULONG_MAX-HASHSHIFT)))
#define HASHMEMS unsigned long
#define HASHREGS unsigned long
#endif

#if (!defined(HASHMEMS))
#error "Uh oh... I have a problem, do you want a 16GB hash table ? !"
#endif

/* Now we are sure that HASHMEMS and HASHREGS can contain the following */
#define HASHMAPSIZE (HASHSIZE+HASHSHIFT+1)

/* Static memory structures */

/* We need a first function that, given an integer h between 1 and
   HASHSIZE+HASHSHIFT, returns ( (h * HASHSHIFT) % HASHSIZE ) )
   We'll map this function in this table */
static HASHMEMS hash_map[HASHMAPSIZE];

/* Then we need a second function that "maps" a char to its weitgh,
   changed to a table this one too, with this macro we can use a char
   as index and not care if it is signed or not, no.. this will not
   cause an addition to take place at each access, trust me, the
   optimizer takes it out of the actual code and passes "label+shift"
   to the linker, and the linker does the addition :) */
static HASHMEMS hash_weight_table[CHAR_MAX - CHAR_MIN + 1];
#define hash_weight(ch) hash_weight_table[ch-CHAR_MIN]

/* This is what the hash function will consider "equal" chars, this function
   MUST be transitive, if HASHEQ(y,x)&&HASHEQ(y,z) then HASHEQ(y,z), and MUST
   be symmetric, if HASHEQ(a,b) then HASHEQ(b,a), obvious ok but... :) */
#define HASHEQ(x,y) ((ToLower(x)) == (ToLower(y)))

/* init_hash
 * Initialize the maps used by hash functions and clear the tables */
void init_hash(void)
{
  int           i;
  int           j;
  unsigned long l;
  unsigned long m;

  /* Here is to what we "map" a char before working on it */
  for (i = CHAR_MIN; i <= CHAR_MAX; i++)
    hash_weight(i) = (HASHMEMS) (HASHSTEP * ((unsigned char)i));

  /* Make them equal for case-independently equal chars, it's
     lame to do it this way but I wanted the code flexible, it's
     possible to change the HASHEQ macro and not touch here.
     I don't actually care about the 32768 loops since it happens
     only once at startup */
  for (i = CHAR_MIN; i <= CHAR_MAX; i++) {
    for (j = CHAR_MIN; j < i; j++) {
      if (HASHEQ(i, j))
        hash_weight(i) = hash_weight(j);
    }
  }

  /* And this is our hash-loop "transformation" function,
     basically it will be hash_map[x] == ((x*HASHSHIFT)%HASHSIZE)
     defined for 0<=x<=(HASHSIZE+HASHSHIFT) */
  for (m = 0; m < (unsigned long)HASHMAPSIZE; m++)
  {
    l = m;
    l *= (unsigned long)HASHSHIFT;
    l %= (unsigned long)HASHSIZE;
    hash_map[m] = (HASHMEMS) l;
  }
}

/* These are the actual hash functions, since they are static
   and very short any decent compiler at a good optimization level
   WILL inline these in the following functions */

/* This is the string hash function,
   WARNING: n must be a valid pointer to a _non-null_ string
   this means that not only strhash(NULL) but also
   strhash("") _will_ coredump, it's responsibility
   the caller to eventually check BadPtr(nick). */

HASHREGS strhash(const char *n)
{
  HASHREGS hash = hash_weight(*n++);
  while (*n)
    hash = hash_map[hash] + hash_weight(*n++);
  return hash_map[hash];
}

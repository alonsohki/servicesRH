#include "stdafx.h"

void tea(const unsigned int *const v,const unsigned int * const k,
   unsigned int *const w)
{
   register unsigned int       y=v[0]^w[0],z=v[1]^w[1],sum=0,delta=0x9E3779B9,n=32;

   while(n-->0)
      {
      y += ((z << 4) ^ (z >> 5)) + (z ^ sum) + k[sum&3];
      sum += delta;
      z += ((y << 4) ^ (y >> 5)) + (y ^ sum) + k[sum>>11 & 3];
      }

   w[0]=y; w[1]=z;
}

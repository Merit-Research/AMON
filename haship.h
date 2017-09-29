//
//  haship.h
//  
//
//  Created by Stilian Stoev on 5/27/14.
//
//

#ifndef _haship_h
#define _haship_h

#include <math.h>
#include <string.h>

#define BRICK_DIMENSION 128 // Databrick has dimensions BRICK_DIMENSION x BRICK_DIMENSION
#define Low8 255
#define Low14 16383
#define Low17 131071

#define Octet1(x) (((x)>>24)&Low8)
#define Octet2(x) (((x)>>16)&Low8)
#define Octet3(x) (((x)>>8)&Low8)
#define Octet4(x) (((x)&Low8))
#define LOW16of32(x) ((x)& 0x0000FFFF)
#define HIGH16of32(x) ((x) >> 16)

struct ipoctets {
  u_int8_t      o0;
  u_int8_t      o1;
  u_int8_t      o2;
  u_int8_t      o3;
};

// The following function for integer powers is borrowed from the
// net... http://stackoverflow.com/users/18814/elias-yarrkov


unsigned long ipow(unsigned long base, int exp);
//
// **************************** code function ****************************
//

u_int32_t code(u_int32_t ip, int to_code, int k);

//
// Before calling "code" allocate an array with the required size and pass
// its pointer to "code"
//
// input:
//  ip  <- IP to code
//
//  to_code <- if TRUE, it encodes otherwise it decodes
//  k   <- order parameter of the code
//
// output: the coded/decoded IP 
//

/*
 * An implementation of a 2-universal hash with 4 precomputed table 2^8-long table look-ups. Motivated by
 * Thorrup & Zhang's function from $5.2.
 */

void init_tables(u_int32_t *T1, u_int32_t *T2, u_int32_t *T3, u_int32_t *T4);

void init_IDX(int *IDX, int reserved);

void init_tables16(u_int32_t *T1, u_int32_t *T2, u_int32_t *T3);

inline u_int64_t Table16(u_int32_t x, u_int32_t T0[], u_int32_t T1[],u_int32_t T2[]);

void init_IDX17(int *IDX, int reserved);

void init_STRATA_IDX17(int *STRATA_IDX_prefix_bin);

void update_STRATA_IDX17(int *STRATA_IDX17_prefix_bin, u_int32_t IP_prefix, int STRATA_index);

#endif

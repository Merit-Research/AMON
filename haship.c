//
//
//  Created by Stilian Stoev on 5/27/14.
//
//


#include <math.h>
#include <string.h>
#include "haship.h"

// The following function for integer powers is borrowed from the
// net... http://stackoverflow.com/users/18814/elias-yarrkov


unsigned long ipow(unsigned long base, int exp)
{
    unsigned long result = 1;
    while (exp)
    {
        if (exp & 1)
            result *=  base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

/*
 * An implementation of a 2-universal hash with 4 precomputed table 2^8-long table look-ups. Motivated by
 * Thorrup & Zhang's function from $5.2.
 */
void init_tables(u_int32_t *T1, u_int32_t *T2, u_int32_t *T3, u_int32_t *T4){
        int i;
        u_int32_t c;

        for (i=0; i<255; i++) {
                c = rand();
                T1[i]= (u_int32_t)c % Low14;
                /*printf("%d ",T1[i]);*/
                c = rand();
                T2[i]= (u_int32_t)c % Low14;
                c = rand();
                T3[i]= (u_int32_t)c % Low14;
                c = rand();
                T4[i]= (u_int32_t)c % Low14;
        }
}

void init_IDX(int *IDX, int reserved){
        int i;
        for (i=0; i<Low14; i++){
                IDX[i] = reserved + rand()% (BRICK_DIMENSION-reserved);
        }
}

void init_tables16(u_int32_t *T1, u_int32_t *T2, u_int32_t *T3){
        int i;
        u_int64_t c;

        for (i=0; i< 65536; i++) {
                c = rand();
                T1[i]= (u_int32_t)c % Low17;
                c = rand();
                T2[i]= (u_int32_t)c % Low17;
                c = rand();
                T3[i]= (u_int32_t)c % Low17;
                c = rand();
                T3[i+65536] = (u_int32_t)c % Low17;
        }
}


inline u_int64_t Table16(u_int32_t x, u_int32_t T0[], u_int32_t T1[],u_int32_t T2[]){
	u_int32_t x0, x1, x2;
	x0 = LOW16of32(x);
	x1 = HIGH16of32(x);
	/*x2 = x0 ^ x1;*/
	 x2 = (x0+x1)&Low17;
	/* modification here was x0+x1 -- at the expense of making it a 3-universal hash
	 *  *but saving on dimension of the tables */
	return T0[x0] ^ T1[x1] ^ T2[x2];
}


void init_IDX17(int *IDX, int reserved){
        for (int i=0; i<pow(2,17); i++){
                IDX[i] = reserved + (rand() % (BRICK_DIMENSION-reserved));
        }
}

//
// Initializing STRATA_IDX17_prefix_bin arrays:
//
void init_STRATA_IDX17(int *STRATA_IDX_prefix_bin){
 for (int i=0; i<pow(2,17);i++){ STRATA_IDX_prefix_bin[i]=-1;};
}

//
// Updating STRATA_IDX17_prefix_bin & STRATA_IDX17_prefix_bit arrays:
//

void update_STRATA_IDX17(int *STRATA_IDX17_prefix_bin, u_int32_t IP_prefix, int STRATA_index){
        STRATA_IDX17_prefix_bin[IP_prefix] = STRATA_index;
}

/* puff_table_generate.c
  Copyright (C) 2002-2013 Mark Adler, all rights reserved
  Extracted into a table generation tool (C) 2026 Adrian "asie" Siekierka

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
 */

#include <stdint.h>
#include <stdio.h>

/*
 * Maximums for allocations and loops.  It is not useful to change these --
 * they are fixed by the deflate format.
 */
#define MAXBITS 15              /* maximum bits in a code */
#define MAXLCODES 286           /* maximum number of literal/length codes */
#define MAXDCODES 30            /* maximum number of distance codes */
#define MAXCODES (MAXLCODES+MAXDCODES)  /* maximum codes lengths to read */
#define FIXLCODES 288           /* number of fixed literal/length codes */

struct huffman {
    short *count;       /* number of symbols of each length */
    short *symbol;      /* canonically ordered symbols */
};

struct huffman_state {
    short lencnt[MAXBITS+1], lensym[FIXLCODES];
    short distcnt[MAXBITS+1], distsym[MAXDCODES];
    struct huffman lencode, distcode;
};

int construct(struct huffman *h, const short *length, int n)
{
    int symbol;         /* current symbol when stepping through length[] */
    int len;            /* current length when stepping through h->count[] */
    int left;           /* number of possible codes left of current length */
    short offs[MAXBITS+1];      /* offsets in symbol table for each length */

    /* count number of codes of each length */
    for (len = 0; len <= MAXBITS; len++)
        h->count[len] = 0;
    for (symbol = 0; symbol < n; symbol++)
        (h->count[length[symbol]])++;   /* assumes lengths are within bounds */
    if (h->count[0] == n)               /* no codes! */
        return 0;                       /* complete, but decode() will fail */

    /* check for an over-subscribed or incomplete set of lengths */
    left = 1;                           /* one possible code of zero length */
    for (len = 1; len <= MAXBITS; len++) {
        left <<= 1;                     /* one more bit, double codes left */
        left -= h->count[len];          /* deduct count from possible codes */
        if (left < 0)
            return left;                /* over-subscribed--return negative */
    }                                   /* left > 0 means incomplete */

    /* generate offsets into symbol table for each length for sorting */
    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++)
        offs[len + 1] = offs[len] + h->count[len];

    /*
     * put symbols in tabCACHE_TABLESle sorted by length, by symbol order within each
     * length
     */
    for (symbol = 0; symbol < n; symbol++)
        if (length[symbol] != 0)
            h->symbol[offs[length[symbol]]++] = symbol;

    /* return zero for complete set, positive for incomplete set */
    return left;
}

static void print_short_array(const char *name, short *array, int length) {
    printf("static const short __far fixed_%s[] = { ", name);
    for (int i = 0; i < length; i++) {
        if (i > 0) printf(", ");
        printf("%d", array[i]);
    }
    printf(" };\n");
}

int main(void) {
    struct huffman_state s;

    int symbol;
    short lengths[FIXLCODES];

    /* construct lencode and distcode */
    s.lencode.count = s.lencnt;
    s.lencode.symbol = s.lensym;
    s.distcode.count = s.distcnt;
    s.distcode.symbol = s.distsym;

    /* literal/length table */
    for (symbol = 0; symbol < 144; symbol++)
        lengths[symbol] = 8;
    for (; symbol < 256; symbol++)
        lengths[symbol] = 9;
    for (; symbol < 280; symbol++)
        lengths[symbol] = 7;
    for (; symbol < FIXLCODES; symbol++)
        lengths[symbol] = 8;
    construct(&s.lencode, lengths, FIXLCODES);

    /* distance table */
    for (symbol = 0; symbol < MAXDCODES; symbol++)
        lengths[symbol] = 5;
    construct(&s.distcode, lengths, MAXDCODES);

    /* print values */
    print_short_array("lencnt", s.lencnt, MAXBITS+1);
    print_short_array("lensym", s.lensym, FIXLCODES);
    print_short_array("distcnt", s.distcnt, MAXBITS+1);
    print_short_array("distsym", s.distsym, MAXDCODES);
}

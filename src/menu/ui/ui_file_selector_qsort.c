/**
 * Copyright (c) 2024 Adrian Siekierka
 *
 * swanshell is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * swanshell is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with swanshell. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <string.h>
#include <ws.h>
#include "ui_file_selector.h"

// https://github.com/DevSolar/pdclib/blob/master/functions/stdlib/qsort.c

/* This implementation is taken from Paul Edward's PDPCLIB.

   Original code is credited to Raymond Gardner, Englewood CO.
   Minor mods are credited to Paul Edwards.
   Some reformatting and simplification done by Martin Baute.
   All code is still Public Domain.
*/

/* For small sets, insertion sort is faster than quicksort.
   T is the threshold below which insertion sort will be used.
   Must be 3 or larger.
*/
#define T 7

/* Macros for handling the QSort stack */
#ifdef __IA16_CALLCVT_NO_ASSUME_SS_DATA
#define PREPARE_STACK uint16_t __far* stack[STACKSIZE]; uint16_t __far* __seg_ss* stackptr = stack
#else
#define PREPARE_STACK uint16_t __far* stack[STACKSIZE]; uint16_t __far* * stackptr = stack
#endif
#define PUSH( base, limit ) stackptr[0] = base; stackptr[1] = limit; stackptr += 2
#define POP( base, limit ) stackptr -= 2; base = stackptr[0]; limit = stackptr[1]
/* TODO: Stack usage is log2( nmemb ) (minus what T shaves off the worst case).
         Worst-case nmemb is platform dependent.
*/
#define STACKSIZE 12

static int qsort_compare(int (*compar)(const file_selector_entry_t __far*, const file_selector_entry_t __far*, void *), void *userdata, uint16_t i, uint16_t j) {
    int result;
    if (ui_file_selector_fno_direct_same_bank(i, j)) {
        result = compar(ui_file_selector_open_fno_direct(i), ui_file_selector_open_fno_direct_nobank(j), userdata);
        outportw(WS_CART_EXTBANK_RAM_PORT, FILE_SELECTOR_INDEX_BANK);
    } else {
        file_selector_entry_t fno_i;
        memcpy(&fno_i, ui_file_selector_open_fno_direct(i), sizeof(file_selector_entry_t));
        result = compar(&fno_i, ui_file_selector_open_fno_direct(j), userdata);
    }
    outportw(WS_CART_EXTBANK_RAM_PORT, FILE_SELECTOR_INDEX_BANK);
    return result;
}

__attribute__((always_inline))
static inline void memswp(uint16_t __far* i, uint16_t __far* j) {
    uint16_t a = *i;
    *i = *j;
    *j = a;
}

void ui_file_selector_qsort(size_t nmemb, int (*compar)(const file_selector_entry_t __far*, const file_selector_entry_t __far*, void *), void *userdata) {
    uint16_t __far* i;
    uint16_t __far* j;
    uint16_t __far* base_          = FILE_SELECTOR_INDEXES;
    uint16_t __far* limit          = base_ + nmemb;
    PREPARE_STACK;
    outportw(WS_CART_EXTBANK_RAM_PORT, FILE_SELECTOR_INDEX_BANK);

    for ( ;; )
    {
        if ( ( size_t )( limit - base_ ) > T ) /* QSort for more than T elements. */
        {
            /* We work from second to last - first will be pivot element. */
            i = base_ + 1;
            j = limit - 1;
            /* We swap first with middle element, then sort that with second
               and last element so that eventually first element is the median
               of the three - avoiding pathological pivots.
               TODO: Instead of middle element, chose one randomly.
            */
            memswp( ( ( ( ( size_t )( limit - base_ ) ) ) / 2 ) + base_, base_ );

            if ( qsort_compare(compar, userdata,  *i, *j ) > 0 )
            {
                memswp( i, j );
            }

            if ( qsort_compare(compar, userdata,  *base_, *j ) > 0 )
            {
                memswp( base_, j );
            }

            if ( qsort_compare(compar, userdata,  *i, *base_ ) > 0 )
            {
                memswp( i, base_ );
            }

            /* Now we have the median for pivot element, entering main Quicksort. */
            for ( ;; )
            {
                do
                {
                    /* move i right until *i >= pivot */
                    i += 1;
                } while ( qsort_compare(compar, userdata,  *i, *base_ ) < 0 );

                do
                {
                    /* move j left until *j <= pivot */
                    j -= 1;
                } while ( qsort_compare(compar, userdata,  *j, *base_ ) > 0 );

                if ( i > j )
                {
                    /* break loop if pointers crossed */
                    break;
                }

                /* else swap elements, keep scanning */
                memswp( i, j );
            }

            /* move pivot into correct place */
            memswp( base_, j );

            /* larger subfile base / limit to stack, sort smaller */
            if ( j - base_ > limit - i )
            {
                /* left is larger */
                PUSH( base_, j );
                base_ = i;
            }
            else
            {
                /* right is larger */
                PUSH( i, limit );
                limit = j;
            }
        }
        else /* insertion sort for less than T elements              */
        {
            for ( j = base_, i = j + 1; i < limit; j = i, i += 1 )
            {
                for ( ; qsort_compare(compar, userdata,  *j, *(j + 1) ) > 0; j -= 1 )
                {
                    memswp( j, j + 1 );

                    if ( j == base_ )
                    {
                        break;
                    }
                }
            }

            if ( stackptr != stack )           /* if any entries on stack  */
            {
                POP( base_, limit );
            }
            else                       /* else stack empty, done   */
            {
                break;
            }
        }
    }
}

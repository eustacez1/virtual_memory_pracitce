#ifndef TLB_H
#define TLB_H

#include <stdio.h>
#include <stdlib.h>
#include  "util.h"

#define TLB_MAX_SIZE 16 // Max TLB entries.

enum TLB_ENTRY{PAGE_NUM, FRAME_NUM, VALIDITY_BIT,PRESENT_BIT,TLB_ENTRY_LENGTH };

typedef struct{
    int page_num, frame_num;
    int validity_bit, present_bit;
} TLB_ENTRY;

typedef TLB_ENTRY* TLB;

int TLB_size = 0; // Current TLB size.
int next_fifo_out = 0; // Next FIFO out index.
/* TLB(translation look aside) data structure. */
//struct TLB{
//    int tlb[TLB_MAX_SIZE][TLB_ENTRY_LENGTH];
//    int next_fifo_out = 0; // Next FIFO out index.
//    int size = 0; // Current TLB size.
//    int tlb_hit_counter = 0; // TLB hit counter.
//    float tlb_rate = 0; // TLB hit rate.
//};


TLB tlb_init(TLB t){

    t = (TLB)malloc(sizeof(TLB_ENTRY)*TLB_MAX_SIZE);

    for (int i = 0; i < TLB_MAX_SIZE; i++) {
        t[i].page_num = INVALID_PAGE; // page_num as index
        t[i].frame_num = INVALID_FRAME_NUM; // frame number
        t[i].validity_bit = INVALID; // invalid
        t[i].present_bit = NOT_PRESENT; // not present
    }

    return t;
};

/* Checks if a page_num is in the TLB
* Returns the frame number if it is in the TLB
* Returns -1 if it is not in the TLB
                                                                                           int consult_tlb(int page_number);
*/
int tlb_retrieve_frame_num(TLB t, int page_number) {
    // If page_number is found, return the corresponding frame number
    for (int i = 0; i < TLB_MAX_SIZE; i++) {
        if (t[i].page_num == page_number && t[i].present_bit == PRESENT && t[i].validity_bit == VALID) {
            return t[i].frame_num;
        }
    }
    return -1;
}

/* Updates the TLB with a new page_num and frame_num            void update_tlb(int page_number, int frame_number);*/
void tlb_update(TLB t, int page_number, int frame_number) {

    // if the TLB is not full, add the new entry
    if (TLB_size < TLB_MAX_SIZE) {
        t[TLB_size].page_num = page_number;
        t[TLB_size].frame_num = frame_number;
        t[TLB_size].validity_bit = VALID; // valid
        t[TLB_size].present_bit = PRESENT; // present
        TLB_size++;
    }

    // ELSE, use FIFO replacement policy
    else{
        t[next_fifo_out].page_num = page_number;
        t[next_fifo_out].frame_num = frame_number;
        t[next_fifo_out].validity_bit = VALID; // valid
        t[next_fifo_out].present_bit = PRESENT; // present
        next_fifo_out = (next_fifo_out + 1) % TLB_MAX_SIZE;
    }

}
#endif


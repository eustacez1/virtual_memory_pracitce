#ifndef PAGETABLE_H
#define PAGETABLE_H
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


typedef struct{
    int frame_num;
    int validity_bit, dirty_bit, present_bit;
} PageTableEntry;

typedef PageTableEntry* PageTable;


/* Initializes page table. */
PageTable page_table_init(PageTable page_table ){

    page_table = (PageTable)malloc(sizeof(PageTableEntry) * PAGE_TABLE_SIZE);

    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i].frame_num = INVALID_FRAME_NUM; //  frame_num
        page_table[i].validity_bit = INVALID; // invalid, 0 = invalid, 1 = valid
        page_table[i].dirty_bit = CLEAN; // dirty bit, 0 = clean, 1 = dirty
        page_table[i].present_bit = NOT_PRESENT; // present bit 0 = disk, 1 = RAM
    }

     return page_table;
}

/* Takes a page_number and checks for a corresponding frame number.
 * Returns frame number or -1 if not found.
 */
int page_table_retrieve_frame_num(PageTable p, int page_number){

    if (page_number>PAGE_TABLE_SIZE) return INVALID_PAGE;
    if (p[page_number].present_bit == NOT_PRESENT || p[page_number].validity_bit == INVALID
        || p[page_number].frame_num == INVALID_FRAME_NUM) return FRAME_IN_DISK;

    return p[page_number].frame_num;
}

void make_dirty(PageTable p, int page_number){
    p[page_number].dirty_bit = DIRTY;
}

#endif
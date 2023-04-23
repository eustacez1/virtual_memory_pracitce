
#ifndef UTIL_H
#define UTIL_H

#define DISK_NAME "BACKING_STORE.bin"
#define PAGE_SIZE 256 // Page size, in bytes.
#define PAGE_TABLE_SIZE 256 // Max page table entries.
#define LOW8_BIT_MASK 255 // Page number size, in bits.
#define FRAME_SIZE 256
#define PAGE_NUM_BITS 8
#define ZERO_OFFSET 0
#define WRITE_BIT_POS 16
#define MAX_FRAMES 256
#define LINE_SIZE 256
#define MAX_OFFSET 256

enum FLAGS{VALID=-10, INVALID, DIRTY, CLEAN, PRESENT, NOT_PRESENT,INVALID_PAGE,INVALID_FRAME_NUM,FRAME_IN_DISK};

/* Get the page number from the virtual address. */
int get_page_number(int virtual_addr) {
    return (virtual_addr >> PAGE_NUM_BITS) & LOW8_BIT_MASK;
}

/* Get the offset from the virtual address. */
int get_offset(int virtual_addr) {
    return (virtual_addr & LOW8_BIT_MASK);
}

/* Get the write bit from the virtual address. */
int get_write_bit(int virtual_addr) {
    return (virtual_addr >> WRITE_BIT_POS) & 1;
}

int get_address(int index, int offset) {
    if(index < 0 || offset > MAX_OFFSET){
        printf("ERR: invalid index: %d or offset: %d\n", index, offset);
        exit(EXIT_FAILURE);
    }
    return FRAME_SIZE*index + offset;
}

#endif


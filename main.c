#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "util.h"
#include "PageTable.h"
#include "TLB.h"
#include "FIFO_Q.h"


#define backing_store "BACKING_STORE.bin"




/* Global Variables */

char memory[FRAME_SIZE*MAX_FRAMES]; // Physical memory. Each frame is FRAME_SIZE byte.
int mem_size = 0; // Number of frames in memory.
Queue* page_replace_queue; // Points to the next frame to be replaced in FIFO.

// Initialize the page table and TLB
TLB tlb ;
PageTable page_table;



int data_access_handler(int page_number,int offset, int write_bit){

    int physical_addr = get_address(page_table[page_number].frame_num, offset);

    if(write_bit){
        memory[physical_addr] += 1;
        make_dirty(page_table, page_number);
    }

    return memory[physical_addr];
}

/*
    * swaps a page from the backing store into memory
    * NOTE: does not validate if the frame is already used or not
    * Returns the frame number of the page that was swapped in
    */
int swap_in( int page_number,int frame_number){

    FILE *backing_store_file;

    // open backing store
    if((backing_store_file = fopen(backing_store, "r+")) == NULL){
        printf("Error: could not open backing store");
        exit(EXIT_FAILURE);
    }

    /* check if page_number is in range
    fseek(backing_store_file, 0, SEEK_END);
    off_t file_length = ftell(backing_store_file);
    if (file_length < get_address(page_number,ZERO_OFFSET)) {
        printf("Error: address number is out of range");
        exit(EXIT_FAILURE);
    }
    */

    // seek to page_number in disk
    if(fseek(backing_store_file, get_address(page_number,ZERO_OFFSET), SEEK_SET)){
        printf("Error: fseek failed");
        exit(EXIT_FAILURE);
    }

    // read page from disk into memory[frame_number]
    if(!fread(&memory[get_address(frame_number,ZERO_OFFSET)], sizeof(char), FRAME_SIZE, backing_store_file)){
        printf("Error: fread failed");
        exit(EXIT_FAILURE);
    }


    fclose(backing_store_file);

//    printf("%s\n",&memory[get_address(frame_number,ZERO_OFFSET)]);

    // add page to page table
    page_table[page_number].validity_bit = VALID;
    page_table[page_number].present_bit = PRESENT;
    page_table[page_number].dirty_bit = CLEAN;
    page_table[page_number].frame_num = frame_number;
    mem_size++;

    // add page to fifo queue
    enqueue(page_replace_queue, page_number);
    return frame_number;
}

/*
    * swaps a frame from memory to the backing store
    * the frame to be swapped out is determined by the page_replace_queue
    * Returns the frame number of the page that was swapped out
    */
int swap_out(){

    FILE *backing_store_file;

    // find page to replace
    int page_to_replace = dequeue(page_replace_queue);
    int frame_to_replace = page_table_retrieve_frame_num(page_table, page_to_replace);

    if(frame_to_replace == INVALID_FRAME_NUM || frame_to_replace == FRAME_IN_DISK){
        printf("Error: frame to replace is invalid or in disk");
        exit(EXIT_FAILURE);
    }

    // if page is dirty, write to disk
    if(page_table[page_to_replace].dirty_bit == DIRTY){

        backing_store_file = fopen(backing_store, "r+");

        // seek to page_number in disk
        if(fseek(backing_store_file, get_address(page_to_replace,ZERO_OFFSET), SEEK_SET)){
            printf("Error: fseek failed");
            exit(EXIT_FAILURE);
        }

        // write frame to disk
        if(!fwrite(&memory[get_address(frame_to_replace,ZERO_OFFSET)], sizeof(char),
                   FRAME_SIZE, backing_store_file)){
            printf("Error: fwrite failed");
            exit(EXIT_FAILURE);
        }
        fclose(backing_store_file);
    }

    // set page to NOT_PRESENT and CLEAN
    page_table[page_to_replace].present_bit = NOT_PRESENT;
    page_table[page_to_replace].frame_num = INVALID_FRAME_NUM;
    page_table[page_to_replace].dirty_bit = CLEAN;

    // update TLB
    for (int i = 0; i < TLB_MAX_SIZE; i++){
        if (tlb[i].page_num == page_to_replace){
            tlb[i].validity_bit = INVALID;
            tlb[i].present_bit = NOT_PRESENT;
            tlb[i].frame_num = INVALID_FRAME_NUM;
        }
    }

    // return frame number
    mem_size--;
    return frame_to_replace;
}

/*
    * Returns the frame number of the given page_number and offset
    */
int page_replacement( int page_number){

    // if frame table is not full, mem_size is the next available frame
    if (mem_size < MAX_FRAMES) return swap_in( page_number, mem_size);

    // if frame table is full (swap_out() returns a frame number to swap in)
    else{
        int frame_out = swap_out();
        swap_in(page_number, frame_out);
        return frame_out;
    }
}


/* function: page fault handler
 * ---------------------
 * 1. Handles the page fault
 * 2. Returns the frame number of the page that was swapped in
 */
int page_fault_handler(int error_type,int page_number){

     switch(error_type){
        case INVALID_PAGE: //process
            printf("ERR: Invalid Page. Exiting.");
            exit(EXIT_FAILURE);
        case FRAME_IN_DISK: // swap_page(frame)
            return page_replacement(page_number);
            break;
        default:
            printf("ERR: Invalid Error Type. Exiting.");
            exit(EXIT_FAILURE);
     }

     return -1;
}


/* function: main memory handler
 * ---------------------
 * 1. Opens the address file and output file
 * 2. Loops through the address file and calls the appropriate handler
 * 3. Closes the address file and output file
 */
int mem_handler(char* filepath) {

    /* --------------------- Initialize Variables --------------------- */
    FILE* addr_input_file;  // Address file ptr
    FILE* output_file;      // Output file ptr

    // Open the address file
    if ((addr_input_file = fopen(filepath, "r")) == NULL) {
        printf("Input file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    // Open the output file
    if ((output_file = fopen("output.txt", "w")) == NULL) {
        printf("Output file could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    char line[LINE_SIZE];
    int virtual_addr = 0, physical_addr = 0, value = 0;
    int page_faults = 0, tlb_hits = 0, address_count = 0;
    int page_number = 0, offset = 0, write_bit = 0;


    // Loop Address File
    while (fgets(line, sizeof(line), addr_input_file)) {

        /* --------------------- Process Address Line --------------------- */

        // Read a single address from file, assign to virtual
        virtual_addr = atoi(line);
        address_count++;

        // Get the page_number, offset, write_bit from the virtual address
        page_number = get_page_number(virtual_addr);
        offset = get_offset(virtual_addr);
        write_bit = get_write_bit(virtual_addr);


        /* -------------------- Read TLB and Page Table -------------------- */

        // Use page_number to find frame_number in TLB, if it exists
        int frame_number = tlb_retrieve_frame_num(tlb, page_number);

        // If frame_number is -1, then it was not found in the TLB
        if (frame_number == -1) {
            // Use page_number to find frame_number in page table
            frame_number =
                page_table_retrieve_frame_num(page_table, page_number);

            // check if frame_number is valid
            switch (frame_number) {

                case FRAME_IN_DISK:
                case INVALID_PAGE:
                    // call handler to process page fault
                    frame_number = page_fault_handler(frame_number, page_number);
                    page_faults++;
                    break;

                // if frame_number is valid, do nothing
                default:
                    break;

            }  // END switch

        }  // END if TLB miss
        else
            tlb_hits++;  // TLB hit

        // Update the TLB with the new frame_number
        tlb_update(tlb, page_number, frame_number);


        /* --------------------- READ/WRITE DATA --------------------- */

        // get the data from the physical address
        value = data_access_handler(page_number, offset, write_bit);

        // Append the results to output_file
        fprintf(output_file, "virtual address: %d\t\t", virtual_addr);
        fprintf(output_file, "physical address: %d\t\t", get_address(frame_number, offset));
        fprintf(output_file, "page number: %3d\t", page_number);
        fprintf(output_file, "write bit: %d\t\t", write_bit);
        fprintf(output_file, "Value: %d\n", value);

    }  // END while address file not EOF

    float fault_rate = (float)page_faults / (float)address_count;
    float tlb_rate = (float)tlb_hits / (float)address_count;

    // Print the statistics to the end of the output file.
    fprintf(output_file, "Number of Translated Addresses = %d\n", address_count);
    fprintf(output_file, "Page Faults = %d\n", page_faults);
    fprintf(output_file, "Page Fault Rate = %.3f\n", fault_rate);
    fprintf(output_file, "TLB Hits = %d\n", tlb_hits);
    fprintf(output_file, "TLB Hit Rate = %.3f\n", tlb_rate);

    // Close all three files.
    fclose(addr_input_file);
    fclose(output_file);



    return EXIT_SUCCESS;

}



int main(){

    /* --------------------- Initialize Variables --------------------- */

    char filepath[] = "addresses.txt";
    page_replace_queue = create_queue(page_replace_queue,PAGE_TABLE_SIZE);
    tlb = tlb_init(tlb);
    page_table = page_table_init(page_table);
    mem_handler(filepath);
}
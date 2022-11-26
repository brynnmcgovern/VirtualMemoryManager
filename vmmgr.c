/**
1. Brynn McGovern
   2370579
   bmcgovern@chapman.edu
   CPSC 380-01
   Assignment 6 - Virtual Address Manager
2. A program to translate logical addresses to physical addresses. It uses a TLB and a page
  table, along with a buffer for reading in values from the file of addresses.
*/
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255

#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255

#define MEMORY_SIZE PAGES * PAGE_SIZE
#define BUFFER_SIZE 10

struct entry {
    unsigned char logical;
    unsigned char physical;
};

struct entry TLB[TLB_SIZE];
int TLBindex = 0;
int pageTable[PAGES];
signed char mainMemory[MEMORY_SIZE];
signed char *backingPointer;

/**
  search()
  @param unsigned char logicalPage
  @return int
  Function to search TLB for logical address and returns the physical address
*/
int search(unsigned char logicalPage) {
    int temp = 0;
    if(TLBindex - TLB_SIZE > 0){
      temp = TLBindex - TLB_SIZE;
    }


    for (int i = temp; i < TLBindex; i++) {
        struct entry *entry = &TLB[i % TLB_SIZE];

        if (entry->logical == logicalPage) {
            return entry->physical;
        }
    }

    return -1;
}

/**
  addToTLB()
  @param unsigned char logical, unsigned char physical
  Function to add to TLB
*/
void addToTLB(unsigned char logical, unsigned char physical) {
    struct entry *entry = &TLB[TLBindex % TLB_SIZE];
    TLBindex++;
    entry->logical = logical;
    entry->physical = physical;
}

/**
  main()
  Main method to implement virtual address manager. Opens BACKING_STORE file and reads in addresses from
  the command line file.
*/
int main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Command line must have two inputs");
        exit(1);
    }

    const char *backingFilename = "BACKING_STORE.bin";
    int backing_fd = open(backingFilename, O_RDONLY);
    backingPointer = (signed char*)mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    const char *inputFilename = argv[1];
    FILE *input_fp = fopen(inputFilename, "r");

    for (int i = 0; i < PAGES; i++) {
        pageTable[i] = -1;
    }

    char buffer[BUFFER_SIZE];

    int totalAddresses = 0;
    int TLBHits = 0;
    int pageFaults = 0;

    unsigned char freePage = 0;

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
        totalAddresses++;
        int logicalAddress = atoi(buffer);
        int offset = logicalAddress & OFFSET_MASK;
        int logicalPage = (logicalAddress >> OFFSET_BITS) & PAGE_MASK;

        int physicalPage = search(logicalPage);

        if (physicalPage != -1) {
            TLBHits++;

        } else {
            physicalPage = pageTable[logicalPage];

            if (physicalPage == -1) {
                pageFaults++;

                physicalPage = freePage;
                freePage++;

                memcpy(mainMemory + physicalPage * PAGE_SIZE, backingPointer + logicalPage * PAGE_SIZE, PAGE_SIZE);

                pageTable[logicalPage] = physicalPage;
            }

            addToTLB(logicalPage, physicalPage);
        }

        int physicalAddress = (physicalPage << OFFSET_BITS) | offset;
        signed char value = mainMemory[physicalPage * PAGE_SIZE + offset];

        printf("Virtual address: %d Physical address: %d Value: %d\n", logicalAddress, physicalAddress, value);
    }

    printf("Page Fault Rate = %.3f\n", pageFaults / (1.0 * totalAddresses));
    printf("TLB Hit Rate = %.3f\n", TLBHits / (1.0 * totalAddresses));

    return 0;
}

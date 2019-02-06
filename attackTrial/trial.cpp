#include <bitset>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <linux/kernel-page-flags.h>
#include <stdint.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <map>
#include <list>
#include <utility>
#include <fstream>
#include <set>
#include <algorithm>
#include <sys/time.h>
#include <sys/resource.h>
#include <sstream>
#include <iterator>
#include <math.h>

// #define THESET 

int theSet = 5;
int validIndices [100];
int numValidIndices = 0;
int arraySize = 1024*1024;
float fraction_of_physical_memory = 0.4;

// #define assert(X) do { if (!(X)) { fprintf(stderr,"assertion '" #X "' failed\n"); exit(-1); } } while (0)
int pagemap = -1;
// Extract the physical page number from a Linux /proc/PID/pagemap entry.
uint64_t frame_number_from_pagemap(uint64_t value) {
  return value & ((1ULL << 54) - 1);
}

uint64_t GetPhysicalMemorySize() {
  struct sysinfo info;
  sysinfo( &info );
  return (size_t)info.totalram * (size_t)info.mem_unit;
}

size_t get_dram_row(void* phys_addr_p) {
  uint64_t phys_addr = (uint64_t) phys_addr_p;
  return phys_addr >> 18;
}

size_t get_dram_mapping(void* phys_addr_p) {
  // uint64_t phys_addr = (uint64_t) phys_addr_p;
  // static const size_t h0[] = { 14, 18 };
  // static const size_t h1[] = { 15, 19 };
  // static const size_t h2[] = { 16, 20 };
  // static const size_t h3[] = { 17, 21 };
  // static const size_t h4[] = { 7, 8, 9, 12, 13, 18, 19 };
  uint64_t phys_addr = (uint64_t) phys_addr_p;
  static const size_t h0[] = { 6,13 };
  static const size_t h1[] = { 14,17 };
  static const size_t h2[] = { 15,18 };
  static const size_t h3[] = { 16,19 };
  // static const size_t h4[] = { 7, 8, 9, 12, 13, 18, 19 };

  size_t count = sizeof(h0) / sizeof(h0[0]);
  size_t hash = 0;
  for (size_t i = 0; i < count; i++) {
    hash ^= (phys_addr >> h0[i]) & 1;
  }
  count = sizeof(h1) / sizeof(h1[0]);
  size_t hash1 = 0;
  for (size_t i = 0; i < count; i++) {
    hash1 ^= (phys_addr >> h1[i]) & 1;
  }
  count = sizeof(h2) / sizeof(h2[0]);
  size_t hash2 = 0;
  for (size_t i = 0; i < count; i++) {
    hash2 ^= (phys_addr >> h2[i]) & 1;
  }
  count = sizeof(h3) / sizeof(h3[0]);
  size_t hash3 = 0;
  for (size_t i = 0; i < count; i++) {
    hash3 ^= (phys_addr >> h3[i]) & 1;
  }
  // count = sizeof(h4) / sizeof(h4[0]);
  // size_t hash4 = 0;
  // for (size_t i = 0; i < count; i++) {
  //   hash4 ^= (phys_addr >> h4[i]) & 1;
  // }
  return (hash3 << 3) | (hash2 << 2) | (hash1 << 1) | hash;
}

uint64_t get_physical_addr(uint64_t virtual_addr) {
  uint64_t value;
  off_t offset = (virtual_addr / 4096) * sizeof(value);
  int got = pread(pagemap, &value, sizeof(value), offset);
  assert(got == 8);

  // Check the "page present" flag.
  assert(value & (1ULL << 63));

  uint64_t frame_num = frame_number_from_pagemap(value);
  return (frame_num * 4096) | (virtual_addr & (4095));
}

int main()
{
  size_t mapping_size = static_cast<uint64_t>((static_cast<double>(GetPhysicalMemorySize()) * fraction_of_physical_memory));
  printf("%u\n", mapping_size);
  void* mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE,
      MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(mapping != (void*)-1);

  // Initialize the mapping so that the pages are non-empty.
  //fprintf(stderr,"[!] Initializing large memory mapping ...");
  for (uint64_t index = 0; index < mapping_size; index += 0x400) {
    *(uint64_t*) (((char*)mapping) + index) = index * 37;
  }
  //fprintf(stderr,"done\n");

  pagemap = open("/proc/self/pagemap", O_RDONLY);
  assert(pagemap >= 0);


  ////////////////////////finished setting up page mapping///////////////////////////////////
	int arr[arraySize];
	for (int i = 0 ; i < arraySize ; i ++)
	{
		arr[i] = i;
	}
	for (int i = 0; i < arraySize; i += 64/sizeof(int)) {
		size_t setx = get_dram_mapping((void*)get_physical_addr((uint64_t)(arr+i)));
        size_t rowx = get_dram_row((void*)get_physical_addr((uint64_t)(arr+i)));
        if (numValidIndices < 100 && setx == theSet) {
        	validIndices[numValidIndices] = i;
        	numValidIndices++;
        }
    }
    printf("%d valid indies:\n", numValidIndices);

	return 0;
}

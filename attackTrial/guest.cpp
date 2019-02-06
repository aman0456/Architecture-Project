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
#include <string>
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

// long long unsigned int syncTime = 100;
long long unsigned int syncTime = 5e8;
int theSet = 5;
int validIndices [100];
int numValidIndices = 0;
int arraySize = 1024*1024;
int timeInterval = 1;
float fraction_of_physical_memory = 0.2;
int num_reads = 5000;
int MAX_INPUT_LENGTH = 1000;
// int initialTransmit = 0x41f;
int initialTransmit = 0b10000011111;
int lenInitialTransmit = 11;
uint64_t rdtsc() {
	uint64_t a, d;
	asm volatile ("xor %%rax, %%rax\n" "cpuid"::: "rax", "rbx", "rcx", "rdx");
	asm volatile ("rdtscp" : "=a" (a), "=d" (d) : : "rcx");
	a = (d << 32) | a;
	return a;
}

// ----------------------------------------------
uint64_t rdtsc2() {
	uint64_t a, d;
	asm volatile ("rdtscp" : "=a" (a), "=d" (d) : : "rcx");
	asm volatile ("cpuid"::: "rax", "rbx", "rcx", "rdx");
	a = (d << 32) | a;
	return a;
}

void flush(void* p) {
	asm volatile ("clflush 0(%0)\n"
			:
			: "c" (p)
			: "rax");
}

// #define assert(X) do { if (!(X)) { fprintf(stderr,"assertion '" #X "' failed\n"); exit(-1); } } while (0)
int pagemap = -1;
// Extract the physical page number from a Linux /proc/PID/pagemap entry.
uint64_t frame_number_from_pagemap(uint64_t value) {
	return value & ((1ULL << 54) - 1);
}

uint64_t getTiming(void* first) {
	size_t min_res = (-1ull);
	for (int i = 0; i < 4; i++) {
		size_t number_of_reads = num_reads;
		volatile size_t *f = (volatile size_t *) first;
		// volatile size_t *s = (volatile size_t *) second;

		for (int j = 0; j < 10; j++)
			sched_yield();
		size_t t0 = rdtsc();

		while (number_of_reads-- > 0) {
			*f;
			*(f + number_of_reads);

			// *s;
			// *(s + number_of_reads);

			asm volatile("clflush (%0)" : : "r" (f) : "memory");
			// asm volatile("clflush (%0)" : : "r" (s) : "memory");
		}

		uint64_t res = (rdtsc2() - t0) / (num_reads);
		for (int j = 0; j < 10; j++)
			sched_yield();
		if (res < min_res)
			min_res = res;
	}
	return min_res;
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

	void* mapping = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE,
			MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(mapping != (void*)-1);

	// Initialize the mapping so that the pages are non-empty.
	for (uint64_t index = 0; index < mapping_size; index += 0x400) {
		*(uint64_t*) (((char*)mapping) + index) = index * 37;
	}

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
		// printf("set = %u, addr = %lu, phyaddr = %lu\n", setx, arr+i, get_physical_addr((uint64_t)(arr+i)));
		if (numValidIndices < 100 && setx == theSet) {
			validIndices[numValidIndices] = i;
			numValidIndices++;
		}
	}
	assert(numValidIndices > 0);
	int chosenIndex = validIndices[numValidIndices/2];
	int totalWaitTime = 0, totalOuterRep = 100;
	int numrep = 10;
	long long int currentTime = rdtsc()/syncTime;
	char toTransmit[MAX_INPUT_LENGTH];
	printf("Please enter the string to transfer: ");
	scanf("%s", toTransmit);
	int toTransmitlen = 0;
	// int ind = 0;
	while(toTransmit[toTransmitlen] != '\0') toTransmitlen++;
	lenInitialTransmit = initialTransmit.length();
	printf("Sending %s\n", toTransmit);
	// toTransmit = (toTransmit << 4) + 5; // append 101111111111 at the end.
	// toTransmit = (toTransmit << 12) + (3071); // append 101111111111 at the end.
	currentTime = rdtsc()/syncTime;
	while (rdtsc()/syncTime <= currentTime) ;
	int i = 0;
	currentTime = rdtsc()/syncTime;
	for ( ; i < lenInitialTransmit ; i ++) //len of initial Tranmit
	{
		while(rdtsc()/syncTime == currentTime)
		{
			if ((initialTransmit >> i) % 2)
			{
				uint64_t time = getTiming(arr + chosenIndex);
				// printf("time = %lu\n", time);
			}
		}
		currentTime ++;
	}
	printf("sent initial synchronising sequence\n");
	for (i=0 ; i < toTransmitlen ; i ++) //len of initial Tranmit
	{
		int num = toTransmit[i];
		// printf("num : %d\n", num);
		for (int j = 0; j < 8; j++){

			while(rdtsc()/syncTime == currentTime)
			{
				if ((num >> j) % 2)
				{
					uint64_t time = getTiming(arr + chosenIndex);
					// printf("time = %lu\n", time);
				}
			}
			currentTime ++;
		}
		printf("sent %c\n", toTransmit[i]);
	}
	for ( i=0; i < 8 ; i ++) //len of initial Tranmit
	{
		while(rdtsc()/syncTime == currentTime)
		{
			// if ((initialTransmit >> i) % 2)
			// {
			// 	uint64_t time = getTiming(arr + chosenIndex);
			// 	// printf("time = %lu\n", time);
			// }
		}
		currentTime ++;
	}
	printf("sent \\0 for synchronising\n");
	printf("Transmission Done\n");
	return 0;
}

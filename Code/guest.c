#include <stddef.h>
#include <stdint.h>


// Would be used for calling hypercalls
struct Hyper_Call_Type{
	uint32_t HC_num;
	uint32_t data;
};


// Would always use port 250 for it
static void HCALL(uint32_t value) {
	uint16_t port = 250;
	asm("outl %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

void HC_print8bit(uint8_t val)
{
	outb(0xE9, val);
}

void HC_print32bit(uint32_t val)
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 1;
	hcall.data = val;
	HCALL((uint32_t)(intptr_t)&(hcall));
}

uint32_t HC_numExits()
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 2;
	HCALL((uint32_t)(intptr_t)&(hcall));
	return hcall.data;
}

void HC_printStr(char *str)
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 3;
	hcall.data = (uint32_t)(intptr_t)str;
	HCALL((uint32_t)(intptr_t)&(hcall));
}

// Creating buffer to get input from HC_numExitsByType()
char data1[50];
char data2[50];
int call_num = 0;
char *HC_numExitsByType()
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 4;
	if(call_num == 0){
		hcall.data = (uint32_t)(intptr_t)data1;
		HCALL((uint32_t)(intptr_t)&(hcall));
		call_num += 1;
		return data1;
	}
	else if(call_num == 1){
		hcall.data = (uint32_t)(intptr_t)data2;
		HCALL((uint32_t)(intptr_t)&(hcall));
		return data2;
	}
	else{
		// this is assuming function won't be called more than 2 times as said in moodle answer by prof
		return NULL;
	}
	
}

uint32_t HC_gvaToHva(uint32_t gva)
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 5;
	hcall.data = gva;
	HCALL((uint32_t)(intptr_t)&(hcall));
	return hcall.data;
}

void HC_Check()
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 6;
	HCALL((uint32_t)(intptr_t)&(hcall));
}
void HC_Sleep()
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 7;
	HCALL((uint32_t)(intptr_t)&(hcall));
}

void HC_DoNothing(uint32_t val)
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 8;
	hcall.data = val;
	HCALL((uint32_t)(intptr_t)&(hcall));
}

void HC_DoNothing2()
{
	// Fill in here
	struct Hyper_Call_Type hcall;
	hcall.HC_num = 9;
	// hcall.data = val;
	HCALL((uint32_t)(intptr_t)&(hcall));
}

char ramesh[496*(1<<16)];



// Define a simple pseudo-random number generator
int custom_rand(int max_range, unsigned int *seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff; // Linear congruential generator
    return *seed % (max_range);
}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {

	// ramesh[0] = 23;
	// ramesh[999] = 23;
	// ramesh[1999] = 23;

	// HC_print32bit((uint32_t)(intptr_t)&ramesh[0]);

    int max_range = 496*(1<<16);
    unsigned int seed = 42; // Change this to your desired seed value
    

	while(1){

        int random_number = custom_rand(max_range, &seed);
        ramesh[random_number] = 23;
		// for(int i = 0; i<496*(1<<16);i++){
		// 	// HC_print32bit(i);
		// 	HC_DoNothing2();
		// 	ramesh[i] = 23;
		// }
	}

	


	// /*----------Don't modify this section. We will use grading script---------*/
	// /*---Your submission will fail the testcases if you modify this section---*/
	// const char *p;

	// for (p = "Hello 695!\n"; *p; ++p)
	// 	HC_print8bit(*p);
		
	// HC_print32bit(2048);
	// HC_print32bit(4294967295);

	// uint32_t num_exits_a, num_exits_b;
	// num_exits_a = HC_numExits();

	// char *str = "CS695 Assignment 2\n";
	// HC_printStr(str);

	// num_exits_b = HC_numExits();

	// HC_print32bit(num_exits_a);
	// HC_print32bit(num_exits_b);

	// char *firststr = HC_numExitsByType();
	// uint32_t hva;
	// hva = HC_gvaToHva(1024);
	// HC_print32bit(hva);
	// hva = HC_gvaToHva(4294967295);
	// HC_print32bit(hva);
	// char *secondstr = HC_numExitsByType();

	// HC_printStr(firststr);
	// HC_printStr(secondstr);
	/*------------------------------------------------------------------------*/

	*(long *) 0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a" (42) : "memory");
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <time.h>

#include <pthread.h>
#include <unistd.h> 

#define MAX(a, b) ((a) > (b) ? (a) : (b))


/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)


struct vm {
	int dev_fd;
	int vm_fd;
	char *mem;
};

struct vcpu {
	int vcpu_fd;
	struct kvm_run *kvm_run;
};

void vm_init(struct vm *vm, size_t mem_size)
{
	int kvm_version;
	struct kvm_userspace_memory_region memreg;

	vm->dev_fd = open("/dev/kvm", O_RDWR);
	if (vm->dev_fd < 0) {
		perror("open /dev/kvm");
		exit(1);
	}

	kvm_version = ioctl(vm->dev_fd, KVM_GET_API_VERSION, 0);
	if (kvm_version < 0) {
		perror("KVM_GET_API_VERSION");
		exit(1);
	}

	if (kvm_version != KVM_API_VERSION) {
		fprintf(stderr, "Got KVM api version %d, expected %d\n",
			kvm_version, KVM_API_VERSION);
		exit(1);
	}

	vm->vm_fd = ioctl(vm->dev_fd, KVM_CREATE_VM, 0);
	if (vm->vm_fd < 0) {
		perror("KVM_CREATE_VM");
		exit(1);
	}

        if (ioctl(vm->vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
                perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (vm->mem == MAP_FAILED) {
		perror("mmap mem");
		exit(1);
	}

	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	memreg.slot = 0;
	memreg.flags = KVM_MEM_LOG_DIRTY_PAGES;
	memreg.guest_phys_addr = 0;
	memreg.memory_size = mem_size;
	memreg.userspace_addr = (unsigned long)vm->mem;
        if (ioctl(vm->vm_fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
		perror("KVM_SET_USER_MEMORY_REGION");
                exit(1);
	}
}

void vcpu_init(struct vm *vm, struct vcpu *vcpu)
{
	int vcpu_mmap_size;

	vcpu->vcpu_fd = ioctl(vm->vm_fd, KVM_CREATE_VCPU, 0);
        if (vcpu->vcpu_fd < 0) {
		perror("KVM_CREATE_VCPU");
                exit(1);
	}

	vcpu_mmap_size = ioctl(vm->dev_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
        if (vcpu_mmap_size <= 0) {
		perror("KVM_GET_VCPU_MMAP_SIZE");
                exit(1);
	}

	vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
			     MAP_SHARED, vcpu->vcpu_fd, 0);
	if (vcpu->kvm_run == MAP_FAILED) {
		perror("mmap kvm_run");
		exit(1);
	}
}

// similar to struct made in guest.c
struct Hyper_Call_Type{
	uint32_t HC_num;
	uint32_t data;
};

void read_dirty_bitmap(struct vm *vm, struct kvm_dirty_log *dirty_log, int number_of_pages) {
	dirty_log->dirty_bitmap = (char*)malloc(number_of_pages * sizeof(char));
    if (ioctl(vm->vm_fd, KVM_GET_DIRTY_LOG, dirty_log) < 0) {
        perror("KVM_GET_DIRTY_LOG");
        exit(1);
    }
}

// HELPER FUNCTIONS TO CREATE A BST BASED SET
// Define the node structure for the binary search tree
struct Node {
    int value;
    struct Node* left;
    struct Node* right;
};
 
// Function to create a new node
struct Node* createNode(int value) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->value = value;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}
 
// Function to insert a value into the BST
int insert(struct Node** root, int value) {
    // If the tree is empty, create a new node and make it the root
    if (*root == NULL) {
        *root = createNode(value);
        return 0;
    }
 
    // Traverse the tree to find the appropriate position to insert the value
    struct Node* current = *root;
    while (current != NULL) {
        // If the value is already in the set, return -1
        if (current->value == value) {
            return -1;
        }
        // If the value is less than the current node's value, go left
        else if (value < current->value) {
            if (current->left == NULL) {
                current->left = createNode(value);
                return 0;
            }
            current = current->left;
        }
        // If the value is greater than the current node's value, go right
        else {
            if (current->right == NULL) {
                current->right = createNode(value);
                return 0;
            }
            current = current->right;
        }
    }
    return 0;
}

// Function to clear the BST

void clear(struct Node* root){ 
    if (root == NULL){
        return; 
    }

    clear(root->left);
    clear(root->right);
    free(root);
}

// fills the 'sample' array (of size 'sample_size') with random samples (in range [0, num_pages - 1])
void generate_random_sample(int * sample, int sample_size, int num_pages){
	struct Node* root = NULL;
	int ctr = 0;

	while(ctr!=sample_size){
		int random = rand() % (num_pages);
		if (insert(&root, random) == -1) {
			continue;
		}
		sample[ctr] = random;
		ctr++;
	}

	// clearing the temp data structure
	clear(root);
}

double slow_moving_frac = 0;
int slow_moving_wse = 0;
double fast_moving_frac = 0;
int fast_moving_wse = 0;

int get_sample_accessed(char* dirty_bitmap, int * sample, int sample_size){
	int accessed = 0;
	for(int i = 0;i < sample_size; i++){
		char curr_bit = ((dirty_bitmap[sample[i] / 8]) >> (sample[i] % 8)) & 1;
		if(curr_bit == 1){
			accessed++;
		}
	}
	return accessed;
}


void* working_set_calculator(void* arg) {
    struct vm *vm = (struct vm *)arg;

    struct kvm_dirty_log dirty_log;

	int number_of_pages = 1 << 13;

	int sample_size = 100;

	int memory_size = 1<<25;

	int page_size = 1<<12;

	int sampling_period = 50;



	fprintf(stderr, "CONFIGURATION PARAMETERS\n");
	fprintf(stderr, "TOTAL NUMBER OF PAGES : \"%d\"\n", number_of_pages);
	fprintf(stderr, "TOTAL MEMORY SIZE : \"%d\" Bytes\n", memory_size);
	fprintf(stderr, "PAGE SIZE : \"%d\" Bytes\n", page_size);
	fprintf(stderr, "SAMPLE SIZE : \"%d\" pages\n", sample_size);
	fprintf(stderr, "SAMPLING PERIOD : \"%d\" seconds\n", sampling_period);

	fprintf(stderr, "\n\n");
	int ctr = 0;
	
	while(1){
		ctr++;
		sleep(sampling_period);

		dirty_log.slot = 0;
		read_dirty_bitmap(vm, &dirty_log, number_of_pages);
		// Process the dirty bitmap, update dirty pages information
		// fprintf(stderr, "Reading dirty bitmap for slot %u\n", dirty_log.slot);

		int sample[sample_size];

		generate_random_sample(sample, sample_size, number_of_pages);

		int total_accessed = 0;
		for (int i = 0; i < number_of_pages; i++) {
			for (int j = 7; j >= 0; j--) {
				char curr_bit = (((char*)(dirty_log.dirty_bitmap))[i] >> j) & 1;
				if(curr_bit == 1) total_accessed++;
			}
		}

		// ACTUAL VALUES
		double frac_accessed = (((double)total_accessed)/((double)number_of_pages));
		int actual_wse = (int)(frac_accessed * memory_size);

		// SAMPLE
		int sample_accessed = get_sample_accessed((char*)dirty_log.dirty_bitmap, sample, sample_size);
		double sample_frac_accessed = (((double)sample_accessed)/((double)sample_size));
		int sample_wse = (int)(sample_frac_accessed * memory_size);

		// MOVING AVERAGE
		if(ctr==1){
			// initialising these values

			slow_moving_frac = sample_frac_accessed;
			fast_moving_frac = sample_frac_accessed;

			slow_moving_wse = sample_wse;
			fast_moving_wse = sample_wse;
		}
		else{
			slow_moving_frac = slow_moving_frac*0.9 + sample_frac_accessed*0.1;
			fast_moving_frac = fast_moving_frac*0.1 + sample_frac_accessed*0.9;

			slow_moving_wse = slow_moving_wse*0.9 + sample_wse*0.1;
			fast_moving_wse = fast_moving_wse*0.1 + sample_wse*0.9;
		}
		

		double max_frac;
		int max_wse;

		max_frac = MAX(slow_moving_frac, fast_moving_frac);
		max_frac = MAX(max_frac, sample_frac_accessed);

		max_wse = MAX(slow_moving_wse, fast_moving_wse);
		max_wse = MAX(max_wse, sample_wse);

		fprintf(stderr, "RUN \"%d\"\n", ctr);

		fprintf(stderr, "ACTUAL         : Number of pages accessed are : \"%d\" \n",total_accessed);
		fprintf(stderr, "CURRENT SAMPLE : Number of pages accessed are : \"%d\" \n",sample_accessed);

		fprintf(stderr, "\n");

		fprintf(stderr, "ACTUAL         : Percentage of pages accessed is : \"%f\" %% \n",frac_accessed*100.0);
		fprintf(stderr, "CURRENT SAMPLE : Percentage of pages accessed is : \"%f\" %% \n", sample_frac_accessed*100.0);
		fprintf(stderr, "SLOW MOVING AVG: Percentage of pages accessed is : \"%f\" %% \n", slow_moving_frac*100.0);
		fprintf(stderr, "FAST MOVING AVG: Percentage of pages accessed is : \"%f\" %% \n", fast_moving_frac*100.0);
		fprintf(stderr, "MAX ESTIMATE   : Percentage of pages accessed is : \"%f\" %% \n", max_frac*100.0);

		fprintf(stderr, "\n");

		fprintf(stderr, "ACTUAL         : Working set estimate is : \"%d\" Bytes \n", actual_wse);
		fprintf(stderr, "CURRENT SAMPLE : Working set estimate is : \"%d\" Bytes \n", sample_wse);
		fprintf(stderr, "SLOW MOVING AVG: Working set estimate is : \"%d\" Bytes \n", slow_moving_wse);
		fprintf(stderr, "FAST MOVING AVG: Working set estimate is : \"%d\" Bytes \n", fast_moving_wse);
		fprintf(stderr, "MAX ESTIMATE   : Working set estimate is : \"%d\" Bytes \n", max_wse);

		fprintf(stderr, "\n");

		// ERROR STATISTICS
		fprintf(stderr, "CURRENT SAMPLE : Error in Percentage of pages accessed is : \"%f\" %% \n", (sample_frac_accessed-frac_accessed)*100.0);
		fprintf(stderr, "SLOW MOVING AVG: Error in Percentage of pages accessed is : \"%f\" %% \n", (slow_moving_frac-frac_accessed)*100.0);
		fprintf(stderr, "FAST MOVING AVG: Error in Percentage of pages accessed is : \"%f\" %% \n", (fast_moving_frac-frac_accessed)*100.0);
		fprintf(stderr, "MAX ESTIMATE   : Error in Percentage of pages accessed is : \"%f\" %% \n", (max_frac-frac_accessed)*100.0);


		fprintf(stderr, "\n");

		fprintf(stderr, "CURRENT SAMPLE : Error in working set estimate is : \"%d\" Bytes \n", sample_wse - actual_wse);
		fprintf(stderr, "SLOW MOVING AVG: Error in working set estimate is : \"%d\" Bytes \n", slow_moving_wse - actual_wse);
		fprintf(stderr, "FAST MOVING AVG: Error in working set estimate is : \"%d\" Bytes \n", fast_moving_wse - actual_wse);
		fprintf(stderr, "MAX ESTIMATE   : Error in working set estimate is : \"%d\" Bytes \n", max_wse - actual_wse);


		fprintf(stderr, "\n");


		fprintf(stderr, "\n\n");
	}

	// BELOW WAS TO CHECK IF BITMAP IS CLEARED WHEN WE READ IT

	// printf("READING AGAIN-----------------------\n");

	// read_dirty_bitmap(vm, &dirty_log);
	// // Process the dirty bitmap, update dirty pages information
	// printf("Reading dirty bitmap for slot %u\n", dirty_log.slot);

	// tot = 0;
	// for (int i = 0; i < 64; i++) {
	// 	for (int j = 7; j >= 0; j--) {
	// 		char curr_bit = (((char*)(dirty_log.dirty_bitmap))[i] >> j) & 1;
	// 		if(curr_bit == 1) tot++;
	// 	}
	// }
	// printf("Number of pages accessed are : %d\n",tot);
	// printf("\n\n");

	// sleep(3);


    return NULL;
}

int run_vm(struct vm *vm, struct vcpu *vcpu, size_t sz)
{
	struct kvm_regs regs;
	uint64_t memval = 0;

	int numExits_IN = 0;
	int numExits_OUT = 0;

	pthread_t thread;
    
    pthread_create(&thread, NULL, working_set_calculator, (void*)(vm));

	for (;;) {
		if (ioctl(vcpu->vcpu_fd, KVM_RUN, 0) < 0) {
			perror("KVM_RUN");
			exit(1);
		}

		switch (vcpu->kvm_run->exit_reason) {
		case KVM_EXIT_HLT:
			goto check;

		case KVM_EXIT_IO:
			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT
			    && vcpu->kvm_run->io.port == 0xE9) {
				// Updating the count of appropriate numExits
				numExits_OUT += 1;
				char *p = (char *)vcpu->kvm_run;
				fwrite(p + vcpu->kvm_run->io.data_offset,
				       vcpu->kvm_run->io.size, 1, stdout);
				fflush(stdout);
				continue;
			}

			else if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT
			    && vcpu->kvm_run->io.port == 250) {
				char *p = (char *)(vcpu->kvm_run );
				uint32_t* addr_GVA = (uint32_t*)(p + vcpu->kvm_run->io.data_offset);

				// Getting GPA for given GVA
				struct kvm_translation get_HPA;
				get_HPA.linear_address = *addr_GVA;
				if(ioctl(vcpu->vcpu_fd, KVM_TRANSLATE, &get_HPA) == -1){
					printf("IOCTL failed for KVM_TRANSLATE\n");
				}

				// Finding HVA from GPA which is stored in get_HPA.physical_address
				struct Hyper_Call_Type* addr_HVA = (struct Hyper_Call_Type*)(vm->mem + get_HPA.physical_address);

				if(addr_HVA->HC_num == 1){
					// Updating the count of appropriate numExits
					numExits_OUT += 1;

					// Printing the 32 bit data passed
					printf("%u\n", addr_HVA->data);
				}
				else if(addr_HVA->HC_num == 2){
					// Updating the count of appropriate numExits
					numExits_IN += 1;

					// Updating the data field of struct 
					addr_HVA->data = (uint32_t)(numExits_IN + numExits_OUT);
				}
				else if(addr_HVA->HC_num == 3){
					// Updating the count of appropriate numExits
					numExits_OUT += 1;

					// Getting string GPA for given string GVA
					struct kvm_translation get_string_HPA;
					get_string_HPA.linear_address = addr_HVA->data;
					if(ioctl(vcpu->vcpu_fd, KVM_TRANSLATE, &get_string_HPA) == -1){
						printf("IOCTL failed for KVM_TRANSLATE\n");
					}

					// Finding string HVA from string GPA which is stored in get_string_HPA.physical_address
					char* addr_string_HVA = (char*)(vm->mem + get_string_HPA.physical_address);

					// Printing the string passed
					printf("%s", addr_string_HVA);
				}
				else if(addr_HVA->HC_num == 4){
					// Updating the count of appropriate numExits
					numExits_IN += 1;

					// Getting string GPA for given string GVA
					struct kvm_translation get_string_HPA;
					get_string_HPA.linear_address = addr_HVA->data;
					if(ioctl(vcpu->vcpu_fd, KVM_TRANSLATE, &get_string_HPA) == -1){
						printf("IOCTL failed for KVM_TRANSLATE\n");
					}

					// Finding string HVA from string GPA which is stored in get_string_HPA.physical_address
					char* addr_string_HVA = (char*)(vm->mem + get_string_HPA.physical_address);

					// Copying data into the Guest VM passed buffer
					sprintf(addr_string_HVA, "IO in: %d\nIO out: %d\n", numExits_IN, numExits_OUT);
				}
				else if(addr_HVA->HC_num == 5){
					// Updating the count of appropriate numExits
					numExits_IN += 1;

					// Getting adrress in GPA for given address in GVA
					struct kvm_translation get_addr_HPA;
					get_addr_HPA.linear_address = addr_HVA->data;
					if(ioctl(vcpu->vcpu_fd, KVM_TRANSLATE, &get_addr_HPA) == -1){
						printf("IOCTL failed for KVM_TRANSLATE\n");
					}
					if(get_addr_HPA.valid == 0){
						printf("Invalid GVA\n");
						addr_HVA->data = 0;
					}
					else{
						addr_HVA->data = (uint32_t)(intptr_t)(vm->mem + get_addr_HPA.physical_address);
					}
				}
				else if(addr_HVA->HC_num == 6){
					working_set_calculator(vm);
				}
				else if(addr_HVA->HC_num == 7){
					sleep(1);
				}
				else if(addr_HVA->HC_num == 8){
				}
				else if(addr_HVA->HC_num == 9){
				}
				else{
					printf("Incorrect hypercall\n");
				}
				continue;	
			}

			/* fall through */
		default:
			fprintf(stderr,	"Got exit_reason %d,"
				" expected KVM_EXIT_HLT (%d)\n",
				vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
			exit(1);
		}
	}

 check:
	if (ioctl(vcpu->vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		perror("KVM_GET_REGS");
		exit(1);
	}

	if (regs.rax != 42) {
		printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
		return 0;
	}

	memcpy(&memval, &vm->mem[0x400], sz);
	if (memval != 42) {
		printf("Wrong result: memory at 0x400 is %lld\n",
		       (unsigned long long)memval);
		return 0;
	}

	return 1;
}

extern const unsigned char guest16[], guest16_end[];

int run_real_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing real mode\n");

        if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	sregs.cs.selector = 0;
	sregs.cs.base = 0;

        if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest16, guest16_end-guest16);
	return run_vm(vm, vcpu, 2);
}

static void setup_protected_mode(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 1,
		.s = 1, /* Code/data */
		.l = 0,
		.g = 1, /* 4KB granularity */
	};

	sregs->cr0 |= CR0_PE; /* enter protected mode */

	sregs->cs = seg;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

extern const unsigned char guest32[], guest32_end[];

int run_protected_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing protected mode\n");

        if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

        if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	// Change as suggested by the moodle query answer
	regs.rsp = 2 << 20;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest32, guest32_end-guest32);
	return run_vm(vm, vcpu, 4);
}

static void setup_paged_32bit_mode(struct vm *vm, struct kvm_sregs *sregs)
{
	uint32_t pd_addr = 0x2000;
	uint32_t *pd = (void *)(vm->mem + pd_addr);

	/* A single 4MB page to cover the memory region */
	pd[0] = PDE32_PRESENT | PDE32_RW | PDE32_USER | PDE32_PS;
	/* Other PDEs are left zeroed, meaning not present. */

	sregs->cr3 = pd_addr;
	sregs->cr4 = CR4_PSE;
	sregs->cr0
		= CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs->efer = 0;
}

int run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing 32-bit paging\n");

        if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);
	setup_paged_32bit_mode(vm, &sregs);

        if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	// Change as suggested by the moodle query answer
	regs.rsp = 2 << 20;

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest32, guest32_end-guest32);
	return run_vm(vm, vcpu, 4);
}

extern const unsigned char guest64[], guest64_end[];

static void setup_64bit_code_segment(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 0,
		.s = 1, /* Code/data */
		.l = 1,
		.g = 1, /* 4KB granularity */
	};

	sregs->cs = seg;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}


static void setup_long_mode(struct vm *vm, struct kvm_sregs *sregs){

	uint64_t pml4_addr = (2 << 24) - (1 << 12);
    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    uint64_t pdpt_addr = (2 << 24) - (1 << 13);
    uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

    uint64_t pd_addr = (2 << 24) - 3*(1 << 12);
    uint64_t *pd = (void *)(vm->mem + pd_addr);

	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
    pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;

	for(int num = 0; num < 16; num++){
		uint64_t pt_addr = (2 << 24) - (4+num)*(1 << 12);  // Address of page table
		uint64_t *pt = (void *)(vm->mem + pt_addr);
    	pd[num] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (pt_addr);  // Set to page table address

		// Set up page table entries for 4 KB pages
		for (int i = 0; i < 512; ++i) {
			pt[i] = ((i * 0x1000)+(num*512*(1<<12))) | PDE64_PRESENT | PDE64_RW | PDE64_USER;
		}

		// pt[1] = pt[1] & ~(PDE64_PRESENT);

	}
    sregs->cr3 = pml4_addr;
    sregs->cr4 = CR4_PAE | CR4_PSE;  // Enable PSE (Page Size Extension) to allow 4 KB pages
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs->efer = EFER_LME | EFER_LMA;

    setup_64bit_code_segment(sregs);
}


int run_long_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;


        if (ioctl(vcpu->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_long_mode(vm, &sregs);

        if (ioctl(vcpu->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	/* Create stack at top of 2 MB page and grow down. */
	regs.rsp = (2 << 24) - 20*(1 << 12);

	if (ioctl(vcpu->vcpu_fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest64, guest64_end-guest64);
	return run_vm(vm, vcpu, 8);
}


int main(int argc, char **argv)
{
	struct vm vm;
	struct vcpu vcpu;
	enum {
		REAL_MODE,
		PROTECTED_MODE,
		PAGED_32BIT_MODE,
		LONG_MODE,
	} mode = REAL_MODE;
	int opt;

	srand(time(NULL));


	while ((opt = getopt(argc, argv, "rspl")) != -1) {
		switch (opt) {
		case 'r':
			mode = REAL_MODE;
			break;

		case 's':
			mode = PROTECTED_MODE;
			break;

		case 'p':
			mode = PAGED_32BIT_MODE;
			break;

		case 'l':
			mode = LONG_MODE;
			break;

		default:
			fprintf(stderr, "Usage: %s [ -r | -s | -p | -l ]\n",
				argv[0]);
			return 1;
		}
	}

	vm_init(&vm, 0x2000000);
	vcpu_init(&vm, &vcpu);

	switch (mode) {
	case REAL_MODE:
		return !run_real_mode(&vm, &vcpu);

	case PROTECTED_MODE:
		return !run_protected_mode(&vm, &vcpu);

	case PAGED_32BIT_MODE:
		return !run_paged_32bit_mode(&vm, &vcpu);

	case LONG_MODE:
		return !run_long_mode(&vm, &vcpu);
	}

	return 1;
}

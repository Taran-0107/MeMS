/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions 
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include<stdio.h>
#include<stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this 
macro to make the output of all system same and conduct a fair evaluation. 
*/
#define PAGE_SIZE 4096

struct chain_node {
    struct chain_node* next;
    struct chain_node* prev;
    int size;
    int state;
    void* V_addr;
    void* P_addr;
};

struct main_node {
    struct main_node* next;
    struct main_node* prev;
    struct chain_node* ch;
    int size;
};

typedef struct chain_node chain_node;
typedef struct main_node main_node;

main_node* main_nodes;
chain_node* chain_nodes;

int main_node_index = 0;
int chain_node_index = 0;
int fd;

main_node* ptr;
main_node* head;
void* sVadd;

main_node* create_main_node(main_node* next, main_node* prev, chain_node* ch, int size) {
    if(main_node_index+1>PAGE_SIZE/sizeof(main_node)){
        main_nodes = (main_node*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        main_node_index=0;
    }
    main_node* m = &main_nodes[main_node_index++];
    m->next = next;
    m->prev = prev;
    m->ch = ch;
    m->size = size;
    return m;
}

chain_node* create_chain_node(chain_node* next, chain_node* prev, int size, int state, void* vadd, void* padd) {
    if(chain_node_index+1>PAGE_SIZE/sizeof(chain_node)){
        chain_nodes = (chain_node*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        chain_node_index=0;
    }
    chain_node* m = &chain_nodes[chain_node_index++];
    m->next = next;
    m->prev = prev;
    m->size = size;
    m->state = state;
    m->V_addr = vadd;
    m->P_addr = padd;
    return m;
}

/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_init() {
    fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    main_nodes = (main_node*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    chain_nodes = (chain_node*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    main_node* h = create_main_node(NULL, NULL, NULL, 0);
    h->size = 0;
    ptr = h;
    head = h;
    sVadd = 0;
    sVadd+=1000;
}

/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish() {
    main_node* mstart = head;
    while (mstart != NULL) {
        if (mstart->ch != NULL) {
            munmap(mstart->ch->P_addr, mstart->size);
        }
        mstart = mstart->next;
    }
    munmap(main_nodes, PAGE_SIZE);
    munmap(chain_nodes, PAGE_SIZE);
}


/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available. 

Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/ 

void* mems_malloc(size_t size) {
    int c1 = 1;
    void* temp_vadd = sVadd;
    main_node*ptr2=head;
    while(ptr2!=NULL){
        if (ptr2->ch != NULL) {
            chain_node* start = ptr2->ch;
            while (start != NULL) {
                if (start->state == 0 && start->size >= size) {
                    start->state = 1;
                    if (start->size > size) {
                        chain_node* n = create_chain_node(start->next, start, start->size - size, 0, start->V_addr + size, start->P_addr + size);
                        if (start->next != NULL) {
                            start->next->prev = n;
                        }
                        start->next = n;
                    }
                    start->size = size;
                    c1 = 0;
                    return start->V_addr;
                }
                temp_vadd = start->V_addr + start->size;
                start = start->next;
            }
        }
        ptr2=ptr2->next;
    }

    if (c1) {
        int ts = size + (PAGE_SIZE - size % PAGE_SIZE) % PAGE_SIZE;
        void* temp_padd = mmap(NULL, ts, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (temp_padd == MAP_FAILED) {
            perror("mmap");
            return NULL;
        }
        chain_node* temp = create_chain_node(NULL, NULL, size, 1, temp_vadd, temp_padd);
        if (ts - size != 0) {
            chain_node* n = create_chain_node(NULL, temp, ts - size, 0, temp_vadd + size, temp_padd + size);
            temp->next = n;
        }
        ptr->next = create_main_node(NULL, ptr, temp, ts);
        ptr = ptr->next;
        return temp->V_addr;
    }
    return NULL;
}

/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
void mems_print_stats() {
    int count = 1;
    int pages = 0;
    int unused = 0;
    main_node* mstart = head->next;
    printf("\n****---------MeMS System Stats----------****\n");
    while (mstart != NULL) {
        int count2 = 1;
        chain_node* cstart = mstart->ch;
        printf("Main chain node %d [size: %d bytes(%d pages)]: \n", count, mstart->size, mstart->size / PAGE_SIZE);
        while (cstart != NULL) {
            char* h = "HOLE";
            if (cstart->state == 1) {
                h = "PROCESS";
            } else {
                unused += cstart->size;
            }
            void* start_addr = cstart->V_addr;
            void* end_addr = cstart->V_addr + cstart->size-1;
            printf("    sub chain %d | size %d | %s | [%lu:%lu]\n", count2, cstart->size, h, (unsigned long)start_addr, (unsigned long)end_addr);
            count2++;
            cstart = cstart->next;
        }
        pages += mstart->size / PAGE_SIZE;
        count++;
        mstart = mstart->next;
    }
    printf("pages used: %d\n", pages);
    printf("unused memory: %d bytes\n", unused);
}

/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void* mems_get(void* v_ptr) {
    main_node* mstart = head;
    while (mstart != NULL) {
        chain_node* cstart = mstart->ch;
        while (cstart != NULL) {
            if (v_ptr < cstart->V_addr + cstart->size - 1) {
                return cstart->P_addr + (unsigned long)(v_ptr - cstart->V_addr);
            }
            cstart = cstart->next;
        }
        mstart = mstart->next;
    }
    return NULL;
}


/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
void mems_free(void* v_ptr) {
    main_node* mstart = head;
    while (mstart != NULL) {
        chain_node* cstart = mstart->ch;
        while (cstart != NULL) {
            if (v_ptr < cstart->V_addr + cstart->size - 1) {
                cstart->state = 0;
                if (cstart->next != NULL) {
                    if (cstart->next->state == 0) {
                        cstart->size += cstart->next->size;
                        cstart->next = cstart->next->next;
                    }
                }
                if (cstart->prev != NULL) {
                    if (cstart->prev->state == 0) {
                        cstart->prev->size += cstart->size;
                        cstart->prev->next = cstart->next;
                        if (cstart->next != NULL) {
                            cstart->next->prev = cstart->prev;
                        }
                    }
                }
                return;
            }
            cstart = cstart->next;
        }
        mstart = mstart->next;
    }
}

# MeMS
Memory Management System( OS Project IIITD 2nd Year ,section B)

below are definitions of the functions defined in the code:
    mems_init():
        This function initializes the MeMS system.
        It opens the /dev/zero file and prepares the file descriptor (fd) for mapping memory.
        It creates the initial main node (head) and sets up the initial state of the MeMS system.
        This function also initializes the main_nodes and chain_nodes arrays to manage main and chain nodes.

    mems_finish():
        This function is called at the end of the MeMS system to clean up and release any memory used.
        It unmaps the memory segments allocated for the chain nodes.

    mems_malloc(size_t size):
        This function allocates memory of the specified size.
        It checks if there is any free memory segment in the existing chain nodes and reuses it if it's large enough.
        If there's no suitable free memory, it uses the mmap system call to allocate a new memory segment and updates the free list accordingly.
        The function returns a MeMS virtual address (allocated memory).

    mems_print_stats():
        This function prints statistics about the MeMS system.
        It displays information about the pages used and unused memory.
        It lists details about each main chain node and its sub-chain segments (PROCESS or HOLE).
        It also prints the starting and ending addresses of each memory segment.

    mems_get(void* v_ptr):
        This function takes a MeMS virtual address as input and returns the corresponding MeMS physical address.
        It searches through the main chain and its sub-chains to find the mapping.

    mems_free(void* v_ptr):
        This function frees memory pointed to by the MeMS virtual address.
        It marks the corresponding memory segment as unused in the sub-chain.
        If the adjacent segments are also unused, it merges them into a single larger free segment.

    create_main_node() and create_chain_node():
        These functions are helper functions to create main and chain nodes.
        They ensure that when the max index of the main or chain node arrays is reached, new memory segments are allocated for additional nodes.

The code also includes macros for defining the PAGE_SIZE, and it properly manages the free list to allocate and release memory segments.

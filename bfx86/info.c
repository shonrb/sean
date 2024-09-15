// Helper C program to get the values for some relevant syscall constants
#include <sys/stat.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    printf("struct stat size: %ld\n", 
           sizeof(struct stat));
    printf("size field offset: %ld\n", 
           offsetof(struct stat, st_size));
    printf("size field size: %ld\n",
            sizeof(off_t));
            
    printf("PROT_READ: %d\n", PROT_READ);
    printf("PROT_WRITE: %d\n", PROT_WRITE);
    printf("MAP_ANONYMOUS: %d\n", MAP_ANONYMOUS);
    printf("MAP_PRIVATE: %d\n", MAP_PRIVATE);
}
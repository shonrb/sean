/*
The original challenge is as follows:

    - To write a set of functions for managing a variable number of byte queues
    - Each queue can have a variable length
    - It is assumed the amount of available memory is small and fixed

The interface for the required function set is defined as:

    - Q * create_queue(); 
      Creates a FIFO byte queue, returning a handle to it.

    - void destroy_queue(Q * q); 
      Destroy an earlier created byte queue.

    - void enqueue_byte(Q * q, unsigned char b); 
      Adds a new byte to a queue.

    - unsigned char dequeue_byte(Q * q); 
      Pops the next byte off the FIFO queue.

The type Q presented in the interface description can be defined in 
any way the implementation requires.

The code is not allowed to call malloc() or other heap management routines. 
Instead, all storage (other than local variables in the functions) 
must be within a provided array: 

    - unsigned char data[2048];

Other requirements:

    - Memory efficiency is important. On average while the system is running, 
      there will be about 15 queues with an average of 80 or so bytes in each.

    - The functions may be asked to create a larger number of queues with 
      less bytes in each, or to create a smaller number of queues with more 
      bytes in each.

    - Execution speed is important. Worst-case performance when adding and 
      removing bytes is more important than average-case performance.

    - If a request cannot be satisfied due to lack of memory, the code 
      should call a provided failure function, which will not return to the caller:

        - void on_out_of_memory();

    - Similarly if the caller makes an illegal request, like attempting to dequeue 
      a byte from an empty queue, a provided failure function should be invoked, 
      which will not return back:

        - void on_illegal_operation();

    - There may be spikes in the number of queues allocated, or in the size 
      of an individual queue.

    - The code should not assume a maximum number of bytes in a queue (other 
      than that imposed by the total amount of memory available, of course!). 
      It can be assumed that no more than 64 queues will ever be created at once.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define DATA_SIZE  2048

// The data will be indexed in 8 byte "blocks"
#define BLOCK_SIZE sizeof(long)
#define NUM_BLOCKS (DATA_SIZE / BLOCK_SIZE)
#define BYTES_IN_NODE 6

// Each queue node contains a smaller byte queue with a max length of 6.
// Doing this gives good space efficiency.
struct node {
    unsigned char next;
    unsigned char length;
    unsigned char bytes[6];
};

struct queue {
    unsigned char  head;
    unsigned char  tail;
    unsigned char  is_empty;
    unsigned char  new_node_needed;
    int            _unused;
};


unsigned char data[DATA_SIZE] = {0};

typedef struct queue Q;

Q * create_queue(); // Creates a FIFO byte queue, returning a handle to it.
void destroy_queue(Q * q); // Destroy an earlier created byte queue.
void enqueue_byte(Q * q, unsigned char b); // Adds a new byte to a queue.
unsigned char dequeue_byte(Q * q); // Pops the next byte off the FIFO queue.
void on_out_of_memory();
void on_illegal_operation();

/* MEMORY FUNCTIONS
 * For dealing with allocation, deallocation, reading and indexing.
 */

/* Get a pointer to the (index)th block of memory.
 */
void *mem_read(unsigned index) 
{
    return (long*) data + index;
}

/* Get the index of a given block.
 */
unsigned char get_mem_index(void *node) 
{
    return ((long*) node - (long*) data);
}

/* Get a pointer to the first free block of memory in 
 * the data array, and reassign first_free to the next free block.
 */
void *mem_allocate() 
{
    long *first_free = mem_read(0);

    // if the first_free pointer is uninitialised, initialise it
    if (*first_free == 0)
        *first_free = 1;
    else if (*first_free == NUM_BLOCKS)
        on_out_of_memory();
    
    // Get the space at first_free, which is guaranteed to be free
    long *res = mem_read(*first_free);

    // March the first_free index forward until a free space is found 
    // or it goes out of bounds. The next call will fail if it goes out of bounds.
    for (;;) {
        ++*first_free;
        long *block = mem_read(*first_free);

        if (*first_free == NUM_BLOCKS || *block == 0)
            break;
    }

    return res;
}

/* Sets a block to zero, and reassigns first_free if needed. 
 */
void mem_free(void *mem) 
{
    long *block = (long*) mem;
    *block = 0;

    // If the newly freed space is before
    // first_free, replace first_free.
    unsigned long *first_free = mem_read(0);
    unsigned long  new_free   = get_mem_index(block);

    if (new_free < *first_free) 
        *first_free = new_free;
}

/* NODE FUNCTIONS
 * For dealing with a node's internal byte queue.
 */

bool is_node_full(const struct node *node) 
{
    return node->length == BYTES_IN_NODE;
}

/* Push a new byte to a node's internal queue
 */
void push_to_node(struct node *node, unsigned char byte) 
{
    assert(!is_node_full(node));
    node->bytes[node->length] = byte;
    node->length++;
}

/* Pop byte to a node's internal queue, update the start index
 */
unsigned char pop_from_node(struct node *node) 
{
    
    assert(node->length > 0);
    unsigned char ret = node->bytes[0];
    node->length--;

    memcpy(node->bytes, node->bytes+1, node->length);

    return ret;
}

/* MAIN INTERFACE
 */

Q * create_queue() 
{
    Q *q = (Q*) mem_allocate();
    q->is_empty = true;
    q->new_node_needed = true;
    return q;
}

void destroy_queue(Q * q) 
{
    if (q == NULL)
        on_illegal_operation();

    unsigned char index = q->head;

    // Walk through the queue, freeing each node
    while (index != 0) {
        struct node *node = mem_read(q->head);
        index = node->next;
        mem_free(node);
    }

    // Free the queue
    mem_free(q);
}

void enqueue_byte(Q * q, unsigned char b) 
{
    // Fail if the queue is uninitialised or destroyed
    if (q == NULL)
        on_illegal_operation();

    if (q->new_node_needed) {
        // A new node is needed, either because the previous node is full
        // or the queue is empty.
        struct node   *new_node  = mem_allocate();
        unsigned char  new_index = get_mem_index(new_node);
        push_to_node(new_node, b);
        new_node->next = 0;

        if (q->is_empty) {
            // If the queue is empty, add the new node as the head
            q->head = new_index;
            q->is_empty = false;    
        } else {
            // Otherwise, set the current tail's next to the new node
            struct node *current_tail = mem_read(q->tail);
            current_tail->next = get_mem_index(new_node);
        }
        
        // Regardless, update the tail and new_node_needed
        q->tail = new_index;
        q->new_node_needed = false;
    } else {
        // There is space in the existing tail node, so push the byte to it
        struct node *current_tail = mem_read(q->tail);
        push_to_node(current_tail, b);

        // If the new byte causes the node to be full, a new node will be needed
        // next enqueue_byte() call
        if (is_node_full(current_tail))
            q->new_node_needed = true;
    }
}

unsigned char dequeue_byte(Q * q) 
{
    // Fail if the queue is null or empty
    if (q == NULL || q->is_empty)
        on_illegal_operation();

    // Retrieve the node at the front of the queue,
    // pop a byte, and test if the node was emptied.
    struct node   *head = mem_read(q->head);
    unsigned char  ret  = pop_from_node(head);

    if (head->length == 0) {
        // If the pop call empties the node, free it.
        int next = head->next;
        mem_free(head);

        // If the freed head was the end of the queue,
        // mark the queue as empty. Otherwise replace the head.
        if (next == 0) {
            q->is_empty = true;
            q->new_node_needed = true;
        } else {
            q->head = next;
        } 
    }

    return ret;
}

void on_out_of_memory() 
{
    printf("Out of memory\n");
    exit(1);
}

void on_illegal_operation() 
{
    printf("Illegal operation\n");
    exit(1);
}

int main() 
{
    // Test output
    Q * q0 = create_queue();
    enqueue_byte(q0, 0);
    enqueue_byte(q0, 1);
    Q * q1 = create_queue();
    enqueue_byte(q1, 3);
    enqueue_byte(q0, 2);
    enqueue_byte(q1, 4);
    printf("%d ", dequeue_byte(q0));
    printf("%d\n", dequeue_byte(q0));
    enqueue_byte(q0, 5);
    enqueue_byte(q1, 6);
    printf("%d ", dequeue_byte(q0));
    printf("%d\n", dequeue_byte(q0));
    destroy_queue(q0);
    printf("%d ", dequeue_byte(q1));
    printf("%d ", dequeue_byte(q1));
    printf("%d\n", dequeue_byte(q1));
    destroy_queue(q1);
    
    // should give as output:
    // 0 1
    // 2 5
    // 3 4 6
}

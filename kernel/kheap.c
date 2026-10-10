/* Kernel heap: first-fit allocator over a fixed region reserved by the
 * linker script. Each block has a header; free() merges adjacent free
 * blocks so the heap doesn't fragment into unusable pieces. */
#include "kernel.h"

extern char heap_start[], heap_end[];

typedef struct block {
    size_t size;            /* payload bytes, excluding this header */
    bool free;
    struct block *next;
} block_t;

#define ALIGN 16
#define HDR ((sizeof(block_t) + ALIGN - 1) & ~(ALIGN - 1))
static block_t *head;

void kheap_init(void) {
    head = (block_t *)heap_start;
    head->size = (size_t)(heap_end - heap_start) - HDR;
    head->free = true;
    head->next = NULL;
}

void *kmalloc(size_t n) {
    if (n == 0) return NULL;
    n = (n + ALIGN - 1) & ~(size_t)(ALIGN - 1);
    for (block_t *b = head; b; b = b->next) {
        if (!b->free || b->size < n) continue;
        if (b->size >= n + HDR + ALIGN) {             /* split off the remainder */
            block_t *rest = (block_t *)((char *)b + HDR + n);
            rest->size = b->size - n - HDR;
            rest->free = true;
            rest->next = b->next;
            b->next = rest;
            b->size = n;
        }
        b->free = false;
        return (char *)b + HDR;
    }
    return NULL;
}

void kfree(void *p) {
    if (!p) return;
    block_t *b = (block_t *)((char *)p - HDR);
    if (b->free) panic("kfree: double free");
    b->free = true;
    for (block_t *c = head; c && c->next; ) {          /* coalesce neighbours */
        if (c->free && c->next->free) {
            c->size += HDR + c->next->size;
            c->next = c->next->next;
        } else {
            c = c->next;
        }
    }
}

void kheap_stats(size_t *used, size_t *free_bytes, int *blocks) {
    *used = *free_bytes = 0; *blocks = 0;
    for (block_t *b = head; b; b = b->next) {
        (*blocks)++;
        if (b->free) *free_bytes += b->size; else *used += b->size;
    }
}

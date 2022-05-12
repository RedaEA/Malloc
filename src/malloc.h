#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

void *malloc(size_t size);

struct alloc_block
{
    struct alloc_block *next;
    size_t size_res;
    size_t size;
    struct block_manage *manager;
};

struct block_manage
{
    void *address;
    struct block_manage *next;
    size_t size;
    size_t is_free;
};

#endif /* !MALLOC_H  */

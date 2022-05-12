#include "allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

struct blk_allocator *blka_new(void)
{
    struct blk_allocator *new = malloc(sizeof(struct blk_allocator));
    new->meta = NULL;
    return new;
}

void blka_delete(struct blk_allocator *blka)
{
    if (blka)
    {
        struct blk_meta *new = blka->meta;
        while (new)
        {
            blka_pop(blka);
            new = blka->meta;
        }
        free(blka);
    }
}

struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size)
{
    const size_t total_size = size + sizeof(struct blk_meta);
    struct blk_meta *new = mmap(NULL, total_size, PROT_WRITE | PROT_READ,
                                MAP_PRIVATE | MAP_ANON, -1, 0);
    const size_t len_page = sysconf(_SC_PAGESIZE);
    size_t nb_pages = (total_size / len_page) + 1;
    if (total_size % len_page == 0)
    {
        new->size = 0;
    }
    else
    {
        new->size = len_page *nb_pages - sizeof(struct blk_meta);
    }
    new->next = blka->meta;
    blka->meta = new;
    return new;
}

void blka_free(struct blk_meta *blk)
{
    if (blk)
    {
        munmap(blk, sizeof(struct blk_meta));
    }
}

void blka_pop(struct blk_allocator *blka)
{
    if (blka)
    {
        if (blka->meta)
        {
            struct blk_meta *tmp = blka->meta->next;
            munmap(blka->meta, sizeof(struct blk_meta));
            blka->meta = tmp;
        }
    }
}

#define _GNU_SOURCE

#include "malloc.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

struct alloc_block *block = NULL;

size_t good(struct alloc_block *current_block, size_t size)
{
    struct block_manage *cursor = current_block->manager;
    while (cursor)
    {
        if (cursor->is_free && ((cursor->size - 32) >= size))
        {
            return 1;
        }
        cursor = cursor->next;
    }
    return 0;
}

struct alloc_block *new_page(size_t size, void *address)
{
    const size_t len_page = sysconf(_SC_PAGESIZE);
    size_t size_block_manage = 32;
    size_t size_alloc_block = 32;
    size_t total = size_alloc_block + size + size_block_manage;

    size_t nb_pages;
    if (total % len_page == 0)
    {
        nb_pages = total / len_page;
    }
    else
    {
        nb_pages = (total / len_page) + 1;
    }
    struct alloc_block *new = NULL;
    new = mmap(address, nb_pages * len_page, PROT_WRITE | PROT_READ,
               MAP_PRIVATE | MAP_ANON, -1, 0);
    if (new == MAP_FAILED)
    {
        return NULL;
    }
    new->size = nb_pages *len_page;
    new->size_res = new->size - total;
    new->next = NULL;

    void *block_address = new;
    char *block_address2 = block_address;
    block_address = block_address2 + 32;

    new->manager = block_address;
    new->manager->size = size + size_block_manage;

    new->manager->is_free = 0;
    new->manager->address = new->manager;

    char *manager_address = new->manager->address;
    if (new->size_res > 32)
    {
        void *manager_2 = manager_address + new->manager->size;
        new->size_res -= 32;
        new->manager->next = manager_2;
        new->manager->next->is_free = 1;
        new->manager->next->size = new->size_res + 32;
        new->manager->next->next = NULL;
        new->manager->next->address = new->manager->next;
    }
    else
    {
        new->size_res = 0;
        new->manager->next = NULL;
    }

    return new;
}

void split(struct block_manage *current_manager, size_t total, size_t size)
{
    if (current_manager->size - total > 32)
    {
        struct block_manage *tmp = current_manager->next;

        char *new_address = current_manager->address;
        void *add = new_address + total;

        current_manager->next = add;
        current_manager->next->address = current_manager->next;
        current_manager->next->next = tmp;
        current_manager->next->size = current_manager->size - 32 - size;
        current_manager->next->is_free = 1;
    }
}

void *block_parse(size_t size, struct alloc_block *current_block)
{
    size_t size_block_manage = 32;
    size_t total = size + size_block_manage;
    struct block_manage *cursor = current_block->manager;
    while (cursor && (cursor->is_free != 1 || (cursor->size - 32) < size))
    {
        cursor = cursor->next;
    }
    if (!cursor)
    {
        return NULL;
    }
    split(cursor, total, size);

    size_t old_size = cursor->size;
    cursor->size = total;
    cursor->is_free = 0;
    current_block->size_res -= size;
    if (!cursor->next)
    {
        if (old_size - total > 32)
        {
            char *cursor_next_address = cursor->address;
            void *nextnext = cursor_next_address + cursor->size;

            void *block_address = current_block;
            char *block_max = block_address;
            block_max += current_block->size;
            void *max = block_max;
            if (nextnext < max)
            {
                cursor->next = nextnext;
                cursor->next->is_free = 1;
                cursor->next->size = old_size - 32 - size;
                cursor->next->address = cursor->next;
                cursor->next->next = NULL;
                current_block->size_res -= 32;
            }
            else
            {
                cursor->next = NULL;
            }
        }
        else
        {
            cursor->next = NULL;
            current_block->size_res = 0;
        }
    }

    char *cursor_next_address = cursor->address;
    void *res = cursor_next_address + size_block_manage;
    return res;
}

void *block_allocation(size_t size)
{
    size_t size_block_manage = 32;

    if (block->size_res >= size && good(block, size))
    {
        return block_parse(size, block);
    }

    else
    {
        struct alloc_block *cursor = block;
        while (cursor->next && !good(cursor->next, size))
        {
            cursor = cursor->next;
        }
        if (cursor->next && good(cursor->next, size))
        {
            return block_parse(size, cursor->next);
        }

        else
        {
            cursor->next = new_page(size, cursor + cursor->size);
            if (!cursor->next)
            {
                return NULL;
            }
            char *cursor_next_manager_address = cursor->next->manager->address;
            void *res = cursor_next_manager_address + size_block_manage;
            return res;
        }
    }
}

__attribute__((visibility("default"))) void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    while (size % 16 != 0)
    {
        size++;
    }

    size_t size_block_manage = 32;
    size_t size_alloc_block = 32;
    if (!block)
    {
        const size_t len_page = sysconf(_SC_PAGESIZE);
        size_t total = size_block_manage + size + size_alloc_block;
        size_t nb_pages;
        if (total % len_page == 0)
        {
            nb_pages = total / len_page;
        }
        else
        {
            nb_pages = (total / len_page) + 1;
        }
        block = mmap(NULL, nb_pages * len_page, PROT_WRITE | PROT_READ,
                     MAP_PRIVATE | MAP_ANON, -1, 0);

        if (block == MAP_FAILED)
        {
            return NULL;
        }
        block->size = nb_pages * len_page;
        block->size_res = block->size - total;
        block->next = NULL;

        void *block_address = block;
        char *block_address2 = block_address;
        block_address = block_address2 + 32;

        block->manager = block_address;
        block->manager->size = size + size_block_manage;

        block->manager->is_free = 0;
        block->manager->address = block->manager;
        char *manager_address = block->manager->address;
        if (block->size_res > 32)
        {
            void *manager_2 = manager_address + block->manager->size;
            block->size_res -= 32;
            block->manager->next = manager_2;
            block->manager->next->is_free = 1;
            block->manager->next->size = block->size_res + 32;
            block->manager->next->next = NULL;
            block->manager->next->address = block->manager->next;
        }
        else
        {
            block->size_res = 0;
            block->manager->next = NULL;
        }

        return manager_address + 32;
    }
    return block_allocation(size);
}

void fusion(struct alloc_block *current_block)
{
    struct block_manage *cursor = current_block->manager;
    while (cursor)
    {
        if (cursor->is_free)
        {
            while (cursor->next && cursor->next->is_free)
            {
                cursor->size = cursor->size + cursor->next->size;
                cursor->next = cursor->next->next;
            }
        }
        cursor = cursor->next;
    }
}

void free_intern(void *ptr, struct alloc_block *current_block,
                 struct alloc_block *prev, int cas)
{
    struct block_manage *cursor = current_block->manager;
    if (current_block == ptr)
    {
        if (cas == 0)
        {
            struct alloc_block *tmp = block;
            block = block->next;
            munmap(tmp, tmp->size);
        }
        else
        {
            prev->next = current_block->next;
            munmap(current_block, current_block->size);
        }
    }
    else
    {
        while (cursor)
        {
            char *ptr_address = cursor->address;
            ptr_address += 32;
            if (ptr == ptr_address)
            {
                cursor->is_free = 1;
                current_block->size_res += cursor->size - 32;
                cursor = block->manager;
                int bool1 = 0;
                while (cursor && bool1 == 0)
                {
                    if (cursor->is_free == 0)
                    {
                        bool1 = 1;
                    }
                    cursor = cursor->next;
                }
                if (!bool1)
                {
                    if (cas == 0)
                    {
                        struct alloc_block *tmp = block;
                        block = block->next;
                        munmap(tmp, tmp->size);
                    }
                    else
                    {
                        prev->next = current_block->next;
                        munmap(current_block, current_block->size);
                    }
                }
                else
                {
                    fusion(current_block);
                }
                break;
            }
            else
            {
                cursor = cursor->next;
            }
        }
    }
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (ptr)
    {
        char *ptr_v = ptr;
        void *block_address = block;

        char *born_max = block_address;
        born_max += block->size;

        if (ptr >= block_address && ptr_v < born_max)
        {
            free_intern(ptr, block, NULL, 0);
        }
        else
        {
            struct alloc_block *cursor = block;
            void *cursor_next = cursor->next;
            born_max = cursor_next;
            born_max += cursor->next->size;
            while (cursor->next && ((cursor_next > ptr) || (born_max <= ptr_v)))
            {
                cursor = cursor->next;
                cursor_next = cursor->next;
                born_max = cursor_next;
                born_max += cursor->next->size;
            }
            if (cursor->next && ptr >= cursor_next && (ptr_v < born_max))
            {
                free_intern(ptr, cursor->next, cursor, 1);
            }
        }
    }
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    size_t over;
    if (__builtin_mul_overflow(size, nmemb, &over))
    {
        return NULL;
    }
    void *res = malloc(over);
    if (!res)
    {
        return NULL;
    }
    char *tmp = res;
    char *res_c = res;
    while (tmp < res_c + over)
    {
        *tmp = 0;
        tmp++;
    }
    return res;
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    if (!ptr)
    {
        return malloc(size);
    }
    void *res = malloc(size);
    mempcpy(res, ptr, size);
    free(ptr);
    return res;
}

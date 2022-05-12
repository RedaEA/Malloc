#include "page_begin.h"

void *page_begin(void *ptr, size_t page_size)
{
    char *i = ptr;
    char *nb = NULL;
    while (nb <= i - page_size)
    {
        nb += page_size;
    }
    return nb;
}

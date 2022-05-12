#include "beware_overflow.h"

void *beware_overflow(void *ptr, size_t nmemb, size_t size)
{
    size_t res;
    if (__builtin_mul_overflow(size, nmemb, &res))
    {
        return NULL;
    }
    char *a = ptr;
    a += res;

    return a;
}

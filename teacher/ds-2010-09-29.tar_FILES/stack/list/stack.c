#include <stddef.h>

#include "stack.h"

STACK *stack_creat(int size)
{
      return llist_creat(size);
}

void stack_destroy(STACK *ptr)
{
      llist_destroy(ptr);
}

int stack_push(STACK *ptr, void *data)
{
      return llist_insert(ptr, data, LLIST_FORWARD);
}

static int always_match(const void *p1, const void *p2)
{
      return 0;
}

int stack_pop(STACK *ptr, void *data)
{
      return llist_fetch(ptr, data, NULL, always_match, LLIST_FORWARD);
}

#ifndef INT_LIST_H
#define INT_LIST_H

#include "indef.h"

// Opaque pointer to the int_list structure
typedef struct node {
    int data;
    struct node *next;
} INT_NODE;

typedef struct list {
    INT_NODE *head;
    int count;
} INT_LIST;

// Create a new int_list
INT_LIST *int_list_create();

// Destroy a int_list and free its memory
void int_list_destroy(INT_LIST *list);

// Print the int_list (useful for testing)
void int_list_print(INT_LIST *list);

// Get the size of the int_list
int int_list_count(INT_LIST *list);

// Check if the int_list is empty
int int_list_empty(INT_LIST *list);

// Add an element to the tail of the int_list
/* int int_list_append(INT_LIST *list, int data); */

// Add an element to the head of the int_list
int int_list_prepend(INT_LIST *list, int data);

// reverse prepend list, so no append function needed
void int_list_reverse(INT_LIST *list);

// asdf
void int_list_remove_node(INT_LIST *list, INT_NODE *node);

#endif

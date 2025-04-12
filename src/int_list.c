#include "indef.h"
#include "int_list.h"

INT_LIST *int_list_create()
{
    INT_LIST *int_list = malloc(sizeof(INT_LIST));
    assert(int_list != NULL);
    int_list->head = NULL;
    int_list->count = 0;
    return int_list;
}

static void _node_destroy(INT_NODE *node)
{
    if (node->next != NULL) { 
        _node_destroy(node->next);
    }
    free(node);
}

void int_list_destroy(INT_LIST *int_list)
{
    assert(int_list != NULL);
    assert(int_list->head != NULL);
    _node_destroy(int_list->head);
    free(int_list);
}

void int_list_print(INT_LIST *list)
{
    INT_NODE *current = list->head;
    while (current) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

int int_list_count(INT_LIST *list)
{
    assert(list != NULL);
    return list->count;
}

int int_list_empty(INT_LIST *list)
{
    assert(list != NULL);
    if (list->head == NULL)
        return 1;
    else
        return 0;
}

// node specific functions
int int_list_append(INT_LIST *list, int data)
{
    assert(list != NULL);

    INT_NODE *new_node = malloc(sizeof(INT_NODE));
    if (!new_node) { return 0; }
    new_node->data = data;
    new_node->next = NULL;

    if (list->head == NULL) {
        list->head = new_node;
    } else {
        INT_NODE *current = list->head;
        for (;;) {
            if (current->next == NULL) {
                current->next = new_node;
                break;
            } else {
                current = current->next;
            }
        }
    }
    list->count++;
    return 1;
}

int int_list_prepend(INT_LIST *list, int data)
{
    assert(list != NULL);
    INT_NODE *new_node = malloc(sizeof(INT_NODE));
    if (!new_node) { return 0; }
    // setting to NULL to avoid mem errors
    new_node->data = data;
    new_node->next = NULL;
    // head gets address of new_node, new_node->next gets address of prev node
    new_node->next = list->head;
    list->head = new_node;
    list->count++;
    return 1;
}

void int_list_reverse(INT_LIST *list)
{
    assert(list != NULL);
	INT_NODE *current = list->head;
	INT_NODE *prev = NULL;
	INT_NODE *temp = NULL;
	// point nodes to the previous node in the list
	while (current->next) {
		temp = current->next;
		current->next = prev;
		prev = current;
		current = temp;
	}
	current->next = prev;
	list->head = current;
}

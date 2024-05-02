#include "linked_list.h"

// Creates and returns a new list
list_t* list_create()
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_t* list = (list_t*) malloc(sizeof(list_t));
    if (list == NULL) {
        return NULL;
    }

    // Initalize head and count
    list -> head = NULL;
    list -> count = 0;
    return list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (list == NULL) {
        return;
    }
    
    list_node_t* head = list->head;
    
    while (head != NULL) {
        list_node_t* next_head = list_next(head);
        free(head);
        head = next_head;
    }
    
    free(list);
}

// Returns beginning of the list
list_node_t* list_begin(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */

    return list->head;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */

    return node->next;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return list->count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    list_node_t* head = list -> head;
    while (head != NULL) {
        if ((head->data) == data) {
            return head;
        }
        head = list_next(head);
    }
    return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */

    list_node_t* new_node = (list_node_t*) malloc (sizeof(list_node_t));
    if (new_node == NULL) {
        return;
    }

    // Insert into the beginning of list
    new_node->data = data;
    new_node->prev = NULL;
    new_node->next = list->head;
    if(list->head != NULL) {
        list->head->prev = new_node;
    }
    list->head = new_node;

    (list->count)++;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    if (list == NULL || node == NULL) {
        return;
    }

    list_node_t* f_node = list_find(list, node -> data);

    // Removes and frees node
    if (f_node != NULL) {
        if ((f_node->prev) != NULL) {
            (f_node -> prev) -> next = f_node -> next;
        } else {
            list -> head = f_node -> next;
        }

        if ((f_node -> next) != NULL) {
            (f_node -> next) -> prev = f_node -> prev;
        }

        free(node);
        (list->count)--;
    }

}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data))
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */

    list_node_t* head = list->head;

    // Runs function for each element
    while (head != NULL) {
        func(head->data);
        head = list_next(head);
    }
}

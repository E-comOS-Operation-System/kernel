/*
 * E-comOS Microkernel - Linked list implementation
 */

#ifndef KERNEL_INTERNAL_LIST_H
#define KERNEL_INTERNAL_LIST_H

// Simple doubly-linked list
struct list_node {
    struct list_node *next;
    struct list_node *prev;
};

struct list_head {
    struct list_node *first;
    struct list_node *last;
    size_t count;
};

// List operations
void list_init(struct list_head *head);
void list_add(struct list_head *head, struct list_node *node);
void list_remove(struct list_head *head, struct list_node *node);

#endif
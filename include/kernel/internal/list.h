/*
    E-comOS Kernel - Doubly-linked List
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_INTERNAL_LIST_H
#define KERNEL_INTERNAL_LIST_H

#include <stddef.h>

typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

typedef struct {
    ListNode *first;
    ListNode *last;
    size_t    count;
} ListHead;

void listInit(ListHead *head);
void listAdd(ListHead *head, ListNode *node);
void listRemove(ListHead *head, ListNode *node);

#endif

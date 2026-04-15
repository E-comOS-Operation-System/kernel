/*
    E-comOS Kernel - Doubly-linked List
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/internal/list.h>

void listInit(ListHead *head) {
    head->first = 0;
    head->last  = 0;
    head->count = 0;
}

void listAdd(ListHead *head, ListNode *node) {
    node->next = 0;
    node->prev = head->last;
    if (head->last)
        head->last->next = node;
    else
        head->first = node;
    head->last = node;
    head->count++;
}

void listRemove(ListHead *head, ListNode *node) {
    if (node->prev)
        node->prev->next = node->next;
    else
        head->first = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        head->last = node->prev;
    node->next = 0;
    node->prev = 0;
    head->count--;
}

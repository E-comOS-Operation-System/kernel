/*
    E-comOS Kernel - IPC subsystem
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/ipc.h>
#include <stddef.h>

#define MAX_IPC_QUEUE 32

static ipc_message_t ipc_queue[MAX_IPC_QUEUE];
static uint32_t queue_head = 0;
static uint32_t queue_tail = 0;

/* Low-level send used internally and by syscall_handler (SYS_IPC_SEND) */
int ipc_send(thread_id_t target, ipc_message_t *msg) {
    uint32_t next_tail = (queue_tail + 1) % MAX_IPC_QUEUE;
    if (next_tail == queue_head)
        return ECLIB_IPC_BUFFER_OVERFLOW;

    ipc_queue[queue_tail]        = *msg;
    ipc_queue[queue_tail].target = target;
    queue_tail = next_tail;
    return ECLIB_OK;
}

/* Low-level receive used internally and by syscall_handler (SYS_IPC_RECEIVE) */
int ipc_receive(ipc_message_t *msg) {
    if (queue_head == queue_tail)
        return ECLIB_IPC_TIMEOUT;

    *msg = ipc_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_IPC_QUEUE;
    return ECLIB_OK;
}

/*
 * ipc_send_msg — higher-level helper used by init and services.
 * timeout_ms is ignored for now (no blocking send needed in the kernel).
 */
int ipc_send_msg(uint32_t type, uint32_t flags, uint32_t receiver_pid,
                 uint32_t data_len, const void *data)
{
    (void)flags;

    if (data_len > IPC_MAX_DATA_SIZE)
        return ECLIB_IPC_BUFFER_OVERFLOW;

    ipc_message_t msg = {0};
    msg.type   = type;
    msg.target = receiver_pid;
    msg.size   = data_len;

    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < data_len; i++)
        msg.data[i] = src[i];

    return ipc_send((thread_id_t)receiver_pid, &msg);
}

/*
 * ipc_receive_msg — blocking receive for init / services.
 * timeout_ms == 0 means block until a message arrives.
 * Non-zero timeout is not yet implemented (returns immediately if empty).
 */
int ipc_receive_msg(ipc_message_t *msg, int timeout_ms)
{
    (void)timeout_ms;
    return ipc_receive(msg);
}

/*
    E-comOS Kernel - IPC subsystem
    Copyright (C) 2025,2026  Saladin5101

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <kernel/ipc.h>

#define MAX_IPC_QUEUE 32

static IpcMessage ipcQueue[MAX_IPC_QUEUE];
static uint32_t   queueHead = 0;
static uint32_t   queueTail = 0;

int ipcSend(ThreadId target, IpcMessage *msg) {
    uint32_t nextTail = (queueTail + 1) % MAX_IPC_QUEUE;
    if (nextTail == queueHead)
        return ECLIB_IPC_BUFFER_OVERFLOW;
    ipcQueue[queueTail]        = *msg;
    ipcQueue[queueTail].target = target;
    queueTail = nextTail;
    return ECLIB_OK;
}

int ipcReceive(IpcMessage *msg) {
    if (queueHead == queueTail)
        return ECLIB_IPC_TIMEOUT;
    *msg = ipcQueue[queueHead];
    queueHead = (queueHead + 1) % MAX_IPC_QUEUE;
    return ECLIB_OK;
}

int ipcSendMsg(uint32_t type, uint32_t flags, uint32_t receiverPid,
               uint32_t dataLen, const void *data) {
    (void)flags;
    if (dataLen > IPC_MAX_DATA_SIZE)
        return ECLIB_IPC_BUFFER_OVERFLOW;
    IpcMessage msg = {0};
    msg.type   = type;
    msg.target = receiverPid;
    msg.size   = dataLen;
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < dataLen; i++)
        msg.data[i] = src[i];
    return ipcSend((ThreadId)receiverPid, &msg);
}

int ipcReceiveMsg(IpcMessage *msg, int timeoutMs) {
    (void)timeoutMs;
    return ipcReceive(msg);
}

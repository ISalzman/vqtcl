/* V9 dynamic memory buffers
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include <stdlib.h>
#include <string.h>

typedef struct Overflow {
    char b[4096]; /* must be first member so that addr of b == addr of struct */
    Overflow_p next;
} Overflow;

void InitBuffer (Buffer* bp) {
    bp->fill.c = bp->buf;
    bp->limit = bp->buf + sizeof bp->buf;
    bp->head = 0;
    bp->ofill = 0;
    bp->saved = 0;
    bp->result = 0;
}

void ReleaseBuffer (Buffer* bp, int keep) {
    while (bp->head != 0) {
        Overflow_p op = bp->head;
        bp->head = op->next;
        free(op);
    }
    if (!keep && bp->result != 0)
        free(bp->result);
}

void AddToBuffer (Buffer* bp, const void* data, intptr_t len) {
    intptr_t n;
    while (len > 0) {
        if (bp->fill.c >= bp->limit) {
            if (bp->head == 0 || 
                    bp->ofill >= bp->head->b + sizeof bp->head->b) {
                Overflow_p op = (Overflow_p) malloc(sizeof(Overflow));
                op->next = bp->head;
                bp->head = op;
                bp->ofill = op->b;
            }
            memcpy(bp->ofill, bp->buf, sizeof bp->buf);
            bp->ofill += sizeof bp->buf;
            bp->saved += sizeof bp->buf;
            n = bp->fill.c - bp->slack;
            memcpy(bp->buf, bp->slack, n);
            bp->fill.c = bp->buf + n;
        }
        n = len;
        if (n > bp->limit - bp->fill.c)
            n = bp->limit - bp->fill.c; /* TODO: copy big chunks to overflow */
        memcpy(bp->fill.c, data, n);
        bp->fill.c += n;
        data = (const char*) data + n;
        len -= n;
    }
}

int NextFromBuffer (Buffer* bp, char** firstp, int* pcount) {
    int count;

    if (*firstp == 0) {
        Overflow_p p = bp->head, q = 0;
        while (p != 0) {
            Overflow_p t = p->next;
            p->next = q;
            q = p;
            p = t;
        }

        bp->head = q;
        bp->used = 0;
    } else if (*firstp == bp->head->b) {
        bp->head = bp->head->next;
        bp->used += *pcount;
        free(*firstp);
    } else if (*firstp == bp->buf)
        return *pcount == 0;

    if (bp->head != 0) {
        *firstp = bp->head->b;
        count = bp->saved - bp->used;
        if (count > (int) sizeof bp->head->b)
            count = sizeof bp->head->b;
    } else {
        count = bp->fill.c - bp->buf;
        *firstp = bp->buf;
    }

    *pcount = count;
    return *pcount;
}

/* TODO: this code can probably be avoided: use NextFromBuffer in callers */
void *BufferAsPtr (Buffer* bp, int fast) {
    intptr_t len;
    char *data, *ptr = 0;
    int cnt;

    if (fast && bp->saved == 0)
        return bp->buf;

    len = BufferCount(bp);
    if (bp->result == 0)
        bp->result = malloc(len);

    for (data = bp->result; NextFromBuffer(bp, &ptr, &cnt); data += cnt)
        memcpy(data, ptr, cnt);

    return bp->result;
}

#ifndef VLERQ_H
#define VLERQ_H

#include <stdint.h>

#define vCOPYRIGHT    "Copyright (C) 1996-2008 Jean-Claude Wippler"
#define vVERSION      "1.9.0"

typedef enum { 
    v_NIL, 
    v_INT,
    v_POS, 
    v_LONG,
    v_FLOAT,
    v_DOUBLE,
    v_STRING,
    v_BYTES, 
    v_VIEW
} vType;

#define vTYPES "NIPLFDSBV" /* canonical char code, indexed by vqType */

typedef uint8_t vOctet;
typedef int64_t vLong;

typedef struct vColPtr_s *vColPtr;
typedef struct vView_s *vView;

typedef union {
    int i;
    float f;
    void *p;
    const char *s;
    vOctet *o;
    vColPtr c;
    vView v;
               
    vLong l;    /* l overlaps pair.two on 32b machines */
    double d;   /* d overlaps pair.two on 32b machines */

    struct {
        void *one;
        union {
            int j;
            void *q;
        } two;
    } pair;
} vAny;

#endif

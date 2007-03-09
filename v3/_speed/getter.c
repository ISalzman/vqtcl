#include <malloc.h>

int* buf = 0;

#define N 1000000

typedef struct { int o; const int* ptr; int i; } Col;
typedef struct { int o; int i; } Val;

void init(int n) {
  if (buf == 0) {
    int i;
    buf = (int*) malloc(n * sizeof(int));
    for (i = 0; i < n; ++i)
      buf[i] = i;
  }
}
  
int f0(Col* col, int i, Val* val) {
  return 1;
}  

int f1(Col* col, int i) {
  col->i = col->ptr[i];
  return 1;
}  

int f2(Col* col, int i, Val* val) {
  val->i = col->ptr[i];
  return 1;
}  

int f3(Col* col, int i, int* ptr) {
  col->i = ptr[i];
  return 1;
}  

int f4(int i, int* ptr) {
  return ptr[i];
}  

int f5(Col* col, int i) {
  return col->ptr[i];
}  

void g0 (int n, int (*f)(Col*,int,Val*)) {
  Col col;
  Val val;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f(&col, i, &val);
}

void g1 (int n, int (*f)(Col*,int)) {
  Col col;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f(&col, i);
}

void g2 (int n, int (*f)(Col*,int,Val*)) {
  Col col;
  Val val;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f(&col, i, &val);
}

void g3 (int n, int (*f)(Col*,int,int*)) {
  Col col;
  int i, j;
  init(n);
  j = 0;
  for (i = 0; i < n; ++i)
    j += f(&col, i, buf);
}

void g4 (int n, int (*f)(int,int*)) {
  int i, j;
  init(n);
  j = 0;
  for (i = 0; i < n; ++i)
    j += f(i, buf);
}

void g5 (int n, int (*f)(Col*,int)) {
  Col col;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f(&col, i);
}

int main() {
  g0(N,f0);
  g1(N,f1);
  g2(N,f2);
  g3(N,f3);
  g4(N,f4);
  g5(N,f5);
  return 0;
}

#! /usr/bin/env tclkit
# compare getter low-level timings

source ~/bin/critcl
package require critcl
namespace import critcl::*

set N 10000000

# biggie
if 0 {
	41.0   g0                                       (10x)
       169.7   g1                                       (10x)
       187.0   g2                                       (10x)
       172.7   g3                                       (10x)
	25.8   g4                                       (10x)
	25.3   g5                                       (10x)
  Thu Sep 21 11:38:55 CEST 2006 (times in msec, N = 10000000)
}

ccode {
  int* buf = 0;

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

  int f3(Col* col, int i, const int* ptr) {
    col->i = ptr[i];
    return 1;
  }  

  int f4(int i, const int* ptr) {
    return ptr[i];
  }  
 
  int f5(Col* col, int i) {
    return col->ptr[i];
  }  
}

cproc g0 {int n} void {
  Col col;
  Val val;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f0(&col, i, &val);
}

cproc g1 {int n} void {
  Col col;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f1(&col, i);
}

cproc g2 {int n} void {
  Col col;
  Val val;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f2(&col, i, &val);
}

cproc g3 {int n} void {
  Col col;
  int i, j;
  init(n);
  j = 0;
  for (i = 0; i < n; ++i)
    j += f3(&col, i, buf);
}

cproc g4 {int n} void {
  int i, j;
  init(n);
  j = 0;
  for (i = 0; i < n; ++i)
    j += f4(i, buf);
}

cproc g5 {int n} void {
  Col col;
  int i, j;
  init(n);
  col.ptr = buf;
  j = 0;
  for (i = 0; i < n; ++i)
    j += f5(&col, i);
}

proc t {msg cnt cmd} {
  puts [format {%10.1f   %-40s (%dx)} \
                  [expr {[lindex [time $cmd $cnt] 0]/1000.0}] $msg $cnt]
}
proc xt args {}

t {g0} 10 { g0 $::N }
t {g1} 10 { g1 $::N }
t {g2} 10 { g2 $::N }
t {g3} 10 { g3 $::N }
t {g4} 10 { g4 $::N }
t {g5} 10 { g5 $::N }

puts "[clock format [clock seconds]] (times in msec, N = $N)"

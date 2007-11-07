/* Vlerq extension for Tcl */

#include <Python/Python.h>

void initvq5py (void);

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

void initvq5py (void) {
  PyObject* m;
  
  m = Py_InitModule3("vq5py", module_methods, "Python binding to Vlerq.");
  
  if (m == NULL)
    return;
}

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define EXAMPLE_MODULE
#include "examplemodule.h"

static char * 
PyExample_DoSomething(int command)
{
    char * mess;

    if(command < 0) mess = "Test 1 successful";
    if(command >= 0) mess = "Test 2 successful";

    return mess;
}

static PyObject *
example_displaymessage(PyObject *self, PyObject *args)
{
    int command;
    char * message;

    if(!PyArg_ParseTuple(args, "i", &command))
        return NULL;
    message = PyExample_DoSomething(command);
    return PyUnicode_FromString(message);
}

static PyMethodDef ExampleMethods[] = {
    {"displaymessage", example_displaymessage, METH_VARARGS, "do something exiciting."},

    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef examplemodule = {
    PyModuleDef_HEAD_INIT, 
    "example",
    NULL, 
    -1,
    ExampleMethods
};

PyMODINIT_FUNC 
PyInit_example(void)
{
    PyObject *m;
    static void *PyExample_API[PyExample_API_pointers];
    PyObject *c_api_object;

    m = PyModule_Create(&examplemodule);
    if(m == NULL)
        return NULL;
    
    PyExample_API[PyExample_DoSomething_NUM] = (void *)PyExample_DoSomething;
    c_api_object = PyCapsule_New((void *)PyExample_API, "example._C_API", NULL);

    if (PyModule_AddObject(m, "_C_API", c_api_object) < 0){
        Py_XDECREF(c_api_object);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
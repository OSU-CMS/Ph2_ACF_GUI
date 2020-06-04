#ifndef Py_EXAMPLEMODULE_H
#define Py_EXAMPLEMODULE_H
#ifdef __cplusplus
extern "C" {
#endif 

#define PyExample_DoSomething_NUM 0
#define PyExample_DoSomething_RETURN char *
#define PyExample_DoSomething_PROTO (int command)

#define PyExample_API_pointers 1

#ifdef EXAMPLE_MODULE

static PyExample_DoSomething_RETURN PyExample_DoSomething PyExample_DoSomething_PROTO;

#else

static void **PyExample_API;

#define PyExample_DoSomething \
 (*(PyExample_DoSomething_RETRUN (*)PyExample_DoSomething_PROTO) PyExample_API[PyExample_DoSomething_NUM])

static int 
import_example(void)
{
    PyExample_API = (void **)PyCapsule_Import("example._C_API", 0);
    return (PyExample_API != NULL) ? 0 : -1;
}

#endif

#ifdef __cplusplus
}
#endif

#endif 



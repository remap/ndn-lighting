//
//  cfgMgr.c
//  keyLab
//
//  Created by nano on 6/27/11.
//  Copyright 2011 UCLA. All rights reserved.
//

#include "cfgMgr.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "limits.h"
#include "assert.h"
#include "stdlib.h"

#ifdef __APPLE__ 
#include <Python/Python.h>
#else 
#include <Python.h>
#endif

PyObject *pName, *pModule, *pDict, *pFunc;
PyObject *pArgs, *pValue;
PyObject *main_module, *global_dict, *expression;

//char * getConfigString(char *paramName);

/* this is now a library

int main(){
    
    printf("Key / Config Manager \n");
	
    printf("i returned %s \n", getConfigString("appPath"));
    
    return 0;
}
 */

// loads python module in C

int initConfigModule(){
    
    Py_Initialize();
    
    // Build the name object
    pName = PyString_FromString("cfgMgr");
    
    // Load the module object
    pModule = PyImport_Import(pName);
    
    // pDict is a borrowed reference 
    pDict = PyModule_GetDict(pModule);
    
    return 0;
}

// closes python module in C

int closeConfigModule(){
    // Clean up
    Py_DECREF(pModule);
    Py_DECREF(pName);
    // Finish the Python Interpreter
    Py_Finalize();
    return 0;
}

char * getConfigString(char *paramName){
    
    initConfigModule();
    
    char *strRet = NULL; 
    // TODO: add error handling for calling null param
    pFunc = PyDict_GetItemString(pDict, paramName);
    pValue = PyObject_CallFunctionObjArgs(pFunc, NULL);
    PyArg_Parse(pValue, "s", &strRet);

    closeConfigModule();
    
    return strRet;
}

int getMfrDeviceCount(){
    
    initConfigModule();
    
    int intRet = 0;
    pFunc = PyDict_GetItemString(pDict, "getTotalDeviceCount");
    pValue = PyObject_CallFunctionObjArgs(pFunc, NULL);
    PyArg_Parse(pValue, "i", &intRet);
    
    closeConfigModule();
    
    return intRet;
}

char * getMfrDeviceID(int id){
    
    initConfigModule();
    printf("getting mfr id from %i\n",id);
    char *strRet = NULL;
    PyObject *temp = Py_BuildValue("i",id);
    // TODO: add error handling for calling null param
    pFunc = PyDict_GetItemString(pDict, "getDeviceID");
    pValue = PyObject_CallFunctionObjArgs(pFunc, temp, NULL);
    PyArg_Parse(pValue, "s", &strRet);
    
    closeConfigModule();
    
    return strRet;
}

char * getMfrDeviceIP(int id){
    
    initConfigModule();
    printf("getting IP from %i\n",id);
    char *strRet = NULL;
    PyObject *temp = Py_BuildValue("i",id);
    // TODO: add error handling for calling null param
    pFunc = PyDict_GetItemString(pDict, "getDeviceIP");
    pValue = PyObject_CallFunctionObjArgs(pFunc, temp, NULL);
    PyArg_Parse(pValue, "s", &strRet);
    
    closeConfigModule();
    
    return strRet;
}

char * getMfrDeviceType(int id){
    
    initConfigModule();
    printf("getting IP from %i\n",id);
    char *strRet = NULL;
    PyObject *temp = Py_BuildValue("i",id);
    // TODO: add error handling for calling null param
    pFunc = PyDict_GetItemString(pDict, "getDeviceType");
    pValue = PyObject_CallFunctionObjArgs(pFunc, temp, NULL);
    PyArg_Parse(pValue, "s", &strRet);
    
    closeConfigModule();
    
    return strRet;
}

/// this is a 'scratchpad' function - derived / permanent functions are above. 

int pyScratch(){
    
    // Initialize the Python Interpreter
    
    Py_Initialize();
    
    // Build the name object
    pName = PyString_FromString("cfgMgr");
    
    // Load the module object
    pModule = PyImport_Import(pName);
    
    // pDict is a borrowed reference 
    pDict = PyModule_GetDict(pModule);
    
    // pFunc is also a borrowed reference 
    pFunc = PyDict_GetItemString(pDict, "multiply");
    
    if (PyCallable_Check(pFunc))
    {
        PyObject_CallObject(pFunc, NULL);
    } else 
    {
        PyErr_Print();
    }

    // test int return
    pValue = PyObject_CallFunctionObjArgs(pFunc, NULL);
	long ret = PyInt_AsLong(pValue);
	printf("i got %li \n", ret);

    // test string return
    pFunc = PyDict_GetItemString(pDict, "appPath");
    pValue = PyObject_CallFunctionObjArgs(pFunc, NULL);
    
    char *strRet = NULL;
    PyArg_Parse(pValue, "s", &strRet);
    printf("i got %s \n", strRet);

    // test arbitrary value return
    pFunc = PyDict_GetItemString(pDict, "appPath");
    pValue = PyObject_CallFunctionObjArgs(pFunc, NULL);
    
    strRet = NULL;
    PyArg_Parse(pValue, "s", &strRet);
    printf("i got %s \n", strRet);
    
    
    // Clean up
    
    Py_DECREF(pModule);
    Py_DECREF(pName);
    
    // Finish the Python Interpreter
    
    Py_Finalize();
    
    return 0;
    
}

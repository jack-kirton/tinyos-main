/*
 * Copyright (c) 2005 Stanford University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * SWIG interface specification for TOSSIM. This file defines
 * the top-level TOSSIM and Mote objects which are exported to
 * Python. The typemap at the beginning allows a script to
 * use Python files as a parameter to a function that takes a
 * FILE* as a parameter (e.g., the logging system in sim_log.h).
 *
 * @author Philip Levis
 * @date   Nov 22 2005
 */

%module TOSSIM

%{
#include <memory.h>
#include <tossim.h>

#include <functional>

enum {
  PRIMITIVE_INTEGER      = 0,
  PRIMITIVE_FLOAT   = 1,
  PRIMITIVE_UNKNOWN = 2
};

int lengthOfType(const char* type) {
  if (strcmp(type, "uint8_t") == 0) {
    return sizeof(uint8_t);
  }
  else if (strcmp(type, "uint16_t") == 0) {
    return sizeof(uint16_t);
  }
  else if (strcmp(type, "uint32_t") == 0) {
    return sizeof(uint32_t);
  }
  else if (strcmp(type, "int8_t") == 0) {
    return sizeof(int8_t);
  }
  else if (strcmp(type, "int16_t") == 0) {
    return sizeof(int16_t);
  }
  else if (strcmp(type, "int32_t") == 0) {
    return sizeof(int32_t);
  }
  else if (strcmp(type, "char") == 0) {
    return sizeof(char);
  }
  else if (strcmp(type, "short") == 0) {
    return sizeof(short);
  }
  else if (strcmp(type, "int") == 0) {
    return sizeof(int);
  }
  else if (strcmp(type, "long") == 0) {
    return sizeof(long);
  }
  else if (strcmp(type, "unsigned char") == 0) {
    return sizeof(unsigned char);
  }
  else if (strcmp(type, "unsigned short") == 0) {
    return sizeof(unsigned short);
  }
  else if (strcmp(type, "unsigned int") == 0) {
    return sizeof(unsigned int);
  }
  else if (strcmp(type, "unsigned long") == 0) {
    return sizeof(unsigned long);
  }
  else if (strcmp(type, "float") == 0) {
    return sizeof(float);
  }
  else if (strcmp(type, "double") == 0) {
    return sizeof(double);
  }
  else {
    return 1;
  }
}

int memoryToPrimitive(const char* type, const char* ptr, long* lval, double* dval) {
  if (strcmp(type, "uint8_t") == 0) {
    uint8_t val;
    memcpy(&val, ptr, sizeof(uint8_t));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "uint16_t") == 0) {
    uint16_t val;
    memcpy(&val, ptr, sizeof(uint16_t));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "uint32_t") == 0) {
    uint32_t val;
    memcpy(&val, ptr, sizeof(uint32_t));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "int8_t") == 0) {
    int8_t val;
    memcpy(&val, ptr, sizeof(int8_t));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "int16_t") == 0) {
    int16_t val;
    memcpy(&val, ptr, sizeof(int16_t));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "int32_t") == 0) {
    int32_t val;
    memcpy(&val, ptr, sizeof(int32_t));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "char") == 0) {
    long val;
    memcpy(&val, ptr, sizeof(char));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "short") == 0) {
    short val;
    memcpy(&val, ptr, sizeof(short));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "int") == 0) {
    int val;
    memcpy(&val, ptr, sizeof(int));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "long") == 0) {
    long val;
    memcpy(&val, ptr, sizeof(long));
    *lval = val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "unsigned char") == 0) {
    unsigned char val;
    memcpy(&val, ptr, sizeof(unsigned char));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "unsigned short") == 0) {
    unsigned short val;
    memcpy(&val, ptr, sizeof(unsigned short));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "unsigned int") == 0) {
    unsigned int val;
    memcpy(&val, ptr, sizeof(unsigned int));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "unsigned long") == 0) {
    unsigned long val;
    memcpy(&val, ptr, sizeof(unsigned long));
    *lval = (long)val;
    return PRIMITIVE_INTEGER;
  }
  else if (strcmp(type, "float") == 0) {
    float val;
    memcpy(&val, ptr, sizeof(float));
    *dval = (double)val;
    return PRIMITIVE_FLOAT;
  }
  else if (strcmp(type, "double") == 0) {
    double val;
    memcpy(&val, ptr, sizeof(double));
    *dval = val;
    return PRIMITIVE_FLOAT;
  }
  else {
    return PRIMITIVE_UNKNOWN;
  }
}

PyObject* valueFromScalar(const char* type, const char* ptr, int len) {
  long lval;
  double dval;
  int rval = memoryToPrimitive(type, ptr, &lval, &dval);
  switch(rval) {
    case PRIMITIVE_INTEGER:
      return PyInt_FromLong(lval);
    case PRIMITIVE_FLOAT:
      return PyFloat_FromDouble(dval);
    case PRIMITIVE_UNKNOWN:
    default:
      return PyString_FromStringAndSize(ptr, len);
  }
}

PyObject* listFromArray(const char* type, const char* ptr, int len) {
  long lval;
  double dval;
  int elementLen = lengthOfType(type);
  PyObject* list = PyList_New(0);
  //printf("Generating list of %s\n", type);
  for (const char* tmpPtr = ptr; tmpPtr < ptr + len; tmpPtr += elementLen) {
    PyList_Append(list, valueFromScalar(type, tmpPtr, elementLen));    
  }
  return list;
}

// From: https://stackoverflow.com/questions/11516809/c-back-end-call-the-python-level-defined-callbacks-with-swig-wrapper#new-answer
class PyCallback
{
private:
    PyObject *func;
    PyCallback& operator=(const PyCallback&); // Not allowed
public:
    PyCallback(PyCallback&& o) : func(o.func) {
      o.func = NULL;
    }
    PyCallback(const PyCallback& o) : func(o.func) {
      Py_XINCREF(func);
    }
    PyCallback(PyObject *pfunc) : func(pfunc) {
      Py_XINCREF(func);
      assert(PyCallable_Check(func));
    }
    ~PyCallback() {
      Py_XDECREF(func);
    }
    bool operator()() const {
      if (!func || Py_None == func || !PyCallable_Check(func))
        return false;
      PyObject *args = PyTuple_New(0);
      PyObject *result = PyObject_Call(func,args,NULL);
      bool bool_result = result != NULL && PyObject_IsTrue(result);
      Py_DECREF(args);
      Py_XDECREF(result);
      return bool_result;
    }
    void operator()(unsigned int i) const {
      if (!func || Py_None == func || !PyCallable_Check(func))
        return;
      PyObject *args = Py_BuildValue("(i)", i);
      PyObject *result = PyObject_Call(func,args,NULL);
      Py_DECREF(args);
      Py_XDECREF(result);
    }
};

%}

%include mac.i
%include radio.i
%include packet.i

#ifdef SWIGPYTHON
%typemap(in) FILE * {
  if (!PyFile_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Requires a file as a parameter.");
    return NULL;
  }
  $1 = PyFile_AsFile($input);
}

%typemap(out) variable_string_t {
  if ($1.isArray) {
    //printf("Generating array %s\n", $1.type);
    $result = listFromArray  ($1.type, $1.ptr, $1.len);
  }
  else {
    //printf("Generating scalar %s\n", $1.type);
    $result = valueFromScalar($1.type, $1.ptr, $1.len);
  }
  if ($result == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Error generating Python type from TinyOS variable.");
  }
}

%typemap(in) nesc_app_t* {
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Requires a list as a parameter.");
    return NULL;
  }
  else {
    int size = PyList_Size($input);
    int i = 0;
    nesc_app_t* app;

    if (size % 3 != 0) {
      PyErr_SetString(PyExc_RuntimeError, "List must have 2*N elements.");
      return NULL;
    }

    app = (nesc_app_t*)malloc(sizeof(nesc_app_t));

    app->numVariables = size / 3;
    app->variableNames = (const char**)calloc(app->numVariables, sizeof(char*));
    app->variableTypes = (const char**)calloc(app->numVariables, sizeof(char*));
    app->variableArray = (int*)calloc(app->numVariables, sizeof(int));

    for (i = 0; i < app->numVariables; i++) {
      PyObject* name = PyList_GetItem($input, 3 * i);
      PyObject* array = PyList_GetItem($input, (3 * i) + 1);
      PyObject* format = PyList_GetItem($input, (3 * i) + 2);
      if (PyString_Check(name) && PyString_Check(format)) {
        app->variableNames[i] = PyString_AsString(name);
        app->variableTypes[i] = PyString_AsString(format);
        if (strcmp(PyString_AsString(array), "array") == 0) {
          app->variableArray[i] = 1;
          //printf("%s is an array\n", PyString_AsString(name));
        }
        else {
          app->variableArray[i] = 0;
          //printf("%s is a scalar\n", PyString_AsString(name));
        }
      }
      else {
        app->variableNames[i] = "<bad string>";
        app->variableTypes[i] = "<bad string>";
      }
    }

    $1 = app;
  }
}
#endif

typedef struct variable_string {
  const char* type;
  char* ptr;
  int len;
  int isArray;
} variable_string_t;

typedef struct nesc_app {
  int numVariables;
  const char** variableNames;
  const char** variableTypes;
  int* variableArray;
} nesc_app_t;

class Variable {
 public:
  Variable(const char* name, const char* format, int array, int mote);
  ~Variable();
  variable_string_t getData();  
};

class Mote {
 public:
  Mote(nesc_app_t* app);
  ~Mote();

  unsigned long id();
  
  long long int euid();
  void setEuid(long long int id);

  
  long long int bootTime();
  void bootAtTime(long long int time);

  bool isOn();
  void turnOff();
  void turnOn();
  Variable* getVariable(char* name);

  void addNoiseTraceReading(int val);
  void createNoiseModel();
  int generateNoise(int when);
};

%extend Tossim {
  void runAllEvents(PyObject *continue_events, PyObject *callback) {
    $self->runAllEvents(PyCallback(continue_events), PyCallback(callback));
  }
}

class Tossim {
 public:
  Tossim(nesc_app_t* app);
  ~Tossim();
  
  void init();
  
  long long int time();
  long long int ticksPerSecond(); 
  void setTime(long long int time);
  char* timeStr();

  Mote* currentNode();
  Mote* getNode(unsigned long nodeID);
  void setCurrentNode(unsigned long nodeID);

  void addChannel(char* channel, FILE* file);
  bool removeChannel(char* channel, FILE* file);
  void randomSeed(int seed);

  bool runNextEvent();
  unsigned int runAllEvents(std::function<bool()> continue_events, std::function<void (unsigned int)> callback);

  MAC* mac();
  Radio* radio();
  Packet* newPacket();
};

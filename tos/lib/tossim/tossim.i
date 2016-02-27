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

#define LENGTH_TYPE(NAME) \
if (strcmp(type, #NAME) == 0) { \
    return sizeof(NAME); \
}

size_t lengthOfType(const char* type) {
    LENGTH_TYPE(uint8_t)
    LENGTH_TYPE(uint16_t)
    LENGTH_TYPE(uint32_t)
    LENGTH_TYPE(int8_t)
    LENGTH_TYPE(int16_t)
    LENGTH_TYPE(int32_t)
    LENGTH_TYPE(char)
    LENGTH_TYPE(short)
    LENGTH_TYPE(int)
    LENGTH_TYPE(long)
    LENGTH_TYPE(signed char)
    LENGTH_TYPE(unsigned char)
    LENGTH_TYPE(unsigned short)
    LENGTH_TYPE(unsigned int)
    LENGTH_TYPE(unsigned long)
    LENGTH_TYPE(float)
    LENGTH_TYPE(double)

    //printf("Unknown type (size) '%s'\n", type);

    return 1;
}

#define CONVERT_TYPE(NAME, CONVERT_FUNCTION) \
if (strcmp(type, #NAME) == 0) { \
    NAME val; \
    memcpy(&val, ptr, sizeof(NAME)); \
    return CONVERT_FUNCTION(val); \
}

PyObject* valueFromScalar(const char* type, const void* ptr, size_t len) {
    CONVERT_TYPE(uint8_t, PyLong_FromUnsignedLong)
    CONVERT_TYPE(uint16_t, PyLong_FromUnsignedLong)
    CONVERT_TYPE(uint32_t, PyLong_FromUnsignedLong)
    CONVERT_TYPE(int8_t, PyLong_FromLong)
    CONVERT_TYPE(int16_t, PyLong_FromLong)
    CONVERT_TYPE(int32_t, PyLong_FromLong)
    CONVERT_TYPE(char, PyLong_FromLong)
    CONVERT_TYPE(short, PyLong_FromLong)
    CONVERT_TYPE(int, PyLong_FromLong)
    CONVERT_TYPE(long, PyLong_FromLong)
    CONVERT_TYPE(unsigned char, PyLong_FromUnsignedLong)
    CONVERT_TYPE(unsigned short, PyLong_FromUnsignedLong)
    CONVERT_TYPE(unsigned int, PyLong_FromUnsignedLong)
    CONVERT_TYPE(unsigned long, PyLong_FromUnsignedLong)
    CONVERT_TYPE(float, PyFloat_FromDouble)
    CONVERT_TYPE(double, PyFloat_FromDouble)

    //printf("Unknown type (value) '%s'\n", type);

#if PY_VERSION_HEX < 0x03000000
    return PyString_FromStringAndSize((const char*)ptr, len);
#else
    return PyUnicode_DecodeASCII((const char*)ptr, len, "strict");
#endif
}

PyObject* listFromArray(const char* type, const void* ptr, int len) {
    size_t elementLen = lengthOfType(type);
    PyObject* list = PyList_New(0);
    //printf("Generating list of %s\n", type);
    for (const uint8_t* tmpPtr = (const uint8_t*)ptr; tmpPtr < (const uint8_t*)ptr + len; tmpPtr += elementLen) {
        PyList_Append(list, valueFromScalar(type, tmpPtr, elementLen));    
    }
    return list;
}

// From: https://stackoverflow.com/questions/11516809/c-back-end-call-the-python-level-defined-callbacks-with-swig-wrapper#new-answer
class PyCallback
{
private:
    PyObject *func;
    PyCallback& operator=(const PyCallback&) = delete; // Not allowed
public:
    PyCallback(PyCallback&& o) : func(o.func) {
        o.func = NULL;
    }
    PyCallback(const PyCallback& o) : func(o.func) {
        Py_XINCREF(func);
    }
    PyCallback(PyObject *pfunc) {
        if (!pfunc || Py_None == pfunc || !PyCallable_Check(pfunc))
        {
            PyErr_SetString(PyExc_TypeError, "Requires a callable as a parameter.");
            throw std::runtime_error("Python exception occurred");
        }
        func = pfunc;
        Py_XINCREF(func);
    }
    ~PyCallback() {
        Py_XDECREF(func);
    }

    bool operator()(double t) const {
        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, PyFloat_FromDouble(t));

        PyObject *result = PyObject_Call(func, args, NULL);

        bool bool_result = result != NULL && PyObject_IsTrue(result);

        Py_DECREF(args);
        Py_XDECREF(result);

        if (PyErr_Occurred() != NULL)
        {
            throw std::runtime_error("Python exception occurred");
        }

        return bool_result;
    }

    void operator()(unsigned int i) const {
        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, PyLong_FromUnsignedLong(i));

        PyObject *result = PyObject_Call(func, args, NULL);

        Py_DECREF(args);
        Py_XDECREF(result);

        if (PyErr_Occurred() != NULL)
        {
            throw std::runtime_error("Python exception occurred");
        }
    }
};

FILE* object_to_file(PyObject* o)
{
#if PY_VERSION_HEX < 0x03000000
    if (!PyFile_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Requires a file as a parameter.");
        return NULL;
    }
    return PyFile_AsFile(o);
#else
    long fileno = -1;
    if (PyLong_Check(o))
    {
        fileno = PyLong_AsLong(o);
    }
    else if (PyObject_HasAttrString(o, "fileno"))
    {
        PyObject* fileno_obj = PyObject_CallMethod(o, "fileno", NULL);
        if (fileno_obj == NULL)
        {
            PyErr_SetString(PyExc_TypeError, "Calling fileno failed.");
            return NULL;
        }

        fileno = PyLong_AsLong(fileno_obj);
        Py_DECREF(fileno_obj);

        if (fileno == -1 && PyErr_Occurred())
        {
            PyErr_SetString(PyExc_TypeError, "The result of fileno was incorrect.");
            return NULL;
        }
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "Requires an object with a fileno function or a fileno.");
        return NULL;
    }

    long fileno_dup = dup(fileno);
    if (fileno_dup == -1)
    {
        PyErr_Format(PyExc_TypeError, "Failed to duplicate fileno with error %d.", errno);
        return NULL;
    }

    FILE* result = fdopen(fileno_dup, "w");
    if (result == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Failed to fdopen file.");
        return NULL;
    }

    return result;
#endif
}

%}

%include mac.i
%include radio.i
%include packet.i

#ifdef SWIGPYTHON
// Need to convert from an object from python's "open" to a FILE* so that
// functions that take FILE* objects are correctly handled
%typemap(in) FILE * {
    $1 = object_to_file($input);
    if ($1 == NULL)
    {
        return NULL;
    }
}

%typemap(out) variable_string_t {
    if ($1.isArray) {
        $result = listFromArray($1.type, $1.ptr, $1.len);
    }
    else {
        $result = valueFromScalar($1.type, $1.ptr, $1.len);
    }
    if ($result == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Error generating Python type from TinyOS variable.");
    }
}

%{
// TODO: Fix the memory leak introduced here  by strdup
bool fill_nesc_app(nesc_app_t* app, int i, PyObject* name, PyObject* array, PyObject* format)
{
#if PY_VERSION_HEX < 0x03000000
    if (PyString_Check(name) && PyString_Check(format)) {
        app->variableNames[i] = PyString_AsString(name); // TODO: Should this be strdup'ed?
        app->variableTypes[i] = PyString_AsString(format); // TODO: Should this be strdup'ed?
        app->variableArray[i] = (strcmp(PyString_AsString(array), "array") == 0);

        return true;
    }
    else {
        free(app->variableNames);
        free(app->variableTypes);
        free(app->variableArray);
        free(app);
        PyErr_SetString(PyExc_RuntimeError, "bad string");
        return false;
    }
#else
    if (PyUnicode_Check(name) && PyUnicode_Check(format)) {

        PyObject* name_ascii = PyUnicode_AsASCIIString(name);
        PyObject* format_ascii = PyUnicode_AsASCIIString(format);

        app->variableNames[i] = strdup(PyBytes_AsString(name_ascii));
        app->variableTypes[i] = strdup(PyBytes_AsString(format_ascii));

        Py_DECREF(name_ascii);
        Py_DECREF(format_ascii);

        PyObject* array_ascii = PyUnicode_AsASCIIString(array);
        
        app->variableArray[i] = (strcmp(PyBytes_AsString(array_ascii), "array") == 0);

        Py_DECREF(array_ascii);
        return true;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "bad string");
        return false;
    }
#endif
}

%}

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
        app->variableNames = (const char**)malloc(app->numVariables * sizeof(char*));
        app->variableTypes = (const char**)malloc(app->numVariables * sizeof(char*));
        app->variableArray = (int*)malloc(app->numVariables * sizeof(int));

        for (i = 0; i < app->numVariables; i++) {
            PyObject* name = PyList_GetItem($input, 3 * i);
            PyObject* array = PyList_GetItem($input, (3 * i) + 1);
            PyObject* format = PyList_GetItem($input, (3 * i) + 2);
            if (!fill_nesc_app(app, i, name, array, format))
            {
                free(app->variableNames);
                free(app->variableTypes);
                free(app->variableArray);
                free(app);
                return NULL;
            }
        }

        $1 = app;
    }
}
#endif

typedef struct variable_string {
    const char* type;
    void* ptr;
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

    
    long long int bootTime() const noexcept;
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
    PyObject* runAllEvents(PyObject *continue_events, PyObject *callback) {
        try
        {
            unsigned int result = $self->runAllEvents(PyCallback(continue_events), PyCallback(callback));
            return PyLong_FromUnsignedLong(result);
        }
        catch (std::runtime_error ex)
        {
            return NULL;
        }
    }
}

class Tossim {
 public:
    Tossim(nesc_app_t* app);
    ~Tossim();
    
    void init();
    
    long long int time() const noexcept;
    double timeInSeconds() const noexcept;
    static long long int ticksPerSecond() noexcept;
    void setTime(long long int time) noexcept;
    const char* timeStr() noexcept;

    Mote* currentNode() noexcept;
    Mote* getNode(unsigned long nodeID) noexcept;
    void setCurrentNode(unsigned long nodeID) noexcept;

    void addChannel(const char* channel, FILE* file);
    bool removeChannel(const char* channel, FILE* file);
    void randomSeed(int seed);

    bool runNextEvent();
    unsigned int runAllEvents(std::function<bool(double)> continue_events, std::function<void (unsigned int)> callback);

    MAC* mac();
    Radio* radio();
    Packet* newPacket();
};

class JavaRandom
{
public:
  JavaRandom(long long int seed) noexcept;
  ~JavaRandom() = default;

  void setSeed(long long int seed) noexcept;
  long long int getSeed() const noexcept;

  long long int next(int bits) noexcept;

  double nextDouble() noexcept;
  double nextGaussian() noexcept;
};

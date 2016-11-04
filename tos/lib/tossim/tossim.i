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

%include <std_shared_ptr.i>
%shared_ptr(MAC)
%shared_ptr(Radio)
%shared_ptr(Packet)
%shared_ptr(Variable)

%{
#include <memory.h>
#include <tossim.h>
#include <sim_noise.h>

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
private:
    PyCallback& operator=(const PyCallback&) = delete; // Not allowed
public:
    PyCallback(PyCallback&& o) noexcept : func(o.func)
    {
        o.func = NULL;
    }
    PyCallback(const PyCallback& o) noexcept : func(o.func)
    {
        Py_XINCREF(func);
    }
    PyCallback(PyObject *pfunc) : func(pfunc)
    {
        if (!pfunc || Py_None == pfunc || !PyCallable_Check(pfunc))
        {
            PyErr_SetString(PyExc_TypeError, "Requires a callable as a parameter.");
            throw std::runtime_error("Python exception occurred");
        }
        Py_INCREF(func);
    }
    ~PyCallback() noexcept
    {
        Py_XDECREF(func);
    }

    bool operator()() const {
        PyObject *result = PyObject_CallObject(func, NULL);

        if (result != NULL)
        {
            const bool bool_result = PyObject_IsTrue(result);

            Py_DECREF(result);

            return bool_result;
        }
        else
        {
            throw std::runtime_error("Python exception occurred");
        }
    }

    void operator()(double t) const {
        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, PyFloat_FromDouble(t));

        PyObject *result = PyObject_CallObject(func, args);

        Py_DECREF(args);

        if (result != NULL)
        {
            Py_DECREF(result);
        }
        else
        {
            throw std::runtime_error("Python exception occurred");
        }
    }

    void operator()(long long int i) const {
        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, PyLong_FromLongLong(i));

        PyObject *result = PyObject_CallObject(func, args);

        Py_DECREF(args);

        if (result != NULL)
        {
            Py_DECREF(result);
        }
        else
        {
            throw std::runtime_error("Python exception occurred");
        }
    }

    void operator()(const char* str, size_t length) const {
#if PY_VERSION_HEX < 0x03000000
        PyObject *pystring = PyString_FromStringAndSize(str, length);
#else
        PyObject *pystring = PyUnicode_FromStringAndSize(str, length);
#endif
        if (pystring == NULL) {
            throw std::runtime_error("Bad string");
        }

        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, pystring);

        PyObject *result = PyObject_CallObject(func, args);

        Py_DECREF(args);

        if (result != NULL)
        {
            Py_DECREF(result);
        }
        else
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
    int fileno = PyObject_AsFileDescriptor(o);
    if (fileno == -1)
    {
        return NULL;
    }

    int fileno_dup = dup(fileno);
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
bool fill_nesc_app(NescApp* app, int i, PyObject* name, PyObject* array, PyObject* format)
{
#if PY_VERSION_HEX < 0x03000000
    if (PyString_Check(name) && PyString_Check(format) && PyString_Check(array)) {
        app->variableNames[i] = PyString_AsString(name);
        app->variableTypes[i] = PyString_AsString(format);
        app->variableArray[i] = (strcmp(PyString_AsString(array), "array") == 0);

        return true;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "bad string");
        return false;
    }
#else
    if (PyUnicode_Check(name) && PyUnicode_Check(format) && PyUnicode_Check(array)) {

        PyObject* name_ascii = PyUnicode_AsASCIIString(name);
        PyObject* format_ascii = PyUnicode_AsASCIIString(format);
        PyObject* array_ascii = PyUnicode_AsASCIIString(array);

        if (name_ascii == NULL || format_ascii == NULL || array_ascii == NULL)
        {
            Py_XDECREF(name_ascii);
            Py_XDECREF(format_ascii);
            Py_XDECREF(array_ascii);

            PyErr_SetString(PyExc_RuntimeError, "bad string not ascii");
            return false;
        }

        app->variableNames[i] = PyBytes_AsString(name_ascii);
        app->variableTypes[i] = PyBytes_AsString(format_ascii);
        app->variableArray[i] = (strcmp(PyBytes_AsString(array_ascii), "array") == 0);

        Py_DECREF(name_ascii);
        Py_DECREF(format_ascii);
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

%typemap(in) NescApp {
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Requires a list as a parameter.");
        SWIG_fail;
    }
    else {
        Py_ssize_t size = PyList_Size($input);
        Py_ssize_t i = 0;

        if (size < 0 || size % 3 != 0) {
            PyErr_SetString(PyExc_RuntimeError, "List must have 3*N elements.");
            SWIG_fail;
        }

        NescApp app(static_cast<unsigned int>(size) / 3);

        for (i = 0; i < app.numVariables; i++) {
            PyObject* name = PyList_GET_ITEM($input, 3 * i);
            PyObject* array = PyList_GET_ITEM($input, (3 * i) + 1);
            PyObject* format = PyList_GET_ITEM($input, (3 * i) + 2);
            if (!fill_nesc_app(&app, i, name, array, format))
            {
                SWIG_fail;
            }
        }

        $1 = std::move(app);
    }
}
#endif

%ignore variable_string;
%ignore variable_string_t;
typedef struct variable_string {
    const char* type;
    void* ptr;
    int len;
    bool isArray;
} variable_string_t;

%ignore NescApp;
class NescApp {
public:
    NescApp(unsigned int size)
        : numVariables(size)
        , variableNames(size)
        , variableTypes(size)
        , variableArray(size)
    {
    }

    unsigned int numVariables;
    std::vector<std::string> variableNames;
    std::vector<std::string> variableTypes;
    std::vector<bool> variableArray;
};

class Variable {
 public:
    Variable(const char* name, const char* format, int array, int mote);
    ~Variable();
    variable_string_t getData();  
};

class Mote {
 protected:
    Mote(const NescApp* app);
    ~Mote();

 public:
    unsigned long id() const noexcept;
  
    long long int euid() const noexcept;
    void setEuid(long long int id) noexcept;

    long long int tag() const noexcept;
    void setTag(long long int tag) noexcept;
    
    long long int bootTime() const noexcept;
    void bootAtTime(long long int time);

    bool isOn();
    void turnOff();
    void turnOn();
    std::shared_ptr<Variable> getVariable(const char* name_cstr);

    void reserveNoiseTraces(size_t num_traces);
    void addNoiseTraceReading(int val);
    void createNoiseModel();
    int generateNoise(int when);

    %extend {
        PyObject* addNoiseTraces(PyObject *traces)
        {
            if (!PyList_Check(traces)) {
                PyErr_SetString(PyExc_TypeError, "Requires a list as a parameter.");
                return NULL;
            }

            Py_ssize_t size = PyList_GET_SIZE(traces);

            $self->reserveNoiseTraces(size);

            for (Py_ssize_t i = 0; i != size; ++i)
            {
                PyObject* trace = PyList_GET_ITEM(traces, i);

                long trace_int;

                if (PyLong_Check(trace)) {
                    trace_int = PyLong_AsLong(trace);
                }
                else if (PyInt_Check(trace)) {
                    trace_int = PyInt_AsLong(trace);
                }
                else {
                    PyErr_SetString(PyExc_TypeError, "Requires a list of ints as a parameter.");
                    return NULL;
                }

                if (trace_int < NOISE_MIN || trace_int > NOISE_MAX) {
                    PyErr_Format(PyExc_ValueError, "Noise needs to be in valid range [%d, %d] but was %ld.",
                        NOISE_MIN, NOISE_MAX, trace_int);
                    return NULL;
                }

                $self->addNoiseTraceReading(trace_int);
            }

            Py_RETURN_NONE;
        }
    }
};

%extend Tossim {
    PyObject* addCallback(const char* channel, PyObject *callback) {
        try
        {
            $self->addCallback(channel, PyCallback(callback));
            Py_RETURN_NONE;
        }
        catch (std::runtime_error ex)
        {
            return NULL;
        }
    }

    PyObject* register_event_callback(PyObject *callback, double current_time) {
        try
        {
            $self->register_event_callback(PyCallback(callback), current_time);
            Py_RETURN_NONE;
        }
        catch (std::runtime_error ex)
        {
            return NULL;
        }
    }

    PyObject* runAllEventsWithTriggeredMaxTime(
        double duration,
        double duration_upper_bound,
        PyObject *continue_events)
    {
        try
        {
            long long int result = $self->runAllEventsWithTriggeredMaxTime(
                duration, duration_upper_bound, PyCallback(continue_events));
            return PyLong_FromLongLong(result);
        }
        catch (std::runtime_error ex)
        {
            return NULL;
        }
    }

    PyObject* runAllEventsWithTriggeredMaxTimeAndCallback(
        double duration,
        double duration_upper_bound,
        PyObject *continue_events,
        PyObject *callback)
    {
        try
        {
            long long int result = $self->runAllEventsWithTriggeredMaxTimeAndCallback(
                duration, duration_upper_bound, PyCallback(continue_events), PyCallback(callback));
            return PyLong_FromLongLong(result);
        }
        catch (std::runtime_error ex)
        {
            return NULL;
        }
    }
}

class Tossim {
 public:
    Tossim(NescApp app);
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
    void addCallback(const char* channel, std::function<void(const char*, size_t)> callback);

    void randomSeed(int seed);

    void register_event_callback(std::function<bool(double)> callback, double current_time);

    bool runNextEvent();

    void triggerRunDurationStart();

    long long int runAllEventsWithTriggeredMaxTime(
        double duration,
        double duration_upper_bound,
        std::function<bool()> continue_events);
    long long int runAllEventsWithTriggeredMaxTimeAndCallback(
        double duration,
        double duration_upper_bound,
        std::function<bool()> continue_events,
        std::function<void(long long int)> callback);

    std::shared_ptr<MAC> mac();
    std::shared_ptr<Radio> radio();
    std::shared_ptr<Packet> newPacket();
};

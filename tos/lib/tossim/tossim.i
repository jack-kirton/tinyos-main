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
        SWIG_fail;
    }
}


%typemap(out) variable_string_t {
    if (strcmp($1.type, "<no such variable>") == 0) {
        PyErr_Format(PyExc_RuntimeError, "no such variable");
        SWIG_fail;
    }

    if ($1.isArray) {
        $result = listFromArray($1.type, $1.ptr, $1.len);
    }
    else {
        $result = valueFromScalar($1.type, $1.ptr, $1.len);
    }

    if ($result == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Error generating Python type from TinyOS variable.");
        SWIG_fail;
    }
}

%{
bool fill_nesc_app(NescApp* app, PyObject* name, PyObject* value)
{
    if (!PyTuple_Check(value) || PyTuple_GET_SIZE(value) != 2)
    {
        PyErr_SetString(PyExc_RuntimeError, "bad value");
        return false;
    }

    PyObject* array = PyTuple_GET_ITEM(value, 0);
    PyObject* format = PyTuple_GET_ITEM(value, 1);

#if PY_VERSION_HEX < 0x03000000
    if (PyString_Check(name) && PyString_Check(format) && PyBool_Check(array)) {
        app->variables[PyString_AsString(name)] =
            std::make_tuple<bool, std::string>(
                array == Py_True,
                PyString_AsString(format)
            );

        return true;
    }
#else
    if (PyUnicode_Check(name) && PyUnicode_Check(format) && PyBool_Check(array)) {

        PyObject* name_ascii = PyUnicode_AsASCIIString(name);
        PyObject* format_ascii = PyUnicode_AsASCIIString(format);

        if (name_ascii == NULL || format_ascii == NULL)
        {
            Py_XDECREF(name_ascii);
            Py_XDECREF(format_ascii);

            PyErr_SetString(PyExc_RuntimeError, "bad string not ascii");
            return false;
        }

        app->variables[PyBytes_AsString(name_ascii)] =
            std::make_tuple<bool, std::string>(
                array == Py_True,
                PyBytes_AsString(format_ascii)
            );

        Py_DECREF(name_ascii);
        Py_DECREF(format_ascii);
        
        return true;
    }
#endif
    else {
        PyErr_SetString(PyExc_RuntimeError, "bad string");
        return false;
    }
}
%}

%typemap(in) NescApp {
    if (!PyDict_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Requires a dict as a parameter.");
        SWIG_fail;
    }
    else {
        NescApp app;

        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next($input, &pos, &key, &value))
        {
            if (!fill_nesc_app(&app, key, value))
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
    std::unordered_map<std::string, std::tuple<bool, std::string>> variables;
};

#define REVERSE_CONVERT_TYPE(NAME, CONVERT_FUNCTION) \
if (fmt == #NAME) { \
    const NAME val = (NAME)CONVERT_FUNCTION(data); \
    if (PyErr_Occurred()) \
    { \
        return NULL; \
    } \
    $self->setData(val); \
    Py_RETURN_NONE; \
}

%nodefaultctor Variable;
class Variable {
 public:
    ~Variable();

    variable_string_t getData();

    %extend {
        PyObject* setData(PyObject* data)
        {
            const std::string& fmt = $self->getFormat();

            if (PyString_CheckExact(data))
            {
                char* bytes = PyString_AS_STRING(data);
                Py_ssize_t size = PyString_GET_SIZE(data);

                bool result = $self->setData(bytes, size);

                if (!result)
                {
                    PyErr_Format(PyExc_RuntimeError,
                        "The provided bytes are of length %zd whereas %zu was expected.",
                        size, $self->getLen());
                    return NULL;
                }

                Py_RETURN_NONE;
            }

            REVERSE_CONVERT_TYPE(uint8_t, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(uint16_t, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(uint32_t, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(uint64_t, PyLong_AsUnsignedLongLong)
            REVERSE_CONVERT_TYPE(int8_t, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(int16_t, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(int32_t, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(int64_t, PyLong_AsLongLong)
            REVERSE_CONVERT_TYPE(char, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(short, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(int, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(long, PyLong_AsLong)
            REVERSE_CONVERT_TYPE(unsigned char, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(unsigned short, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(unsigned int, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(unsigned long, PyLong_AsUnsignedLong)
            REVERSE_CONVERT_TYPE(float, PyFloat_AsDouble)
            REVERSE_CONVERT_TYPE(double, PyFloat_AsDouble)

            PyErr_Format(PyExc_TypeError, "Unknown type.");
            return NULL;
        }
    }
};

%nodefaultctor Mote;
class Mote {
 public:
    ~Mote();
    
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

    %exception getVariable(const char*) {
        try {
            $action
        }
        catch (std::runtime_error ex) {
            PyErr_Format(PyExc_RuntimeError, "No such variable as %s.", arg2);
            SWIG_fail;
        }
    }

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
%#if PY_VERSION_HEX < 0x03000000
                else if (PyInt_Check(trace)) {
                    trace_int = PyInt_AsLong(trace);
                }
%#endif
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
    Tossim(NescApp app, bool should_free=true);
    ~Tossim();
    
    void init();
    
    long long int time() const noexcept;
    double timeInSeconds() const noexcept;
    static long long int ticksPerSecond() noexcept;
    void setTime(long long int time) noexcept;
    std::string timeStr() const;

    Mote* currentNode();
    Mote* getNode(unsigned long nodeID);
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

    MAC& mac();
    Radio& radio();
    std::shared_ptr<Packet> newPacket();
};

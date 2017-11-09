
#ifndef COOJA_MAX_BUFFER_SIZE
#define COOJA_MAX_BUFFER_SIZE 256
#endif

// This variable has a special name, see MspDebugOutput.java in Cooja.
volatile const char* cooja_debug_ptr;

void cooja_printf(const char* fmt, ...)  __attribute__((noinline)) @C() @spontaneous()
{
    char buf[COOJA_MAX_BUFFER_SIZE];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);

    atomic {
        cooja_debug_ptr = buf;
    }

    va_end(ap);
}

module CoojaPrintfC
{
    provides interface CoojaPrintf;
}

implementation
{
    command void CoojaPrintf.print_str(const char * const s)
    {
        cooja_debug_ptr = s;
    }
}

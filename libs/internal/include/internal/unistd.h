
// Compatibility include to substitute for unistd.h on Windows.

#ifdef _MSC_VER

#include <io.h>
#include <process.h>
#include <direct.h>
#define pid_t int

#else

#include <unistd.h>

#endif

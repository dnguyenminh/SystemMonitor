#include "SystemInfo.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

std::string getComputerName() {
    char name[256];

#if defined(_WIN32)
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) {
        return std::string(name);
    }
#elif defined(__linux__) || defined(__APPLE__)
    if (gethostname(name, sizeof(name)) == 0) {
        return std::string(name);
    }
#endif

    return "UnknownHost";
}

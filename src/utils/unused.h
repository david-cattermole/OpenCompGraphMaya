#ifndef OPENCOMPGRAPHMAYA_UTILS_UNUSED_H
#define OPENCOMPGRAPHMAYA_UTILS_UNUSED_H

// Used to indicate to the user that a variable is not used, and
// avoids the compilier from printing warnings/errors about unused
// variables.
//
// https://stackoverflow.com/questions/308277/what-are-the-consequences-of-ignoring-warning-unused-parameter/308286#308286
#ifdef _WIN32  // Windows MSVC

#define UNUSED(expr)                                                \
    __pragma(warning(push))                                         \
    __pragma(warning(disable:4127))                                 \
    do { (void)(expr); } while (0);                                 \
    __pragma(warning(pop))
#else  // Linux and MacOS
#define UNUSED(expr) do { (void)(expr); } while (0)
#endif

#endif // OPENCOMPGRAPHMAYA_UTILS_UNUSED_H

#ifndef _ERR_H_
#define _ERR_H_

// Error codes
// Positive values indicate an error in pthreads or another standard C library
// Zero indicates success
enum Pw2Error {
    MEMORY_ALLOCATION_ERROR = -1, // An allocation attempt in the implementation returned NULL
    CLOSED_POOL_ERROR = -2, // An attempt to defer work to a threadpool that is not acceptng work
    // (e.g. in the process of being destroyed)
    NULL_POINTER_ERROR = -3, // Null pointer was passed as an argument to function
    ZERO_THREADS_ERROR = -4 // `pool_size` == 0
};

// Macros to reduce clutter; call a function and deal with the failure whenever the call fails
// As a bonus, potentially failing calls now stand out of the rest of the code if macro uses are highlighted

// Return error code on error; can only be used in int functions
#define return_on_err(stmt) \
    do { \
        int err_ = stmt; \
        if (err_ != 0) { \
            return err_; \
        } \
    } while (0)

// Silently ignore the error
#define silent_on_err(stmt) stmt

#endif //_ERR_H_

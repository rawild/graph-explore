#include <sys/time.h>

// TIME HELPERS
extern double elapsed_base;

// timestamp()
//    Return the current absolute time as a real number of seconds.
double timestamp(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (double) now.tv_usec / 1000000;
}

// elapsed(double elapsed_base)
//    Return the number of seconds that have elapsed since `elapsed_base`.
double elapsed() {
    return timestamp() - elapsed_base;
}


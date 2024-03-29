#ifndef KAPUC_HELPER_H
#define KAPUC_HELPER_H

#define SUPER_AGGRESSIVE_DEBUG

#ifdef BENCH_TIMER
#include <time.h>
#define BENCH_TIMER_SETUP                                                      \
  clock_t begin = clock();                                                     \
  clock_t end;                                                                 \
  double time_spent;
#define BENCH_TIMER_HELPER(n, b)                                               \
  begin = clock();                                                             \
  b end = clock();                                                             \
  time_spent = (float)(end - begin) / CLOCKS_PER_SEC;                          \
  log_info("[BENCH_TIMER] time %s: %.7f", n, time_spent);
#else
#define BENCH_TIMER_SETUP
#define BENCH_TIMER_HELPER(n, b) b
#endif

#endif // KAPUC_HELPER_H

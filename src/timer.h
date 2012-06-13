
#ifndef TIMING_H
#define TIMING_H

struct Timer;

struct Timer* CreateTimer();
int DestroyTimer(struct Timer** t);
int startTimer(struct Timer* t);
int stopTimer(struct Timer* t);
double getUserTime(struct Timer* t);
double getSystemTime(struct Timer* t);
double getWallTime(struct Timer* t);

void printMemoryStats();

// FIXME: need to sort out timing for win32...

/* A useful way to time a single function call? */
#ifdef _MSC_VER
#  define TIME_FUNCTION_CALL
#else
#  define TIME_FUNCTION_CALL(time_array,timer,return_value,function,...)  \
  struct Timer* timer = CreateTimer();                                  \
  startTimer(timer);                                                    \
  return_value = function(__VA_ARGS__);                                 \
  stopTimer(timer);                                                     \
  time_array[0] += getUserTime(timer);                                  \
  time_array[1] += getSystemTime(timer);                                \
  time_array[2] += getWallTime(timer);                                  \
  DestroyTimer(&timer)
#endif

#endif /* TIMING_H */

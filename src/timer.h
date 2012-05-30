
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

/* A useful way to time a single function call? */
#define TIME_FUNCTION_CALL(time_array,timer,return_value,function,...)  \
  struct Timer* timer = CreateTimer();                                  \
  startTimer(timer);                                                    \
  return_value = function(__VA_ARGS__);                                 \
  stopTimer(timer);                                                     \
  time_array[0] += getUserTime(timer);                                  \
  time_array[1] += getSystemTime(timer);                                \
  time_array[2] += getWallTime(timer);                                  \
  DestroyTimer(&timer)

#endif /* TIMING_H */

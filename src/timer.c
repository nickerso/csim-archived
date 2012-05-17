/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is CellMLSimulator.
 *
 * The Initial Developer of the Original Code is
 * David Nickerson <nickerso@users.sourceforge.net>.
 * Portions created by the Initial Developer are Copyright (C) 2007-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <stdio.h>
#include <stdlib.h>
/* Timing information -- using BSD timers since that is
   what CM uses */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
// not working on mac os x
#if 0
#include <malloc.h>
#endif

#include "timer.h"

struct Timer
{
  enum {NONE,STARTED,STOPPED} state;
  double user[2];    /* CPU user */
  double system[2];  /* CPU system */
  double wall[2];    /* Wall clock */
};

struct Timer* CreateTimer()
{
  struct Timer* t = (struct Timer*)malloc(sizeof(struct Timer));
  t->user[0] = 0;
  t->user[1] = 0;
  t->system[0] = 0;
  t->system[1] = 0;
  t->wall[0] = 0;
  t->wall[1] = 0;
  t->state = NONE;
  return(t);
}

int DestroyTimer(struct Timer** t)
{
  free(*t);
  *t = NULL;
  return(1);
}

int startTimer(struct Timer* t)
{
  struct rusage r;
  struct timeval  tv;
  struct timezone tz;
  double c = 1000000.0;

  /* This could be used for memory stuff also */
  getrusage(RUSAGE_SELF,&r);
  /* number of seconds since epoch */
  gettimeofday(&tv,&tz);

  /* convert useconds to seconds */
  t->user[0]   = (double)(r.ru_utime.tv_sec) + (double)r.ru_utime.tv_usec/c;
  t->system[0] = (double)(r.ru_stime.tv_sec) + (double)r.ru_stime.tv_usec/c;
  t->wall[0]   = ((double)tv.tv_sec) + ((double)tv.tv_usec)/c;

  t->state = STARTED;
  return(1);
}

int stopTimer(struct Timer* t)
{
  struct rusage r;
  struct timeval  tv;
  struct timezone tz;
  double c = 1000000.0;

  if (t->state != STARTED)
  {
    fprintf(stderr,"Timer not started\n");
    return(0);
  }
  
  /* This could be used for memory stuff also */
  getrusage(RUSAGE_SELF,&r);
  /* number of seconds since epoch */
  gettimeofday(&tv,&tz);

  /* convert useconds to seconds */
  t->user[1]   = (double)(r.ru_utime.tv_sec) + (double)r.ru_utime.tv_usec/c;
  t->system[1] = (double)(r.ru_stime.tv_sec) + (double)r.ru_stime.tv_usec/c;
  t->wall[1]   = ((double)tv.tv_sec) + ((double)tv.tv_usec)/c;

  t->state = STOPPED;
  return(1);
}

double getUserTime(struct Timer* t)
{
  if (t->state == STOPPED)
  {
    return(t->user[1] - t->user[0]);
  }
  else
  {
    fprintf(stderr,"Timer not stopped\n");
    return(-1);
  }
}

double getSystemTime(struct Timer* t)
{
  if (t->state == STOPPED)
  {
    return(t->system[1] - t->system[0]);
  }
  else
  {
    fprintf(stderr,"Timer not stopped\n");
    return(-1);
  }
}

double getWallTime(struct Timer* t)
{
  if (t->state == STOPPED)
  {
    return(t->wall[1] - t->wall[0]);
  }
  else
  {
    fprintf(stderr,"Timer not stopped\n");
    return(-1);
  }
}

void printMemoryStats()
{
  /* this code should work, but most of it isn't implemented
     under Linux */
#if 0
  struct rusage r;

  getrusage(RUSAGE_SELF,&r);

  long ru_maxrss = r.ru_maxrss;       /* maximum resident set size */
  long ru_ixrss = r.ru_ixrss;         /* integral shared memory size */
  long ru_idrss = r.ru_idrss;         /* integral unshared data size */
  long ru_isrss = r.ru_isrss;         /* integral unshared stack size */
  long ru_minflt = r.ru_minflt;       /* page reclaims */
  long ru_majflt = r.ru_majflt;       /* page faults */
  long ru_nswap = r.ru_nswap;         /* swaps */
  long ru_inblock = r.ru_inblock;     /* block input operations */
  long ru_oublock = r.ru_oublock;     /* block output operations */
  long ru_msgsnd = r.ru_msgsnd;       /* messages sent */
  long ru_msgrcv = r.ru_msgrcv;       /* messages received */
  long ru_nsignals = r.ru_nsignals;   /* signals received */
  long ru_nvcsw = r.ru_nvcsw;         /* voluntary context switches */
  long ru_nivcsw = r.ru_nivcsw;       /* involuntary context switches */

  printf("maximum resident set size: %ld\n",ru_maxrss);
  printf("integral shared memory size: %ld\n",ru_ixrss);
  printf("integral unshared data size: %ld\n",ru_idrss);
  printf("integral unshared stack size: %ld\n",ru_isrss);
  printf("page reclaims: %ld\n",ru_minflt);
  printf("page faults: %ld\n",ru_majflt);
  printf("swaps: %ld\n",ru_nswap);
  printf("block input operations: %ld\n",ru_inblock);
  printf("block output operations: %ld\n",ru_oublock);
  printf("messages sent: %ld\n",ru_msgsnd);
  printf("messages received: %ld\n",ru_msgrcv);
  printf("signals received: %ld\n",ru_nsignals);
  printf("voluntary context switches: %ld\n",ru_nvcsw);
  printf("involuntary context switches: %ld\n",ru_nivcsw);
#endif
// not working on mac os x
#if 0

  /* apparently casting the int's inside mallinfo to unsigned int's will
     get the right sizes if you are using more than 2GB...maybe */
  struct mallinfo mInfo = mallinfo();
  unsigned systemBytes = (unsigned)(mInfo.arena);
  unsigned inUseBytes = (unsigned)(mInfo.uordblks);
  unsigned freeBytes = (unsigned)(mInfo.fordblks);
  /*systemBytes /= 1024;
  inUseBytes /= 1024;
  freeBytes /= 1024;*/
  printf("  Total allocated space: %u bytes\n",systemBytes);
  printf("                 in use: %u bytes\n",inUseBytes);
  printf("                   free: %u bytes\n",freeBytes);
#endif
#if 0
  printf("non-mmapped space allocated from system: %d\n",mInfo.arena);
  printf("number of free chunks: %d\n",mInfo.ordblks);
  printf("number of fastbin blocks: %d\n",mInfo.smblks);
  printf("number of mmapped regions: %d\n",mInfo.hblks);
  printf("space in mmapped regions: %d\n",mInfo.hblkhd);
  printf("maximum total allocated space: %d\n",mInfo.usmblks);
  printf("space available in freed fastbin blocks: %d\n",mInfo.fsmblks);
  printf("total allocated space: %d\n",mInfo.uordblks);
  printf("total free space: %d\n",mInfo.fordblks);
  printf("top-most, releasable (via malloc_trim) space: %d\n",mInfo.keepcost);
  malloc_stats();
#endif
}

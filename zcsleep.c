#include <stdio.h>
#include <time.h>
#include <errno.h>

int timespec_add(struct timespec *accum, const struct timespec *addend);
int sleep_loop(clockid_t clock_id, const struct timespec *duration);

int clock_specified = 0;
clockid_t clockid = CLOCK_REALTIME; /* default to the real-time clock */

int main(int argc, char **argv)
{
    struct timespec ts;

    ts.tv_sec = 5;
    ts.tv_nsec = 0;

    printf("time to sleep: {%lld,%ld}\n", (long long) ts.tv_sec, ts.tv_nsec);

    return sleep_loop(CLOCK_MONOTONIC, &ts);
}

#define GIGA ((long) (1000L * 1000L * 1000L))

int timespec_add(struct timespec *accum, const struct timespec *addend)
{
    long sec, nsec;
    if (accum == NULL || addend == NULL)
        return 0;

    nsec = accum->tv_nsec + addend->tv_nsec;
    sec  = accum->tv_sec  + addend->tv_sec + (nsec / GIGA);

    if (sec < accum->tv_sec) /* overflow! */
        return 0;

    accum->tv_sec  = sec;
    accum->tv_nsec = nsec % GIGA;

    return 1;
}

/* Given the clock to use and the duration we are supposed to sleep,
   try sleeping for that duration, handling interruptions due to signals */
int sleep_loop(clockid_t clock_id, const struct timespec *duration)
{
    int err = 0;
    struct timespec begin, target;

    if(clock_gettime(clock_id, &target) != 0) { /* the start time - now */
        fputs("Unable to get time for specified clock!\n", stderr);
        return 1;
    }

    if (!timespec_add(&target, duration)) /* get our desired wakeup time */
        return 1;

    while((err = clock_nanosleep(clock_id, TIMER_ABSTIME, &target, NULL)) != 0) {
        if (err != EINTR)
            return err;
    }

    return 0;
}

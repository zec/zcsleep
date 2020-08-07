/*
 * zcsleep - sleep for a given amount, using a non-default system clock
 *
 * Copyright (c) 2020 Zachary Catlin. See COPYING for terms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>


#define GIGA ((long) (1000L * 1000L * 1000L))

int process_arg(const char *arg, const char *prog_name, clockid_t *clkid,
                struct timespec *ts);
int clock_supported(clockid_t clk);
int timespec_add(struct timespec *accum, const struct timespec *addend);
int sleep_loop(clockid_t clock_id, const struct timespec *duration);

typedef struct {
    const char * const cmdline_option;
    const char * const help_description;
    clockid_t clk_id;
} clk_opt;

#define TBL_ENTRY(clk, opt, desc) { opt, desc " (" #clk ")", clk },

const clk_opt options[] = {
    TBL_ENTRY(CLOCK_REALTIME, "-r", "system real-time clock")
#if defined(CLOCK_MONOTONIC)
    TBL_ENTRY(CLOCK_MONOTONIC, "-m",  "monotonically increasing clock")
#endif
#if defined(CLOCK_BOOTTIME)
    TBL_ENTRY(CLOCK_BOOTTIME, "-b",  "monotonically increasing clock, including time when system is suspended")
#endif
};

#define NUM_CLKS (sizeof(options) / sizeof(options[0]))

int main(int argc, char **argv)
{
    struct timespec ts;
    int i;
    int n;
    clockid_t clockid = CLOCK_REALTIME; /* default to the real-time clock */

    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    if (argc < 2) {
        fprintf(stderr, "%s: no args specified\nUsage: %s [clock] duration\n",
                argv[0], argv[0]);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if ((n = process_arg(argv[i], argv[0], &clockid, &ts)) != 0)
            return n - 1;
    }

    return sleep_loop(clockid, &ts);
}

/* returns 0 normally, 1 on "program should exit normally", >1 for abnormal exit */
int process_arg(const char *arg, const char *prog_name, clockid_t *clkid,
                struct timespec *ts)
{
    static int clock_specified = 0;
    static int duration_specified = 0;

    double time_in_seconds;
    double whole_number_seconds;
    time_t wns_tt;
    char *endp = NULL;
    int i;

    if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
        fprintf(stderr,
                "Usage: %s [clock] duration\n"
                "Duration is in seconds\n"
                "\n"
                "Available clocks:\n",
                prog_name);
        for (i = 0; i < NUM_CLKS; i++) {
            if (clock_supported(options[i].clk_id))
                fprintf(stderr, "  %5s %s\n",
                        options[i].cmdline_option, options[i].help_description);
        }
        return 1;
    }

    for (i = 0; i < NUM_CLKS; i++) {
        if (!strcmp(arg, options[i].cmdline_option)) {
            if (clock_specified) {
                fprintf(stderr, "%s: Multiple clocks specified!\n", prog_name);
                return 2;
            }
            clock_specified = 1;

            if (!clock_supported(options[i].clk_id)) {
                fprintf(stderr,
                        "%s: The clock for \'%s\' is not supported on this system\n",
                        prog_name, arg);
                return 2;
            }
            *clkid = options[i].clk_id;
            return 0;
        }
    }

    /* If we get here, the only thing left to be is a duration */
    if (duration_specified) {
        fprintf(stderr, "%s: extra duration or unrecogized option \'%s\'\n",
                prog_name, arg);
        return 2;
    }
    duration_specified = 1;

    errno = 0;
    time_in_seconds = strtod(arg, &endp);
    if ((errno != 0) || (endp == arg) || (endp == NULL)) {
        fprintf(stderr, "%s: invalid duration \'%s\'\n", prog_name, arg);
        return 2;
    }

    while (*endp != '\0') {
        if (!isspace(*endp++)) {
            fprintf(stderr, "%s: invalid duration \'%s\'\n",
                    prog_name, arg);
            return 2;
        }
    }

    /* If we get here, arg really was a valid number (and the first number
       in the arguments); let's turn it into a timespec */
    if (time_in_seconds < 0.0) {
        fprintf(stderr, "%s: negative duration not allowed!\n", prog_name);
        return 2;
    }

    whole_number_seconds = floor(time_in_seconds);
    wns_tt = (time_t) whole_number_seconds;
    if (whole_number_seconds != (double) wns_tt) {
        fprintf(stderr, "%s: duration %s is too long!\n", prog_name, arg);
        return 2;
    }

    ts->tv_sec = wns_tt;
    ts->tv_nsec = (long) ((time_in_seconds - whole_number_seconds) * (double)GIGA);
    return 0;
}

/* Is the given clockid_t supported on this system? */
int clock_supported(clockid_t clk)
{
    int n;
    struct timespec ts;

    errno = 0;
    n = clock_getres(clk, &ts);

    return (n == 0) || (errno != EINVAL);
}



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
        if (err == EINVAL)
            fprintf(stderr, "Specified clock is not allowed!\n");

        if (err != EINTR)
            return err;
    }

    return 0;
}

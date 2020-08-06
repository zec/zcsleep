#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>

/* for getpid() and sigaction */
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void null_handler(int x)
{
    return;
}

int main(int argc, char **argv)
{
    struct timespec ts, tr;
    int ret_val;

    struct sigaction new_act;

    new_act.sa_handler = null_handler;
    sigfillset(&new_act.sa_mask);
    new_act.sa_flags = 0;
    sigaction(SIGUSR1, &new_act, NULL);
    sigaction(SIGUSR2, &new_act, NULL);
    sigaction(SIGINT,  &new_act, NULL);

    printf("hello, world\n");
    printf("PID %llu\n", (unsigned long long) getpid());

    ts.tv_sec = 5;
    ts.tv_nsec = 0;

    tr.tv_sec = 9999;
    tr.tv_nsec = 1729;

    printf("time to sleep: {%lld,%ld}\n", (long long) ts.tv_sec, ts.tv_nsec);

    errno = 0;
    ret_val = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &tr);

    printf("\ntime to sleep: {%lld,%ld}\n", (long long) ts.tv_sec, ts.tv_nsec);
    printf("remaining time: {%lld,%ld}\n", (long long) tr.tv_sec, tr.tv_nsec);
    printf("return value: %d (%s)\n", ret_val, strerror(ret_val));
    printf("errno = %d (%s)\n", errno, strerror(errno));
    printf("EINTR = %d\n", EINTR);

    return 0;
}

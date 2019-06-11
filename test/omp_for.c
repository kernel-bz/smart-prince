#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>    ///-fopenmp -lgomp

static void get_micro_time(long *sec, long *ms)
{
    //struct timespec ts;
    struct timeval tv;
    //clock_t sec;

    //clock_gettime(CLOCK_REALTIME, &ts);
    gettimeofday(&tv, NULL);
    //time(&sec);

    *sec = tv.tv_sec;
    *ms = tv.tv_usec;
}

static void loop1(void)
{
    int i, s=0, cnt=0;
    long s_sec, e_sec, s_ms, e_ms;

    get_micro_time(&s_sec, &s_ms);

    for (i=1; i<=100; i++) {
            cnt++;
            s += cnt;
    }

    get_micro_time(&e_sec, &e_ms);
    printf("time=%ld sec %ld us, cnt=%d, sum=%d\n"
                    , e_sec-s_sec, e_ms-s_ms, cnt, s);
    cnt=0;
    s=0;

    get_micro_time(&s_sec, &s_ms);
    //#pragma omp parallel shared(cnt, s) reduction(+:s)
    //#pragma omp parallel shared(cnt, s)
    #pragma omp parallel reduction(+:s)
    {
        #pragma omp for
        for (i=1; i<=100; i++) {
                //cnt++;
                //s += cnt;
                s += i;
        }
    }
    get_micro_time(&e_sec, &e_ms);
    printf("time=%ld sec %ld us, cnt=%d, sum=%d\n"
                    , e_sec-s_sec, e_ms-s_ms, i, s);

}

static void loop2(void)
{
    int i, j, cnt=0, s=0;
    long s_sec, e_sec, s_ms, e_ms;

    get_micro_time(&s_sec, &s_ms);

    for (i=1; i<=10; i++) {
        for (j=1; j<=10; j++) {
                cnt++;
                s += cnt;
        }
    }
    get_micro_time(&e_sec, &e_ms);
    printf("time=%ld sec %ld us, cnt=%d, sum=%d\n"
                    , e_sec-s_sec, e_ms-s_ms, cnt, s);

    cnt=0;
    s=0;

    get_micro_time(&s_sec, &s_ms);

    #pragma omp parallel shared(cnt) reduction(+:s)
    {
        #pragma omp for
        for (i=1; i<=10; i++) {
            for (j=1; j<=10; j++) {
                    cnt++;
                    s += cnt;
            }
        }
    }
    get_micro_time(&e_sec, &e_ms);
    printf("time=%ld sec %ld us, cnt=%d, sum=%d\n"
                    , e_sec-s_sec, e_ms-s_ms, cnt, s);
}


int main()
{
    loop1();
    loop2();

    return 0;
}

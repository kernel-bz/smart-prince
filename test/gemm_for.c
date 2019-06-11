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

void gemm(int M, int N, int K, float ALPHA,
        float *A, int lda,
        float *B, int ldb,
        float *C, int ldc)
{
    int i,j,k;
    for(i = 0; i < M; ++i){
        for(j = 0; j < N; ++j){
            register float sum = 0;
            for(k = 0; k < K; ++k){
                sum += ALPHA*A[i*lda+k]*B[j*ldb + k];
            }
            C[i*ldc+j] += sum;
        }
    }
}

void gemm_omp(int M, int N, int K, float ALPHA,
        float *A, int lda,
        float *B, int ldb,
        float *C, int ldc)
{
    int i,j,k;
    register float sum = 0;

    #pragma omp parallel for shared(C) reduction(+:sum)
    for(i = 0; i < M; ++i){
        for(j = 0; j < N; ++j){
            ///register float sum = 0;
            sum = 0;
            for(k = 0; k < K; ++k){
                sum += ALPHA*A[i*lda+k]*B[j*ldb + k];
            }
            C[i*ldc+j] += sum;
        }
    }
}


int main()
{
    int i;
    float A[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    float B[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    float C[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    long s_sec, e_sec, s_ms, e_ms;

    get_micro_time(&s_sec, &s_ms);
    gemm(3, 3, 3, 1, A, 0, B, 0, C, 0);
    get_micro_time(&e_sec, &e_ms);
    printf("run time: %ld sec %ld us\n", e_sec-s_sec, e_ms-s_ms);

    for(i=0; i<9; i++)
        printf("%f, ", C[i]);
    printf("\n");

    for(i=0; i<9; i++) {
        A[i] = i+1;
        B[i] = i+1;
        C[i] = i+1;
    }
    get_micro_time(&s_sec, &s_ms);
    gemm_omp(3, 3, 3, 1, A, 0, B, 0, C, 0);
    get_micro_time(&e_sec, &e_ms);
    printf("run time: %ld sec %ld us\n", e_sec-s_sec, e_ms-s_ms);

    for(i=0; i<9; i++)
        printf("%f, ", C[i]);
    printf("\n");

    return 0;
}

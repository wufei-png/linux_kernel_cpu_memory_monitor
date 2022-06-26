
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

double cpu_load(double start, double end, double used) 
{
    return used / (end - start) * 100;
}

double cpu_time_used_s(void) 
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
		// RUSAGE_SELF
    //           返回调用进程的资源使用统计信息，
    //           这是所有线程使用的资源的总和
    //           过程。
    return (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / (double)1000000 
				+ (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / (double)1000000;
}

double get_time_s() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / (double)1000000;
}

int main(int argc, char *argv[])
{
	unsigned long x, i;
	double start = get_time_s();

	printf(">>>>>>>my pid is %d\n", getpid());
	
	sleep(5);
	
	srand(time(NULL));
	x = rand();
	for (i = 0; i < 1000000000; i++) {
		x ^= rand();
		x |= rand();
		x &= ~rand();
	}

	double end = get_time_s();
		double real_time_used = end - start;
		double cpu_time_used = cpu_time_used_s();
	
		printf("start: %.3fs, end: %.3fs\n"
			   "real_time_used: %.3f\n"
			   "cpu_time_used: %.3fs, cpu_load: %.3f%%\n",
			   start, end, real_time_used, cpu_time_used, cpu_load(start, end, cpu_time_used));

	return 0;
}


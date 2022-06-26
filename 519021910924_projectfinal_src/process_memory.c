
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
    return (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / (double)1000000 
				+ (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / (double)1000000;
}

double get_time_s() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / (double)1000000;
}

void memory_usleep(int usecs)//单位ms
{
#define NANOS_PER_USEC 1000
#define USEC_PER_SEC   1000000
usecs=usecs*1000;
    long seconds = usecs / USEC_PER_SEC;
    long nanos   = (usecs % USEC_PER_SEC) * NANOS_PER_USEC;
    struct timespec t = { .tv_sec = seconds, .tv_nsec = nanos };
    int ret;
    do
    {
        ret = nanosleep( &t, &t );
        // need to loop, `nanosleep` might return before sleeping
        // for the complete time (see `man nanosleep` for details)
    } while (ret == -1 && (t.tv_sec || t.tv_nsec));
}

int main(int argc, char *argv[])
{
	unsigned long i;
	double start = get_time_s();

	printf(">>>>>>>my pid is %d\n", getpid());
	
	sleep(5);

	char *ptr;

	while(1) {
		memory_usleep(1);
		ptr = malloc(8092);//malloc 向系统申请分配指定size个字节的内存空间。返回类型是 void* 类型
		memcpy(ptr, "this is test memory@@@@@@@@", 20);
		free(ptr);
//C 库函数 void *memcpy(void *str1, const void *str2, size_t n) 从存储区 str2 复制 n 个字节到存储区 str1。
		//printf("mem size %lu\n", all_mem_size);
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



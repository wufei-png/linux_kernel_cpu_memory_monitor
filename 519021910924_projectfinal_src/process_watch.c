
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

#define PWATCH_DEVICE_NAME "/dev/pwatch"
// int sampling_time_ms = 10;//default
double cpu_time_used_s(void) 
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
		// RUSAGE_SELF
    //           返回调用进程的资源使用统计信息，
    //           这是所有线程使用的资源的总和
    //           过程。
    return (double)usage.ru_stime.tv_sec*(double)1000000 + (double)usage.ru_stime.tv_usec 
				+ (double)usage.ru_utime.tv_sec*(double)1000000 + (double)usage.ru_utime.tv_usec ;
}

double get_time_s() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec*(double)1000000 + (double)tv.tv_usec;
}

static void pwatch_microsleep(unsigned int usecs)
{
#define NANOS_PER_USEC 1000
#define USEC_PER_SEC   1000000
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

void pw_calc_usage(char *buf, unsigned long *cpu_time, unsigned long *mem_size)
{
	char *token, *last;
	int seq = 0;

	seq = 0;
	token = strtok(buf, " ");//空格为标识符分割
	//C 库函数 char *strtok(char *str, const char *delim) 分解字符串 str 为一组字符串，delim 为分隔符。该函数返回被分解的第一个子字符串
	while (token) {
		// printf("token %s\n", token);
		if (seq == 0) {
			*cpu_time = strtoul(token, &last, 10);//转换成十进制
		} else {
			*mem_size = strtoul(token, &last, 10);
		}
		
		token = strtok(NULL, " ");
		/*Asecond call to strtok using a NULL
        as the first parameter returns a pointer
        to the character following the token*/
		seq++;
	}

	return;
}

int main(int argc, char *argv[])
{
	int count, time;
	int fd, pid;
	char filename[64] = {0};
	FILE *fp;
	int read_count, readn, writen;
	char read_buf[256] = {0};
	
	if (argc != 3) {
		printf("USAG:%s <pid> <sampling count> <sampling time(ms)>\n", argv[0]);
		return -1;
	}

	pid 	= atoi(argv[1]); 
	count 	= atoi(argv[2]);//采样次数
	// sampling_time_ms = atoi(argv[3]);
	sprintf(filename, "%d_sampling.txt", pid);
	fp = fopen(filename, "w+");
	if (fp == NULL) {
		fprintf(stderr, "Error: Unable to open %s\n", filename);
		return -1;
	}
	
    if (-1 == (fd = open(PWATCH_DEVICE_NAME, O_RDWR))) {
		fprintf(stderr, "Error: Unable to open %s, error %s\n", PWATCH_DEVICE_NAME, strerror(errno));
		fclose(fp);
   		return -1;
    }

	/* 通知监控进程的pid */
	memset(read_buf, 0x00, 256);
	readn = sprintf(read_buf, "%d", pid);
	writen = write(fd, read_buf, readn);//pwk_write
	if (writen <= 0) {
		fprintf(stderr, "Error: write fd %d error\n", fd);
		return -1;
	}
	
	/* 读取监控信息 */
	double cpu_usag;
	unsigned long cpu_time, cpu_time1, cpu_time2;
	unsigned long memsize;
	read_count = 0;
	char buf[256];

	//base
	memset(read_buf, 0x00, 256);
	readn = read(fd, read_buf, 256);
	if (readn <= 0) {
		fprintf(stderr, "Error: read fd %d error\n", fd);
		return -1;
	}
	pw_calc_usage(read_buf, &cpu_time1, &memsize);
	double time_first,time_last=0;
	while (read_count < count) {
		// pwatch_microsleep(sampling_time_ms*1000);

		memset(read_buf, 0x00, 256);
		readn = read(fd, read_buf, 256);
		if (readn <= 0) {
			fprintf(stderr, "Error: read fd %d error\n", fd);
			break;
		}
		read_count++;
		time_last=time_first;
		time_first = get_time_s();//系统时间，也就是默认cpu一直在工作
		pw_calc_usage(read_buf, &cpu_time2, &memsize);
		if (cpu_time2 >= cpu_time1) {
			cpu_time = cpu_time2 - cpu_time1;
		} else {
			cpu_time = cpu_time1 - cpu_time2;
		}
		//printf(">>cpu_time %lu", cpu_time);

		cpu_usag = (double)(cpu_time) / (time_first-time_last);//除以时间差，得到cpu使用率
		// memsize = memsize / (time_first-time_last);//除以时间差，得到内存频率 这里由于我之前多乘了一个PAGESIZE，单位是字节每us,改过来后这个数据到个位数了，所以要复现的话除以的数不能太小，否则精度损失有点大，可以分母除以1k，单位变成字节每ms即可，如下。
		memsize = memsize / ((time_first-time_last)/1000);
		readn = snprintf(buf, 256, "%.3lf %lu\n", cpu_usag, memsize);
		fwrite(buf, 1, readn, fp);
		fflush(fp);
		//printf(">>%s", buf);
		
		cpu_time1 = cpu_time2;
	}

	/* 关闭文件和fd */
	fclose(fp);
	close(fd);

	return 0;
}


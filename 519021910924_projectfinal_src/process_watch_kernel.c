
/*
 * 我们使用字符设备驱动实现，没有使用/proc的文件
 * 而且返回的是累计的数据，增量由用户空间程序计算
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/times.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/sched/autogroup.h>
#include <linux/sched/loadavg.h>
#include <linux/sched/stat.h>
#include <linux/sched/mm.h>
#include <linux/sched/coredump.h>
#include <linux/sched/task.h>
#include <linux/sched/cputime.h>
#include <linux/pgtable.h>
#include <linux/module.h>
#include<linux/time.h>
#include <linux/time64.h>
#include <linux/rtc.h>
#define PWK_DEVICE_NAME 	"pwatch"
#define PWK_CLASS_NAME 		"pwatch_class"
int __debug = 1;
#define pwk_dbg(format, arg...) do {\
		if (__debug)\
			pr_info(PWK_CLASS_NAME ": %s: " format, __FUNCTION__, ##arg); \
} while (0)
#define pwk_err(format, arg...) 	pr_err(PWK_CLASS_NAME ": " format, ##arg)
#define pwk_info(format, arg...) 	pr_info(PWK_CLASS_NAME ": " format, ##arg)
#define pwk_warn(format, arg...) 	pr_warn(PWK_CLASS_NAME ": " format, ##arg)

static struct class  *pwk_class 	= NULL;
static struct device *pwk_device 	= NULL;
static int pwk_major_num;
static unsigned long watch_pid;

unsigned long cpu_time;
unsigned long mem_size;

static int  currenttime(void)
{
	struct timespec64 tv;
	struct rtc_time tm;
	int year, mon, day, hour, min, sec;
	get_timespec64(&tv,NULL);
	rtc_time64_to_tm(tv.tv_sec, &tm);
	year = tm.tm_year + 1900;
	mon = tm.tm_mon + 1;
	day = tm.tm_mday;
	hour = tm.tm_hour + 8;
	min = tm.tm_min;
	sec = tm.tm_sec;
	printk("Current time: %d-%02d-%02d %02d:%02d:%02d\n", year, mon, day, hour, min, sec);
	return 0;
}

// typedef struct {//hash
// 		int count=-1;
// } kv;
// static kv hash_table[108000000];

// BKDR Hash Function
unsigned int BKDRHash(pte_t *ptep)
{		char *str;
		str = (char *)kmalloc(18 * sizeof(char), GFP_KERNEL); // 假设最长的int数据为4位
		char *str_start = str;
		sprintf(str, "%s", &ptep);
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
 
    while (*str)
    {
        hash = hash * seed + (*str);
				str=str+1;
    }
    // pr_info("hash:%d",(hash & 0x3FFFFFFF)) ;
		kfree(str_start);
		return (hash & 0x3FFFFFFF);
}
static int init=1;
static int wf_test(struct vm_area_struct *vma,//ptep_test_and_clear_young
					    unsigned long address,
					    pte_t *ptep)
{
	if(init){
		pr_info("%s%s","pte_addr:",&ptep);
		
		init=0;
	}
	pte_t pte = *ptep;
	int r = 1;
	if (!pte_young(pte))
		r = 0;
	else
		set_pte_at(vma->vm_mm, address, ptep, pte_mkold(pte));
	//pr_info("%s","wf_test_out");
	return r;
}


static  pte_t* _find_pte(struct vm_area_struct *vma, unsigned long addr)
{	//pr_info("%s","pte_inter");
    struct mm_struct *mm = vma->vm_mm;
		//pr_info("%s:%d","mm:",mm==NULL);
    pgd_t *pgd = pgd_offset(mm, addr);
    p4d_t *p4d = NULL;
    pud_t *pud = NULL;
    pmd_t *pmd = NULL;
    pte_t *pte = NULL;

    if(pgd_none(*pgd) || pgd_bad(*pgd))
		{//pr_info("%s","error_1");
        return NULL;
		}
    p4d = p4d_offset(pgd, addr);
    if(p4d_none(*p4d) || p4d_bad(*p4d))
				{//pr_info("%s","error_2");
        return NULL;
						}
    pud = pud_offset(p4d, addr);
				
    if(pud_none(*pud) || pud_bad(*pud))
       {//pr_info("%s","error_3"); 
				return NULL;
						}
    pmd = pmd_offset(pud, addr);
    if(pmd_none(*pmd) || pmd_bad(*pmd))
				{//pr_info("%s","error_4");
        return NULL;
						}
    pte = pte_offset_map(pmd, addr);
    if(pte_none(*pte) || !pte_present(*pte))
				{//pr_info("%s","error_5");
        return NULL;
		}
		//pr_info("%s","pte_out");
    return pte;
}
static unsigned long get_mem_freq(struct vm_area_struct *mmap_start){
	unsigned long mem=0;
	unsigned long sum=0;
	struct vm_area_struct *mmap=mmap_start;
	//currenttime();
	unsigned int hash=0;
	// while (mmap){//遍历vm_area_struct
	// 	unsigned long i=mmap->vm_start;
	// 	for (;i<mmap->vm_end;i++)//遍历虚拟地址
	// 	{
	// 		//pr_info("%s:%d","i",i);
	// 	pte_t *pte=_find_pte(mmap,i);//找到虚拟地址所在的page
	// 	if(pte)//存在NULL的情况{
	// 		hash=BKDRHash(pte);//hash
	// 		hash_table[hash].count+=1;
	// 	}
	// 	// pr_info("%s:%lu","mem",mem);
	// 	}
	// 	mmap=mmap->vm_next;
	// 	};

	while (mmap){//遍历vm_area_struct
		unsigned long i=mmap->vm_start;
		for (;i<mmap->vm_end;i++)//遍历虚拟地址
		{
			//pr_info("%s:%d","i",i);
			sum+=1;
		pte_t *pte=_find_pte(mmap,i);//找到虚拟地址所在的pte
		if(pte)//存在NULL的情况
			mem+=(unsigned long)wf_test(mmap,i,pte);
		// pr_info("%s:%lu","mem",mem);
		}
		mmap=mmap->vm_next;
		};
	//currenttime();
	pr_info("函数结束了");
	pr_info("%s:%lu","sum",sum);
		return mem;
	
}
static void pwk_accu_thread_rusage(struct task_struct *t, u64* utime,unsigned long* mem)
{
	u64 tgutime, tgstime;
	task_cputime_adjusted(t, &tgutime, &tgstime);
	*utime += tgutime;
	*mem+= get_mem_freq(t->mm->mmap);
	// r->ru_nvcsw += t->nvcsw;//nvcsw表示进程主动切换次数 ,ru_nvcsw 自愿上下文切换
	// r->ru_nivcsw += t->nivcsw; //上一行的非自愿
	// r->ru_minflt += t->min_flt;///ru_minflt * any page faults not requiring I/O */
	// r->ru_majflt += t->maj_flt;//min_flt ：次要页面错误的数量。 maj_flt ：主要页面错误的数量。
}

static int pwk_get_rusge(int pid, struct rusage *r)
{
	struct task_struct *t;
	struct task_struct *p;
	u64 tgutime, tgstime, utime;//累计cpu时间
	unsigned long mem = 0;//累计内存读写次数
	utime  = 0;
	int num_thread=0;//线程数
	int num_process=1;//进程数
	memset((char *)r, 0, sizeof (*r));
	p = get_pid_task(find_vpid(pid), PIDTYPE_PID);///find_vpid int pid-> 
// struct task_struct *get_pid_task(struct pid *pid, enum pid_type type)
// 这个函数用于通过全局pid和其类型找到对应的task结构，并增加这个task的usage
	 // https://www.cnblogs.com/linhaostudy/p/9582870.html
	if (p == NULL) {
		pwk_dbg("find pid %d error\n", pid);
		return -1;
	}

// 	void task_cputime_adjusted(struct task_struct *p, u64 *ut, u64 *st)
// {
//         *ut = p->utime;utime 是在用户空间的进程中实际执行的时间量，以毫秒为单位。 stime 是代表进程在 OS 内核中花费的时间量，也以毫秒为单位。
// utime 表示程序程序的什么时间? /proc/[pid]/stat 还有⼀个参数为 stime ，它表示什么？你觉得在这
// 个实验⾥⾯适合使⽤它吗?stime是系统时间，即进程在内核模式下花费的时间，而utime是在用户模式下花费的时间。这些值取决于该特定过程的安排。没有为其更新定义此类间隔。随着各个模式的时间变化，它们会快速更新。 当系统调用发生时，进程进入内核模式。
// }
	if(p==NULL)
			pr_info("p空指针出错了");
	if(p->mm->mmap==NULL)
			pr_info("p->mm->mmap空指针出错了");
	mem+= get_mem_freq(p->mm->mmap);
	// pwk_dbg("154hang11111111111111111\n");
	pwk_accu_thread_rusage(p,&utime,&mem);//主进程的
	t = p;
	// pwk_dbg("151hang11111111111111111\n");
	// #define while_each_thread(g, t) 
// 	while ((t = next_thread(t)) != g)
	while_each_thread(p, t) {
		pr_info("有子线程，进入！");
		num_thread+=1;
		pwk_accu_thread_rusage(t,&utime,&mem);
	};//遍历子线程
	// pwk_dbg("155hang11111111111111111\n");

	struct list_head *list;
	struct task_struct *child_task;
	struct task_struct *child_task_t;
list_for_each(list, &p->children) { //遍历子进程
pr_info("有子进程，进入！");
num_process+=1;
child_task = list_entry(list, struct task_struct, sibling); /* task now points to one of current’s children */ 
//https://stackoverflow.com/questions/34704761/why-sibling-list-is-used-to-get-the-task-struct-while-fetching-the-children-of-a 
//https://blog.//csdn.net/naedzq/article/details/51723538
pwk_accu_thread_rusage(child_task,&utime,&mem);
child_task_t=child_task;
	while_each_thread(child_task, child_task_t) {//遍历子进程中的子线程
		pr_info("有子进程中的子线程，进入！");
		num_thread+=1;
		pwk_accu_thread_rusage(child_task_t,&utime,&mem);
	};
}
pr_info("进程数：%d线程数：%d",num_process,num_thread);
// pwk_dbg("173hang11111111111111111\n");
	// stime += tgstime;
	// r->ru_nvcsw += p->signal->nvcsw;
	// r->ru_nivcsw += p->signal->nivcsw;
	// r->ru_minflt += p->signal->min_flt;
	// r->ru_majflt += p->signal->maj_flt;
	// r->ru_inblock += p->signal->inblock;//文件系统必须代表进程从磁盘读取的次数。
	// r->ru_oublock += p->signal->oublock;//文件系统必须代表进程写入磁盘的次数。

// 	long int ru_maxrss
// The maximum resident set size used, in kilobytes. That is, the maximum number of kilobytes of physical memory that processes used simultaneously.


	r->ru_utime = ns_to_kernel_old_timeval(utime);//时间转化
	// r->ru_stime = ns_to_kernel_old_timeval(stime);
/**
 * ns_to_kernel_old_timeval - Convert nanoseconds to timeval
 * @nsec:	the nanoseconds value to be converted
 *
 * Returns the timeval representation of the nsec parameter.
 */
	pwk_dbg("ru_utime: %lu\n", r->ru_utime);
	r->ru_maxrss =  mem* (PAGE_SIZE); /* convert pages to bytes */ //ru_maxrss 随便取一个变量存储内存数据把
	pwk_dbg("ru_utime: %lu, ru_maxrss: %lu,\n", r->ru_utime, r->ru_maxrss);
	return 0;
}

static int pwk_open(struct inode *the_inode, struct file *f)
{
        return 0;
}

static ssize_t pwk_read(struct file *f, char *buf, size_t len, loff_t *offset)
{
	char read_buf[256] = {0};
	int readn = 0;
	struct rusage rused;
	//unsigned long calc_mem_size, calc_cpu_time;

	if (pwk_get_rusge(watch_pid, &rused) == -1) {
		return 0;
	}

	cpu_time = (rused.ru_utime.tv_sec * 1000000 + rused.ru_utime.tv_usec);//long int tv_sec This represents the number of whole seconds of elapsed time.
//https://stackoverflow.com/questions/9407556/why-are-both-tv-sec-and-tv-usec-significant-in-determining-the-duration-of-a-tim

	mem_size = rused.ru_maxrss;//* (PAGE_SIZE);多乘了一次
	// mem_size = rused.ru_maxrss;
	pwk_dbg("cpu_time %lu, mem_size %lu\n", cpu_time, mem_size);

	//calc_cpu_time = cpu_time - base_cpu_time;
	readn = snprintf(read_buf, 256, "%lu %lu", cpu_time, mem_size);//用空格分隔

	pwk_dbg("pwk_read return %s, len %d\n", read_buf, readn);
	
	if (copy_to_user(buf, read_buf, readn)) {
		printk(KERN_ERR "Unable to copy struct to user\n");
		return -EFAULT;
	}
	return readn;
}

static ssize_t pwk_write(struct file *f, const char *buf, size_t len, loff_t *offset)
{
	char write_buf[256] = {0};
	char*after;
	//struct rusage rused;
	
	if (copy_from_user(write_buf, buf, len)) {
		pwk_err("Unable to copy struct to user\n");
		return -EFAULT;
	}
	
	watch_pid = simple_strtoul(write_buf, &after, 10);
	pwk_dbg("pwk_write buf %s, len %ld, watch_pid %lu,"
		"\n", write_buf, len, watch_pid);
	
	return len;
}

static int pwk_release(struct inode *the_inode, struct file *f)
{
	pwk_dbg("closed\n");
		
	return 0;
}


static struct file_operations pwk_fops = {
	.open 		= pwk_open,
	.read 		= pwk_read,
	.write 		= pwk_write,
	.release 	= pwk_release,
};


static char *pwk_devnode(struct device *dev, umode_t *mode)//typedef unsigned short		umode_t;
{
	if (mode)
		*mode = 0777;
		
	return NULL;
}

static int __init pwk_init(void)
{
	int retval;
  
  	pwk_major_num = register_chrdev(0, PWK_DEVICE_NAME, &pwk_fops);//注册设备
		//major device number or 0 for dynamic allocation
		//https://blog.csdn.net/liangkaiming/article/details/6234238
		// 一个字符设备或者块设备都有一个主设备号和次设备号。主设备号和次设备号统称为设备号。主设备号用来表示一个特定的驱动程序。次设备号用来表示使用该驱动程序的各设备。
  	if (pwk_major_num < 0) {
		pwk_err("failed to register device: error %d\n", pwk_major_num);
		retval = pwk_major_num;
		goto failed_chrdevreg;
  	}
 
  	pwk_class = class_create(THIS_MODULE, PWK_CLASS_NAME);// create a struct class structure
		//  #define THIS_MODULE (&__this_module)
		/* This is a #define to keep the compiler from merging different
 * instances of the __key variable */
  	if (IS_ERR(pwk_class)) {
		pwk_err("failed to register device class '%s'\n", PWK_CLASS_NAME);
    	retval = PTR_ERR(pwk_class);
   		goto failed_classreg;
  	}
 
	pwk_class->devnode = pwk_devnode;
	//devnode Callback to provide the devtmpfs
	//devtmpfs 是一个由内核填充的具有自动化设备节点的文件系统。
  	pwk_device = device_create(pwk_class, NULL, MKDEV(pwk_major_num, 0),
                              NULL, PWK_DEVICE_NAME);
//creates a device and registers it with sysfs
// struct device * device_create(struct class * class, struct device * parent, dev_t devt, void * drvdata, const char * fmt, ...);
// class
// pointer to the struct class that this device should be registered to
// parent
// pointer to the parent struct device of this new device, if any
// devt
// the dev_t for the char device to be added
// drvdata
// the data to be added to the device for callbacks
// fmt
// string for the device's name
//dev_t makedev(unsigned int maj, unsigned int min);
// 给定主要和次要设备 ID，makedev() 将它们组合产生一个设备 ID，作为函数结果返回。

  	if (IS_ERR(pwk_device)) {
 		pwk_err("failed to create device '%s'\n", PWK_DEVICE_NAME);
   		retval = PTR_ERR(pwk_device);
   		goto failed_devreg;
  	}
  
  	pwk_info("pwk device registered using major %d.\n", pwk_major_num);
  
  	return 0;
        
 failed_devreg:
        class_unregister(pwk_class);
        class_destroy(pwk_class);
 failed_classreg:
        unregister_chrdev(pwk_major_num, PWK_DEVICE_NAME);
 failed_chrdevreg:
        return -1;
}

static void __exit pwk_exit(void)
{
	device_destroy(pwk_class, MKDEV(pwk_major_num, 0));
	class_unregister(pwk_class);
	class_destroy(pwk_class);
	unregister_chrdev(pwk_major_num, PWK_DEVICE_NAME);
	
	pwk_info("Unloading pwk module.\n");

	return;
}

module_init(pwk_init);
module_exit(pwk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("process watch character device module");


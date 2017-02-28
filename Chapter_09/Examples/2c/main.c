// Lab 9 - based on Example 7.3	:  rdtscll + mdelay
//		:  main.c

//  Simple standalone program ... inw + jiffies + rdtscl() + mdelay()

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>

#include <linux/sched.h>	// jiffies

#include <asm/msr.h>		// machine-specific registers;rdtsc()
#include <linux/delay.h>	// mdelay();udelay();

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,0,1)
#include <linux/vmalloc.h>
#endif

MODULE_LICENSE("GPL");

#define HELLO "hello"
#define MYDEV "MYDEV"
#define PROC_hello_LEN 8

#define CPU1MHZ (1024*1024) 

struct proc_hello_data {
	char proc_hello_name[PROC_hello_LEN + 1];
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
	char *proc_hello_value;
#else
	char proc_hello_value[132];
#endif
	char proc_hello_flag;
};

static struct proc_hello_data hello_data;

static struct proc_dir_entry *proc_hello;
static struct proc_dir_entry *proc_mydev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int eof[1];

static ssize_t read_hello(struct file *file, char *buf,
size_t len, loff_t *ppos)
#else
static int read_hello (char *buf, char **start, off_t offset,
                int len, int *eof, void *unused)
#endif

{
	struct proc_hello_data *usrsp=&hello_data;
	int buflen=0;
	unsigned long long elapsedll; 
	unsigned long long mhzll;
	volatile unsigned long long startll, endll;

	startll=inw(0x70);	// units is 1/MHz .. uSec

  if (*eof!=0) { *eof=0; return 0; }
	else if (usrsp->proc_hello_flag) {
		usrsp->proc_hello_flag=0;
		buflen=sprintf(buf, "Hello .. I got \"%s\" \"%lld\" \n", 
			usrsp->proc_hello_value, startll); 
	}
  else
  {
    elapsedll=endll-startll;
    // mhzll=elapsedll/MYCPUMHZ;
    buflen=sprintf(buf,"Hello:PID=%d\njiffies=%ld\nst=%lld,end=%lld\ndiff=%lld .. (%d MHZ), True cpu_khz=%d\n",
			(int)current->pid,jiffies,startll,endll,elapsedll,
			(int)elapsedll/(1024*1024),cpu_khz);
  }
//

	*eof = 1;
	return buflen;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static ssize_t write_hello(struct file *file, const char __user *buf,
  size_t count, loff_t *ppos)
#else
static int write_hello (struct file *file,const char * buf,
                unsigned long count, void *data)
#endif
{
	int length=count;
	struct proc_hello_data *usrsp=&hello_data;

	length = (length<PROC_hello_LEN)? length:PROC_hello_LEN;

	if (copy_from_user(usrsp->proc_hello_value, buf, length)) 
		return -EFAULT;

	usrsp->proc_hello_value[length-1]=0;
	usrsp->proc_hello_flag=1;
	return(length);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static const struct file_operations proc_fops =
{
 .owner = THIS_MODULE,
 .read  = read_hello,
 .write = write_hello
};
#endif

static int my_init (void) 
{

  proc_mydev = proc_mkdir(MYDEV,0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
  proc_hello = proc_create(HELLO, 0777, proc_mydev, &proc_fops);
#else
  proc_hello = create_proc_entry(HELLO,0,proc_mydev);
  proc_hello->read_proc = read_hello;
  proc_hello->write_proc = write_hello;
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
	proc_hello->owner = THIS_MODULE;
#endif

	hello_data.proc_hello_flag=0;

	// module init message
	printk(KERN_INFO "lab7: main initialized!\n");
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
	hello_data.proc_hello_value=vmalloc(132);
#endif
	
	return 0;
}

static void my_exit (void) {

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
	vfree(hello_data.proc_hello_value);
#endif

  if (proc_hello)
    remove_proc_entry (HELLO, proc_mydev);
  if (proc_mydev)
    remove_proc_entry (MYDEV, 0);

	if (proc_hello)
		remove_proc_entry ("hello", 0);

  // module exit message
  printk(KERN_INFO "lab7: main destroyed!\n");
}
module_init (my_init);
module_exit (my_exit);

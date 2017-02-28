// Example# 5.2 .. simple *completion* example (main.c)
//   main.c

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/slab.h>  // for kmalloc()

#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/sem.h>

#include <linux/sched.h>

#include <linux/completion.h>
  
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,0,1)
#include <linux/vmalloc.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
#include <generated/utsrelease.h>
#define init_MUTEX(a) sema_init(a,1)
#else
#include <linux/utsrelease.h>
#endif

MODULE_LICENSE("GPL");

#define PROC_HELLO_LEN 8
#define HELLO "hello"
#define MYDEV "MYDEV"

struct proc_hello_data {
	char proc_hello_name[PROC_HELLO_LEN + 1];
	char proc_hello_value[132];
	char proc_hello_flag;
	
	struct semaphore *proc_hello_sem;
	struct completion *proc_hello_completion;
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

	if (*eof!=0) { *eof=0; return 0; }

  printk(KERN_ALERT "2470:5.2: process %i (%s) about to sleep\n",
		current->pid, current->comm);

	wait_for_completion(hello_data.proc_hello_completion);

  printk(KERN_ALERT "2470:5.2: process %i (%s) is now awake\n",
		current->pid, current->comm);

	if (usrsp->proc_hello_flag) {
		usrsp->proc_hello_flag=0;
		snprintf(buf,len,"Hello .. I got \"%s\"\n",usrsp->proc_hello_value); 
	}
	else
		snprintf(buf,len,"Hello from process %d\n", (int)current->pid);

	*eof = 1;
	return(strlen(buf));
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
	int err=0;
	struct proc_hello_data *usrsp=&hello_data;

	length = (length<PROC_HELLO_LEN)? length:PROC_HELLO_LEN;

  // semaphore decr
  if (down_interruptible(hello_data.proc_hello_sem))
  	return -ERESTARTSYS;

	err=copy_from_user(usrsp->proc_hello_value, buf, length); 

  // semaphore incr
  up(hello_data.proc_hello_sem);

	// signal completion
	complete(hello_data.proc_hello_completion);

	// check for copy_from_user error here (immediately upon sem release)
	if (err)  
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

static int my_init (void) {
	proc_mydev = proc_mkdir(MYDEV,0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
  proc_hello = proc_create("hello", 0777, proc_mydev, &proc_fops);
#else
	proc_hello = create_proc_entry(HELLO,0,proc_mydev);
	proc_hello->read_proc = read_hello;
	proc_hello->write_proc = write_hello;
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
	proc_hello->owner = THIS_MODULE;
#endif

	hello_data.proc_hello_sem=(struct semaphore *) 
		kmalloc(sizeof(struct semaphore),GFP_KERNEL);
	hello_data.proc_hello_completion=(struct completion *) 
		kmalloc(sizeof(struct completion),GFP_KERNEL);

	init_MUTEX(hello_data.proc_hello_sem);
	init_completion(hello_data.proc_hello_completion);

	hello_data.proc_hello_flag=0;

        // module init message
        printk(KERN_ALERT "2470:5.2: main initialized!\n");
	return 0;
}

static void my_exit (void) {

	kfree(hello_data.proc_hello_sem);
	kfree(hello_data.proc_hello_completion);

	if (proc_hello)
		remove_proc_entry (HELLO, proc_mydev);
	if (proc_mydev)
		remove_proc_entry (MYDEV, 0);

        // module exit message
        printk(KERN_ALERT "2470:5.2: main destroyed!\n");
}

module_init (my_init);
module_exit (my_exit);

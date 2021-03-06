// Example# 6.4b:  Simple Char Driver 
//		with Static or Dynamic Major# as requested at run-time.

//		create a directory entry in /proc
//		  create a read-only proc entry for this driver.
//		  create multiple devices .. same major#, many minor#
//		      .. uses simplified access to device fields (2.6)
//		works only in 2.6 (.. a la Example# 3.2)

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>		// 2.6
#include <asm/uaccess.h>

#include <linux/moduleparam.h>	  // for module_param

#include <linux/proc_fs.h>	      // for proc entry.
#include <linux/sched.h>		// for current->pid
#include <linux/slab.h>    // for kmalloc

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#include <linux/vmalloc.h>
#endif

MODULE_LICENSE("GPL");   /*  Kernel isn't tainted .. but doesn't 
			     it doesn't matter for SUSE anyways :-( 
			*/

#define CDD		"CDD2"
#define myCDD  		"myCDD"

#define CDDMAJOR  	32
#define CDDMINOR  	0		// 2.6
#define CDDNUMDEVS  	6		// 2.6

#define CDD_PROCLEN     32

// static unsigned int counter = 0;

static unsigned int CDDmajor = CDDMAJOR;

static unsigned int CDDparm = CDDMAJOR;
module_param(CDDparm,int,0);

// static struct cdev cdev;

struct CDD_dev{
	char 	CDD_name[CDD_PROCLEN + 1];
	char 	CDD_value[132];
	int	counter;	
	struct semaphore CDD_sem;
	dev_t	CDD_devno;
	struct cdev CDD_cdev;
};

static struct CDD_dev CDD_devdata[CDDNUMDEVS];

static struct proc_dir_entry *proc_myCDD;
static struct proc_dir_entry *proc_dirCDD;

struct CDD_proc {
	char CDD_procname[CDD_PROCLEN + 1];
	char CDD_procvalue[132];
	char CDD_procflag;
};

static struct CDD_proc CDD_procdata;

static int CDD_open (struct inode *inode, struct file *file)
{
	// MOD_INC_USE_COUNT;

	struct CDD_dev *thisCDD=
		container_of(inode->i_cdev, struct CDD_dev, CDD_cdev);
	file->private_data=thisCDD;

	return 0;
}

static int CDD_release (struct inode *inode, struct file *file)
{
	// MOD_DEC_USE_COUNT;
	return 0;
}

static ssize_t CDD_read (struct file *file, char *buf, 
size_t count, loff_t *ppos)
{
	int len, err;

	// unsigned int CDDmajor=imajor(file->f_dentry->d_inode);
	// unsigned int CDDminor=iminor(file->f_dentry->d_inode);

	struct CDD_dev *thisCDD=file->private_data;
	char *string=thisCDD->CDD_value;

	if( thisCDD->counter <= 0 ) return 0;

	err = copy_to_user(buf,string,thisCDD->counter);

	if (err != 0) return -EFAULT;

	len = thisCDD->counter;
	thisCDD->counter = 0;
	return len;
}

static ssize_t CDD_write (struct file *file, const char *buf, 
size_t count, loff_t *ppos)
{
	int err;

	// unsigned int CDDmajor=imajor(file->f_dentry->d_inode);
	// unsigned int CDDminor=iminor(file->f_dentry->d_inode);

	struct CDD_dev *thisCDD=file->private_data;
	char *string=(thisCDD->CDD_value) + thisCDD->counter + (long) *ppos;

	err = copy_from_user(string,buf,count);
	if (err != 0) return -EFAULT;

	thisCDD->counter += count;
	return count;
}

static struct file_operations CDD_fops =
{
	// for LINUX_VERSION_CODE 2.4.0 and later 
	owner:	THIS_MODULE, 	// struct module *owner
	open:	CDD_open, 	// open method 
	read:   CDD_read,	// read method 
	write:  CDD_write, 	// write method 
	release:  CDD_release 	// release method
};


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int eof[1];

static ssize_t read_hello(struct file *file, char *buf,
size_t len, loff_t *ppos)
{
  off_t offset=*ppos;
#else
static int read_hello (char *buf, char **start, off_t offset,
    int len, int *eof, void *unused)
{
#endif
  struct CDD_proc *usrsp=&CDD_procdata;
  int n=0;

  if (*eof!=0) { *eof=0; return 0; }

  if (offset) { n=0; }
  else if (usrsp->CDD_procflag) {
     usrsp->CDD_procflag=0;
     n=sprintf(buf, "Hello..I got \"%s\"\n",usrsp->CDD_procvalue);
  }
  else
     n=sprintf(buf, "Hello from process %d\n", (int)current->pid);

    *eof = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
    if(n)
      *ppos=len;
#endif
    return n;
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
	struct CDD_proc *usrsp=&CDD_procdata;

	length = (length<CDD_PROCLEN)? length:CDD_PROCLEN;

	if (copy_from_user(usrsp->CDD_procvalue, buf, length))
		return -EFAULT;

	usrsp->CDD_procvalue[length-1]=0;
	usrsp->CDD_procflag=1;
	return(length);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static const struct file_operations proc_fops =
{
 .owner = THIS_MODULE,
 .read  = read_hello,
 .write  = write_hello,
};
#endif

static int CDD_init(void)
{
	int i, errno;
	dev_t devno;
	struct cdev *cdevp;
	struct CDD_dev *thisCDD;


	CDDmajor = CDDparm;

	if (CDDmajor) {
		//  Step 1a of 2:  create/populate device numbers
		devno = MKDEV(CDDmajor, CDDMINOR);

		//  Step 1b of 2:  request/reserve Major Number from Kernel
		i = register_chrdev_region(devno,CDDNUMDEVS,CDD);
		if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i;}
	}
	else {
		//  Step 1c of 2:  Request a Major Number Dynamically.
		i = alloc_chrdev_region(&devno, CDDMINOR, CDDNUMDEVS, CDD);
		if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i;}
		CDDmajor = MAJOR(devno);
		printk(KERN_ALERT "kernel assigned major number: %d to CDD\n", CDDmajor);
	}
		
	// initialize and allocate the devices
	for (i=0;i<CDDNUMDEVS; i++) {
		// set our references here .. 1 of 2
		thisCDD=&(CDD_devdata[i]);
		cdevp=&(thisCDD->CDD_cdev);

		// set our references here .. 2 of 2
		devno = MKDEV(CDDmajor,i);

		// start initializing our devices
		sema_init(&(thisCDD->CDD_sem),1);	
		thisCDD->CDD_devno = devno;

       		//  Step 2a of 2:  initialize cdev struct
		cdev_init(cdevp, &CDD_fops);	

       		//  Step 2b of 2:  register device with kernel
       		cdevp->owner = THIS_MODULE;
       		cdevp->ops = &CDD_fops;
		errno = cdev_add(cdevp, devno, CDDNUMDEVS);
		if (errno) { printk(KERN_ALERT "Error (%d) adding %s(%d)\n",errno,CDD,i); return errno;}

	}

	// Create the necessary proc entries
  proc_dirCDD = proc_mkdir(myCDD,0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
  proc_myCDD = proc_create("hello", 0777, proc_dirCDD, &proc_fops);
#else
  proc_myCDD = create_proc_entry(HELLO,0,proc_myCDD);
  proc_myCDD->read_proc = read_hello;
  proc_myCDD->write_proc = write_hello;
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
  proc_myCDD->owner = THIS_MODULE;
#endif

	return 0;
}

static void CDD_exit(void)
{
	int i=0;
	struct cdev *cdevp;
	struct CDD_dev *thisCDD;
	dev_t devno;

  if (proc_myCDD) remove_proc_entry ("hello", proc_dirCDD);
  if (proc_dirCDD) remove_proc_entry (myCDD, 0); 

	// initialize and allocate the devices
	for (i=0;i<CDDNUMDEVS; i++) {
		// set our references here .. 1 of 2
		thisCDD=&(CDD_devdata[i]);
		cdevp=&(thisCDD->CDD_cdev);

		//  Step 1 of 2:  unregister device with kernel
		cdev_del(cdevp);
	}

	//  Step 2a of 2:  create/populate device numbers
	devno = MKDEV(CDDmajor, CDDMINOR);

	//  Step 2b of 2:  Release request/reserve of Major Number from Kernel
	unregister_chrdev_region(devno, CDDNUMDEVS);

	if (CDDmajor != CDDparm) 
		printk(KERN_ALERT "kernel unassigned major number: %d from CDD\n", CDDmajor);
}

module_init(CDD_init);
module_exit(CDD_exit);


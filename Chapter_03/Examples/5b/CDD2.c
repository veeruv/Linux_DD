// Example# 3.5b:  Simple Char Driver with Dynamic Major# 
//                works only in 2.6 (.. a la Example# 3.2)

//      using dev_t, struct cdev (2.6)          (example 3.1b)
//      using myCDD structure                   (example 3.2b)
//      using file->private_data structure      (example 3.3b)
//      using container_of() macro              (example 3.4b)
//	using Dynamic Major#			(here)

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>		// 2.6
#include <asm/uaccess.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,0,1)
#include <linux/vmalloc.h>
#endif

MODULE_LICENSE("GPL");   /*  Kernel isn't tainted .. but doesn't 
			     it doesn't matter for SUSE anyways :-( */

#define CDD		"CDD2"
#define CDDMAJOR  	0
// #define CDDMAJOR  	32
#define CDDMINOR  	0	// 2.6
#define CDDNUMDEVS  	1	// 2.6

// static unsigned int counter = 0;
// static char string[132];
// static struct cdev cdev;

static unsigned int CDDmajor = CDDMAJOR;

dev_t   firstdevno;
struct CDDdev_struct {
        unsigned int 	counter;
        char 		*CDD_storage;
        struct cdev 	cdev;
};

static struct CDDdev_struct myCDD;

static int CDD_open (struct inode *inode, struct file *file)
{
  struct CDDdev_struct *thisCDD=
         container_of(inode->i_cdev, struct CDDdev_struct, cdev);

  file->private_data=thisCDD;

	return 0;
}

static int CDD_release (struct inode *inode, struct file *file)
{
	struct CDDdev_struct *thisCDD=file->private_data;

	if( thisCDD->counter <= 0 ) return 0;   // for compiler warning!

	// MOD_DEC_USE_COUNT;
	return 0;
}

static ssize_t CDD_read (struct file *file, char *buf, 
size_t count, loff_t *ppos)
{
	int len, err;
	struct CDDdev_struct *thisCDD=file->private_data;

	if( thisCDD->counter <= 0 ) return 0;

	err = copy_to_user(buf,thisCDD->CDD_storage,thisCDD->counter);

	if (err != 0) return -EFAULT;

	len = thisCDD->counter;
	thisCDD->counter = 0;
	return len;
}

static ssize_t CDD_write (struct file *file, const char *buf, 
size_t count, loff_t *ppos)
{
	int err;
	struct CDDdev_struct *thisCDD=file->private_data;

	err = copy_from_user(thisCDD->CDD_storage,buf,count);
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

static int CDD_init(void)
{
	int i;
	struct CDDdev_struct *thisCDD=&myCDD;

  thisCDD->CDD_storage=vmalloc(4096);

	if (CDDmajor) 
	{
		//  Step 1a of 2:  create/populate device numbers
   	firstdevno = MKDEV(CDDmajor, CDDMINOR);

   	//  Step 1b of 2:  request/reserve Major Number from Kernel
    i = register_chrdev_region(firstdevno,CDDNUMDEVS,CDD);
    if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i;}
	}
	else 
	{
		//  Step 1c of 2:  Request a Major Number Dynamically.
		i = alloc_chrdev_region(&firstdevno, CDDMINOR, CDDNUMDEVS, CDD);
		if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i;}
		CDDmajor = MAJOR(firstdevno);
		printk(KERN_ALERT "kernel assigned major number: %d to CDD\n", CDDmajor);
	}
		

		//  Step 2a of 2:  initialize thisCDD->cdev struct
		cdev_init(&(thisCDD->cdev), &CDD_fops);

		//  Step 2b of 2:  register device with kernel
		thisCDD->cdev.owner = THIS_MODULE;
		thisCDD->cdev.ops = &CDD_fops;
		i = cdev_add(&(thisCDD->cdev), firstdevno, CDDNUMDEVS);
		if (i) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i; }

		return 0;
}

static void CDD_exit(void)
{
	struct CDDdev_struct *thisCDD=&myCDD;

  vfree(thisCDD->CDD_storage);

	//  Step 1 of 2:  unregister device with kernel
	cdev_del(&(thisCDD->cdev));

	//  Step 2b of 2:  Release request/reserve of Major Number from Kernel
	unregister_chrdev_region(firstdevno, CDDNUMDEVS);

	if (CDDmajor != CDDMAJOR) printk(KERN_ALERT "kernel unassigned major number: %d from CDD\n", CDDmajor);
}

module_init(CDD_init);
module_exit(CDD_exit);

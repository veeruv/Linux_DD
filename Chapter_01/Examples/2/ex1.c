  
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,0,1)
#include <linux/vmalloc.h>
#endif
  
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
	file->f_path.dentry->d_name.name);
#else
	file->f_dentry->d_name.name);
#endif
  
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
	unsigned int CDDminor=iminor(file->f_path.dentry->d_inode);
#else
	unsigned int CDDminor=iminor(file->f_dentry->d_inode);
#endif
  

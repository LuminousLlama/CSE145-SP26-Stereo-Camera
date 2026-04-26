/***************************************************************
模块名	    	 	：
文件名	     		：universalEthernet.c
相关文件			：
文件实现功能		：
作者		 		：28692
版本		 		：V1.0
-----------------------------------------------------------------
修改记录:
日  期		    版本		修改人		修改内容
2015/07/15		1.0         28692    	初始版本
*****************************************************************/

#include "universalEthernet.h"

#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/cdev.h>
#include <linux/version.h> // KERNEL_VERSION
#include <linux/fs.h>

unsigned int trace_param = 0;

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0) 
extern struct net init_net;
extern int nf_register_net_hook(struct net *net, const struct nf_hook_ops *reg);
extern void nf_unregister_net_hook(struct net *net, const struct nf_hook_ops *reg);
#endif

static struct nf_hook_ops	cameraHook;

static unsigned int deviceMajor = 0; 
static dev_t 		deviceNo;

module_param(deviceMajor, int, S_IRUGO);
module_param(trace_param, uint, S_IRUGO | S_IWUSR);

//////////////////////////////////////////////////////////////////////////////////////////////////
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)

unsigned int hookEtherPacket(uint32_t hooknum,
									struct sk_buff **skb,
									const struct net_device *in, 
									const struct net_device *out,  
									int32_t (*okfn)(struct sk_buff *)) 
									
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)    

unsigned int hookEtherPacket(uint32_t hooknum, 
								struct sk_buff *skb,  
                       			const struct net_device *in, 
								const struct net_device *out,  
								int32_t (*okfn)(struct sk_buff *)) 

#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)

unsigned int hookEtherPacket(const struct nf_hook_ops *aOperations, 
								struct sk_buff *skb,  
                       			const struct net_device *in, 
								const struct net_device *out,  
								int32_t (*okfn)(struct sk_buff *)) 
#else

unsigned int hookEtherPacket(void *priv,
								struct sk_buff *skb,
								const struct nf_hook_state *state)
#endif
{  
	int ret = 0;
	RAY_sk_buff_t * skbuf;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)   
   	skbuf = (RAY_sk_buff_t *)(*skb);
#else
	skbuf = (RAY_sk_buff_t *)skb;
#endif
	
	ret = CameraDev_Process_Packet( skbuf );

	return ( ret == 0 ) ? NF_ACCEPT : NF_DROP;
}  

////////////////////////////////////////////////////////////////////////////////////////////////////
static int camera_open(struct inode *inode, struct file *filp)  
{  
	void * ptStreamItem = NULL;

	if ((NULL == inode) || (NULL == filp))
	{		
		LOG_PRINTF("Memery Malloc failed.");
		return -1;	
	}	

	ptStreamItem = File_OperateOpen();

	if( ptStreamItem == NULL )
	{
		LOG_PRINTF("File_OperateOpen failed.");
		return -1;	
	}

	filp->private_data = ptStreamItem;

	LOG_PRINTF("open file, inode %p, filp %p, priv %p", inode, filp, filp->private_data);
	return 0;  
}  


static int camera_close(struct inode *inode, struct file *filp)  
{ 
	void 				*ptStreamItem = NULL;
	int		 			ret = -1;

	ptStreamItem = filp->private_data;

	ret = File_OperateClose(ptStreamItem);
	if(ret < 0)
	{
		LOG_PRINTF("File close failed.");
		return -1;
	}

	LOG_PRINTF("close file, ret %d, inode %p, filp %p, priv %p", ret, inode, filp, filp->private_data);

	return 0;  
}  

static int32_t camera_mmap(struct file *filp, struct vm_area_struct *vma)
{
	void 				*ptStreamItem = NULL;
	int32_t 			ret = -1;
	
	ptStreamItem = filp->private_data;
	
	ret = File_OperateMmap(ptStreamItem, (void *)vma);
	
	return ret;
}


static unsigned int camera_poll(struct file *filp, struct poll_table_struct *wait)
{  
	void * ptStreamItem = NULL;
	uint32_t 			mask = 0;

	ptStreamItem = filp->private_data;
	
	mask = File_OperatePoll(ptStreamItem, filp, (void *)wait);	
	
	return mask;  
}

static ssize_t camera_read(struct file *filp, char __user *buf, size_t size, loff_t *fpos)
{  
	void				*ptStreamItem = NULL;
	long				length = 0;	
	
	ptStreamItem = filp->private_data;

	length = File_OperateRead(ptStreamItem, buf, (long)size);	
	
	return length;
}


static long camera_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)  
{ 
	void		*ptStreamItem = NULL;	
	long		ret = -1;

	ptStreamItem = filp->private_data;

	ret = File_OperateIoCtrl(ptStreamItem, cmd, (void *)arg);

	return ret;	
}

const struct file_operations camera_fops = 
{  
	.owner 			= THIS_MODULE,      
	.open 			= camera_open,	
	.release 		= camera_close,
	.poll 			= camera_poll,
	.mmap 			= camera_mmap,	
	.read			= camera_read,
	.unlocked_ioctl = camera_ioctl,
};  


static int32_t __init camera_init(void)  
{	
	struct cdev * rcdev = NULL;
	int ret = -1;

	if (deviceMajor > 0)
	{
		deviceNo = MKDEV(deviceMajor, 0);
		ret = register_chrdev_region(deviceNo, 1, DEVICE_NAME);
		if (ret < 0) 
		{		
			LOG_PRINTF("Register chrdev region failed.");
			return ret;
		}
	}
	else
	{
		ret = alloc_chrdev_region(&deviceNo, 0, 1, DEVICE_NAME);
		if (ret < 0) 
		{		
			LOG_PRINTF("Alloc chrdev region failed.");
			return ret;
		}
		deviceMajor = MAJOR(deviceNo);		
	}

	CameraDev_Init();
	
	rcdev = (struct cdev *)CameraDev_Get_cdev();
	cdev_init(rcdev, &camera_fops);
	rcdev->owner = THIS_MODULE;
		
	ret = cdev_add(rcdev, deviceNo, 1);
	if (ret < 0) 
	{			
		LOG_PRINTF("Device add cdev failed.");
		unregister_chrdev_region(deviceNo, 1);
		return ret;
	}

	cameraHook.hook  = (void *)hookEtherPacket;  
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
	cameraHook.owner = NULL;  
#endif
	cameraHook.pf = PF_INET;				
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
	cameraHook.hooknum = NF_IP_PRE_ROUTING;
#else
	cameraHook.hooknum = NF_INET_PRE_ROUTING; 
#endif    			
	cameraHook.priority = NF_IP_PRI_FIRST; 
   
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 10, 0)    
	nf_register_hook(&cameraHook); 
#else
	nf_register_net_hook(&init_net, &cameraHook); 
#endif

	LOG_PRINTF("[ universalEthernet driver loaded successfully. ], deviceMajor %d", deviceMajor);
	return 0;
}


static void __exit camera_exit(void)
{
	struct cdev * rcdev = NULL;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 10, 0)    
	nf_unregister_hook(&cameraHook);
#else
	nf_unregister_net_hook(&init_net, &cameraHook);
#endif
	rcdev = (struct cdev *)CameraDev_Get_cdev();
	cdev_del(rcdev);
	CameraDev_Fini();

	unregister_chrdev_region(deviceNo, 1);	
	
	LOG_PRINTF("[ universalEthernet driver exit successfully. ], deviceMajor %d", deviceMajor);
}


module_init(camera_init);  
module_exit(camera_exit);  


MODULE_AUTHOR(FILTER_COMPANY);
MODULE_LICENSE(FILTER_GPL);
MODULE_DESCRIPTION(FILTER_MODULE_NAME);
MODULE_VERSION(FILTER_VERSION);

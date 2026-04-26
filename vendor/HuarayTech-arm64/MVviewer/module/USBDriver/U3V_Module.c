#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>
#include <linux/mm.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>

#include "U3V_Module.h"

unsigned int u3v_no_drop_param = 0;
unsigned int u3v_trace_param = U3V_TRACE_INFO | U3V_TRACE_ERROR;

/* ------------------------------------------------------------------------
 * V4L2 file operations
 */

static int U3V_v4l2_open(struct file *file)
{
	void* dev = NULL;
	int ret = 0;
	dev = video_drvdata(file);

	if(dev == NULL)
	{
		LOG_PRINTF("[Info] device has already been disconnected before!\n");
		return -ENODEV;
	}

	ret = TL_U3v_v4l2_open(dev, file);
	if(ret < 0)
	{
		LOG_PRINTF("[Info] v4l2 open failed.\n");
		return ret;
	}
	
	LOG_PRINTF("[Info] v4l2 open success.\n");
	return ret;
}

static int U3V_v4l2_release(struct file *file)
{
	void* handle = file->private_data;

	if (NULL == video_drvdata(file))
	{
		LOG_PRINTF("[Info] device has already been disconnected before!\n");

		TL_U3v_moduleResources_release(handle);
		kfree(handle);
		file->private_data = NULL;
		return -ENODEV;	
	}	
	
	TL_U3v_v4l2_release(handle, file);
	LOG_PRINTF("[Info] v4l2 close success.\n");
	
	return 0;
}

static long U3V_v4l2_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return TL_U3v_v4l2_do_ioctl((void*)file->private_data, file->f_flags & O_NONBLOCK, cmd, (void*)arg);
}

static ssize_t U3V_v4l2_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

static int U3V_v4l2_mmap(struct file *file, struct vm_area_struct *vma)
{
	void* handle = file->private_data;
	return TL_U3v_v4l2_mmap(handle, (void*)vma);
}

static unsigned int U3V_v4l2_poll(struct file *file, poll_table *wait)
{
	void* handle = file->private_data;
	return TL_U3v_v4l2_poll(handle, file, wait);
}

const struct v4l2_file_operations U3V_fops = {
	.owner		= THIS_MODULE,
	.open		= U3V_v4l2_open,
	.release	= U3V_v4l2_release,
	.unlocked_ioctl	= U3V_v4l2_ioctl,
	.read		= U3V_v4l2_read,
	.mmap		= U3V_v4l2_mmap,
	.poll		= U3V_v4l2_poll,
};

static void U3V_release(struct video_device *vdev)
{
	video_device_release(vdev);
}

static int U3V_ModuleProbe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct v4l2_device* v4l2Dev = NULL;
	struct video_device* videoDev = NULL;
	struct usb_device* udev = interface_to_usbdev(intf);
	void* u3vDev = NULL;
	int ret = 0;

	ret = TL_ModuleProbe((void*)intf, (void**)&u3vDev, (void**)&v4l2Dev);
	if(ret != 0)
	{
		LOG_PRINTF("[Err] U3V_ModuleProbe: Failed to load u3v device.\n");
		return ret;
	}

	/* Register the device with V4L. */
	videoDev = video_device_alloc();
	if (videoDev == NULL) {
		LOG_PRINTF("[Err] U3V_ModuleProbe: Failed to allocate video device.\n");
		return -ENOMEM;
	}

	videoDev->v4l2_dev = (struct v4l2_device*)v4l2Dev;
	videoDev->fops = &U3V_fops;
	videoDev->release = U3V_release;
	if(udev->product != NULL)
	{
		strlcpy(videoDev->name, udev->product, sizeof(videoDev->name));
	}

	if(TL_U3v_register_video(u3vDev, (void*)videoDev) < 0)
	{
		goto error;
	}

	/* Save our data pointer in the interface data. */
	usb_set_intfdata(intf, u3vDev);

	LOG_PRINTF("Found device %s (%04x:%04x) busnum(%d) devnum(%d) portnum(%d)\n",
		udev->product ? udev->product : "<unnamed>",
		le16_to_cpu(udev->descriptor.idVendor),
		le16_to_cpu(udev->descriptor.idProduct),
		udev->bus->busnum,
		udev->devnum,
		udev->portnum);

	LOG_PRINTF("[Info] Success to load u3vDriver for the u3v device.(%d)\n", u3v_trace_param);
	return 0;

error:
	TL_U3v_unregister_video(u3vDev);
	return -ENODEV;
}

static void U3V_ModuleDisconnect(struct usb_interface *intf)
{
	void *u3vDev = usb_get_intfdata(intf);

	/* Set the USB interface data to NULL. This can be done outside the
	 * lock, as there's no other reader.
	 */
	usb_set_intfdata(intf, NULL);
	TL_ModuleDisconnect(u3vDev);

	LOG_PRINTF("[Info] Success to unload u3vDriver.\n");
}

static int U3V_ModuleSuspend(struct usb_interface *intf, pm_message_t message)
{
	void* u3vDev = usb_get_intfdata(intf);
	
	return TL_U3v_video_suspend(u3vDev);
}

static int __u3v_resume(struct usb_interface *intf, int reset)
{
	void* u3vDev = usb_get_intfdata(intf);
	
	return TL_U3v_video_resume(u3vDev);
}

static int U3V_ModuleResume(struct usb_interface *intf)
{
	return __u3v_resume(intf, 0);
}

static int U3V_ModuleResetResume(struct usb_interface *intf)
{
	return __u3v_resume(intf, 1);
}

module_param_named(nodrop, u3v_no_drop_param, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(nodrop, "Don't drop incomplete frames");
module_param_named(trace, u3v_trace_param, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(trace, "Trace level bitmask");

static struct usb_device_id u3v_ids[] = {
	
	/* USB3 Vision Class */
	{ USB_INTERFACE_INFO(USB_CLASS_MISC, 0x05, 0x00) },
	{}
};

MODULE_DEVICE_TABLE(usb, u3v_ids);

struct usb_driver u3v_driver = {
	.name		= "u3vvideo",
	.probe		= U3V_ModuleProbe,
	.disconnect	= U3V_ModuleDisconnect,
	.suspend	= U3V_ModuleSuspend,
	.resume		= U3V_ModuleResume,
	.reset_resume	= U3V_ModuleResetResume,
	.id_table	= u3v_ids,
	.supports_autosuspend = 1,
};

static int __init U3V_ModuleInit(void)
{
	int result;

	result = usb_register(&u3v_driver);
	if (result == 0)
		printk(KERN_INFO DRIVER_DESC " (" DRIVER_VERSION ")\n");
	return result;
}

static void __exit U3V_ModuleCleanup(void)
{
	usb_deregister(&u3v_driver);
}

module_init(U3V_ModuleInit);
module_exit(U3V_ModuleCleanup);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);



#ifndef _U3V_KERNEL_H_
#define _U3V_KERNEL_H_

#include "RayDriverApi/RayUsb.h"

//log
#define filename(x) RAY_strrchr(x,'/') ? RAY_strrchr(x,'/') + 1 : x
#define LOG_PRINTF( msg, ... ) \
	RAY_printk( KERN_ERR "[%s(%d) %s] " msg, filename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__ );

//Driver specific constants.
#define DRIVER_VERSION_NUMBER	KERNEL_VERSION(1, 0, 0)
#define DRIVER_VERSION		"2.1.0"
#define DRIVER_AUTHOR		"Machine Vision"
#define DRIVER_DESC			"Driver For USB3 Vision Devices"

//v4l2
extern int TL_U3v_v4l2_open(void *u3vDev, void *file);
extern void TL_U3v_v4l2_release(void *v4l2Data, void *file);
extern void TL_U3v_moduleResources_release(void *v4l2Data);
extern long TL_U3v_v4l2_do_ioctl(void *v4l2Data, unsigned int flag, unsigned int cmd, void *arg);
extern int TL_U3v_v4l2_mmap(void *v4l2Data, void* vma);
extern unsigned int TL_U3v_v4l2_poll(void *v4l2Data, void *file, void* wait);

//u3vDriver
extern int TL_ModuleProbe(void* intf, void** u3vDev, void** v4l2Dev);
extern int TL_U3v_register_video(void* u3vDev, void* videoDev);
extern void TL_U3v_unregister_video(void* dev);
extern void TL_ModuleDisconnect(void *dev);
extern int TL_U3v_video_suspend(void* dev);
extern int TL_U3v_video_resume(void* dev);

#endif /* _U3V_KERNEL_H_ */

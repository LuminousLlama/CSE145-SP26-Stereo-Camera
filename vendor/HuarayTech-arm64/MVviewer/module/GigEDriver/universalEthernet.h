/***************************************************************
模块名	    	 	：
文件名	     		：universalEthernet.h
相关文件			：
文件实现功能		：
作者		 		：28692
版本		 		：V1.0
-----------------------------------------------------------------
修改记录:
日  期		    版本		修改人		修改内容
2015/07/15		1.0         28692    	初始版本
*****************************************************************/
#ifndef _UNIVERSALETHERNET__H_
#define _UNIVERSALETHERNET__H_

#include "RayDriverApi/RayApi.h"
#include "RayDriverApi/RayFile.h"
#include "RayDriverApi/RayNet.h"

//////////////////////////////////////////////////////////////////////////////

#define __FILENAME__   RAY_strrchr(__FILE__, '/') ? (RAY_strrchr(__FILE__, '/') + 1) : __FILE__

#define LOG_PRINTF( fmt, ... ) \
	RAY_printk( KERN_ERR "[%s:%d:%s] " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__ )

/* char device node info */
#define DEVICE_NAME        		"universalEthernet"

// Automatically generated resources file
#define FILTER_MODULE_NAME 		"Driver For GigE Vision Devices"
#define FILTER_COMPANY          "Machine Vision"
#define FILTER_GPL				"GPL"

#define FILTER_VERSION          "2.1.1"

// device func
extern int CameraDev_Init(void);
extern int CameraDev_Fini(void);
extern RAY_cdev_t * CameraDev_Get_cdev(void);
extern int CameraDev_Process_Packet( RAY_sk_buff_t * skb );

void * File_OperateOpen(void);
int File_OperateClose(void * ptStreamInfo);
long File_OperateIoCtrl(void *ptStreamItem, unsigned int cmd, void *argp);  
long File_OperateRead(void * ptStreamInfo, char * argp, long size );
int File_OperateMmap(void * ptStreamInfo, void * argp);
unsigned int File_OperatePoll(void * ptStreamInfo, void * ptFile, void * argp);

#endif

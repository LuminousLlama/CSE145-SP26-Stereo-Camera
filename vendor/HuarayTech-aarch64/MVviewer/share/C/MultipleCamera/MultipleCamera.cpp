/// \file
/// \~chinese
/// \brief 多相机拉流处理示例
/// \example MultipleCamera.cpp
/// \~english
/// \brief Multiple cameras grab stream sample
/// \example MultipleCamera.cpp

//**********************************************************************
// 本Demo为简单演示SDK的使用，没有附加修改相机IP的代码，在运行之前，请使
// 用相机客户端修改相机IP地址的网段与主机的网段一致。                 
// This Demo shows how to use GenICam API(C) to write a simple program.
// Please make sure that the camera and PC are in the same subnet before running the demo.
// If not, you can use camera client software to modify the IP address of camera to the same subnet with PC. 
//**********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "IMVApi.h"

#define MAX_DEVICE_NUM	16

#define sleep(ms)	usleep(1000 * ms)

typedef struct _CameraInfo
{
	IMV_HANDLE handle;
	IMV_DeviceInfo info;
}CameraInfo;

typedef struct _MultipleCamera
{
	unsigned int	nOpenCameraNum;
	CameraInfo		cameraInfo[MAX_DEVICE_NUM];
}MultipleCamera;

// 数据帧回调函数
// Data frame callback function
static void onGetFrame(IMV_Frame* pFrame, void* pUser)
{
	CameraInfo	*pCameraInfo = (CameraInfo*)pUser;

	if (pFrame == NULL)
	{
		printf("pFrame is NULL\n");
		return;
	}

	if (pCameraInfo == NULL)
	{
		printf("pCameraInfo is NULL\n");
		return;
	}

	printf("cameraKey[%s] frame blockId = %llu\n", pCameraInfo->info.cameraKey, pFrame->frameInfo.blockId);

	return;
}

// ***********开始： 这部分处理与SDK操作相机无关，用于显示设备列表 ***********
// ***********BEGIN: These functions are not related to API call and used to display device info***********
static void displayDeviceInfo(IMV_DeviceList deviceInfoList)
{
	IMV_DeviceInfo*	pDevInfo = NULL;
	unsigned int cameraIndex = 0;
	char vendorNameCat[11];
	char cameraNameCat[16];

	// 打印Title行 
	// Print title line 
	printf("\nIdx Type Vendor     Model      S/N             DeviceUserID    IP Address    \n");
	printf("------------------------------------------------------------------------------\n");

	for (cameraIndex = 0; cameraIndex < deviceInfoList.nDevNum; cameraIndex++)
	{
		pDevInfo = &deviceInfoList.pDevInfo[cameraIndex];
		// 设备列表的相机索引  最大表示字数：3
		// Camera index in device list, display in 3 characters 
		printf("%-3d", cameraIndex + 1);

		// 相机的设备类型（GigE，U3V，CL，PCIe）
		// Camera type 
		switch (pDevInfo->nCameraType)
		{
			case typeGigeCamera:printf(" GigE");break;
			case typeU3vCamera:printf(" U3V ");break;
			case typeCLCamera:printf(" CL  ");break;
			case typePCIeCamera:printf(" PCIe");break;
			default:printf("     ");break;
		}

		// 制造商信息  最大表示字数：10 
		// Camera vendor name, display in 10 characters 
		if (strlen(pDevInfo->vendorName) > 10)
		{
			memcpy(vendorNameCat, pDevInfo->vendorName, 7);
			vendorNameCat[7] = '\0';
			strcat(vendorNameCat, "...");
			printf(" %-10.10s", vendorNameCat);
		}
		else
		{
			printf(" %-10.10s", pDevInfo->vendorName);
		}

		// 相机的型号信息 最大表示字数：10 
		// Camera model name, display in 10 characters 
		printf(" %-10.10s", pDevInfo->modelName);

		// 相机的序列号 最大表示字数：15 
		// Camera serial number, display in 15 characters 
		printf(" %-15.15s", pDevInfo->serialNumber);

		// 自定义用户ID 最大表示字数：15 
		// Camera user id, display in 15 characters 
		if (strlen(pDevInfo->cameraName) > 15)
		{
			memcpy(cameraNameCat, pDevInfo->cameraName, 12);
			cameraNameCat[12] = '\0';
			strcat(cameraNameCat, "...");
			printf(" %-15.15s", cameraNameCat);
		}
		else
		{
			printf(" %-15.15s", pDevInfo->cameraName);
		}

		// GigE相机时获取IP地址 
		// IP address of GigE camera 
		if (pDevInfo->nCameraType == typeGigeCamera)
		{
			printf(" %s", pDevInfo->DeviceSpecificInfo.gigeDeviceInfo.ipAddress);
		}

		printf("\n");
	}

	return;
}

int openCamera(MultipleCamera *pMultipleCameraInfo)
{
	unsigned int index = 0;
	int ret = IMV_OK;
	unsigned int openCameraNum = 0;

	IMV_HANDLE devHandle = NULL;

	if (!pMultipleCameraInfo)
	{
		printf("pMultipleCameraInfo is NULL!\n");
		return IMV_INVALID_PARAM;
	}

	for (index = 0; index < pMultipleCameraInfo->nOpenCameraNum; index++)
	{
		devHandle = NULL;

		// 创建设备句柄
		// Create Device Handle
		ret = IMV_CreateHandle(&devHandle, modeByIndex, (void*)&index);
		if (IMV_OK != ret)
		{
			printf("Create devHandle1 failed! index[%u], ErrorCode[%d]\n", index, ret);
			continue;
		}

		// 获取设备信息
		// Get device information
		ret = IMV_GetDeviceInfo(devHandle, &pMultipleCameraInfo->cameraInfo[openCameraNum].info);
		if (IMV_OK != ret)
		{
			printf("Get device info failed! index[%u], ErrorCode[%d]\n", index, ret);
		}

		// 打开相机 
		// Open camera 
		ret = IMV_Open(devHandle);
		if (IMV_OK != ret)
		{
			printf("Open camera1 failed! cameraKey[%s], ErrorCode[%d]\n", 
				pMultipleCameraInfo->cameraInfo[openCameraNum].info.cameraKey, ret);

			// 销毁设备句柄
			// Destroy Device Handle
			IMV_DestroyHandle(devHandle);
			continue;
		}

		pMultipleCameraInfo->cameraInfo[openCameraNum].handle = devHandle;

		openCameraNum++;
	}

	pMultipleCameraInfo->nOpenCameraNum = openCameraNum;

	return IMV_OK;
}

int startGrabbing(MultipleCamera *pMultipleCameraInfo)
{
	unsigned int index = 0;
	int ret = IMV_OK;

	if (!pMultipleCameraInfo)
	{
		printf("pMultipleCameraInfo is NULL!\n");
		return IMV_INVALID_PARAM;
	}

	for (index = 0; index < pMultipleCameraInfo->nOpenCameraNum; index++)
	{
		// 注册数据帧回调函数
		// Register data frame callback function
		ret = IMV_AttachGrabbing(pMultipleCameraInfo->cameraInfo[index].handle, onGetFrame, (void*)&pMultipleCameraInfo->cameraInfo[index]);
		if (IMV_OK != ret)
		{
			printf("Attach grabbing failed! cameraKey[%s], ErrorCode[%d]\n",
				pMultipleCameraInfo->cameraInfo[index].info.cameraKey, ret);
			continue;
		}

		// 开始拉流 
		// Start grabbing 
		ret = IMV_StartGrabbing(pMultipleCameraInfo->cameraInfo[index].handle);
		if (IMV_OK != ret)
		{
			printf("Start grabbing failed! EcameraKey[%s], ErrorCode[%d]\n",
				pMultipleCameraInfo->cameraInfo[index].info.cameraKey, ret);
		}
		
	}

	return IMV_OK;
}

int stopGrabbing(MultipleCamera *pMultipleCameraInfo)
{
	unsigned int index = 0;
	int ret = IMV_OK;

	if (!pMultipleCameraInfo)
	{
		printf("pMultipleCameraInfo is NULL!\n");
		return IMV_INVALID_PARAM;
	}

	for (index = 0; index < pMultipleCameraInfo->nOpenCameraNum; index++)
	{
		// 判断设备是否正在拉流 
		// Check whether device is grabbing or not 
		if (IMV_IsGrabbing(pMultipleCameraInfo->cameraInfo[index].handle))
		{
			// 停止拉流 
			// Stop grabbing 
			ret = IMV_StopGrabbing(pMultipleCameraInfo->cameraInfo[index].handle);
			if (IMV_OK != ret)
			{
				printf("Stop grabbing failed! EcameraKey[%s], ErrorCode[%d]\n",
					pMultipleCameraInfo->cameraInfo[index].info.cameraKey, ret);
			}

		}
	}

	return IMV_OK;
}

int closeDevice(MultipleCamera *pMultipleCameraInfo)
{
	unsigned int index = 0;
	int ret = IMV_OK;

	if (!pMultipleCameraInfo)
	{
		printf("pMultipleCameraInfo is NULL!\n");
		return IMV_INVALID_PARAM;
	}

	for (index = 0; index < pMultipleCameraInfo->nOpenCameraNum; index++)
	{
		// 判断设备是否已打开
		// Check whether device is opened or not
		if (IMV_IsOpen(pMultipleCameraInfo->cameraInfo[index].handle))
		{
			// 关闭相机
			// Close camera
			ret = IMV_Close(pMultipleCameraInfo->cameraInfo[index].handle);
			if (IMV_OK != ret)
			{
				printf("Close grabbing failed! EcameraKey[%s], ErrorCode[%d]\n",
					pMultipleCameraInfo->cameraInfo[index].info.cameraKey, ret);
			}

		}

		// 销毁设备句柄
		// Destroy Device Handle
		ret = IMV_DestroyHandle(pMultipleCameraInfo->cameraInfo[index].handle);
		if (IMV_OK != ret)
		{
			printf("Destroy device Handle failed! EcameraKey[%s], ErrorCode[%d]\n",
				pMultipleCameraInfo->cameraInfo[index].info.cameraKey, ret);
		}
	}

	return IMV_OK;
}


// ***********结束： 这部分处理与SDK操作相机无关，用于显示设备列表 ***********
// ***********END: These functions are not related to API call and used to display device info***********

int main()
{
	int ret = IMV_OK;
	MultipleCamera multipleCameraInfo;
	IMV_DeviceList deviceInfoList;

	memset(&multipleCameraInfo, 0, sizeof(multipleCameraInfo));

	// 发现设备 
	// discover camera 
	ret = IMV_EnumDevices(&deviceInfoList, interfaceTypeAll);
	if (IMV_OK != ret)
	{
		printf("Enumeration devices failed! ErrorCode[%d]\n", ret);
		getchar();
		return -1;
	}
	
	if (deviceInfoList.nDevNum < 1)
	{
		printf("no camera\n");
		getchar();
		return -1;
	}

	// 打印相机基本信息（序号,类型,制造商信息,型号,序列号,用户自定义ID,IP地址） 
	// Print camera info (Index, Type, Vendor, Model, Serial number, DeviceUserID, IP Address) 
	displayDeviceInfo(deviceInfoList);

	sleep(2000);

	multipleCameraInfo.nOpenCameraNum = deviceInfoList.nDevNum;

	do
	{
		// 打开相机 
		// Open camera 
		ret = openCamera(&multipleCameraInfo);
		if (IMV_OK != ret)
		{
			break;
		}

		if (multipleCameraInfo.nOpenCameraNum == 0)
		{
			printf("no camera is opened!\n");
			break;
		}

		// 开始拉流 
		// Start grabbing 
		ret = startGrabbing(&multipleCameraInfo);
		if (IMV_OK != ret)
		{
			break;
		}

		// 取图2秒
		// get frame 2 seconds
		sleep(2000);

		// 停止拉流 
		// Stop grabbing 
		ret = stopGrabbing(&multipleCameraInfo);
		if (IMV_OK != ret)
		{
			break;
		}

	} while (false);

	// 关闭相机
	// Close camera 
	closeDevice(&multipleCameraInfo);

	printf("Press a key to exit...\n");
	getchar();

	return 0;
}
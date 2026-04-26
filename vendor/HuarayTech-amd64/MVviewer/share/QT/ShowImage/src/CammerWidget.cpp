#include "CammerWidget.h"
#include "ui_cammerwidget.h"

#define DEFAULT_SHOW_RATE (30)
#define TIMESTAMPFREQUENCY 125000000	//大华相机的时间戳频率固定为125,000,000Hz

//回调函数
static void FrameCallback(IMV_Frame* pFrame, void* pUser)
{
	CammerWidget* pCammerWidget = (CammerWidget*)pUser;
	if (!pCammerWidget)
	{
		printf("pCammerWidget is NULL!\n");
		return;
	}

	CFrameInfo frameInfo;
	frameInfo.m_nWidth = (int)pFrame->frameInfo.width;
	frameInfo.m_nHeight = (int)pFrame->frameInfo.height;
	frameInfo.m_nBufferSize = (int)pFrame->frameInfo.size;
	frameInfo.m_nPaddingX = (int)pFrame->frameInfo.paddingX;
	frameInfo.m_nPaddingY = (int)pFrame->frameInfo.paddingY;
	frameInfo.m_ePixelType = pFrame->frameInfo.pixelFormat;
	frameInfo.m_pImageBuf = (unsigned char *)malloc(sizeof(unsigned char) * frameInfo.m_nBufferSize);
	frameInfo.m_nTimeStamp = pFrame->frameInfo.timeStamp;

	// 内存申请失败，直接返回
	// memory application failed, return directly
	if (frameInfo.m_pImageBuf != NULL)
	{
		memcpy(frameInfo.m_pImageBuf, pFrame->pData, frameInfo.m_nBufferSize);

		if (pCammerWidget->_displayFrameQueue.size() > 16)
		{
			CFrameInfo frameOld;
			if (pCammerWidget->_displayFrameQueue.get(frameOld))
			{
				free(frameOld.m_pImageBuf);
				frameOld.m_pImageBuf = NULL;
			}
		}

		pCammerWidget->_displayFrameQueue.push_back(frameInfo);
	}
}

// 显示线程
// display thread
void* displayThread(void* pUser)
{
	CammerWidget* pCammerWidget = (CammerWidget*)pUser;
	if (!pCammerWidget)
	{
		printf("pCammerWidget is NULL!\n");
		return NULL;
	}

	pCammerWidget->DisplayThreadProc();

	return NULL;
}

CammerWidget::CammerWidget(QWidget *parent) :
QWidget(parent)
, ui(new Ui::CammerWidget)
	, m_currentCameraKey("")
	, m_devHandle(NULL)
	, m_threadID(0)
	, m_isExitDisplayThread(false)
	, m_dDisplayInterval(0)
	, m_nTimestampFreq(TIMESTAMPFREQUENCY)
	, m_nFirstFrameTime(0)
	, m_nLastFrameTime(0)
	, m_handler(NULL)
{
    ui->setupUi(this);

	m_hWnd = (VR_HWND)this->winId();
	// 默认显示30帧
	setDisplayFPS(30);   

	// 启动显示线程
	pthread_create(&m_threadID, 0, displayThread, this);

	if (m_threadID == 0)
	{
		printf("Failed to create display thread!\n");
	}
	else
	{
		m_isExitDisplayThread = false;
	}
}

CammerWidget::~CammerWidget()
{
	// 关闭显示线程
	m_isExitDisplayThread = true;
	pthread_join(m_threadID, NULL);
    delete ui;
}

//设置曝光
bool CammerWidget::SetExposeTime(double exposureTime)
{
	if (!m_devHandle)
	{
		return false;
	}

	int ret = IMV_OK;

	ret = IMV_SetDoubleFeatureValue(m_devHandle, "ExposureTime", exposureTime);
	if (IMV_OK != ret)
	{
		printf("set ExposureTime value = %0.2f fail, ErrorCode[%d]\n", exposureTime, ret);
		return false;
	}

	return true;
}

//设置增益
bool CammerWidget::SetAdjustPlus(double gainRaw)
{
	if (!m_devHandle)
	{
		return false;
	}

	int ret = IMV_OK;

	ret = IMV_SetDoubleFeatureValue(m_devHandle, "GainRaw", gainRaw);
	if (IMV_OK != ret)
	{
		printf("set GainRaw value = %0.2f fail, ErrorCode[%d]\n", gainRaw, ret);
		return false;
	}

	return true;
}

//打开相机
bool CammerWidget::CameraOpen(void)
{
	int ret = IMV_OK;

	if (m_currentCameraKey.length() == 0)
	{
		printf("open camera fail. No camera.\n");
		return false;
	}

	if (m_devHandle)
	{
		printf("m_devHandle is already been create!\n");
		return false;
	}
	QByteArray cameraKeyArray = m_currentCameraKey.toLocal8Bit();
	char* cameraKey = cameraKeyArray.data();

	ret = IMV_CreateHandle(&m_devHandle, modeByCameraKey, (void*)cameraKey);
	if (IMV_OK != ret)
	{
		printf("create devHandle failed! cameraKey[%s], ErrorCode[%d]\n", cameraKey, ret);
		return false;
	}

	// 打开相机 
	// Open camera 
	ret = IMV_Open(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("open camera failed! ErrorCode[%d]\n", ret);
		return false;
	}

	return true;
}

//关闭相机
bool CammerWidget::CameraClose(void)
{
	if (!m_devHandle)
	{
		return false;
	}

	int ret = IMV_OK;

	if (!m_devHandle)
	{
		printf("close camera fail. No camera.\n");
		return false;
	}

	if (false == IMV_IsOpen(m_devHandle))
	{
		printf("camera is already close.\n");
		return false;
	}

	ret = IMV_Close(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("close camera failed! ErrorCode[%d]\n", ret);
		return false;
	}

	ret = IMV_DestroyHandle(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("destroy devHandle failed! ErrorCode[%d]\n", ret);
		return false;
	}

	m_devHandle = NULL;

	return true;
}

//开始采集
bool CammerWidget::CameraStart()
{
	if (!m_devHandle)
	{
		return false;
	}

	int ret = IMV_OK;

	if (IMV_IsGrabbing(m_devHandle))
	{
		printf("camera is already grebbing.\n");
		return false;
	}


	ret = IMV_AttachGrabbing(m_devHandle, FrameCallback, this);
	if (IMV_OK != ret)
	{
		printf("Attach grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	ret = IMV_StartGrabbing(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("start grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	return true;
}

//停止采集
bool CammerWidget::CameraStop()
{
	if (!m_devHandle)
	{
		return false;
	}

	int ret = IMV_OK;
	if (!IMV_IsGrabbing(m_devHandle))
	{
		printf("camera is already stop grebbing.\n");
		return false;
	}

	ret = IMV_StopGrabbing(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("Stop grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	// 清空显示队列 
	// clear display queue
	CFrameInfo frameOld;
	while (_displayFrameQueue.get(frameOld))
	{
		free(frameOld.m_pImageBuf);
		frameOld.m_pImageBuf = NULL;
	}

	_displayFrameQueue.clear();

	return true;
}

//切换采集方式、触发方式 （连续采集、外部触发、软件触发）
void CammerWidget::CameraChangeTrig(ETrigType trigType)
{
	if (!m_devHandle)
	{
		return;
	}

	int ret = IMV_OK;

	if (trigContinous == trigType)
	{
		// 设置触发模式
		// set trigger mode
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "Off");
		if (IMV_OK != ret)
		{
			printf("set TriggerMode value = Off fail, ErrorCode[%d]\n", ret);
			return;
		}
	}
	else if (trigSoftware == trigType)
	{
		// 设置触发器
		// set trigger
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSelector", "FrameStart");
		if (IMV_OK != ret)
		{
			printf("set TriggerSelector value = FrameStart fail, ErrorCode[%d]\n", ret);
			return;
		}

		// 设置触发模式
		// set trigger mode
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "On");
		if (IMV_OK != ret)
		{
			printf("set TriggerMode value = On fail, ErrorCode[%d]\n", ret);
			return;
		}

		// 设置触发源为软触发
		// set triggerSource as software trigger
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSource", "Software");
		if (IMV_OK != ret)
		{
			printf("set TriggerSource value = Software fail, ErrorCode[%d]\n", ret);
			return;
		}
	}
	else if (trigLine == trigType)
	{
		// 设置触发器
		// set trigger
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSelector", "FrameStart");
		if (IMV_OK != ret)
		{
			printf("set TriggerSelector value = FrameStart fail, ErrorCode[%d]\n", ret);
			return;
		}

		// 设置触发模式
		// set trigger mode
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "On");
		if (IMV_OK != ret)
		{
			printf("set TriggerMode value = On fail, ErrorCode[%d]\n", ret);
			return;
		}

		// 设置触发源为Line1触发
		// set trigggerSource as Line1 trigger 
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSource", "Line1");
		if (IMV_OK != ret)
		{
			printf("set TriggerSource value = Line1 fail, ErrorCode[%d]\n", ret);
			return;
		}
	}
}

//执行一次软触发
void CammerWidget::ExecuteSoftTrig(void)
{
	if (!m_devHandle)
	{
		return;
	}

	int ret = IMV_OK;

	ret = IMV_ExecuteCommandFeature(m_devHandle, "TriggerSoftware");
	if (IMV_OK != ret)
	{
		printf("ExecuteSoftTrig fail, ErrorCode[%d]\n", ret);
		return;
	}

	usleep(100 * 1000);

	printf("ExecuteSoftTrig success.\n");
	return;
}

//检测像机数、序列号
void CammerWidget::CameraCheck(void)
{
	IMV_DeviceList deviceInfoList;
	if (IMV_OK != IMV_EnumDevices(&deviceInfoList, interfaceTypeAll))
	{
		printf("Enumeration devices failed!\n");
		return;
	}

	// 打印相机基本信息（key, 制造商信息, 型号, 序列号）
	for (unsigned int i = 0; i < deviceInfoList.nDevNum; i++)
	{
		printf("Camera[%d] Info :\n", i);
		printf("    key           = [%s]\n", deviceInfoList.pDevInfo[i].cameraKey);
		printf("    vendor name   = [%s]\n", deviceInfoList.pDevInfo[i].vendorName);
		printf("    model         = [%s]\n", deviceInfoList.pDevInfo[i].modelName);
		printf("    serial number = [%s]\n", deviceInfoList.pDevInfo[i].serialNumber);
	}

	if (deviceInfoList.nDevNum < 1)
	{
		printf("no camera.\n");
	//	msgBoxWarn(tr("Device Disconnected."));
	}
	else
	{
		//默认设置列表中的第一个相机为当前相机，其他操作比如打开、关闭、修改曝光都是针对这个相机。
		m_currentCameraKey = deviceInfoList.pDevInfo[0].cameraKey;
	}
}

// 显示
bool CammerWidget::ShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, IMV_EPixelType pixelFormat)
{
	if (NULL == pRgbFrameBuf ||
		nWidth == 0 ||
		nHeight == 0)
	{
		printf("%s image is invalid.\n", __FUNCTION__);
		return false;
	}

	if (NULL == m_handler)
	{
		open(nWidth, nHeight);
		if (NULL == m_handler)
		{
			return false;
		}
	}

	if (m_params.nWidth != nWidth || m_params.nHeight != nHeight)
	{
		close();
		open(nWidth, nHeight);
	}

	VR_FRAME_S renderParam;
	memset(&renderParam, 0, sizeof(renderParam));
	renderParam.data[0] = pRgbFrameBuf;
	renderParam.stride[0] = nWidth;
	renderParam.nWidth = nWidth;
	renderParam.nHeight = nHeight;
	if (pixelFormat == gvspPixelMono8)
	{
		renderParam.format = VR_PIXEL_FMT_MONO8;
	}
	else
	{
		renderParam.format = VR_PIXEL_FMT_RGB24;
	}

	if (VR_SUCCESS == VR_RenderFrame(m_handler, &renderParam, NULL))
	{
		return true;
	}
	return false;
}

// 显示线程
void CammerWidget::DisplayThreadProc()
{
	while (!m_isExitDisplayThread)
	{
		CFrameInfo frameInfo;

		if (false == _displayFrameQueue.get(frameInfo))
		{
			usleep(1000);
			continue;
		}

		// 判断是否要显示。超过显示上限（30帧），就不做转码、显示处理
		if (!this->isTimeToDisplay(frameInfo.m_nTimeStamp))
		{
			/* 释放内存 */
			free(frameInfo.m_pImageBuf);
			continue;
		}

		/* mono8格式可不做转码，直接显示，其他格式需要经过转码才能显示 */
		if (gvspPixelMono8 == frameInfo.m_ePixelType)
		{
			/* 显示 */
			if (false == ShowImage(frameInfo.m_pImageBuf, frameInfo.m_nWidth, frameInfo.m_nHeight, frameInfo.m_ePixelType))
			{
				printf("_render.display failed.\n");
			}
			/* 释放内存 */
			free(frameInfo.m_pImageBuf);
		}
		else
		{
			/* 转码 */
			unsigned char *pRGBAbuffer = NULL;
			int nRgbaBufferSize = 0;
			nRgbaBufferSize = frameInfo.m_nWidth * frameInfo.m_nHeight * 4;
			pRGBAbuffer = (unsigned char *)malloc(nRgbaBufferSize);
			if (pRGBAbuffer == NULL)
			{
				/* 释放内存 */
				free(frameInfo.m_pImageBuf);
				printf("pRGBAbuffer malloc failed.\n");
				continue;
			}

			IMV_PixelConvertParam stPixelConvertParam;
			stPixelConvertParam.nWidth = frameInfo.m_nWidth;
			stPixelConvertParam.nHeight = frameInfo.m_nHeight;
			stPixelConvertParam.ePixelFormat = frameInfo.m_ePixelType;
			stPixelConvertParam.pSrcData = frameInfo.m_pImageBuf;
			stPixelConvertParam.nSrcDataLen = frameInfo.m_nBufferSize;
			stPixelConvertParam.nPaddingX = frameInfo.m_nPaddingX;
			stPixelConvertParam.nPaddingY = frameInfo.m_nPaddingY;
			stPixelConvertParam.eBayerDemosaic = demosaicNearestNeighbor;
			stPixelConvertParam.eDstPixelFormat = gvspPixelBGRA8;
			stPixelConvertParam.pDstBuf = pRGBAbuffer;
			stPixelConvertParam.nDstBufSize = nRgbaBufferSize;

			int ret = IMV_PixelConvert(m_devHandle, &stPixelConvertParam);
			if (IMV_OK != ret)
			{
				// 释放内存 
				// release memory
				printf("image convert to BGRA failed! ErrorCode[%d]\n", ret);
				free(frameInfo.m_pImageBuf);
				free(pRGBAbuffer);
				pRGBAbuffer = NULL;
				continue;
			}

			/* 释放内存 */
			free(frameInfo.m_pImageBuf);

			/* 显示 */
			if (false == ShowImage(pRGBAbuffer, frameInfo.m_nWidth, frameInfo.m_nHeight, frameInfo.m_ePixelType))
			{
				printf("_render.display failed.\n");
			}
			free(pRGBAbuffer);
			pRGBAbuffer = NULL;
		}
	}
}

bool CammerWidget::isTimeToDisplay(uint64_t nCurTime)
{
	m_mxTime.lock();

	// 不显示
	if (m_dDisplayInterval <= 0)
	{
		m_mxTime.unlock();
		return false;
	}

	// 时间戳频率获取失败, 默认全显示. 这种情况理论上不会出现
	if (m_nTimestampFreq <= 0)
	{
		m_mxTime.unlock();
		return true;
	}

	// 第一帧必须显示
	if (m_nFirstFrameTime == 0 || m_nLastFrameTime == 0)
	{
		m_nFirstFrameTime = nCurTime;
		m_nLastFrameTime = nCurTime;

		m_mxTime.unlock();
		return true;
	}

	// 当前时间戳比之前保存的小
	if (nCurTime < m_nFirstFrameTime)
	{
		m_nFirstFrameTime = nCurTime;
		m_nLastFrameTime = nCurTime;
		m_mxTime.unlock();
		return true;
	}

	// 当前帧和上一帧的间隔
	uintmax_t nDif = nCurTime - m_nLastFrameTime;
	double dTimstampInterval = 1.0 / m_nTimestampFreq;

	double dAcquisitionInterval = nDif * 1000 * dTimstampInterval;

	if (dAcquisitionInterval >= m_dDisplayInterval)
	{
		// 保存最后一帧的时间戳
		m_nLastFrameTime = nCurTime;
		m_mxTime.unlock();
		return true;
	}

	// 当前帧相对于第一帧的时间间隔
	uintmax_t nDif2 = nCurTime - m_nFirstFrameTime;
	double dCurrentFrameTime = nDif2 * 1000 * dTimstampInterval;

	if (dCurrentFrameTime > 1000 * 60 * 30) // 每隔一段时间更新起始时间
	{
		m_nFirstFrameTime = nCurTime;
	}
	// 保存最后一帧的时间戳
	m_nLastFrameTime = nCurTime;

	dCurrentFrameTime = fmod(dCurrentFrameTime, m_dDisplayInterval);

	if ((dCurrentFrameTime * 2 < dAcquisitionInterval)
		|| ((m_dDisplayInterval - dCurrentFrameTime) * 2 <= dAcquisitionInterval))
	{
		m_mxTime.unlock();
		return true;
	}

	m_mxTime.unlock();
	return false;
}

void CammerWidget::setDisplayFPS(int nFPS)
{
	m_mxTime.lock();
	if (nFPS > 0)
	{
		m_dDisplayInterval = 1000.0 / nFPS;
	}
	else
	{
		m_dDisplayInterval = 0;
	}
	m_mxTime.unlock();
}

bool CammerWidget::open(int width, int height)
{
	if (NULL != m_handler ||
		0 == width ||
		0 == height)
	{
		return false;
	}

	memset(&m_params, 0, sizeof(m_params));
#ifndef __linux__
	m_params.eVideoRenderMode = VR_MODE_GDI;
#else
	m_params.eVideoRenderMode = VR_MODE_OPENGLX;
#endif
	m_params.hWnd = m_hWnd/*(VR_HWND)this->winId()*/;
	m_params.nWidth = width;
	m_params.nHeight = height;

	VR_ERR_E ret = VR_Open(&m_params, &m_handler);
	if (ret == VR_NOT_SUPPORT)
	{
		printf("%s cant't display BGR on this computer\n", __FUNCTION__);
		return false;
	}

	if (ret != VR_SUCCESS)
	{
		printf("%s open failed. error code[%d]\n", __FUNCTION__, ret);
		return false;
	}

	return true;
}

bool CammerWidget::close()
{
	if (m_handler != NULL)
	{
		VR_Close(m_handler);
		m_handler = NULL;
	}
	return true;
}

void CammerWidget::closeEvent(QCloseEvent * event)
{
	if (event)
	{
		IMV_DestroyHandle(m_devHandle);
		m_devHandle = NULL;
	}
}

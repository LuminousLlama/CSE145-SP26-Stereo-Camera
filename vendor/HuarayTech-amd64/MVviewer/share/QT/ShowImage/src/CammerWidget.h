#ifndef _CAMMER_WIDGET_H__
#define _CAMMER_WIDGET_H__

#include <unistd.h>
#include <math.h>
#include <QWidget>
#include <QMessageBox>
#include "Media/VideoRender.h"
#include "IMVAPI/IMVApi.h"
#include "MessageQue.h"
#include <pthread.h>

class CFrameInfo
{
public:
	CFrameInfo()
	{
		m_pImageBuf = NULL;
		m_nBufferSize = 0;
		m_nWidth = 0;
		m_nHeight = 0;
		m_ePixelType = gvspPixelMono8;
		m_nPaddingX = 0;
		m_nPaddingY = 0;
		m_nTimeStamp = 0;
	}

	~CFrameInfo()
	{
	}

public:
	unsigned char*	m_pImageBuf;
	int				m_nBufferSize;
	int				m_nWidth;
	int				m_nHeight;
	IMV_EPixelType	m_ePixelType;
	int				m_nPaddingX;
	int				m_nPaddingY;
	uint64_t		m_nTimeStamp;
};

namespace Ui {
class CammerWidget;
}

class CammerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CammerWidget(QWidget *parent = 0);
    ~CammerWidget();

	//枚举触发方式
	enum ETrigType
	{
		trigContinous = 0,	//连续拉流
		trigSoftware = 1,	//软件触发
		trigLine = 2,		//外部触发		
	};
	//设置曝光
	bool SetExposeTime(double exposureTime);
	//设置增益
	bool SetAdjustPlus(double gainRaw);
	//打开相机
	bool CameraOpen(void);
	//关闭相机
	bool CameraClose(void);
	//开始采集
	bool CameraStart(void);
	//停止采集
	bool CameraStop(void);
	//切换采集方式、触发方式 （连续采集、外部触发、软件触发）
	void CameraChangeTrig(ETrigType trigType = trigContinous);
	//执行一次软触发（该接口为大华添加，用于执行软触发）            
	void ExecuteSoftTrig(void);
	//检测像机数、序列号
	void CameraCheck(void);
	/// \brief 显示一帧图像
	/// \param [in] pRgbFrameBuf 要显示的图像数据
	/// \param [in] nWidth 图像的宽
	/// \param [in] nHeight 图像的高
	/// \param [in] pixelFormat 图像的格式
	/// \retval true 显示失败
	/// \retval false  显示成功 
	bool ShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, IMV_EPixelType pixelFormat);

	// 显示线程
	void DisplayThreadProc();

	TMessageQue<CFrameInfo>				_displayFrameQueue;// 显示队列

private:
    Ui::CammerWidget *ui;
	
	QString								m_currentCameraKey;			// 当前相机key | current camera key
	IMV_HANDLE							m_devHandle;				// 相机句柄 | camera handle
	pthread_t							m_threadID;
	bool								m_isExitDisplayThread;

	QMutex								m_mxTime;
	double								m_dDisplayInterval;         // 显示间隔
	uint64_t							m_nTimestampFreq;           // 时间戳频率
	uint64_t							m_nFirstFrameTime;          // 第一帧的时间戳
	uint64_t							m_nLastFrameTime;           // 上一帧的时间戳

	VR_HANDLE          m_handler;           /* 绘图句柄 */
	VR_OPEN_PARAM_S    m_params;            /* 显示参数 */

	// 设置显示频率，默认一秒钟显示30帧
	void setDisplayFPS(int nFPS);

	// 计算该帧是否显示
	bool isTimeToDisplay(uint64_t nCurTime);

	/* 显示相关函数 */
	bool open(int width, int height);
	bool close();

	void closeEvent(QCloseEvent * event);

	VR_HWND		m_hWnd;
};

#endif // _CAMMER_WIDGET_H__

// RayUsb.h

#ifndef INCLUDED_RAYUSB_H
#define INCLUDED_RAYUSB_H

#include "RayApi.h"

// <1> linux usb

typedef struct RAY_dma_addr_st
{
	char data[12];
} RAY_dma_addr_t;


typedef struct RAY_v4l2_buffer_st
{
	char data[92];
} RAY_v4l2_buffer;

typedef struct RAY_v4l2_device_st
{
	char data[160];
} RAY_v4l2_device;

//log output
#define U3V_TRACE_INFO			(1 << 0)
#define U3V_TRACE_ERROR			(1 << 1)
#define U3V_TRACE_MAIN			(1 << 2)
#define U3V_TRACE_CONTROL		(1 << 3)
#define U3V_TRACE_STREAMEVENT	(1 << 4)
#define U3V_TRACE_V4L2			(1 << 5)
#define U3V_TRACE_VIDEO			(1 << 6)
#define U3V_TRACE_DRIVER		(1 << 7)
#define U3V_TRACE_QUEUE			(1 << 8)

//buffer state
enum u3v_buffer_state {
	U3V_BUF_STATE_IDLE		= 0,
	U3V_BUF_STATE_QUEUED	= 1,
	U3V_BUF_STATE_ACTIVE	= 2,
	U3V_BUF_STATE_READY		= 3,
	U3V_BUF_STATE_DONE		= 4,
	U3V_BUF_STATE_ERROR		= 5,
};

//queue
#define U3V_QUEUE_IDLE			0
#define U3V_QUEUE_STREAM_START	1
#define U3V_QUEUE_STREAM_STOP	2

//urb
#define RAY_URB_NO_TRANSFER_DMA_MAP		0x0004
#define RAY_VFL_TYPE_GRABBER			0

//v4l2_ioctl 序号
#define RAY_VIDIOC_MASK					0x000000ff
#define RAY_VIDIOC_REQBUFS				0x00000008
#define RAY_VIDIOC_QUERYBUF				0x00000009
#define RAY_VIDIOC_QBUF					0x0000000f
#define RAY_VIDIOC_DQBUF				0x00000011
#define RAY_VIDIOC_STREAMON				0x00000012
#define RAY_VIDIOC_STREAMOFF			0x00000013
#define RAY_VIDIOC_INTERFACE_CONTROL	0x00000064
#define RAY_VIDIOC_STREAM_EVENT			0x00000065
#define RAY_VIDIOC_LOST_FRAME_COUNT		0x00000066
#define RAY_VIDIOC_INTERFACE_CONTROL_ASYNC	0x00000067
#define RAY_VIDIOC_ALLOC_MEMERY			0x00000068
#define RAY_VIDIOC_FREE_MEMERY			0x00000069
#define RAY_VIDIOC_DEVICE_STATUS		0x0000006a
#define RAY_VIDIOC_STREAM_STATISTICS	0x0000006b
#define RAY_VIDIOC_STREAM_STATISTICS_RESET 0x0000006c
#define RAY_VIDIOC_STREAM_STRATEGY		0x0000006d
#define RAY_VIDIOC_BUFFER_CLEAR			0x0000006e

#define RAY_V4L2_MEMORY_MMAP			0x01

#define RAY_V4L2_BUF_FLAG_MAPPED		0x01
#define	RAY_V4L2_BUF_FLAG_QUEUED		0x02
#define	RAY_V4L2_BUF_FLAG_DONE			0x04

#define RAY_V4L2_BUF_TYPE_VIDEO_CAPTURE	0x01

//socket
#define RAY_POLLIN      0x0001
#define RAY_POLLPRI     0x0002
#define RAY_POLLOUT     0x0004
#define RAY_POLLERR     0x0008
#define RAY_POLLHUP     0x0010
#define RAY_POLLNVAL    0x0020
#define RAY_POLLRDNORM  0x0040
#define RAY_POLLRDBAND  0x0080
#define RAY_POLLWRNORM  0x0100
#define RAY_POLLWRBAND  0x0200
#define RAY_POLLMSG     0x0400
#define RAY_POLLREMOVE  0x1000
#define RAY_POLLRDHUP   0x2000

//return 判断
typedef enum 
{
	RAY_RESULT_OK = 0,				// success
	RAY_RESULT_ENOMEM,				// no memory
	RAY_RESULT_EINVAL,				// invalid value
	RAY_RESULT_EBUSY,				// operate busy
	RAY_RESULT_ENOENT,				// No such file or directory
	RAY_RESULT_ECONNRESET,			// Connection reset by peer
	RAY_RESULT_ESHUTDOWN,			// Cannot send after transport endpoint shutdown
	RAY_RESULT_ESTATE,				// error state
	RAY_RESULT_EIO,					// I/O error
	RAY_RESULT_EQUEUEEMPTY,			// queue empty error
	RAY_RESULT_EU3VSTATE,			// u3v state error
	RAY_RESULT_ETYPE,				// type error
	RAY_RESULT_ETIMEOUT,			// time out


	RAY_RESULT_NODEFIND = 100
}RAY_RESULT;

#define RAY_PAGE_SHIFT	12

extern void* RAY_usb_get_dev(void* udev);
extern void* RAY_usb_get_intf(void* intf);
extern void* RAY_interface_to_usbdev(void* intf);
extern int RAY_intfNumSet(void* intf);
extern void* RAY_usbHostConfigGet(void* udev);
extern int RAY_interfaceNumGet(void* usb_host_config);
extern void* RAY_intfaceConfigGet(void* usb_host_config, int index, int* interfaceProtocol, int* interfaceNum);
extern void* RAY_usbIntfGet(void* intf);
extern int RAY_v4l2BufType(int v4l2BufType);
extern void RAY_urbConfigGet(void* urb, uint8_t** buffer, unsigned int* lenth);
extern unsigned int RAY_bufOffsetGet(void* buf);
extern void RAY_bufOffsetSet(void* buf, unsigned int bytes);
extern unsigned int RAY_writedBytesGet(void* buf);
extern void RAY_writedBytesSet(void* buf, unsigned int value);
extern unsigned int RAY_min(unsigned int len, unsigned int maxLen);
extern unsigned int RAY_bufIndexGet(void* buf);

//urb
extern void* RAY_usb_alloc_urb(int iso_packets, int gfp_flags);
extern int RAY_usb_submit_urb(void* urb, int mem_flags);
void* RAY_usb_buffer_alloc(void* dev, int size, int mem_flags, int gfp_flags, RAY_dma_addr_t* dma);

extern void RAY_usb_buffer_free(void* dev, int size, void *addr, RAY_dma_addr_t* dma);
extern void RAY_usb_kill_urb(void* urb);
extern void RAY_usb_free_urb(void* urb);
extern void RAY_usb_fill_bulk_urb(void* urb, void* udev, unsigned int pipe, char* urbBuffer, int bufferLen, void* complete, void* context);

extern int RAY_u3vStreamDataGet(void* urb, void** stream);
extern int RAY_u3vControlBulkInDataGet(void* urb, void** control);
extern int RAY_u3vControlBulkOutDataGet(void* urb);

extern unsigned int RAY_DIV_ROUND_UP(unsigned int size, unsigned int psize);

//endpoint
extern void RAY_endPointGet(void* intf, void** endPoint, int index);
extern unsigned short RAY_maxPacketSizeGet(void* ep);
extern int RAY_usb_endpoint_is_bulk_in(void* ep);
extern int RAY_usb_endpoint_is_bulk_out(void* ep);
extern unsigned int RAY_endPointAddrGet(void* ep);
extern unsigned int RAY_endPointNumGet(void* intf);
extern unsigned int RAY_usb_rcvbulkpipe(void* dev, unsigned int bEndpointAddress);
extern unsigned int RAY_usb_sndbulkpipe(void* dev, unsigned int bEndpointAddress);

extern unsigned short RAY_le16_to_cpu(unsigned short le_val);
extern void RAY_urbDataSet(void** dstData, void* srcData, RAY_dma_addr_t* dma, unsigned int transfer_flags);
extern void RAY_usb_put_dev(void* udev);
extern void RAY_usb_put_intf(void* intf);
extern int RAY_usb_autopm_get_interface(void* intf);
extern void RAY_usb_autopm_put_interface(void* intf);
extern void RAY_v4l2DataSet(void* handle, void* file);
extern void RAY_v4l2ReqGet(void* v4l2Req, int* type, int* memoryType, int* count, int* payloadSize);
extern void RAY_v4l2ReqSet(void* v4l2Req, int count);
extern void RAY_v4l2BufGet(void* buf, int* type, int* index, int* memoryType);
extern unsigned int RAY_IOcmdTypeGet(unsigned int cmdType);
extern int RAY_v4l2MemoryTypeGet(int v4l2Type);
extern void RAY_v4l2BufSet(void* buf, int index, int size, int len, int type);
extern void RAY_copyV4l2Data(void* dstBuf, void* srcBuf);
extern int RAY_v4l2BufFlag(int v4l2BufFlags);
extern void RAY_v4l2BufFlagSet(void* buf, int state);
extern void RAY_v4l2BufReservedGet(void* buf, unsigned int* reserved);
extern void RAY_v4l2BufOffsetGet(void* buf, unsigned int* offset);
extern unsigned int RAY_pageShiftGet(unsigned int pageShift);

//vma
extern void RAY_vmaDataGet(void* buf, unsigned long* startAddr, unsigned long* endAddr, unsigned long* pgoff);
extern void RAY_vmaFlagVMIOSet(void* vma);
extern void* RAY_vmPrivateDataGet(void* vma);

extern void RAY_video_set_drvdata(void* videoDev, void* u3vDevData);
extern int RAY_video_register_device(void* videoDev, int type, int nr);
extern void RAY_video_device_release(void* videoDev);
extern void RAY_video_unregister_device(void* videoDev);
extern void RAY_v4l2_device_unregister(void* vdev);
extern unsigned int RAY_v4l2_device_register(void* intf, void* vdev);

extern void RAY_urbTransferBufLenSet(void* buf, unsigned int len);
extern void RAY_urbActualLenGet(void* buf, unsigned int* len);;
extern unsigned int pollStatusGet(unsigned int pollStatus);
extern int RAY_wait_event_interruptible_timeout_queue(RAY_wait_queue_head_t* wq, unsigned int* flag, int* state, unsigned long timeOut);

#endif // INCLUDED_RAYADT_H


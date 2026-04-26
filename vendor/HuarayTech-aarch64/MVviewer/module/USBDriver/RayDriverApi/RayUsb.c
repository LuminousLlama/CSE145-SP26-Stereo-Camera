// RayUsb.c
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>
#include <linux/usb.h>
#include <media/v4l2-device.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>

#include "RayUsb.h"

void* RAY_usb_get_dev(void* udev)
{
	return usb_get_dev( (struct usb_device*)udev );
}

void* RAY_usb_get_intf(void* intf)
{
	return usb_get_intf( (struct usb_interface*)intf );	
}

void* RAY_interface_to_usbdev(void* intf)
{
	struct usb_interface* intfData = (struct usb_interface*)intf;
	return interface_to_usbdev( intfData );
}

int RAY_intfNumSet(void* intf)
{
	struct usb_interface* usbIntf = (struct usb_interface*)intf;
	return usbIntf->cur_altsetting->desc.bInterfaceNumber;
}

void* RAY_usbHostConfigGet(void* udev)
{
	struct usb_device* usbDev = (struct usb_device*)udev;
	return usbDev->actconfig;
}

int RAY_interfaceNumGet(void* usb_host_config)
{
	struct usb_host_config* usbHostConfig = (struct usb_host_config*)usb_host_config;
	return usbHostConfig->desc.bNumInterfaces;
}

void* RAY_intfaceConfigGet(void* usb_host_config, int index, int* interfaceProtocol, int* interfaceNum)
{
	struct usb_host_config* usbHostConfig = (struct usb_host_config*)usb_host_config;
	struct usb_interface* usbIntf = usbHostConfig->interface[index];

	if(usbIntf)
	{
		*interfaceProtocol = usbIntf->cur_altsetting->desc.bInterfaceProtocol;
		*interfaceNum = usbIntf->cur_altsetting->desc.bInterfaceNumber;
	}

	return (void*)usbIntf;
}

void* RAY_usbIntfGet(void* intf)
{
	return usb_get_intf( (struct usb_interface*)intf );	
}

int RAY_v4l2BufType(int v4l2BufType)
{
	int __v4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	switch( v4l2BufType )
	{
	case RAY_V4L2_BUF_TYPE_VIDEO_CAPTURE :
		__v4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		break;
	default :
		break;
	}

	return __v4l2BufType;
}

void RAY_urbConfigGet(void* urb, uint8_t** buffer, unsigned int* lenth)
{
	struct urb* urbConfig = (struct urb*)urb;

	if(urbConfig->actual_length != 0)
	{
		*lenth = urbConfig->actual_length;
		*buffer = urbConfig->transfer_buffer;
	}

	return;
}

unsigned int RAY_bufOffsetGet(void* buf)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;

	if(v4l2Buf)
	{
		return v4l2Buf->m.offset;
	}

	return 0;
}

void RAY_bufOffsetSet(void* buf, unsigned int offsetValue)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;

	if(v4l2Buf)
	{
		v4l2Buf->bytesused += offsetValue;
	}
}

unsigned int RAY_writedBytesGet(void* buf)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;

	if(v4l2Buf)
	{
		return v4l2Buf->bytesused;
	}

	return 0;
}

void RAY_writedBytesSet(void* buf, unsigned int value)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;

	if(v4l2Buf)
	{	
		v4l2Buf->bytesused = value;
	}
	return;
}

unsigned int RAY_min(unsigned int len, unsigned int maxLen)
{
	return min((unsigned int)len, maxLen);
}

unsigned int RAY_bufIndexGet(void* buf)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;
	return v4l2Buf->index;
}

int RAY_u3vStreamDataGet(void* urb, void** stream)
{
	struct urb* urbData = (struct urb*)urb;
	if(urbData)
	{
		*stream = urbData->context;
		return urbData->status;
	}

	return -1;
}

int RAY_u3vControlBulkInDataGet(void* urb, void** control)
{
	struct urb* urbData = (struct urb*)urb;
	if(urbData)
	{
		*control = urbData->context;
		return urbData->status;
	}

	return -1;
}

int RAY_u3vControlBulkOutDataGet(void* urb)
{
	struct urb* urbData = (struct urb*)urb;
	if(urbData)
	{
		return urbData->status;
	}
	
	return -1;
}

int RAY_usb_submit_urb(void* urb, int mem_flags)
{
	int memFlags = GFP_KERNEL;

	switch( mem_flags )
	{
	case RAY_GFP_KERNEL :
		memFlags = GFP_KERNEL;
		break;
	case RAY_GFP_NOIO :
		memFlags = GFP_NOIO;
		break;
	case RAY_GFP_ATOMIC :
		memFlags = GFP_ATOMIC;
		break;
	default :
		break;
	}

	return usb_submit_urb( (struct urb*)urb, memFlags);
}

void* RAY_usb_buffer_alloc(void* dev, int size, int mem_flags, int gfp_flags, RAY_dma_addr_t* dma)
{
	int memFlags = GFP_KERNEL;
	int gfpFlags = __GFP_NOWARN;

	switch( mem_flags )
	{
	case RAY_GFP_KERNEL :
		memFlags = GFP_KERNEL;
		break;
	case RAY_GFP_NOIO :
		memFlags = GFP_NOIO;
		break;
	default :
		break;
	}

	switch( gfp_flags )
	{
	case RAY_GFP_NOWARN :
		gfpFlags = __GFP_NOWARN;
		break;
	default :
		break;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
	return usb_buffer_alloc((struct usb_device *)dev, size, memFlags | gfpFlags, (dma_addr_t *)dma);
#else
	return usb_alloc_coherent( (struct usb_device *)dev, size, memFlags | gfpFlags, (dma_addr_t *)dma );
#endif
}

void RAY_usb_buffer_free(void* dev, int size, void *addr, RAY_dma_addr_t* dma)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
	usb_buffer_free( (struct usb_device*)dev, size, addr, *(dma_addr_t *)dma );
#else
	usb_free_coherent( (struct usb_device*)dev, size, addr, *(dma_addr_t *)dma );
#endif
	return;
}

unsigned int RAY_DIV_ROUND_UP(unsigned int size, unsigned int psize)
{
	return DIV_ROUND_UP(size, psize);
}

void RAY_usb_kill_urb(void* urb)
{
	usb_kill_urb( (struct urb*)urb );
	return;
}

void RAY_usb_free_urb(void* urb)
{
	usb_free_urb( (struct urb*)urb );
	return;
}

void RAY_endPointGet(void* intf, void** endPoint, int index)
{
	struct usb_interface* usbIntf = (struct usb_interface*)intf;
	if(usbIntf && usbIntf->cur_altsetting)
	{
		*endPoint = &(usbIntf->cur_altsetting->endpoint[index]);
	}

	return;
}

unsigned short RAY_maxPacketSizeGet(void* ep)
{
	struct usb_host_endpoint* endPoint = (struct usb_host_endpoint*)ep;
	if(endPoint)
	{
		return endPoint->desc.wMaxPacketSize;
	}

	return 0;
}

int RAY_usb_endpoint_is_bulk_in(void* ep)
{
	struct usb_host_endpoint* endPoint = (struct usb_host_endpoint*)ep;
	return usb_endpoint_is_bulk_in(&endPoint->desc);
}

int RAY_usb_endpoint_is_bulk_out(void* ep)
{
	struct usb_host_endpoint* endPoint = (struct usb_host_endpoint*)ep;
	return usb_endpoint_is_bulk_out(&endPoint->desc);
}

unsigned int RAY_endPointAddrGet(void* ep)
{
	struct usb_host_endpoint* endPoint = (struct usb_host_endpoint*)ep;
	if(endPoint)
	{
		return endPoint->desc.bEndpointAddress;
	}

	return 0;
}

unsigned int RAY_usb_rcvbulkpipe(void* dev, unsigned int bEndpointAddress)
{
	return usb_rcvbulkpipe( (struct usb_device*)dev, bEndpointAddress );
}

unsigned int RAY_usb_sndbulkpipe(void* dev, unsigned int bEndpointAddress)
{
	return usb_sndbulkpipe( (struct usb_device*)dev, bEndpointAddress );
}

unsigned int RAY_endPointNumGet(void* intf)
{
	struct usb_interface* usbIntf = (struct usb_interface*)intf;

	if(usbIntf && usbIntf->cur_altsetting)
	{
		return usbIntf->cur_altsetting->desc.bNumEndpoints;
	}

	return 0;
}


unsigned short RAY_le16_to_cpu(unsigned short le_val)
{
	return le16_to_cpu(le_val);
}

void* RAY_usb_alloc_urb(int iso_packets, int gfp_flags)
{
	int memFlags = GFP_KERNEL;

	switch( gfp_flags )
	{
	case RAY_GFP_KERNEL :
		memFlags = GFP_KERNEL;
		break;
	case RAY_GFP_NOIO :
		memFlags = GFP_NOIO;
		break;
	default :
		break;
	}

	return usb_alloc_urb(iso_packets, memFlags);
}

void RAY_usb_fill_bulk_urb(void* urb, void* udev, unsigned int pipe, char* urbBuffer, int bufferLen, void* complete, void* context)
{
	struct urb* urbData = (struct urb*)urb;
	struct usb_device* usbDev = (struct usb_device*)udev;

	usb_fill_bulk_urb(urbData, usbDev, pipe, urbBuffer, bufferLen, (usb_complete_t)complete, context);
	return;
}

void RAY_urbDataSet(void** dstData, void* srcData, RAY_dma_addr_t* dma, unsigned int transfer_flags)
{
	struct urb* srcUrbData = (struct urb*)srcData;
	int transferFlags = URB_NO_TRANSFER_DMA_MAP;

	switch( transfer_flags )
	{
	case RAY_URB_NO_TRANSFER_DMA_MAP :
		transferFlags = URB_NO_TRANSFER_DMA_MAP;
		break;
	default :
		break;
	}

	if(srcUrbData)
	{
		srcUrbData->transfer_flags = transferFlags;
		srcUrbData->transfer_dma = *((dma_addr_t*)dma);
		*dstData = (void*)srcUrbData;
	}

	return;
}

unsigned int RAY_v4l2_device_register(void* intf, void* vdev)
{
	struct usb_interface* usbIntf = (struct usb_interface*)intf;
	return v4l2_device_register(&usbIntf->dev, (struct v4l2_device*)vdev);
}

void RAY_video_set_drvdata(void* videoDev, void* u3vDevData)
{
	struct video_device* vdev = (struct video_device*)videoDev;
	video_set_drvdata(vdev, u3vDevData);

	return;
}

int RAY_video_register_device(void* videoDev, int type, int nr)
{
	int videoType = VFL_TYPE_GRABBER;

	switch( type )
	{
	case RAY_VFL_TYPE_GRABBER :
		videoType = VFL_TYPE_GRABBER;
		break;
	default :
		break;
	}
	return video_register_device( (struct video_device*)videoDev, videoType, nr );
}

void RAY_video_device_release(void* videoDev)
{
	video_device_release( (struct video_device*)videoDev );
	return;
}

void RAY_video_unregister_device(void* videoDev)
{
	video_unregister_device( (struct video_device*)videoDev );
}

void RAY_v4l2_device_unregister(void* vdev)
{
	v4l2_device_unregister( (struct v4l2_device*)vdev );
	return;
}

void RAY_usb_put_dev(void* udev)
{
	usb_put_dev( (struct usb_device*)udev );
	return;
}

void RAY_usb_put_intf(void* intf)
{
	usb_put_intf( (struct usb_interface*)intf );
	return;
}

int RAY_usb_autopm_get_interface(void* intf)
{
	return usb_autopm_get_interface( (struct usb_interface*)intf );
}

void RAY_usb_autopm_put_interface(void* intf)
{
	usb_autopm_put_interface( (struct usb_interface*)intf );
	return;
}

void RAY_v4l2DataSet(void* handle, void* file)
{
	struct file* fileData = (struct file*)file;
	fileData->private_data = handle;
	return;
}

void RAY_v4l2ReqGet(void* v4l2Req, int* type, int* memoryType, int* count, int* payloadSize)
{
	struct v4l2_requestbuffers rb = {0};	
	RAY_copy_from_user(&rb, (struct v4l2_requestbuffers*)v4l2Req, sizeof(rb));

	*type = rb.type;
	*memoryType = rb.memory;
	*count = rb.count;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
	*payloadSize = rb.reserved[0];
#else
	*payloadSize = rb.capabilities;
#endif

	return;
}

void RAY_v4l2ReqSet(void* v4l2Req, int count)
{
	struct v4l2_requestbuffers *rb = (struct v4l2_requestbuffers*)v4l2Req;
	if(rb)
	{
		rb->count = count;
	}
	
	return;
}

void RAY_v4l2BufGet(void* buf, int* type, int* index, int* memoryType)
{
	struct v4l2_buffer v4l2Buf = {0};

	RAY_copy_from_user(&v4l2Buf, (struct v4l2_buffer*)buf, sizeof(v4l2Buf));

	*type = v4l2Buf.type;
	*index = v4l2Buf.index;
	*memoryType = v4l2Buf.memory;
	
	return;
}

unsigned int RAY_IOcmdTypeGet(unsigned int cmdType)
{
	unsigned int __cmdType = RAY_VIDIOC_MASK;

	switch( cmdType )
	{
	case VIDIOC_REQBUFS :
		__cmdType = RAY_VIDIOC_REQBUFS;
		break;
	case VIDIOC_QUERYBUF :
		__cmdType = RAY_VIDIOC_QUERYBUF;
		break;
	case VIDIOC_QBUF :
		__cmdType = RAY_VIDIOC_QBUF;
		break;
	case VIDIOC_DQBUF :
		__cmdType = RAY_VIDIOC_DQBUF;
		break;
	case VIDIOC_STREAMON :
		__cmdType = RAY_VIDIOC_STREAMON;
		break;
	case VIDIOC_STREAMOFF :
		__cmdType = RAY_VIDIOC_STREAMOFF;
		break;
	default :
		__cmdType = cmdType & RAY_VIDIOC_MASK;
		break;
	}

	return __cmdType;
}

int RAY_v4l2MemoryTypeGet(int v4l2Type)
{
	int __v4l2Type = V4L2_MEMORY_MMAP;

	switch( v4l2Type )
	{
	case RAY_V4L2_MEMORY_MMAP :
		__v4l2Type = V4L2_MEMORY_MMAP;
		break;
	default :
		break;
	}

	return __v4l2Type;
}

void RAY_copyV4l2Data(void* dstBuf, void* srcBuf)
{
	RAY_copy_to_user((struct v4l2_buffer *)dstBuf, (struct v4l2_buffer *)srcBuf, sizeof(struct v4l2_buffer));
	return;
}

int RAY_v4l2BufFlag(int v4l2BufFlags)
{
	int __v4l2BufFlags = V4L2_BUF_FLAG_MAPPED;

	switch( v4l2BufFlags )
	{
	case RAY_V4L2_BUF_FLAG_MAPPED :
		__v4l2BufFlags = V4L2_BUF_FLAG_MAPPED;
		break;
	case RAY_V4L2_BUF_FLAG_QUEUED :
		__v4l2BufFlags = V4L2_BUF_FLAG_QUEUED;
		break;
	case RAY_V4L2_BUF_FLAG_DONE :
		__v4l2BufFlags = V4L2_BUF_FLAG_DONE;
		break;
	default :
		break;
	}

	return __v4l2BufFlags;
}

void RAY_v4l2BufSet(void* buf, int index, int size, int len, int type)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;

	memset(v4l2Buf, 0, sizeof(struct v4l2_buffer));
	v4l2Buf->index = index;
	v4l2Buf->m.offset = index * size;
	v4l2Buf->length = len;
	v4l2Buf->type = type;
	v4l2Buf->field = V4L2_FIELD_NONE;
	v4l2Buf->memory = V4L2_MEMORY_MMAP;
	v4l2Buf->flags = 0;

	return;
}

void RAY_v4l2BufFlagSet(void* buf, int state)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;
	if(v4l2Buf)
	{
		v4l2Buf->flags |= state;
	}
	
	return;
}

void RAY_v4l2BufReservedGet(void* buf, unsigned int* reserved)
{
	struct v4l2_buffer v4l2Buf = {0};
	
	RAY_copy_from_user(&v4l2Buf, (struct v4l2_buffer*)buf, sizeof(v4l2Buf));

	*reserved = v4l2Buf.reserved;

	return;
}

void RAY_urbTransferBufLenSet(void* buf, unsigned int len)
{
	struct urb* transferBuf = (struct urb*)buf;
	if(transferBuf)
	{
		transferBuf->transfer_buffer_length = len;
	}
	
	return;
}

void RAY_urbActualLenGet(void* buf, unsigned int* len)
{
	struct urb* transferBuf = (struct urb*)buf;
	if(transferBuf)
	{
		*len = transferBuf->actual_length;
	}
	
	return;
}

void RAY_v4l2BufOffsetGet(void* buf, unsigned int* offset)
{
	struct v4l2_buffer* v4l2Buf = (struct v4l2_buffer*)buf;

	if(v4l2Buf)
	{
		*offset = v4l2Buf->m.offset;
	}
	
	return;
}

unsigned int RAY_pageShiftGet(unsigned int pageShift)
{
	int __pageShift = PAGE_SHIFT;

	switch( pageShift )
	{
	case RAY_PAGE_SHIFT :
		__pageShift = PAGE_SHIFT;
		break;
	default :
		break;
	}

	return __pageShift;
}

void RAY_vmaDataGet(void* vma, unsigned long* startAddr, unsigned long* endAddr, unsigned long* pgoff)
{
	struct vm_area_struct* vmaData = (struct vm_area_struct*)vma;
	if(vmaData)
	{
		*startAddr = vmaData->vm_start;
		*endAddr = vmaData->vm_end;
		*pgoff = vmaData->vm_pgoff;
	}

	return;
}

void RAY_vmaFlagVMIOSet(void* vma)
{
	struct vm_area_struct* vmaData = (struct vm_area_struct*)vma;
	if(vmaData)
	{
		vmaData->vm_flags |= VM_IO;
	}
	
	return;
}

void* RAY_vmPrivateDataGet(void* vma)
{
	struct vm_area_struct* vmaData = (struct vm_area_struct*)vma;
	return vmaData->vm_private_data;
}

unsigned int pollStatusGet(unsigned int pollStatus)
{
	unsigned int __pollStatus = POLLIN;

	switch( pollStatus )
	{
	case RAY_POLLIN :
		__pollStatus = POLLIN;
		break;
	case RAY_POLLOUT :
		__pollStatus = POLLOUT;
		break;
	case RAY_POLLRDNORM :
		__pollStatus = POLLRDNORM;
		break;
	case RAY_POLLWRNORM :
		__pollStatus = POLLWRNORM;
		break;
	case RAY_POLLERR :
		__pollStatus = POLLERR;
		break;
	case RAY_POLLHUP :
		__pollStatus = POLLHUP;
		break;
	case RAY_POLLNVAL :
		__pollStatus = POLLNVAL;
		break;
	case RAY_POLLRDBAND :
		__pollStatus = POLLRDBAND;
		break;
	case RAY_POLLWRBAND :
		__pollStatus = POLLWRBAND;
		break;
	case RAY_POLLMSG :
		__pollStatus = POLLMSG;
		break;
	case RAY_POLLREMOVE :
		__pollStatus = POLLREMOVE;
		break;
	case RAY_POLLRDHUP :
		__pollStatus = POLLRDHUP;
		break;
	default :
		break;
	}

	return __pollStatus;
}

int RAY_wait_event_interruptible_timeout_queue(RAY_wait_queue_head_t* wq, unsigned int* flag, int* state, unsigned long timeOut)
{
	wait_queue_head_t* wait = (wait_queue_head_t *)wq;
	return wait_event_interruptible_timeout(*wait, *flag == U3V_QUEUE_STREAM_STOP || *state == U3V_BUF_STATE_DONE, timeOut);
}

///////////////////////////////////////////////////////////
// <1> linux usb


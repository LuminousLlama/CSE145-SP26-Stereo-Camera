PWD = $(shell pwd)

#(1) 交叉编译工具
CROSS_COMPILE =
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
RM = rm
CHMOD = chmod
CP = cp

TARGET := u3v_drv

#(2) 依赖头文件和lib路径 
KERNEL_SRC := /lib/modules/$(shell uname -r)/build 

EX_LIB_MODULE := libModule/libu3v-core.a

EX_LIB_MODULE_OBJS := libModule/u3v_control.o libModule/u3v_driver.o libModule/u3v_queue.o libModule/u3v_stream_event.o libModule/u3v_v4l2.o libModule/u3v_video.o

EX_DRIVERAPI_OBJS := U3V_Module.o RayDriverApi/RayUsb.o RayDriverApi/RayApi.o

#(3) 目标文件名 
obj-m += $(TARGET).o

ifeq ($(platform),ArbitraryArm)
DST_PATH=../../../../Lib/$(platform)/
$(TARGET)-objs := $(EX_DRIVERAPI_OBJS) $(EX_LIB_MODULE_OBJS)
else
$(TARGET)-objs := $(EX_DRIVERAPI_OBJS) $(EX_LIB_MODULE)
endif

#(4) 编译 
all:
	$(MAKE) -C $(KERNEL_SRC) $(CCFLAGS) M=$(PWD) modules
	$(RM) -rf *.mod.c *.order *.symvers *~ *.o *.unsigned .*.cmd .tmp*
	$(CHMOD) 777 *.ko
ifeq ($(platform),ArbitraryArm)
	$(CP) -rf $(TARGET).ko $(DST_PATH)
endif
	
#(5) 清除
clean:
	$(RM) -rf *.o *.mod.c *.order *.symvers *.ko *.unsigned .*.cmd .tmp*

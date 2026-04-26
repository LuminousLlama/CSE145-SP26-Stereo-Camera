PWD = $(shell pwd)

#(1) 交叉编译工具
CROSS_COMPILE =
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld

#(2) 操作指令
RM = rm
CHMOD = chmod
CP = cp
MV = mv

#(3) 依赖头文件和lib路径 
KERNEL_SRC := /lib/modules/$(shell uname -r)/build 

KERNEL_OBJ := libModule/packetCheck.o libModule/eventProcess.o libModule/fileOperate.o libModule/packetResend.o libModule/frameRegroup.o libModule/ringBuffer.o libModule/timer.o 

KERNEL_CORE := universalEthernet.o RayDriverApi/RayApi.o RayDriverApi/RayFile.o RayDriverApi/RayNet.o

KERNEL_LIB := libModule/libuniversalEthernet-core.o

EXTRA_CFLAGS += -D_UNIX_ -D_PT_KERNEL_ -DRAY_ARM_UNSUPPORT_WORKQUEUE

#(4) 清除编译过程中生成的文件
KERNEL_DEL := *.mod.c *.order *.symvers *~ *.unsigned .*.cmd .tmp* *.o

KERNEL_DELS := libModule/*.mod.c libModule/*.order libModule/*.symvers libModule/*~ libModule/*.unsigned libModule/.*.cmd libModule/.tmp* libModule/*.o

#(5) 目标文件名 
#HW_TYPE:=$(shell uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/')
ifeq ($(platform),m32x86)
	driverName=m32x86
else ifeq ($(platform),m64x86)
	driverName=m64x86
else
	driverName=arm
endif

obj-m += mvUniversalForEthernet.o

MODULE_NAME := mvUniversalForEthernet

ifeq ($(platform),ArbitraryArm)
DST_PATH=../../../../Lib/$(platform)/
$(MODULE_NAME)-objs := $(KERNEL_CORE) $(KERNEL_OBJ)
else
$(MODULE_NAME)-objs := $(KERNEL_CORE) $(KERNEL_LIB)
endif
			
#(6) 编译 
all: 	
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules
	$(CHMOD) 755 *.ko 
	$(RM) -rf $(KERNEL_DEL) $(KERNEL_DELS)
ifeq ($(platform),ArbitraryArm)
	$(CP) -rf *.ko $(DST_PATH)
endif

#(7) 清除
clean:
	$(RM) -rf $(KERNEL_DEL) $(KERNEL_DELS) *.ko 

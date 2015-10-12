LOCAL_PATH := $(call my-dir)

# Building the vpuapi
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
 	vpuapi.c \
	vpuapifunc.c \
	../src/vpuhelper.c \
	../src/vpuio.c \

 LOCAL_SRC_FILES += \
 	../vdi/linux/vdi.c \
   	../vdi/linux/vdi_osal.c


# LOCAL_SRC_FILES += \
#  		../vdi/socket/vdi.c \
#  		../vdi/socket/vdi_osal.c \
# 		../vdi/socket/hpi_client.c


LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libvpu

LOCAL_CFLAGS := -DCONFIG_DEBUG_LEVEL=255
# LOCAL_CFLAGS += -DCNM_FPGA_PLATFORM

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES :=       \
		libutils      \
		libdl  		 \
		libdvm 

		
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src		\
					$(TOP)/hardware/vpu	\
					$(TOP)/hardware/vpu/include		
					


include $(BUILD_SHARED_LIBRARY)


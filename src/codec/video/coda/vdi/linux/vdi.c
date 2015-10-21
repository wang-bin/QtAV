//------------------------------------------------------------------------------
// File: vdi.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef	_KERNEL_
#include <linux/delay.h> 
#endif
#include <signal.h>		/* SIGIO */
#include <fcntl.h>		/* fcntl */
#include <pthread.h>
#include <sys/mman.h>		/* mmap */
#include <sys/ioctl.h>		/* fopen/fread */
#include <sys/errno.h>		/* fopen/fread */
#include <sys/types.h>
#include <sys/time.h>
#include "driver/vpu.h"
#include "../vdi.h"
#include "../vdi_osal.h"

#ifdef CNM_FPGA_PLATFORM		//this definition is only for chipsnmedia FPGA board env. so for SOC env of customers can be ignored.

#ifdef SUPPORT_128BIT_BUS
#define HPI_BUS_LEN 16	
#define HPI_BUS_LEN_ALIGN 15
#else
#define HPI_BUS_LEN 8
#define HPI_BUS_LEN_ALIGN 7
#endif


/*------------------------------------------------------------------------
ChipsnMedia HPI register definitions
------------------------------------------------------------------------*/
#define HPI_CHECK_STATUS			1
#define HPI_WAIT_TIME				0x100000
#define HPI_BASE					0x20030000
#define HPI_ADDR_CMD				(0x00<<2)
#define HPI_ADDR_STATUS				(0x01<<2)
#define HPI_ADDR_ADDR_H				(0x02<<2)
#define HPI_ADDR_ADDR_L				(0x03<<2)
#define HPI_ADDR_ADDR_M				(0x06<<2)
#define HPI_ADDR_DATA				(0x80<<2)

#ifdef SUPPORT_128BIT_BUS
#define HPI_CMD_WRITE_VALUE			((16 << 4) + 2)
#define HPI_CMD_READ_VALUE			((16 << 4) + 1)
#else
#define HPI_CMD_WRITE_VALUE			((8 << 4) + 2)
#define HPI_CMD_READ_VALUE			((8 << 4) + 1)
#endif

#define HPI_MAX_PKSIZE 256

// chipsnmedia clock generator in FPGA

#define	DEVICE0_ADDR_COMMAND		0x75
#define DEVICE0_ADDR_PARAM0			0x76
#define	DEVICE0_ADDR_PARAM1			0x77
#define	DEVICE1_ADDR_COMMAND		0x78
#define DEVICE1_ADDR_PARAM0			0x79
#define	DEVICE1_ADDR_PARAM1			0x7a
#define DEVICE_ADDR_SW_RESET		0x7b

#define ACLK_MAX					30
#define ACLK_MIN					16
#define CCLK_MAX					30
#define CCLK_MIN					16


static void hpi_init_fpga(int core_idx);
static int hpi_init(unsigned long core_idx, unsigned long dram_base);
static void hpi_release(unsigned long core_idx);
static void hpi_write_register(unsigned long core_idx, void * base, unsigned int addr, unsigned int data, pthread_mutex_t io_mutex);
static unsigned int hpi_read_register(unsigned long core_idx, void * base, unsigned int addr, pthread_mutex_t io_mutex);
static int hpi_write_memory(unsigned long core_idx,void * base, unsigned int addr, unsigned char *data, int len, int endian, pthread_mutex_t io_mutex);
static int hpi_read_memory(unsigned long core_idx, void * base, unsigned int addr, unsigned char *data, int len, int endian, pthread_mutex_t io_mutex);
static int hpi_hw_reset(void * base);
static int hpi_set_timing_opt(unsigned long core_idx, void * base, pthread_mutex_t io_mutex);
static int ics307m_set_clock_freg(void * base, int Device, int OutFreqMHz, int InFreqMHz);

static void * s_hpi_base;
static unsigned long s_dram_base;

static unsigned int hpi_read_reg_limit(unsigned long core_idx, unsigned int addr, unsigned int *data, pthread_mutex_t io_mutex);
static unsigned int hpi_write_reg_limit(unsigned long core_idx,unsigned int addr, unsigned int data, pthread_mutex_t io_mutex);
static unsigned int pci_read_reg(unsigned int addr);
static void pci_write_reg(unsigned int addr, unsigned int data);
static void pci_write_memory(unsigned int addr, unsigned char *buf, int size);
static void pci_read_memory(unsigned int addr, unsigned char *buf, int size);


#endif 

#ifdef CNM_FPGA_PLATFORM
//#define SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
//#define SUPPORT_INTERRUPT
#	define VPU_BIT_REG_SIZE		(0x4000*MAX_NUM_VPU_CORE)
#	define VPU_BIT_REG_BASE		0x10000000
#	define VDI_SRAM_BASE_ADDR	0x00
#	define VDI_SRAM_SIZE		0x20000
#	define VDI_SYSTEM_ENDIAN	VDI_BIG_ENDIAN
#	define VDI_DRAM_PHYSICAL_BASE	0x80000000
#	define VDI_DRAM_PHYSICAL_SIZE	(128*1024*1024)
#else
#	define SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#	define SUPPORT_INTERRUPT
#	define VPU_BIT_REG_SIZE	(0x4000*MAX_NUM_VPU_CORE)
#		define VDI_SRAM_BASE_ADDR	0x00000000	// if we can know the sram address in SOC directly for vdi layer. it is possible to set in vdi layer without allocation from driver
#		define VDI_SRAM_SIZE		0x20000		// FHD MAX size, 0x17D00  4K MAX size 0x34600
#	define VDI_SYSTEM_ENDIAN VDI_LITTLE_ENDIAN
#endif

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define VPU_CORE_BASE_OFFSET 0x4000
#endif

#define VPU_DEVICE_NAME "/dev/vpu"
#undef DMA_DEBUG 

typedef pthread_mutex_t	MUTEX_HANDLE;


typedef struct vpudrv_buffer_pool_t
{
	vpudrv_buffer_t vdb;
	int inuse;
} vpudrv_buffer_pool_t;

typedef struct  {
	unsigned long coreIdx;
	int vpu_fd;	
	vpu_instance_pool_t *pvip;
	int task_num;
	int clock_state;	
#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#else
	vpudrv_buffer_t vdb_video_memory;
#endif
	vpudrv_buffer_t vdb_register;
	vpu_buffer_t vpu_common_memory;
	vpudrv_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
	int vpu_buffer_pool_count;

#ifdef CNM_FPGA_PLATFORM	
	MUTEX_HANDLE io_mutex;
#endif
} vdi_info_t;

///////////////////////////////////////////
///////////////API for SWICH///////////////
///////////////////////////////////////////
static int dma_fd = -1;

static inline unsigned int readl(const volatile void *a)
{
        unsigned int ret = *(const volatile unsigned int *)a;
        asm("memb");
        return ret;
}

static inline void writel(unsigned int b, volatile void *a)
{
        *(volatile unsigned int *)a = b;
        asm("memb");
}

static inline unsigned char readb(const volatile void *a)
{
        unsigned char ret = *(const volatile unsigned char *)a;
        asm("memb");
        return ret;
}

static inline void writeb(unsigned char b, volatile void *a)
{
        *(volatile unsigned char *)a = b;
        asm("memb");
}


int dma_enable(void)
{
        dma_fd = open("/dev/dma_dev", O_RDWR);
        printf("dam_fd = %d\n", dma_fd);
        if (dma_fd < 0) {
                return -1;
        } else {
                return 0;
        }
}

/* 
 * IO 方式写显存
 * dst: 显存地址
 * src: 主存地址
 * len: 数据长度
 * 无返回值
 */
void io_write_video_mem(unsigned char * dst, unsigned char * src, int len)
{
        int i;
#if 0
        int head, mid, tail;
        unsigned long tmp = (unsigned long)dst;
        head = (tmp % 4) ? (4 - tmp % 4) : 0;
        tail = (tmp + len) % 4;
        mid = len - head - tail;

        for (i = 0; i < head; i++, src++, dst++) {
                writeb(*src, dst);
        }

        for (i = 0; i < mid; i += 4, src += 4, dst += 4) {
                writel(*(unsigned int*)src, dst);
        }

        for (i = 0; i < tail; i++, src++, dst++) {
                writeb(*src, dst);
        }
#else
        for (i = 0; i < len; i++, src++, dst++) {
	//        printf("addr=%#lx, data=%#lx\n",dst,*src);
                writeb(*src, dst);
       }

//        for (i = 0; i < len; i++) {
//                printf("io_write_video_mem addr=%#lx, data=%#lx\n",dst,*src);
//		dst++ = src++;
//        }

#endif
}

/* 
 * IO 方式读显存
 * dst: 主存地址
 * src: 显存地址
 * len: 数据长度
 * 无返回值
 */
void io_read_video_mem(unsigned char * dst, unsigned char * src, int len)
{
        int i;
#if 0
        int head, mid, tail;
        unsigned long tmp = (unsigned long)src;
        head = (tmp % 4) ? (4 - tmp % 4) : 0;
        tail = (tmp + len) % 4;
        mid = len - (head + tail);

        for (i = 0; i < head; i++, src++, dst++) {
                *dst = readb(src);
        }

        for (i = 0; i < mid; i += 4, src += 4, dst +=4) {
                *((unsigned int *)dst) = readl(src);
        }

        for (i = 0; i < tail; i++, src++, dst++) {
                *dst = readb(src);
        }
#else
        for (i = 0; i < len; i++, src++, dst++) {
                *dst = readb(src);
        }
#endif
}

void dma_copy_to_vmem(unsigned int dst, unsigned char* src, int len)
{
        struct dma_info *dma_info;
        dma_info = malloc(sizeof(struct dma_info));
//      printf("malloc dma_info: %p\n", dma_info);
        dma_info->vsrc          = (unsigned long)src;
        dma_info->vdst          = (unsigned long)dst;
        dma_info->size          = len;
        dma_info->direction     = DMAR;
#ifdef DMA_DEBUG
        printf("before dma_info.vsrc=%#lx ,dma_info.vdst=%#lx,dma_info.size=%#lx dma_info.direction=%#x\n",
                        dma_info->vsrc,dma_info->vdst,dma_info->size,dma_info->direction);
#endif
        ioctl(dma_fd, DMA_TRANSFER, dma_info);
        free(dma_info);
}

void dma_copy_from_vmem(unsigned char* dst, unsigned int src, int len)
{
        struct dma_info *dma_info;
        memset(dst, 0, len);
        dma_info = malloc(sizeof(struct dma_info));
//      printf("malloc dma_info: %p\n", dma_info);
        dma_info->vsrc          = (unsigned long)src;
        dma_info->vdst          = (unsigned long)dst;
        dma_info->size          = len;
        dma_info->direction     = DMAW;
#ifdef DMA_DEBUG
        printf("before dma_info.vsrc=%#lx ,dma_info.vdst=%#lx,dma_info.size=%#lx dma_info.direction=%#x\n",
                dma_info->vsrc,dma_info->vdst,dma_info->size,dma_info->direction);
#endif
        ioctl(dma_fd, DMA_TRANSFER, dma_info);
        free(dma_info);
}

void dma_copy_in_vmem(unsigned int dst, unsigned int src, int len)
{
        struct dma_info *dma_info;
        dma_info = malloc(sizeof(struct dma_info));
        dma_info->vsrc          = (unsigned long)src;
        dma_info->vdst          = (unsigned long)dst;
        dma_info->size          = len;
        dma_info->direction     = DMAV;
#ifdef DMA_DEBUG
        printf("before dma_info.vsrc=%#lx ,dma_info.vdst=%#lx,dma_info.size=%#lx dma_info.direction=%#x\n",
                      dma_info->vsrc,dma_info->vdst,dma_info->size,dma_info->direction);
#endif
        ioctl(dma_fd, DMA_TRANSFER, dma_info);
        free(dma_info);
}

///////////////////////////////////////////
/////////////////END///////////////////////
///////////////////////////////////////////


static vdi_info_t s_vdi_info[MAX_NUM_VPU_CORE];
static unsigned long s_vpu_reg_base;
static unsigned long s_vpu_reg_size = BIT_REG_SIZE;

static int swap_endian(unsigned char *data, int len, int endian);
static int allocate_common_memory(unsigned long coreIdx);


int vdi_probe(unsigned long coreIdx)
{
	int ret;

	ret = vdi_init(coreIdx);
#ifdef CNM_FPGA_PLATFORM
#else
	vdi_release(coreIdx);
#endif	

	return ret;
}

int vdi_init(unsigned long coreIdx)
{
	pthread_mutexattr_t vpu_mutexattr;
	pthread_mutexattr_t disp_mutexattr;
	vdi_info_t *vdi;
	int i=0;

	i = i;	
	if (coreIdx > MAX_NUM_VPU_CORE)
		return 0;

	vdi = &s_vdi_info[coreIdx];

	if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
	{
		vdi->task_num++;		
		return 0;
	}


	vdi->vpu_fd = open(VPU_DEVICE_NAME, O_RDWR);	// if this API supports VPU parallel processing using multi VPU. the driver should be made to open multiple times.
	if (vdi->vpu_fd < 0) {
		VLOG(ERR, "[VDI] Can't open vpu driver. permission would make the problem. try to change permission like chown user.iser /dev/vpu\n");		
		return -1;
	}

	memset(&vdi->vpu_buffer_pool, 0x00, sizeof(vpudrv_buffer_pool_t)*MAX_VPU_BUFFER_POOL);
	if (!vdi_get_instance_pool(coreIdx))
	{
		VLOG(INFO, "[VDI] fail to create shared info for saving context \n");		
		goto ERR_VDI_INIT;
	}

	if(!vdi->pvip->instance_pool_inited)
	{
		pthread_mutexattr_init(&vpu_mutexattr);
		pthread_mutexattr_setpshared(&vpu_mutexattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init((pthread_mutex_t *)vdi->pvip->vpu_mutex, &vpu_mutexattr);
		
		pthread_mutexattr_init(&disp_mutexattr);
		pthread_mutexattr_setpshared(&disp_mutexattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init((pthread_mutex_t *)vdi->pvip->vpu_disp_mutex, &disp_mutexattr);
	}


	if (vdi_lock(coreIdx) < 0)
	{
		VLOG(ERR, "[VDI] fail to handle lock function\n");
		goto ERR_VDI_INIT;
	}

#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#else
	if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO, &vdi->vdb_video_memory) < 0)
	{
		VLOG(ERR, "[VDI] fail to get video memory information\n");		
		goto ERR_VDI_INIT;
	}

#if 0
	if (REQUIRED_VPU_MEMORY_SIZE > vdi->vdb_video_memory.size)
	{
		VLOG(ERR, "[VDI] Warning : VPU memory will be overflow\n");
	}
#endif

#endif

	if (allocate_common_memory(coreIdx) < 0) 
	{
		VLOG(ERR, "[VDI] fail to get vpu common buffer from driver\n");		
		goto ERR_VDI_INIT;
	}


#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#else
	if (!vdi->pvip->instance_pool_inited)
		osal_memset(&vdi->pvip->vmem, 0x00, sizeof(video_mm_t));

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	if (vmem_init(&vdi->pvip->vmem, (unsigned long)vdi->vdb_video_memory.phys_addr + (vdi->pvip->vpu_common_buffer.size*MAX_NUM_VPU_CORE),
		vdi->vdb_video_memory.size - (vdi->pvip->vpu_common_buffer.size*MAX_NUM_VPU_CORE)) < 0)
#else
	if (vmem_init(&vdi->pvip->vmem, (unsigned long)vdi->vdb_video_memory.phys_addr + vdi->pvip->vpu_common_buffer.size,
		vdi->vdb_video_memory.size - vdi->pvip->vpu_common_buffer.size) < 0)
#endif
	{
		VLOG(ERR, "[VDI] fail to init vpu memory management logic\n");			
		goto ERR_VDI_INIT;
	}
#endif

	vdi->vdb_register.size = VPU_BIT_REG_SIZE;
	vdi->vdb_register.virt_addr = (unsigned long)mmap(NULL, vdi->vdb_register.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, 0);
	if ((void *)vdi->vdb_register.virt_addr == MAP_FAILED) 
	{
		VLOG(ERR, "[VDI] fail to map vpu registers \n");
		goto ERR_VDI_INIT;
	}

	if (vdi_read_register(coreIdx, BIT_CUR_PC) == 0) // if BIT processor is not running.
	{
		for (i=0; i<64; i++)
			vdi_write_register(coreIdx, (i*4) + 0x100, 0x0); 
	}
	vdi_set_clock_gate(coreIdx, 1);
	VLOG(INFO, "[VDI] map vdb_register coreIdx=%d, virtaddr=0x%lx, size=%d\n", coreIdx, vdi->vdb_register.virt_addr, vdi->vdb_register.size);

	vdi->coreIdx = coreIdx; 
	vdi->task_num++;	
	vdi_unlock(coreIdx);

	
	VLOG(INFO, "[VDI] success to init driver \n");	
        
	if(dma_enable() < 0){
                VLOG(ERR, "[VDI] fail to open swich dma dev\n");
                goto ERR_VDI_INIT;
        }
	
	return 0;

ERR_VDI_INIT:
	vdi_unlock(coreIdx);
	vdi_release(coreIdx);
	return -1;
}

int vdi_set_bit_firmware_to_pm(unsigned long coreIdx, const unsigned short *code)
{
	int i;
	vpu_bit_firmware_info_t bit_firmware_info;
	vdi_info_t *vdi = &s_vdi_info[coreIdx];

	if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return 0;	

	bit_firmware_info.size = sizeof(vpu_bit_firmware_info_t);	
	bit_firmware_info.core_idx = coreIdx;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER	
	bit_firmware_info.reg_base_offset = (coreIdx*VPU_CORE_BASE_OFFSET);
#else
	bit_firmware_info.reg_base_offset = 0;
#endif	
	for (i=0; i<512; i++) 
		bit_firmware_info.bit_code[i] = code[i];
	
	if (write(vdi->vpu_fd, &bit_firmware_info, bit_firmware_info.size) < 0)
	{
		VLOG(ERR, "[VDI] fail to vdi_set_bit_firmware core=%d\n", bit_firmware_info.core_idx);
		return -1;
	}
	
	return 0;
}

int vdi_release(unsigned long coreIdx)
{
	int i;
	vpudrv_buffer_t vdb;
	vdi_info_t *vdi = &s_vdi_info[coreIdx];

	if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return 0;

	
	if (vdi_lock(coreIdx) < 0)
	{
		VLOG(ERR, "[VDI] fail to handle lock function\n");
		return -1;
	}

	if (vdi->task_num > 1) // means that the opened instance remains 
	{
		vdi->task_num--;
		vdi_unlock(coreIdx);
		return 0;
	}
	
	munmap((void *)vdi->vdb_register.virt_addr, vdi->vdb_register.size);
	osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));
	vdb.size = 0;	
	// get common memory information to free virtual address
	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_common_memory.phys_addr >= vdi->vpu_buffer_pool[i].vdb.phys_addr &&
			vdi->vpu_common_memory.phys_addr < (vdi->vpu_buffer_pool[i].vdb.phys_addr + vdi->vpu_buffer_pool[i].vdb.size))
		{
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			vdb = vdi->vpu_buffer_pool[i].vdb;
			break;
		}
	}

	if (vdb.size > 0)
	{
#ifdef CNM_FPGA_PLATFORM

#else
		munmap((void *)vdb.virt_addr, vdb.size);
#endif
		memset(&vdi->vpu_common_memory, 0x00, sizeof(vpu_buffer_t));
	}

	vdi->task_num--;

	if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00)
	{
		close(vdi->vpu_fd);
		vdi->vpu_fd = -1;

#ifdef CNM_FPGA_PLATFORM
		pthread_mutex_destroy(&vdi->io_mutex);		
		hpi_release(coreIdx);
#endif
	}

	vdi_unlock(coreIdx);

	
	memset(vdi, 0x00, sizeof(vdi_info_t));
        
	if (dma_fd > 0) {
                close(dma_fd);
        }

	return 0;
}




int vdi_get_common_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
		return -1;

	osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

	return 0;
}


int allocate_common_memory(unsigned long coreIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	
	vpudrv_buffer_t vdb;
	int i;

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd==0x00)
		return -1;

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
	
	if (vdi->pvip->vpu_common_buffer.size == 0)
	{
		vdb.size = SIZE_COMMON*MAX_NUM_VPU_CORE;
#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
		if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_COMMON_MEMORY, &vdb) < 0)
		{
			VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vdb.size);
			return -1;
		}
#else
		vdb.phys_addr = vdi->vdb_video_memory.phys_addr; // set at the beginning of base address
		vdb.base =  vdi->vdb_video_memory.base;
#endif
		
#ifdef CNM_FPGA_PLATFORM
		vdb.virt_addr = (unsigned long)vdb.phys_addr;		
#else
		vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, virt_to_phys(vdb.base));
		if ((void *)vdb.virt_addr == MAP_FAILED) 
		{
			VLOG(ERR, "[VDI] fail to map common memory phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
			return -1;
		}
#endif

		VLOG(INFO, "[VDI] allocate_common_memory, physaddr=0x%x, virtaddr=0x%x\n", (int)vdb.phys_addr, (int)vdb.virt_addr);
		
		// convert os driver buffer type to vpu buffer type
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
		vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
		vdi->pvip->vpu_common_buffer.phys_addr = (unsigned long)(vdb.phys_addr + (coreIdx*SIZE_COMMON));
		vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base + (coreIdx*SIZE_COMMON));
		vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (coreIdx*SIZE_COMMON));
#else
		vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
		vdi->pvip->vpu_common_buffer.phys_addr = (unsigned long)(vdb.phys_addr);
		vdi->pvip->vpu_common_buffer.base = (unsigned long)(vdb.base);
		vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr);
#endif
		osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

	}	
	else
	{
		vdb.size = SIZE_COMMON*MAX_NUM_VPU_CORE;
		vdb.phys_addr = vdi->pvip->vpu_common_buffer.phys_addr; // set at the beginning of base address
		vdb.base =  vdi->pvip->vpu_common_buffer.base;			

#ifdef CNM_FPGA_PLATFORM
		vdb.virt_addr = (unsigned long)vdb.phys_addr;		
#else
		vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, virt_to_phys(vdb.base));
		if ((void *)vdb.virt_addr == MAP_FAILED) 
		{
			VLOG(ERR, "[VDI] fail to map common memory phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
			return -1;
		}		
#endif
		
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
		vdi->pvip->vpu_common_buffer.virt_addr = (unsigned long)(vdb.virt_addr + (coreIdx*SIZE_COMMON)); 
#else
		vdi->pvip->vpu_common_buffer.virt_addr = vdb.virt_addr; 
#endif
		osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer, sizeof(vpudrv_buffer_t));

		VLOG(INFO, "[VDI] allocate_common_memory, physaddr=0x%x, virtaddr=0x%x\n", (int)vdi->pvip->vpu_common_buffer.phys_addr, (int)vdi->pvip->vpu_common_buffer.virt_addr);		
	}

	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].inuse == 0)
		{
			vdi->vpu_buffer_pool[i].vdb = vdb;
			vdi->vpu_buffer_pool_count++;
			vdi->vpu_buffer_pool[i].inuse = 1;
			break;
		}
	}

	VLOG(INFO, "[VDI] vdi_get_common_memory physaddr=0x%x, size=%d, virtaddr=0x%x\n", (int)vdi->vpu_common_memory.phys_addr, (int)vdi->vpu_common_memory.size, (int)vdi->vpu_common_memory.virt_addr);
	

	return 0;
}



vpu_instance_pool_t *vdi_get_instance_pool(unsigned long coreIdx)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	
	if (coreIdx > MAX_NUM_VPU_CORE)
		return NULL;

	vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00 )
		return NULL;

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
	if (!vdi->pvip)
	{
		vdb.size = sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*2;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
		vdb.size  *= MAX_NUM_VPU_CORE;
#endif
		if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_INSTANCE_POOL, &vdb) < 0)
		{
			VLOG(ERR, "[VDI] fail to allocate get instance pool physical space=%d\n", (int)vdb.size);
			return NULL;
		}
		vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
		if ((void *)vdb.virt_addr == MAP_FAILED) 
		{
			VLOG(ERR, "[VDI] fail to map instance pool phyaddr=0x%x, size = %d\n", (int)vdb.phys_addr, (int)vdb.size);
			return NULL;
		}

		vdi->pvip = (vpu_instance_pool_t *)(vdb.virt_addr + (coreIdx*(sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE)*2)));

		vdi->pvip->vpu_mutex = (void *)((unsigned long)vdi->pvip + sizeof(vpu_instance_pool_t));	//change the pointer of vpu_mutex to at end pointer of vpu_instance_pool_t to assign at allocated position.
		vdi->pvip->vpu_disp_mutex = (void *)((unsigned long)vdi->pvip + sizeof(vpu_instance_pool_t) + sizeof(MUTEX_HANDLE));

		VLOG(INFO, "[VDI] instance pool physaddr=0x%#lx, virtaddr=0x%#lx, base=0x%#lx, size=%d\n", (int)vdb.phys_addr, (int)vdb.virt_addr, (int)vdb.base, (int)vdb.size);
	}

		
	return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(unsigned long coreIdx, unsigned long instIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	int inst_num;

	if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return -1;

	if (ioctl(vdi->vpu_fd, VDI_IOCTL_OPEN_INSTANCE, &instIdx) < 0)
	{
		VLOG(ERR, "[VDI] fail to deliver open instance num instIdx=%d\n", (int)instIdx);
		return -1;
	}

	if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_INSTANCE_NUM, &inst_num) < 0)
	{
		VLOG(ERR, "[VDI] fail to deliver open instance num instIdx=%d\n", (int)instIdx);
		return -1;
	}

	vdi->pvip->vpu_instance_num = inst_num;

	return 0;
}

int vdi_close_instance(unsigned long coreIdx, unsigned long instIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	int inst_num;

	if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return -1;

	if (ioctl(vdi->vpu_fd, VDI_IOCTL_CLOSE_INSTANCE, &instIdx) < 0)
	{
		VLOG(ERR, "[VDI] fail to deliver open instance num instIdx=%d\n", (int)instIdx);
		return -1;
	}

	if (ioctl(vdi->vpu_fd, VDI_IOCTL_GET_INSTANCE_NUM, &inst_num) < 0)
	{
		VLOG(ERR, "[VDI] fail to deliver open instance num instIdx=%d\n", (int)instIdx);
		return -1;
	}

	vdi->pvip->vpu_instance_num = inst_num;

	return 0;
}

int vdi_get_instance_num(unsigned long coreIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	

	if(!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return -1;

	return vdi->pvip->vpu_instance_num;
}

int vdi_hw_reset(unsigned long coreIdx) // DEVICE_ADDR_SW_RESET
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	

	if(!vdi || !vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00)
		return -1;
	
#ifdef CNM_FPGA_PLATFORM
	hpi_hw_reset((void *)vdi->vdb_register.virt_addr);
	return 1;
#else
	return ioctl(vdi->vpu_fd, VDI_IOCTL_RESET, 0);
#endif
		
}

int vdi_lock(unsigned long coreIdx)
{
#if 1
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	const int MUTEX_TIMEOUT = 0x7fffffff;

	while(1)
	{
		int _ret, i;
		for(i = 0; (_ret = pthread_mutex_trylock((pthread_mutex_t *)vdi->pvip->vpu_mutex)) != 0 && i < MUTEX_TIMEOUT; i++) 
		{
			if(i == 0) 
				VLOG(ERR, "vdi_lock: mutex is already locked - try again\n");				
#ifdef	_KERNEL_
			udelay(1*1000);
#else
			usleep(1*1000);
#endif // _KERNEL_
			if (i == 1000) // check whether vpu is really busy 
			{
				if (vdi_read_register(coreIdx, BIT_BUSY_FLAG) == 0)
					break;
			}
				
			if (i > VPU_BUSY_CHECK_TIMEOUT)  
			{
				if (vdi_read_register(coreIdx, BIT_BUSY_FLAG) == 1)
				{
					vdi_write_register(coreIdx, BIT_BIT_STREAM_PARAM, (1 << 2));
					vdi_write_register(coreIdx, BIT_INT_REASON, 0);
					vdi_write_register(coreIdx, BIT_INT_CLEAR, 1);					
				}
				break;
			}
		}
		
		if(_ret == 0) 
			break;
		
		VLOG(ERR, "vdi_lock: can't get lock - force to unlock and clear pendingInst[%d:%s]\n", _ret, strerror(_ret));
		vdi_unlock(coreIdx);
		vdi->pvip->pendingInst = NULL;		
		
	}		

	return 0;
#endif
#if 0
        const int MUTEX_TIMEOUT = 10;   // ms
        struct timespec ts;
        vdi_info_t *vdi = &s_vdi_info[coreIdx]; 

        ts.tv_sec = time(NULL) + MUTEX_TIMEOUT;
        ts.tv_nsec = 0;
         
        if (pthread_mutex_timedlock((pthread_mutex_t *)vdi->pvip->vpu_mutex, &ts) == ETIMEDOUT)
                return -1;
        
        return 0;
#endif
}
void vdi_unlock(unsigned long coreIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];

	pthread_mutex_unlock((pthread_mutex_t *)vdi->pvip->vpu_mutex);
}

int vdi_disp_lock(unsigned long coreIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	const int MUTEX_TIMEOUT = 5000;  // ms
		
	while(1)
    {
        int _ret, i;
        for(i = 0; (_ret = pthread_mutex_trylock((pthread_mutex_t *)vdi->pvip->vpu_disp_mutex)) != 0 && i < MUTEX_TIMEOUT; i++)
        {
            if(i == 0)
				VLOG(ERR, "vdi_disp_lock: mutex is already locked - try again\n");                
#ifdef	_KERNEL_
			udelay(1*1000);
#else
			usleep(1*1000);
#endif // _KERNEL_
        }

        if(_ret == 0)
            break;

		VLOG(ERR, "vdi_disp_lock: can't get lock - force to unlock. [%d:%s]\n", _ret, strerror(_ret));
        vdi_disp_unlock(coreIdx);
    }

    return 0;
}

void vdi_disp_unlock(unsigned long coreIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	
	pthread_mutex_unlock((pthread_mutex_t *)vdi->pvip->vpu_disp_mutex);
}



void vdi_write_register(unsigned long coreIdx, unsigned int addr, unsigned int data)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	unsigned int *reg_addr = NULL;
    reg_addr = reg_addr;
	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return;

#ifdef CNM_FPGA_PLATFORM
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	hpi_write_register(coreIdx, (void *)vdi->vdb_register.virt_addr, VPU_BIT_REG_BASE + (coreIdx*VPU_CORE_BASE_OFFSET) + addr, data, vdi->io_mutex);	
#else
	hpi_write_register(coreIdx, (void *)vdi->vdb_register.virt_addr, VPU_BIT_REG_BASE+ addr, data, vdi->io_mutex);
#endif	
#else
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	reg_addr = (unsigned long *)(addr + (void *)vdi->vdb_register.virt_addr + (coreIdx*VPU_CORE_BASE_OFFSET));
#else
	reg_addr = (unsigned long *)(addr + (void *)vdi->vdb_register.virt_addr);
//	reg_addr = (unsigned int *)(addr + s_vpu_reg_base);
#endif
	*(volatile unsigned int *)reg_addr = data;
//	printf("VpuWriteReg addr=%#x,data=%#x\n",addr,data);	
#endif
}

unsigned int vdi_read_register(unsigned long coreIdx, unsigned int addr)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	unsigned int *reg_addr = NULL;
    reg_addr = reg_addr;

#ifdef CNM_FPGA_PLATFORM
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	return hpi_read_register(coreIdx, (void *)vdi->vdb_register.virt_addr, VPU_BIT_REG_BASE + (coreIdx*VPU_CORE_BASE_OFFSET) + addr, vdi->io_mutex);	
#else
	return hpi_read_register(coreIdx, (void *)vdi->vdb_register.virt_addr, VPU_BIT_REG_BASE+addr, vdi->io_mutex);
#endif

#else

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	reg_addr = (unsigned long *)(addr + (void *)vdi->vdb_register.virt_addr + (coreIdx*VPU_CORE_BASE_OFFSET));	
#else
//	printf("addr=%#lx,s_vpu_reg_base=%#lx\n",addr, s_vpu_reg_base);
	reg_addr = (unsigned long *)(addr + (void *)vdi->vdb_register.virt_addr);
//	reg_addr = (unsigned int *)(addr + s_vpu_reg_base);
//	printf("data=%#x\n",*(volatile unsigned int *)reg_addr);
#endif
	return *(volatile unsigned int *)reg_addr;
#endif
}

int vdi_write_memory(unsigned long coreIdx, unsigned int addr, unsigned char *data, int len, int endian)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	unsigned long offset = 0;
	int i;
//    offset = offset;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	coreIdx = 0;
#endif
	vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].inuse == 1)
		{
			vdb = vdi->vpu_buffer_pool[i].vdb;
			if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
				break;
		}
	}

	if (!vdb.size) {
		VLOG(ERR, "address 0x%08x is not mapped address!!!\n", (int)addr);
		return -1;
	}

	offset = addr - (unsigned long)vdb.phys_addr;

	swap_endian(data, len, endian);
//	osal_memcpy((void *)((unsigned long)vdb.virt_addr+offset), data, len);	
#ifndef USE_DMA
	io_write_video_mem((const void *)((unsigned long)vdb.virt_addr+offset), data, len);
#else
	if(len < 0x200)
		io_write_video_mem((const void *)((unsigned long)vdb.virt_addr+offset), data, len);
	else
        	dma_copy_to_vmem(addr, data, len);
#endif

	return len;
}

int vdi_read_memory(unsigned long coreIdx, unsigned int addr, unsigned char *data, int len, int endian)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	unsigned long offset = 0;
	int i = 0;
    offset = offset;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	coreIdx = 0;
#endif
	vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].inuse == 1)
		{
			vdb = vdi->vpu_buffer_pool[i].vdb;
			if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
				break;		
		}
	}

	if (!vdb.size)
		return -1;

	offset = addr - (unsigned long)vdb.phys_addr;	

//	osal_memcpy(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
#ifndef USE_DMA
	io_read_video_mem(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
#else
	if(len < 0x200)
		io_read_video_mem(data, (const void *)((unsigned long)vdb.virt_addr+offset), len);
	else
        	dma_copy_from_vmem(data, addr, len);
#endif

	swap_endian(data, len,  endian);

	return len;
}


int vdi_allocate_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#else
	unsigned long offset;
#endif
	vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	coreIdx = 0;
#endif
	
	vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;	

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	vdb.size = vb->size;
#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	if (ioctl(vdi->vpu_fd, VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0)
	{
		VLOG(ERR, "[VDI] fail to vdi_allocate_dma_memory size=%d\n", vb->size);		
		return -1;
	}
#else
	vdb.phys_addr = (unsigned long long)vmem_alloc(&vdi->pvip->vmem, vdb.size, 0);

	if (vdb.phys_addr == (unsigned long)-1)
		return -1; // not enough memory
	
	offset = (unsigned long)(vdb.phys_addr - vdi->vdb_video_memory.phys_addr);
	vdb.base = (unsigned long )vdi->vdb_video_memory.base + offset;
#endif
	
	vb->phys_addr = (unsigned long)vdb.phys_addr;
	vb->base = (unsigned long)vdb.base;
	
#ifdef CNM_FPGA_PLATFORM	
	vb->virt_addr = (unsigned long)vb->phys_addr;
#else
	//map to virtual address
	vdb.virt_addr = (unsigned long)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE,
		MAP_SHARED, vdi->vpu_fd, virt_to_phys(vdb.base));
	if ((void *)vdb.virt_addr == MAP_FAILED) 
	{
		memset(vb, 0x00, sizeof(vpu_buffer_t));
		return -1;
	}
	vb->virt_addr = vdb.virt_addr;
#endif

	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].inuse == 0)
		{
			vdi->vpu_buffer_pool[i].vdb = vdb;
			vdi->vpu_buffer_pool_count++;
			vdi->vpu_buffer_pool[i].inuse = 1;
			break;
		}
	}

	VLOG(INFO, "[VDI] vdi_allocate_dma_memory, physaddr=0x%p, virtaddr=0x%p, size=%d\n", vb->phys_addr, vb->virt_addr, vb->size);
	return 0;
}


int vdi_attach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;	
#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
#else
	unsigned long offset;
#endif
	vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	coreIdx = 0;
#endif

	vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;	
	
	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	vdb.size = vb->size;
	vdb.phys_addr = vb->phys_addr;
#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	vdb.base = vb->base;
#else
 	offset = (unsigned long)(vdb.phys_addr - vdi->vdb_video_memory.phys_addr);
 	vdb.base = (unsigned long )vdi->vdb_video_memory.base + offset;
#endif
	
	vdb.virt_addr = vb->phys_addr;

	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
		{
			vdi->vpu_buffer_pool[i].vdb = vdb;
			vdi->vpu_buffer_pool[i].inuse = 1;			
			break;
		}
		else
		{
			if (vdi->vpu_buffer_pool[i].inuse == 0)
			{
				vdi->vpu_buffer_pool[i].vdb = vdb;
				vdi->vpu_buffer_pool_count++;
				vdi->vpu_buffer_pool[i].inuse = 1;
				break;
			}
		}		
	}
	

	return 0;
}

int vdi_dettach_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
	
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	coreIdx = 0;
#endif

	vdi = &s_vdi_info[coreIdx];

	if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;
	
	if (vb->size == 0)
		return -1;
	
	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
		{
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			break;
		}
	}

	return 0;
}

void vdi_free_dma_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
	vpudrv_buffer_t vdb;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	coreIdx = 0;
#endif
	
	vdi = &s_vdi_info[coreIdx];

	if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return ;
	
	if (vb->size == 0)
		return ;

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
	{
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr)
		{
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			vdb = vdi->vpu_buffer_pool[i].vdb;
			break;
		}
	}

	if (!vdb.size)
	{
		VLOG(ERR, "[VDI] invalid buffer to free address = 0x%x\n", (int)vdb.virt_addr);
		return ;
	}

#ifdef SUPPORT_ALLOCATE_MEMORY_FROM_DRIVER
	ioctl(vdi->vpu_fd, VDI_IOCTL_FREE_PHYSICALMEMORY, &vdb);
#else
	vmem_free(&vdi->pvip->vmem, (unsigned long)vdb.phys_addr, 0);
#endif

	
#ifdef CNM_FPGA_PLATFORM	
#else
	if (munmap((void *)vdb.virt_addr, vdb.size) != 0)
	{
		VLOG(ERR, "[VDI] fail to vdi_free_dma_memory virtial address = 0x%x\n", (int)vdb.virt_addr);					
	}
#endif
	osal_memset(vb, 0, sizeof(vpu_buffer_t));	
}

int vdi_get_sram_memory(unsigned long coreIdx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	vpudrv_buffer_t vdb;

	if(!vb || !vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;

	

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	if (VDI_SRAM_SIZE > 0)	// if we can know the sram address directly in vdi layer, we use it first for sdram address
	{
#ifdef CNM_FPGA_PLATFORM
		vb->phys_addr = VDI_SRAM_BASE_ADDR;		// CNM FPGA platform has different SRAM per core
#else
		vb->phys_addr = VDI_SRAM_BASE_ADDR+(coreIdx*VDI_SRAM_SIZE);		
#endif
		vb->size = VDI_SRAM_SIZE;

		return 0;
	}

	return 0;
}


int vdi_set_clock_gate(unsigned long coreIdx, int enable)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	int ret;
	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;

	vdi->clock_state = enable;
	ret = ioctl(vdi->vpu_fd, VDI_IOCTL_SET_CLOCK_GATE, &enable);	
	
	return ret;
}

int vdi_get_clock_gate(unsigned long coreIdx)
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	int ret;
	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;

	ret = vdi->clock_state;
	return ret;
}



int vdi_wait_bus_busy(unsigned long coreIdx, int timeout, unsigned int gdi_busy_flag)
{
	Int64 elapse, cur;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	gettimeofday(&tv, NULL);
	elapse = tv.tv_sec*1000 + tv.tv_usec/1000;

	while(1)
	{
		if (vdi_read_register(coreIdx, gdi_busy_flag) == 0x77)
			break;

		gettimeofday(&tv, NULL);
		cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		if ((cur - elapse) > timeout)
		{
			VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, PC=0x%x\n", vdi_read_register(coreIdx, 0x018));
			return -1;
		}
#ifdef	_KERNEL_	//do not use in real system. use SUPPORT_INTERRUPT;
		udelay(1*1000);
#else
		usleep(1*1000);
#endif // _KERNEL_		
	}
	return 0;

}


int vdi_wait_vpu_busy(unsigned long coreIdx, int timeout, unsigned int addr_bit_busy_flag)
{
	
	Int64 elapse, cur;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	gettimeofday(&tv, NULL);
	elapse = tv.tv_sec*1000 + tv.tv_usec/1000;

	while(1)
	{
		if (vdi_read_register(coreIdx, addr_bit_busy_flag) == 0)
			break;

		gettimeofday(&tv, NULL);
		cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		if ((cur - elapse) > timeout)
		{
			VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, PC=0x%x\n", vdi_read_register(coreIdx, 0x018));
			return -1;
		}
#ifdef	_KERNEL_	//do not use in real system. use SUPPORT_INTERRUPT;
		udelay(1*1000);
#else
		usleep(1*1000);
#endif // _KERNEL_		
	}
	return 0;

}


int vdi_wait_interrupt(unsigned long coreIdx, int timeout, unsigned int addr_bit_int_reason)
{
#ifdef SUPPORT_INTERRUPT
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	int ret;
//printf("enter kernel wait interrupt\n");
	ret = ioctl(vdi->vpu_fd, VDI_IOCTL_WAIT_INTERRUPT, timeout);
	if (ret != 0)
		ret = -1;
	return ret;	
#else
	Int64 elapse, cur;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	gettimeofday(&tv, NULL);
	elapse = tv.tv_sec*1000 + tv.tv_usec/1000;

	while(1)
	{
		if (vdi_read_register(coreIdx, addr_bit_int_reason))
			break;

		gettimeofday(&tv, NULL);
		cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		if ((cur - elapse) > timeout)
		{
			return -1;
		}
#ifdef	_KERNEL_	//do not use in real system. use SUPPORT_INTERRUPT;
		udelay(1*1000);
#else
		usleep(1*1000);
#endif // _KERNEL_		
	}
	return 0;
#endif
}




static int read_pinfo_buffer(int coreIdx, int addr)
{
	int ack;
	int rdata;
#define VDI_LOG_GDI_PINFO_ADDR  (0x1068)
#define VDI_LOG_GDI_PINFO_REQ   (0x1060)
#define VDI_LOG_GDI_PINFO_ACK   (0x1064)
#define VDI_LOG_GDI_PINFO_DATA  (0x106c)
	//------------------------------------------
	// read pinfo - indirect read
	// 1. set read addr     (GDI_PINFO_ADDR)
	// 2. send req          (GDI_PINFO_REQ)
	// 3. wait until ack==1 (GDI_PINFO_ACK)
	// 4. read data         (GDI_PINFO_DATA)
	//------------------------------------------
	vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_ADDR, addr);
	vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_REQ, 1);

	ack = 0;
	while (ack == 0)
	{
		ack = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_ACK);
	}

	rdata = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_DATA);

	//printf("[READ PINFO] ADDR[%x], DATA[%x]", addr, rdata);
	return rdata;
}


static void printf_gdi_info(int coreIdx, int num)
{
	int i;
	int bus_info_addr;
	int tmp;
	
	VLOG(INFO, "\n**GDI information for GDI_10\n");	

	for (i=0; i < num; i++)
	{
		VLOG(INFO, "index = %02d", i);
#define VDI_LOG_GDI_INFO_CONTROL 0x1400
		bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i*0x14;

		tmp = read_pinfo_buffer(coreIdx, bus_info_addr);	//TiledEn<<20 ,GdiFormat<<17,IntlvCbCr,<<16 GdiYuvBufStride
		VLOG(INFO, " control = 0x%08x", tmp);

		bus_info_addr += 4;
		tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
		VLOG(INFO, " pic_size = 0x%08x", tmp);

		bus_info_addr += 4;
		tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
		VLOG(INFO, " y-top = 0x%08x", tmp);
		
		bus_info_addr += 4;
		tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
		VLOG(INFO, " cb-top = 0x%08x", tmp);
		

		bus_info_addr += 4;
		tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
		VLOG(INFO, " cr-top = 0x%08x", tmp);		
		VLOG(INFO, "\n");				
	}
}


void vdi_log(unsigned long coreIdx, int cmd, int step)
{
	// BIT_RUN command
	enum {
		SEQ_INIT = 1,
		SEQ_END = 2,
		PIC_RUN = 3,
		SET_FRAME_BUF = 4,
		ENCODE_HEADER = 5,
		ENC_PARA_SET = 6,
		DEC_PARA_SET = 7,
		DEC_BUF_FLUSH = 8,
		RC_CHANGE_PARAMETER	= 9,
		VPU_SLEEP = 10,
		VPU_WAKE = 11,
		ENC_ROI_INIT = 12,
		FIRMWARE_GET = 0xf,
		VPU_RESET = 0x10,
	};

	int i;

	switch(cmd)
	{
	case SEQ_INIT:
		if (step == 1) {
			VLOG(INFO, "\n**SEQ_INIT start\n");		
		}
		else if (step == 2)	{
			VLOG(INFO, "\n**SEQ_INIT timeout\n");		
		}
		else {
			VLOG(INFO, "\n**SEQ_INIT end \n");					
		}
		break;
	case SEQ_END:
		if (step == 1) {
			VLOG(INFO, "\n**SEQ_END start\n");
		}
		else if (step == 2) {
			VLOG(INFO, "\n**SEQ_END timeout\n");					
		}
		else {
			VLOG(INFO, "\n**SEQ_END end\n");					
		}
		break;
	case PIC_RUN:
		if (step == 1) {
			VLOG(INFO, "\n**PIC_RUN start\n");			
		}
		else if (step == 2) {
			VLOG(INFO, "\n**PIC_RUN timeout\n");		
		}
		else  {
			VLOG(INFO, "\n**PIC_RUN end\n");	
		}
		break;
	case SET_FRAME_BUF:
		if (step == 1) {
			VLOG(INFO, "\n**SET_FRAME_BUF start\n");	
		}
		else if (step == 2) {
			VLOG(INFO, "\n**SET_FRAME_BUF timeout\n");			
		}
		else  {
			VLOG(INFO, "\n**SET_FRAME_BUF end\n");					
		}
		break;
	case ENCODE_HEADER:
		if (step == 1) {
			VLOG(INFO, "\n**ENCODE_HEADER start\n");	
		}
		else if (step == 2) {
			VLOG(INFO, "\n**ENCODE_HEADER timeout\n");			
		}
		else  {
			VLOG(INFO, "\n**ENCODE_HEADER end\n");					
		}
		break;
	case RC_CHANGE_PARAMETER:
		if (step == 1) {
			VLOG(INFO, "\n**RC_CHANGE_PARAMETER start\n");			
		}
		else if (step == 2) {
			VLOG(INFO, "\n**RC_CHANGE_PARAMETER timeout\n");			
		}
		else {
			VLOG(INFO, "\n**RC_CHANGE_PARAMETER end\n");					
		}
		break;

	case DEC_BUF_FLUSH:
		if (step == 1) {
			VLOG(INFO, "\n**DEC_BUF_FLUSH start\n");			
		}
		else if (step == 2) {
			VLOG(INFO, "\n**DEC_BUF_FLUSH timeout\n");
		}
		else {
			VLOG(INFO, "\n**DEC_BUF_FLUSH end ");
		}
		break;
	case FIRMWARE_GET:
		if (step == 1) {
			VLOG(INFO, "\n**FIRMWARE_GET start\n");
		}
		else if (step == 2)  {
			VLOG(INFO, "\n**FIRMWARE_GET timeout\n");
		}
		else {
			VLOG(INFO, "\n**FIRMWARE_GET end\n");		
		}
		break;
	case VPU_RESET:
		if (step == 1) {
			VLOG(INFO, "\n**VPU_RESET start\n");
		}
		else if (step == 2) {
			VLOG(INFO, "\n**VPU_RESET timeout\n");
		}
		else  {
			VLOG(INFO, "\n**VPU_RESET end\n");
		}
		break;
	case ENC_PARA_SET:
		if (step == 1)	// 
			VLOG(INFO, "\n**ENC_PARA_SET start\n");
		else if (step == 2)
			VLOG(INFO, "\n**ENC_PARA_SET timeout\n");
		else
			VLOG(INFO, "\n**ENC_PARA_SET end\n");
		break;
	case DEC_PARA_SET:
		if (step == 1)	// 
			VLOG(INFO, "\n**DEC_PARA_SET start\n");
		else if (step == 2)
			VLOG(INFO, "\n**DEC_PARA_SET timeout\n");
		else
			VLOG(INFO, "\n**DEC_PARA_SET end\n");
		break;
	default:
		if (step == 1) {
			VLOG(INFO, "\n**ANY CMD start\n");
		}
		else if (step == 2) {
			VLOG(INFO, "\n**ANY CMD timeout\n");
		}
		else {
			VLOG(INFO, "\n**ANY CMD end\n");
		}
		break;
	}

	for (i=0; i<0x200; i=i+16)
	{
		VLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
			vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
			vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
	}
	if ((cmd == PIC_RUN && step== 0) || cmd == VPU_RESET)
	{
		printf_gdi_info(coreIdx, 32);


#define VDI_LOG_MBC_BUSY 0x0440
#define VDI_LOG_MC_BASE	 0x0C00
#define VDI_LOG_MC_BUSY	 0x0C04
#define VDI_LOG_GDI_BUS_STATUS (0x10F4)

#define VDI_LOG_ROT_SRC_IDX	 (0x400 + 0x10C)
#define VDI_LOG_ROT_DST_IDX	 (0x400 + 0x110)
		VLOG(INFO, "MBC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MBC_BUSY));
		VLOG(INFO, "MC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MC_BUSY));
		VLOG(INFO, "MC_MB_XY_DONE=(y:%d, x:%d)\n", (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 20) & 0x3F, (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 26) & 0x3F);
		VLOG(INFO, "GDI_BUS_STATUS = %x\n", vdi_read_register(coreIdx, VDI_LOG_GDI_BUS_STATUS));

		VLOG(INFO, "ROT_SRC_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_SRC_IDX));
		VLOG(INFO, "ROT_DST_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_DST_IDX));

		VLOG(INFO, "P_MC_PIC_INDEX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x200));
		VLOG(INFO, "P_MC_PIC_INDEX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x20c));
		VLOG(INFO, "P_MC_PIC_INDEX_2 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x218));
		VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x230));
		VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x23C));
		VLOG(INFO, "P_MC_PIC_INDEX_4 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x248));
		VLOG(INFO, "P_MC_PIC_INDEX_5 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x254));
		VLOG(INFO, "P_MC_PIC_INDEX_6 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x260));
		VLOG(INFO, "P_MC_PIC_INDEX_7 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x26C));
		VLOG(INFO, "P_MC_PIC_INDEX_8 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x278));
		VLOG(INFO, "P_MC_PIC_INDEX_9 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x284));
		VLOG(INFO, "P_MC_PIC_INDEX_a = %x\n", vdi_read_register(coreIdx, MC_BASE+0x290));
		VLOG(INFO, "P_MC_PIC_INDEX_b = %x\n", vdi_read_register(coreIdx, MC_BASE+0x29C));
		VLOG(INFO, "P_MC_PIC_INDEX_c = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2A8));
		VLOG(INFO, "P_MC_PIC_INDEX_d = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2B4));

		VLOG(INFO, "P_MC_PICIDX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x028));
		VLOG(INFO, "P_MC_PICIDX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x02C));

	}
}


int swap_endian(unsigned char *data, int len, int endian)
{
	unsigned int *p;
	unsigned int v1, v2, v3;
	int i;
	const int sys_endian = VDI_SYSTEM_ENDIAN;
	int swap = 0;
	p = (unsigned int *)data;

	if(endian == sys_endian)
		swap = 0;	
	else
		swap = 1;

	if (swap)
	{		
		if (endian == VDI_LITTLE_ENDIAN ||
			endian == VDI_BIG_ENDIAN) {
				for (i=0; i<len/4; i+=2)
				{
					v1 = p[i];
					v2  = ( v1 >> 24) & 0xFF;
					v2 |= ((v1 >> 16) & 0xFF) <<  8;
					v2 |= ((v1 >>  8) & 0xFF) << 16;
					v2 |= ((v1 >>  0) & 0xFF) << 24;
					v3 =  v2;
					v1  = p[i+1];
					v2  = ( v1 >> 24) & 0xFF;
					v2 |= ((v1 >> 16) & 0xFF) <<  8;
					v2 |= ((v1 >>  8) & 0xFF) << 16;
					v2 |= ((v1 >>  0) & 0xFF) << 24;
					p[i]   =  v2;
					p[i+1] = v3;
				}
		} 
		else
		{
			int swap4byte = 0;
			if (endian == VDI_32BIT_LITTLE_ENDIAN)
			{
				if (sys_endian == VDI_BIG_ENDIAN)
				{
					swap = 1;
					swap4byte = 0;
				}
				else if (sys_endian == VDI_32BIT_BIG_ENDIAN)
				{
					swap = 1;
					swap4byte = 1;
				}	
				else if (sys_endian == VDI_LITTLE_ENDIAN)
				{
					swap = 0;
					swap4byte = 1;
				}				
			}
			else	// VDI_32BIT_BIG_ENDIAN
			{
				if (sys_endian == VDI_LITTLE_ENDIAN)
				{
					swap = 1;
					swap4byte = 0;
				}
				else if (sys_endian == VDI_32BIT_LITTLE_ENDIAN)
				{
					swap = 1;
					swap4byte = 1;
				}
				else if (sys_endian == VDI_BIG_ENDIAN)
				{
					swap = 0;
					swap4byte = 1;
				}
			}
			if (swap) {
				for (i=0; i<len/4; i++) {
					v1 = p[i];
					v2  = ( v1 >> 24) & 0xFF;
					v2 |= ((v1 >> 16) & 0xFF) <<  8;
					v2 |= ((v1 >>  8) & 0xFF) << 16;
					v2 |= ((v1 >>  0) & 0xFF) << 24;
					p[i] = v2;
				}
			}
			if (swap4byte) {
				for (i=0; i<len/4; i+=2) {
					v1 = p[i];
					v2 = p[i+1];
					p[i]   = v2;
					p[i+1] = v1;
				}
			}
		}
	}

	return swap;
}





#ifdef CNM_FPGA_PLATFORM

int vdi_set_timing_opt(unsigned long coreIdx )
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];
	//    int         id;

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;


	return hpi_set_timing_opt(coreIdx, (void *)vdi->vdb_register.virt_addr, vdi->io_mutex);
}

int vdi_set_clock_freg(unsigned long coreIdx, int Device, int OutFreqMHz, int InFreqMHz )
{
	vdi_info_t *vdi = &s_vdi_info[coreIdx];

	if(!vdi || vdi->vpu_fd==-1 || vdi->vpu_fd == 0x00)
		return -1;

	return ics307m_set_clock_freg((void *)vdi->vdb_register.virt_addr, Device, OutFreqMHz, InFreqMHz);
}


int hpi_init(unsigned long core_idx, unsigned long dram_base)
{
    if (core_idx>MAX_NUM_VPU_CORE)
        return -1;

	s_dram_base = dram_base;

	return 1;
}

void hpi_init_fpga(int core_idx)
{
	// init FPGA
	int aclk_freq, cclk_freq;


	vdi_hw_reset(core_idx);
	usleep(500*1000);
	vdi_set_timing_opt(core_idx);
	
	// Clock Default
	aclk_freq		= 30;
	cclk_freq		= 30;

	printf("Set default ACLK to %d\n", aclk_freq);
	vdi_set_clock_freg(core_idx, 0, aclk_freq, 10);	// ACLK	
	printf("Set default CCLK to %d\n", cclk_freq);
	vdi_set_clock_freg(core_idx, 1, cclk_freq, 10);	// CCLK

	usleep(500*1000);
}

void hpi_release(unsigned long core_idx)
{
}

void hpi_write_register(unsigned long core_idx, void * base, unsigned int addr, unsigned int data, pthread_mutex_t io_mutex)
{
	int status;

	if (pthread_mutex_lock(&io_mutex) < 0)
		return;
		
	
	s_hpi_base = base;

	pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_DATA, ((data >> 16) & 0xFFFF));
	pci_write_reg(HPI_ADDR_DATA + 4, (data & 0xFFFF));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_WRITE_VALUE);

	
	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = (status>>1) & 1;
	} while (status == 0);

	pthread_mutex_unlock(&io_mutex);	
}



unsigned int hpi_read_register(unsigned long core_idx, void * base, unsigned int addr, pthread_mutex_t io_mutex)
{
	int status;
	unsigned int data;

	if (pthread_mutex_lock(&io_mutex) < 0)
		return -1;

	s_hpi_base = base;
	
	pci_write_reg(HPI_ADDR_ADDR_H, ((addr >> 16)&0xffff));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));
	
	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_READ_VALUE);

	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = status & 1;
	} while (status == 0);


	data = pci_read_reg(HPI_ADDR_DATA) << 16;
	data |= pci_read_reg(HPI_ADDR_DATA + 4);

	pthread_mutex_unlock(&io_mutex);	

	return data;
}


int hpi_write_memory(unsigned long core_idx, void * base, unsigned int addr, unsigned char *data, int len, int endian, pthread_mutex_t io_mutex)
{	
	unsigned char *pBuf;
	unsigned char lsBuf[HPI_BUS_LEN];
	int lsOffset;	

	
	if (addr < s_dram_base) {
		fprintf(stderr, "[HPI] invalid address base address is 0x%lu\n", s_dram_base);
		return 0;
	}

	if (pthread_mutex_lock(&io_mutex) < 0)
		return 0;
	
	if (len==0) {
		pthread_mutex_unlock(&io_mutex);
		return 0;
	}
	


	addr = addr - s_dram_base;
	s_hpi_base = base; 

	lsOffset = addr - (addr/HPI_BUS_LEN)*HPI_BUS_LEN;
	if (lsOffset)
	{
		pci_read_memory((addr/HPI_BUS_LEN)*HPI_BUS_LEN, lsBuf, HPI_BUS_LEN);
        swap_endian(lsBuf, HPI_BUS_LEN, endian);	
		pBuf = (unsigned char *)malloc((len+lsOffset+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
		if (pBuf)
		{
			memset(pBuf, 0x00, (len+lsOffset+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
			memcpy(pBuf, lsBuf, HPI_BUS_LEN);
			memcpy(pBuf+lsOffset, data, len);
			swap_endian(pBuf, ((len+lsOffset+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN), endian);	
			pci_write_memory((addr/HPI_BUS_LEN)*HPI_BUS_LEN, (unsigned char *)pBuf, ((len+lsOffset+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN));	
			free(pBuf);
		}
	}
	else
	{
		pBuf = (unsigned char *)malloc((len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
		if (pBuf) {
			memset(pBuf, 0x00, (len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
			memcpy(pBuf, data, len);
			swap_endian(pBuf, (len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN, endian);	
			pci_write_memory(addr, (unsigned char *)pBuf,(len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);	
			free(pBuf);
		}
	}
	
	pthread_mutex_unlock(&io_mutex);	

	return len;	
}

int hpi_read_memory(unsigned long core_idx, void * base, unsigned int addr, unsigned char *data, int len, int endian, pthread_mutex_t io_mutex)
{
	unsigned char *pBuf;
	unsigned char lsBuf[HPI_BUS_LEN];
	int lsOffset;	
	
	if (addr < s_dram_base) {
		fprintf(stderr, "[HPI] invalid address base address is 0x%lu\n", s_dram_base);
		return 0;
	}

	if (pthread_mutex_lock(&io_mutex) < 0)
		return 0;

	if (len==0) {
		pthread_mutex_unlock(&io_mutex);
		return 0;
	}
	
	addr = addr - s_dram_base;
	s_hpi_base = base; 

	lsOffset = addr - (addr/HPI_BUS_LEN)*HPI_BUS_LEN;
	if (lsOffset)
	{
		pci_read_memory((addr/HPI_BUS_LEN)*HPI_BUS_LEN, lsBuf, HPI_BUS_LEN);	
		swap_endian(lsBuf, HPI_BUS_LEN, endian);	
		len = len-(HPI_BUS_LEN-lsOffset);
		pBuf = (unsigned char *)malloc(((len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN));
		if (pBuf)
		{
			memset(pBuf, 0x00, ((len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN));
			pci_read_memory((addr+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN, pBuf, ((len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN));	
			swap_endian(pBuf, ((len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN), endian);	

			memcpy(data, lsBuf+lsOffset, HPI_BUS_LEN-lsOffset);
			memcpy(data+HPI_BUS_LEN-lsOffset, pBuf, len);

			free(pBuf);
		}
	}
	else
	{
		pBuf = (unsigned char *)malloc((len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
		if (pBuf)
		{
			memset(pBuf, 0x00, (len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
			pci_read_memory(addr, pBuf, (len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN);
			swap_endian(pBuf, (len+HPI_BUS_LEN_ALIGN)&~HPI_BUS_LEN_ALIGN, endian);	
			memcpy(data, pBuf, len);
			free(pBuf);
		}
	}

	pthread_mutex_unlock(&io_mutex);	

	return len;
}


int hpi_hw_reset(void * base)
{

	s_hpi_base = base;
	pci_write_reg(DEVICE_ADDR_SW_RESET<<2, 1);		// write data 1	
	return 0;
}



unsigned int hpi_write_reg_limit(unsigned long core_idx, unsigned int addr, unsigned int data, pthread_mutex_t io_mutex)
{
	int status;
	int i;

	if (pthread_mutex_lock(&io_mutex) < 0)
		return 0;

	pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_DATA, ((data >> 16) & 0xFFFF));
	pci_write_reg(HPI_ADDR_DATA + 4, (data & 0xFFFF));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_WRITE_VALUE);
	
	i = 0;
	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = (status>>1) & 1;
		if (i++ > 10000)
		{
			pthread_mutex_unlock(&io_mutex);
			return 0;
		}
	} while (status == 0);

	pthread_mutex_unlock(&io_mutex);	

	return 1;
}



unsigned int hpi_read_reg_limit(unsigned long core_idx, unsigned int addr, unsigned int *data, pthread_mutex_t io_mutex)
{
	int status;
	int i;

	if (pthread_mutex_lock(&io_mutex) < 0)
		return 0;


	pci_write_reg(HPI_ADDR_ADDR_H, ((addr >> 16)&0xffff));
	pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

	pci_write_reg(HPI_ADDR_CMD, HPI_CMD_READ_VALUE);

	i=0;
	do {
		status = pci_read_reg(HPI_ADDR_STATUS);
		status = status & 1;
		if (i++ > 10000)
		{
			pthread_mutex_unlock(&io_mutex);
			return 0;
		}
	} while (status == 0);


	*data = pci_read_reg(HPI_ADDR_DATA) << 16;
	*data |= pci_read_reg(HPI_ADDR_DATA + 4);

	pthread_mutex_unlock(&io_mutex);	

	return 1;
}


/*------------------------------------------------------------------------
	Usage : used to program output frequency of ICS307M
	Artument :
		Device		: first device selected if 0, second device if 1.
		OutFreqMHz	: Target output frequency in MHz.
		InFreqMHz	: Input frequency applied to device in MHz
					  this must be 10 here.
	Return : TRUE if success, FALSE if invalid OutFreqMHz.
------------------------------------------------------------------------*/



int ics307m_set_clock_freg(void * base, int Device, int OutFreqMHz, int InFreqMHz)
{
	
	int		VDW, RDW, OD, SDW, tmp;
	int		min_clk ; 
	int		max_clk ;

	s_hpi_base = base;
	if ( Device == 0 )
	{   
		min_clk = ACLK_MIN ;
		max_clk = ACLK_MAX ;
	}
	else
	{   
		min_clk = CCLK_MIN ;
		max_clk = CCLK_MAX ;
	}
	
	if (OutFreqMHz < min_clk || OutFreqMHz > max_clk) {
	   // printf ("Target Frequency should be from %2d to %2d !!!\n", min_clk, max_clk);
		return 0;
	}

	if (OutFreqMHz >= min_clk && OutFreqMHz < 14) {
		switch (OutFreqMHz) {
		case 6: VDW=4; RDW=2; OD=10; break;
		case 7: VDW=6; RDW=2; OD=10; break;
		case 8: VDW=8; RDW=2; OD=10; break;
		case 9: VDW=10; RDW=2; OD=10; break;
		case 10: VDW=12; RDW=2; OD=10; break;
		case 11: VDW=14; RDW=2; OD=10; break;
		case 12: VDW=16; RDW=2; OD=10; break;
		case 13: VDW=18; RDW=2; OD=10; break;
		} 
	} else {
		VDW = OutFreqMHz - 8;	// VDW
		RDW = 3;				// RDW
		OD = 4;					// OD
	} 

	switch (OD) {			// change OD to SDW: s2:s1:s0 
		case 0: SDW = 0; break;
		case 1: SDW = 0; break;
		case 2: SDW = 1; break;
		case 3: SDW = 6; break;
		case 4: SDW = 3; break;
		case 5: SDW = 4; break;
		case 6: SDW = 7; break;
		case 7: SDW = 4; break;
		case 8: SDW = 2; break;
		case 9: SDW = 0; break;
		case 10: SDW = 0; break;
		default: SDW = 0; break;
	}

	
	if (Device == 0) {	// select device 1
		tmp = 0x20 | SDW;
		pci_write_reg((DEVICE0_ADDR_PARAM0)<<2, tmp);		// write data 0
		tmp = ((VDW << 7)&0xff80) | RDW;
		pci_write_reg((DEVICE0_ADDR_PARAM1)<<2, tmp);		// write data 1
		tmp = 1;
		pci_write_reg((DEVICE0_ADDR_COMMAND)<<2, tmp);		// write command set
		tmp = 0;
		pci_write_reg((DEVICE0_ADDR_COMMAND)<<2, tmp);		// write command reset
	} else {			// select device 2
		tmp = 0x20 | SDW;
		pci_write_reg((DEVICE1_ADDR_PARAM0)<<2, tmp);		// write data 0
		tmp = ((VDW << 7)&0xff80) | RDW;
		pci_write_reg((DEVICE1_ADDR_PARAM1)<<2, tmp);		// write data 1
		tmp = 1;
		pci_write_reg((DEVICE1_ADDR_COMMAND)<<2, tmp);		// write command set
		tmp = 0;
		pci_write_reg((DEVICE1_ADDR_COMMAND)<<2, tmp);		// write command reset
	}
	return 1;
}


int hpi_set_timing_opt(unsigned long core_idx, void * base, pthread_mutex_t io_mutex) 
{
	int i;
	unsigned int iAddr;
	unsigned int uData;
	unsigned int uuData;
	int iTemp;
	int testFail;
#define MIX_L1_Y_SADDR			(0x11000000 + 0x0138)
#define MIX_L1_CR_SADDR         (0x11000000 + 0x0140)

	s_hpi_base = base;

	i=2;
	// find HPI maximum timing register value
	do {
		i++;
		//iAddr = BIT_BASE + 0x100;
		iAddr = MIX_L1_Y_SADDR;
		uData = 0x12345678;
		testFail = 0;
		printf ("HPI Tw, Tr value: %d\r", i);

		pci_write_reg(0x70<<2, i);
		pci_write_reg(0x71<<2, i);
		if (i<15) 
			pci_write_reg(0x72<<2, 0);
		else
			pci_write_reg(0x72<<2, i*20/100);

		for (iTemp=0; iTemp<10000; iTemp++) {
			uuData = 0;
			if (hpi_write_reg_limit(core_idx, iAddr, uData, io_mutex)==0) {
				testFail = 1;
				break;
			}
			if (hpi_read_reg_limit(core_idx,iAddr, &uuData, io_mutex)==0) {
				testFail = 1;
				break;
			} 
			if (uuData != uData) {
				testFail = 1;
				break;
			}
			else {
				if (hpi_write_reg_limit(core_idx, iAddr, 0, io_mutex)==0) {
					testFail = 1;
					break;
				}
			}

			iAddr += 4;
			/*
			if (iAddr == BIT_BASE + 0x200)
			iAddr = BIT_BASE + 0x100;
			*/
			if (iAddr == MIX_L1_CR_SADDR)
				iAddr = MIX_L1_Y_SADDR;
			uData++;
		}
	} while (testFail && i < HPI_SET_TIMING_MAX);

	pci_write_reg(0x70<<2, i);
	pci_write_reg(0x71<<2, i+i*40/100);
	pci_write_reg(0x72<<2, i*20/100);

	printf ("\nOptimized HPI Tw value : %x\n", pci_read_reg(0x70<<2));
	printf ("Optimized HPI Tr value : %x\n", pci_read_reg(0x71<<2));
	printf ("Optimized HPI Te value : %x\n", pci_read_reg(0x72<<2));



	return i;
}


void pci_write_reg(unsigned int addr, unsigned int data)
{
	unsigned long *reg_addr = (unsigned long *)(addr + s_hpi_base);
	*(volatile unsigned long *)reg_addr = data;	
}

unsigned int pci_read_reg(unsigned int addr)
{
	unsigned long *reg_addr = (unsigned long *)(addr + s_hpi_base);
	return *(volatile unsigned long *)reg_addr;
}


void pci_read_memory(unsigned int addr, unsigned char *buf, int size)
{

	int status;
	int i, j, k;
	int data = 0;

	i = j = k = 0;
	
    for (i=0; i < size / HPI_MAX_PKSIZE; i++) 
	{
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));		
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));

		pci_write_reg(HPI_ADDR_CMD, (((HPI_MAX_PKSIZE) << 4) + 1));
		
		do 
		{
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = status & 1;
		} while (status==0);
		
		for (j=0; j<HPI_MAX_PKSIZE/2; j++) 
		{
			data = pci_read_reg(HPI_ADDR_DATA + j * 4);
            buf[k  ] = (data >> 8) & 0xFF;
            buf[k+1] = data & 0xFF;
            k = k + 2;
		}
		
        addr += HPI_MAX_PKSIZE;
    }
	
    size = size % HPI_MAX_PKSIZE;
    
	if ( ((addr + size) & 0xFFFFFF00) != (addr & 0xFFFFFF00))
        size = size;
	
    if (size) 
	{
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));
		
		pci_write_reg(HPI_ADDR_CMD, (((size) << 4) + 1));
		
		do 
		{
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = status & 1;
			
		} while (status==0);
		
		for (j = 0; j < size / 2; j++) 
		{
			data = pci_read_reg(HPI_ADDR_DATA + j*4);
            buf[k  ] = (data >> 8) & 0xFF;
            buf[k+1] = data & 0xFF;
            k = k + 2;
		}
    }
}

void pci_write_memory(unsigned int addr, unsigned char *buf, int size)
{
	int status;
	int i, j, k;
	int data = 0;
	
	i = j = k = 0;
	
    for (i = 0; i < size/HPI_MAX_PKSIZE; i++)
	{
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));
		
		for (j=0; j < HPI_MAX_PKSIZE/2; j++) 
		{            
			data = (buf[k] << 8) | buf[k+1];
			pci_write_reg(HPI_ADDR_DATA + j * 4, data);
            k = k + 2;
		}
	
		pci_write_reg(HPI_ADDR_CMD, (((HPI_MAX_PKSIZE) << 4) + 2));
		
		do 
		{
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = (status >> 1) & 1;
		} while (status==0);
		
        addr += HPI_MAX_PKSIZE;
    }
	
    size = size % HPI_MAX_PKSIZE;
	
	if (size) 
	{
		pci_write_reg(HPI_ADDR_ADDR_H, (addr >> 16));
		pci_write_reg(HPI_ADDR_ADDR_L, (addr & 0xffff));
		
		for (j = 0; j< size / 2; j++) 
		{
            data = (buf[k] << 8) | buf[k+1];
			pci_write_reg(HPI_ADDR_DATA + j * 4, data);
            k = k + 2;
		}
		
		pci_write_reg(HPI_ADDR_CMD, (((size) << 4) + 2));
		
		do 
		{
			status = 0;
			status = pci_read_reg(HPI_ADDR_STATUS);
			status = (status>>1) & 1;
			
		} while (status==0);
    }
}

#endif



#endif	//#if defined(linux) || defined(__linux) || defined(ANDROID)

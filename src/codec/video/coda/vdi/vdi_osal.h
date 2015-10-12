//------------------------------------------------------------------------------
// File: log.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _VDI_OSAL_H_
#define _VDI_OSAL_H_



enum {NONE=0, INFO, WARN, ERR, TRACE, MAX_LOG_LEVEL};
enum 
{
    LOG_HAS_DAY_NAME   =    1, /**< Include day name [default: no] 	      */
    LOG_HAS_YEAR       =    2, /**< Include year digit [no]		      */
    LOG_HAS_MONTH	  =    4, /**< Include month [no]		      */
    LOG_HAS_DAY_OF_MON =    8, /**< Include day of month [no]	      */
    LOG_HAS_TIME	  =   16, /**< Include time [yes]		      */
    LOG_HAS_MICRO_SEC  =   32, /**< Include microseconds [yes]             */
    LOG_HAS_FILE	  =   64, /**< Include sender in the log [yes] 	      */
    LOG_HAS_NEWLINE	  =  128, /**< Terminate each call with newline [yes] */
    LOG_HAS_CR	  =  256, /**< Include carriage return [no] 	      */
    LOG_HAS_SPACE	  =  512, /**< Include two spaces before log [yes]    */
    LOG_HAS_COLOR	  = 1024, /**< Colorize logs [yes on win32]	      */
    LOG_HAS_LEVEL_TEXT = 2048 /**< Include level text string [no]	      */    
};
enum {
    TERM_COLOR_R	= 2,    /**< Red            */
    TERM_COLOR_G	= 4,    /**< Green          */
    TERM_COLOR_B	= 1,    /**< Blue.          */
    TERM_COLOR_BRIGHT = 8    /**< Bright mask.   */
};

#define MAX_PRINT_LENGTH 512

#ifdef ANDROID
#include <utils/Log.h>
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "VPUAPI"
#endif

#define VLOG LogMsg



#define LOG_ENABLE_FILE	SetLogDecor(GetLogDecor()|LOG_HAS_FILE);


typedef void * osal_file_t;
# ifndef SEEK_SET
# define	SEEK_SET	0
# endif

# ifndef SEEK_CUR
# define	SEEK_CUR	1
# endif

# ifndef SEEK_END
# define	SEEK_END	2
# endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#else

#ifndef stdout
# define	stdout	(void * )1
#endif
#ifndef stderr
# define	stderr	(void * )1
#endif

#endif

#if defined (__cplusplus)
extern "C" {
#endif 

	int InitLog(void);
	void DeInitLog(void);
	
	void SetMaxLogLevel(int level);
	int GetMaxLogLevel(void);
	
	void SetLogColor(int level, int color);
	int GetLogColor(int level);
	
	void SetLogDecor(int decor);
	int GetLogDecor(void);
	
	//log print
	void LogMsg(int level, const char *format, ...);
	
	//timer
	void timer_start(void);
	void timer_stop(void);
	double timer_elapsed_us(void);
	double timer_elapsed_ms(void);
	int timer_is_valid(void);
	double timer_frequency(void);

	//math
	int math_div(int number, int denom);
	int math_modulo(int number, int denom);

	//terminal
	void osal_init_keyboard(void);
	void osal_close_keyboard(void);

	//memory
	void * osal_memcpy (void *dst, const void * src, int count);
	void * osal_memset (void *dst, int val,int count);
	void * osal_malloc(int size);
    void * osal_realloc(void* ptr, int size);
	void osal_free(void *p);

	
	osal_file_t osal_fopen(const char * osal_file_tname, const char * mode);
	int osal_fwrite(const void * p, int size, int count, osal_file_t fp);
	int osal_fread(void *p, int size, int count, osal_file_t fp);
	long osal_ftell(osal_file_t fp);
	int osal_fseek(osal_file_t fp, long offset, int origin);
	int osal_fclose(osal_file_t fp);
	int osal_fflush(osal_file_t fp);
	int osal_fprintf(osal_file_t fp, const char * _Format, ...);
	int osal_fscanf(osal_file_t fp, const char * _Format, ...);
	int osal_kbhit(void);
	int osal_getch(void);
	int osal_flush_ch(void);
    int osal_feof(osal_file_t fp);
	void * osal_create_mutex(const char *name);
	void osal_close_mutex(void *handle);
	int  osal_mutex_lock(void *handle);
	int  osal_mutex_unlock(void *handle);
		
#if defined (__cplusplus)
}
#endif 

#endif //#ifndef _VDI_OSAL_H_

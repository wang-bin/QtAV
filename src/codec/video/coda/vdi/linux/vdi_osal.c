//------------------------------------------------------------------------------
// File: vdi_osal.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID)

#include "vpuconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../vdi_osal.h"

#include <time.h>
#include <sys/time.h>
#include <termios.h>
#ifndef USE_KERNEL_MODE
#include <unistd.h>   // for read()
#endif
static struct termios initial_settings, new_settings;
static int peek_character = -1;

static int log_colors[MAX_LOG_LEVEL] = { 
	0,
	TERM_COLOR_R|TERM_COLOR_G|TERM_COLOR_B|TERM_COLOR_BRIGHT, 	//INFO
	TERM_COLOR_R|TERM_COLOR_B|TERM_COLOR_BRIGHT,	//WARN
	TERM_COLOR_R|TERM_COLOR_BRIGHT,	// ERR
	TERM_COLOR_R|TERM_COLOR_G|TERM_COLOR_B	//TRACE
};

static unsigned log_decor = LOG_HAS_TIME | LOG_HAS_FILE | LOG_HAS_MICRO_SEC |
			    LOG_HAS_NEWLINE |
			    LOG_HAS_SPACE | LOG_HAS_COLOR;
static int max_log_level = MAX_LOG_LEVEL;		
static FILE *fpLog  = NULL;



static void term_restore_color();
static void term_set_color(int color);

int InitLog() 
{
	//fpLog = fopen("c:\\out.log", "w");

	return 1;	
}

void DeInitLog()
{
	if (fpLog)
	{
		fclose(fpLog);
		fpLog = NULL;
	}
}

void SetLogColor(int level, int color)
{
	log_colors[level] = color;
}

int GetLogColor(int level)
{
	return log_colors[level];
}

void SetLogDecor(int decor)
{
	log_decor = decor;
}

int GetLogDecor()
{
	return log_decor;
}

void SetMaxLogLevel(int level)
{
	max_log_level = level;
}
int GetMaxLogLevel()
{
	return max_log_level;
}

void LogMsg(int level, const char *format, ...)
{
	va_list ptr;
	char logBuf[MAX_PRINT_LENGTH] = {0};

	
	if (level > max_log_level)
		return;		

	va_start( ptr, format );	
#if defined(WIN32) || defined(__MINGW32__)
	_vsnprintf( logBuf, MAX_PRINT_LENGTH, format, ptr );	
#else
	vsnprintf( logBuf, MAX_PRINT_LENGTH, format, ptr );
#endif
	va_end(ptr);

	if (log_decor & LOG_HAS_COLOR)
		term_set_color(log_colors[level]);	
	
#ifdef ANDROID
	if (level == ERR)
	{
	ALOGE("%s", logBuf);
  }
  else
  {
			ALOGI("%s", logBuf);
  }
	fputs(logBuf, stdout);
#else
	fputs(logBuf, stdout);
#endif
		
		
	if ((log_decor & LOG_HAS_FILE) && fpLog)
	{
		fwrite(logBuf, strlen(logBuf), 1,fpLog);
		fflush(fpLog);
	}
		
	if (log_decor & LOG_HAS_COLOR)
		term_restore_color();
}




static void term_set_color(int color)
{	
	/* put bright prefix to ansi_color */
    char ansi_color[12] = "\033[01;3";

return;

    if (color & TERM_COLOR_BRIGHT)
		color ^= TERM_COLOR_BRIGHT;
    else
		strcpy(ansi_color, "\033[00;3");

    switch (color) {
    case 0:
	/* black color */
	strcat(ansi_color, "0m");
	break;
    case TERM_COLOR_R:
	/* red color */
	strcat(ansi_color, "1m");
	break;
    case TERM_COLOR_G:
	/* green color */
	strcat(ansi_color, "2m");
	break;
    case TERM_COLOR_B:
	/* blue color */
	strcat(ansi_color, "4m");
	break;
    case TERM_COLOR_R | TERM_COLOR_G:
	/* yellow color */
	strcat(ansi_color, "3m");
	break;
    case TERM_COLOR_R | TERM_COLOR_B:
	/* magenta color */
	strcat(ansi_color, "5m");
	break;
    case TERM_COLOR_G | TERM_COLOR_B:
	/* cyan color */
	strcat(ansi_color, "6m");
	break;
    case TERM_COLOR_R | TERM_COLOR_G | TERM_COLOR_B:
	/* white color */
	strcat(ansi_color, "7m");
	break;
    default:
	/* default console color */
	strcpy(ansi_color, "\033[00m");
	break;
    }

    fputs(ansi_color, stdout);
	
}

static void term_restore_color(void)
{
	term_set_color(log_colors[4]);
}


static double timer_frequency_;

struct timeval tv_start;
struct timeval tv_end;


void timer_init()
{

}

void timer_start()
{
	timer_init();
}

void timer_stop()
{
	gettimeofday(&tv_end, NULL);
}

double timer_elapsed_ms()
{
	double ms;
	ms = timer_elapsed_us()/1000.0;	
	return ms;
}

double timer_elapsed_us()
{
	double elapsed = 0;
	double start_us = 0;
	double end_us = 0;
	end_us = tv_end.tv_sec*1000*1000 + tv_end.tv_usec;
	start_us = tv_start.tv_sec*1000*1000 + tv_start.tv_usec;
	elapsed =  end_us - start_us;	
	return elapsed;

}

int timer_is_valid()
{
	return timer_frequency_ != 0;
}

double timer_frequency()
{
	return timer_frequency_;
}

void osal_init_keyboard()
{
    tcgetattr(0,&initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
	peek_character = -1;
}

void osal_close_keyboard()
{
    tcsetattr(0, TCSANOW, &initial_settings);
}

int osal_kbhit()
{
	unsigned char ch;
	int nread;

//	return 0;	// need to implement for the own system

    if (peek_character != -1) return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1)
    {
        peek_character = (int)ch;
        return 1;
    }
    return 0;
}


int osal_getch()
{
	int val;
	char ch;

//	return -1;	// need to implement for the own system

    if(peek_character != -1)
    {
    	val = peek_character;
        peek_character = -1;
        return val;
    }
    read(0,&ch,1);
    return ch;
}

int osal_flush_ch(void)
{
//	return -1;	// need to implement for the own system

	fflush(stdout);
	return 1;
}

void * osal_memcpy(void * dst, const void * src, int count)
{
	return memcpy(dst, src, count);
}

void * osal_memset(void *dst, int val, int count)
{
	return memset(dst, val, count);
}

void * osal_malloc(int size)
{
	return malloc(size);
}

void * osal_realloc(void* ptr, int size)
{
    return realloc(ptr, size);
}

void osal_free(void *p)
{
	free(p);
}

int osal_fflush(osal_file_t fp)
{
	return fflush(fp);
}

int osal_feof(osal_file_t fp)
{
    return feof((FILE *)fp);
}

osal_file_t osal_fopen(const char * osal_file_tname, const char * mode)
{
	return fopen(osal_file_tname, mode);
}
int osal_fwrite(const void * p, int size, int count, osal_file_t fp)
{
	return fwrite(p, size, count, fp);
}
int osal_fread(void *p, int size, int count, osal_file_t fp)
{
	return fread(p, size, count, fp);
}
long osal_ftell(osal_file_t fp)
{
	return ftell(fp);
}

int osal_fseek(osal_file_t fp, long offset, int origin)
{
	return fseek(fp, offset, origin);
}
int osal_fclose(osal_file_t fp)
{
	return fclose(fp);
}

int osal_fscanf(osal_file_t fp, const char * _Format, ...)
{
	int ret;

	va_list arglist;
	va_start(arglist, _Format);

	ret = vfscanf(fp, _Format, arglist);

	va_end(arglist);

	return ret;
}

int osal_fprintf(osal_file_t fp, const char * _Format, ...)
{
	int ret;
	va_list arglist;
	va_start(arglist, _Format);

	ret = vfprintf(fp, _Format, arglist);

	va_end(arglist);

	return ret;
}



//------------------------------------------------------------------------------
// math related api
//------------------------------------------------------------------------------
#ifndef I64
typedef long long I64;
#endif

// 32 bit / 16 bit ==> 32-n bit remainder, n bit quotient
static int fixDivRq(int a, int b, int n)
{
    I64 c;
    I64 a_36bit;
    I64 mask, signBit, signExt;
    int  i;

    // DIVS emulation for BPU accumulator size
    // For SunOS build
    mask = 0x0F; mask <<= 32; mask |= 0x00FFFFFFFF; // mask = 0x0FFFFFFFFF;
    signBit = 0x08; signBit <<= 32;                 // signBit = 0x0800000000;
    signExt = 0xFFFFFFF0; signExt <<= 32;           // signExt = 0xFFFFFFF000000000;

    a_36bit = (I64) a;

    for (i=0; i<n; i++) {
        c =  a_36bit - (b << 15);
        if (c >= 0)
            a_36bit = (c << 1) + 1;
        else
            a_36bit = a_36bit << 1;

        a_36bit = a_36bit & mask;
        if (a_36bit & signBit)
            a_36bit |= signExt;
    }

    a = (int) a_36bit;
    return a;                           // R = [31:n], Q = [n-1:0]
}

int math_div(int number, int denom)
{
    int  c;
    c = fixDivRq(number, denom, 17);             // R = [31:17], Q = [16:0]
    c = c & 0xFFFF;
    c = (c + 1) >> 1;                   // round
    return (c & 0xFFFF);
}

#endif	//#if defined(linux) || defined(__linux) || defined(ANDROID) 

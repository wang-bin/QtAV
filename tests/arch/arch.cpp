/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/
// change from qtbase/config.tests/arch/arch.cpp

#define QGLOBAL_H
#include "qprocessordetection.h"

/* vc: arm, mips, sh, x86, x86_64, ia64*/
#if defined(Q_PROCESSOR_ALPHA)
#warning "ARCH*=alpha"
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_ARM_32)
#ifdef _MSC_VER
#pragma message ("ARCH*=arm")
#else
#warning "ARCH*=arm"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_ARM_64)
#ifdef _MSC_VER
#pragma message ("ARCH*=arm64")
#else
#warning "ARCH*=arm64"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_ARM)
#ifdef _MSC_VER
#pragma message ("ARCH*=arm")
#else
#warning "ARCH*=arm"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_AVR32)
#warning "ARCH*=avr32"
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_BLACKFIN)
#warning "ARCH*=bfin"
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_X86_32)
#ifdef _MSC_VER
#pragma message ("ARCH*=x86")
#else
#warning "ARCH*=x86"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_X86_64)
#ifdef _MSC_VER
#pragma message ("ARCH*=x86_64")
#else
#warning "ARCH*=x86_64"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_IA64)
#ifdef _MSC_VER
#pragma message ("ARCH*=ia64")
#else
#warning "ARCH*=ia64"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_MIPS)
#ifdef _MSC_VER
#pragma message ("ARCH*=mips")
#else
#warning "ARCH*=mips"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_POWER)
#warning "ARCH*=power"
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_S390)
#warning "ARCH*=s390"
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_SH)
#ifdef _MSC_VER
#pragma message ("ARCH*=sh")
#else
#warning "ARCH*=sh"
#endif /*_MSC_VER*/
#define ARCH_DETECTED
#endif
#if defined(Q_PROCESSOR_SPARC)
#warning "ARCH*=sparc"
#define ARCH_DETECTED
#endif
#ifndef ARCH_DETECTED
#warning "ARCH=unknown"
#endif

        // This is the list of features found in GCC or MSVC
        // We don't use all of them, but this is ready for future expansion

// -- x86 --
#ifdef __3dNOW__
// 3dNow!, introduced with the AMD K6-2, discontinued after 2010
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=3dnow")
#else
#warning "ARCH_SUB*=3dnow"
#endif /*_MSC_VER*/
#endif
#ifdef __3dNOW_A__
// Athlon
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=3dnow-a")
#else
#warning "ARCH_SUB*=3dnow-a"
#endif /*_MSC_VER*/
#endif
#ifdef __ABM__
// Advanced Bit Manipulation, AMD Barcelona (family 10h)
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=abm")
#else
#warning "ARCH_SUB*=abm"
#endif /*_MSC_VER*/
#endif
#ifdef __AES__
// AES New Instructions, Intel Core-i7 second generation ("Sandy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=aes")
#else
#warning "ARCH_SUB*=aes"
#endif /*_MSC_VER*/
#endif
#ifdef __AVX__
// Advanced Vector Extensions, Intel Core-i7 second generation ("Sandy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=avx")
#else
#warning "ARCH_SUB*=avx"
#endif /*_MSC_VER*/
#endif
#ifdef __AVX2__
// AVX 2, Intel Core 4th Generation ("Haswell")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=avx2")
#else
#warning "ARCH_SUB*=avx2"
#endif /*_MSC_VER*/
#endif
#ifdef __AVX512F__
// AVX512 Foundation, Intel Xeon Phi codename "Knights Landing"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=avx512f")
#else
#warning "ARCH_SUB*=avx512f"
#endif /*_MSC_VER*/
#endif
#ifdef __AVX512CD__
// AVX512 Conflict Detection, Intel Xeon Phi codename "Knights Landing"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=avx512cd")
#else
#warning "ARCH_SUB*=avx512cd"
#endif /*_MSC_VER*/
#endif
#ifdef __AVX512ER__
// AVX512 Exponentiation & Reciprocal, Intel Xeon Phi codename "Knights Landing"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=avx512ef")
#else
#warning "ARCH_SUB*=avx512ef"
#endif /*_MSC_VER*/
#endif
#ifdef __AVX512PF__
// AVX512 Prefetch, Intel Xeon Phi codename "Knights Landing"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=avx512pf")
#else
#warning "ARCH_SUB*=avx512pf"
#endif /*_MSC_VER*/
#endif
#ifdef __BMI__
// Bit Manipulation Instructions 1, Intel Core 4th Generation ("Haswell"), AMD "Bulldozer 2"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=bmi")
#else
#warning "ARCH_SUB*=bmi"
#endif /*_MSC_VER*/
#endif
#ifdef __BMI2__
// Bit Manipulation Instructions 2, Intel Core 4th Generation ("Haswell")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=bmi12")
#else
#warning "ARCH_SUB*=bmi12"
#endif /*_MSC_VER*/
#endif
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
// cmpxchg16b instruction, Intel Pentium 4 64-bit ("Nocona"), AMD Barcelona (family 10h)
// Notably, this instruction is missing on earlier AMD Athlon 64
#warning "ARCH_SUB*=cx16"
#endif
#ifdef __F16C__
// 16-bit floating point conversion, Intel Core 3rd Generation ("Ivy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=f16c")
#else
#warning "ARCH_SUB*=f16c"
#endif /*_MSC_VER*/
#endif
#ifdef __FMA__
// Fused Multiply-Add with 3 arguments, Intel Core 4th Generation ("Haswell"), AMD "Bulldozer 2"
// a.k.a. "FMA3"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=fma")
#else
#warning "ARCH_SUB*=fma"
#endif /*_MSC_VER*/
#endif
#ifdef __FMA4__
// Fused Multiply-Add with 4 arguments, AMD "Bulldozer"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=fma4")
#else
#warning "ARCH_SUB*=fma4"
#endif /*_MSC_VER*/
#endif
#ifdef __FSGSBASE__
// rdfsgsbase, wrfsgsbase, Intel Core 3rd Generation ("Ivy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=fsgsbase")
#else
#warning "ARCH_SUB*=fsgsbase"
#endif /*_MSC_VER*/
#endif
#ifdef __LWP__
// LWP instructions, AMD "Bulldozer"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=lwp")
#else
#warning "ARCH_SUB*=lwp"
#endif /*_MSC_VER*/
#endif
#ifdef __LZCNT__
// Leading-Zero bit count, Intel Core 4th Generation ("Haswell")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=lzcnt")
#else
#warning "ARCH_SUB*=lzcnt"
#endif /*_MSC_VER*/
#endif
#ifdef __MMX__
// Multimedia Extensions, Pentium MMX, AMD K6-2
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=mmx")
#else
#warning "ARCH_SUB*=mmx"
#endif /*_MSC_VER*/
#endif
#ifdef __MOVBE__
// Move Big Endian, Intel Atom & "Haswell"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=movbe")
#else
#warning "ARCH_SUB*=movbe"
#endif /*_MSC_VER*/
#endif
#ifdef __NO_SAHF__
// missing SAHF instruction in 64-bit, up to Intel Pentium 4 64-bit ("Nocona"), AMD Athlon FX
// Note: the macro is not defined, so this will never show up
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=no-sahf")
#else
#warning "ARCH_SUB*=no-sahf"
#endif /*_MSC_VER*/
#endif
#ifdef __PCLMUL__
// (Packed) Carry-less multiplication, Intel Core-i7 second generation ("Sandy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=pclmul")
#else
#warning "ARCH_SUB*=pclmul"
#endif /*_MSC_VER*/
#endif
#ifdef __POPCNT__
// Population Count (count of set bits), Intel Core-i7 second generation ("Sandy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=popcnt")
#else
#warning "ARCH_SUB*=popcnt"
#endif /*_MSC_VER*/
#endif
#ifdef __RDRND__
// Random number generator, Intel Core 3rd Generation ("Ivy Bridge")
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=rdrnd")
#else
#warning "ARCH_SUB*=rdrnd"
#endif /*_MSC_VER*/
#endif
#ifdef __SHA__
// SHA-1 and SHA-256 instructions, Intel processor TBA
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sha")
#else
#warning "ARCH_SUB*=sha"
#endif /*_MSC_VER*/
#endif
#if defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1) || defined(_M_X64)
// Streaming SIMD Extensions, Intel Pentium III, AMD Athlon
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sse")
#else
#warning "ARCH_SUB*=sse"
#endif /*_MSC_VER*/
#endif
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
// SSE2, Intel Pentium-M, Intel Pentium 4, AMD Opteron and Athlon 64
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sse2")
#else
#warning "ARCH_SUB*=sse2"
#endif /*_MSC_VER*/
#endif
#ifdef __SSE3__
// SSE3, Intel Pentium 4 "Prescott", AMD Athlon 64 rev E
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sse3")
#else
#warning "ARCH_SUB*=sse3"
#endif /*_MSC_VER*/
#endif
#ifdef __SSSE3__
// Supplemental SSE3, Intel Core 2 ("Merom"), AMD "Bulldozer"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=ssse3")
#else
#warning "ARCH_SUB*=ssse3"
#endif /*_MSC_VER*/
#endif
#ifdef __SSE4A__
// SSE4a, AMD Barcelona
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sse4a")
#else
#warning "ARCH_SUB*=sse4a"
#endif /*_MSC_VER*/
#endif
#ifdef __SSE4_1__
// SSE 4.1, Intel Core2 45nm shrink ("Penryn"), AMD "Bulldozer"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sse4.1")
#else
#warning "ARCH_SUB*=sse4.1"
#endif /*_MSC_VER*/
#endif
#ifdef __SSE4_2__
// SSE 4.2, Intel Core-i7 ("Nehalem"), AMD "Bulldozer"
// Since no processor supports SSE4.2 without 4.1 and since no Intel processor
// supports SSE4a, define "sse4" to indicate SSE4"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=sse4.2")
#pragma message ("ARCH_SUB*=sse4")
#else
#warning "ARCH_SUB*=sse4.2"
#warning "ARCH_SUB*=sse4"
#endif /*_MSC_VER*/
#endif
#ifdef __TBM__
// TBM, AMD "Bulldozer"
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=tbm")
#else
#warning "ARCH_SUB*=tbm"
#endif /*_MSC_VER*/
#endif
#ifdef __XOP__
// XOP, AMD "Bulldozer"
#warning "ARCH_SUB*=xop"
#endif

// -- ARM --
#ifdef __ARM_NEON__
#warning "ARCH_SUB*=neon"
#endif
#ifdef __IWMMXT__
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=iwmmxt")
#else
#warning "ARCH_SUB*=iwmmxt"
#endif /*_MSC_VER*/
#endif

// -- SPARC --
#ifdef __VIS__
#warning "ARCH_SUB*=vis"
# if __VIS__ >= 0x200
#warning "ARCH_SUB*=vis2"
# endif
# if __VIS__ >= 0x300
#warning "ARCH_SUB*=vis3"
# endif
#endif

// -- MIPS --
# if __mips_dsp
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=dsp")
#else
#warning "ARCH_SUB*=dsp"
#endif /*_MSC_VER*/
# endif
# if __mips_dspr2
#ifdef _MSC_VER
#pragma message ("ARCH_SUB*=dsp2")
#else
#warning "ARCH_SUB*=dsp2"
#endif /*_MSC_VER*/
# endif

// -- POWER, PowerPC --
#ifdef __ALTIVEC__
#warning "ARCH_SUB*=altivec"
#endif

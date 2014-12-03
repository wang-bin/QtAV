/*
 * Copyright (c) 2014 Jens Kuske <jenskuske@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits /* mark stack as non-executable */
#endif

.text
.syntax unified
.arch armv7-a
.fpu neon
.thumb

.macro thumb_function fname
	.global \fname
#ifdef __ELF__
	.hidden \fname
	.type \fname, %function
#endif
	.thumb_func
\fname:
.endm

.macro end_function fname
#ifdef __ELF__
	.size \fname, .-\fname
#endif
.endm

SRC	.req r0
DST	.req r1
PITCH	.req r2
CNT	.req r3
TLINE	.req r4
HEIGHT	.req r5
REST	.req r6
NTILES	.req r7
TMPSRC	.req r8
DST2	.req r9
TSIZE	.req r12
NEXTLIN	.req lr

thumb_function tiled_to_planar
	push	{r4, r5, r6, r7, r8, lr}
	ldr	HEIGHT, [sp, #24]
	add	NEXTLIN, r3, #31
	lsrs	NTILES, r3, #5
	bic	NEXTLIN, NEXTLIN, #31
	and	REST, r3, #31
	lsl	NEXTLIN, NEXTLIN, #5
	subs	PITCH, r2, r3
	movs	TLINE, #32
	rsb	NEXTLIN, NEXTLIN, #32
	mov	TSIZE, #1024

	/* y loop */
1:	cbz	NTILES, 3f
	mov	CNT, NTILES

	/* x loop complete tiles */
2:	pld	[SRC, TSIZE]
	vld1.8	{d0 - d3}, [SRC :256], TSIZE
	subs	CNT, #1
	vst1.8	{d0 - d3}, [DST]!
	bne	2b

3:	cbnz	REST, 4f

	/* fix up dest pointer if pitch != width */
7:	add	DST, PITCH

	/* fix up src pointer at end of line */
	subs	TLINE, #1
	itee	ne
	addne	SRC, NEXTLIN
	subeq	SRC, #992
	moveq	TLINE, #32

	subs	HEIGHT, #1
	bne	1b
	pop	{r4, r5, r6, r7, r8, pc}

	/* partly copy last tile of line */
4:	mov	TMPSRC, SRC
	tst	REST, #16
	beq	5f
	vld1.8	{d0 - d1}, [TMPSRC :128]!
	vst1.8	{d0 - d1}, [DST]!
5:	add	SRC, TSIZE
	ands	CNT, REST, #15
	beq	7b
6:	vld1.8	{d0[0]}, [TMPSRC]!
	subs	CNT, #1
	vst1.8	{d0[0]}, [DST]!
	bne	6b
	b	7b
end_function tiled_to_planar

thumb_function tiled_deinterleave_to_planar
	push	{r4, r5, r6, r7, r8, r9, lr}
	mov     DST2, r2
	ldr	HEIGHT, [sp, #32]
	ldr	r4, [sp, #28]
	add	NEXTLIN, r4, #31
	lsrs	NTILES, r4, #5
	bic	NEXTLIN, NEXTLIN, #31
	ubfx	REST, r4, #1, #4
	lsl	NEXTLIN, NEXTLIN, #5
	sub	PITCH, r3, r4, lsr #1
	movs	TLINE, #32
	rsb	NEXTLIN, NEXTLIN, #32
	mov	TSIZE, #1024

	/* y loop */
1:	cbz	NTILES, 3f
	mov	CNT, NTILES

	/* x loop complete tiles */
2:	pld	[SRC, TSIZE]
	vld2.8	{d0 - d3}, [SRC :256], TSIZE
	subs	CNT, #1
	vst1.8	{d0 - d1}, [DST]!
	vst1.8	{d2 - d3}, [DST2]!
	bne	2b

3:	cbnz	REST, 4f

	/* fix up dest pointer if pitch != width */
7:	add	DST, PITCH
	add	DST2, PITCH

	/* fix up src pointer at end of line */
	subs	TLINE, #1
	itee	ne
	addne	SRC, NEXTLIN
	subeq	SRC, #992
	moveq	TLINE, #32

	subs	HEIGHT, #1
	bne	1b
	pop	{r4, r5, r6, r7, r8, r9, pc}

	/* partly copy last tile of line */
4:	mov	TMPSRC, SRC
	tst	REST, #8
	beq	5f
	vld2.8	{d0 - d1}, [TMPSRC :128]!
	vst1.8	{d0}, [DST]!
	vst1.8	{d1}, [DST2]!
5:	add	SRC, TSIZE
	ands	CNT, REST, #7
	beq	7b
6:	vld2.8	{d0[0], d1[0]}, [TMPSRC]!
	subs	CNT, #1
	vst1.8	{d0[0]}, [DST]!
	vst1.8	{d1[0]}, [DST2]!
	bne	6b
	b	7b
end_function tiled_deinterleave_to_planar

/*	$OpenBSD: types.h,v 1.12 1997/11/30 18:50:18 millert Exp $	*/
/*	$NetBSD: types.h,v 1.29 1996/11/15 22:48:25 jtc Exp $	*/

#ifndef _INC_TYPES_H_
#define _INC_TYPES_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif /* !NULL */

typedef __signed char              int8_t;
typedef unsigned char            u_int8_t;
typedef short                     int16_t;
typedef unsigned short          u_int16_t;
typedef int                       int32_t;
typedef unsigned int            u_int32_t;
typedef long long                 int64_t;
typedef unsigned long long      u_int64_t;

typedef int32_t                 register_t;

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

typedef	u_int64_t	u_quad_t;	/* quads */
typedef	int64_t		quad_t;
typedef	quad_t *	qaddr_t;

typedef u_int32_t        size_t;


#define MIN(_a, _b)	\
	({		\
		typeof(_a) __a = (_a);	\
		typeof(_b) __b = (_b);	\
		__a <= __b ? __a : __b;	\
	})

/* Static assert, for compile-time assertion checking */
#define static_assert(c) switch (c) case 0: case(c):

#define offsetof(type, member)  ((size_t)(&((type *)0)->member))

/* Rounding; only works for n = power of two */
#define ROUND(a, n)	(((((u_long)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n)	(((u_long)(a)) & ~((n)-1))


/*color definitions*/
/*
BLACK 0 ºÚ
BLUE 1 À¼ 
GREEN 2 ÂÌ
CYAN 3 Çà 
RED 4 ºì 
MAGENTA 5 Ñóºì
BROWN 6 ×Ø
LIGHTGRAY 7 µ­»Ò

DARKGRAY 8 Éî»Ò
LIGHTBLUE 9 µ­À¼ 
LIGHTGREEN a µ­ÂÌ
LIGHTCYAN b µ­Çà
LIGHTRED c µ­ºì
LIGHTMAGENTA d µ­Ñóºì 
YELLOW e »Æ 
WHITE f °×
*/

#define F_BLACK       0x0000
#define F_BLUE        0x0100
#define F_GREEN       0x0200
#define F_CYAN        0x0300
#define F_RED         0x0400
#define F_MAGENTA     0x0500
#define F_BROWN       0x0600
#define F_LIGHTGRAY   0x0700
#define F_DARKGRAY    0x0800
#define F_LIGHTBLUE   0x0900
#define F_LIGHTGREEN  0x0a00
#define F_LIGHTCYAN   0x0b00
#define F_LIGHTEAD    0x0c00
#define F_LIGHTMAGENT 0x0d00
#define F_YELLOW      0x0e00
#define F_WHITE       0x0f00

#define F_DEFAULT     0x0000

#define B_BLACK       0x0000
#define B_BLUE        0x1000
#define B_GREEN       0x2000
#define B_CYAN        0x3000
#define B_RED         0x4000
#define B_MAGENTA     0x5000
#define B_BROWN       0x6000
#define B_LIGHTGRAY   0x7000
#endif /* !_INC_TYPES_H_ */

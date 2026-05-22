#ifndef __INTRINS_H__
#define __INTRINS_H__

/* Intrinsics for 8051 */

#define _nop_() __asm nop __endasm

#define _testbit_(bit) __asm mov c,bit __endasm

extern void _cror_(unsigned char *, unsigned char);
extern void _crol_(unsigned char *, unsigned char);
extern void _iror_(unsigned char *, unsigned char);
extern void _irol_(unsigned char *, unsigned char);
extern unsigned char _lrot_(unsigned char, unsigned char);
extern unsigned char _lror_(unsigned char, unsigned char);
extern unsigned char _chkfloat_(float);

#endif
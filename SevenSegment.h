// SevenSegment.h

#ifndef _SEVENSEGMENT_h
#define _SEVENSEGMENT_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


//Standard 7-segment LED
//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---    DP
//      D
//
// D7 = DP
// D6 = a
// D5 = b
// D4 = c
// D3 = d
// D2 = e
// D1 = f
// D0 = g

// Some common special symbols
enum segs : byte { seg_G = 1, seg_F = (1 << 1), seg_E = (1 << 2), seg_D = (1 << 3), seg_C = (1 << 4), seg_B = (1 << 5), seg_A = (1 << 6), seg_DP = (1 << 7) };
//static const uint8_t seg_DP = ((uint8_t)1 << 7);

// Some common special symbols
enum dcd_segs : byte { SEG_BLANK = 0b00000000, SEG_DASH = 0b00000001, SEG_PCTL = 0b01100011, SEG_PCTR = 0b00011101, SEG_ALL_DP = 0b11111111, SEG_ALL = 0b01111111 };

static const byte dig_ndec[] =
{ //  No-Decode Mode Data Bits
  /*      0b0abcdefg */
	/* 0 */ 0b01111110,
	/* 1 */ 0b00110000,
	/* 2 */ 0b01101101,
	/* 3 */ 0b01111001,
	/* 4 */ 0b00110011,
	/* 5 */ 0b01011011,
	/* 6 */ 0b01011111,
	/* 7 */ 0b01110000,
	/* 8 */ 0b01111111,
	/* 9 */ 0b01111011,
// if not in use you can ommit the rest and save some memory
	/* A */ 0b01110111,
	/* b */ 0b00011111,
	/* c */ 0b00001101,
	/* d */ 0b00111101,
	/* E */ 0b01001111,
	/* F */ 0b01000111,
	/* G */ 0b01011110,
	/* H */ 0b00110111,
	/* I */ 0b00000110,
	/* J */ 0b00111100,
	/* k */ 0b00000111,
	/* L */ 0b00001110,
	/* m */ 0b00100011,
	/* n */ 0b00010101,
	/* o */ 0b00011101,
	/* P */ 0b01100111,
	/* q */ 0b01110011,
	/* r */ 0b00000101,
	/* S */ 0b01011011,
	/* t */ 0b00001111,
	/* u */ 0b00011100,
	/* v */ 0b00111110,
	/* w */ 0b00010011,
	/* x */ 0b00100101,
	/* y */ 0b00111011,
	/* Z */ 0b01101101,
	/* - */ 0b00000001,
	/*   */ 0b00000000,
	/* % */ 0b01100011,
	/* % */ 0b00011101
};


#endif


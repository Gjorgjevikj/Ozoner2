// SevenSegmentDirect.h

#ifndef _SEVENSEGMENTDIRECT_h
#define _SEVENSEGMENTDIRECT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "SevenSegment.h"

typedef uint8_t pinid_t;

/// <summary> DigitsMap Template Class </summary>
/// <remarks>
/// Implements a pin mapping for simple digit switching 
/// for direct control of 7-segment displays and interleaving
/// ACT determines digit selection level (whethere we are driving a Common Cathode (CC) or 
/// Common Anode (CA) display and wherethere we are driving them directly or using a driver circuit with opposite logic) 
/// For driving directly a CC ACT should be false (low level for sinking the current of all the segments of that digit)
/// the next arguments are the pins for the digits right (LSD) to left (MSD)
/// fully static class, uses variadic templates
/// can support any number of digits (currently limited to 8, can be easily extended to more if needed)
/// </remarks>
template < bool ACT, pinid_t... D >
class DigitsMap
{
private:
	/// <summary> pin numbers for the digits, 0-th beeing the rightmost digit </summary>
	const static pinid_t displayDigit[sizeof...(D)]; 
public:
	/// <summary> initializes the pins </summary>
	static void init()
	{
		const int dummy[sizeof...(D)] = { (pinMode(D, OUTPUT), 0)... };
		(void)dummy;
		const int dummy2[sizeof...(D)] = { (digitalWrite(D, !ACT), 0)... };
		(void)dummy2;
	}

	/// <summary> Turns on a digit </summary>
	/// <param name="i"> The digit to be turned on </param>
	static void digitOn(pinid_t i)
	{
		/// LOW for CC - sinks all the current of segments of digit i
		digitalWrite(displayDigit[i], ACT); 
	}

	/// <summary> Turns off a digit </summary>
	/// <param name="i"> The digit to be turned off </param>
	static void digitOff(pinid_t i)
	{
		/// HIGH for CC 
		digitalWrite(displayDigit[i], !ACT); 
	}

	/// <summary> Turns off all the digits </summary>
	static void allOff()
	{
		const int dummy[sizeof...(D)] = { (digitalWrite(D, !ACT), 0)... };
		(void)dummy;
	}
};

/// initialises the array that stores the pin number allocated for each of the digits
template < bool ACT, pinid_t... D >
const pinid_t DigitsMap<ACT, D...>::displayDigit[sizeof...(D)] = { D... };

/// <summary>
/// SegmentsMap Template Class - Implements a pin mapping for direct control of any-segment led display
/// </summary>
/// <remarks>
/// ACT determines level (whethere we are driving a CC or CA display directly or using a driver circuit) 
/// for direct driving a CC - common Cathode from the pin ACT should be true, false for Common Anode 
/// can also be specified to be LOW (false) if the pin is driving circuit for sourcing current that is activated by LOW level
/// the next arguments are the pins for the each of the segments 
/// fully static class, uses variadic templates, can support any number of segments
/// </remarks>
template < bool ACT, pinid_t... P >
class SegmentsMap
{
public:
	/// <summary> initializes the pins </summary>
	static void init()
	{
		const int dummy[sizeof...(P)] = { (pinMode(P, OUTPUT), 0)... };
		(void)dummy;
		const int dummy2[sizeof...(P)] = { (digitalWrite(P, !ACT), 0)... };
		(void)dummy2;
	}

	/// <summary> Set the digit to be displayed </summary>
	/// <param name="d"> The digit (symbol) to be displayed as combination of bits each determening where there the segment shuld be on or off </param>
	static void setSegments(uint8_t d) // d contains the bitmask of the active segments in (dp)abcdefg order
	{
		uint8_t m = 0b10000000;
		const int dummy[sizeof...(P)] = { (digitalWrite(P, (d & m) ? ACT : !ACT), m >>= 1)... };
		(void)dummy;
	}
};

/// <summary>
/// SevenSegmentPlusDP Template Class - pin mapping for 7-segment led display 
/// </summary>
/// <remarks>
/// Derived from SegmentsMap class Implements a pin mapping for direct control of 7-segment led display 
/// ACT determines level (whethere we are driving a CC or CA display directly or using a driver circuit) 
/// the pins attached to each of the segments in the following orded: dot point, segment_A to segment_G
/// </remarks>
template < bool ACT, pinid_t SegP, pinid_t SegA, pinid_t SegB, pinid_t SegC, pinid_t SegD, pinid_t SegE, pinid_t SegF, pinid_t SegG>
class SevenSegmentPlusDP : public SegmentsMap< ACT, SegP, SegA, SegB, SegC, SegD, SegE, SegF, SegG>
{ };

/// <summary>
/// SegmentDisplay Template Class - template for directly driven 7-segment display 
/// </summary>
/// <remarks>
/// the first argument is template for the 7-segmnt display (defininig active level and pins for segments)
/// DACT determines level for digit activation and 
/// than the pins for cselecting each of the digits, right to left
/// can support any number of digits (currently limited to 8, can be easily extended to more if needed)
/// supports setting the brightness (for all the digits)
/// supports blinking including setting the blink rate and duty, switching the blinking for of each digit separately
/// </remarks>
template < class SevenSegmentPlusDP,
	template < bool DACT, pinid_t... D > class digits, // pins for digits : 0=rightmost to leftmost 
	bool DACT, pinid_t... D >
	class SegmentDisplay
{
protected:
	// settings
	/// digits shown, 0 - rightmost digit, active segments encoded in bitmask
	uint8_t displayDigit[sizeof...(D)]; 

public:
	SegmentDisplay() 
		//: blinkCycleCounter(0), blinkDuration(100), ison(true), blinkMask(0), dutyMask(0)
	{  }

	/// <summary> initializes the display </summary>
	void init()
	{
		SevenSegmentPlusDP::init();
		digits<DACT, D...>::init();
		clear();
	}

	/// <summary> returns the physical number of digits of the display </summary> 
	/// <returns> the number of digits of the display </returns>
	uint8_t nDigits() const
	{
		return sizeof...(D); // scanD;
	}

	/// <summary> Clears the display turning all the segments of all the digits off </summary> 
	void clear()
	{
		for (int i = 0; i < (sizeof...(D)); i++)
			displayDigit[i] = SEG_BLANK; // initially blanks
	}

// Low level methods

	/// <summary> Set the signal to turn ON specified digit of the display </summary> 
	/// <param name="i"> specifies the digit index, 0 = rightmost </param>
	void turnOnDigit(uint8_t i)
	{
		digits<DACT, D...>::digitOn(i);
	}

	/// <summary> Set the signal to turn OFF specified digit of the display </summary> 
	/// <param name="i"> specifies the digit index, 0 = rightmost </param>
	void turnOffDigit(uint8_t i)
	{
		digits<DACT, D...>::digitOff(i);
	}

	/// <summary> Set the signals to lit the segments given as bitmask </summary> 
	/// <param name="d"> bitmask of the segments </param>
	void setSegs(uint8_t d)
	{
		SevenSegmentPlusDP::setSegments(d);
	}

	/// <summary> Set values of the segments to show a digit </summary> 
	/// <param name="i"> the digit to be shown </param>
	void setDigit(uint8_t i)
	{
		SevenSegmentPlusDP::setSegments(displayDigit[i]);
	}

	/// <summary> Turns ON the dot point on the specified digit </summary> 
	/// <param name="dd"> specifies the digit index, 0 = rightmost </param>
	void setDP(int dd)
	{
		displayDigit[dd] |= seg_DP; // 0b10000000; 
	}

	/// <summary> Turns OFF the dot point on the specified digit </summary> 
	/// <param name="dd"> specifies the digit index, 0 = rightmost </param>
	void resetDP(int dd)
	{
		displayDigit[dd] &= ~seg_DP; // 0b01111111;
	}

	/// <summary> Direct access to the mitmask of the i-th digit </summary> 
	/// <param name="i"> specifies the digit index, 0 = rightmost </param>
	uint8_t & operator[] (uint8_t i) // storing/accessing the segment mask of i-th digit
	{
		return displayDigit[i];
	}

	/// <summary> The service function that updates the display - Basic (no blinking, no brightness)
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <remark> Basic version - no brighness controll, no blinking supported
	/// updates one digit on each call cycling through all the digits 
	/// contains internal static counter to track digit multiplexing </remarks>
	void isrDisplayUpdate()
	{
		static uint8_t dig2upd = 0;
		isrDisplayUpdate(dig2upd++);
	}

	/// <summary> The service function that updates the display - Basic (no blinking, no brightness)
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <param name="dig2upd"> externaly sipplied cycle counter </param>
	/// <remark> Basic version - no brighness controll, no blinking supported
	/// updates one digit on each call cycling through all the digits </remarks>
	void isrDisplayUpdate(uint8_t dig2upd)
	{
		digits<DACT, D...>::digitOff(dig2upd++ % (sizeof...(D))); // scanD);
		SevenSegmentPlusDP::setSegments(displayDigit[dig2upd % (sizeof...(D))]); // scanD]);
		digits<DACT, D...>::digitOn(dig2upd % (sizeof...(D)));
	}

	/// <summary> Sets the digit c on position j of the display </summary> 
	/// <param name="c"> the digit to be displayed </param>
	/// <param name="j"> position of the digit </param>
	void show(char c, int j)
	{
		if (isdigit(c))
			displayDigit[j] = dig_ndec[c - '0'];
		else if (isalpha(toupper(c)))
			displayDigit[j] = dig_ndec[toupper(c) - 'A' + 10];
		else if (c == '-')
			displayDigit[j] = SEG_DASH;
		else
			displayDigit[j] = SEG_BLANK; // dig_ndec['Z' + 2 - 'A' + 10];
	}

// High level methods

	/// <summary> Displays an integer on the display </summary> 
	/// <remark> croped to the left </remark>
	/// <param name="n"> the number to be displayed </param>
	/// <param name="lz"> whtethere leading zeroes to be shown </param>
	void show(int n, bool lz = false)
	{
		int i = 0; // store the dcd code of the digits in displayDigit[], displayDigit[0] ->LSD
		bool sgn = true;
		if (n < 0)
		{
			sgn = false;
			n = -n;
		}

		do
		{
			displayDigit[i++] = dig_ndec[n % 10];
			n /= 10;
		} while (i < (sizeof...(D)) && n);

		if (!sgn)
		{
			displayDigit[i++] = SEG_DASH; // dig_ndec[36];
			lz = false;
		}
		while (i < (sizeof...(D)))
		{
			displayDigit[i++] = lz ? dig_ndec[0] : SEG_BLANK; // 0; //dig_ndec[37]; // leading zeroes or blanks
		}
	}

	/// <summary> Display the number x using dd decimal digits </summary> 
	/// <remark> croped to the left </remark>
	/// <param name="x"> the number to be displayed </param>
	/// <param name="dd"> the number of decimal places </param>
	void show(float x, int dd)
	{
		// convert and display it as integer 
		for (int i = 0; i < dd; i++)
			x *= 10;
		show(static_cast<int>(x));
		// add the decimal digit at the appropriate place
		if (dd)
			displayDigit[dd] |= seg_DP; // 0b10000000;
	}

	/// <summary> Display a message </summary> 
	/// <remark> croped to the right </remark>
	/// <param name="message"> the message to be shown </param>
	void show(char *message)
	{
		for (int i = 0, j = (sizeof...(D)) - 1; j >= 0 && message[i]; i++, j--)
		{
			if (isdigit(message[i]))
				displayDigit[j] = dig_ndec[message[i] - '0'];
			else if (isalpha(toupper(message[i])))
				displayDigit[j] = dig_ndec[toupper(message[i]) - 'A' + 10];
			else if (message[i] == '-')
				displayDigit[j] = SEG_DASH;
			else
				displayDigit[j] = SEG_BLANK;
		}
	}

};

template < class SevenSegmentPlusDP,
	template < bool DACT, pinid_t... D > class digits, // pins for digits : 0=rightmost to leftmost 
	bool DACT, pinid_t... D >
	class SegmentDisplayBlink : public SegmentDisplay<SevenSegmentPlusDP, digits, DACT, D...>
{
protected:
	// settings
	/// blink duration in update cycles
	uint16_t blinkDuration;
	/// digits that are blinking, bitmask 1 - blinking, LSB - rightmost digit
	uint8_t blinkMask;

	// internal operation
	/// cycle counter for blink (internal) - tracks the cycles for duty/blinking
	uint16_t blinkCycleCounter;
	/// the amount of time the digit is on in the cycle
	uint16_t blinkOnDuty;
	/// if digits are shown (on or off during blinking) 
	bool ison;

public:
	SegmentDisplayBlink() : SegmentDisplay<SevenSegmentPlusDP, digits, DACT, D...>(), 
		blinkCycleCounter(0), blinkDuration(100), ison(true), blinkMask(0)
	{  }

	/// <summary> initializes the display </summary>
	void init()
	{
		SegmentDisplay<SevenSegmentPlusDP, digits, DACT, D...>::init();
	}

	/// <summary> Sets the blinking period and duty </summary> 
	/// <param name="BlinkDuration"> blink duration in update cycles </param>
	/// <param name="BlinkDuty"> blink duty in percents (1-99) </param>
	/// <remark> The duration and the blinking duty are common for the whole display 
	/// but blinking can be turned on/off for each digit separately </remark>
	void setBlinkDuration(uint16_t BlinkDuration, uint8_t BlinkDuty = 50)
	{
		blinkDuration = BlinkDuration;
		setBlinkOnDuty(BlinkDuty);
	}

	/// <summary> Sets the blinking duty </summary> 
	/// <param name="BlinkDuty"> blink duty in percents (1-99) </param>
	void setBlinkOnDuty(uint8_t BlinkDuty)
	{
		blinkOnDuty = (unsigned long)BlinkDuty*blinkDuration / 100;
	}

	/// <summary> Get the blinking duty </summary> 
	/// <returns> blink duty in cycles <returns>
	unsigned long getBlinkOnDuty()
	{
		return blinkOnDuty;
	}

	/// <summary> Turns blinking on for a specified digit or the whole display </summary> 
	/// <param name="DigitToBlink"> index of the digit </param>
	void blinkOn(int8_t DigitToBlink = -1) // default -1 -> alldigits blink
	{
		if (DigitToBlink == -1)
			blinkMask = 0xff;
		else if (DigitToBlink >= 0)
			blinkMask |= (1 << DigitToBlink);
	}

	/// <summary> Turns blinking off for a specified digit or the whole display </summary> 
	/// <param name="DigitToBlink"> index of the digit </param>
	void blinkOff(int8_t DigitToBlink = -1) // default -1 -> alldigits stop blinking
	{
		if (DigitToBlink == -1)
			blinkMask = 0x00;
		else if (DigitToBlink >= 0)
			blinkMask &= ~(1 << DigitToBlink);
	}

	/// <summary> The service function that updates the display - Supports blinking 
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <remark> 
	/// updates one digit on each call cycling through all the digits 
	/// contains internal static counter to track digit multiplexing </remarks>
	void isrDisplayUpdateBlink()
	{
		static uint8_t dig2upd = 0;
		isrDisplayUpdateBlink(dig2upd++);
	}

	/// <summary> The service function that updates the display - Supports blinking 
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <param name="dig2upd"> externaly sipplied cycle counter </param>
	/// <remark> 
	/// update one digit on each call cycling through all the digits </remarks>
	void isrDisplayUpdate(uint8_t dig2upd)
	{
		if (blinkMask) // any digit is blinking
		{
			ison = (blinkCycleCounter++ < blinkOnDuty);
			if (blinkCycleCounter > blinkDuration)
				blinkCycleCounter = 0;
		}
		digits<DACT, D...>::digitOff(dig2upd++ % (sizeof...(D))); // scanD);
		uint8_t di = (dig2upd % (sizeof...(D)));
		if (ison || !(blinkMask & (1 << di))) // this digit is on during blinking or not blinking
		{
			setDigit(di); 
			digits<DACT, D...>::digitOn(di);
		}
	}
};

/// <summary>
/// Defined constants for setting the duty cycle when interleaving the digits
/// effectivly for brightness controll of the display
/// </summary>
/// <remark> The bits in the bitmapped value represent the refersh cycles in which the digits are on or off 
/// the 0 means skiped cycle, 0 is treated as special case and is not off all the time but on </remark> 
enum DutyPct : uint8_t {
	DUTY_100 = 0, DUTY_50 = 0b10,
	DUTY_33 = 0b100, DUTY_66 = 0b110,
	DUTY_25 = 0b1000, DUTY_75 = 0b1110,
	DUTY_20 = 0b10000, DUTY_40 = 0b10010, DUTY_60 = 0b10101, DUTY_80 = 0b11011
};

template < class SevenSegmentPlusDP,
	template < bool DACT, pinid_t... D > class digits, // pins for digits : 0=rightmost to leftmost 
	bool DACT, pinid_t... D >
	class SegmentDisplayBrighness : public SegmentDisplay<SevenSegmentPlusDP, digits, DACT, D...>
{
protected:
	/// bit combination that specifies duty cycle - 0 meaning 100%, see enum DutyPct
	uint8_t dutyMask;
	/// bitmapped shift register for duty control (internal)
	uint8_t currentDuty;

public:
	SegmentDisplayBrighness() : SegmentDisplay<SevenSegmentPlusDP, digits, DACT, D...>(),
		dutyMask(0), currentDuty(100)
	{  }

	/// <summary> initializes the display </summary>
	void init()
	{
		SegmentDisplay<SevenSegmentPlusDP, digits, DACT, D...>::init();
	}

	/// <summary> Sets the duty of the update cycle - controls the brightness </summary> 
	/// <param name="dMask"> bitmask for brightness controll </param>
	/// <seealso cref="enum DutyPct">see enum DutyPct</seealso>
	void setDutyMask(uint8_t dMask)
	{
		dutyMask = currentDuty = dMask;
	}

	/// <summary> Resets the duty to full brightness </summary> 
	void resetDutyMask()
	{
		setDutyMask(0);
	}

	/// <summary> The service function that updates the display - Supports brightness controll
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <remark> 
	/// updates one digit on each call cycling through all the digits 
	/// contains internal static counter to track digit multiplexing </remarks>
	void isrDisplayUpdate()
	{
		static uint8_t dig2upd = 0;
		isrDisplayUpdate(dig2upd++);
	}

	/// <summary> The service function that updates the display - Supports brightness controll
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <param name="dig2upd"> externaly sipplied cycle counter </param>
	/// <remark> 
	/// update one digit on each call cycling through all the digits </remarks>
	void isrDisplayUpdate(uint8_t dig2upd)
	{
		digits<DACT, D...>::digitOff(dig2upd++ % (sizeof...(D))); // scanD);
		uint8_t di = (dig2upd % (sizeof...(D)));

		if (dutyMask && !di) // duty defined (0 means 100%)
		{ // update the currentDuty ones per scann!
			currentDuty >>= 1;
			if (currentDuty == 0)
				currentDuty = dutyMask;
		}

		if (!dutyMask || (currentDuty & 1)) // no duty mask set (dutyMask == 0 mans 10%% duty) or duty on cycle
		{
			setDigit(di);
			digits<DACT, D...>::digitOn(di);
		}
	}
};

template < class SevenSegmentPlusDP,
	template < bool DACT, pinid_t... D > class digits, // pins for digits : 0=rightmost to leftmost 
	bool DACT, pinid_t... D >
	class SegmentDisplayBlinkDuty : public SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>
{
	using SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>::blinkMask;
	using SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>::ison;
	using SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>::blinkDuration;
	using SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>::blinkCycleCounter;
	using SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>::blinkOnDuty;

protected:
	/// bit combination that specifies duty cycle - 0 meaning 100%, see enum DutyPct
	uint8_t dutyMask;
	/// bitmapped shift register for duty control (internal)
	uint8_t currentDuty;

public:
	SegmentDisplayBlinkDuty() : SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>(),
		dutyMask(0), currentDuty(100)
	{  }

	/// <summary> initializes the display </summary>
	void init()
	{
		SegmentDisplayBlink<SevenSegmentPlusDP, digits, DACT, D...>::init();
	}

	/// <summary> Sets the duty of the update cycle - controls the brightness </summary> 
	/// <param name="dMask"> bitmask for brightness controll </param>
	/// <seealso cref="enum DutyPct">
	void setDutyMask(uint8_t dMask)
	{
		dutyMask = currentDuty = dMask;
	}

	/// <summary> Resets the duty to full brightness </summary> 
	void resetDutyMask()
	{
		setDutyMask(0);
	}

	/// <summary> The service function that updates the display - Supports brightness controll
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <remark> updates one digit on each call cycling through all the digits 
	/// contains internal static counter to track digit multiplexing </remarks>
	void isrDisplayUpdate()
	{
		static uint8_t dig2upd = 0;
		isrDisplayUpdate(dig2upd++);
	}

	/// <summary> The service function that updates the display - Supports blinking & brightness controll
	/// should be called repeatedly from a loop or from an timer ISR </summary> 
	/// <param name="dig2upd"> externaly sipplied cycle counter </param>
	/// <remark> update one digit on each call cycling through all the digits </remarks>
	void isrDisplayUpdate(uint8_t dig2upd)
	{
		if (blinkMask) // any digit is blinking
		{
			ison = (blinkCycleCounter++ < blinkOnDuty);
			if (blinkCycleCounter > blinkDuration)
				blinkCycleCounter = 0;
		}

		digits<DACT, D...>::digitOff(dig2upd++ % (sizeof...(D))); 
		uint8_t di = (dig2upd % (sizeof...(D)));

		if (dutyMask && !di) // duty defined (0 means 100%)
		{ // update the currentDuty ones per scann!
			currentDuty >>= 1;
			if (currentDuty == 0)
				currentDuty = dutyMask;
		}

		if (ison || !(blinkMask & (1 << di))) // this digit is on during blinking or not blinking
		{
			if (!dutyMask || (currentDuty & 1)) // no duty mask set (dutyMask == 0 mans 10%% duty) or duty on cycle
			{
				setDigit(di);
				digits<DACT, D...>::digitOn(di);
			}
		}
	}
};

#endif


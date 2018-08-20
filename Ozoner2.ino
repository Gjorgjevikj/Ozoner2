/*
    Name:       Ozoner2.ino
    Created:	23.07.2018 15:14:18
    Author:     Dejan-PC\Dejan
*/

#include <EEPROM.h>
#include <EEWrap.h> 

#include <TimerOne.h> 
//#include <TimerFreeTone.h> 

#include "SevenSegment.h"
#include "SevenSegmentDirect.h"
#include "PushButton.h"

#include "myDefs.h"

//#define DEBUGSERIAL


// Ozoner soldered
// Pins used:
// segment driving: D2, D3, D5, D6, D7, A1, A2, A3
const int segA = A3;  // >>  11
const int segB = A2; // >>  7
const int segC = A1; // >>  4
const int segD = D7;  // >>  2
const int segE = D6;  // >>  1
const int segF = D3;  // >>  10
const int segG = D2; // >>  5
const int segP = D5; // >>  3

// Digit selectors: D4, A0
const int d1 = D4;   // >> 8
const int d0 = A0;  // >> 6

// Passive buzzer: D8
const int irqBeepPin = D8;

//Push buttons
const int PB_PIN_PLUS = D12;
const int PB_PIN_MINUS = D10;
const int PB_PIN_START = D11;

const int LED_PIN = D13;
const int DEVICE_PIN = D9;

const int NTC_PIN = A7;

/*

// Ozoner protoboard

const int segA = A1;  // >>  11
const int segB = A2; // >>  7
const int segC = A3; // >>  4
const int segD = D7;  // >>  2
const int segE = D6;  // >>  1
const int segF = D5;  // >>  10
const int segG = D3; // >>  5
const int segP = D2; // >>  3

					 // Digit selectors: D6, D9, D10, D11
const int d3 = A5;   // >> 12
const int d2 = A4;   // >> 9
const int d1 = A0;   // >> 8
const int d0 = D4;  // >> 6

// Passive buzzer: D8
const int irqBeepPin = D8;

//Push buttons
const int PB_PIN_PLUS = D10;
const int PB_PIN_MINUS = D11;
const int PB_PIN_START = D12;
const int NTC_PIN = A7;

const int LED_PIN = D13;
const int DEVICE_PIN = D9;
*/
////////////////////////////////////////////////////////


// Global objects for devices
// Display Common Anode
SegmentDisplayBlinkDuty< SegmentsMap<LOW, segP, segA, segB, segC, segD, segE, segF, segG>, DigitsMap, HIGH, d0, d1> display;

#define DISP_M_READY ( seg_A | seg_G | seg_D )
#define DISP_M_STOP ( seg_B | seg_C | seg_E | seg_F)
#define DISP_M_UP_O ( seg_A | seg_B | seg_F | seg_G)
#define DISP_M_LOW_O ( seg_C | seg_D | seg_E | seg_G)

/*

// Pins used:
// segment driving: D2, D3, D4, D5, D7, A3, A4, A5

const int segA = A5;  // >>  11
const int segB = A3; // >>  7
const int segC = D4; // >>  4
const int segD = D3;  // >>  2
const int segE = D2;  // >>  1
const int segF = A4;  // >>  10
const int segG = D5; // >>  5
const int segP = D7; // >>  3

// Digit selectors: D6, D9, D10, D11
const int d3 = D6;   // >> 12
const int d2 = D9;   // >> 9
const int d1 = D10;   // >> 8
const int d0 = D11;  // >> 6

// Passive buzzer: D8
const int irqBeepPin = D8;

//Push buttons
const int PB_PIN_PLUS = A0;
const int PB_PIN_MINUS = A1;
const int PB_PIN_START = A2;

const int LED_PIN = D13;
const int DEVICE_PIN = D12;

// Global objects for devices
// Display
SegmentDisplayBlinkDuty< SegmentsMap<HIGH, segP, segA, segB, segC, segD, segE, segF, segG>, DigitsMap, LOW, d0, d1, d2, d3 > display;
*/

// Push buttons
PushButtonAutoAcceleratedRepeat<> ButtonPlus(PB_PIN_PLUS); // + button
PushButton2SpeedAutoRepeat<> ButtonMinus(PB_PIN_MINUS); // - button
PushButton<> ButtonStartStop(PB_PIN_START); // start / stop button

// Compile-time configurable constants
const unsigned long dispUpdTus = 200;

//Runtime globals
// Value to be changed by up/down

int val = 15, *pval = &val;
int cntDtime;

bool beepOn = false;
volatile uint8_t irqToneDiv = 0; // 0,1,2,3,4
byte defaultBrightness = DUTY_60;

uint8_e durs EEMEM; // duration 1-99
int8_e otpTemp EEMEM; // 30-85
uint8_e brightness EEMEM; // 1-5
uint32_e totalMinutesWork EEMEM;

const static byte brightnessMasks[] = { DUTY_20, DUTY_40, DUTY_60, DUTY_80, DUTY_100 }; // 0 is max brightness

float temperature = 25.0;

unsigned long devicerun;
float Vcc = 5.0;

// Usefull functions

long readVcc() {
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA, ADSC)); // measuring

	uint8_t low = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both

	long result = (high << 8) | low;

	//  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	result = 1126400  / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
}

// Changes a value of an integer by cahanveBy, checking bounds
bool intChange(int *pval, int changeBy, int lowLimit = -32768, int upLimit = 32767)
{
	if ((changeBy > 0) && (*pval > upLimit - changeBy) || (changeBy < 0) && (*pval < lowLimit - changeBy))
		return false;
	*pval += changeBy;
	return true;
}

// Changes a value of an integer by cahanveBy, checking bounds
bool longChange(unsigned long *pval, long changeBy, long lowLimit = 0, long upLimit = 99000)
{
	if ((changeBy > 0) && (*pval > upLimit - changeBy) || (changeBy < 0) && (*pval < lowLimit - changeBy))
		return false;
	*pval += changeBy;
	return true;
}

void pbService()
{
	intChange(pval, 1, 1, 99);
	tone(irqBeepPin, 8000, 20UL);
}

void mbService()
{
	intChange(pval, -1, 1, 99);
	tone(irqBeepPin, 6000, 20UL);
}

void pbServiceT()
{
	intChange(pval, 1, 30, 85);
	tone(irqBeepPin, 8000, 20UL);
}

void mbServiceT()
{
	intChange(pval, -1, 30, 85);
	tone(irqBeepPin, 6000, 20UL);
}

void pbServiceB()
{
	intChange(pval, 1, 1, 5);
	display.setDutyMask(brightnessMasks[*pval - 1]);
	tone(irqBeepPin, 8000, 20UL);
}

void mbServiceB()
{
	intChange(pval, -1, 1, 5);
	display.setDutyMask(brightnessMasks[*pval - 1]);
	tone(irqBeepPin, 6000, 20UL);
}


// Changes a value of an integer by cahangBy, checking bounds
void UpDownSet(int &v, void(*keypess_plusB)(), void(*keypess_minusB)())
{
	pval = &v;
	display.clear();
	display.show(v);

	display.setBlinkDuration(2000, 85);
	display.blinkOn();

	ButtonPlus.registerKeyPressCallback(keypess_plusB);
	ButtonMinus.registerKeyPressCallback(keypess_minusB);

	while (ButtonStartStop.stateChanged() != BUTTON_PRESSED) // press start button to exit seting
	{
		display.show(v);
		ButtonPlus.handle();
		ButtonMinus.handle();
		delay(1);
	}
	display.blinkOff();
	//beepOn = true;
	v = *pval;
}

void UpDownSetB(int &v, void(*keypess_plusB)(), void(*keypess_minusB)())
{
	pval = &v;
//	display.clear();
	display.show('b', 1);
	display.setDP(1);
	display.show((char)('0'+v),0);

	display.setBlinkDuration(2000, 85);
	display.blinkOn(0);

	ButtonPlus.registerKeyPressCallback(keypess_plusB);
	ButtonMinus.registerKeyPressCallback(keypess_minusB);

	while (ButtonStartStop.stateChanged() != BUTTON_PRESSED) // press start button to exit seting
	{
		display.show((char)('0' + v), 0);
		ButtonPlus.handle();
		ButtonMinus.handle();
		delay(1);
	}
	display.blinkOff();
	//beepOn = true;
	v = *pval;
}

inline void DeviceOn()
{
	digitalWrite(DEVICE_PIN, HIGH);   // turn the LED on 
#ifdef DEBUGSERIAL
	Serial.println("Device turned ON");
#endif
	devicerun = millis();
}

inline void DeviceOff()
{
	digitalWrite(DEVICE_PIN, LOW);   // turn the LED off 
#ifdef DEBUGSERIAL
	Serial.println("Device turned OFF");
	Serial.print("Device was running for: ");
	Serial.print(millis() - devicerun);
	Serial.println("ms.");
	Serial.print("totalMinutesWork was: ");
	Serial.println(totalMinutesWork);
#endif
	totalMinutesWork += (millis() - devicerun) / (1000UL * 60);
#ifdef DEBUGSERIAL
	Serial.print("totalMinutesWork changed to: ");
	Serial.println(totalMinutesWork);
#endif
}

inline void LedOn()
{
	digitalWrite(LED_PIN, HIGH);   // turn the LED on 
}

inline void LedOff()
{
	digitalWrite(LED_PIN, LOW);   // turn the LED off 
}

void toogleLED()
{
	static bool isOn = false;
	isOn = !isOn; // change the isPressed
	if (isOn)
		LedOn();   // turn the LED on 
	else
		LedOff();   // turn the LED off 
}

void StartupBlink()
{
	tone(irqBeepPin, 2000, 500UL);
	display.setSegs(SEG_ALL);
	for (byte i = 0; i < display.nDigits(); i++)
		display.turnOnDigit(i);
	for (byte i = 0; i < 3; i++)
	{
		LedOn();
		delay(100);
		LedOff();
		delay(100);
	}
	//delay(200);
	//LedOff();
	display.clear();
}


void Test()
{
	tone(irqBeepPin, 2000, 500UL);
	display.setSegs(SEG_ALL);
	for (byte i = 0; i < display.nDigits(); i++)
	{
		display.turnOnDigit(i);
		display.setSegs(seg_A);
		delay(1000);
		display.setSegs(seg_B);
		delay(1000);
		display.setSegs(seg_C);
		delay(1000);
		display.setSegs(seg_D);
		delay(1000);
		display.setSegs(seg_E);
		delay(1000);
		display.setSegs(seg_F);
		delay(1000);
		display.setSegs(seg_G);
		delay(1000);
		display.setSegs(seg_DP);
		delay(1000);
		display.turnOffDigit(i);
	}

	//LedOff();
	display.clear();
}

void updateDisplay(void)
{
	static uint8_t irqCnt = 0;
	static unsigned long blinkCnt = 0;

	display.isrDisplayUpdate(irqCnt++);

	if (beepOn)
		digitalWrite(irqBeepPin, (irqCnt >> irqToneDiv) % 2);
}

/*
byte CountDown(unsigned int s) // in seconds
{
	unsigned long t = s * 1000L;
	unsigned long start = millis();
	unsigned long endt = start + t;
	unsigned long us; // current ms

	bool callOnceFlag = false;
	bool beepAcive = false;
	//unsigned long val;

#ifdef DEBUGSERIAL	
	Serial.print("time received in CountDown ");
	Serial.print(s);
	Serial.println("s");
	Serial.println(start);
	Serial.println(t);
	Serial.println(endt);
	Serial.print("Counting down ");
	Serial.print(t / 1000);
	Serial.println(" seconds...");
#endif

	display.clear();
	display.setBlinkDuration(1000);
	display.blinkOff();
	while ((us = millis()) < endt)
	{
		unsigned long remms = endt - us;
		unsigned int rems = remms / 1000;

//		int remin = rems / 60;
//		int remsec = rems % 60;

//		display.show(char('0' + remin % 10), 2);
//		display.show(char('0' + remsec / 10), 1);
//		display.show(char('0' + remsec % 10), 0);
		display.show(rems);
		if (rems % 2)
			display.setDP(0);

		if (ButtonStartStop.stateChanged() == BUTTON_PRESSED)
		{
//			display.blinkOn();
			return (1); // stopped
		}
		if (ButtonPlus.stateChanged() == BUTTON_PRESSED)
		{
			// Plus 1 unit
			// endt += 1000;
			longChange(&endt, 1000, 1000, 99000);
		}
		if (ButtonMinus.stateChanged() == BUTTON_PRESSED)
		{
			// Plus 1 unit
			// endt -= 1000;
			longChange(&endt, -1000, 1000, 99000);
		}
	}
//	display.blinkOff();
	return(0); // finshed regularly

}

byte CountDownM(unsigned int m) // in minutes
{
	unsigned long t = m * 60L * 1000L;
	unsigned long start = millis();
	unsigned long endt = start + t;
	unsigned long us; // current ms

	bool beepAcive = false;

#ifdef DEBUGSERIAL	
	Serial.print("time received in CountDown ");
	Serial.print(m);
	Serial.println("s");
	Serial.println(start);
	Serial.println(t);
	Serial.println(endt);
	Serial.print("Counting down ");
	Serial.print(t / 1000);
	Serial.println(" seconds...");
#endif

	display.clear();
	//display.setBlinkDuration(1000);
	display.blinkOff();
	while ((us = millis()) < endt)
	{
		unsigned long remms = endt - us;
		unsigned int rems = remms / 1000;

		//		int remin = rems / 60;
		//		int remsec = rems % 60;

		//		display.show(char('0' + remin % 10), 2);
		//		display.show(char('0' + remsec / 10), 1);
		//		display.show(char('0' + remsec % 10), 0);
		display.show(rems/60+1);
		if (rems % 2)
			display.setDP(0);

		if (ButtonStartStop.stateChanged() == BUTTON_PRESSED)
		{
			//			display.blinkOn();
			return (1); // stopped
		}
		if (ButtonPlus.stateChanged() == BUTTON_PRESSED)
		{
			// Plus 1 unit
			// endt += 1000;
#ifdef DEBUGSERIAL
			Serial.print("endt was ");
			Serial.println(endt);
#endif
			longChange(&endt, 1000*60L, 1000*60L, 99000*60L);
#ifdef DEBUGSERIAL
			Serial.print("endt changed to ");
			Serial.println(endt);
#endif
		}
		if (ButtonMinus.stateChanged() == BUTTON_PRESSED)
		{
			// Plus 1 unit
			// endt -= 1000;
#ifdef DEBUGSERIAL
			Serial.print("endt was ");
			Serial.println(endt);
#endif
			longChange(&endt, -1000*60L, 1000*60L, 99000*60L);
#ifdef DEBUGSERIAL
			Serial.print("endt changed to ");
			Serial.println(endt);
#endif
		}
	}
	//	display.blinkOff();
	return(0); // finshed regularly
}
*/

// get the temperature from an NTC thermistor connected to A7
float readTemp()
{
	float average, steinhart;
	// measured...
/*	const long thermNom = 10500;
	const long bCoef = 4600;
	const long r0 = 99600;
	*/
	const long thermNom = 10500;
	const long bCoef = 4600;
	const long r0 = 98500;

	const float tempNomK = 273.15 + 25;
	
	//	for (int i = 0; i< NUMSAMPLES; i++)
		average = analogRead(NTC_PIN);
	//average /= NUMSAMPLES;
	//int aval = analogRead(ANALOGIN);

	float va = (average*1.1) / 1023;

	long r = va / (Vcc - va)*r0;
	average = r;

	steinhart = average / thermNom;     // (R/Ro)
	steinhart = log(steinhart);                  // ln(R/Ro)
	steinhart /= bCoef;                   // 1/B * ln(R/Ro)
	steinhart += 1.0 / tempNomK; // + (1/To)
	steinhart = 1.0 / steinhart;                 // Invert
	steinhart -= 273.15;                         // convert to C

	//Serial.println(steinhart);
	return steinhart;
}

byte CountDownMs(int &m) // in minutes
{
	unsigned long t = m * 60L * 1000L; // in milliseconds
	unsigned long start = millis();
	unsigned long endt = start + t;
	unsigned long us; // current ms
	bool beepAcive = false;
	int eventcouter = 0;

#ifdef DEBUGSERIAL	
	Serial.print("time received in CountDown ");
	Serial.print(m);
	Serial.println(" min.");
	Serial.println(start);
	Serial.println(t);
	Serial.println(endt);
	Serial.print("Counting down ");
	Serial.print(t / 1000);
	Serial.println(" sec...");
#endif

	display.clear();
	//display.setBlinkDuration(1000);
	display.blinkOff();
	display.setBlinkOnDuty(75);
	while ((us = millis()) < endt)
	{
		unsigned long remms = endt - us;
		unsigned int rems = remms / 1000;
		int remm = rems / 60 + 1;

		if (ButtonPlus.isPressed() && ButtonMinus.isPressed())
		{
			display.blinkOff();
			display.show((int)temperature);
		}
		else
		{
			if (rems < 60)
			{
				display.show(rems + 1);
				display.blinkOn();
			}
			else
			{
				display.blinkOff();
				display.show(remm);
				if (rems % 2)
				{
					display.setDP(0);
					LedOn();
				}
				else
					LedOff();
			}
		}
		if (ButtonStartStop.stateChanged() == BUTTON_PRESSED)
		{
			display.blinkOff();
			m = remm;
			return (1); // stopped
		}

		int premm = remm;
		pval = &remm;
		if (!(ButtonPlus.isPressed() && ButtonMinus.isPressed()))
		{
			ButtonPlus.handle();
			ButtonMinus.handle();
		}
		
		if (premm != remm) // increased / decreased
		{
#ifdef DEBUGSERIAL
			Serial.print("endt was ");
			Serial.println(endt);
#endif
			longChange(&endt, (remm - premm) * 1000L * 60L, 1000 * 60L, 99000 * 60L);
#ifdef DEBUGSERIAL
			Serial.print("endt changed to ");
			Serial.println(endt);
#endif
		} 

		//temperature = readTemp();
		if (us % 1000 == 0)
		{
#ifdef DEBUGSERIAL
			Serial.println(temperature);
#endif
			temperature = temperature * 0.85 + 0.15 * readTemp();
		//delay(1);
			if (temperature > otpTemp)
			{
				eventcouter++;
				Serial.print('+');
				if (eventcouter > 10)
				{
#ifdef DEBUGSERIAL
					Serial.println();
					Serial.print("Temperature too high. OTP activated! T=");
					Serial.println(temperature);
#endif
					m = remm;
					return(2);
				}
			}
			else
				eventcouter = 0;
		}
	}
	display.blinkOff();
	return(0); // finshed regularly
}

byte CountEnded() 
{
//	display.clear();
	display.show("00");
	display.blinkOff();
	unsigned long updperiod = 100, lastupd = millis();
	//const uint8_t brightnesslevel[] = { DUTY_20, DUTY_40, DUTY_60, DUTY_80, DUTY_100 };
	//brightnessMasks[]
	uint8_t brightneidxchange = 1, brightneidx = 0;

	//beepOn = true;
	while (ButtonStartStop.stateChanged() != BUTTON_RELEASED
		&& ButtonPlus.stateChanged() != BUTTON_RELEASED
		&& ButtonMinus.stateChanged() != BUTTON_RELEASED)
	{
/*		if (millis() - lastupd > updperiod)
		{
			lastupd = millis();
			if (display[0] == SEG_BLANK)
				display[0] = SEG_DASH;
			else
				display[0] = SEG_BLANK;
		}*/
		if (millis() - lastupd > updperiod)
		{
			lastupd = millis();
			if (brightneidx == 0)
				brightneidxchange = 1;
			else if (brightneidx == 4)
				brightneidxchange = -1;
			brightneidx += brightneidxchange;
			//brightnessc % sizeof(brightnesslevel)
			display.setDutyMask(brightnessMasks[brightneidx]);
		}

		if (ButtonStartStop.isPressed() || ButtonPlus.isPressed() || ButtonMinus.isPressed())
		{
			display.show("--");
			tone(irqBeepPin, 1000, 200UL);
//			beepOn = true;
		}
		delay(1);
	}
	//beepOn = false;
	display.setDutyMask(defaultBrightness);
	return(0); // finshed regularly
}

byte Waiting()
{
/*
const static byte cycle2dig[8][2] = {
		{ seg_A, 0 },
		{ seg_B, 0 },
		{ seg_C, 0 },
		{ seg_D, 0 },
		{ 0, seg_D },
		{ 0, seg_E },
		{ 0, seg_F },
		{ 0, seg_A },
	};
	*/
	const static byte cycle2dig[8][2] = {
		{ seg_A | seg_B, 0 },
		{ seg_B | seg_C, 0 },
		{ seg_C | seg_D, 0 },
		{ seg_D, seg_D },
		{ 0, seg_D | seg_E },
		{ 0, seg_E | seg_F },
		{ 0, seg_F | seg_A },
		{ seg_A, seg_A },
	};
	int stateIndex = 0;
	display.blinkOff();
	unsigned long updperiod = 250, lastupd = millis();

	//beepOn = true;
	while (ButtonStartStop.stateChanged() != BUTTON_RELEASED
		&& ButtonPlus.stateChanged() != BUTTON_RELEASED
		&& ButtonMinus.stateChanged() != BUTTON_RELEASED)
	{
		if (millis() - lastupd > updperiod)
		{
			lastupd = millis();
			if (stateIndex > 7)
				stateIndex = 0;
			display[0]= cycle2dig[stateIndex][0];
			display[1] = cycle2dig[stateIndex][1];
			stateIndex++;
		}

		if (ButtonStartStop.isPressed() || ButtonPlus.isPressed() || ButtonMinus.isPressed())
		{
			display.show("--");
			tone(irqBeepPin, 1000, 200UL);
		}
		delay(1);
	}
	display.setDutyMask(defaultBrightness);
	return(0); // finshed regularly
}



void setup()
{
#ifdef DEBUGSERIAL	
	Serial.begin(9600);
	delay(200);
	Serial.println("Start.");
#endif
	pinMode(LED_PIN, OUTPUT);
	pinMode(DEVICE_PIN, OUTPUT);
	pinMode(irqBeepPin, OUTPUT);
	digitalWrite(DEVICE_PIN, LOW);
	
	display.init();
	ButtonPlus.init();
	ButtonMinus.init();
	ButtonStartStop.init();

	// init and test
	delay(10);
	Vcc = readVcc() / 1000.0;
#ifdef DEBUGSERIAL	
	Serial.print("Vcc=");
	Serial.println(Vcc);
#endif

	StartupBlink();
	//Test();

	Timer1.initialize(dispUpdTus);
	Timer1.attachInterrupt(updateDisplay); // display update ISR 

	if (ButtonStartStop.isPressed()) // enter setup...
	{
		if (ButtonPlus.isPressed()) // reset all EEPROM to default
		{
#ifdef DEBUGSERIAL	
			Serial.println("Resetting EEPROM!!! ");
#endif
			durs = 10;
			otpTemp = 40;
			brightness = 3;
			totalMinutesWork = 61;
		}

#ifdef DEBUGSERIAL	
		Serial.print("The device has worked for: ");
		Serial.println(totalMinutesWork);
#endif
		// Show total work time 
		tone(irqBeepPin, 1200, 200);
		if (totalMinutesWork<99UL)
			display.show(totalMinutesWork);
		else if (totalMinutesWork<10 * 60UL)
			display.show((float)totalMinutesWork / 60, 1);
		else
		{
			display.show(totalMinutesWork / 60);
			display.setDP(0);
		}
		delay(200);
		tone(irqBeepPin, 1200, 200);

		while (ButtonStartStop.stateChanged() != BUTTON_RELEASED)
			;

// Set over temperature protection switch off temperature
		display.setBlinkOnDuty(50);
		display.setBlinkDuration(1000);
		display.show("ot");
		display.setDP(0);
		display.setDP(1);
		display.blinkOn();
		delay(500);
		while (ButtonStartStop.stateChanged() != BUTTON_PRESSED)
			;

		int otpTemp2set = otpTemp;
#ifdef DEBUGSERIAL	
		Serial.print("otp was: ");
		Serial.println(otpTemp2set);
#endif
		UpDownSet(otpTemp2set, pbServiceT, mbServiceT);
		otpTemp = otpTemp2set;
		tone(irqBeepPin, 1500, 400);

#ifdef DEBUGSERIAL	
		Serial.print("otp changed to: ");
		Serial.println(otpTemp2set);
#endif
		while (ButtonStartStop.stateChanged() != BUTTON_RELEASED)
			;

// Set dispaly brightness
		display.show('b',1);
		display.setDP(1);
		display.show((char)('0' + brightness), 0);
		delay(500);

		int brightness2set = brightness;
#ifdef DEBUGSERIAL	
		Serial.print("brightness was: ");
		Serial.println(brightness2set);
#endif
		UpDownSetB(brightness2set, pbServiceB, mbServiceB);
		brightness = brightness2set;
		tone(irqBeepPin, 1500, 400);
#ifdef DEBUGSERIAL	
		Serial.print("brightness changed to: ");
		Serial.println(brightness2set);
#endif
		while (ButtonStartStop.stateChanged() != BUTTON_RELEASED)
			;

	}
	analogReference(INTERNAL);
	delay(10);

	temperature = 0.0;
	const int repeatTempMeasurments = 5;
	readTemp();
	delay(25);

	//test analog reads...
	// if the NTC thermistor is discconected high values (>1020) will be received)
	int readings = 0;
	for (int i = 0; i < repeatTempMeasurments; i++)
		readings += analogRead(NTC_PIN);
	readings /= repeatTempMeasurments;

	if (readings > 1020)
	{
		display.setBlinkDuration(3000);
		display.blinkOn();
		display.show("Et");
		while (ButtonStartStop.stateChanged() != BUTTON_RELEASED)
		{
			tone(irqBeepPin, 400, 3000UL);
			delay(5000);
		}
	}

/*
	for (int i = 0; i < repeatTempMeasurments; i++)
	{
		//temperature = temperature * 0.8 + readTemp() * 0.2;
		float t = readTemp();
		temperature += t;
#ifdef DEBUGSERIAL	
		Serial.print(analogRead(NTC_PIN));
		Serial.print("  t=");
		Serial.println(t);
#endif
		delay(1);
	}
	temperature /= repeatTempMeasurments;

#ifdef DEBUGSERIAL	
	Serial.print("Initial temperature: ");
	Serial.println(temperature);
#endif
*/

	temperature = readTemp();
#ifdef DEBUGSERIAL	
	Serial.print("Initial temperature: ");
	Serial.println(temperature);
#endif

	if (durs > 99 || durs < 1)
		durs = 10;

	if (otpTemp < 25 || otpTemp > 85)
		otpTemp = 55;

	if (brightness < 1 || brightness > 5)
		brightness = 3;

	defaultBrightness = brightnessMasks[brightness - 1];

	display.setDutyMask(defaultBrightness);

	//display.show((int)temperature);

	cntDtime = durs;


	/// testing
	// reset EEPROM parameters
	// totalMinutesWork = 11*60UL;
/*	
	durs = 10;
	otpTemp = 30; 
	brightness = 3;
	totalMinutesWork = 55;
*/
	//otpTemp = 30;

}

void loop()
{
	//display.show(val);
	UpDownSet(cntDtime, pbService, mbService);
	durs = cntDtime;
#ifdef DEBUGSERIAL	
	Serial.print("Seted time");
	Serial.print(cntDtime);
	Serial.println("s");
#endif

	delay(100);
	display.show(cntDtime);
	tone(irqBeepPin, 2000, 100UL);

	DeviceOn();
	byte r = CountDownMs(cntDtime);
	DeviceOff();

	if (r == 1) // stopped prematurely
	{
		tone(irqBeepPin, 500, 1200UL);
		display.show("--");
		// wait for press and release
		while (ButtonStartStop.stateChanged() != BUTTON_RELEASED)
			;

		display.blinkOn();
		display.setBlinkDuration(600);
		display.show(cntDtime);
		delay(200);

		while (ButtonStartStop.stateChanged() != BUTTON_PRESSED)
			;
		tone(irqBeepPin, 1000, 200);
		while (ButtonStartStop.stateChanged() != BUTTON_RELEASED)
			;

		display.blinkOff();
	}
	else if (r == 2) // over temperature protection
	{
#ifdef DEBUGSERIAL
		Serial.println("OTP reported!!!");
#endif

		display.show("ot");
		display.blinkOn();
		display.setBlinkDuration(2000);
		int cyclcnt = 0;

		while((ButtonStartStop.stateChanged() != BUTTON_RELEASED))
		{
			cyclcnt++;
			if (cyclcnt == 1)
			{
				display.show("ot");
				display.setBlinkDuration(2000);
				tone(irqBeepPin, 500);
			}
			else if (cyclcnt == 500)
			{
				tone(irqBeepPin, 1000);
			}
			else if (cyclcnt == 2000)
			{
				display.show(cntDtime);
				display.setBlinkDuration(600);
				noTone(irqBeepPin);
#ifdef DEBUGSERIAL	
				Serial.println(temperature);
#endif
			}
			else if (cyclcnt > 5000)
				cyclcnt=0;
			delay(1);
		}
		noTone(irqBeepPin);
		display.blinkOff();
	}
	else
	{
		//display.show("end ");
		display[0] = DISP_M_UP_O;
		display[1] = DISP_M_LOW_O;
		//display.setBlinkDuration(4000, 50);
		//display.blinkOn();
		for (int i = 0; i < 4; i ++)
		{
			tone(irqBeepPin, 1000, 1500UL);
			for (int j = 0; j < 5; j++)
			{
				display[0] = DISP_M_UP_O;
				display[1] = DISP_M_LOW_O;
				delay(500);
				display[1] = DISP_M_UP_O;
				display[0] = DISP_M_LOW_O;
				delay(500);
			}
			if (ButtonStartStop.stateChanged() == BUTTON_RELEASED)
				break;
			//delay(2000);
			//noTone(irqBeepPin);
			//delay(5000);
		}
		//tone(irqBeepPin, 1000, 1500UL);
		//delay(2000);

/*		tone(irqBeepPin, 2000);
		delay(1500);
		tone(irqBeepPin, 500);
		delay(1000);
		noTone(irqBeepPin);*/
		Waiting();
	}
	//delay(500);
	//CountEnded();
}

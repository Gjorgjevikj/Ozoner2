// PushButton.h
// (yet another) set of classes for pushbutton debouncing
// supports active high and active low pusbuttons
// debouncinh, autorepeat and acceleration of autorepeat
// (c) Dejan Gjorgjevikj, 2018

#ifndef _PUSHBUTTON_h
#define _PUSHBUTTON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

enum ButtonStateChange { BUTTON_NOCHANGE, BUTTON_PRESSED, BUTTON_RELEASED };

/// <summary>
/// PushButton Template Class 
/// </summary>
/// <remarks>
/// Implements a basic push button functionality with debouncing
/// ACT determines active low = false (default) 
/// - push button that connects to GND when pressed 
/// or active high push button
/// </remarks>
template < bool ACT = false >
class PushButton
{
 protected:
// configuration
	 /// <summary> the pin the button is connected to </summary>
	 byte pin;
	 /// <summary> the time for debouncing (maybe should be static ... if common for all buttons) </summary>
	 unsigned long debounceDelay; 

// operation
/// <summary> the last time the output pin was toggled </summary> 
	 unsigned long stateChangedTimeStamp;  
/// <summary> true while waiting for button to stabilize - waiting for the debunce time to pass </summary> 
	 bool debounceWaiting : 1; 
/// <summary> the previous state of the button </summary> 
	 bool previousButtonState : 1 ;

/// <summary> !!! used for autorepeat feature in derived class PushButtonAutoRepeat but allocated here as bit field to save memory </summary> 
	 bool singlePress : 1 ; 

 public:
	 /// <summary> PushButton constructor </summary>
	 /// <param name="pbPin"> The pin the button is connected to </param>
	 /// <param name="DebounceDelay"> The delay in milliseconds for debouncing </param>
	PushButton(byte pbPin, unsigned long DebounceDelay = 50) : 
		pin(pbPin), debounceDelay(DebounceDelay), 
		debounceWaiting(false), previousButtonState(false) 
	{ }

	/// <summary> initializes the PushButton object </summary>
	void init()
	{
		pinMode(pin, ACT ? INPUT : INPUT_PULLUP); // acive low	
		// stateChangedTimeStamp = 0;
		debounceWaiting = false; // not in debounce
		previousButtonState = false; // not pressed
	}

	/// <summary> The button press has been registered before autorepeat is triggered </summary>
	/// <returns> Has the button press already been registered </returns>
	/// <remarks> provides acces to bitmapped varibale in the base class for derived classes 
	/// Workaround for Arduino compiler (avr-gcc and others) problems https://pastebin.com/hFNx3FVG
	/// as accesing protected members of a template base class from indirectly derived classes is not suppoted by this compiler 
	/// as of 22.07.2018 </remarks>
	bool getSinglePress()
	{
		return singlePress;
	}
	/// <summary> Mark that the button press has been registered before autorepeat is triggered </summary>
	/// <remarks> provides acces to bitmapped varibale in the base class for derived classes 
	/// Workaround for Arduino compiler (avr-gcc and others) problems https://pastebin.com/hFNx3FVG
	/// as accesing protected members of a template base class from indirectly derived classes is not suppoted by this compiler 
	/// as of 22.07.2018 </remarks>
	void setSinglePress(bool v)
	{
		singlePress = v;
	}

	/// <summary> Sets the debouncing delay </summary>
	/// <param name="delay"> The delay in milliseconds for debouncing </param>
	void setDebounceDelay(unsigned long delay)
	{
		debounceDelay = delay;
	}

	/// <summary> Gets the debouncing delay </summary>
	/// <returns> the debouncing delay in milliseconds </returns>
	unsigned long getDebounceDelay() const
	{
		return debounceDelay;
	}

	/// <summary> Is the button (held) pressed or not </summary>
	/// <returns> true if the button is pressed in the moment </returns>
	bool isPressed()
	{
		// if ACT is false (button active low) bbeing pressedDebounced means reding low signal
		return ACT ? digitalRead(pin) : !digitalRead(pin);
	}

	/// <summary> Detect state change of a button </summary>
	/// <returns> BUTTON_PRESSED, BUTTON_RELEASED or BUTTON_NOCHANGE </returns>
	/// <remark> returns BUTTON_PRESSED if the button has been pressed, but only after the debounce time has passed 
	/// BUTTON_RELEASED if the button has been released but only after the debounce time has passed 
	/// otherwise returns BUTTON_NOCHANGE.
	/// To be called repeatidly in a loop </remark>
	byte stateChanged()
	{
		byte r = BUTTON_NOCHANGE; 
		bool currentButtonState = isPressed(); // read the state of the button

		if (debounceWaiting) // are we waiting for the debounce period to pass?
		{
			if (millis() - stateChangedTimeStamp > debounceDelay) // been waiting long enough
			{
				if (previousButtonState ^ currentButtonState) // still the same
					r = (previousButtonState<<1) | currentButtonState;
				debounceWaiting = false;
				previousButtonState = currentButtonState;
			}
		}
		else // !debounceWaiting
		{
			if (previousButtonState ^ currentButtonState) // has been pressedDebounced (was up and is down now)
			{
				debounceWaiting = true;
				stateChangedTimeStamp = millis(); // record the time
			}
		}
		return r;
	}
};

/// <summary>
/// PushButtonAutoRepeat Template Class 
/// </summary>
/// <remarks>
/// Implements a push button functionality with debouncing and autorepeat features
/// ACT determines active low = false (default) 
/// - push button that connects to GND when pressed 
/// or active high push button
/// adds autorepeat features and callback function to be called automaticaly when the button is pressedDebounced
/// keeps track of the time the button has been held pressedDebounced
/// </remarks>
template < bool ACT = false >
class PushButtonAutoRepeat : public PushButton<ACT>
{
	using PushButton<ACT>::debounceWaiting;
	using PushButton<ACT>::singlePress;
	using PushButton<ACT>::stateChangedTimeStamp;
	using PushButton<ACT>::debounceDelay;
	using PushButton<ACT>::previousButtonState;

protected:
	/// <summary> the delay before autorepeat begins </summary> 
	unsigned long repeatDelay; 
	/// <summary> the autorepeating period </summary> 
	unsigned long repeatPeriod; 
	/// <summary> the function to be called on each keypress event </summary> 
	void(*keyPressCallback)();

	// operational
	/// <summary> the time the last update was called in autorepeat </summary> 
	unsigned long lastChangeTime; 

public:
	/// <summary> PushButtonAutoRepeat constructor </summary>
	/// <param name="pbPin"> The pin the button is connected to </param>
	/// <param name="KeyPressCallBackFunction"> The function to be called on each keypress </param>
	/// <param name="RepeatDelay"> The delay in milliseconds before autorepeat begins </param>
	/// <param name="AutoRepeatingPeriod"> The period in milliseconds at which a new keypress will be automatically produced </param>
	/// <param name="DebounceDelay"> The delay in milliseconds for debouncing </param>
	PushButtonAutoRepeat(byte pbPin, void(*KeyPressCallBackFunction)() = NULL, 
		unsigned long RepeatDelay = 500, unsigned long AutoRepeatingPeriod = 200, unsigned long DebounceDelay = 50) :
		PushButton<ACT>(pbPin, DebounceDelay), keyPressCallback(KeyPressCallBackFunction), 
		repeatDelay(RepeatDelay), repeatPeriod(AutoRepeatingPeriod)
	{ 
		singlePress = false;
	}

	/// <summary> initializes the PushButtonAutoRepeat object </summary>
	void init()
	{
		PushButton<ACT>::init();
		singlePress = false;
	}

	/// <summary> Registers the callback function to be called on each keypress </summary>
	/// <param name="keyPressFunction"> The function to be called on each keypress </param>
	void registerKeyPressCallback(void(*keyPressFunction)())
	{
		keyPressCallback = keyPressFunction;
	}

	/// <summary> Sets the delay the button has to be hold pressed before autorepet starts </summary>
	/// <param name="RepeatDelay"> The delay in milliseconds before autorepeat begins </param>
	void setRepeatDelay(unsigned long RepeatDelay)
	{
		repeatDelay = RepeatDelay;
	}

	/// <summary> get the delay the button has to be hold pressed before autorepet starts </summary>
	/// <returns> The delay in milliseconds before autorepeat begins </returns>
	unsigned long getRepeatDelay() const
	{
		return repeatDelay;
	}
	
	/// <summary> Sets the period at which a new keypress will be automatically produced</summary>
	/// <param name="AutoRepeatingPeriod"> The period in milliseconds at which a new keypress will be autotically produced </param>
	void setRepeatPeriod(unsigned long AutoRepeatingPeriod)
	{
		repeatPeriod = AutoRepeatingPeriod;
	}

	/// <summary> Gets the period at which a new keypress will be automatically produced</summary>
	/// <returns> The period in milliseconds at which a new keypress is autotically produced </returns>
	unsigned long getRepeatPeriod() const
	{
		return repeatPeriod;
	}

	/// <summary> For how long the button has is pressed </summary>
	/// <returns> the time in milliseconds the button is beeing held pressed </returns>
	unsigned long heldDown()
	{
		unsigned long r = 0UL;
		int currentButtonState = isPressed();

		if (debounceWaiting)
		{
			unsigned long duration = millis() - stateChangedTimeStamp;
			if (!previousButtonState && currentButtonState) // was Up and is Down now (hass been pressedDebounced)
				r = duration;
			else
				r = 0;
			debounceWaiting = false;
			previousButtonState = currentButtonState;
		}
		else // !debounceWaiting
		{
			if (!previousButtonState && currentButtonState) // has just been pressed 
			{
				debounceWaiting = true; // start debounceWaiting (waiting for debounceDelay to stabilize)
				stateChangedTimeStamp = millis(); // record the time waiting started
			}
			else // no change in isPressed (either was and is up, or was and is down)
			{
				if (previousButtonState && currentButtonState) // was and still is down (beeing held pressedDebounced)
					r = millis() - stateChangedTimeStamp;
				else
					previousButtonState = currentButtonState;
			}
		}
		return r;
	}

	/// <summary> To be called repeatidly in a loop (services autorepeating calls) </summary>
	/// <remark> The callback function has to be registered first
	/// keeps track on the button isPressed nad calls the callback function 
	/// one call after the debounce time has passed and than if the button is still beeing held down 
	/// for more than repeatDelay ms, autorepeat starts calling the function every repeatPeriod ms </remark> 
	void handle()
	{
		unsigned long bpDur = heldDown(); // will return value > 0 if the button is pressed
		if (!singlePress && bpDur > debounceDelay) // first notice of this keypress
		{
			keyPressCallback();
			singlePress = true;
			lastChangeTime = millis();
		}
		if (bpDur > repeatDelay) // has been held pressedDebounced for more than repeatDelat ms
		{
			if (millis() - lastChangeTime > repeatPeriod) // if the period for repeating has passed, fire another press
			{
				keyPressCallback();
				lastChangeTime = millis();
			}
		}
		if (bpDur == 0) // button is released - reset isPressed
			singlePress = false;
	}
};

/// <summary>
/// PushButton2SpeedAutoRepeat Template Class 
/// </summary>
/// <remarks>
/// Implements a push button functionality with debouncing and autorepeat feature with slow and faster speed
/// starts to repete faster after the button has been held longer
/// ACT determines active low = false (default) 
/// - push button that connects to GND when pressed 
/// or active high push button
/// adds autorepeat features and callback function to be called automaticaly when the button is pressedDebounced
/// keeps track of the time the button has been held pressed
/// </remarks>
template < bool ACT = false >
class PushButton2SpeedAutoRepeat : public PushButtonAutoRepeat<ACT>
{
	using PushButtonAutoRepeat<ACT>::lastChangeTime;
	using PushButtonAutoRepeat<ACT>::repeatDelay;
	using PushButtonAutoRepeat<ACT>::repeatPeriod;

protected:
	/// <summary> when hold down the delay before autorepeat changes to faster speed </summary> 
	unsigned long repeatDelayAcc;
	/// <summary> the period for faster autorepeating </summary> 
	unsigned long repeatPeriodAcc;

public:
	/// <summary> PushButton2SpeedAutoRepeat constructor </summary>
	/// <param name="pbPin"> The pin the button is connected to </param>
	/// <param name="KeyPressCallBackFunction"> The function to be called on each keypress </param>
	/// <param name="RepeatDelay"> The delay in milliseconds before autorepeat begins </param>
	/// <param name="AutoRepeatingPeriod"> The period in milliseconds at which a new keypress will be automatically produced </param>
	/// <param name="RepeatAccelerateDelay"> The delay before autorepeat starts to accelerate </param>
	/// <param name="RepeatPeriodAcc"> The period in milliseconds for autorepeat at faster speed </param>
	/// <param name="DebounceDelay"> The delay in milliseconds for debouncing </param>
	PushButton2SpeedAutoRepeat(byte pbPin, void(*KeyPressCallBackFunction)() = NULL,
		unsigned long RepeatDelay = 500, unsigned long AutoRepeatingPeriod = 200,
		unsigned long RepeatAccelerateDelay = 2000, unsigned long RepeatPeriodAcc = 50, unsigned long DebounceDelay = 50)
		: PushButtonAutoRepeat<ACT>(pbPin, KeyPressCallBackFunction, RepeatDelay, AutoRepeatingPeriod, DebounceDelay),
		repeatDelayAcc(RepeatAccelerateDelay), repeatPeriodAcc(RepeatPeriodAcc) { }

	/// <summary> To be called repeatidly in a loop (services autorepeating calls) </summary>
	/// <remark> The callback function has to be registered first
	/// keeps track on the button isPressed nad calls the callback function 
	/// one call after the debounce time has passed and than if the button is still beeing held down 
	/// for more than repeatDelay ms, autorepeat starts calling the function every repeatPeriod ms 
	/// after repeatDelayAcc milliseconds starts to autorepeat at faster speed </remark> 
	void handle()
	{
		unsigned long bpDur = heldDown();
		if (!getSinglePress() && bpDur>getDebounceDelay())
		{
			keyPressCallback();
			setSinglePress(true);
			lastChangeTime = millis();
		}
		if (bpDur > repeatDelay)
		{
			unsigned long now = millis();
			if (bpDur > repeatDelayAcc)
			{
				if (now - lastChangeTime > repeatPeriodAcc)
				{
					keyPressCallback();
					lastChangeTime = now;
				}
			}
			else
			{
				if (now - lastChangeTime > repeatPeriod)
				{
					keyPressCallback();
					lastChangeTime = now;
				}
			}
		}
		if (bpDur == 0)
			setSinglePress(false);
	}
};

/// <summary>
/// PushButtonAutoAcceleratedRepeat Template Class 
/// </summary>
/// <remarks>
/// Implements a push button functionality with debouncing and autorepeat feature with acceleration
/// repetes faster as the button is beeing held longer
/// ACT determines active low = false (default) 
/// - push button that connects to GND when pressed 
/// or active high push button
/// adds autorepeat features and callback function to be called automaticaly when the button is pressedDebounced
/// keeps track of the time the button has been held pressed
/// </remarks>
// adds acceleration (faster repet aret) the button is beeinh held longer
template < bool ACT = false >
class PushButtonAutoAcceleratedRepeat : public PushButtonAutoRepeat<ACT>
{
	using PushButtonAutoRepeat<ACT>::lastChangeTime;
	using PushButtonAutoRepeat<ACT>::repeatDelay;
	using PushButtonAutoRepeat<ACT>::repeatPeriod;

protected:
	/// <summary> when held down the delay before autorepeat starts to accelerate </summary> 
	unsigned long repeatDelayAcc; 
	/// <summary> the acceleration factor </summary> 
	unsigned long repeatAcc; 
	/// <summary> minimum period below which it will not accelerate </summary> 
	unsigned long repeatMinPeriod; 
	/// <summary> the current period for autorepeat </summary> 
	unsigned long currentRepeatPeriod; 

public:
	/// <summary> PushButtonAutoAcceleratedRepeat constructor </summary>
	/// <param name="pbPin"> The pin the button is connected to </param>
	/// <param name="KeyPressCallBackFunction"> The function to be called on each keypress </param>
	/// <param name="RepeatDelay"> The delay in milliseconds before autorepeat begins </param>
	/// <param name="AutoRepeatingPeriod"> The period in milliseconds at which a new keypress will be automatically produced </param>
	/// <param name="RepeatDelayAcc"> The delay before autorepeat starts to accelerate </param>
	/// <param name="RepeatAcc"> The the acceleration in milliseconds the RepeatDelay will be shortened on each update </param>
	/// <param name="repeatMinPeriod"> The period in milliseconds below which it will not accelerate </param>
	/// <param name="DebounceDelay"> The delay in milliseconds for debouncing </param>
	PushButtonAutoAcceleratedRepeat(byte pbPin, void(*KeyPressCallBackFunction)() = NULL,
		unsigned long RepeatDelay = 500, unsigned long AutoRepeatingPeriod = 200, 
		unsigned long RepeatDelayAcc = 2000, unsigned long RepeatAcc = 10, 
		unsigned long repeatMinPeriod = 20, unsigned long DebounceDelay = 50)
		: PushButtonAutoRepeat<ACT>(pbPin, KeyPressCallBackFunction, RepeatDelay,
			AutoRepeatingPeriod, DebounceDelay), repeatDelayAcc(RepeatDelayAcc), 
			repeatAcc(RepeatAcc), repeatMinPeriod(repeatMinPeriod) { }

	/// <summary> To be called repeatidly in a loop (services autorepeating calls) </summary>
	/// <remark> The callback function has to be registered first
	/// keeps track on the button isPressed nad calls the callback function 
	/// one call after the debounce time has passed and than if the button is still beeing held down 
	/// for more than repeatDelay ms, autorepeat starts calling the function every repeatPeriod ms 
	/// after configured milliseconds starts to accelerate autorepeat at given factor </remark> 
	void handle()
	{
		unsigned long bpDur = heldDown(); 
		if (!getSinglePress() && bpDur > getDebounceDelay())
		{
			keyPressCallback();
			setSinglePress(true);
			lastChangeTime = millis();
			currentRepeatPeriod = repeatPeriod;
		}
		if (bpDur > repeatDelay)
		{
			unsigned long now = millis();
			if (now - lastChangeTime > currentRepeatPeriod + repeatMinPeriod)
			{
				if (currentRepeatPeriod>= repeatAcc)
					currentRepeatPeriod-= repeatAcc;
				keyPressCallback();
				lastChangeTime = now;
			}
		}
		if (bpDur == 0)
			setSinglePress(false);
	}
};

#endif


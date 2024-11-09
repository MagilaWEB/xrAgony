#pragma once
struct RenderDeviceStatictics;
struct IRenderDevice
{
	// Device frame increment.
	virtual void incrementFrame() = 0;

	// Get the current frame of the device
	virtual size_t getFrame() const = 0;

	// Get the global device time integer in milliseconds.
	virtual size_t TimeGlobal_ms() const = 0;

	// Get the time of the previous device frame as an integer in milliseconds.
	virtual size_t TimeDelta_ms() const = 0;

	// Get the global device time floating point in seconds.
	virtual float TimeGlobal_sec() const = 0;

	// Get the time of the previous device frame as floating point in seconds.
	virtual float TimeDelta_sec() const = 0;
	virtual size_t TimeContinual() const = 0;

	// Set the time speed factor.
	virtual void time_factor(float time_factor) = 0;

	// Get the set time rate coefficient.
	virtual float time_factor() const = 0;

	// Get the device time as an integer in milliseconds to the point where this method is called.
	virtual size_t TimerAsync_ms() const = 0;

	// Get the device time as floating pont in seconds to the point where this method is called.
	virtual float TimerAsync_sec() const = 0;

	// Get the Renderdevice statistics object.
	virtual const RenderDeviceStatictics& GetStats() const = 0;

	virtual bool isGameProcess() const = 0;
	virtual CTimer_paused* GetTimerGlobal() = 0;
};
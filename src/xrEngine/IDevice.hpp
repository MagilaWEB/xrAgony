#pragma once
struct RenderDeviceStatictics;
class CRenderDevice;

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

	// Setting a pause or removal pause in the gameplay.
	virtual void Pause(bool bOn, bool bTimer, bool bSound, bool reason) = 0;

	// Get the pause status in the gameplay.
	virtual bool Paused() const = 0;

	// At the time of the call in game time or not.
	virtual bool IsGameProcess() const = 0;

	// An object of global time.
	virtual CTimer_paused* GetTimerGlobal() = 0;

	// Operator* cast from IRenderdevice to CRenderdevice.
	virtual CRenderDevice* operator*() = 0;

	// Function cast from IRenderdevice to CRenderdevice.
	virtual CRenderDevice* cast() = 0;
};
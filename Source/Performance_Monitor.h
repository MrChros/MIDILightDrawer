#pragma once

using namespace System::Collections::Generic;

namespace MIDILightDrawer
{

	// Method timing statistics
	public ref class MethodStats
	{
	public:
		property System::String^ MethodName;
		property System::Int64 TotalTime;      // in microseconds
		property int CallCount;
		property System::Int64 MinTime;        // in microseconds
		property System::Int64 MaxTime;        // in microseconds

		// Constructor
		MethodStats();

		// Method to add a new measurement
		void AddMeasurement(System::Int64 duration);
	};

	// Timer class for scoped measurements
	public ref class ScopedTimer
	{
	private:
		System::String^ m_methodName;
		Dictionary<System::String^, MethodStats^>^ m_stats;
		System::Int64 m_startTicks;
		System::Int64 m_frequency;

	public:
		// Constructor starts the timer
		ScopedTimer(System::String^ methodNameFull, Dictionary<System::String^, MethodStats^>^ stats);

		// Destructor stops the timer and records the measurement
		~ScopedTimer();

		// Finalizer
		!ScopedTimer();
	};

	// Main performance monitor class
	public ref class PerformanceMonitor
	{
	private:
		Dictionary<System::String^, MethodStats^>^ m_stats;

	public:
		// Constructor
		PerformanceMonitor();

		// Reset all measurements
		void Reset();

		// Get all stats sorted by total time
		array<MethodStats^>^ GetSortedStats();

		// Generate a formatted report
		System::String^ GetReport();

		// Create a new timer for a method
		ScopedTimer^ CreateTimer(System::String^ methodName);

		// Compare method for sorting
		static int CompareStats(MethodStats^ a, MethodStats^ b)
		{
			return b->TotalTime.CompareTo(a->TotalTime);
		}

		// Internal access to stats dictionary for the timer
		property Dictionary<System::String^, MethodStats^>^ Stats {
			Dictionary<System::String^, MethodStats^>^ get() { return m_stats; }
		}
	};

} // namespace MIDILightDrawer

// Macro for easy method timing
#define TIME_METHOD(monitor)	MIDILightDrawer::ScopedTimer^ __timer = monitor->CreateTimer(gcnew System::String(__FUNCTION__))
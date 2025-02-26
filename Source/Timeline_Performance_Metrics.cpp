#include "Timeline_Performance_Metrics.h"

namespace MIDILightDrawer
{
	PerformanceMetrics::PerformanceMetrics()
	{
		_FrameTimer = gcnew Stopwatch();
		_FrameTimings = gcnew Queue<double>();

		Reset();
	}

	void PerformanceMetrics::StartFrame()
	{
		_FrameTimer->Restart();
	}

	void PerformanceMetrics::EndFrame()
	{
		_FrameTimer->Stop();
		_LastFrameTime = _FrameTimer->Elapsed.TotalMilliseconds;

		_FrameTimings->Enqueue(_LastFrameTime);
		if (_FrameTimings->Count > MAX_FRAME_SAMPLES) {
			_FrameTimings->Dequeue();
		}
	}

	double PerformanceMetrics::GetFPS()
	{
		if (_LastFrameTime <= 0.0) {
			return 0.0;
		}

		return 1000.0 / _LastFrameTime;
	}

	double PerformanceMetrics::GetAverageFrameTime()
	{
		if (_FrameTimings->Count == 0) {
			return 0.0;
		}

		double total = 0.0;
		for each (double timing in _FrameTimings) {
			total += timing;
		}

		return total / _FrameTimings->Count;
	}

	double PerformanceMetrics::GetAverageFPS()
	{
		double avgFrameTime = GetAverageFrameTime();
		if (avgFrameTime <= 0.0)
			return 0.0;

		return 1000.0 / avgFrameTime;
	}

	void PerformanceMetrics::Reset()
	{
		_FrameTimings->Clear();
		_LastFrameTime = 0.0;
	}

	System::String^ PerformanceMetrics::GetReport()
	{
		return System::String::Format("FPS: {0:F1} (Avg: {1:F1}) - Frame Time: {2:F2}ms", GetFPS(), GetAverageFPS(), GetAverageFrameTime());
	}
}
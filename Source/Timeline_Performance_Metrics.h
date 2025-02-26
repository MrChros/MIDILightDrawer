#pragma once

using namespace System::Diagnostics;
using namespace System::ComponentModel;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	ref class PerformanceMetrics
	{
	private:
		static const int MAX_FRAME_SAMPLES = 60;

		Stopwatch^		_FrameTimer;
		System::Collections::Generic::Queue<double>^	_FrameTimings;
		double			_LastFrameTime;

	public:
		PerformanceMetrics();

		void StartFrame();
		void EndFrame();
		double GetFPS();
		double GetAverageFrameTime();
		double GetAverageFPS();
		void Reset();
		System::String^ GetReport();

		property double LastFrameTime {
			double get() { return _LastFrameTime; }
		}
	};
}
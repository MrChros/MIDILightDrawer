#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	ref class PerformanceMetrics
	{
	public:
		// Frame timing
		System::Diagnostics::Stopwatch^ frameTimer;
		System::Collections::Generic::Queue<double>^ frameTimings;
		static const int MAX_FRAME_SAMPLES = 60;

		// Draw counts
		int measuresDrawn;
		int tracksDrawn;
		int notesDrawn;
		int barsDrawn;

		// Resource usage
		int gdiObjectsCreated;
		int gdiObjectsReused;


	public:
		PerformanceMetrics();

		void StartFrame();
		void EndFrame();
		double GetAverageFrameTime();
		void Reset();
		String^ GetReport();
	};
}
#include "Timeline_Performance_Metrics.h"

namespace MIDILightDrawer
{
	PerformanceMetrics::PerformanceMetrics()
	{
		frameTimer = gcnew System::Diagnostics::Stopwatch();
		frameTimings = gcnew System::Collections::Generic::Queue<double>();
		Reset();
	}

	void PerformanceMetrics::StartFrame()
	{
		frameTimer->Restart();
	}

	void PerformanceMetrics::EndFrame()
	{
		frameTimer->Stop();
		frameTimings->Enqueue(frameTimer->Elapsed.TotalMilliseconds);
		if (frameTimings->Count > MAX_FRAME_SAMPLES) {
			frameTimings->Dequeue();
		}
	}

	double PerformanceMetrics::GetAverageFrameTime()
	{
		if (frameTimings->Count == 0) return 0.0;

		double total = 0.0;
		for each (double timing in frameTimings) {
			total += timing;
		}
		return total / frameTimings->Count;
	}

	void PerformanceMetrics::Reset()
	{
		frameTimings->Clear();
		measuresDrawn		= 0;
		tracksDrawn			= 0;
		notesDrawn			= 0;
		barsDrawn			= 0;
		gdiObjectsCreated	= 0;
		gdiObjectsReused	= 0;
	}

	System::String^ PerformanceMetrics::GetReport()
	{
		System::Text::StringBuilder^ report = gcnew System::Text::StringBuilder();
		report->AppendLine("Performance Report:");
		report->AppendFormat("Average Frame Time: {0:F2}ms\n", GetAverageFrameTime());
		report->AppendFormat("Zoom Level: {0:F2}\n", zoomLevel);
		// report->AppendFormat("Elements Drawn: Measures={0}, Tracks={1}, Notes={2}, Bars={3}\n", measuresDrawn, tracksDrawn, notesDrawn, barsDrawn);
		// report->AppendFormat("GDI+ Objects: Created={0}, Reused={1}\n", gdiObjectsCreated, gdiObjectsReused);
		return report->ToString();
	}
}
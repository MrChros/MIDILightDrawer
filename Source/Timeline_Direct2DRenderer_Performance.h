#pragma once

#include "Timeline_Direct2DRenderer.h"
#include "Performance_Monitor.h"

namespace MIDILightDrawer
{
	public ref class Timeline_Direct2DRenderer_Performance : public Timeline_Direct2DRenderer
	{
	public:
		Timeline_Direct2DRenderer_Performance(List<Track^>^ tracks, List<Measure^>^ measures, double zoomlevel, System::Drawing::Point^ scrollposition);

		// Override key methods to add timing
		virtual bool Initialize(System::Windows::Forms::Control^ control, LoadingStatusCallback^ loadingCallback) override;
		virtual bool BeginDraw() override;
		virtual bool EndDraw() override;
		virtual bool DrawWidgetBackground() override;
		virtual bool DrawTrackBackground() override;
		virtual bool DrawMeasureNumbers() override;
		virtual bool DrawTrackContent(Track^ hoverTrack) override;
		virtual bool DrawToolPreview() override;
		virtual bool DrawTrackHeaders() override;
		virtual bool DrawTrackDividers(Track^ hoverTrack) override;
		virtual bool DrawGridLines(float totalHeight) override;

		// Additional performance monitoring methods
		System::String^ GetPerformanceReport();
		void ResetPerformanceMonitor();

	protected:
		!Timeline_Direct2DRenderer_Performance();
		~Timeline_Direct2DRenderer_Performance();

	private:
		PerformanceMonitor^ m_perfMonitor;
	};

}
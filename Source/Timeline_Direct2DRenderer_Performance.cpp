#include "Timeline_Direct2DRenderer_Performance.h"

namespace MIDILightDrawer {

	Timeline_Direct2DRenderer_Performance::Timeline_Direct2DRenderer_Performance(
		List<Track^>^ tracks,
		List<Measure^>^ measures,
		double zoomlevel,
		System::Drawing::Point^ scrollposition)
		: Timeline_Direct2DRenderer(tracks, measures, zoomlevel, scrollposition)
	{
		m_perfMonitor = gcnew PerformanceMonitor();
	}

	Timeline_Direct2DRenderer_Performance::~Timeline_Direct2DRenderer_Performance()
	{
		this->!Timeline_Direct2DRenderer_Performance();
	}

	Timeline_Direct2DRenderer_Performance::!Timeline_Direct2DRenderer_Performance()
	{
		if (m_perfMonitor != nullptr)
		{
			m_perfMonitor = nullptr;
		}
	}

	bool Timeline_Direct2DRenderer_Performance::Initialize(System::Windows::Forms::Control^ control)
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::Initialize(control);
	}

	bool Timeline_Direct2DRenderer_Performance::BeginDraw()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::BeginDraw();
	}

	bool Timeline_Direct2DRenderer_Performance::EndDraw()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::EndDraw();
	}

	bool Timeline_Direct2DRenderer_Performance::DrawWidgetBackground()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawWidgetBackground();
	}

	bool Timeline_Direct2DRenderer_Performance::DrawTrackBackground()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawTrackBackground();
	}

	bool Timeline_Direct2DRenderer_Performance::DrawMeasureNumbers()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawMeasureNumbers();
	}

	bool Timeline_Direct2DRenderer_Performance::DrawTrackContent(Track^ hoverTrack)
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawTrackContent(hoverTrack);
	}

	bool Timeline_Direct2DRenderer_Performance::DrawToolPreview()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawToolPreview();
	}

	bool Timeline_Direct2DRenderer_Performance::DrawTrackHeaders()
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawTrackHeaders();
	}

	bool Timeline_Direct2DRenderer_Performance::DrawTrackDividers(Track^ hoverTrack)
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawTrackDividers(hoverTrack);
	}

	bool Timeline_Direct2DRenderer_Performance::DrawGridLines(float totalHeight)
	{
		TIME_METHOD(m_perfMonitor);
		return Timeline_Direct2DRenderer::DrawGridLines(totalHeight);
	}

	System::String^ Timeline_Direct2DRenderer_Performance::GetPerformanceReport()
	{
		return m_perfMonitor->GetReport();
	}

	void Timeline_Direct2DRenderer_Performance::ResetPerformanceMonitor()
	{
		m_perfMonitor->Reset();
	}

}
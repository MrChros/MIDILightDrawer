#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	ref class TimelineResourceManager {
	private:
		// Pen pools
		static const int POOL_SIZE = 32;
		array<Pen^>^ gridPenPool;
		array<Pen^>^ stringPenPool;
		array<Pen^>^ durationPenPool;
		int currentGridPen;
		int currentStringPen;
		int currentDurationPen;

		// Brush pools
		array<SolidBrush^>^ brushPool;
		int currentBrush;

		// Cached calculations
		Dictionary<int, int>^ tickToPixelCache;
		Dictionary<int, int>^ pixelToTickCache;
		static const int CACHE_SIZE = 1024;

		// Visible range tracking
		Rectangle lastVisibleRange;
		double lastZoomLevel;

		// Performance counters
		int penPoolHits;
		int brushPoolHits;
		int cacheHits;

	public:
		TimelineResourceManager();


		Pen^		GetGridPen(Color color, float width);
		Pen^		GetStringPen(Color color, float width, array<float>^ dashPattern);
		Pen^		GetDurationPen(Color color, float width, array<float>^ dashPattern);
		SolidBrush^ GetBrush(Color color);

		int			CacheTickToPixel(int tick, double zoomLevel, int baseConversion);
		bool		NeedsRedraw(Rectangle visibleRange, double zoomLevel);
		void		GetPerformanceStats(int% penHits, int% brushHits, int% cachingHits);
		void		ResetStats();
		void		Cleanup();
	};
}
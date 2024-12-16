#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward declarations
	ref class Track;
	ref class Measure;
	ref class TrackMeasure;
	ref class Beat;
	ref class Note;
	ref class BarEvent;
	ref class PerformanceMetrics;
	
	// Enhanced visibility tracking with caching
	ref class EnhancedVisibilityTracker {
	private:
		// Viewport state
		Rectangle currentViewport;
		double currentZoom;
		Point^ currentScroll;

	public:
		// Visibility caches
		Dictionary<Track^, Rectangle>^ trackBoundsCache;
		Dictionary<int, Rectangle>^ measureBoundsCache;
		List<Track^>^ visibleTracks;
		List<int>^ visibleMeasures;

		// Note visibility optimization
		Dictionary<Track^, List<Beat^>^>^ visibleBeats;
		Dictionary<Beat^, Rectangle>^ beatBoundsCache;

		// Bar visibility optimization
		Dictionary<Track^, List<BarEvent^>^>^ visibleBars;
		Dictionary<BarEvent^, Rectangle>^ barBoundsCache;

		// Constants
		static const int VISIBILITY_MARGIN = 50; // pixels

	public:
		EnhancedVisibilityTracker();

		bool NeedsUpdate(Rectangle viewport, double zoom, Point^ scroll);
		void Update(Rectangle viewport, double zoom, Point^ scroll, List<Track^>^ tracks, List<Measure^>^ measures, int headerHeight, int trackHeaderWidth, PerformanceMetrics^ metrics);
		void ClearCaches();

		void UpdateTrackVisibility(List<Track^>^ tracks, int headerHeight,PerformanceMetrics^ metrics);
		void UpdateMeasureVisibility(List<Measure^>^ measures, int trackHeaderWidth, PerformanceMetrics^ metrics);
		void UpdateBeatVisibility(List<Track^>^ tracks, List<Measure^>^ measures, PerformanceMetrics^ metrics);
		void UpdateBarVisibility(List<Track^>^ tracks, PerformanceMetrics^ metrics);
		
		bool IsTrackVisible(Track^ track);
		bool IsMeasureVisible(int measureIndex);
		bool IsBeatVisible(Track^ track, Beat^ beat);
		bool IsBarVisible(Track^ track, BarEvent^ bar);

		Rectangle GetTrackBounds(Track^ track);
		Rectangle GetMeasureBounds(int measureIndex);
		Rectangle GetBeatBounds(Beat^ beat);
		Rectangle GetBarBounds(BarEvent^ bar);

	private:
		Rectangle ExpandViewport(Rectangle viewport);
		Rectangle CalculateBeatBounds(Beat^ beat, Track^ track, TrackMeasure^ measure);
		Rectangle CalculateBarBounds(BarEvent^ bar, Track^ track);

		// Helper method to convert ticks to pixels (simplified version)
		static int TicksToPixels(int ticks, double zoom);

		// Helper method to get track height (simplified version)
		static int GetTrackHeight(Track^ track);
	};
}
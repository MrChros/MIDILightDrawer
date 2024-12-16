#include "Timeline_Enhanced_Visibility_Tracker.h"

#include "Widget_Timeline.h"
#include "Timeline_Performance_Metrics.h"
#include "Timeline_Enhanced_Visibility_Tracker.h"

namespace MIDILightDrawer
{
	EnhancedVisibilityTracker::EnhancedVisibilityTracker()
	{
		trackBoundsCache	= gcnew Dictionary<Track^, Rectangle>();
		measureBoundsCache	= gcnew Dictionary<int, Rectangle>();
		visibleTracks		= gcnew List<Track^>();
		visibleMeasures		= gcnew List<int>();
		visibleBeats		= gcnew Dictionary<Track^, List<Beat^>^>();
		beatBoundsCache		= gcnew Dictionary<Beat^, Rectangle>();
		visibleBars			= gcnew Dictionary<Track^, List<BarEvent^>^>();
		barBoundsCache		= gcnew Dictionary<BarEvent^, Rectangle>();

		currentViewport = Rectangle::Empty;
		currentZoom = 0.0;
		currentScroll = gcnew Point(0, 0);
	}

	bool EnhancedVisibilityTracker::NeedsUpdate(Rectangle viewport, double zoom, Point^ scroll)
	{
		return viewport != currentViewport ||
			Math::Abs(zoom - currentZoom) > 0.001 ||
			scroll != currentScroll;
	}

	void EnhancedVisibilityTracker::Update(Rectangle viewport, double zoom, Point^ scroll, List<Track^>^ tracks, List<Measure^>^ measures, int headerHeight, int trackHeaderWidth, PerformanceMetrics^ metrics)
	{
		if (!NeedsUpdate(viewport, zoom, scroll)) {
			return;
		}

		metrics->StartFrame();

		currentViewport = viewport;
		currentZoom = zoom;
		currentScroll = scroll;

		// Clear caches
		ClearCaches();

		// Update track visibility
		UpdateTrackVisibility(tracks, headerHeight, metrics);

		// Update measure visibility
		UpdateMeasureVisibility(measures, trackHeaderWidth, metrics);

		// Update beat and note visibility
		UpdateBeatVisibility(tracks, measures, metrics);

		// Update bar visibility
		UpdateBarVisibility(tracks, metrics);

		metrics->EndFrame();
	}

	void EnhancedVisibilityTracker::ClearCaches()
	{
		trackBoundsCache	->Clear();
		measureBoundsCache	->Clear();
		visibleTracks		->Clear();
		visibleMeasures		->Clear();
		visibleBeats		->Clear();
		beatBoundsCache		->Clear();
		visibleBars			->Clear();
		barBoundsCache		->Clear();
	}

	void EnhancedVisibilityTracker::UpdateTrackVisibility(List<Track^>^ tracks, int headerHeight, PerformanceMetrics^ metrics)
	{
		int y = headerHeight - currentScroll->Y;
		Rectangle expandedViewport = ExpandViewport(currentViewport);

		for each (Track ^ track in tracks) {
			int trackHeight = GetTrackHeight(track);
			Rectangle trackBounds(0, y, currentViewport.Width, trackHeight);

			if (trackBounds.IntersectsWith(expandedViewport)) {
				visibleTracks->Add(track);
				trackBoundsCache[track] = trackBounds;
				metrics->tracksDrawn++;
			}

			y += trackHeight;
		}
	}

	void EnhancedVisibilityTracker::UpdateMeasureVisibility(List<Measure^>^ measures, int trackHeaderWidth, PerformanceMetrics^ metrics)
	{
		int x = trackHeaderWidth - currentScroll->X;
		Rectangle expandedViewport = ExpandViewport(currentViewport);

		for (int i = 0; i < measures->Count; i++) {
			Measure^ measure = measures[i];
			int measureWidth = TicksToPixels(measure->Length, currentZoom);
			Rectangle measureBounds(x, 0, measureWidth, currentViewport.Height);

			if (measureBounds.IntersectsWith(expandedViewport)) {
				visibleMeasures->Add(i);
				measureBoundsCache[i] = measureBounds;
				metrics->measuresDrawn++;
			}

			x += measureWidth;
		}
	}

	void EnhancedVisibilityTracker::UpdateBeatVisibility(List<Track^>^ tracks, List<Measure^>^ measures, PerformanceMetrics^ metrics)
	{
		for each (Track ^ track in visibleTracks) {
			visibleBeats[track] = gcnew List<Beat^>();

			if (track->Measures == nullptr) continue;

			for (int i = 0; i < track->Measures->Count; i++) {
				if (!visibleMeasures->Contains(i)) continue;

				TrackMeasure^ measure = track->Measures[i];
				if (measure == nullptr || measure->Beats == nullptr) continue;

				for each (Beat ^ beat in measure->Beats) {
					Rectangle beatBounds = CalculateBeatBounds(beat, track, measure);
					if (beatBounds.IntersectsWith(currentViewport)) {
						visibleBeats[track]->Add(beat);
						beatBoundsCache[beat] = beatBounds;
						metrics->notesDrawn += beat->Notes->Count;
					}
				}
			}
		}
	}

	void EnhancedVisibilityTracker::UpdateBarVisibility(List<Track^>^ tracks, PerformanceMetrics^ metrics)
	{
		for each (Track ^ track in visibleTracks) {
			visibleBars[track] = gcnew List<BarEvent^>();

			for each (BarEvent ^ bar in track->Events) {
				Rectangle barBounds = CalculateBarBounds(bar, track);
				if (barBounds.IntersectsWith(currentViewport)) {
					visibleBars[track]->Add(bar);
					barBoundsCache[bar] = barBounds;
					metrics->barsDrawn++;
				}
			}
		}
	}

	bool EnhancedVisibilityTracker::IsTrackVisible(Track^ track)
	{
		return visibleTracks->Contains(track);
	}

	bool EnhancedVisibilityTracker::IsMeasureVisible(int measureIndex)
	{
		return visibleMeasures->Contains(measureIndex);
	}

	bool EnhancedVisibilityTracker::IsBeatVisible(Track^ track, Beat^ beat)
	{
		return visibleBeats->ContainsKey(track) &&
			visibleBeats[track]->Contains(beat);
	}

	bool EnhancedVisibilityTracker::IsBarVisible(Track^ track, BarEvent^ bar)
	{
		return visibleBars->ContainsKey(track) &&
			visibleBars[track]->Contains(bar);
	}

	Rectangle EnhancedVisibilityTracker::GetTrackBounds(Track^ track)
	{
		Rectangle bounds;
		if (trackBoundsCache->TryGetValue(track, bounds)) {
			return bounds;
		}
		return Rectangle::Empty;
	}

	Rectangle EnhancedVisibilityTracker::GetMeasureBounds(int measureIndex)
	{
		Rectangle bounds;
		if (measureBoundsCache->TryGetValue(measureIndex, bounds)) {
			return bounds;
		}
		return Rectangle::Empty;
	}

	Rectangle EnhancedVisibilityTracker::GetBeatBounds(Beat^ beat)
	{
		Rectangle bounds;
		if (beatBoundsCache->TryGetValue(beat, bounds)) {
			return bounds;
		}
		return Rectangle::Empty;
	}

	Rectangle EnhancedVisibilityTracker::GetBarBounds(BarEvent^ bar)
	{
		Rectangle bounds;
		if (barBoundsCache->TryGetValue(bar, bounds)) {
			return bounds;
		}
		return Rectangle::Empty;
	}

	Rectangle EnhancedVisibilityTracker::ExpandViewport(Rectangle viewport) {
		return Rectangle(
			viewport.X - VISIBILITY_MARGIN,
			viewport.Y - VISIBILITY_MARGIN,
			viewport.Width + VISIBILITY_MARGIN * 2,
			viewport.Height + VISIBILITY_MARGIN * 2
		);
	}

	Rectangle EnhancedVisibilityTracker::CalculateBeatBounds(Beat^ beat, Track^ track, TrackMeasure^ measure)
	{
		Rectangle trackBounds;
		if (!trackBoundsCache->TryGetValue(track, trackBounds)) {
			return Rectangle::Empty;
		}

		int x = TicksToPixels(beat->StartTick + measure->StartTick, currentZoom) -
			currentScroll->X;
		int width = TicksToPixels(beat->Duration, currentZoom);

		return Rectangle(x, trackBounds.Y, width, trackBounds.Height);
	}

	Rectangle EnhancedVisibilityTracker::CalculateBarBounds(BarEvent^ bar, Track^ track)
	{
		Rectangle trackBounds;
		if (!trackBoundsCache->TryGetValue(track, trackBounds)) {
			return Rectangle::Empty;
		}

		int x = TicksToPixels(bar->StartTick, currentZoom) - currentScroll->X;
		int width = TicksToPixels(bar->Length, currentZoom);

		return Rectangle(x, trackBounds.Y, width, trackBounds.Height);
	}

	// Helper method to convert ticks to pixels (simplified version)
	int EnhancedVisibilityTracker::TicksToPixels(int ticks, double zoom){
		double baseScale = 16.0 / Widget_Timeline::TICKS_PER_QUARTER;
		return (int)Math::Round((double)ticks * baseScale * zoom);
	}

	// Helper method to get track height (simplified version)
	int EnhancedVisibilityTracker::GetTrackHeight(Track^ track)
	{
		return track->ShowTablature ?
			Widget_Timeline::DEFAULT_TRACK_HEIGHT + (int)Widget_Timeline::MIN_TRACK_HEIGHT_WITH_TAB :
			Widget_Timeline::DEFAULT_TRACK_HEIGHT;
	}
}
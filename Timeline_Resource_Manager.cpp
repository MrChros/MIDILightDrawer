#include "Timeline_Resource_Manager.h"

namespace MIDILightDrawer
{
	TimelineResourceManager::TimelineResourceManager()
	{
		// Initialize pools
		gridPenPool = gcnew array<Pen^>(POOL_SIZE);
		stringPenPool = gcnew array<Pen^>(POOL_SIZE);
		durationPenPool = gcnew array<Pen^>(POOL_SIZE);
		brushPool = gcnew array<SolidBrush^>(POOL_SIZE);

		currentGridPen = 0;
		currentStringPen = 0;
		currentDurationPen = 0;
		currentBrush = 0;

		// Initialize caches
		tickToPixelCache = gcnew Dictionary<int, int>(CACHE_SIZE);
		pixelToTickCache = gcnew Dictionary<int, int>(CACHE_SIZE);

		lastVisibleRange = Rectangle::Empty;
		lastZoomLevel = -1.0;

		penPoolHits = 0;
		brushPoolHits = 0;
		cacheHits = 0;
	}

	Pen^ TimelineResourceManager::GetGridPen(Color color, float width)
	{
		int currentIndex = currentGridPen;

		// Create or update pen at current index
		if (gridPenPool[currentIndex] != nullptr) {
			gridPenPool[currentIndex]->Color = color;
			gridPenPool[currentIndex]->Width = width;
			penPoolHits++;
		}
		else {
			gridPenPool[currentIndex] = gcnew Pen(color, width);
		}

		// Move to next index for next request
		currentGridPen = (currentGridPen + 1) % POOL_SIZE;

		// Return pen at current index
		return gridPenPool[currentIndex];
	}

	Pen^ TimelineResourceManager::GetStringPen(Color color, float width, array<float>^ dashPattern)
	{
		int currentIndex = currentStringPen;

		// Create or update pen at current index
		if (stringPenPool[currentIndex] != nullptr) {
			stringPenPool[currentIndex]->Color = color;
			stringPenPool[currentIndex]->Width = width;

			// Handle dash pattern
			if (dashPattern != nullptr && dashPattern->Length > 0) {
				// Validate dash pattern
				bool isValid = true;
				for (int i = 0; i < dashPattern->Length; i++) {
					if (dashPattern[i] <= 0) {
						isValid = false;
						break;
					}
				}

				if (isValid) {
					stringPenPool[currentIndex]->DashPattern = dashPattern;
				}
				else {
					stringPenPool[currentIndex]->DashStyle = Drawing2D::DashStyle::Solid;
				}
			}
			else {
				stringPenPool[currentIndex]->DashStyle = Drawing2D::DashStyle::Solid;
			}

			penPoolHits++;
		}
		else {
			stringPenPool[currentIndex] = gcnew Pen(color, width);
			if (dashPattern != nullptr && dashPattern->Length > 0) {
				stringPenPool[currentIndex]->DashPattern = dashPattern;
			}
		}

		// Move to next index for next request
		currentStringPen = (currentStringPen + 1) % POOL_SIZE;

		// Return pen at current index
		return stringPenPool[currentIndex];
	}

	Pen^ TimelineResourceManager::GetDurationPen(Color color, float width, array<float>^ dashPattern)
	{
		int currentIndex = currentDurationPen;

		if (durationPenPool[currentIndex] != nullptr) {
			durationPenPool[currentIndex]->Color = color;
			durationPenPool[currentIndex]->Width = width;
			if (dashPattern != nullptr) {
				durationPenPool[currentIndex]->DashPattern = dashPattern;
			}
			penPoolHits++;
		}
		else {
			durationPenPool[currentIndex] = gcnew Pen(color, width);
			if (dashPattern != nullptr) {
				durationPenPool[currentIndex]->DashPattern = dashPattern;
			}
		}

		// Move to next index for next request
		currentDurationPen = (currentDurationPen + 1) % POOL_SIZE;

		// Return pen at current index
		return durationPenPool[currentIndex];
	}

	// Get a brush from the pool
	SolidBrush^ TimelineResourceManager::GetBrush(Color color) {
		int currentIndex = currentBrush;

		if (brushPool[currentIndex] != nullptr) {
			brushPool[currentIndex]->Color = color;
			brushPoolHits++;
		}
		else {
			brushPool[currentIndex] = gcnew SolidBrush(color);
		}

		// Move to next index for next request
		currentBrush = (currentBrush + 1) % POOL_SIZE;

		// Return brush at current index
		return brushPool[currentIndex];
	}

	int TimelineResourceManager::CacheTickToPixel(int tick, double zoomLevel, int baseConversion)
	{
		int key = tick * 1000 + (int)(zoomLevel * 1000);  // Combined key
		int result;

		if (tickToPixelCache->TryGetValue(key, result)) {
			cacheHits++;
			return result;
		}

		if (tickToPixelCache->Count >= CACHE_SIZE) {
			tickToPixelCache->Clear();  // Simple cache clearing strategy
		}

		tickToPixelCache[key] = baseConversion;
		return baseConversion;
	}

	bool TimelineResourceManager::NeedsRedraw(Rectangle visibleRange, double zoomLevel)
	{
		if (lastZoomLevel != zoomLevel) {
			lastZoomLevel = zoomLevel;
			lastVisibleRange = visibleRange;
			return true;
		}

		// Check if visible range has changed significantly
		if (Math::Abs(visibleRange.X - lastVisibleRange.X) > 5 ||
			Math::Abs(visibleRange.Y - lastVisibleRange.Y) > 5 ||
			Math::Abs(visibleRange.Width - lastVisibleRange.Width) > 5 ||
			Math::Abs(visibleRange.Height - lastVisibleRange.Height) > 5) {

			lastVisibleRange = visibleRange;
			return true;
		}

		return false;
	}

	void TimelineResourceManager::GetPerformanceStats(int% penHits, int% brushHits, int% cachingHits)
	{
		penHits = penPoolHits;
		brushHits = brushPoolHits;
		cachingHits = cacheHits;
	}

	void TimelineResourceManager::ResetStats()
	{
		penPoolHits		= 0;
		brushPoolHits	= 0;
		cacheHits		= 0;
	}

	void TimelineResourceManager::Cleanup()
	{
		for (int i = 0; i < POOL_SIZE; i++) {
			if (gridPenPool[i] != nullptr) {
				delete gridPenPool[i];
				gridPenPool[i] = nullptr;
			}
			if (stringPenPool[i] != nullptr) {
				delete stringPenPool[i];
				stringPenPool[i] = nullptr;
			}
			if (durationPenPool[i] != nullptr) {
				delete durationPenPool[i];
				durationPenPool[i] = nullptr;
			}
			if (brushPool[i] != nullptr) {
				delete brushPool[i];
				brushPool[i] = nullptr;
			}
		}

		tickToPixelCache->Clear();
		pixelToTickCache->Clear();
	}
}
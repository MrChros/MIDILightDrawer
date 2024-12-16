#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer 
{
	// Forward declarations
	ref class Track;
	ref class BarEvent;
	ref class TimelineTool;
	ref class PointerTool;
	ref class DrawTool;
	ref class SplitTool;
	ref class EraseTool;
	ref class DurationTool;
	ref class ColorTool;
	enum class TimelineToolType;
	ref class TimelineResourceManager;
	ref class PerformanceMetrics;
	ref class EnhancedVisibilityTracker;

	enum class TrackHeightPreset {
		Compact		= 30,	// Minimum height
		Normal		= 40,	// Default height
		Large		= 60,	// Larger view
		Extended	= 80	// Maximum detail
	};

	public ref class Measure {
	public:
		static const int TICKS_PER_QUARTER = 960;

		property int StartTick;
		property int Length;
		property int Numerator;
		property int Denominator;
		property String^ Marker_Text;

		Measure(int startTick, int num, int denom, String^ marker_text) {
			StartTick = startTick;
			Numerator = num;
			Denominator = denom;
			Marker_Text = marker_text;
			// Calculate length based on time signature
			Length = 4 * TICKS_PER_QUARTER * num / denom;
		}

		virtual String^ ToString() override {
			return String::Format("{0}/{1}", Numerator, Denominator);
		}
	};

	public ref class Note {
	public:
		property int String;
		property int Value;		// Fret/pitch
	};

	public ref class Beat {
	public:
		property int StartTick;
		property int Duration;	// In ticks
		property List<Note^>^ Notes;

		Beat() {
			Notes = gcnew List<Note^>();
		}
	};

	public ref class TrackMeasure : public Measure {
	public:
		// Constructor that calls base constructor
		TrackMeasure(int startTick, int num, int denom, String^ marker_text)
			: Measure(startTick, num, denom, marker_text) {
			Beats = gcnew List<Beat^>();
		}

		// Additional constructor that creates from existing measure
		TrackMeasure(Measure^ measure)
			: Measure(measure->StartTick, measure->Numerator, measure->Denominator, measure->Marker_Text) {
			Beats = gcnew List<Beat^>();
		}

		// Beats collection specific to this track's measure
		property List<Beat^>^ Beats;

		// Static comparison method for beats
		static int CompareBeatsByTick(Beat^ a, Beat^ b) {
			return a->StartTick.CompareTo(b->StartTick);
		}

		// Helper methods for beat management
		void AddBeat(Beat^ beat) {
			Beats->Add(beat);
			// Sort beats by start time
			Beats->Sort(gcnew Comparison<Beat^>(&TrackMeasure::CompareBeatsByTick));
		}

		Beat^ GetBeatAtTick(int tick) {
			for each(Beat ^ beat in Beats) {
				if (beat->StartTick <= tick &&
					beat->StartTick + beat->Duration > tick) {
					return beat;
				}
			}
			return nullptr;
		}
	};

	public ref class Widget_Timeline : public UserControl {
	public:
		// Theme colors struct
		value struct ThemeColors {
			Color Background;
			Color HeaderBackground;
			Color Text;
			Color MeasureLine;
			Color BeatLine;
			Color SubdivisionLine;
			Color TempoMarker;
			Color KeySignature;
			Color SelectionHighlight;
			Color TrackBackground;
			Color TrackBorder;
		};

	public:
		Widget_Timeline();
		~Widget_Timeline();

		// Theme property
		property ThemeColors Theme {
			ThemeColors get() { return currentTheme; }
			void set(ThemeColors value) { currentTheme = value; }
		}

		property List<Track^>^ Tracks {
			List<Track^>^ get() { return tracks; }
		}

		property List<Measure^>^ Measures {
			List<Measure^>^ get() { return measures; }
		}

		property int TotalTicks {
			int get() {
				int total = 0;
				for each (Measure ^ m in measures) {
					total += m->Length;
				}
				return total;
			}
		}

		property TimelineToolType CurrentTool {
			TimelineToolType get() { return currentToolType; }
			void set(TimelineToolType value) { SetCurrentTool(value); }
		}

		property Point^ ScrollPosition {
			Point^ get() { return scrollPosition; }
		}

		property Track^ SelectedTrack {
			Track^ get() { return selectedTrack; }
			void set(Track^ value) { selectedTrack = value; }
		}

		property BarEvent^ SelectedBar {
			BarEvent^ get() { return selectedBar; }
			void set(BarEvent^ value) { selectedBar = value; }
		}

		property Rectangle SelectionRectangle {
			Rectangle get();
		}

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;
		void OnMouseDown(MouseEventArgs^ e) override;
		void OnMouseMove(MouseEventArgs^ e) override;
		void OnMouseUp(MouseEventArgs^ e) override;
		void OnMouseWheel(MouseEventArgs^ e) override;
		void OnKeyDown(KeyEventArgs^ e) override;
		void OnKeyUp(KeyEventArgs^ e) override;
		void OnHandleCreated(EventArgs^ e) override;

		virtual bool ProcessDialogKey(Keys keyData) override {
			OnKeyDown(gcnew KeyEventArgs(keyData));
			return true;
		}

		virtual bool IsInputKey(Keys keyData) override {
			return true;  // Tell Windows Forms that all keys are input keys
		}
		

	public:
		// Constants
		static const int TICKS_PER_QUARTER				= 960;
		static const int DEFAULT_TRACK_HEIGHT			= 40;
		static const int HEADER_HEIGHT					= 60;
		static const int MIN_PIXELS_BETWEEN_GRIDLINES	= 20;
		static const int TRACK_HEADER_WIDTH				= 150;
		static const int TRACK_PADDING					= 4;
		static const int MINIMUM_TRACK_HEIGHT			= 30;
		static const int TRACK_RESIZE_HANDLE_HEIGHT		= 5;
		static const double MIN_ZOOM_LEVEL				= 0.1;	// 1/10x zoom
		static const double MAX_ZOOM_LEVEL				= 20.0;	// 20x zoom
		static const float TAB_PADDING					= 4.0f;	// Padding for tablature elements
		static const float MIN_VISIBLE_FONT_SIZE		= 4.0f;	// Minimum readable font size
		static const float FIXED_STRING_SPACING			= 12.0f;
		static const float MIN_TRACK_HEIGHT_WITH_TAB	= TRACK_PADDING * 2 + FIXED_STRING_SPACING * 5;

		// Public methods
		void ZoomIn		();
		void ZoomOut	();
		void SetZoom	(double zoom);
		void ScrollTo	(Point newPosition);
		void AddTrack(String^ name, int octave);
		void SetTrackHeight(Track^ track, int height);
		void AddBarToTrack(Track^ track, int startTick, int length, Color color);
		void AddMeasure(int numerator, int denominator);
		void AddMeasure(int numerator, int denominator, String^ marker_text);
		Beat^ AddBeat(Track^ track, int measureIndex, int startTick, int durationInTicks);
		void AddNote(Beat^ beat, int stringNumber, int value);
		Measure^ GetMeasureAtTick(int tick);
		void Clear();
		void StartBarDrag(BarEvent^ bar, Track^ track, Point startPoint);
		void UpdateBarDrag(Point newPoint);
		void EndBarDrag();
		int  SnapTickToGrid(int tick);
		void ScrollToMeasure(int measureNumber);
		void GetVisibleMeasureRange(int% firstMeasure, int% lastMeasure);
		int  GetMeasureStartTick(int measureNumber);
		int  GetMeasureLength(int measureNumber);
		bool IsMeasureVisible(int measureNumber);
		void SetCurrentTool(TimelineToolType tool);
		void UpdateCursor(System::Windows::Forms::Cursor^ cursor);

		Track^ GetTrackAtPoint(Point p);
		BarEvent^ GetBarAtPoint(Point p);
		int  TicksToPixels(int ticks);
		int  PixelsToTicks(int pixels);
		Rectangle GetTrackBounds(Track^ track);
		Rectangle GetTrackContentBounds(Track^ track);

		PointerTool^	GetPointerTool()	{ return (PointerTool^) (tools[TimelineToolType::Pointer]);		}
		DrawTool^		GetDrawTool()		{ return (DrawTool^)	(tools[TimelineToolType::Draw]);		}
		SplitTool^		GetSplitTool()		{ return (SplitTool^)	(tools[TimelineToolType::Split]);		}
		EraseTool^		GetEraseTool()		{ return (EraseTool^)	(tools[TimelineToolType::Erase]);		}
		DurationTool^	GetDurationTool()	{ return (DurationTool^)(tools[TimelineToolType::Duration]);	}
		ColorTool^		GetColorTool()		{ return (ColorTool^)	(tools[TimelineToolType::Color]);		}

		List<BarEvent^>^ GetSelectedBars();
		bool IsBarSelected(BarEvent^ bar);

		String^ SaveBarEventsToFile(String^ filePath);
		String^ LoadBarEventsFromFile(String^ filePath);

		void LogPerformanceMetrics();

	private:
		static ThemeColors CreateDarkTheme() {
			ThemeColors theme;
			theme.Background			= Color::FromArgb(255, 18, 18, 18);
			theme.HeaderBackground		= Color::FromArgb(255, 25, 25, 25);
			theme.Text					= Color::FromArgb(255, 220, 220, 220);
			theme.MeasureLine			= Color::FromArgb(255, 90, 90, 90);
			theme.BeatLine				= Color::FromArgb(255, 60, 60, 60);
			theme.SubdivisionLine		= Color::FromArgb(255, 40, 40, 40);
			theme.TempoMarker			= Color::FromArgb(255, 255, 165, 0);	// Orange
			theme.KeySignature			= Color::FromArgb(255, 120, 190, 255);	// Light blue
			theme.SelectionHighlight	= Color::FromArgb(255, 65, 105, 225);
			theme.TrackBackground		= Color::FromArgb(255, 30, 30, 30);
			theme.TrackBorder			= Color::FromArgb(255, 45, 45, 45);

			return theme;
		}

		ref struct ViewState {
			int LastVisibleStartTick;
			int LastVisibleEndTick;
			double LastZoomLevel;
			Point^ LastScrollPosition;
			Dictionary<String^, SizeF>^ TextMeasurements;

			ViewState() {
				LastVisibleStartTick = -1;
				LastVisibleEndTick = -1;
				LastZoomLevel = -1;
				LastScrollPosition = gcnew Point(0, 0);
				TextMeasurements = gcnew Dictionary<String^, SizeF>();
			}
		};

		ViewState^					viewState;
		TimelineResourceManager^	resourceManager;
		EnhancedVisibilityTracker^	visibilityTracker;
		PerformanceMetrics^			performanceMetrics;

		// Internal state
		ThemeColors currentTheme;
		double zoomLevel;
		Point^ scrollPosition;
		List<Track^>^ tracks;
		List<Measure^>^ measures;

		// Drawing buffers
		BufferedGraphicsContext^ bufferContext;
		BufferedGraphics^ graphicsBuffer;

		// Musical state
		int currentTimeSignatureNumerator;
		int currentTimeSignatureDenominator;

		// Font for measure numbers
		System::Drawing::Font^ measureFont;
		System::Drawing::Font^ markerFont;

		// State for track management
		Dictionary<Track^, int>^ trackHeights;
		Track^		selectedTrack;
		Track^		hoveredTrack;
		BarEvent^	selectedBar;
		bool		isDraggingTrackDivider;
		int			dragStartY;
		Track^		trackBeingResized;
		Track^		resizeHoverTrack;
		int			resizeStartY;
		int			initialTrackHeight;

		// Tools
		TimelineToolType currentToolType;
		TimelineTool^ currentTool;
		Dictionary<TimelineToolType, TimelineTool^>^ tools;

		// Drawing Support for Tablature
		Drawing::Font^	cachedTabFont;
		Pen^			cachedStringPen;
		Pen^			cachedDurationPen;
		SolidBrush^		cachedTextBrush;

		// Private methods
		void InitializeComponent();
		void InitializeToolSystem();
		void InitializeTablatureResources();
		void UpdateBuffer();
		void DrawTimeline(Graphics^ g);
		void DrawTracks(Graphics^ g);
		
		void UpdateScrollPosition(Point mousePosition);
		void UpdateScrollBounds();
		double GetRelativePositionInMeasure(int tick);
		void SetZoomLevelAtPoint(double newZoom, Point referencePoint);
		void UpdateVisibilityTracker(Graphics^ g);

		// New helper methods for timeline drawing
		void DrawMeasureLines		(Graphics^ g, Rectangle contentRect);
		void DrawBeatLines			(Graphics^ g, Rectangle^ clipRect);
		void DrawSubdivisionLines	(Graphics^ g, Rectangle^ clipRect);
		void DrawMeasureNumbers		(Graphics^ g, Rectangle^ clipRect);
		void ToolPreview			(Graphics^ g);
		void DrawNormalBar			(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawGhostBar			(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawSelectedBar		(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawMoveHandles		(Graphics^ g, Rectangle barBounds);
		int  GetTicksPerMeasure		();
		int  GetTicksPerBeat		();
		float GetSubdivisionLevel	();

		// Track-related methods
		void DrawTrackHeaders	(Graphics^ g, bool drawBorders);
		void DrawTrackNames		(Graphics^ g);
		void DrawTrackContent	(Graphics^ g);
		void DrawTrackBackground(Graphics^ g);
		void DrawTrackDividers	(Graphics^ g);
		void DrawTrackName		(Graphics^ g, Track^ track, Rectangle headerBounds);
		void DrawTrackBorders	(Graphics^ g, Track^ track, Rectangle bounds);
		void DrawGridLines		(Graphics^ g);
		void DrawSelectionAndPreviews(Graphics^ g);
		void DrawTrackEvents	(Graphics^ g, Track^ track, Rectangle bounds);
		void DrawTrackTablature	(Graphics^ g, Track^ track, Rectangle bounds);
		void DrawBeatNotes		(Graphics^ g, Beat^ beat, Rectangle bounds, int beatTick, array<int>^ stringYPositions);
		void DrawBeatDuration	(Graphics^ g, Beat^ beat, Rectangle bounds, array<float>^ stringYPositions);

		int GetTrackTop			(Track^ track);
		int GetTrackHeight		(Track^ track);
		int GetTotalTracksHeight();
		Rectangle GetTrackHeaderBounds(Track^ track);
		SizeF GetCachedTextSize(String^ text, Drawing::Font^ font, Graphics^ g);
		
		void SetTrackHeightPreset(Track^ track, TrackHeightPreset preset);
		void SetAllTracksHeightPreset(TrackHeightPreset preset);
		void BeginTrackResize(Track^ track, int mouseY);
		void UpdateTrackResize(int mouseY);
		void EndTrackResize();
		bool IsOverTrackDivider(Point mousePoint, Track^% track);
		

		// Helper methods for measure management
		void RecalculateMeasurePositions();
		void UpdateDrawingForMeasures();

		// Scroll handling
		System::Windows::Forms::HScrollBar^ hScrollBar;
		int virtualScrollPosition;			// Tracks actual scroll position
		static const int SCROLL_UNIT = 50;	// Pixels per scroll unit

		void UpdateScrollBarRange();
		void OnScroll(Object^ sender, ScrollEventArgs^ e);
		double GetVirtualWidth();
		int GetScrollUnits(double width);

		System::Windows::Forms::VScrollBar^ vScrollBar;
		void UpdateVerticalScrollBarRange();
		void OnVerticalScroll(Object^ sender, ScrollEventArgs^ e);

		// Dragging
		bool isDraggingBar;
		Point^ dragStartPoint;
		Point^ currentMousePoint;
		int dragStartTick;
		BarEvent^ draggedBar;
		Track^ dragSourceTrack;
		Track^ dragTargetTrack;

	};

	// Supporting classes
	public ref class Track {
	public:
		Track(String^ trackName, int octave);

		property String^ Name {
			String^ get() { return name; }
			void set(String^ value) { name = value; }
		}

		property List<BarEvent^>^ Events {
			List<BarEvent^>^ get() { return events; }
		}

		property bool IsSelected {
			bool get() { return isSelected; }
			void set(bool value) { isSelected = value; }
		}
		
		property int Octave {
			int get() { return octave; }
		}

		property List<TrackMeasure^>^ Measures;

		property bool ShowTablature;

		// Add methods for bar management
		void AddBar(int startTick, int length, Color color);
		void RemoveBar(BarEvent ^ bar);

		static int CompareBarEvents(BarEvent^ a, BarEvent^ b);
		static Comparison<BarEvent^>^ barComparer = gcnew Comparison<BarEvent^>(&Track::CompareBarEvents);

	private:
		String^ name;
		int octave;
		List<BarEvent^>^ events;
		bool isSelected;
	};

	public ref class BarEvent {
	public:
		BarEvent(int start, int len, Color c);

		property int StartTick {
			int get() { return startTick; }
			void set(int value) { startTick = value; }
		}

		property int Length {
			int get() { return length; }
			void set(int value) { length = value; }
		}

		property Color Color {
			System::Drawing::Color get() { return color; }
			void set(System::Drawing::Color value) { color = value; }
		}

		property int OriginalStartTick;

	private:
		int startTick;
		int length;
		System::Drawing::Color color;
	};

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
		TimelineResourceManager() {
			// Initialize pools
			gridPenPool = gcnew array<Pen^>(POOL_SIZE);
			stringPenPool = gcnew array<Pen^>(POOL_SIZE);
			durationPenPool = gcnew array<Pen^>(POOL_SIZE);
			brushPool = gcnew array<SolidBrush^>(POOL_SIZE);

			// Pre-create some common resources
			for (int i = 0; i < POOL_SIZE; i++) {
				gridPenPool[i]		= gcnew Pen(Color::Black, 1.0f);
				stringPenPool[i]	= gcnew Pen(Color::Black, 1.0f);
				durationPenPool[i]	= gcnew Pen(Color::Black, 1.0f);
				brushPool[i]		= gcnew SolidBrush(Color::Black);
			}

			currentGridPen = 0;
			currentStringPen = 0;
			currentDurationPen = 0;
			currentBrush = 0;

			// Initialize caches
			tickToPixelCache = gcnew Dictionary<int, int>(CACHE_SIZE);
			pixelToTickCache = gcnew Dictionary<int, int>(CACHE_SIZE);

			lastVisibleRange	= Rectangle::Empty;
			lastZoomLevel		= -1.0;

			penPoolHits		= 0;
			brushPoolHits	= 0;
			cacheHits		= 0;
		}

		// Get a pen from the pool for grid lines
		Pen^ GetGridPen(Color color, float width) {
			if (gridPenPool[currentGridPen] != nullptr) {
				gridPenPool[currentGridPen]->Color = color;
				gridPenPool[currentGridPen]->Width = width;
				penPoolHits++;
			}
			else {
				gridPenPool[currentGridPen] = gcnew Pen(color, width);
			}

			currentGridPen = (currentGridPen + 1) % POOL_SIZE;
			return gridPenPool[currentGridPen];
		}

		// Get a pen from the pool for string lines
		Pen^ GetStringPen(Color color, float width, array<float>^ dashPattern) {
			if (stringPenPool[currentStringPen] != nullptr) {
				stringPenPool[currentStringPen]->Color = color;
				stringPenPool[currentStringPen]->Width = width;
				stringPenPool[currentStringPen]->DashPattern = dashPattern;
				penPoolHits++;
			}
			else {
				stringPenPool[currentStringPen] = gcnew Pen(color, width);
				stringPenPool[currentStringPen]->DashPattern = dashPattern;
			}

			currentStringPen = (currentStringPen + 1) % POOL_SIZE;
			return stringPenPool[currentStringPen];
		}

		// Get a brush from the pool
		SolidBrush^ GetBrush(Color color) {
			if (brushPool[currentBrush] != nullptr) {
				brushPool[currentBrush]->Color = color;
				brushPoolHits++;
			}
			else {
				brushPool[currentBrush] = gcnew SolidBrush(color);
			}

			currentBrush = (currentBrush + 1) % POOL_SIZE;
			return brushPool[currentBrush];
		}

		// Cache tick to pixel conversion
		int CacheTickToPixel(int tick, double zoomLevel, int baseConversion) {
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

		// Check if redraw is needed based on visible range
		bool NeedsRedraw(Rectangle visibleRange, double zoomLevel) {
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

		// Get performance statistics
		void GetPerformanceStats(int% penHits, int% brushHits, int% cachingHits) {
			penHits = penPoolHits;
			brushHits = brushPoolHits;
			cachingHits = cacheHits;
		}

		// Reset performance counters
		void ResetStats() {
			penPoolHits = 0;
			brushPoolHits = 0;
			cacheHits = 0;
		}

		// Cleanup resources
		void Cleanup() {
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
	};

	ref class PerformanceMetrics {
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

		PerformanceMetrics() {
			frameTimer = gcnew System::Diagnostics::Stopwatch();
			frameTimings = gcnew System::Collections::Generic::Queue<double>();
			Reset();
		}

		void StartFrame() {
			frameTimer->Restart();
		}

		void EndFrame() {
			frameTimer->Stop();
			frameTimings->Enqueue(frameTimer->Elapsed.TotalMilliseconds);
			if (frameTimings->Count > MAX_FRAME_SAMPLES) {
				frameTimings->Dequeue();
			}
		}

		double GetAverageFrameTime() {
			if (frameTimings->Count == 0) return 0.0;

			double total = 0.0;
			for each (double timing in frameTimings) {
				total += timing;
			}
			return total / frameTimings->Count;
		}

		void Reset() {
			frameTimings->Clear();
			measuresDrawn = 0;
			tracksDrawn = 0;
			notesDrawn = 0;
			barsDrawn = 0;
			gdiObjectsCreated = 0;
			gdiObjectsReused = 0;
		}

		String^ GetReport() {
			System::Text::StringBuilder^ report = gcnew System::Text::StringBuilder();
			report->AppendLine("Performance Report:");
			report->AppendFormat("Average Frame Time: {0:F2}ms\n", GetAverageFrameTime());
			report->AppendFormat("Elements Drawn: Measures={0}, Tracks={1}, Notes={2}, Bars={3}\n",
				measuresDrawn, tracksDrawn, notesDrawn, barsDrawn);
			report->AppendFormat("GDI+ Objects: Created={0}, Reused={1}\n",
				gdiObjectsCreated, gdiObjectsReused);
			return report->ToString();
		}
	};

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
		EnhancedVisibilityTracker() {
			trackBoundsCache = gcnew Dictionary<Track^, Rectangle>();
			measureBoundsCache = gcnew Dictionary<int, Rectangle>();
			visibleTracks = gcnew List<Track^>();
			visibleMeasures = gcnew List<int>();
			visibleBeats = gcnew Dictionary<Track^, List<Beat^>^>();
			beatBoundsCache = gcnew Dictionary<Beat^, Rectangle>();
			visibleBars = gcnew Dictionary<Track^, List<BarEvent^>^>();
			barBoundsCache = gcnew Dictionary<BarEvent^, Rectangle>();

			currentViewport = Rectangle::Empty;
			currentZoom = 0.0;
			currentScroll = gcnew Point(0, 0);
		}

		bool NeedsUpdate(Rectangle viewport, double zoom, Point^ scroll) {
			return viewport != currentViewport ||
				Math::Abs(zoom - currentZoom) > 0.001 ||
				scroll != currentScroll;
		}

		void Update(Rectangle viewport, double zoom, Point^ scroll,
			List<Track^>^ tracks, List<Measure^>^ measures,
			int headerHeight, int trackHeaderWidth,
			PerformanceMetrics^ metrics) {
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

		void ClearCaches() {
			trackBoundsCache->Clear();
			measureBoundsCache->Clear();
			visibleTracks->Clear();
			visibleMeasures->Clear();
			visibleBeats->Clear();
			beatBoundsCache->Clear();
			visibleBars->Clear();
			barBoundsCache->Clear();
		}

		void UpdateTrackVisibility(List<Track^>^ tracks, int headerHeight,
			PerformanceMetrics^ metrics) {
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

		void UpdateMeasureVisibility(List<Measure^>^ measures, int trackHeaderWidth,
			PerformanceMetrics^ metrics) {
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

		void UpdateBeatVisibility(List<Track^>^ tracks, List<Measure^>^ measures,
			PerformanceMetrics^ metrics) {
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

		void UpdateBarVisibility(List<Track^>^ tracks, PerformanceMetrics^ metrics) {
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

		bool IsTrackVisible(Track^ track) {
			return visibleTracks->Contains(track);
		}

		bool IsMeasureVisible(int measureIndex) {
			return visibleMeasures->Contains(measureIndex);
		}

		bool IsBeatVisible(Track^ track, Beat^ beat) {
			return visibleBeats->ContainsKey(track) &&
				visibleBeats[track]->Contains(beat);
		}

		bool IsBarVisible(Track^ track, BarEvent^ bar) {
			return visibleBars->ContainsKey(track) &&
				visibleBars[track]->Contains(bar);
		}

		Rectangle GetTrackBounds(Track^ track) {
			Rectangle bounds;
			if (trackBoundsCache->TryGetValue(track, bounds)) {
				return bounds;
			}
			return Rectangle::Empty;
		}

		Rectangle GetMeasureBounds(int measureIndex) {
			Rectangle bounds;
			if (measureBoundsCache->TryGetValue(measureIndex, bounds)) {
				return bounds;
			}
			return Rectangle::Empty;
		}

		Rectangle GetBeatBounds(Beat^ beat) {
			Rectangle bounds;
			if (beatBoundsCache->TryGetValue(beat, bounds)) {
				return bounds;
			}
			return Rectangle::Empty;
		}

		Rectangle GetBarBounds(BarEvent^ bar) {
			Rectangle bounds;
			if (barBoundsCache->TryGetValue(bar, bounds)) {
				return bounds;
			}
			return Rectangle::Empty;
		}

	private:
		Rectangle ExpandViewport(Rectangle viewport) {
			return Rectangle(
				viewport.X - VISIBILITY_MARGIN,
				viewport.Y - VISIBILITY_MARGIN,
				viewport.Width + VISIBILITY_MARGIN * 2,
				viewport.Height + VISIBILITY_MARGIN * 2
			);
		}

		Rectangle CalculateBeatBounds(Beat^ beat, Track^ track, TrackMeasure^ measure) {
			Rectangle trackBounds;
			if (!trackBoundsCache->TryGetValue(track, trackBounds)) {
				return Rectangle::Empty;
			}

			int x = TicksToPixels(beat->StartTick + measure->StartTick, currentZoom) -
				currentScroll->X;
			int width = TicksToPixels(beat->Duration, currentZoom);

			return Rectangle(x, trackBounds.Y, width, trackBounds.Height);
		}

		Rectangle CalculateBarBounds(BarEvent^ bar, Track^ track) {
			Rectangle trackBounds;
			if (!trackBoundsCache->TryGetValue(track, trackBounds)) {
				return Rectangle::Empty;
			}

			int x = TicksToPixels(bar->StartTick, currentZoom) - currentScroll->X;
			int width = TicksToPixels(bar->Length, currentZoom);

			return Rectangle(x, trackBounds.Y, width, trackBounds.Height);
		}

		// Helper method to convert ticks to pixels (simplified version)
		static int TicksToPixels(int ticks, double zoom) {
			double baseScale = 16.0 / Widget_Timeline::TICKS_PER_QUARTER;
			return (int)Math::Round((double)ticks * baseScale * zoom);
		}

		// Helper method to get track height (simplified version)
		static int GetTrackHeight(Track^ track) {
			return track->ShowTablature ?
				Widget_Timeline::DEFAULT_TRACK_HEIGHT + (int)Widget_Timeline::MIN_TRACK_HEIGHT_WITH_TAB :
				Widget_Timeline::DEFAULT_TRACK_HEIGHT;
		}
	};

}
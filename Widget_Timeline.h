#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

#include "Widget_Timeline_Common.h"
#include "Timeline_Resource_Manager.h"
#include "Timeline_Performance_Metrics.h"
#include "Timeline_Enhanced_Visibility_Tracker.h"

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
		static const int DEFAULT_TRACK_HEIGHT			= 100;
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
}
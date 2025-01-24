#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

#include "Theme_Manager.h"
#include "Widget_Timeline_Common.h"
#include "Widget_Timeline_Classes.h"
#include "Timeline_Tool_Interface.h"
#include "Timeline_Resource_Manager.h"
#include "Timeline_Direct2DRenderer.h"
#include "Timeline_Performance_Metrics.h"

#define USE_DIRECT_X	true

namespace MIDILightDrawer 
{
	// Forward Declarations
	ref class TimelineTool;
	ref class PointerTool;
	ref class DrawTool;
	ref class SplitTool;
	ref class EraseTool;
	ref class DurationTool;
	ref class ColorTool;
	ref class FadeTool;
	ref class StrobeTool;
	enum class TimelineToolType;

	public ref class Widget_Timeline : public UserControl, public ITimelineAccess {
	public:
		// Constants
		static const int TICKS_PER_QUARTER				= 960;
		static const int INITIAL_TICK_OFFSET			= TICKS_PER_QUARTER;
		static const int DEFAULT_TRACK_HEIGHT			= 140;	// 100
		static const int HEADER_HEIGHT					= 60;
		//static const int MIN_PIXELS_BETWEEN_GRIDLINES	= 20;
		static const int TRACK_HEADER_WIDTH				= 150;
		static const int TRACK_PADDING					= 4;
		static const int MINIMUM_TRACK_HEIGHT			= 30;
		static const int TRACK_RESIZE_HANDLE_HEIGHT		= 5;
		static const int SCROLL_UNIT					= 50;	// Pixels per scroll unit
		static const int BUTTON_SIZE					= 24;
		static const int BUTTON_MARGIN					= 6;
		static const double MIN_ZOOM_LEVEL				= 0.1;	// 1/10x zoom
		static const double MAX_ZOOM_LEVEL				= 20.0;	// 20x zoom
		static const float TAB_PADDING					= 4.0f;	// Padding for tablature elements
		static const float MIN_VISIBLE_FONT_SIZE		= 4.0f;	// Minimum readable font size
		static const float FIXED_STRING_SPACING			= 12.0f;
		static const float MIN_TRACK_HEIGHT_WITH_TAB	= TRACK_PADDING * 2 + FIXED_STRING_SPACING * 5;

	public:
		Widget_Timeline();
		~Widget_Timeline();

		// Adding methods
		void AddTrack(String^ name, int octave);
		void AddMeasure(int numerator, int denominator, int tempo);
		void AddMeasure(int numerator, int denominator, int tempo, String^ marker_text);
		Beat^ AddBeat(Track^ track, int measureIndex, int startTick, int durationInTicks, bool is_dotted);
		void AddNote(Beat^ beat, int stringNumber, int value, bool is_tied);
		void AddBarToTrack(Track^ track, int startTick, int length, Color color);

		// Clear method
		void Clear();

		// View methods
		void ZoomIn		();
		void ZoomOut	();
		void SetZoom	(double zoom);
		void ScrollTo	(Point newPosition);
		void SetTrackHeight(Track^ track, int height);
		void SetAllTracksHeight(int height);
		void SetToolSnapping(SnappingType type);
		
		// Tools setter/getter
		void SetCurrentTool(TimelineToolType tool);
		PointerTool^	GetPointerTool()	{ return (PointerTool^) (tools[TimelineToolType::Pointer]);		}
		DrawTool^		GetDrawTool()		{ return (DrawTool^)	(tools[TimelineToolType::Draw]);		}
		SplitTool^		GetSplitTool()		{ return (SplitTool^)	(tools[TimelineToolType::Split]);		}
		EraseTool^		GetEraseTool()		{ return (EraseTool^)	(tools[TimelineToolType::Erase]);		}
		DurationTool^	GetDurationTool()	{ return (DurationTool^)(tools[TimelineToolType::Duration]);	}
		ColorTool^		GetColorTool()		{ return (ColorTool^)	(tools[TimelineToolType::Color]);		}
		FadeTool^		GetFadeTool()		{ return (FadeTool^)	(tools[TimelineToolType::Fade]);		}
		StrobeTool^		GetStrobeTool()		{ return (StrobeTool^)	(tools[TimelineToolType::Strobe]);		}

		virtual TimelineToolType GetCurrentToolType() {
			return currentToolType;
		}

		virtual ITimelineToolAccess^ GetToolAccess() {
			return safe_cast<ITimelineToolAccess^>(tools[currentToolType]);
		}

		// Other interface metohds for the tools		
		int  SnapTickToGrid(int tick);
		int  SnapTickBasedOnType(int tick, Point mousePos);
		void ScrollToMeasure(int measureNumber);
		int  GetMeasureStartTick(int measureNumber);
		int  GetMeasureLength(int measureNumber);
		void UpdateCursor(System::Windows::Forms::Cursor^ cursor);

		int  TicksToPixels(int ticks);						// Transferred
		int  PixelsToTicks(int pixels);						// Transferred
		
		Track^		GetTrackAtPoint(Point p);				// Transferred
		Rectangle	GetTrackBounds(Track^ track);			// Transferred
		Rectangle	GetTrackHeaderBounds(Track^ track);		// Transferred
		Rectangle	GetTrackContentBounds(Track^ track);	// Transferred
		Measure^	GetMeasureAtTick(int tick);				// Transferred
		BarEvent^	GetBarAtPoint(Point p);					// Transferred

		String^ SaveBarEventsToFile(String^ filePath);		// No Need
		String^ LoadBarEventsFromFile(String^ filePath);	// No Need

		void LogPerformanceMetrics();						// No Need

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

	private:
		System::Resources::ResourceManager^ _Resources;
		
		Timeline_Direct2DRenderer^	D2DRenderer;
		TimelineResourceManager^	resourceManager;
		PerformanceMetrics^			performanceMetrics;

		ThemeColors		currentTheme;
		TrackButtonId	hoveredButton;
		
		List<Track^>^	tracks;
		List<Measure^>^ measures;

		// Drawing buffers
		BufferedGraphicsContext^	bufferContext;
		BufferedGraphics^			graphicsBuffer;

		// Scroll Bars
		System::Windows::Forms::HScrollBar^ hScrollBar;
		System::Windows::Forms::VScrollBar^ vScrollBar;

		// Font for measure numbers
		System::Drawing::Font^ measureFont;
		System::Drawing::Font^ markerFont;
		System::Drawing::Font^ cachedTabFont;

		// Drawing Support for Tablature
		Pen^		cachedStringPen;
		Pen^		cachedDurationPen;
		SolidBrush^ cachedTextBrush;

		Track^ trackBeingResized;
		Track^ resizeHoverTrack;
		
		Point^ scrollPosition;
		Point^ dragStartPoint;
		Point^ currentMousePoint;

		int	resizeStartY;
		int	initialTrackHeight;

		double zoomLevel;

		// Tools
		TimelineToolType	currentToolType;
		TimelineTool^		currentTool;
		SnappingType		snappingType;
		Dictionary<TimelineToolType, TimelineTool^>^ tools;

		// Private methods
		void InitializeComponent();
		void InitializeToolSystem();
		void InitializeTablatureResources();
		void UpdateBuffer();

		// New helper methods for timeline drawing
		void DrawMeasureLines				(Graphics^ g, Rectangle contentRect);
		void DrawBeatLines					(Graphics^ g, Rectangle^ clipRect);
		void DrawSubdivisionLines			(Graphics^ g, Rectangle^ clipRect);
		void DrawMeasureNumbers				(Graphics^ g);
		void DrawToolPreview				(Graphics^ g);
		void DrawToolPreviewDrawTool		(Graphics^ g);
		void DrawToolPreviewDrawToolDraw	(Graphics^ g, DrawTool^ drawTool);
		void DrawToolPreviewDrawToolErase	(Graphics^ g, DrawTool^ drawTool);
		void DrawToolPreviewDrawToolMove	(Graphics^ g, DrawTool^ drawTool);
		void DrawToolPreviewDrawToolResize	(Graphics^ g, DrawTool^ drawTool);
		void DrawToolPreviewEraseTool		(Graphics^ g);
		void DrawToolPreviewDurationTool	(Graphics^ g);
		void DrawToolPreviewColorTool		(Graphics^ g);
		void DrawToolPreviewFadeTool		(Graphics^ g);
		void DrawToolPreviewStrobeTool		(Graphics^ g);
		void DrawPreviewBar					(Graphics^ g, BarEvent^ bar, Track^ track, Point mousePos, BarPreviewType previewType);
		void DrawPreviewBarList				(Graphics^ g, List<BarEvent^>^ bar_list, Track^ track);
		void DrawCreationMovementPreview	(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawDurationPreview			(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawNormalBar					(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawGhostBar					(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawSelectedBar				(Graphics^ g, BarEvent^ bar, Rectangle bounds);
		void DrawMoveHandles				(Graphics^ g, Rectangle barBounds);
		void DrawBarGlowEffect				(Graphics^ g, Rectangle barBounds, Color glowColor, int glowLevels);
		void DrawResizeHandle				(Graphics^ g, Rectangle barBounds, Color handleColor, bool isTargeted);

		// Track-related methods
		void DrawTrackHeaders			(Graphics^ g);
		void DrawTrackContent			(Graphics^ g);
		void DrawTrackBackground		(Graphics^ g);
		void DrawTrackDividers			(Graphics^ g);
		void DrawTrackName				(Graphics^ g, Track^ track, Rectangle headerBounds);
		void DrawTrackButtons			(Graphics^ g, Track^ track, Rectangle headerBounds);
		void DrawTrackButtonText		(Graphics^ g, Rectangle headerBounds, int buttonIndex, String^ text, bool isPressed, bool isHovered, Color baseColor, Color textColor);
		void DrawTrackButtonIcon		(Graphics^ g, Rectangle headerBounds, int buttonIndex, Image^ icon , bool isPressed, bool isHovered, Color baseColor, Color textColor);
		void DrawTrackBorders			(Graphics^ g, Track^ track, Rectangle bounds);
		void DrawGridLines				(Graphics^ g);
		void DrawSelectionAndPreviews	(Graphics^ g);
		void DrawTrackEvents			(Graphics^ g, Track^ track, Rectangle bounds);
		void DrawTrackTablature			(Graphics^ g, Track^ track, Rectangle bounds);
		void DrawTrackTablatureDrum		(Graphics^ g, Track^ track, Rectangle bounds, float logScale);
		void DrawTrackTablatureRegular	(Graphics^ g, Track^ track, Rectangle bounds, float logScale);
		void DrawBeatDuration			(Graphics^ g, Beat^ beat  , Rectangle bounds, array<float>^ stringYPositions);
		void DrawTieLines				(Graphics^ g, Track^ track, Rectangle bounds, array<float>^ stringYPositions, float scaledFontSize);
		void DrawDrumSymbol				(Graphics^ g, DrumNotationType symbolType, float x, float y, float size);
		TabStringInfo DrawTablatureStrings(Graphics^ g, Rectangle bounds, float availableHeight, float logScale, int numStrings);

		float	GetSubdivisionLevel	();
		int		GetTrackTop			(Track^ track);	// Transferred
		int		GetTotalTracksHeight();				// Transferred

		void	BeginTrackResize(Track^ track, int mouseY);
		void	UpdateTrackResize(int mouseY);
		void	EndTrackResize();
		bool	IsOverTrackDivider(Point mousePoint, Track^% track);
		bool	IsOverTrackButton(Track^ track, int buttonIndex, Point mousePoint);
		Rectangle GetTrackButtonBounds(Rectangle headerBounds, int buttonIndex);
		Rectangle CalculateBarBounds(BarEvent^ bar, Rectangle bounds);				// Transferred

		// Helper methods for measure management
		void	RecalculateMeasurePositions();
		void	UpdateDrawingForMeasures();

		double	GetRelativePositionInMeasure(int tick);
		double	GetVirtualWidth();
		void	SetZoomLevelAtPoint(double newZoom, Point referencePoint);
		void	UpdateScrollBarRange();
		int		GetScrollUnits(double width);
						
		void	UpdateScrollBounds();
		void	UpdateVerticalScrollBarRange();
		void	OnScroll(Object^ sender, ScrollEventArgs^ e);
		void	OnVerticalScroll(Object^ sender, ScrollEventArgs^ e);

	public:
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
	};
}
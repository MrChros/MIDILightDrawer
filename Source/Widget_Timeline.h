#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

#include "Settings.h"
#include "Form_Loading.h"
#include "Theme_Manager.h"
#include "Widget_Timeline_Common.h"
#include "Widget_Timeline_Classes.h"
#include "Timeline_Tool_Interface.h"
#include "Timeline_Command_Manager.h"
#include "Timeline_Direct2DRenderer.h"
#include "Timeline_Performance_Metrics.h"


#ifdef _DEBUG
	#include "Timeline_Direct2DRenderer_Performance.h"
	typedef MIDILightDrawer::Timeline_Direct2DRenderer_Performance D2D_Renderer_t;
#else
	typedef MIDILightDrawer::Timeline_Direct2DRenderer D2D_Renderer_t;
#endif


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
		event EventHandler<TimelineToolType>^ ToolChanged;

		// Constants
		static const int INITIAL_TICK_OFFSET			= Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		static const int DEFAULT_TRACK_HEIGHT			= 140;
		static const int MINIMUM_TRACK_HEIGHT			= 30;
		static const int TRACK_RESIZE_HANDLE_HEIGHT		= 5;
		static const int SCROLL_UNIT					= 50;	// Pixels per scroll unit
		static const int DEFAULT_FADE_TICK_QUANTIZATION = Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 4; // 16th Note
		
		static const float TAB_PADDING					= 4.0f;	// Padding for tablature elements
		static const float MIN_VISIBLE_FONT_SIZE		= 4.0f;	// Minimum readable font size
		static const float MIN_TRACK_HEIGHT_WITH_TAB	= Timeline_Direct2DRenderer::TRACK_PADDING * 2 + Timeline_Direct2DRenderer::FIXED_STRING_SPACING * 5;

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
		void Undo();
		void Redo();

		// View methods
		void ZoomIn();
		void ZoomOut();
		void SetZoom(double zoom);
		void ScrollTo(Point newPosition);
		void SetTrackHeight(Track^ track, int height);
		void SetAllTracksHeight(int height);
		void SetToolSnapping(SnappingType type);

		void ShowContextMenu(BarEvent^ bar, Point location);

		// Tools Setter & Getter
		void SetCurrentTool(TimelineToolType tool);
		PointerTool^ GetPointerTool();
		DrawTool^ GetDrawTool();
		SplitTool^ GetSplitTool();
		EraseTool^ GetEraseTool();
		DurationTool^ GetDurationTool();
		ColorTool^ GetColorTool();
		FadeTool^ GetFadeTool();
		StrobeTool^ GetStrobeTool();

		// Interface Methods to the Direct 2D Renderer
		virtual TimelineCommandManager^ CommandManager();
		virtual TimelineToolType CurrentToolType();
		virtual ITimelineToolAccess^ ToolAccess();
		virtual TrackButtonId HoverButton();

		// Other interface methods for the tools
		int  SnapTickToGrid(int tick);
		int  SnapTickBasedOnType(int tick, Point mousePos);
		void ScrollToMeasure(int measureNumber);
		int  GetMeasureStartTick(int measureNumber);
		int  GetMeasureLength(int measureNumber);
		void UpdateCursor(System::Windows::Forms::Cursor^ cursor);
		bool DoesBarExist(BarEvent^ bar);

		int  TicksToPixels(int ticks);
		int  PixelsToTicks(int pixels);
		
		Track^		GetTrackAtPoint(Point p);
		Rectangle	GetTrackBounds(Track^ track);
		Rectangle	GetTrackHeaderBounds(Track^ track);
		Rectangle	GetTrackContentBounds(Track^ track);
		Measure^	GetMeasureAtTick(int tick);
		BarEvent^	GetBarAtPoint(Point p);

		String^ SaveBarEventsToFile(String^ filePath);
		String^ LoadBarEventsFromFile(String^ filePath);

		void LogPerformanceMetrics();

#ifdef _DEBUG
		String^ GetRendererPerformanceReport() {
			return _D2DRenderer->GetPerformanceReport();
		}

		void ResetRendererPerformanceMonitor() {
			_D2DRenderer->ResetPerformanceMonitor();
		}
#endif

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;
		void OnMouseClick(MouseEventArgs^ e) override;
		void OnMouseDown(MouseEventArgs^ e) override;
		void OnMouseMove(MouseEventArgs^ e) override;
		void OnMouseUp(MouseEventArgs^ e) override;
		void OnMouseWheel(MouseEventArgs^ e) override;
		void OnKeyDown(KeyEventArgs^ e) override;
		void OnKeyUp(KeyEventArgs^ e) override;
		void OnHandleCreated(EventArgs^ e) override;

		virtual bool ProcessDialogKey(Keys keyData) override;
		virtual bool IsInputKey(Keys keyData) override;


	private:
		TimelineCommandManager^ _CommandManager;
		D2D_Renderer_t^			_D2DRenderer;
		PerformanceMetrics^		_PerformanceMetrics;
		
		ThemeColors		_CurrentTheme;
		TrackButtonId	_HoveredButton;
		
		List<Track^>^	_Tracks;
		List<Measure^>^ _Measures;

		// Scroll Bars
		System::Windows::Forms::HScrollBar^ _HScrollBar;
		System::Windows::Forms::VScrollBar^ _VScrollBar;

		// Context Menu
		System::Resources::ResourceManager^ _Resources;
		System::Windows::Forms::ContextMenuStrip^ _ContextMenu;
		BarEvent^ _ContextMenuBar;

		Track^ _TrackBeingResized;
		Track^ _ResizeHoverTrack;
		
		Point^ _ScrollPosition;

		int	_ResizeStartY;
		int	_InitialTrackHeight;

		double _ZoomLevel;

		// Tools
		TimelineToolType	_CurrentToolType;
		TimelineTool^		_CurrentTool;
		SnappingType		_SnappingType;
		Dictionary<TimelineToolType, TimelineTool^>^ _Tools;

		// Private methods
		void InitializeComponent();
		void InitializeToolSystem();
		void InitializeContextMenu();

		void CreateContextMenuCommon();
		void CreateContextMenuSolid();
		void CreateContextMenuFade();
		void CreateContextMenuStrobe();
		void CreateContextMenuSubChangeColor(String^ menuTitle);
		void CreateContextMenuSubChangeQuantization(int currentQuantization);
		ToolStripMenuItem^ CreateContextMenuItem(String^ text, bool clickable, Image^ image);

		void HandleContextMenuClick(System::Object^ sender, EventArgs^ e);

		float	GetSubdivisionLevel	();
		int		GetTrackTop			(Track^ track);
		int		GetTotalTracksHeight();

		void	BeginTrackResize(Track^ track, int mouseY);
		void	UpdateTrackResize(int mouseY);
		void	EndTrackResize();
		bool	IsOverTrackDivider(Point mousePoint, Track^% track);
		bool	IsOverTrackButton(Track^ track, int buttonIndex, Point mousePoint);
		Rectangle GetTrackButtonBounds(Rectangle headerBounds, int buttonIndex);
		Rectangle CalculateBarBounds(BarEvent^ bar, Rectangle bounds);

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
		property ThemeColors Theme {
			ThemeColors get();
			void set(ThemeColors value);
		}

		property List<Track^>^ Tracks {
			List<Track^> ^ get();
		}

		property List<Measure^>^ Measures {
			List<Measure^> ^ get();
		}

		property int TotalTicks {
			int get();
		}

		property TimelineToolType CurrentTool {
			TimelineToolType get();
			void set(TimelineToolType value);
		}

		property Point^ ScrollPosition {
			Point^ get();
		}
	};
}
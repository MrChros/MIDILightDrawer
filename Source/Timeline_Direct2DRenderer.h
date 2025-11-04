#pragma once

#include <string>
#include "Timeline_Tool_Interface.h"

// Forward declare the native renderer
class Timeline_Direct2DRenderer_Native;

using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
    ref class Track;
    ref class Measure;
    ref class Beat;
    ref class BarEvent;
	ref class Collapsible_Left_Panel;
    
    ref class TimelineTool;
    ref class PointerTool;
    ref class DrawTool;
    ref class ColorTool;
    ref class DurationTool;
    
	enum class BarEventType;
	enum class TimelineToolType;
    enum class DrumNotationType;
    enum class BarPreviewType;
	enum class LoadingStage;
	enum class Easing;

    value struct TabStringInfo;
    value struct TrackButtonId;

	public delegate void LoadingStatusCallback(LoadingStage stage);
    public delegate void LoadingProgressCallback(float progress);

	// Managed wrapper for TimelineD2DRenderer
    public ref class Timeline_Direct2DRenderer
    {
    public:
		// General
        static const int TICKS_PER_QUARTER				= 960;
        static const int HEADER_HEIGHT					= 60;
        static const int MIN_PIXELS_BETWEEN_GRIDLINES   = 20;
		// Tracks
        static const int TRACK_HEADER_WIDTH				= 150;
        static const int TRACK_PADDING					= 4;
		// Track Buttons
        static const int BUTTON_SIZE					= 24;
		static const int BUTTON_MARGIN					= 6;
		// Left Panel
		static const int PANEL_DEFAULT_WIDTH			= 250;
		static const int PANEL_MIN_WIDTH				= 230;
		static const int PANEL_MAX_WIDTH				= 300;
		static const int PANEL_RESIZE_HANDLE_WIDTH		= 5;
		static const int PANEL_BUTTON_SIZE				= 40;
		static const int PANEL_BUTTON_MARGIN			= (HEADER_HEIGHT - PANEL_BUTTON_SIZE) / 2;
		// Rendering Spacings
        static const float FIXED_STRING_SPACING			= 12.0f;
        static const float MIN_MEASURE_NUMBER_SPACING	= 20.0f;
        static const float MIN_MEASURE_BEAT_SPACING		= 20.0f;
        static const float MIN_NOTE_SPACING_PIXELS		= 10.0f;
        // Min Width for rendering Easings
        static const int MIN_WIDTH_FOR_CURVES           = 60;   // Minimum width in pixels to draw curves
        static const float MIN_ZOOM_FOR_CURVES          = 0.8f; // Minimum zoom level to show curves
		// Zoom Level
		static const double MIN_ZOOM_LEVEL				= 0.1;	// 1/10x zoom
		static const double MAX_ZOOM_LEVEL				= 20.0;	// 20x zoom
		// Track overlay colors
		static const unsigned int MUTED_OVERLAY_COLOR	= 0x40FF0000; // Red, 25% alpha
		static const unsigned int SOLOED_OVERLAY_COLOR	= 0x40FFFF00; // Yellow, 25% alpha

        value struct CachedThemeColors {
            System::Drawing::Color Background;
            System::Drawing::Color HeaderBackground;
            System::Drawing::Color Text;
            System::Drawing::Color MeasureLine;
            System::Drawing::Color BeatLine;
            System::Drawing::Color SubdivisionLine;
            System::Drawing::Color SelectionHighlight;
            System::Drawing::Color TrackBackground;
            System::Drawing::Color TrackBorder;
        };

        enum class TablatureResolution {
            FullDetail,      // Show all notes
            BeatLevel,       // Show notes on beat boundaries
            HalfMeasure,     // Show notes at half-measure intervals
            MeasureLevel     // Show only first note of each measure
        };

        value struct LevelOfDetail {
            int MeasureSkipFactor;
            bool ShowBeatLines;
            bool ShowAllNotes;
            bool ShowTieLines;
            TablatureResolution TabResolution;
        };

    public:
        Timeline_Direct2DRenderer(List<Track^>^ tracks, List<Measure^>^ measures, Collapsible_Left_Panel^ leftPanel, double zoomlevel, System::Drawing::Point^ scrollposition);
        virtual ~Timeline_Direct2DRenderer();
		!Timeline_Direct2DRenderer();

        // Initialization
		bool Initialize(System::Windows::Forms::Control^ control, LoadingStatusCallback^ loadingCallback, LoadingProgressCallback^ progressCallback);
        void Resize(int width, int height);
        void SetThemeColors(System::Drawing::Color background, System::Drawing::Color headerbackground, System::Drawing::Color text, System::Drawing::Color measureline, System::Drawing::Color beatline, System::Drawing::Color subdivisionline, System::Drawing::Color selectionhighlight, System::Drawing::Color trackbackground, System::Drawing::Color trackborder);
        void SetZoomLevel(double zoomlevel);
        void SetScrollPositionReference(System::Drawing::Point^ scrollposition);
        void SetTimelineAccess(ITimelineAccess^ access);
        void PreloadImages(LoadingProgressCallback^ progressCallback);
        void PreloadTabTexts(LoadingProgressCallback^ progressCallback);
		void PreloadDrumSymbols(LoadingProgressCallback^ progressCallback);
		void PreloadDurationSymbols(LoadingProgressCallback^ progressCallback);

        // Drawing Methods
		bool BeginDraw();
		bool EndDraw();

        // Widget Timeline Drawing 
        void UpdateLevelOfDetail();
        bool DrawWidgetBackground();
        bool DrawTrackBackground();
        bool DrawMeasureNumbers();
        bool DrawTrackContent(Track^ hoverTrack);
        bool DrawToolPreview();
		void DrawFPSCounter(float x, double fps, double frameTimeMs);
		void DrawLeftPanel(bool beingResized);


        // Sub-Methods for DrawMeasureNumbers
		bool DrawBeatNumbers(Measure^ measure, float x, float measureNumberY, float subdivLevel, int measureNumber, int ticksPerBeat);


        // Sub-Methods for DrawTrackContent
        void DrawGridLines(float totalHeight);
        void DrawSubdivisionLines(float totalHeight, int startTick, int endTick);
        void DrawBeatLines(float totalHeight, int startTick, int endTick);
        void DrawMeasureLines(float totalHeight, int startTick, int endTick);


        // Sub-Methods for DrawTrackContent - DrawTrackEvents and DrawTrackTablature
        void DrawTrackEvents(Track^ track, System::Drawing::Rectangle trackContentBounds, TimelineToolType currentToolType);
        bool DrawTrackTablature(Track^ track, System::Drawing::Rectangle trackContentBounds);
        bool DrawTrackTablatureDrum(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale);
        bool DrawTrackTablatureRegular(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale);
        void DrawBeatDuration(Beat^ beat, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions);
        void DrawTieLines(Track^ track, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions, float scaledFontSize);
        void DrawDrumSymbol(DrumNotationType symbolType, float x, float y, float size);
        TabStringInfo DrawTablatureStrings(System::Drawing::Rectangle bounds, float availableHeight, float logScale, int numStrings);
		float GetTablatureScaledFontSize(float logScale);
		float GetTablatureScaledStringSpacing(float logScale);
        bool ShouldRenderBeat(Beat^ beat, Measure^ measure, TablatureResolution resolution);

        bool DrawTrackHeaders();
        bool DrawTrackButtons(Track^ track, System::Drawing::Rectangle trackHeaderBounds);
        bool DrawTrackButtonText(System::Drawing::Rectangle trackHeaderBounds, int buttonIndex, System::String^ text, bool isPressed, bool isHovered, System::Drawing::Color baseColor, System::Drawing::Color textColor);
        bool DrawTrackButtonIcon(System::Drawing::Rectangle trackHeaderBounds, int buttonIndex, System::Drawing::Image^ icon, bool isPressed, bool isHovered, System::Drawing::Color baseColor, System::Drawing::Color textColor);
        void DrawTrackButtonBase(System::Drawing::Rectangle buttonBounds, bool isPressed, bool isHovered, System::Drawing::Color baseColor);
        bool DrawTrackDividers(Track^ hoverTrack);


        // Sub-Methods for DrawToolPreview
        void DrawToolPreviewPointerTool();
        void DrawToolPreviewPointerToolMoving();
        void DrawToolPreviewPointerToolResizing();
        void DrawToolPreviewPointerToolPasting();
        void DrawToolPreviewDrawTool();
        void DrawToolPreviewDrawToolDraw();
		void DrawToolPreviewDrawToolErase();
		void DrawToolPreviewDrawToolMove();
		void DrawToolPreviewDrawToolResize();
        void DrawToolPreviewEraseTool();
		void DrawToolPreviewEraseToolDrawCross(System::Drawing::Rectangle barBounds, bool isHovered);
        void DrawToolPreviewDurationTool();
        void DrawToolPreviewColorTool();
        bool DrawToolPreviewFadeTool();
        bool DrawToolPreviewStrobeTool();

		// Sub-Methods for Left Panel Rendering
		void DrawLeftPanelExpanded(bool beingResized);
		void DrawLeftPanelContent();
		void DrawLeftPanelHidden();
		void DrawLeftPanelToggleButton(float x, float y, bool isCollapsed);
		void DrawLeftPanelEventProperties(BarEvent^ event, float startY);
		void DrawLeftPanelSectionHeader(const std::wstring& headerText, float Y);
		void DrawLeftPanelPropertyRow(const std::wstring& label, const std::wstring& value, float Y);
		void DrawLeftPanelColorProperty(const std::wstring& label, System::Drawing::Color color, float Y);
		void DrawLeftPanelSolidEventProperties(BarEvent^ event, float startY);
		void DrawLeftPanelFadeEventProperties(BarEvent^ event, float startY);
		void DrawLeftPanelStrobeEventProperties(BarEvent^ event, float startY);
		void DrawLeftPanelMultipleEventProperties(float startY);
		std::wstring GetEventTypeText(BarEventType type);
		std::wstring GetEasingText(Easing easing);
		std::wstring FormatTickPosition(int tick);
		std::wstring FormatTickDuration(int durationTicks);

        // Smaller Supporting Drawing Methods
		void DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
		void DrawNormalBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawNormalBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawNormalBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawPreviewBar(BarEvent^ bar, Track^ track, System::Drawing::Point mousePos, BarPreviewType previewType);
        void DrawPreviewBarList(List<BarEvent^>^ bars, Track^ track);
        void DrawCreationMovementPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawCreationMovementPreviewSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawCreationMovementPreviewFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawCreationMovementPreviewStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreviewSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreviewFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreviewStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawColorPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds, System::Drawing::Color currentColor, float opacity, float borderWidth);

        void DrawGhostBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
        void DrawGhostBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawGhostBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawGhostBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawSelectedBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
        void DrawPastePreviewBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
        void DrawPastePreviewBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawPastePreviewBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawPastePreviewBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawBarGlowEffect(System::Drawing::Rectangle barBounds, System::Drawing::Color glowColor, int glowLevels);
        void DrawSelectionRectangle(System::Drawing::Rectangle selectionRectangle);
		void DrawToolEnhancements(BarEvent^ bar, System::Drawing::Rectangle barBounds);
		void DrawResizeHandle(System::Drawing::Rectangle barBounds, bool isTargeted);

        // Drawing Support Methods
        float TicksToPixels(int ticks);
        float PixelsToTicks(int pixels);
        float GetSubdivisionLevel(float pixelsPerBeat);
        int GetMeasureSkipFactor(float pixelsBetweenMeasures);
        int GetTrackTop(Track^ track);
		int GetLeftPanelWidth();
		int GetLeftPanelAndTrackHeaderWidth();
        float GetTotalTrackHeight();
        Track^ GetTrackAtPoint(System::Drawing::Point point);
        Measure^ GetMeasureAtTick(int tick);
        BarEvent^ GetBarAtPoint(System::Drawing::Point point);
        System::Drawing::Rectangle GetTrackBounds(Track^ track);
        System::Drawing::Rectangle GetTrackHeaderBounds(Track^ track);
        System::Drawing::Rectangle GetTrackContentBounds(Track^ track);
        System::Drawing::Rectangle GetBarBounds(BarEvent^ bar, System::Drawing::Rectangle bounds);
        System::Drawing::Rectangle GetGhostBarBounds(BarEvent^ bar, System::Drawing::Rectangle bounds);
        System::Drawing::Rectangle GetTrackButtonBounds(int buttonIndex, System::Drawing::Rectangle trackHeaderBounds);

    private:
        System::Resources::ResourceManager^ _Resources;
        Timeline_Direct2DRenderer_Native* _NativeRenderer;	// Pointer to native renderer
        System::IntPtr _ControlHandle;						// Handle to the managed control
        
        bool m_disposed;
        bool m_themeColorsCached;
        CachedThemeColors m_ColorTheme;
        LevelOfDetail m_LevelOfDetail;

        System::Windows::Forms::Control^ _Control;
        List<Track^>^			_Tracks;
        List<Measure^>^			_Measures;
		Collapsible_Left_Panel^	_Left_Panel;
        float					_ZoomLevel;
        System::Drawing::Point^	_ScrollPosition;
        ITimelineAccess^		_ToolAccessDelegate;
       

        std::wstring ConvertString(System::String^ str);
		std::wstring ToHexString(int value);

	protected:
		virtual void Cleanup();
    };
}
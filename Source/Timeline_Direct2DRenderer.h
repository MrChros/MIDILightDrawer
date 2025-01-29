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
    
    ref class TimelineTool;
    ref class PointerTool;
    ref class DrawTool;
    ref class ColorTool;
    ref class DurationTool;
    
    enum class TimelineToolType;
    enum class DrumNotationType;
    enum class BarPreviewType;

    value struct TabStringInfo;
    value struct TrackButtonId;


    // Managed wrapper for TimelineD2DRenderer
    public ref class Timeline_Direct2DRenderer
    {
    public:
        static const int TICKS_PER_QUARTER				= 960;
        static const int HEADER_HEIGHT					= 60;
        static const int MIN_PIXELS_BETWEEN_GRIDLINES   = 20;
        static const int TRACK_HEADER_WIDTH				= 150;
        static const int TRACK_PADDING					= 4;
        static const int BUTTON_SIZE					= 24;
		static const int BUTTON_MARGIN					= 6;
        static const float FIXED_STRING_SPACING			= 12.0f;

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

    public:
        Timeline_Direct2DRenderer(List<Track^>^ tracks, List<Measure^>^ measures, double zoomlevel, System::Drawing::Point^ scrollposition);
        virtual ~Timeline_Direct2DRenderer();
		!Timeline_Direct2DRenderer();

        // Initialization
        virtual bool Initialize(System::Windows::Forms::Control^ control);
        virtual void Resize(int width, int height);
        virtual void SetThemeColors(System::Drawing::Color background, System::Drawing::Color headerbackground, System::Drawing::Color text, System::Drawing::Color measureline, System::Drawing::Color beatline, System::Drawing::Color subdivisionline, System::Drawing::Color selectionhighlight, System::Drawing::Color trackbackground, System::Drawing::Color trackborder);
        virtual void SetZoomLevel(double zoomlevel);
        virtual void SetScrollPositionReference(System::Drawing::Point^ scrollposition);
        virtual void SetTimelineAccess(ITimelineAccess^ access);
        virtual void PreloadImages();

        // Drawing Methods
		virtual bool BeginDraw();
		virtual bool EndDraw();


        // Widget Timeline Drawing 
        virtual bool DrawWidgetBackground();
        virtual bool DrawTrackBackground();
        virtual bool DrawMeasureNumbers();
        virtual bool DrawTrackContent(Track^ hoverTrack);
        virtual bool DrawToolPreview();


        // Sub-Methods for DrawMeasureNumbers
		virtual bool DrawBeatNumbers(Measure^ measure, float x, float measureNumberY, float subdivLevel, int measureNumber, int ticksPerBeat);


        // Sub-Methods for DrawTrackContent
        virtual bool DrawGridLines(float totalHeight);
        virtual bool DrawSubdivisionLines(float totalHeight, int startTick, int endTick);
        virtual bool DrawBeatLines(float totalHeight, int startTick, int endTick);
        virtual bool DrawMeasureLines(float totalHeight, int startTick, int endTick);

        // Sub-Methods for DrawTrackContent - DrawTrackEvents and DrawTrackTablature
        virtual bool DrawTrackEvents(Track^ track, System::Drawing::Rectangle trackContentBounds, TimelineToolType currentToolType);
        virtual bool DrawTrackTablature(Track^ track, System::Drawing::Rectangle trackContentBounds);
        virtual bool DrawTrackTablatureDrum(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale);
        virtual bool DrawTrackTablatureRegular(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale);
        virtual void DrawBeatDuration(Beat^ beat, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions);
        virtual void DrawTieLines(Track^ track, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions, float scaledFontSize);
        virtual void DrawDrumSymbol(DrumNotationType symbolType, float x, float y, float size);
        virtual TabStringInfo DrawTablatureStrings(System::Drawing::Rectangle bounds, float availableHeight, float logScale, int numStrings);
        
        virtual bool DrawTrackHeaders();
        virtual bool DrawTrackButtons(Track^ track, System::Drawing::Rectangle trackHeaderBounds);
        virtual bool DrawTrackButtonText(System::Drawing::Rectangle trackHeaderBounds, int buttonIndex, System::String^ text, bool isPressed, bool isHovered, System::Drawing::Color baseColor, System::Drawing::Color textColor);
        virtual bool DrawTrackButtonIcon(System::Drawing::Rectangle trackHeaderBounds, int buttonIndex, System::Drawing::Image^ icon, bool isPressed, bool isHovered, System::Drawing::Color baseColor, System::Drawing::Color textColor);
        virtual void DrawTrackButtonBase(System::Drawing::Rectangle buttonBounds, bool isPressed, bool isHovered, System::Drawing::Color baseColor);
        virtual bool DrawTrackDividers(Track^ hoverTrack);


        // Sub-Methods for DrawToolPreview
        bool DrawToolPreviewPointerTool();
        bool DrawToolPreviewPointerToolMoving();
        bool DrawToolPreviewPointerToolPasting();
        bool DrawToolPreviewDrawTool();
        bool DrawToolPreviewDrawToolDraw();
        bool DrawToolPreviewDrawToolErase();
        bool DrawToolPreviewDrawToolMove();
        bool DrawToolPreviewDrawToolResize();
        bool DrawToolPreviewEraseTool();
        bool DrawToolPreviewDurationTool();
        bool DrawToolPreviewColorTool();
        bool DrawToolPreviewFadeTool();
        bool DrawToolPreviewStrobeTool();


        // Smaller Supporting Drawing Methods
		void DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
		void DrawNormalBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawNormalBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawNormalBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawPreviewBar(BarEvent^ bar, Track^ track, System::Drawing::Point mousePos, BarPreviewType previewType);
        bool DrawPreviewBarList(List<BarEvent^>^ bars, Track^ track);
        bool DrawCreationMovementPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreviewSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreviewFade(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        void DrawDurationPreviewStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds);

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
        void DrawResizeHandle(System::Drawing::Rectangle barBounds, bool isTargeted);
        void DrawSelectionRectangle(System::Drawing::Rectangle selectionRectangle);

        // Drawing Support Methods
        float TicksToPixels(int ticks);
        float PixelsToTicks(int pixels);
        float GetSubdivisionLevel(float pixelsPerBeat);
        int GetTrackTop(Track^ track);
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
        Timeline_Direct2DRenderer_Native* _NativeRenderer;  // Pointer to native renderer
        System::IntPtr _ControlHandle;                      // Handle to the managed control
        
        bool m_disposed;
        bool m_themeColorsCached;
        CachedThemeColors m_ColorTheme;

        System::Windows::Forms::Control^ _Control;
        List<Track^>^           _Tracks;
        List<Measure^>^         _Measures;
        float                   _ZoomLevel;
        System::Drawing::Point^ _ScrollPosition;
        ITimelineAccess^        _ToolAccessDelegate;
       

        std::wstring ConvertString(System::String^ str);

	protected:
		virtual void Cleanup();
    };

} // namespace MIDILightDrawer
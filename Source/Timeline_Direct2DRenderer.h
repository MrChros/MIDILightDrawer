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


    // Managed wrapper for TimelineD2DRenderer
    public ref class Timeline_Direct2DRenderer
    {
    public:
        static const int TICKS_PER_QUARTER				= 960;
        static const int HEADER_HEIGHT					= 60;
        static const int MIN_PIXELS_BETWEEN_GRIDLINES   = 20;
        static const int TRACK_HEADER_WIDTH				= 150;
        static const int TRACK_PADDING					= 4;
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
        ~Timeline_Direct2DRenderer();
        !Timeline_Direct2DRenderer();

        // Initialization
        bool Initialize(System::Windows::Forms::Control^ control);
        void Resize(int width, int height);
        void SetThemeColors(System::Drawing::Color background, System::Drawing::Color headerbackground, System::Drawing::Color text, System::Drawing::Color measureline, System::Drawing::Color beatline, System::Drawing::Color subdivisionline, System::Drawing::Color selectionhighlight, System::Drawing::Color trackbackground, System::Drawing::Color trackborder);
        void SetZoomLevel(double zoomlevel);
        void SetScrollPositionReference(System::Drawing::Point^ scrollposition);
        void SetTimelineAccess(ITimelineAccess^ access);

        // Drawing Methods
        bool BeginDraw();
        bool EndDraw();


        // Widget Timeline Drawing 
        bool DrawWidgetBackground();
        bool DrawTrackBackground();
        bool DrawMeasureNumbers();
        bool DrawTrackContent(Track^ hoverTrack);
        bool DrawToolPreview();


        // Sub-Methods for DrawMeasureNumbers
        bool DrawBeatNumbers(Measure^ measure, float x, float measureNumberY, float subdivLevel, int measureNumber, int ticksPerBeat);


        // Sub-Methods for DrawTrackContent
        bool DrawGridLines(float totalHeight);
        bool DrawSubdivisionLines(float totalHeight, int startTick, int endTick);
        bool DrawBeatLines(float totalHeight, int startTick, int endTick);
        bool DrawMeasureLines(float totalHeight, int startTick, int endTick);

        // Sub-Methods for DrawTrackContent - DrawTrackEvents and DrawTrackTablature
        bool DrawTrackEvents(Track^ track, System::Drawing::Rectangle trackContentBounds, TimelineToolType currentToolType);
        bool DrawTrackTablature(Track^ track, System::Drawing::Rectangle trackContentBounds);
        bool DrawTrackTablatureDrum(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale);
        bool DrawTrackTablatureRegular(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale);
        void DrawBeatDuration(Beat^ beat, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions);
        void DrawTieLines(Track^ track, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions, float scaledFontSize);
        void DrawDrumSymbol(DrumNotationType symbolType, float x, float y, float size);
        TabStringInfo DrawTablatureStrings(System::Drawing::Rectangle bounds, float availableHeight, float logScale, int numStrings);
        
        bool DrawTrackHeaders();
        bool DrawTrackButtons(Track^ track, System::Drawing::RectangleF headerBounds);
        void DrawRoundedButtonBackground(System::Drawing::RectangleF bounds, bool isPressed, bool isHovered, System::Drawing::Color baseColor);
        bool DrawTrackDividers(Track^ hoverTrack);


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
        bool DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle bounds);
        bool DrawPreviewBar(BarEvent^ bar, Track^ track, System::Drawing::Point mousePos, BarPreviewType previewType);
        bool DrawPreviewBarList(List<BarEvent^>^ bars, Track^ track);
        bool DrawCreationMovementPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds);
        bool DrawDurationPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds);

        bool DrawGhostBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
        bool DrawSelectedBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
        bool DrawPastePreviewBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds);
        bool DrawDropTargetIndicator(System::Drawing::Rectangle bounds);
        void DrawMoveHandles(System::Drawing::Rectangle barBounds); // ToDo here...
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

    private:
        bool m_disposed;
        bool m_themeColorsCached;
        CachedThemeColors m_ColorTheme;

        Timeline_Direct2DRenderer_Native* m_pNativeRenderer;    // Pointer to native renderer
        System::IntPtr m_controlHandle;                         // Handle to the managed control

        System::Windows::Forms::Control^ _Control;
        List<Track^>^           _Tracks;
        List<Measure^>^         _Measures;
        float                   _ZoomLevel;
        System::Drawing::Point^ _ScrollPosition;
        ITimelineAccess^        _ToolAccessDelegate;
       

        std::wstring ConvertString(System::String^ str);
        void Cleanup();
    };

} // namespace MIDILightDrawer
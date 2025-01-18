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
    ref class BarEvent;
    enum class TimelineToolType;
    ref class TimelineTool;
    ref class PointerTool;
    ref class DrawTool;
    ref class ColorTool;
    ref class DurationTool;

    // Managed wrapper for TimelineD2DRenderer
    public ref class Timeline_Direct2DRenderer
    {
    public:
        static const int TICKS_PER_QUARTER				= 960;
        static const int HEADER_HEIGHT					= 60;
        static const int MIN_PIXELS_BETWEEN_GRIDLINES   = 20;
        static const int TRACK_HEADER_WIDTH				= 150;
        static const int TRACK_PADDING					= 4;

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

        // Widget Timeline Drawing Sub-Methods
        bool DrawBeatNumbers(Measure^ measure, float x, float measureNumberY, float subdivLevel, int measureNumber, int ticksPerBeat);

        bool DrawGridLines(float totalHeight);
        bool DrawSubdivisionLines(float totalHeight, int startTick, int endTick);
        bool DrawBeatLines(float totalHeight, int startTick, int endTick);
        bool DrawMeasureLines(float totalHeight, int startTick, int endTick);

        bool DrawTrackEvents(Track^ track, System::Drawing::Rectangle contentBounds, TimelineToolType currentToolType);
        bool DrawTrackTablature(Track^ track, System::Drawing::Rectangle contentBounds, System::Drawing::Color stringColor);
        bool DrawTrackHeaders();

        bool DrawTrackButtons(Track^ track, System::Drawing::RectangleF headerBounds);
        void DrawRoundedButtonBackground(System::Drawing::RectangleF bounds, bool isPressed, bool isHovered, System::Drawing::Color baseColor);

        bool DrawTrackDividers(Track^ hoverTrack);

        // Smaller Supporting Drawing Methods
        bool DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle bounds);
        bool DrawGhostBar(BarEvent^ bar, System::Drawing::Rectangle bounds);
        bool DrawSelectedBar(BarEvent^ bar, System::Drawing::Rectangle bounds);
        bool DrawPastePreviewBar(BarEvent^ bar, System::Drawing::Rectangle bounds);
        bool DrawDropTargetIndicator(System::Drawing::Rectangle bounds);
        void DrawBarGlowEffect(System::Drawing::Rectangle barBounds, System::Drawing::Color glowColor, int glowLevels);

        // Drawing Support Methods
        float TicksToPixels(int ticks);
        float PixelsToTicks(int pixels);
        float GetSubdivisionLevel(float pixelsPerBeat);
        int GetTrackTop(Track^ track);
        float GetTotalTrackHeight();


        // Original Drawing Operations (keeping these for backward compatibility)
        bool DrawLine(float x1, float y1, float x2, float y2, System::Drawing::Color color, float strokeWidth);
        bool DrawRectangle(System::Drawing::RectangleF rect, System::Drawing::Color color, float strokeWidth);
        bool FillRectangle(System::Drawing::RectangleF rect, System::Drawing::Color color);
        bool DrawGridLine(float x1, float y1, float x2, float y2, System::Drawing::Color color, bool isDashed);
        bool DrawMeasureLine(float x, float y1, float y2, System::Drawing::Color color);

        // New Drawing Operations (more explicit parameter passing)
        bool DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float strokeWidth);
        bool DrawRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth);
        bool FillRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a);
        bool DrawGridLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, bool isDashed);
        bool DrawMeasureLine(float x, float y1, float y2, float r, float g, float b, float a);

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
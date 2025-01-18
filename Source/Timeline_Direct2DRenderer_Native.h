#pragma once

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "user32.lib")

#include <d2d1.h>
#include <vector>
#include <string>
#include <dwrite.h>

// Forward declarations
struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1SolidColorBrush;
struct ID2D1StrokeStyle;
struct IDWriteFactory;

struct MeasureInfo {
    int number;
    int startTick;
    int length;
    int numerator;
    int denominator;
    std::wstring markerText;
};

class Timeline_Direct2DRenderer_Native
{
public:
    Timeline_Direct2DRenderer_Native();
    ~Timeline_Direct2DRenderer_Native();

    // Initialization
    bool Initialize(HWND hwnd);
    void Cleanup();

    // Resource Management
    bool CreateDeviceResources();
    void ReleaseDeviceResources();

    // Add getters for text formats
    IDWriteTextFormat* GetMeasureNumberFormat() const { return m_pMeasureNumberFormat; }
    IDWriteTextFormat* GetMarkerTextFormat()    const { return m_pMarkerTextFormat; }
    IDWriteTextFormat* GetTimeSignatureFormat() const { return m_pTimeSignatureFormat; }
    IDWriteTextFormat* GetQuarterNoteFormat()   const { return m_pQuarterNoteFormat; }
    IDWriteTextFormat* GetTrackHeaderFormat()   const { return m_pTrackHeaderFormat; }

    // Render Target Management
    bool ResizeRenderTarget(UINT width, UINT height);
    bool BeginDraw();
    bool EndDraw();
    void Clear(const D2D1_COLOR_F& color);
    void Clear(float r, float g, float b, float a);

    // Original methods with D2D1 types
    bool DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float strokeWidth);
    bool DrawLineDashed(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth, const float* dashes, UINT32 dashCount);
    bool DrawRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth);
    bool FillRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    bool FillRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a);
    bool DrawRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth);
    bool FillRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color);
    bool FillRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a);
    bool DrawEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float strokeWidth);
    bool FillEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color);
   
    // Path Drawing
    bool DrawPath(ID2D1Geometry* geometry, const D2D1_COLOR_F& color, float strokeWidth);
    bool FillPath(ID2D1Geometry* geometry, const D2D1_COLOR_F& color);

    // Text Drawing
    bool DrawText(const std::wstring& text, const D2D1_RECT_F& layoutRect, const D2D1_COLOR_F& color, IDWriteTextFormat* textFormat, D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE);

    // Layer Support
    bool PushLayer(const D2D1_LAYER_PARAMETERS& layerParameters, ID2D1Layer* layer);
    void PopLayer();

    // Clipping
    void PushAxisAlignedClip(const D2D1_RECT_F& clipRect, D2D1_ANTIALIAS_MODE antialiasMode);
    void PopAxisAlignedClip();

    // Transform Operations
    void SetTransform(const D2D1_MATRIX_3X2_F& transform);
    void GetTransform(D2D1_MATRIX_3X2_F* transform);

    // Resource Creation Helpers
    bool CreateLayer(ID2D1Layer** layer);
    bool CreatePathGeometry(ID2D1PathGeometry** geometry);
   
    //bool DrawGridLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, bool isDashed);
    //bool DrawGridLineRaw(float x1, float y1, float x2, float y2, float r, float g, float b, float a, bool isDashed);
    //bool DrawMeasureLine(float x, float y1, float y2, const D2D1_COLOR_F& color);
    //bool DrawMeasureLineRaw(float x, float y1, float y2, float r, float g, float b, float a);

    //bool DrawMeasureNumbers(float width, float headerHeight, const std::vector<MeasureInfo>& measures, const D2D1_COLOR_F& headerBackground, const D2D1_COLOR_F& textColor, float scrollX, float zoomLevel);

private:
    // Direct2D Resources
    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;

    // DirectWrite resources
    IDWriteFactory* m_pDWriteFactory;
    IDWriteTextFormat* m_pMeasureNumberFormat;  // For measure numbers
    IDWriteTextFormat* m_pMarkerTextFormat;     // For marker text
    IDWriteTextFormat* m_pTimeSignatureFormat;  // For time signatures
    IDWriteTextFormat* m_pQuarterNoteFormat;    // For sub time-divisions
    IDWriteTextFormat* m_pTrackHeaderFormat;    // For Track Header Text

    // Cached Resources
    struct CachedBrush {
        D2D1_COLOR_F color;
        ID2D1SolidColorBrush* brush;
    };
    std::vector<CachedBrush> m_brushCache;

    // Stroke Styles
    ID2D1StrokeStyle* m_pDashedStroke;
    ID2D1StrokeStyle* m_pSolidStroke;

    // Resource Management Helpers
    ID2D1SolidColorBrush* GetCachedBrush(const D2D1_COLOR_F& color);
    bool CreateStrokeStyles();
    bool CreateTextFormats();

    // Window Handle
    HWND m_hwnd;

    // State Tracking
    bool m_resourcesValid;

    // Prevent copying
    Timeline_Direct2DRenderer_Native(const Timeline_Direct2DRenderer_Native&) = delete;
    Timeline_Direct2DRenderer_Native& operator=(const Timeline_Direct2DRenderer_Native&) = delete;
};
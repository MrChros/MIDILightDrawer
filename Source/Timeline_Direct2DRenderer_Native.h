#pragma once

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "user32.lib")

#include <d2d1.h>
#include <vector>
#include <string>
#include <dwrite.h>
#include <utility>
#include <unordered_map>


// Defines
# define DURATION_SYMBOL_DURATIONS_COUNT	15


// Forward declarations
struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1SolidColorBrush;
struct ID2D1StrokeStyle;
struct IDWriteFactory;

struct CachedBrush
{
	D2D1_COLOR_F Color;
	ID2D1SolidColorBrush* Brush;
};

struct CachedBitmap
{
	std::wstring Key;
	ID2D1Bitmap* Bitmap;
};

struct TabTextCacheEntry
{
	float FontSize;
	std::unordered_map<int, ID2D1Bitmap*> TabTexts;
	std::unordered_map<int, float> TextWidths;
};

struct DrumSymbolCacheEntry
{
	float StringSpace;
	std::unordered_map<int, ID2D1Bitmap*> DrumSymbols;
};

struct DurationSymbolCacheEntry
{
	float ZoomLevel;
	std::unordered_map<int, ID2D1Bitmap*> DurationSymbols;
	ID2D1Bitmap* DotSymbol;
	ID2D1Bitmap* TripletText;
};

class Timeline_Direct2DRenderer_Native
{
private:
	const int POSSIBLE_DURATIONS[DURATION_SYMBOL_DURATIONS_COUNT] = {
		3840,	// Whole note (960 * 4)
		1920,	// Half note (960 * 2)
		960,	// Quarter note
		480,	// 8th note
		240,	// 16th note
		120,	// 32nd note
		5760,	// Dotted whole note
		2880,	// Dotted half note
		1440,	// Dotted quarter note
		720,	// Dotted 8th note
		360,	// Dotted 16th note
		180,	// Dotted 32nd note
		640,	// Quarter note triplet
		320,	// 8th note triplet
		160		// 16th note triplet
	};

	
public:
    Timeline_Direct2DRenderer_Native();
    ~Timeline_Direct2DRenderer_Native();

    // Initialization
    bool Initialize(HWND hwnd);
    void Cleanup();

    // Resource Management
    bool CreateDeviceResources();
    void ReleaseDeviceResources();
	bool PreloadTabText(float fontSize, const D2D1_COLOR_F& textColor, const D2D1_COLOR_F& bgColor);
	bool PreloadDrumSymbol(float stringSpace, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor);
	bool PreloadDurationSymbols(float zoomLevel, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor);

    // Add getters for text formats
    IDWriteTextFormat* GetMeasureNumberFormat() const { return m_pMeasureNumberFormat;	}
    IDWriteTextFormat* GetMarkerTextFormat()    const { return m_pMarkerTextFormat;		}
    IDWriteTextFormat* GetTimeSignatureFormat() const { return m_pTimeSignatureFormat;	}
    IDWriteTextFormat* GetQuarterNoteFormat()   const { return m_pQuarterNoteFormat;	}
    IDWriteTextFormat* GetTrackHeaderFormat()   const { return m_pTrackHeaderFormat;	}
    IDWriteTextFormat* UpdateTablatureFormat(float fontSize);

    // Render Target Management
    bool ResizeRenderTarget(UINT width, UINT height);
    bool BeginDraw();
    bool EndDraw();
    void Clear(const D2D1_COLOR_F& color);
    void Clear(float r, float g, float b, float a);

    // Original methods with D2D1 types
    bool DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float strokeWidth);
    bool DrawLines(const D2D1_POINT_2F* points, UINT32 pointCount, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth);
    bool DrawRectangleDashed(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float dashLength, float gapLength);
    bool FillRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    bool FillRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a);
	bool FillRectangleGradient2(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorLeft, const D2D1_COLOR_F& colorRight);
	bool FillRectangleGradient3(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorLeft, const D2D1_COLOR_F& colorCenter, const D2D1_COLOR_F& colorRight);
	bool FillRectangleGradient(const D2D1_RECT_F& rect, const D2D1_GRADIENT_STOP* gradientStops, UINT32 count);
	bool FillRectangleStripes(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float stripeWidth);
	bool DrawRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float strokeWidth);
    bool DrawRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth);
    bool FillRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color);
    bool FillRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a);
    bool DrawEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float strokeWidth);
    bool FillEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color);
    bool DrawPolygon(const D2D1_POINT_2F* points, UINT32 pointCount, const D2D1_COLOR_F& color, float strokeWidth);
    bool FillDiamond(const D2D1_POINT_2F* points, UINT32 pointCount, const D2D1_COLOR_F& color);
    bool DrawTieLine(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2, const D2D1_POINT_2F& point3, const D2D1_POINT_2F& point4, const D2D1_COLOR_F& color, float strokeWidth);

    // Path Drawing
    bool DrawPath(ID2D1Geometry* geometry, const D2D1_COLOR_F& color, float strokeWidth);
    bool FillPath(ID2D1Geometry* geometry, const D2D1_COLOR_F& color);

    // Text Drawing
    bool DrawText(const std::wstring& text, const D2D1_RECT_F& layoutRect, const D2D1_COLOR_F& color, IDWriteTextFormat* textFormat, D2D1_DRAW_TEXT_OPTIONS options = D2D1_DRAW_TEXT_OPTIONS_NONE);
    float MeasureTextWidth(const std::wstring& text, IDWriteTextFormat* textFormat);

    // Bitmap Drawing
    bool PreloadBitmap(const std::wstring& key, System::Drawing::Image^ image);
    bool DrawCachedBitmap(const std::wstring& key, const D2D1_RECT_F& destRect, float opacity = 1.0f);

	// Draw Cached Texts & Symbols
	bool DrawCachedTabText(int fretNumber, const D2D1_RECT_F& destRect, float fontSize);
	bool DrawCachedDrumSymbol(int symbolIndex, const D2D1_RECT_F& destRect, float stringSpace);
	TabTextCacheEntry* GetNearestCachedTabTextEntry(float fontSize);
	DrumSymbolCacheEntry* GetNearestCachedDrumSymbolEntry(float stringSpace);

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
   
private:
    // Direct2D Resources
    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;

	HWND m_hwnd;			// Window Handle
	bool m_resourcesValid;	// State Tracking

	// Stroke Styles
	ID2D1StrokeStyle* m_pSolidStroke;
	ID2D1StrokeStyle* m_pDashedStroke;

	ID2D1LinearGradientBrush* m_pLinearGradientBrush;

    // DirectWrite resources
    IDWriteFactory* m_pDWriteFactory;
    IDWriteTextFormat* m_pMeasureNumberFormat;  // For measure numbers
    IDWriteTextFormat* m_pMarkerTextFormat;     // For marker text
    IDWriteTextFormat* m_pTimeSignatureFormat;  // For time signatures
    IDWriteTextFormat* m_pQuarterNoteFormat;    // For sub time-divisions
    IDWriteTextFormat* m_pTrackHeaderFormat;    // For Track Header Text
    IDWriteTextFormat* m_pTablatureFormat;      // For Tablature Text

    // Cached Resources
    std::vector<CachedBrush>				m_BrushCache;
    std::vector<CachedBitmap>				m_BitmapCache;
	std::vector<TabTextCacheEntry>			m_TabTextCache;
	std::vector<DrumSymbolCacheEntry>		m_DrumSymbolCache;
	std::vector<DurationSymbolCacheEntry>	m_DurationSymbolCache;
	
    // Resource Management Helpers
	bool CreateStrokeStyles();
	bool CreateTextFormats();
	bool CreateTabTextBitmap(const std::wstring& text, float fontSize, ID2D1Bitmap** ppBitmap, const D2D1_COLOR_F& textColor, const D2D1_COLOR_F& bgColor);
	bool CreateDrumSymbolBitmap(int symbolType, float size, ID2D1Bitmap** ppBitmap, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor);
	ID2D1Geometry* CreateDrumSymbolGeometry(int symbolType, float size, float bitmapSize);

	void CleanupTablatureCache();	// Obsolete?

	bool CreateBitmapFromImage(System::Drawing::Image^ image, ID2D1Bitmap** ppBitmap);
	bool CreateAndCacheBitmap(const std::wstring& key, System::Drawing::Image^ image);
	bool DrawBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& destRect, float opacity = 1.0f);

	ID2D1SolidColorBrush* GetCachedBrush(const D2D1_COLOR_F& color);
	ID2D1Bitmap* GetCachedBitmap(const std::wstring& key);

	// Prevent copying
	Timeline_Direct2DRenderer_Native(const Timeline_Direct2DRenderer_Native&) = delete;
	Timeline_Direct2DRenderer_Native& operator=(const Timeline_Direct2DRenderer_Native&) = delete;
};
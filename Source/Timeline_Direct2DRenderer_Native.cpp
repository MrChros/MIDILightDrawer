#include "Timeline_Direct2DRenderer_Native.h"

#include <algorithm>

Timeline_Direct2DRenderer_Native::Timeline_Direct2DRenderer_Native():
    m_hwnd(nullptr),
    m_pD2DFactory(nullptr),
    m_pRenderTarget(nullptr),
    m_pSolidStroke(nullptr),
	m_pLinearGradientBrush(nullptr),
    m_pDWriteFactory(nullptr),
    m_pMeasureNumberFormat(nullptr),
    m_pMarkerTextFormat(nullptr),
    m_pTimeSignatureFormat(nullptr),
    m_pQuarterNoteFormat(nullptr),
    m_pTrackHeaderFormat(nullptr),
    m_pTablatureFormat(nullptr),
    m_resourcesValid(false)
{
}

Timeline_Direct2DRenderer_Native::~Timeline_Direct2DRenderer_Native()
{
    Cleanup();
}

bool Timeline_Direct2DRenderer_Native::Initialize(HWND hwnd)
{
    if (!hwnd) {
        return false;
	}

    m_hwnd = hwnd;

    // Create D2D factory
    D2D1_FACTORY_OPTIONS options = {};

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, reinterpret_cast<void**>(&m_pD2DFactory));

    if (FAILED(hr)) {
        return false;
	}

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));

    if (FAILED(hr))
        return false;

    // Create text formats
    if (!CreateTextFormats()) {
        return false;
	}

    // Create stroke styles
    if (!CreateStrokeStyles()) {
        return false;
	}

    return CreateDeviceResources();
}

void Timeline_Direct2DRenderer_Native::Cleanup()
{
    ReleaseDeviceResources();

    // Release brush cache
    for (auto& cached : m_brushCache)
    {
        if (cached.brush)
            cached.brush->Release();
    }
    m_brushCache.clear();

    // Release bitmap cache
    for (auto& cached : m_bitmapCache) {
        if (cached.bitmap) {
            cached.bitmap->Release();
        }
    }
    m_bitmapCache.clear();

    // Release text formats
    if (m_pMeasureNumberFormat)
    {
        m_pMeasureNumberFormat->Release();
        m_pMeasureNumberFormat = nullptr;
    }

    if (m_pMarkerTextFormat)
    {
        m_pMarkerTextFormat->Release();
        m_pMarkerTextFormat = nullptr;
    }

    if (m_pTimeSignatureFormat)
    {
        m_pTimeSignatureFormat->Release();
        m_pTimeSignatureFormat = nullptr;
    }

    if (m_pQuarterNoteFormat)
    {
        m_pQuarterNoteFormat->Release();
        m_pQuarterNoteFormat = nullptr;
    }

    if (m_pTrackHeaderFormat)
    {
        m_pTrackHeaderFormat->Release();
        m_pTrackHeaderFormat = nullptr;
    }

    if (m_pTablatureFormat)
    {
        m_pTablatureFormat->Release();
        m_pTablatureFormat = nullptr;
    }

    // Release DirectWrite factory
    if (m_pDWriteFactory)
    {
        m_pDWriteFactory->Release();
        m_pDWriteFactory = nullptr;
    }

    if (m_pSolidStroke)
    {
        m_pSolidStroke->Release();
        m_pSolidStroke = nullptr;
    }

    // Release factory
    if (m_pD2DFactory)
    {
        m_pD2DFactory->Release();
        m_pD2DFactory = nullptr;
    }
}

bool Timeline_Direct2DRenderer_Native::CreateDeviceResources()
{
    if (!m_hwnd || !m_pD2DFactory)
        return false;

    if (m_resourcesValid)
        return true;

    // Get window size
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Create render target
    /*
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(m_hwnd, size);
    */

    // Create render target with optimized properties
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE,  // Use hardware rendering
        D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED
        ),
        96.0f,  // Default DPI
        96.0f
    );

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        m_hwnd,
        D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top),
        D2D1_PRESENT_OPTIONS_IMMEDIATELY  // Present results immediately
    );

    HRESULT hr = m_pD2DFactory->CreateHwndRenderTarget(
        rtProps,
        hwndProps,
        &m_pRenderTarget
    );

    if (FAILED(hr)) {
        return false;
    }

    // Enable more conservative rendering options
    //m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    //m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);

    m_resourcesValid = true;
    return true;
}

void Timeline_Direct2DRenderer_Native::ReleaseDeviceResources()
{
    // Release brushes
    for (auto& cached : m_brushCache)
    {
        if (cached.brush)
            cached.brush->Release();
    }
    m_brushCache.clear();

    // Release render target
    if (m_pRenderTarget)
    {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }

    m_resourcesValid = false;
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::UpdateTablatureFormat(float fontsize)
{
    if (!m_pDWriteFactory)
        return nullptr;

    // Release previous dynamic text format if it exists
    if (m_pTablatureFormat)
    {
        m_pTablatureFormat->Release();
        m_pTablatureFormat = nullptr;
    }

    // Create new text format with specified size
    HRESULT hr = m_pDWriteFactory->CreateTextFormat(
        L"Arial",               // Font family name
        NULL,                   // Font collection (NULL sets it to use system font collection)
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontsize,              // Font size
        L"en-us",              // Locale
        &m_pTablatureFormat
    );

    if (FAILED(hr)) {
        return nullptr;
    }

    // Set text alignment
    m_pTablatureFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pTablatureFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return m_pTablatureFormat;
}

bool Timeline_Direct2DRenderer_Native::ResizeRenderTarget(UINT width, UINT height)
{
    if (!m_pRenderTarget) {
        return false;
    }

    HRESULT hr = m_pRenderTarget->Resize(D2D1::SizeU(width, height));

    return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::BeginDraw()
{
    if (!m_resourcesValid && !CreateDeviceResources())
        return false;

    m_pRenderTarget->BeginDraw();
    return true;
}

bool Timeline_Direct2DRenderer_Native::EndDraw()
{
    if (!m_pRenderTarget) {
        return false;
    }

    HRESULT hr = m_pRenderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
    {
        this->ReleaseDeviceResources();
        return false;
    }
    
    return SUCCEEDED(hr);
}

void Timeline_Direct2DRenderer_Native::Clear(const D2D1_COLOR_F& color)
{
    if (m_pRenderTarget)
    {
        m_pRenderTarget->Clear(color);
    }
}

void Timeline_Direct2DRenderer_Native::Clear(float r, float g, float b, float a)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    this->Clear(color);
}

ID2D1SolidColorBrush* Timeline_Direct2DRenderer_Native::GetCachedBrush(const D2D1_COLOR_F& color)
{
    // Check cache first
    for (const auto& cached : m_brushCache)
    {
        if (memcmp(&cached.color, &color, sizeof(D2D1_COLOR_F)) == 0)
            return cached.brush;
    }

    // Create new brush
    ID2D1SolidColorBrush* brush = nullptr;
    HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(color, &brush);

    if (FAILED(hr))
        return nullptr;

    // Add to cache
    CachedBrush newCached;
    newCached.color = color;
    newCached.brush = brush;
    m_brushCache.push_back(newCached);

    return brush;
}

bool Timeline_Direct2DRenderer_Native::CreateStrokeStyles()
{
    // Create solid stroke style
    D2D1_STROKE_STYLE_PROPERTIES solidProps = D2D1::StrokeStyleProperties(
        D2D1_CAP_STYLE_FLAT,
        D2D1_CAP_STYLE_FLAT,
        D2D1_CAP_STYLE_FLAT,
        D2D1_LINE_JOIN_MITER,
        10.0f,
        D2D1_DASH_STYLE_SOLID,
        0.0f
    );

    HRESULT hr = m_pD2DFactory->CreateStrokeStyle(
        solidProps,
        nullptr,
        0,
        &m_pSolidStroke
    );

    return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::CreateTextFormats()
{
    if (!m_pDWriteFactory)
        return false;

    // Create measure number format
    HRESULT hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &m_pMeasureNumberFormat);

    if (FAILED(hr))
        return false;

    // Set text alignment
    m_pMeasureNumberFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pMeasureNumberFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    

    // Create test marker format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &m_pMarkerTextFormat);

    if (FAILED(hr))
        return false;

    // Set text alignment
    m_pMarkerTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pMarkerTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create time signature format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &m_pTimeSignatureFormat);

    if (FAILED(hr))
        return false;

    // Set text alignment
    m_pTimeSignatureFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pTimeSignatureFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create quarter note format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 8.0f, L"en-us", &m_pQuarterNoteFormat);

    if (FAILED(hr))
        return false;

    // Set text alignment
    m_pQuarterNoteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pQuarterNoteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create quarter note format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &m_pTrackHeaderFormat);

    if (FAILED(hr))
        return false;

    // Set text alignment
    m_pTrackHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_pTrackHeaderFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create quarter note format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &m_pTablatureFormat);

    if (FAILED(hr))
        return false;

    // Set text alignment
    m_pTablatureFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pTablatureFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return true;
}

bool Timeline_Direct2DRenderer_Native::CreateBitmapFromImage(System::Drawing::Image^ image, ID2D1Bitmap** ppBitmap)
{
    if (!m_pRenderTarget || !image || !ppBitmap) {
        return false;
    }

    // Create temporary GDI+ bitmap to access pixels
    System::Drawing::Bitmap^ gdiBitmap = gcnew System::Drawing::Bitmap(image);
    System::Drawing::Rectangle rect(0, 0, gdiBitmap->Width, gdiBitmap->Height);
    System::Drawing::Imaging::BitmapData^ bitmapData = gdiBitmap->LockBits(rect, System::Drawing::Imaging::ImageLockMode::ReadOnly, System::Drawing::Imaging::PixelFormat::Format32bppPArgb);

    // Create D2D bitmap properties
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    // Create the bitmap
    HRESULT hr = m_pRenderTarget->CreateBitmap(
        D2D1::SizeU(gdiBitmap->Width, gdiBitmap->Height),
        bitmapData->Scan0.ToPointer(),
        bitmapData->Stride,
        props,
        ppBitmap
    );

    gdiBitmap->UnlockBits(bitmapData);
    delete gdiBitmap;

    return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::DrawBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& destRect, float opacity)
{
    if (!m_pRenderTarget || !bitmap) {
        return false;
    }

    m_pRenderTarget->DrawBitmap(
        bitmap,
        destRect,
        opacity,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
    );

    return true;
}

bool Timeline_Direct2DRenderer_Native::CreateAndCacheBitmap(const std::wstring& key, System::Drawing::Image^ image)
{
    ID2D1Bitmap* Bitmap = nullptr;
    if (!CreateBitmapFromImage(image, &Bitmap)) {
        return false;
    }

    m_bitmapCache.push_back({ key, Bitmap });

    return true;
}

ID2D1Bitmap* Timeline_Direct2DRenderer_Native::GetCachedBitmap(const std::wstring& key)
{
    auto it = std::find_if(m_bitmapCache.begin(), m_bitmapCache.end(),
        [&key](const CachedBitmap& cached) { return cached.key == key; });
    return it != m_bitmapCache.end() ? it->bitmap : nullptr;
}

bool Timeline_Direct2DRenderer_Native::DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget)
        return false;

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    /*
    // Something is wrong with 'm_pSolidStroke'
    if (m_pSolidStroke) {
        m_pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth, m_pSolidStroke);
    }
    else {
        m_pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth);
    }
    */

    m_pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth);

    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float strokeWidth)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);

    return DrawLine(x1, y1, x2, y2, color, strokeWidth);
}

bool Timeline_Direct2DRenderer_Native::DrawLines(const D2D1_POINT_2F* points, UINT32 pointCount, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget || !points || pointCount < 2) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    for (UINT32 i = 0; i < pointCount - 1; i++) {
        m_pRenderTarget->DrawLine(points[i], points[i + 1], brush, strokeWidth);
    }

    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    // There is an issue with 'm_pSolidStroke'
    m_pRenderTarget->DrawRectangle(rect, brush, strokeWidth);

    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);

    return DrawRectangle(rect, color, strokeWidth);
}

bool Timeline_Direct2DRenderer_Native::DrawRectangleDashed(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float dashLength, float gapLength)
{
    if (!m_pRenderTarget) {
        return false;
    }

    // Get brush
    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    // Top line
    float x = rect.left;
    while (x < rect.right) {
        float segmentEnd = min(x + dashLength, rect.right);
        m_pRenderTarget->DrawLine(D2D1::Point2F(x, rect.top),
            D2D1::Point2F(segmentEnd, rect.top),
            brush, strokeWidth);
        x += dashLength + gapLength;
    }

    // Right line
    float y = rect.top;
    while (y < rect.bottom) {
        float segmentEnd = min(y + dashLength, rect.bottom);
        m_pRenderTarget->DrawLine(D2D1::Point2F(rect.right, y),
            D2D1::Point2F(rect.right, segmentEnd),
            brush, strokeWidth);
        y += dashLength + gapLength;
    }

    // Bottom line
    x = rect.left;
    while (x < rect.right) {
        float segmentEnd = min(x + dashLength, rect.right);
        m_pRenderTarget->DrawLine(D2D1::Point2F(x, rect.bottom),
            D2D1::Point2F(segmentEnd, rect.bottom),
            brush, strokeWidth);
        x += dashLength + gapLength;
    }

    // Left line
    y = rect.top;
    while (y < rect.bottom) {
        float segmentEnd = min(y + dashLength, rect.bottom);
        m_pRenderTarget->DrawLine(D2D1::Point2F(rect.left, y),
            D2D1::Point2F(rect.left, segmentEnd),
            brush, strokeWidth);
        y += dashLength + gapLength;
    }

    return true;
}

bool Timeline_Direct2DRenderer_Native::FillRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillRectangle(rect, brush);
    return true;
}

bool Timeline_Direct2DRenderer_Native::FillRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);

    return FillRectangle(rect, color);
}

bool Timeline_Direct2DRenderer_Native::FillRectangleGradient2(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorLeft, const D2D1_COLOR_F& colorRight)
{
	D2D1_GRADIENT_STOP gradientStops[2];
	gradientStops[0].color = colorLeft;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = colorRight;
	gradientStops[1].position = 1.0f;

	return FillRectangleGradient(rect, gradientStops, 2);
}

bool Timeline_Direct2DRenderer_Native::FillRectangleGradient3(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorLeft, const D2D1_COLOR_F& colorCenter, const D2D1_COLOR_F& colorRight)
{
	D2D1_GRADIENT_STOP gradientStops[3];
	gradientStops[0].color = colorLeft;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = colorCenter;
	gradientStops[1].position = 0.5f;
	gradientStops[2].color = colorRight;
	gradientStops[2].position = 1.0f;

	return FillRectangleGradient(rect, gradientStops, 3);
}

bool Timeline_Direct2DRenderer_Native::FillRectangleGradient(const D2D1_RECT_F& rect, const D2D1_GRADIENT_STOP* gradientStops, UINT32 count)
{
	if (!m_pRenderTarget || gradientStops == NULL) {
		return false;
	}
	
	ID2D1GradientStopCollection* pGradientStops = NULL;
	
	HRESULT hr = m_pRenderTarget->CreateGradientStopCollection(
		gradientStops,
		count,
		D2D1_GAMMA_2_2,
		D2D1_EXTEND_MODE_CLAMP,
		&pGradientStops
	);

	if (FAILED(hr)) {
		return false;
	}

	hr = m_pRenderTarget->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(
			D2D1::Point2F(rect.left,  (rect.bottom - rect.left) / 2),
			D2D1::Point2F(rect.right, (rect.bottom - rect.left) / 2)),
		pGradientStops,
		&m_pLinearGradientBrush
	);

	if (FAILED(hr)) {
		return false;
	}

	m_pRenderTarget->FillRectangle(rect, m_pLinearGradientBrush);
	
	return true;
}

bool Timeline_Direct2DRenderer_Native::DrawRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }
    
    m_pRenderTarget->DrawRoundedRectangle(rect, brush, strokeWidth);
    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_ROUNDED_RECT rectRound = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), 2, 2); // Radius to be updated here

    return DrawRoundedRectangle(rectRound, color, strokeWidth);
}

bool Timeline_Direct2DRenderer_Native::FillRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillRoundedRectangle(rect, brush);
    return true;
}

bool Timeline_Direct2DRenderer_Native::FillRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_ROUNDED_RECT rectRound = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), 2, 2); // Radius to be updated here

    return FillRoundedRectangle(rectRound, color);
}

bool Timeline_Direct2DRenderer_Native::DrawEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->DrawEllipse(ellipse, brush, strokeWidth);
    return true;
}

bool Timeline_Direct2DRenderer_Native::FillEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillEllipse(ellipse, brush);
    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawPolygon(const D2D1_POINT_2F* points, UINT32 pointCount, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget || !points || pointCount < 3) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    // Draw lines connecting each point, including closing the polygon
    for (UINT32 i = 0; i < pointCount; i++)
    {
        const D2D1_POINT_2F& p1 = points[i];
        const D2D1_POINT_2F& p2 = points[(i + 1) % pointCount]; // Wrap around to first point
        m_pRenderTarget->DrawLine(p1, p2, brush, strokeWidth);
    }

    return true;
}

bool Timeline_Direct2DRenderer_Native::FillDiamond(const D2D1_POINT_2F* points, UINT32 pointCount, const D2D1_COLOR_F& color)
{
    if (!m_pRenderTarget || !points || pointCount != 4) {  // For diamonds only
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    // For a diamond, points typically are:
    // points[0] = top
    // points[1] = right
    // points[2] = bottom
    // points[3] = left

    // Calculate the center point
    D2D1_POINT_2F center = {
        (points[0].x + points[2].x) / 2,  // Center X
        (points[0].y + points[2].y) / 2   // Center Y
    };

    // Calculate size (assuming uniform diamond)
    float width = points[1].x - points[3].x;
    float height = points[2].y - points[0].y;

    // Save current transform
    D2D1_MATRIX_3X2_F oldTransform;
    m_pRenderTarget->GetTransform(&oldTransform);

    // Create transform matrix for 45-degree rotation around center
    D2D1_MATRIX_3X2_F rotationMatrix = D2D1::Matrix3x2F::Rotation(45.0f, center);
    m_pRenderTarget->SetTransform(rotationMatrix);

    // Draw rotated rectangle
    D2D1_RECT_F rect = D2D1::RectF(
        center.x - width / 2.8284f,  // Adjust for 45-degree rotation (1/√2 ≈ 1/1.4142)
        center.y - height / 2.8284f,
        center.x + width / 2.8284f,
        center.y + height / 2.8284f
    );

    m_pRenderTarget->FillRectangle(rect, brush);

    // Restore original transform
    m_pRenderTarget->SetTransform(oldTransform);

    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawTieLine(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2, const D2D1_POINT_2F& point3, const D2D1_POINT_2F& point4, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget || !m_pD2DFactory) {
        return false;
    }

    // Get brush for the color
    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    // Use a fixed number of segments for reliability
    const int numSegments = 10;
    D2D1_POINT_2F prevPoint = point1;

    for (int i = 1; i <= numSegments; i++)
    {
        // Calculate t for this segment (0 to 1)
        float t = i / (float)numSegments;
        float u = 1.0f - t;

        // Calculate powers once
        float u2 = u * u;
        float u3 = u2 * u;
        float t2 = t * t;
        float t3 = t2 * t;

        // Cubic Bezier formula
        D2D1_POINT_2F currentPoint;
        currentPoint.x = u3 * point1.x + 3.0f * u2 * t * point2.x + 3.0f * u * t2 * point3.x + t3 * point4.x;
        currentPoint.y = u3 * point1.y + 3.0f * u2 * t * point2.y + 3.0f * u * t2 * point3.y + t3 * point4.y;

        m_pRenderTarget->DrawLine(prevPoint, currentPoint, brush, strokeWidth);
        prevPoint = currentPoint;
    }

    return true;

    // The main bezier draw geometry function does not work for some reason. I have no clue why. May investiagte at a later point.
    /*
    // Create path geometry for the bezier curve
    ID2D1PathGeometry* pathGeometry = nullptr;
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&pathGeometry);

    if (FAILED(hr) || !pathGeometry) {
        return false;
    }

    // Create geometry sink
    ID2D1GeometrySink* sink = nullptr;
    hr = pathGeometry->Open(&sink);

    if (FAILED(hr) || !sink) {
        pathGeometry->Release();
        return false;
    }

    // Basic geometry construction - absolute minimal steps
    sink->BeginFigure(point1, D2D1_FIGURE_BEGIN_FILLED);

    // Try just a single bezier segment
    sink->AddBezier(
        D2D1::BezierSegment(
            point2,
            point3,
            point4
        )
    );

    sink->EndFigure(D2D1_FIGURE_END_OPEN);

    // Must close the sink before drawing
    hr = sink->Close();
    sink->Release();

    // Try to stream the geometry for debug
    if (SUCCEEDED(hr)) {
        // Draw with a much thicker stroke for testing
        const float testStrokeWidth = 5.0f;  // Make it very visible

        // Try both filled and stroked
        //m_pRenderTarget->FillGeometry(pathGeometry, brush);
        m_pRenderTarget->DrawGeometry(pathGeometry, brush, testStrokeWidth);
        m_pRenderTarget->Flush();
    }

    pathGeometry->Release();
    pathGeometry = nullptr;
    
    return SUCCEEDED(hr);
    */
}

bool Timeline_Direct2DRenderer_Native::DrawPath(ID2D1Geometry* geometry, const D2D1_COLOR_F& color, float strokeWidth)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->DrawGeometry(geometry, brush, strokeWidth);
    return true;
}

bool Timeline_Direct2DRenderer_Native::FillPath(ID2D1Geometry* geometry, const D2D1_COLOR_F& color)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillGeometry(geometry, brush);
    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawText(const std::wstring& text, const D2D1_RECT_F& layoutRect, const D2D1_COLOR_F& color, IDWriteTextFormat* textFormat, D2D1_DRAW_TEXT_OPTIONS options)
{
    if (!m_pRenderTarget || !textFormat) {
        return false;
    }

    textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->DrawText(text.c_str(), static_cast<UINT32>(text.length()), textFormat, layoutRect, brush, options);

    return true;
}

float Timeline_Direct2DRenderer_Native::MeasureTextWidth(const std::wstring& text, IDWriteTextFormat* textFormat)
{
    if (!m_pDWriteFactory || !textFormat || text.empty()) {
        return 0.0f;
    }

    // Create a text layout
    IDWriteTextLayout* pTextLayout = nullptr;
    HRESULT hr = m_pDWriteFactory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        textFormat,
        10000.0f, // Set a large max width to get full text width
        100.0f,   // Height doesn't matter for width measurement
        &pTextLayout
    );

    if (FAILED(hr) || !pTextLayout) {
        return 0.0f;
    }

    // Get the metrics
    DWRITE_TEXT_METRICS metrics = {};
    hr = pTextLayout->GetMetrics(&metrics);

    // Release the text layout
    pTextLayout->Release();

    if (FAILED(hr)) {
        return 0.0f;
    }

    return metrics.widthIncludingTrailingWhitespace;
}

bool Timeline_Direct2DRenderer_Native::PreloadBitmap(const std::wstring& key, System::Drawing::Image^ image)
{
    return CreateAndCacheBitmap(key, image);
}

bool Timeline_Direct2DRenderer_Native::DrawCachedBitmap(const std::wstring& key, const D2D1_RECT_F& destRect, float opacity)
{
    ID2D1Bitmap* Bitmap = GetCachedBitmap(key);
    return Bitmap ? DrawBitmap(Bitmap, destRect, opacity) : false;
}

bool Timeline_Direct2DRenderer_Native::PushLayer(const D2D1_LAYER_PARAMETERS& layerParameters, ID2D1Layer* layer)
{
    if (!m_pRenderTarget || !layer) {
        return false;
    }

    m_pRenderTarget->PushLayer(layerParameters, layer);
    return true;
}

void Timeline_Direct2DRenderer_Native::PopLayer()
{
    if (m_pRenderTarget) {
        m_pRenderTarget->PopLayer();
    }
}

void Timeline_Direct2DRenderer_Native::PushAxisAlignedClip(const D2D1_RECT_F& clipRect, D2D1_ANTIALIAS_MODE antialiasMode)
{
    if (m_pRenderTarget) {
        m_pRenderTarget->PushAxisAlignedClip(clipRect, antialiasMode);
    }
}

void Timeline_Direct2DRenderer_Native::PopAxisAlignedClip()
{
    if (m_pRenderTarget) {
        m_pRenderTarget->PopAxisAlignedClip();
    }
}

void Timeline_Direct2DRenderer_Native::SetTransform(const D2D1_MATRIX_3X2_F& transform)
{
    if (m_pRenderTarget) {
        m_pRenderTarget->SetTransform(transform);
    }
}

void Timeline_Direct2DRenderer_Native::GetTransform(D2D1_MATRIX_3X2_F* transform)
{
    if (m_pRenderTarget && transform) {
        m_pRenderTarget->GetTransform(transform);
    }
}

bool Timeline_Direct2DRenderer_Native::CreateLayer(ID2D1Layer** layer)
{
    if (!m_pRenderTarget || !layer) {
        return false;
    }

    HRESULT hr = m_pRenderTarget->CreateLayer(nullptr, layer);
    return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::CreatePathGeometry(ID2D1PathGeometry** geometry)
{
    if (!m_pD2DFactory || !geometry)
        return false;

    HRESULT hr = m_pD2DFactory->CreatePathGeometry(geometry);
    return SUCCEEDED(hr);
}
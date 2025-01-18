#include "Timeline_Direct2DRenderer_Native.h"
#include <algorithm>

Timeline_Direct2DRenderer_Native::Timeline_Direct2DRenderer_Native():
    m_hwnd(nullptr),
    m_pD2DFactory(nullptr),
    m_pRenderTarget(nullptr),
    m_pDashedStroke(nullptr),
    m_pSolidStroke(nullptr),
    m_pDWriteFactory(nullptr),
    m_pMeasureNumberFormat(nullptr),
    m_pMarkerTextFormat(nullptr),
    m_pTimeSignatureFormat(nullptr),
    m_resourcesValid(false)
{
}

Timeline_Direct2DRenderer_Native::~Timeline_Direct2DRenderer_Native()
{
    Cleanup();
}

bool Timeline_Direct2DRenderer_Native::Initialize(HWND hwnd)
{
    if (!hwnd)
        return false;

    m_hwnd = hwnd;

    // Create D2D factory
    D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
    //options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, reinterpret_cast<void**>(&m_pD2DFactory));

    if (FAILED(hr))
        return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));

    if (FAILED(hr))
        return false;

    // Create text formats
    if (!CreateTextFormats())
        return false;

    // Create stroke styles
    if (!CreateStrokeStyles())
        return false;

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

    // Release DirectWrite factory
    if (m_pDWriteFactory)
    {
        m_pDWriteFactory->Release();
        m_pDWriteFactory = nullptr;
    }

    // Release stroke styles
    if (m_pDashedStroke)
    {
        m_pDashedStroke->Release();
        m_pDashedStroke = nullptr;
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

    if (FAILED(hr))
        return false;

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
    // Create dashed stroke style
    float dashes[] = { 5.0f, 3.0f };
    D2D1_STROKE_STYLE_PROPERTIES dashedProps = D2D1::StrokeStyleProperties(
        D2D1_CAP_STYLE_FLAT,
        D2D1_CAP_STYLE_FLAT,
        D2D1_CAP_STYLE_FLAT,
        D2D1_LINE_JOIN_MITER,
        10.0f,
        D2D1_DASH_STYLE_CUSTOM,
        0.0f
    );

    HRESULT hr = m_pD2DFactory->CreateStrokeStyle(
        dashedProps,
        dashes,
        ARRAYSIZE(dashes),
        &m_pDashedStroke
    );

    if (FAILED(hr))
        return false;

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

    hr = m_pD2DFactory->CreateStrokeStyle(
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

    return true;
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

bool Timeline_Direct2DRenderer_Native::DrawLineDashed(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth, const float* dashes, UINT32 dashCount)
{
    if (!m_pRenderTarget || !m_pD2DFactory)
        return false;

    // Create a custom stroke style for these dashes
    ID2D1StrokeStyle* customDashedStroke = nullptr;
    D2D1_STROKE_STYLE_PROPERTIES dashedProps = D2D1::StrokeStyleProperties(
        D2D1_CAP_STYLE_FLAT,
        D2D1_CAP_STYLE_FLAT,
        D2D1_CAP_STYLE_FLAT,
        D2D1_LINE_JOIN_MITER,
        10.0f,
        D2D1_DASH_STYLE_CUSTOM,
        0.0f
    );

    HRESULT hr = m_pD2DFactory->CreateStrokeStyle(
        dashedProps,
        dashes,
        dashCount,
        &customDashedStroke
    );

    if (FAILED(hr))
        return false;

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        customDashedStroke->Release();
        return false;
    }

    m_pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth, customDashedStroke);

    customDashedStroke->Release();
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

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->DrawText(text.c_str(), static_cast<UINT32>(text.length()), textFormat, layoutRect, brush, options);

    return true;
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

/*
bool Timeline_Direct2DRenderer_Native::DrawGridLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, bool isDashed)
{
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush)
        return false;

    m_pRenderTarget->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        brush,
        1.0f,
        isDashed ? m_pDashedStroke : m_pSolidStroke
    );

    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawMeasureLine(float x, float y1, float y2, const D2D1_COLOR_F& color)
{
    // Measure lines are always solid and slightly thicker
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush)
        return false;

    m_pRenderTarget->DrawLine(
        D2D1::Point2F(x, y1),
        D2D1::Point2F(x, y2),
        brush,
        2.0f,
        m_pSolidStroke
    );

    return true;
}








bool Timeline_Direct2DRenderer_Native::DrawGridLineRaw(float x1, float y1, float x2, float y2, float r, float g, float b, float a, bool isDashed)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);

    return DrawGridLine(x1, y1, x2, y2, color, isDashed);
}

bool Timeline_Direct2DRenderer_Native::DrawMeasureLineRaw(float x, float y1, float y2, float r, float g, float b, float a)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);

    return DrawMeasureLine(x, y1, y2, color);
}

bool Timeline_Direct2DRenderer_Native::DrawMeasureNumbers(float width, float headerHeight, const std::vector<MeasureInfo>& measures, const D2D1_COLOR_F& headerBackground, const D2D1_COLOR_F& textColor, float scrollX, float zoomLevel)
{
    if (!m_pRenderTarget) {
        return false;
    }

    // Draw header background
    D2D1_RECT_F headerRect = D2D1::RectF(0, 0, width, headerHeight);
    m_pRenderTarget->FillRectangle(headerRect, GetCachedBrush(headerBackground));

    // Constants for vertical positioning
    const float markerTextY = 2;
    const float timeSignatureY = markerTextY + 14 + 2;
    const float measureNumberY = timeSignatureY + 14 + 4;

    // Get text brush
    ID2D1SolidColorBrush* textBrush = GetCachedBrush(textColor);

    for (const auto& measure : measures)
    {
        // Convert tick position to pixels with zoom
        float x = (measure.startTick * 16.0f / 960.0f) * zoomLevel + scrollX + 150; // 150 is TRACK_HEADER_WIDTH

        if (x >= 150 && x <= width)
        {
            // Draw marker text if present
            if (!measure.markerText.empty())
            {
                D2D1_RECT_F markerRect = D2D1::RectF(
                    x - 50, markerTextY,
                    x + 50, markerTextY + 14
                );
                m_pRenderTarget->DrawText(
                    measure.markerText.c_str(),
                    static_cast<UINT32>(measure.markerText.length()),
                    m_pMarkerTextFormat,
                    markerRect,
                    textBrush
                );
            }

            // Draw measure number
            wchar_t numText[16];
            swprintf_s(numText, L"%d", measure.number);
            D2D1_RECT_F numberRect = D2D1::RectF(
                x - 25, measureNumberY,
                x + 25, measureNumberY + 14
            );
            m_pRenderTarget->DrawText(
                numText,
                static_cast<UINT32>(wcslen(numText)),
                m_pMeasureNumberFormat,
                numberRect,
                textBrush
            );

            // Draw time signature
            wchar_t timeSignature[16];
            swprintf_s(timeSignature, L"%d/%d", measure.numerator, measure.denominator);
            D2D1_RECT_F sigRect = D2D1::RectF(
                x - 25, timeSignatureY,
                x + 25, timeSignatureY + 14
            );
            m_pRenderTarget->DrawText(
                timeSignature,
                static_cast<UINT32>(wcslen(timeSignature)),
                m_pTimeSignatureFormat,
                sigRect,
                textBrush
            );
        }
    }

    return true;
}
*/
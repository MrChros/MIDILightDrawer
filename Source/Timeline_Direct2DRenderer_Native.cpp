#include "Timeline_Direct2DRenderer_Native.h"


Timeline_Direct2DRenderer_Native::Timeline_Direct2DRenderer_Native():
    m_hwnd(nullptr),
    m_pD2DFactory(nullptr),
    m_pRenderTarget(nullptr),
    m_pSolidStroke(nullptr),
	m_pDashedStroke(nullptr),
	m_pLinearGradientBrush(nullptr),
    m_pDWriteFactory(nullptr),
    m_pMeasureNumberFormat(nullptr),
    m_pMarkerTextFormat(nullptr),
    m_pTimeSignatureFormat(nullptr),
    m_pQuarterNoteFormat(nullptr),
    m_pTrackHeaderFormat(nullptr),
    m_pTablatureFormat(nullptr),
    m_pFPSCounterFormat(nullptr),
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
    for (auto& cached : m_BrushCache)
    {
        if (cached.Brush) {
            cached.Brush->Release();
		}
    }
    m_BrushCache.clear();

    // Release bitmap cache
    for (auto& cached : m_BitmapCache) {
        if (cached.Bitmap) {
            cached.Bitmap->Release();
        }
    }
    m_BitmapCache.clear();

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

    if (m_resourcesValid) {
        return true;
	}

    // Get window size
    RECT rc;
    GetClientRect(m_hwnd, &rc);


    // Create render target with optimized properties
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE,  // Use hardware rendering
        D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED
        ),
        96.0f,  // Default DPI
        96.0f,
        D2D1_RENDER_TARGET_USAGE_NONE
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

    m_resourcesValid = true;

    return true;
}

void Timeline_Direct2DRenderer_Native::ReleaseDeviceResources()
{
    // Release brushes
    for (auto& cached : m_BrushCache)
    {
        if (cached.Brush)
            cached.Brush->Release();
    }
    m_BrushCache.clear();

    // Release render target
    if (m_pRenderTarget)
    {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }

    m_resourcesValid = false;
}

bool Timeline_Direct2DRenderer_Native::PreloadTabText(float fontSize, const D2D1_COLOR_F& textColor, const D2D1_COLOR_F& bgColor)
{
	if (!m_pRenderTarget || !m_resourcesValid) {
		return false;
	}

	float TabTextFontSize = fontSize * 1.0f;

	IDWriteTextFormat* TextFormat = UpdateTablatureFormat(TabTextFontSize);

	TabTextCacheEntry Entry;
	Entry.FontSize = fontSize;

    // Guitar Frets
    for (int TabText = 0; TabText <= 24; TabText++)
    {
        ID2D1Bitmap* Bitmap = nullptr;
        std::wstring TextStr = std::to_wstring(TabText);

        CreateTabTextBitmap(TextStr, TabTextFontSize, &Bitmap, textColor, bgColor);

        Entry.TabTexts[TabText] = Bitmap;
        Entry.TextWidths[TabText] = MeasureTextWidth(TextStr, TextFormat);
    }

    // Drum Notes
	for (int TabText = 30; TabText <= 60; TabText++)
	{
		ID2D1Bitmap* Bitmap = nullptr;
		std::wstring TextStr = std::to_wstring(TabText);

		CreateTabTextBitmap(TextStr, TabTextFontSize, &Bitmap, textColor, bgColor);
		
		Entry.TabTexts[TabText]		= Bitmap;
		Entry.TextWidths[TabText]	= MeasureTextWidth(TextStr, TextFormat);
	}

	m_TabTextCache.push_back(Entry);

	return true;
}

bool Timeline_Direct2DRenderer_Native::PreloadDrumSymbol(float stringSpace, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor)
{
	if (!m_pRenderTarget || !m_resourcesValid) {
		return false;
	}
	
	float SymbolSize = stringSpace * 1.0f;

	DrumSymbolCacheEntry Entry;
	Entry.StringSpace = stringSpace;

	for (int SymbolType = 0; SymbolType < 7; SymbolType++)
	{
		ID2D1Bitmap* Bitmap = nullptr;
		CreateDrumSymbolBitmap(SymbolType, SymbolSize, &Bitmap, symbolColor, bgColor);
		
		Entry.DrumSymbols[SymbolType] = Bitmap;
	}

	m_DrumSymbolCache.push_back(Entry);

	return true;
}

bool Timeline_Direct2DRenderer_Native::PreloadDurationSymbols(float zoomLevel, float logScale, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor)
{
	if (!m_pRenderTarget || !m_resourcesValid) {
		return false;
	}

	DurationSymbolCacheEntry entry;
	entry.ZoomLevel = zoomLevel;

	
	// Create complete symbol for each possible duration
	for (int Duration : POSSIBLE_DURATIONS)
	{
		ID2D1Bitmap* Bitmap = nullptr;
		if (!CreateDurationSymbolBitmap(Duration, zoomLevel, logScale, &Bitmap, symbolColor, bgColor)) {
			return false;
		}
		entry.DurationSymbols[Duration] = Bitmap;
	}

	m_DurationSymbolCache.push_back(entry);

	return true;
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::GetMeasureNumberFormat()
{ 
    return m_pMeasureNumberFormat;
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::GetMarkerTextFormat()    
{
    return m_pMarkerTextFormat;
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::GetTimeSignatureFormat() 
{
    return m_pTimeSignatureFormat; 
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::GetQuarterNoteFormat() 
{
    return m_pQuarterNoteFormat; 
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::GetTrackHeaderFormat() 
{
    return m_pTrackHeaderFormat; 
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::GetFPSCounterFormat()   
{
    return m_pFPSCounterFormat;
}

IDWriteTextFormat* Timeline_Direct2DRenderer_Native::UpdateTablatureFormat(float fontSize)
{
    if (!m_pDWriteFactory) {
        return nullptr;
	}

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
        fontSize,              // Font size
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

void Timeline_Direct2DRenderer_Native::BeginBatch()
{
#if USE_BATCH_RENDERING
    m_CommandBatch.Begin();
    m_ZOrderCounter = 0.0f; // Reset z-order counter at the start of each batch
#endif
}

void Timeline_Direct2DRenderer_Native::ExecuteBatch()
{
#if USE_BATCH_RENDERING
    if (m_pRenderTarget) {
        // Create brush cache map with the correct comparator type
        std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual> brushCache;

        // Convert our existing brush cache to the format needed by CommandBatch
        for (const auto& cached : m_BrushCache) {
            brushCache[cached.Color] = cached.Brush;
        }

        m_CommandBatch.Execute(m_pRenderTarget, brushCache, false);
    }
#endif
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

	if (FAILED(hr)) {
		return false;
	}

	// Create dashed stroke style
	D2D1_STROKE_STYLE_PROPERTIES dashedProps = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_FLAT,
		D2D1_CAP_STYLE_FLAT,
		D2D1_CAP_STYLE_FLAT,
		D2D1_LINE_JOIN_MITER,
		10.0f,
		D2D1_DASH_STYLE_CUSTOM,
		0.0f
	);

	// Define dash pattern
	float dashes[] = { 1.0f, 1.0f };  // These values will be multiplied by stroke width

	hr = m_pD2DFactory->CreateStrokeStyle(
		dashedProps,
		dashes,
		ARRAYSIZE(dashes),
		&m_pDashedStroke
	);

	return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::CreateTabTextBitmap(const std::wstring& text, float fontSize, ID2D1Bitmap** ppBitmap, const D2D1_COLOR_F& textColor, const D2D1_COLOR_F& bgColor)
{
	if (!m_pRenderTarget || !m_pDWriteFactory) {
		return false;
	}

	// Create temporary text format
	IDWriteTextFormat* TextFormat = nullptr;
	HRESULT hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-us", &TextFormat);

	if (FAILED(hr)) {
		return false;
	}

	// Measure text
	IDWriteTextLayout* TextLayout = nullptr;
	hr = m_pDWriteFactory->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.length()), TextFormat, 1000.0f, 100.0f, &TextLayout);

	if (FAILED(hr)) {
		TextFormat->Release();
		return false;
	}

	DWRITE_TEXT_METRICS Metrics;
	TextLayout->GetMetrics(&Metrics);

	// Create bitmap render target
	ID2D1BitmapRenderTarget* BitmapRT = nullptr;
	hr = m_pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(Metrics.width + 2, Metrics.height + 2), &BitmapRT); // Add padding

	if (SUCCEEDED(hr))
	{
		// Draw text to bitmap
		BitmapRT->BeginDraw();
		BitmapRT->Clear(bgColor);

		ID2D1SolidColorBrush* Brush = nullptr;
		BitmapRT->CreateSolidColorBrush(textColor, &Brush);

		if (Brush) {
			BitmapRT->DrawTextLayout(D2D1::Point2F(1, 1), TextLayout, Brush);	// Add 1px padding
			Brush->Release();
		}

		BitmapRT->EndDraw();

		// Get bitmap from render target
		ID2D1Bitmap* Bitmap = nullptr;
		BitmapRT->GetBitmap(&Bitmap);
		*ppBitmap = Bitmap;

		BitmapRT->Release();
	}

	TextLayout->Release();
	TextFormat->Release();

	return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::CreateDrumSymbolBitmap(int symbolType, float size, ID2D1Bitmap** ppBitmap, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor)
{
	if (!m_pRenderTarget) {
		return false;
	}

	float BitmapSize = size * 4.0f;  // Increase padding to 4x the symbol size

	// Create the geometry for the drum symbol
	ID2D1Geometry* SymbolGeometry = CreateDrumSymbolGeometry(symbolType, size, BitmapSize);
	if (!SymbolGeometry) {
		return false;
	}

	// Create bitmap render target with more padding
	ID2D1BitmapRenderTarget* BitmapRT = nullptr;

	HRESULT hr = m_pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(BitmapSize, BitmapSize), &BitmapRT);

	if (SUCCEEDED(hr))
	{
		BitmapRT->BeginDraw();
		BitmapRT->Clear(bgColor);

		ID2D1SolidColorBrush* Brush = nullptr;
		BitmapRT->CreateSolidColorBrush(symbolColor, &Brush);

		if (Brush)
		{
			// Draw the geometry
			if (SymbolGeometry)
			{
				// For filled symbols
				if (symbolType == 0 || symbolType == 5 || symbolType == 6)
				{
					BitmapRT->FillGeometry(SymbolGeometry, Brush);
				}
				// For hollow/outline symbols
				else
				{
					BitmapRT->DrawGeometry(SymbolGeometry, Brush, size * 0.2f);  // Line width proportional to size
				}
			}

			Brush->Release();
		}

		BitmapRT->EndDraw();

		// Get bitmap from render target
		ID2D1Bitmap* Bitmap = nullptr;
		BitmapRT->GetBitmap(&Bitmap);
		*ppBitmap = Bitmap;

		BitmapRT->Release();
	}

	// Clean up the geometry
	if (SymbolGeometry) {
		SymbolGeometry->Release();
	}

	return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::CreateDurationSymbolBitmap(int duration, float zoomLevel, float logScale, ID2D1Bitmap** ppBitmap, const D2D1_COLOR_F& symbolColor, const D2D1_COLOR_F& bgColor)
{
    if (!m_pRenderTarget || !ppBitmap) {
        return false;
    }

    float ScaledStemLength      = std::min(DURATION_SYMBOL_BASE_STEM_LENGTH + (logScale * 6.0f), 35.0f);
    float ScaledLineLength      = std::min(DURATION_SYMBOL_BASE_LINE_LENGTH + (logScale * 2.0f), 15.0f);
    float ScaledLineSpacing     = std::min(DURATION_SYMBOL_BASE_LINE_SPACING + (logScale * 1.0f), 6.0f);
    float ScaledLineThickness   = std::min(1.0f + (logScale * 0.2f), 2.0f);
    float ScaledStemOffset      = std::min(DURATION_SYMBOL_BASE_STEM_OFFSET + (logScale * 2.0f), 20.0f);

    // Create a bitmap render target sized to accommodate the complete symbol
    float SymbolWidth   = ScaledLineLength * 3.0f;                      // Width to accommodate stem and flags
    float SymbolHeight  = ScaledStemOffset * 2.0f + ScaledStemLength;   // Height to accommodate full stem and triplet number
    float SymbolXCenter = SymbolWidth / 2;

    ID2D1BitmapRenderTarget* symbolRT = nullptr;
    HRESULT hr = m_pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(SymbolWidth, SymbolHeight), &symbolRT);

    if (FAILED(hr) || !symbolRT) {
        return false;
    }

    symbolRT->BeginDraw();
    symbolRT->Clear(bgColor);

    ID2D1SolidColorBrush* Brush = nullptr;
    hr = symbolRT->CreateSolidColorBrush(symbolColor, &Brush);

    if (SUCCEEDED(hr) && Brush)
    {
        // Position stem below the bottom string with scaled offset
        float StemStartY = ScaledStemOffset;
        float StemEndY   = StemStartY + ScaledStemLength;

        // Calculate stem position - centered below the note
        float StemX = SymbolXCenter;
        
        int NumLines = 0;
        
        if ((duration >= TICKS_PER_QUARTER * 2) && (duration < TICKS_PER_QUARTER * 4)) // Half note
        {
            float HalfStemLength = ScaledStemLength / 2;
            symbolRT->DrawLine(D2D1::Point2F(StemX, StemStartY + HalfStemLength), D2D1::Point2F(StemX, StemEndY), Brush, ScaledLineThickness);
        }
        else if(duration < TICKS_PER_QUARTER * 2)
        {
            // Draw Full Step
            symbolRT->DrawLine(D2D1::Point2F(StemX, StemStartY), D2D1::Point2F(StemX, StemEndY), Brush, ScaledLineThickness);

                 if (duration <= TICKS_PER_QUARTER / 8) { NumLines = 3; } // 32nd note
            else if (duration <= TICKS_PER_QUARTER / 4) { NumLines = 2; } // 16th note
            else if (duration <= TICKS_PER_QUARTER / 2) { NumLines = 1; } // 8th note

            // Draw horizontal lines
            if (NumLines > 0)
            {
                for (int i = 0; i < NumLines; i++)
                {
                    float LineY         = StemEndY - (i * ScaledLineSpacing);
                    float LineStartX    = StemX;
                    float LineEndX      = StemX + ScaledLineLength;

                    symbolRT->DrawLine(D2D1::Point2F(LineStartX, LineY), D2D1::Point2F(LineEndX, LineY), Brush, ScaledLineThickness);
                }
            }
        }
        
        // Add dot for dotted notes
        if (IsDottedDuration(duration))
        {
            float DotSize = std::min(2.0f + (logScale * 0.5f), 4.0f);
            float DotX = StemX + ScaledLineLength + (DotSize * 2) - 10;
            float DotY = StemEndY - (NumLines * ScaledLineSpacing);

            D2D1_ELLIPSE DotEllipse = D2D1::Ellipse(D2D1::Point2F(DotX, DotY), DotSize, DotSize);
            symbolRT->FillEllipse(DotEllipse, Brush);
        }
        else if (IsTripletDuration(duration))
        {
            // Calculate position for the "3" text
            float TextX = StemX + ScaledLineLength - 10;
            float TextY = StemEndY - (NumLines * ScaledLineSpacing) - 14; // -14 for approx text height

            // Draw the "3"
            D2D1_RECT_F TextRect = D2D1::RectF(TextX, TextY, TextX + 10, TextY + 14);
            std::wstring TripletText = L"3";

            symbolRT->DrawText(TripletText.c_str(), static_cast<UINT32>(TripletText.length()), GetMeasureNumberFormat(), TextRect, Brush);
        }
        
        Brush->Release();
    }

    hr = symbolRT->EndDraw();

    if (SUCCEEDED(hr)) {
        symbolRT->GetBitmap(ppBitmap);
    }

    symbolRT->Release();
    return SUCCEEDED(hr);
}

ID2D1Geometry* Timeline_Direct2DRenderer_Native::CreateDrumSymbolGeometry(int symbolType, float size, float bitmapSize)
{
	if (!m_pD2DFactory) {
		return nullptr;
	}

	ID2D1PathGeometry* PathGeometry = nullptr;
	HRESULT hr = m_pD2DFactory->CreatePathGeometry(&PathGeometry);
	if (FAILED(hr)) {
		return nullptr;
	}

	ID2D1GeometrySink* sink = nullptr;
	hr = PathGeometry->Open(&sink);

	if (FAILED(hr)) {
		PathGeometry->Release();
		return nullptr;
	}

	// Calculate center point and dimensions
	D2D1_POINT_2F Center = D2D1::Point2F(bitmapSize/2, bitmapSize/2);
	float HalfSize = size * 2.0f;

    float DiamondSwage = (HalfSize - HalfSize * 0.714f) / 2.0f;

	switch (symbolType)
	{
		case 0:  // FilledDiamond
		{
			sink->BeginFigure(D2D1::Point2F(Center.x, Center.y - HalfSize + 2*DiamondSwage), D2D1_FIGURE_BEGIN_FILLED);	// Top
			sink->AddLine(D2D1::Point2F(Center.x + HalfSize, Center.y + DiamondSwage));   // Right
			sink->AddLine(D2D1::Point2F(Center.x, Center.y + HalfSize));                  // Bottom
			sink->AddLine(D2D1::Point2F(Center.x - HalfSize, Center.y + DiamondSwage));   // Left
			sink->EndFigure(D2D1_FIGURE_END_CLOSED);
			break;
		}

		case 1:  // HollowDiamond
		{
			sink->BeginFigure(D2D1::Point2F(Center.x, Center.y - HalfSize + 2 * DiamondSwage), D2D1_FIGURE_BEGIN_HOLLOW);	// Top
			sink->AddLine(D2D1::Point2F(Center.x + HalfSize, Center.y + DiamondSwage));     // Right
			sink->AddLine(D2D1::Point2F(Center.x, Center.y + HalfSize));                    // Bottom
			sink->AddLine(D2D1::Point2F(Center.x - HalfSize, Center.y + DiamondSwage));     // Left
			sink->EndFigure(D2D1_FIGURE_END_CLOSED);
			break;
		}

		case 2:  // CircledX
		{
			// Draw circle
			float Radius = HalfSize * 1.0f; // before: 0.8f
			sink->BeginFigure(D2D1::Point2F(Center.x + Radius, Center.y), D2D1_FIGURE_BEGIN_HOLLOW);

			// Draw full circle using two arcs
			sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(Center.x - Radius, Center.y), D2D1::SizeF(Radius, Radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
			sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(Center.x + Radius, Center.y), D2D1::SizeF(Radius, Radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
			sink->EndFigure(D2D1_FIGURE_END_CLOSED);

			// Draw X
			float xSize = Radius * 0.7f;
			sink->BeginFigure(D2D1::Point2F(Center.x - xSize, Center.y - xSize), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x + xSize, Center.y + xSize));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);

			sink->BeginFigure(D2D1::Point2F(Center.x - xSize, Center.y + xSize), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x + xSize, Center.y - xSize));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);
			break;
		}

		case 3:  // AccentedX
		{
			float xSize = HalfSize * 0.8f;
			// Draw accent mark
			sink->BeginFigure(D2D1::Point2F(Center.x - xSize, Center.y - HalfSize * 1.5f), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x, Center.y - HalfSize * 2.0f));
			sink->AddLine(D2D1::Point2F(Center.x + xSize, Center.y - HalfSize * 1.5f));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);

			// Draw X
			sink->BeginFigure(D2D1::Point2F(Center.x - xSize, Center.y - xSize), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x + xSize, Center.y + xSize));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);

			sink->BeginFigure(D2D1::Point2F(Center.x - xSize, Center.y + xSize), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x + xSize, Center.y - xSize));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);
			break;
		}

		case 4:  // RegularX
		{
			float XSize = HalfSize * 0.8f;
			sink->BeginFigure(D2D1::Point2F(Center.x - XSize, Center.y - XSize), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x + XSize, Center.y + XSize));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);

			sink->BeginFigure(D2D1::Point2F(Center.x - XSize, Center.y + XSize), D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(D2D1::Point2F(Center.x + XSize, Center.y - XSize));
			sink->EndFigure(D2D1_FIGURE_END_OPEN);
			break;
		}

		case 5:  // NoteEllipse
		{
			// Adjust ellipse size for better proportions
			float RadiusX = HalfSize * 0.7f;
			float RadiusY = HalfSize * 0.5f;

            float RotationAngleDeg = -20.0f;
            float RotationAngleRad = RotationAngleDeg * (3.14159f / 180.0f); // Convert to radians

            // Create rotation matrix
            D2D1::Matrix3x2F Rotation = D2D1::Matrix3x2F::Rotation(RotationAngleDeg, Center);

            // Transform start point
            D2D1_POINT_2F StartPoint = D2D1::Point2F(Center.x + RadiusX, Center.y);
            D2D1_POINT_2F RotatedStart;
            RotatedStart.x = (StartPoint.x - Center.x) * cos(RotationAngleRad) - (StartPoint.y - Center.y) * sin(RotationAngleRad) + Center.x;
            RotatedStart.y = (StartPoint.x - Center.x) * sin(RotationAngleRad) + (StartPoint.y - Center.y) * cos(RotationAngleRad) + Center.y;

			sink->BeginFigure(D2D1::Point2F(Center.x + RadiusX, Center.y),D2D1_FIGURE_BEGIN_FILLED);

            // Create full ellipse using two arcs with rotation
            sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(Center.x - RadiusX * cos(RotationAngleRad), Center.y - RadiusX * sin(RotationAngleRad)), D2D1::SizeF(RadiusX, RadiusY), RotationAngleDeg, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            sink->AddArc(D2D1::ArcSegment(RotatedStart, D2D1::SizeF(RadiusX, RadiusY), RotationAngleDeg, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
			break;
		}

		case 6:  // Unknown (error symbol)
		{
			float radius = HalfSize * 0.7f;
			sink->BeginFigure(D2D1::Point2F(Center.x + radius, Center.y), D2D1_FIGURE_BEGIN_FILLED);
			sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(Center.x - radius, Center.y), D2D1::SizeF(radius, radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
			sink->EndFigure(D2D1_FIGURE_END_CLOSED);
			break;
		}
	}

	sink->Close();
	sink->Release();

	return PathGeometry;
}

bool Timeline_Direct2DRenderer_Native::IsDottedDuration(int duration) const
{
    return  duration == TICKS_PER_QUARTER * 6       || duration == TICKS_PER_QUARTER * 3        || duration == TICKS_PER_QUARTER * 3 / 2 ||
            duration == TICKS_PER_QUARTER * 3 / 4   || duration == TICKS_PER_QUARTER * 3 / 8    || duration == TICKS_PER_QUARTER * 3 / 16;
}

bool Timeline_Direct2DRenderer_Native::IsTripletDuration(int duration) const
{
    return duration == TICKS_PER_QUARTER * 2 / 3 || duration == TICKS_PER_QUARTER * 2 / 6 || duration == TICKS_PER_QUARTER * 2 / 12;
}

bool Timeline_Direct2DRenderer_Native::CreateTextFormats()
{
    if (!m_pDWriteFactory) {
        return false;
    }

    // Create measure number format
    HRESULT hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &m_pMeasureNumberFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pMeasureNumberFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pMeasureNumberFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    

    // Create test marker format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &m_pMarkerTextFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pMarkerTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pMarkerTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create time signature format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &m_pTimeSignatureFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pTimeSignatureFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pTimeSignatureFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create quarter note format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 8.0f, L"en-us", &m_pQuarterNoteFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pQuarterNoteFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pQuarterNoteFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create quarter note format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &m_pTrackHeaderFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pTrackHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_pTrackHeaderFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create quarter note format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &m_pTablatureFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pTablatureFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_pTablatureFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);



    // Create FPS Counter format
    hr = m_pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &m_pFPSCounterFormat);

    if (FAILED(hr)) {
        return false;
    }

    // Set text alignment
    m_pFPSCounterFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_pFPSCounterFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    return true;
}

bool Timeline_Direct2DRenderer_Native::CreateBitmapFromImage(System::Drawing::Image^ image, ID2D1Bitmap** ppBitmap)
{
    if (!m_pRenderTarget || !image || !ppBitmap) {
        return false;
    }

    // Create temporary GDI+ bitmap to access pixels
    System::Drawing::Bitmap^ GDIBitmap = gcnew System::Drawing::Bitmap(image);
    System::Drawing::Rectangle rect(0, 0, GDIBitmap->Width, GDIBitmap->Height);
    System::Drawing::Imaging::BitmapData^ BitmapData = GDIBitmap->LockBits(rect, System::Drawing::Imaging::ImageLockMode::ReadOnly, System::Drawing::Imaging::PixelFormat::Format32bppPArgb);

    // Create D2D bitmap properties
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    // Create the bitmap
    HRESULT hr = m_pRenderTarget->CreateBitmap(D2D1::SizeU(GDIBitmap->Width, GDIBitmap->Height), BitmapData->Scan0.ToPointer(), BitmapData->Stride, props, ppBitmap);

    GDIBitmap->UnlockBits(BitmapData);
    delete GDIBitmap;

    return SUCCEEDED(hr);
}

bool Timeline_Direct2DRenderer_Native::CreateAndCacheBitmap(const std::wstring& key, System::Drawing::Image^ image)
{
	ID2D1Bitmap* Bitmap = nullptr;
	if (!CreateBitmapFromImage(image, &Bitmap)) {
		return false;
	}

	m_BitmapCache.push_back({ key, Bitmap });

	return true;
}

bool Timeline_Direct2DRenderer_Native::DrawBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& destRect, float opacity)
{
    if (!m_pRenderTarget || !bitmap) {
        return false;
    }

    m_pRenderTarget->DrawBitmap(bitmap, destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

    return true;
}

ID2D1SolidColorBrush* Timeline_Direct2DRenderer_Native::GetCachedBrush(const D2D1_COLOR_F& color)
{
	// Check cache first
	for (const auto& cached : m_BrushCache)
	{
		if (memcmp(&cached.Color, &color, sizeof(D2D1_COLOR_F)) == 0)
			return cached.Brush;
	}

	// Create new brush
	ID2D1SolidColorBrush* brush = nullptr;
	HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(color, &brush);

	if (FAILED(hr))
		return nullptr;

	// Add to cache
	CachedBrush newCached;
	newCached.Color = color;
	newCached.Brush = brush;
	m_BrushCache.push_back(newCached);

	return brush;
}

ID2D1Bitmap* Timeline_Direct2DRenderer_Native::GetCachedBitmap(const std::wstring& key)
{
    auto it = std::find_if(m_BitmapCache.begin(), m_BitmapCache.end(), [&key](const CachedBitmap& cached) { return cached.Key == key; });

    return it != m_BitmapCache.end() ? it->Bitmap : nullptr;
}

bool Timeline_Direct2DRenderer_Native::DrawLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddLine(x1, y1, x2, y2, color, strokeWidth);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
	}

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth);

    return true;
#endif
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
#if USE_BATCH_RENDERING
    m_CommandBatch.AddRectangle(rect, color, strokeWidth);
    return true;
#else    
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
#endif
}

bool Timeline_Direct2DRenderer_Native::DrawRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);

    return DrawRectangle(rect, color, strokeWidth);
}

bool Timeline_Direct2DRenderer_Native::DrawRectangleDashed(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float dashLength, float gapLength)
{
#if USE_BATCH_RENDERING
    float zOrder = m_ZOrderCounter++;
    m_CommandBatch.AddDashedRectangle(rect, color, strokeWidth, dashLength, gapLength, zOrder);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    // Get brush
    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

	// Create dashed stroke style with the specified pattern
	D2D1_STROKE_STYLE_PROPERTIES dashedProps = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_FLAT,
		D2D1_CAP_STYLE_FLAT,
		D2D1_CAP_STYLE_FLAT,
		D2D1_LINE_JOIN_MITER,
		10.0f,
		D2D1_DASH_STYLE_CUSTOM,
		0.0f
	);

	// Calculate dash pattern (normalize by stroke width)
	float dashes[] = { dashLength / strokeWidth, gapLength / strokeWidth };

	// Release existing dashed stroke style
	if (m_pDashedStroke) {
		m_pDashedStroke->Release();
		m_pDashedStroke = nullptr;
	}

	// Create new stroke style with the specified dash pattern
	HRESULT hr = m_pD2DFactory->CreateStrokeStyle(
		dashedProps,
		dashes,
		ARRAYSIZE(dashes),
		&m_pDashedStroke
	);

	if (FAILED(hr)) {
		return false;
	}

	// Draw the rectangle with the dashed stroke style
	m_pRenderTarget->DrawRectangle(rect, brush, strokeWidth, m_pDashedStroke);

	return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::FillRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddFilledRectangle(rect, color);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillRectangle(rect, brush);
    return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::FillRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);

    return FillRectangle(rect, color);
}

bool Timeline_Direct2DRenderer_Native::FillRectangleGradient2(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorLeft, const D2D1_COLOR_F& colorRight)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddGradientRectangle(rect, colorLeft, colorRight, true);
    return true;
#else
    D2D1_GRADIENT_STOP gradientStops[2];
	gradientStops[0].color = colorLeft;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = colorRight;
	gradientStops[1].position = 1.0f;

	return FillRectangleGradient(rect, gradientStops, 2);
#endif
}

bool Timeline_Direct2DRenderer_Native::FillRectangleGradient3(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorLeft, const D2D1_COLOR_F& colorCenter, const D2D1_COLOR_F& colorRight)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddGradientRectangle(rect, colorLeft, colorCenter, colorRight, true);
    return true;
#else
    D2D1_GRADIENT_STOP gradientStops[3];
	gradientStops[0].color = colorLeft;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = colorCenter;
	gradientStops[1].position = 0.5f;
	gradientStops[2].color = colorRight;
	gradientStops[2].position = 1.0f;

	return FillRectangleGradient(rect, gradientStops, 3);
#endif
}

bool Timeline_Direct2DRenderer_Native::FillRectangleGradient(const D2D1_RECT_F& rect, const D2D1_GRADIENT_STOP* gradientStops, UINT32 count)
{
#if USE_BATCH_RENDERING
    if (gradientStops == NULL || count < 2) {
        return false;
    }

    // Convert the gradient stops array to a vector for the CommandBatch
    std::vector<D2D1_GRADIENT_STOP> stops;
    for (UINT32 i = 0; i < count; i++) {
        stops.push_back(gradientStops[i]);
    }

    m_CommandBatch.AddGradientRectangle(rect, stops, true);
    return true;
#else
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
#endif
}

bool Timeline_Direct2DRenderer_Native::FillRectangleStripes(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float stripeWidth)
{
#if !(USE_BATCH_RENDERING)
    if (!m_pRenderTarget) {
		return false;
	}

	ID2D1SolidColorBrush* brush = GetCachedBrush(color);
	if (!brush) {
		return false;
	}
#endif	
	// Calculate the number of stripes needed
	float RectWidth			= rect.right - rect.left;
	float TotalStripeWidth	= stripeWidth * 2; // Including the transparent gap
	int	NumStripes			= static_cast<int>(ceil(RectWidth / TotalStripeWidth));

	// Draw each stripe
	for (int i = 0; i < NumStripes; i++)
	{
		float X = rect.left + (i * TotalStripeWidth);

		D2D1_RECT_F stripeRect = D2D1::RectF(
			X,					// Left
			rect.top,			// Top
			X + stripeWidth,	// Right
			rect.bottom			// Bottom
		);

		// Only draw if the stripe is at least partially within the rectangle
		if (stripeRect.left < rect.right)
        {
			// Clip the stripe if it extends beyond the rectangle
			if (stripeRect.right > rect.right)
            {
				stripeRect.right = rect.right;
			}
#if USE_BATCH_RENDERING
            m_CommandBatch.AddFilledRectangle(stripeRect, color);
#else
			m_pRenderTarget->FillRectangle(stripeRect, brush);
#endif
		}
	}

	return true;

}

bool Timeline_Direct2DRenderer_Native::DrawRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float strokeWidth)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddRoundedRectangle(rect, color, strokeWidth);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }
    
    m_pRenderTarget->DrawRoundedRectangle(rect, brush, strokeWidth);
    return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::DrawRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_ROUNDED_RECT rectRound = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), 2, 2); // Radius to be updated here

    return DrawRoundedRectangle(rectRound, color, strokeWidth);
}

bool Timeline_Direct2DRenderer_Native::FillRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddFilledRoundedRectangle(rect, color);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillRoundedRectangle(rect, brush);
    return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::FillRoundedRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a)
{
    D2D1_COLOR_F color = D2D1::ColorF(r, g, b, a);
    D2D1_ROUNDED_RECT rectRound = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), 2, 2); // Radius to be updated here

    return FillRoundedRectangle(rectRound, color);
}

bool Timeline_Direct2DRenderer_Native::DrawEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float strokeWidth)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddEllipse(ellipse, color, strokeWidth);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->DrawEllipse(ellipse, brush, strokeWidth);
    return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::FillEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddFilledEllipse(ellipse, color);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

    m_pRenderTarget->FillEllipse(ellipse, brush);
    return true;
#endif
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

    ID2D1SolidColorBrush* Brush = GetCachedBrush(color);
    if (!Brush) {
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
    float Width  = points[1].x - points[3].x;
    float Height = points[2].y - points[0].y;

    // Save current transform
    D2D1_MATRIX_3X2_F OldTransform;
    m_pRenderTarget->GetTransform(&OldTransform);

    // Create transform matrix for 45-degree rotation around center
    D2D1_MATRIX_3X2_F RotationMatrix = D2D1::Matrix3x2F::Rotation(45.0f, center);
    m_pRenderTarget->SetTransform(RotationMatrix);

    // Draw rotated rectangle
    D2D1_RECT_F Rect = D2D1::RectF(
        center.x - Width  / 2.8284f,  // Adjust for 45-degree rotation (1/√2 ≈ 1/1.4142)
        center.y - Height / 2.8284f,
        center.x + Width  / 2.8284f,
        center.y + Height / 2.8284f
    );

    m_pRenderTarget->FillRectangle(Rect, Brush);

    // Restore original transform
    m_pRenderTarget->SetTransform(OldTransform);

    return true;
}

bool Timeline_Direct2DRenderer_Native::DrawTieLine(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2, const D2D1_POINT_2F& point3, const D2D1_POINT_2F& point4, const D2D1_COLOR_F& color, float strokeWidth)
{
#if USE_BATCH_RENDERING
    m_CommandBatch.AddTieLine(point1, point2, point3, point4, color, strokeWidth);
    return true;
#else
    if (!m_pRenderTarget || !m_pD2DFactory) {
        return false;
    }

    // Get brush for the color
    ID2D1SolidColorBrush* brush = GetCachedBrush(color);
    if (!brush) {
        return false;
    }

	// Create path geometry
	ID2D1PathGeometry* pathGeometry = nullptr;
	HRESULT hr = m_pD2DFactory->CreatePathGeometry(&pathGeometry);
	if (FAILED(hr) || !pathGeometry) {
		return false;
	}

	// Open the geometry sink
	ID2D1GeometrySink* sink = nullptr;
	hr = pathGeometry->Open(&sink);
	if (FAILED(hr) || !sink) {
		pathGeometry->Release();
		return false;
	}

	// Define the bezier curve
	sink->BeginFigure(point1, D2D1_FIGURE_BEGIN_HOLLOW);

	D2D1_BEZIER_SEGMENT bezier;
	bezier.point1 = point2;  // First control point
	bezier.point2 = point3;  // Second control point
	bezier.point3 = point4;  // End point

	sink->AddBezier(bezier);
	sink->EndFigure(D2D1_FIGURE_END_OPEN);

	// Close the sink
	hr = sink->Close();
	sink->Release();

	if (SUCCEEDED(hr)) {
		// Draw the bezier curve
		m_pRenderTarget->DrawGeometry(pathGeometry, brush, strokeWidth);
	}

	// Clean up
	pathGeometry->Release();

	return SUCCEEDED(hr);
#endif
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
#if USE_BATCH_RENDERING
    if (!textFormat) {
        return false;
    }

    m_CommandBatch.AddText(text, layoutRect, color, textFormat);
    return true;
#else
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
#endif
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
#if USE_BATCH_RENDERING
    ID2D1Bitmap* bitmap = GetCachedBitmap(key);
    if (!bitmap) {
        return false;
    }

    m_CommandBatch.AddBitmap(bitmap, destRect, opacity);
    return true;
#else
    ID2D1Bitmap* Bitmap = GetCachedBitmap(key);
    return Bitmap ? DrawBitmap(Bitmap, destRect, opacity) : false;
#endif
}

bool Timeline_Direct2DRenderer_Native::DrawCachedTabText(int fretNumber, const D2D1_RECT_F& destRect, float fontSize)
{
#if USE_BATCH_RENDERING
    TabTextCacheEntry* cachedEntry = GetNearestCachedTabTextEntry(fontSize);
    if (!cachedEntry) {
        return false;
    }

    auto it = cachedEntry->TabTexts.find(fretNumber);
    if (it == cachedEntry->TabTexts.end()) {
        return false;
    }

    m_CommandBatch.AddBitmap(it->second, destRect, 1.0f);
    //m_CommandBatch.AddTabText(it->second, destRect);
    return true;
#else
    if (!m_pRenderTarget) {
		return false;
	}

	TabTextCacheEntry* CachedEntry = GetNearestCachedTabTextEntry(fontSize);
	if (!CachedEntry) {
		return false;
	}

	auto it = CachedEntry->TabTexts.find(fretNumber);
	if (it == CachedEntry->TabTexts.end()) {
		// Symbol not found in cache - this shouldn't happen if preloading worked correctly
		return false;
	}

	// Draw the cached bitmap
	m_pRenderTarget->DrawBitmap(
		it->second,    // The cached bitmap
		destRect,      // Destination rectangle
		1.0f,          // Opacity
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR  // Use linear interpolation for smooth scaling
	);

	return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::DrawCachedDrumSymbol(int symbolIndex, const D2D1_RECT_F& destRect, float stringSpace)
{
#if USE_BATCH_RENDERING
    DrumSymbolCacheEntry* cachedEntry = GetNearestCachedDrumSymbolEntry(stringSpace);
    if (!cachedEntry) {
        return false;
    }

    auto it = cachedEntry->DrumSymbols.find(symbolIndex);
    if (it == cachedEntry->DrumSymbols.end()) {
        return false;
    }

    m_CommandBatch.AddBitmap(it->second, destRect, 1.0f);
    //m_CommandBatch.AddDrumSymbol(it->second, destRect);
    return true;
#else
    if (!m_pRenderTarget) {
		return false;
	}

	DrumSymbolCacheEntry* CachedEntry = GetNearestCachedDrumSymbolEntry(stringSpace);
	if (!CachedEntry) {
		return false;
	}

	auto it = CachedEntry->DrumSymbols.find(symbolIndex);
	if (it == CachedEntry->DrumSymbols.end()) {
		// Symbol not found in cache - this shouldn't happen if preloading worked correctly
		return false;
	}

	// Draw the cached bitmap
	m_pRenderTarget->DrawBitmap(
		it->second,    // The cached bitmap
		destRect,      // Destination rectangle
		1.0f,          // Opacity
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR  // Use linear interpolation for smooth scaling
	);
	
	return true;
#endif
}

bool Timeline_Direct2DRenderer_Native::DrawCachedDudationSymbol(int duration, const D2D1_RECT_F& destRect, float zoomLevel)
{
#if USE_BATCH_RENDERING
    DurationSymbolCacheEntry* cachedEntry = GetNearestCachedDurationSymbolEntry(zoomLevel);
    if (!cachedEntry) {
        return false;
    }

    auto it = cachedEntry->DurationSymbols.find(duration);
    if (it == cachedEntry->DurationSymbols.end()) {
        return false;
    }

    m_CommandBatch.AddBitmap(it->second, destRect, 1.0f);
    //m_CommandBatch.AddDurationSymbol(it->second, destRect);
    return true;
#else
    if (!m_pRenderTarget) {
        return false;
    }

    DurationSymbolCacheEntry* CachedEntry = GetNearestCachedDurationSymbolEntry(zoomLevel);
    if (!CachedEntry) {
        return false;
    }

    auto it = CachedEntry->DurationSymbols.find(duration);
    if (it == CachedEntry->DurationSymbols.end()) {
        // Symbol not found in cache - this shouldn't happen if preloading worked correctly
        return false;
    }

    // Draw the cached bitmap
    m_pRenderTarget->DrawBitmap(
        it->second,    // The cached bitmap
        destRect,      // Destination rectangle
        1.0f,          // Opacity
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR  // Use linear interpolation for smooth scaling
    );

    return true;
#endif
}

TabTextCacheEntry* Timeline_Direct2DRenderer_Native::GetNearestCachedTabTextEntry(float fontSize)
{
	if (m_TabTextCache.empty()) {
		return nullptr;
	}

	auto it = std::min_element(m_TabTextCache.begin(), m_TabTextCache.end(), [fontSize](const TabTextCacheEntry& a, const TabTextCacheEntry& b)
		{
			return std::abs(a.FontSize - fontSize) < std::abs(b.FontSize - fontSize);
		});

	return &(*it);
}

DrumSymbolCacheEntry* Timeline_Direct2DRenderer_Native::GetNearestCachedDrumSymbolEntry(float stringSpace)
{
	if (m_DrumSymbolCache.empty()) {
		return nullptr;
	}

	// Find the cache entry with the closest font size
	auto it = std::min_element(m_DrumSymbolCache.begin(), m_DrumSymbolCache.end(), [stringSpace](const DrumSymbolCacheEntry& a, const DrumSymbolCacheEntry& b)
		{
			return std::abs(a.StringSpace - stringSpace) < std::abs(b.StringSpace - stringSpace);
		});

	return &(*it);
}

DurationSymbolCacheEntry* Timeline_Direct2DRenderer_Native::GetNearestCachedDurationSymbolEntry(float zoomLevel)
{
    if (m_DurationSymbolCache.empty()) {
        return nullptr;
    }

    // Find the cache entry with the closest font size
    auto it = std::min_element(m_DurationSymbolCache.begin(), m_DurationSymbolCache.end(), [zoomLevel](const DurationSymbolCacheEntry& a, const DurationSymbolCacheEntry& b)
        {
            return std::abs(a.ZoomLevel - zoomLevel) < std::abs(b.ZoomLevel - zoomLevel);
        });

    return &(*it);
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
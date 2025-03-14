#include "Timeline_Direct2DRenderer.h"
#include "Timeline_Direct2DRenderer_Native.h"

#include "Easings.h"
#include "Widget_Timeline_Common.h"
#include "Widget_Timeline_Classes.h"

#include <vcclr.h>
#include <msclr\marshal_cppstd.h>

#define COLOR_TO_RGBA(__color__)                    __color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, __color__.A / 255.0f
#define COLOR_TO_COLOR_F(__color__)                 D2D1::ColorF(__color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, __color__.A / 255.0f)
#define COLOR_TO_COLOR_F_A(__color__, __alpha__)    D2D1::ColorF(__color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, (__color__.A / 255.0f) * __alpha__)
#define RECT_TO_RECT_F(__rect__)                    D2D1::RectF((float)__rect__.Left, (float)__rect__.Top, (float)__rect__.Right, (float)__rect__.Bottom)
#define TO_LOGSCALE(__V__)							(float)Math::Log((float)__V__ + 1.0f, 2)


namespace MIDILightDrawer
{
    Timeline_Direct2DRenderer::Timeline_Direct2DRenderer(List<Track^>^ tracks, List<Measure^>^ measures, Collapsible_Left_Panel^ leftPanel, double zoomlevel, System::Drawing::Point^ scrollposition) : _NativeRenderer(nullptr), m_disposed(false), _ControlHandle(System::IntPtr::Zero), m_themeColorsCached(false)
    {
        this->_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
        
        // Create the native renderer
        this->_NativeRenderer = new Timeline_Direct2DRenderer_Native();

        this->_Control				= nullptr;
        this->_Tracks				= tracks;
        this->_Measures				= measures;
		this->_Left_Panel			= leftPanel;
        this->_ToolAccessDelegate	= nullptr;

        SetZoomLevel(zoomlevel);
        SetScrollPositionReference(scrollposition);
        UpdateLevelOfDetail();
    }

    Timeline_Direct2DRenderer::~Timeline_Direct2DRenderer()
    {
        this->!Timeline_Direct2DRenderer();
    }

    Timeline_Direct2DRenderer::!Timeline_Direct2DRenderer()
    {
        Cleanup();
    }

    bool Timeline_Direct2DRenderer::Initialize(System::Windows::Forms::Control^ control, LoadingStatusCallback^ loadingCallback, LoadingProgressCallback^ progressCallback)
    {
        if (m_disposed || !control) {
            return false;
        }

        this->_Control = control;

        // Store control handle
        _ControlHandle = control->Handle;

		// Initialize the native renderer
		if (!_NativeRenderer->Initialize((HWND)_ControlHandle.ToPointer())) {
			return false;
		}
		
		if (loadingCallback != nullptr)
		{
			loadingCallback(LoadingStage::Images);
			PreloadImages(progressCallback);
			
			loadingCallback(LoadingStage::TabText);
			PreloadTabTexts(progressCallback);

			loadingCallback(LoadingStage::DrumSymbols);
			PreloadDrumSymbols(progressCallback);

            loadingCallback(LoadingStage::DurationSymbols);
            PreloadDurationSymbols(progressCallback);

			loadingCallback(LoadingStage::Complete);
		}
		else
		{ 
            PreloadImages(nullptr);
            PreloadTabTexts(nullptr);
			PreloadDrumSymbols(nullptr);
            PreloadDurationSymbols(nullptr);
		}

		return true;
	}

    void Timeline_Direct2DRenderer::Resize(int width, int height)
    {
        if (m_disposed || !_NativeRenderer) {
            return;
        }

        _NativeRenderer->ResizeRenderTarget(width, height);
    }

    void Timeline_Direct2DRenderer::SetThemeColors(System::Drawing::Color background, System::Drawing::Color headerbackground, System::Drawing::Color text, System::Drawing::Color measureline, System::Drawing::Color beatline, System::Drawing::Color subdivisionline, System::Drawing::Color selectionhighlight, System::Drawing::Color trackbackground, System::Drawing::Color trackborder)
    {
        this->m_ColorTheme.Background           = background;
        this->m_ColorTheme.HeaderBackground     = headerbackground;
        this->m_ColorTheme.Text                 = text;
        this->m_ColorTheme.MeasureLine          = measureline;
        this->m_ColorTheme.BeatLine             = beatline;
        this->m_ColorTheme.SubdivisionLine      = subdivisionline;
        this->m_ColorTheme.SelectionHighlight   = selectionhighlight;
        this->m_ColorTheme.TrackBackground      = trackbackground;
        this->m_ColorTheme.TrackBorder          = trackborder;

        m_themeColorsCached = true;
    }

    void Timeline_Direct2DRenderer::SetZoomLevel(double zoomlevel)
    {
        this->_ZoomLevel = (float)zoomlevel;
    }

    void Timeline_Direct2DRenderer::SetScrollPositionReference(System::Drawing::Point^ scrollposition)
    {
        this->_ScrollPosition = scrollposition;
    }

    void Timeline_Direct2DRenderer::SetTimelineAccess(ITimelineAccess^ access)
    {
        this->_ToolAccessDelegate = access;
    }

    void Timeline_Direct2DRenderer::PreloadImages(LoadingProgressCallback^ progressCallback)
    {
        if (progressCallback != nullptr) {
            progressCallback(0.33f);
        }
       
       // Draw the image with color transformation if needed
        System::Drawing::Imaging::ColorMatrix^ ColorMatrix = gcnew System::Drawing::Imaging::ColorMatrix(
            gcnew array<array<float>^> {
            gcnew array<float>{ m_ColorTheme.Text.R / 255.0f, 0, 0, 0, 0 },
            gcnew array<float>{ 0, m_ColorTheme.Text.G / 255.0f, 0, 0, 0 },
            gcnew array<float>{ 0, 0, m_ColorTheme.Text.B / 255.0f, 0, 0 },
            gcnew array<float>{ 0, 0, 0, m_ColorTheme.Text.A / 255.0f, 0 },
            gcnew array<float>{ 0, 0, 0, 0, 1.0f }
        }
        );
        
        if (progressCallback != nullptr) {
            progressCallback(0.66f);
        }

        System::Drawing::Imaging::ImageAttributes^ Attributes = gcnew System::Drawing::Imaging::ImageAttributes();
        Attributes->SetColorMatrix(ColorMatrix);

        // Preload icons
        System::Drawing::Image^ NoteIcon = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Note_White")));
        System::Drawing::Bitmap^ TransformedIcon = gcnew System::Drawing::Bitmap(NoteIcon->Width, NoteIcon->Height);

        {
            System::Drawing::Graphics ^ g = System::Drawing::Graphics::FromImage(TransformedIcon);
            g->DrawImage(NoteIcon,
                System::Drawing::Rectangle(0, 0, NoteIcon->Width, NoteIcon->Height),    // Dest Rectangle
                0, 0, NoteIcon->Width, NoteIcon->Height,                                // Source Rectangle
                System::Drawing::GraphicsUnit::Pixel,
                Attributes);
            delete g;
        }
        
        this->_NativeRenderer->PreloadBitmap(L"Note_White", TransformedIcon);

        if (progressCallback != nullptr) {
            progressCallback(1.00f);
        }
    }

	void Timeline_Direct2DRenderer::PreloadTabTexts(LoadingProgressCallback^ progressCallback)
	{
        float TotalSteps = (float)(MAX_ZOOM_LEVEL - MIN_ZOOM_LEVEL) / 0.1f;
        float CurrentStep = 0;
        
        for (float z = (float)MIN_ZOOM_LEVEL; z <= (float)MAX_ZOOM_LEVEL; z += 0.1f)
		{
			float LogScale = TO_LOGSCALE(z);
			float FontSize = GetTablatureScaledFontSize(LogScale);

			_NativeRenderer->PreloadTabText(FontSize, COLOR_TO_COLOR_F(m_ColorTheme.Text), COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBackground, 0.3f)); // before 0.86f

            CurrentStep++;
            if (progressCallback != nullptr)
            {
                progressCallback(CurrentStep / TotalSteps);
            }
		}
	}

	void Timeline_Direct2DRenderer::PreloadDrumSymbols(LoadingProgressCallback^ progressCallback)
	{
        float TotalSteps = (float)(MAX_ZOOM_LEVEL - MIN_ZOOM_LEVEL) / 0.1f;
        float CurrentStep = 0;
        
        for(float z = (float)MIN_ZOOM_LEVEL ; z <= (float)MAX_ZOOM_LEVEL ; z += 0.1f)
		{
			float LogScale = TO_LOGSCALE(z);

			float StringSpacing = GetTablatureScaledStringSpacing(LogScale);
			float Size = (StringSpacing / 2.0f) - 1.0f;

			_NativeRenderer->PreloadDrumSymbol(Size, COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f), COLOR_TO_COLOR_F_A(Color::Transparent, 0.0f));

            CurrentStep++;
            if (progressCallback != nullptr)
            {
                progressCallback(CurrentStep / TotalSteps);
            }
		}
	}

    void Timeline_Direct2DRenderer::PreloadDurationSymbols(LoadingProgressCallback^ progressCallback)
    {
        float TotalSteps = (float)(MAX_ZOOM_LEVEL - MIN_ZOOM_LEVEL) / 0.1f;
        float CurrentStep = 0;
        
        for (float z = (float)MIN_ZOOM_LEVEL; z <= (float)MAX_ZOOM_LEVEL; z += 0.1f)
        {
            float LogScale = TO_LOGSCALE(z);

            _NativeRenderer->PreloadDurationSymbols(z, LogScale, COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f), COLOR_TO_COLOR_F_A(Color::Transparent, 0.0f));

            CurrentStep++;
            if (progressCallback != nullptr)
            {
                progressCallback(CurrentStep / TotalSteps);
            }
        }
    }

    bool Timeline_Direct2DRenderer::BeginDraw()
    {
        if (m_disposed || !_NativeRenderer) {
            return false;
        }

        bool Result = _NativeRenderer->BeginDraw();

        // Start a new batch if draw was successful
        if (Result) {
            _NativeRenderer->BeginBatch();
        }

        return Result;
    }

    bool Timeline_Direct2DRenderer::EndDraw()
    {
        if (m_disposed || !_NativeRenderer) {
            return false;
        }

        // Execute the batch before ending draw
        _NativeRenderer->ExecuteBatch();

        return _NativeRenderer->EndDraw();
    }

    void Timeline_Direct2DRenderer::UpdateLevelOfDetail()
    {
        float PixelsPerFullMeasure = TicksToPixels(TICKS_PER_QUARTER * 4);
        
        m_LevelOfDetail.MeasureSkipFactor = GetMeasureSkipFactor(PixelsPerFullMeasure);
        m_LevelOfDetail.ShowBeatLines = (PixelsPerFullMeasure > MIN_MEASURE_BEAT_SPACING);
        m_LevelOfDetail.ShowAllNotes = this->_ZoomLevel >= 1.0f;
        m_LevelOfDetail.ShowTieLines = this->_ZoomLevel >= 0.2f;

        // Determine tick resolution based on zoom level
        float PixelsPerQuarter = TicksToPixels(TICKS_PER_QUARTER);

        if (PixelsPerQuarter >= 15.0f) {
            m_LevelOfDetail.TabResolution = TablatureResolution::FullDetail;
        }
        else if (PixelsPerQuarter >= 10.0f) {
            m_LevelOfDetail.TabResolution = TablatureResolution::BeatLevel;
        }
        else if (PixelsPerQuarter >= 5.0f) {
            m_LevelOfDetail.TabResolution = TablatureResolution::HalfMeasure;
        }
        else {
            m_LevelOfDetail.TabResolution = TablatureResolution::MeasureLevel;
        }
    }

    bool Timeline_Direct2DRenderer::DrawWidgetBackground()
    {
        if (!_NativeRenderer) {
            return false;
        }

        // Create the rectangle for the tracks area
        D2D1_RECT_F WidgetArea = D2D1::RectF(
            0,                              // Left
            0,                              // Top
            (float)this->_Control->Width,   // Right
            (float)this->_Control->Height   // Bottom
        );

        // Fill the rectangle using the native renderer
        return _NativeRenderer->FillRectangle(WidgetArea, COLOR_TO_COLOR_F(m_ColorTheme.Background));
    }
    
    bool Timeline_Direct2DRenderer::DrawTrackBackground()
    {
        if (!_NativeRenderer) {
            return false;
        }

        // Create the rectangle for the tracks area
        D2D1_RECT_F tracksArea = D2D1::RectF(
			(float)GetLeftPanelWidth(),			// Left
            HEADER_HEIGHT,					// Top
            (float)this->_Control->Width,	// Right
            (float)this->_Control->Height	// Bottom
        );

        // Fill the rectangle using the native renderer
        return _NativeRenderer->FillRectangle(tracksArea, COLOR_TO_COLOR_F(m_ColorTheme.TrackBackground));
    }

    bool Timeline_Direct2DRenderer::DrawMeasureNumbers()
    {
        if (!_NativeRenderer || this->_Measures == nullptr || this->_Measures->Count == 0) {
            return false;
        }

        // Fill the header background
        D2D1_RECT_F HeaderRect = D2D1::RectF(0, 0, (float)this->_Control->Width, HEADER_HEIGHT);
        _NativeRenderer->FillRectangle(HeaderRect, COLOR_TO_COLOR_F(m_ColorTheme.HeaderBackground));

        // Constants for vertical positioning within header
        const float MarkerTextY = 2;
        const float TimeSignatureY = MarkerTextY + 14 + 2;
        const float MeasureNumberY = TimeSignatureY + 14 + 4;

        D2D1_COLOR_F D2DTextColor = COLOR_TO_COLOR_F(m_ColorTheme.Text);

        int Accumulated = 0;
        int MeasureNumber = 1;

        // For each measure
        for each(Measure^ Meas in  this->_Measures)
        {
            bool ShouldDrawDetails = (MeasureNumber % m_LevelOfDetail.MeasureSkipFactor) == 0;
                
            // Convert tick position to pixels
            float X = TicksToPixels(Accumulated) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth();

            if (X >= GetLeftPanelAndTrackHeaderWidth() && X <= (float)this->_Control->Width)
            {
                // Draw marker text if present
                if (!System::String::IsNullOrEmpty(Meas->Marker_Text))
                {
                    D2D1_RECT_F MarkerRect = D2D1::RectF(X - 100, MarkerTextY, X + 100, MarkerTextY + 14);

                    std::wstring MarkerText = ConvertString(Meas->Marker_Text);
                    _NativeRenderer->DrawText(MarkerText, MarkerRect, D2DTextColor, _NativeRenderer->GetMarkerTextFormat());
                }
                
                if(ShouldDrawDetails) {
                    // Draw vertical tick mark for measure start
                    _NativeRenderer->DrawLine(X, HEADER_HEIGHT - 5,  X, HEADER_HEIGHT - 0, D2DTextColor, 1.0f);

                    // Draw measure number
                    D2D1_RECT_F NumberRect = D2D1::RectF(X - 25, MeasureNumberY, X + 25, MeasureNumberY + 14);
                    std::wstring NumText = std::to_wstring(MeasureNumber);
                    _NativeRenderer->DrawText(NumText, NumberRect, D2DTextColor, _NativeRenderer->GetMeasureNumberFormat());

                    // Draw time signature
                    D2D1_RECT_F SigRect = D2D1::RectF(X - 25, TimeSignatureY, X + 25, TimeSignatureY + 14);
                    std::wstring TimeSignature = std::to_wstring(Meas->Numerator) + L"/" + std::to_wstring(Meas->Denominator);
                    _NativeRenderer->DrawText(TimeSignature, SigRect, D2DTextColor, _NativeRenderer->GetTimeSignatureFormat());
                }
            }

            // Calculate subdivision level based on time signature
            int TicksPerBeat = Meas->Denominator == 8 ?
                TICKS_PER_QUARTER / 2 :  // 8th note-based measures
                TICKS_PER_QUARTER;       // Quarter note-based measures

            float PixelsPerBeat = TicksToPixels(TicksPerBeat);
            float SubdivLevel = GetSubdivisionLevel(PixelsPerBeat);

            // Determine if we should show subdivisions
            bool ShowSubdivisions = (Meas->Denominator == 8) ?
                (SubdivLevel >= 3) :  // For 8th-based measures
                (SubdivLevel >= 2);   // For quarter-based measures

            if (ShowSubdivisions)
            {
                DrawBeatNumbers(Meas, X, MeasureNumberY, SubdivLevel, MeasureNumber, TicksPerBeat);
            }

            Accumulated += Meas->Length;
            MeasureNumber++;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackContent(Track^ hoverTrack)
    {
        if (!_NativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0 || this->_Measures == nullptr || this->_Measures->Count == 0 || !this->_ToolAccessDelegate) {
            return false;
        }

        // Save current clipping region
        D2D1_RECT_F ContentArea = D2D1::RectF(
            0,
            (float)GetLeftPanelAndTrackHeaderWidth(),
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        _NativeRenderer->PushAxisAlignedClip(ContentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try {
            // 1. Draw grid lines first (these should be visible through track content)
            float totalHeight = GetTotalTrackHeight();

            DrawGridLines(totalHeight);

            TimelineToolType currentTool = this->_ToolAccessDelegate->CurrentToolType();

            // 2. Draw track content (events and tablature)
            for each (Track ^ track in this->_Tracks)
            {
                System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(track);
                TrackContentBounds.Y += this->_ScrollPosition->Y;

                // Skip if track is not visible
                if (TrackContentBounds.Bottom < HEADER_HEIGHT || TrackContentBounds.Top > this->_Control->Height) {
                    continue;
                }

                // Draw events and tablature
                DrawTrackEvents(track, TrackContentBounds, currentTool);

                if (track->ShowTablature) {
                    DrawTrackTablature(track, TrackContentBounds);
                }
            }

            // 3. Draw track headers and borders
            // Commented due to use of batched drawing -> Manual call from Widget_Timeline->OnPaint(...)
            //DrawTrackHeaders();
            //DrawTrackDividers(hoverTrack);
        }
        finally {
            // Restore clipping region
            _NativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreview()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        // Save the original clip region
        D2D1_RECT_F ContentArea = D2D1::RectF(
			(float)GetLeftPanelAndTrackHeaderWidth(),
            HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        _NativeRenderer->PushAxisAlignedClip(ContentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try {
            TimelineToolType currentTool = this->_ToolAccessDelegate->CurrentToolType();

            // Get current tool
            switch (currentTool)
            {
            case TimelineToolType::Pointer:     DrawToolPreviewPointerTool();   break;
            case TimelineToolType::Draw:        DrawToolPreviewDrawTool();      break;
            case TimelineToolType::Split:       break;
            case TimelineToolType::Erase:       DrawToolPreviewEraseTool();     break;
            case TimelineToolType::Duration:    DrawToolPreviewDurationTool();  break;
            case TimelineToolType::Color:       DrawToolPreviewColorTool();     break;
            case TimelineToolType::Fade:        DrawToolPreviewFadeTool();      break;
            case TimelineToolType::Strobe:      DrawToolPreviewStrobeTool();    break;
            }
        }
        finally {
            _NativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }

    void Timeline_Direct2DRenderer::DrawFPSCounter(float x, double fps, double frameTimeMs)
    {
        if (!_NativeRenderer) {
            return;
        }

        const float Opacity = 0.8f;

        std::wstring FPSText = L"";
        if (frameTimeMs > 0) {
            FPSText = std::to_wstring((int)fps) + L" FPS";
        }
        else {
            FPSText = L"-- FPS"; // Display placeholder when no data
        }

        D2D1_COLOR_F TextColor;
        if (fps >= 55.0) {
            TextColor = D2D1::ColorF(0.2f, 1.0f, 0.2f, Opacity);
        }
        else if (fps >= 30.0) {
            TextColor = D2D1::ColorF(1.0f, 1.0f, 0.2f, Opacity);
        }
        else {
            TextColor = D2D1::ColorF(1.0f, 0.2f, 0.2f, Opacity);
        }

        D2D1_RECT_F TextRect = D2D1::RectF(x + 5.0f, (float)PANEL_BUTTON_MARGIN, x + 100.0f, (float)PANEL_BUTTON_MARGIN + 30.0f);
        _NativeRenderer->DrawText(FPSText, TextRect, TextColor, _NativeRenderer->GetFPSCounterFormat());
    }

	void Timeline_Direct2DRenderer::DrawLeftPanel(bool beingResized)
	{
		if (!_NativeRenderer) {
			return;
		}

		if (_Left_Panel->IsExpanded) {
			DrawLeftPanelExpanded(beingResized);
			DrawLeftPanelContent();
		}
		else {
			DrawLeftPanelHidden();
		}
	}

    bool Timeline_Direct2DRenderer::DrawBeatNumbers(Measure^ measure, float x, float measureNumberY, float subdivLevel, int measureNumber, int ticksPerBeat)
    {
        if (!_NativeRenderer) {
            return false;
        }

        // Create slightly transparent color for beat numbers
        D2D1_COLOR_F beatColor = COLOR_TO_COLOR_F(m_ColorTheme.Text);
        beatColor.a *= 0.7f; // 70% opacity

        // Calculate beat positions
        for (int beat = 1; beat < measure->Numerator; beat++)
        {
            float BeatX = x + TicksToPixels(beat * ticksPerBeat);

            // Only draw if beat marker is visible
            if (BeatX >= GetLeftPanelAndTrackHeaderWidth() && BeatX <= (float)this->_Control->Width)
            {
                // Create beat number text (e.g., "1.2" for measure 1, beat 2)
                std::wstring beatText = std::to_wstring(measureNumber) + L"." + std::to_wstring(beat + 1);

                // Define text rectangle
                D2D1_RECT_F beatRect = D2D1::RectF(
                    BeatX - 25,
                    measureNumberY + 4, // Offset slightly below measure numbers
                    BeatX + 25,
                    measureNumberY + 18
                );

                // Draw with smaller font size
                IDWriteTextFormat* quarterNoteFormat = _NativeRenderer->GetQuarterNoteFormat();
                _NativeRenderer->DrawText(beatText, beatRect, beatColor, quarterNoteFormat);

                // Draw tick mark
                _NativeRenderer->DrawLine(BeatX, HEADER_HEIGHT - 2, BeatX, HEADER_HEIGHT, beatColor, 1.0f);
            }
        }

        return true;
    }

    void Timeline_Direct2DRenderer::DrawGridLines(float totalHeight)
    {
        // Calculate visible range in ticks
        int StartTick   = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int EndTick     = (int)PixelsToTicks(-this->_ScrollPosition->X + this->_Control->Width - GetLeftPanelAndTrackHeaderWidth());

        // Draw grid lines in proper order (back to front)
        DrawSubdivisionLines(totalHeight, StartTick, EndTick);
        DrawBeatLines(totalHeight, StartTick, EndTick);
        DrawMeasureLines(totalHeight, StartTick, EndTick);
    }

    void Timeline_Direct2DRenderer::DrawSubdivisionLines(float totalHeight, int startTick, int endTick)
    {
        float SubdivLevel = GetSubdivisionLevel(TicksToPixels(TICKS_PER_QUARTER));
        
        if (SubdivLevel <= 1) {
            return;
        }

        int Accumulated = 0;
        for each (Measure^ Meas in this->_Measures)
        {
            int MeasureStart = Accumulated;
            int TicksPerBeat = TICKS_PER_QUARTER * 4 / Meas->Denominator;
            int TicksPerSubdiv = TicksPerBeat / (int)SubdivLevel;

            // Calculate subdivisions for this measure
            int Subdivisions = (Meas->Length / TicksPerSubdiv);

            for (int subdiv = 1; subdiv < Subdivisions; subdiv++)
            {
                int SubdivTick = MeasureStart + subdiv * TicksPerSubdiv;

                // Skip if this is already a beat or measure line
                if (SubdivTick % TicksPerBeat == 0) {
                    continue;
                }

                if (SubdivTick >= startTick && SubdivTick <= endTick)
                {
                    float X = TicksToPixels(SubdivTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth();
                    _NativeRenderer->DrawLine(X, HEADER_HEIGHT, X, HEADER_HEIGHT + totalHeight, COLOR_TO_RGBA(m_ColorTheme.SubdivisionLine), 1.0f);
                }
            }

            Accumulated += Meas->Length;
        }
    }

    void Timeline_Direct2DRenderer::DrawBeatLines(float totalHeight, int startTick, int endTick)
    {
        if (!m_LevelOfDetail.ShowBeatLines) {
            return;
        }

        int Accumulated = 0;

        for each (Measure^ Meas in this->_Measures)
        {
            int MeasureStart = Accumulated;
            int TicksPerBeat = TICKS_PER_QUARTER * 4 / Meas->Denominator;

            // Draw lines for each beat except measure start
            for (int beat = 1; beat < Meas->Numerator; beat++)
            {
                int beatTick = MeasureStart + beat * TicksPerBeat;

                if (beatTick >= startTick && beatTick <= endTick)
                {
                    float X = TicksToPixels(beatTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth();
                    _NativeRenderer->DrawLine(X, HEADER_HEIGHT, X, HEADER_HEIGHT + totalHeight, COLOR_TO_RGBA(m_ColorTheme.BeatLine), 1.0f);
                }
            }

            Accumulated += Meas->Length;
        }
    }

    void Timeline_Direct2DRenderer::DrawMeasureLines(float totalHeight, int startTick, int endTick)
    {
        int Accumulated = 0;
        int MeasureNumber = 1;

        for each (Measure^ Meas in this->_Measures)
        {
            if (Accumulated > endTick) {
                break;
            }

            bool ShouldDrawDetails = (MeasureNumber % m_LevelOfDetail.MeasureSkipFactor) == 0;

            if (ShouldDrawDetails && Accumulated + Meas->Length >= startTick)
            {
                float X = TicksToPixels(Accumulated) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth();

                if (X >= GetLeftPanelAndTrackHeaderWidth() && X <= (float)this->_Control->Width)
                {
                    _NativeRenderer->DrawLine(X, HEADER_HEIGHT, X, HEADER_HEIGHT + totalHeight, COLOR_TO_RGBA(m_ColorTheme.MeasureLine), 1.0f);
                }
            }

            Accumulated += Meas->Length;
            MeasureNumber++;
        }
    }

    void Timeline_Direct2DRenderer::DrawTrackEvents(Track^ track, System::Drawing::Rectangle trackContentBounds, TimelineToolType currentToolType)
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        // Get tool access interface
        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Calculate visible range in ticks
        int StartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int EndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

        // Track if this track has any selected bars
        bool HasSelectedBars	= ToolAccess->SelectedBars->Count > 0;
        bool IsDragging			= ToolAccess->IsDragging;
        bool IsResizing			= ToolAccess->IsResizing;

        for each (BarEvent^ Bar in track->Events)
        {
            if ((Bar->StartTick + Bar->Duration < StartTick) || (Bar->StartTick > EndTick)) {
                continue;
            }

            bool IsSelected	= HasSelectedBars && ToolAccess->SelectedBars->Contains(Bar);
            
			if (!IsSelected) {
				DrawNormalBar(Bar, trackContentBounds);
			}
			else if (IsSelected && !IsDragging && !IsResizing) {
				DrawSelectedBar(Bar, trackContentBounds);
			}
			else if (IsSelected && (IsDragging || IsResizing)) {
				DrawGhostBar(Bar, trackContentBounds);
			}
        }

        return;
    }

    bool Timeline_Direct2DRenderer::DrawTrackTablature(Track^ track, System::Drawing::Rectangle trackContentBounds)
    {
        if (!track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0) {
            return true;
        }

		float LogScale = TO_LOGSCALE(this->_ZoomLevel);

        if (track->IsDrumTrack && track->ShowAsStandardNotation)
        {
            return DrawTrackTablatureDrum(track, trackContentBounds, LogScale);
        }
        else
        {
            return DrawTrackTablatureRegular(track, trackContentBounds, LogScale);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackTablatureDrum(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale)
    {
        if (!_NativeRenderer) {
            return false;
        }

        float AvailableHeight = (float)(trackContentBounds.Height - TRACK_PADDING * 2);
        TabStringInfo String_Info = DrawTablatureStrings(trackContentBounds, AvailableHeight, logScale, 5);

        if (String_Info.TotalHeight > AvailableHeight) {
            return true;
        }

        try {
            // Calculate visible range
            int VisibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
            int VisibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

            // Calculate required space for Beat Duration drawing
            const float BASE_DURATION_SPACE = 23.0f;
            const float DURATION_SCALE_FACTOR = 20.0f;
            float RequiredSpace = BASE_DURATION_SPACE + (DURATION_SCALE_FACTOR * logScale);

            // Draw notes with symbols
            int MeasureStartTick = 0;

            for (int i = 0; i < track->Measures->Count; i++)
            {
                TrackMeasure^ Measure = track->Measures[i];
                if (Measure == nullptr) {
                    MeasureStartTick += Measure->Length;
                    continue;
                }

                int MeasureEndTick = MeasureStartTick + Measure->Length;

                // Skip if measure is out of visible range
                if (MeasureStartTick > VisibleEndTick || MeasureEndTick < VisibleStartTick)
                {
                    MeasureStartTick = MeasureEndTick;
                    continue;
                }

                for each(Beat^ beat in Measure->Beats)
                {
                    if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0) {
                        continue;
                    }

                    if (!ShouldRenderBeat(beat, Measure, m_LevelOfDetail.TabResolution)) {
                        continue;
                    }

                    float BeatX = (float)(TicksToPixels(beat->StartTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth());

                    if ((beat->Duration > 0) && (beat->Notes->Count > 0) && AvailableHeight > String_Info.TotalHeight + RequiredSpace) {
                        DrawBeatDuration(beat, trackContentBounds, String_Info.StringYPositions);
                    }

                    for each(Note^ note in beat->Notes)
                    {
                        DrumNoteInfo NoteInfo = DrumNotationMap::GetNoteInfo(note->Value);

                        // Calculate Y position (can be between lines)
                        float YPos;
                        int LineIndex = (int)Math::Floor(NoteInfo.StringPosition);
                        float Fraction = NoteInfo.StringPosition - LineIndex;

                        LineIndex -= 1;

                        if (Fraction == 0.0f) { // On the line
                            YPos = String_Info.StringYPositions[LineIndex];
                        }
                        else if (LineIndex < 0) {
                            YPos = String_Info.StringYPositions[0] - (String_Info.StringYPositions[1] - String_Info.StringYPositions[0]) * Fraction;
                        }
                        else { // Between lines
                            YPos = String_Info.StringYPositions[LineIndex] + (String_Info.StringYPositions[LineIndex + 1] - String_Info.StringYPositions[LineIndex]) * Fraction;
                        }

                        DrawDrumSymbol(NoteInfo.SymbolType, BeatX, YPos, ((String_Info.StringYPositions[1] - String_Info.StringYPositions[0]) / 2.0f) - 1);
                    }
                }
                MeasureStartTick += Measure->Length;
            }

            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool Timeline_Direct2DRenderer::DrawTrackTablatureRegular(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale)
    {
        if (!_NativeRenderer) {
            return false;
        }

		float ScaledFontSize = GetTablatureScaledFontSize(logScale);
        float AvailableHeight = (float)(trackContentBounds.Height - TRACK_PADDING * 2);

        TabStringInfo String_Info = DrawTablatureStrings(trackContentBounds, AvailableHeight, logScale, 6);

        if (String_Info.TotalHeight > AvailableHeight) {
            return true;
        }

        try
        {
            // Calculate visible tick range
            int VisibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
            int VisibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

            // Track measure position
            int MeasureStartTick = 0;

            // Process only visible measures
            for (int i = 0; i < track->Measures->Count; i++)
            {
                TrackMeasure^ Measure = track->Measures[i];
                if (Measure == nullptr) {
                    MeasureStartTick += Measure->Length;
                    continue;
                }

                int MeasureEndTick = MeasureStartTick + Measure->Length;

                // Skip if measure is out of visible range
                if (MeasureStartTick > VisibleEndTick || MeasureEndTick < VisibleStartTick)
                {
                    MeasureStartTick = MeasureEndTick;
                    continue;
                }

                // Draw beats in this measure
                for each (Beat^ beat in Measure->Beats)
                {
                    if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0) {
                        continue;
                    }

                    if (!ShouldRenderBeat(beat, Measure, m_LevelOfDetail.TabResolution)) {
                        continue;
                    }

                    int BeatTick = beat->StartTick;

                    // Skip if beat is outside visible range
                    if (BeatTick > VisibleEndTick || BeatTick + beat->Duration < VisibleStartTick) {
                        continue;
                    }

                    float BeatX = (float)(TicksToPixels(BeatTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth());

                    const float BASE_DURATION_SPACE = 23.0f;
                    const float DURATION_SCALE_FACTOR = 20.0f;
                    float RequiredSpace = BASE_DURATION_SPACE + (DURATION_SCALE_FACTOR * logScale);
                        
                    // Draw duration lines for beats with multiple notes
                    if ((beat->Duration > 0) && (beat->Notes->Count > 0) && AvailableHeight > String_Info.TotalHeight + RequiredSpace)
                    {
                        DrawBeatDuration(beat, trackContentBounds, String_Info.StringYPositions);
                    }

                    // Draw the notes
                    for each (Note^ note in beat->Notes)
                    {
                        if (note == nullptr || note->String < 1 || note->String > 6) {
                            continue;
                        }
					                        
						TabTextCacheEntry* CachedText = _NativeRenderer->GetNearestCachedTabTextEntry(ScaledFontSize);

						if (CachedText == nullptr) {
							return false;
						}

						float TextWidth = CachedText->TextWidths[note->Value];

						D2D1_RECT_F TextRect;
						TextRect.left	= BeatX - (TextWidth / 2.0f);
						TextRect.right	= BeatX + (TextWidth / 2.0f);
						TextRect.top	= String_Info.StringYPositions[note->String - 1] - (String_Info.StringSpacing / 2.0f);
						TextRect.bottom = TextRect.top + String_Info.StringSpacing;

						_NativeRenderer->DrawCachedTabText(note->Value, TextRect, ScaledFontSize);
                    }
                }

                MeasureStartTick = MeasureEndTick;
            }

            DrawTieLines(track, trackContentBounds, String_Info.StringYPositions, ScaledFontSize);

            return true;
        }
        catch (...) {
            return false;
        }
    }

    void Timeline_Direct2DRenderer::DrawBeatDuration(Beat^ beat, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions)
    {
        if (!_NativeRenderer || beat->Notes == nullptr || beat->Notes->Count == 0 || beat->Duration <= 0) {
            return;
        }

        // Scale base sizes logarithmically with limits
        const float BASE_STEM_LENGTH = 10.0f;
        const float BASE_LINE_LENGTH = 8.0f;
        const float BASE_STEM_OFFSET = 8.0f;

        DurationSymbolCacheEntry* CachedSymbol = _NativeRenderer->GetNearestCachedDurationSymbolEntry(_ZoomLevel);

        if (CachedSymbol == nullptr) {
            return;
        }

        // Calculate logarithmic scaling factor
        float LogScale = TO_LOGSCALE(this->_ZoomLevel);

        float ScaledStemLength = Math::Min(BASE_STEM_LENGTH + (LogScale * 6.0f), 35.0f);
        float ScaledLineLength = Math::Min(BASE_LINE_LENGTH + (LogScale * 2.0f), 15.0f);
        float ScaledStemOffset = Math::Min(BASE_STEM_OFFSET + (LogScale * 2.0f), 20.0f);

        // Get bottom string Y position
        float BottomStringY = stringYPositions[stringYPositions->Length - 1];

        // Calculate x position centered on the note text
        float NoteX = (float)(TicksToPixels(beat->StartTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth());

        int Duration = beat->Duration;
        if (beat->IsDotted) {
            Duration *= (3 / 2);
        }

        D2D1_RECT_F DurationRect;
        DurationRect.left = NoteX - ScaledLineLength;
        DurationRect.right = NoteX + ScaledLineLength;
        DurationRect.top = BottomStringY;
        DurationRect.bottom = BottomStringY + ScaledStemOffset + ScaledStemLength;

        _NativeRenderer->DrawCachedDudationSymbol(Duration, DurationRect, _ZoomLevel);
    }

    void Timeline_Direct2DRenderer::DrawTieLines(Track^ track, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions, float scaledFontSize)
    {
        if (!_NativeRenderer || !track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0 || !m_LevelOfDetail.ShowTieLines) {
            return;
        }

        // Calculate visible range
        int VisibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int VisibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

        // Create color for tie lines with transparency
        D2D1_COLOR_F TieColor = COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f);

        // Track measure position
        int MeasureStartTick = 0;

        try {
            // For each measure, find tied notes and draw connections
            for (int i = 0; i < track->Measures->Count; i++)
            {
                TrackMeasure^ Measure = track->Measures[i];
                if (Measure == nullptr) {
                    MeasureStartTick += Measure->Length;
                    continue;
                }

                int MeasureEndTick = MeasureStartTick + Measure->Length;

                // Skip if measure is completely outside visible range
                if (MeasureStartTick > VisibleEndTick || MeasureEndTick < VisibleStartTick) {
                    MeasureStartTick = MeasureEndTick;
                    continue;
                }

                // Process each beat to find tied notes
                for each (Beat^ CurrentBeat in Measure->Beats)
                {
                    if (CurrentBeat == nullptr || CurrentBeat->Notes == nullptr) {
                        continue;
                    }

                    // For each tied note in current beat, find its previous note
                    for each (Note^ CurrentNote in CurrentBeat->Notes)
                    {
                        if (!CurrentNote->IsTied) {
                            continue;
                        }

                        // Find the previous note with the same string and value
                        Beat^ PreviousBeat = nullptr;
                        Note^ PreviousNote = nullptr;

                        // Search in current measure first
                        for each (Beat^ CheckBeat in Measure->Beats) {
                            if (CheckBeat->StartTick >= CurrentBeat->StartTick) {
                                break;
                            }

                            // Look for matching note
                            for each (Note ^ CheckNote in CheckBeat->Notes) {
                                if (CheckNote->String == CurrentNote->String && CheckNote->Value == CurrentNote->Value)
                                {
                                    PreviousBeat = CheckBeat;
                                    PreviousNote = CheckNote;
                                }
                            }
                        }

                        // If not found and we're not in first measure, check previous measure
                        if (PreviousNote == nullptr && i > 0)
                        {
                            TrackMeasure^ PrevMeasure = track->Measures[i - 1];

                            if (PrevMeasure != nullptr && PrevMeasure->Beats != nullptr) 
                            {
                                for each (Beat^ CheckBeat in PrevMeasure->Beats) 
                                {
                                    for each (Note^ CheckNote in CheckBeat->Notes)
                                    {
                                        if (CheckNote->String == CurrentNote->String && CheckNote->Value == CurrentNote->Value)
                                        {
                                            PreviousBeat = CheckBeat;
                                            PreviousNote = CheckNote;
                                        }
                                    }
                                }
                            }
                        }

                        // Draw tie line if we found the previous note
                        if (PreviousBeat != nullptr && PreviousNote != nullptr)
                        {
                            // Calculate note text dimensions
                            std::wstring prevNoteText = std::to_wstring(PreviousNote->Value);
                            std::wstring currentNoteText = std::to_wstring(CurrentNote->Value);

                            // Get text format with appropriate size
                            IDWriteTextFormat* textFormat = _NativeRenderer->UpdateTablatureFormat(scaledFontSize);
                            if (!textFormat) {
                                continue;
                            }

                            float PrevNoteWidth = _NativeRenderer->MeasureTextWidth(prevNoteText, textFormat);
                            float CurrentNoteWidth = _NativeRenderer->MeasureTextWidth(currentNoteText, textFormat);

                            // Calculate x positions for both notes
                            float PrevNoteX = (float)(TicksToPixels(PreviousBeat->StartTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth());
                            float CurrentNoteX = (float)(TicksToPixels(CurrentBeat->StartTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth());

                            // Get y position for the string
                            float StringY = stringYPositions[CurrentNote->String - 1];

                            // Calculate start and end points
                            float StartX	= PrevNoteX + (PrevNoteWidth / 2);
                            float EndX		= CurrentNoteX - (CurrentNoteWidth / 2);
                            float StartY	= StringY + (scaledFontSize / 2);
                            float EndY		= StringY + (scaledFontSize / 2);

                            // Calculate control points for Bezier curve that dips below
                            float ControlHeight = 8.0f * (float)Math::Min(2.0f, Math::Max(0.5f, this->_ZoomLevel));

                            // Draw Bezier curve using Direct2D
                            _NativeRenderer->DrawTieLine(
                                D2D1::Point2F(StartX, StartY),											// Start point
                                D2D1::Point2F(StartX + (EndX - StartX) / 3, StartY + ControlHeight),	// First control point
                                D2D1::Point2F(StartX + (EndX - StartX) * 2 / 3, EndY + ControlHeight),	// Second control point
                                D2D1::Point2F(EndX, EndY),												// End point
                                TieColor,
                                1.5f  // Line thickness
                            );
                            
                        }
                    }
                }

                MeasureStartTick = MeasureEndTick;
            }
        }
        catch (...) {
            // Handle any exceptions
        }
    }

    void Timeline_Direct2DRenderer::DrawDrumSymbol(DrumNotationType symbolType, float x, float y, float size)
    {
        if (!_NativeRenderer) {
            return;
        }

		D2D1_RECT_F DestRect = D2D1::RectF(
			x - size,	// Center horizontally
			y - size,	// Center vertically
			x + size,
			y + size
		);

		// Draw using the cached bitmap
		_NativeRenderer->DrawCachedDrumSymbol((int)symbolType, DestRect, size);
    }

    TabStringInfo Timeline_Direct2DRenderer::DrawTablatureStrings(System::Drawing::Rectangle bounds, float availableHeight, float logScale, int numStrings)
    {
        TabStringInfo Info;

		float ScaledStringSpacing = GetTablatureScaledStringSpacing(logScale);

        float Total_Tab_Height = ScaledStringSpacing * (numStrings - 1);
        float VerticalOffset = bounds.Y + TRACK_PADDING + (availableHeight - Total_Tab_Height) / 2;

        // Store calculated values
        Info.StringSpacing = ScaledStringSpacing;
        Info.TotalHeight = Total_Tab_Height;
        Info.VerticalOffset = VerticalOffset;
        Info.StringYPositions = gcnew array<float>(numStrings);

        if (Total_Tab_Height < availableHeight)
        {
            for (int i = 0; i < numStrings; i++) {
                Info.StringYPositions[i] = VerticalOffset + (i * ScaledStringSpacing);
                _NativeRenderer->DrawLine((float)bounds.X, Info.StringYPositions[i], (float)bounds.Right, Info.StringYPositions[i], COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f), 1.0f);
            }
        }

        return Info;
    }

	float Timeline_Direct2DRenderer::GetTablatureScaledFontSize(float logScale)
	{
		// Calculate zoom-based scaling factors
		const float BASE_FONT_SIZE = 9.0f;
		const float FONT_SCALE_FACTOR = 2.0f;

		float scaledFontSize = BASE_FONT_SIZE + (logScale * FONT_SCALE_FACTOR);

		// Clamp font size between min and max values
		return Math::Min(Math::Max(scaledFontSize, 4.0f), 18.0f);
	}

	float Timeline_Direct2DRenderer::GetTablatureScaledStringSpacing(float logScale)
	{
		// Calculate spacing and dimensions
		const float BASE_STRING_SPACING = 10.0f;
		const float SPACING_SCALE_FACTOR = 3.0f;

		float ScaledStringSpacing = BASE_STRING_SPACING + (logScale * SPACING_SCALE_FACTOR);

		return Math::Min(Math::Max(ScaledStringSpacing, 12.0f), 40.0f);
	}

    bool Timeline_Direct2DRenderer::ShouldRenderBeat(Beat^ beat, Measure^ measure, TablatureResolution resolution)
    {
        if (resolution == TablatureResolution::FullDetail) {
            return true;
        }

        // Calculate beat position within measure
        int MeasureStartTick = measure->StartTick;
        int BeatPositionInMeasure = beat->StartTick - MeasureStartTick;

        // Calculate ticks per beat for this measure
        int TicksPerBeat = TICKS_PER_QUARTER * 4 / measure->Denominator;

        // Calculate total beats in measure
        int TotalBeats = measure->Numerator;

        switch (resolution)
        {
            case TablatureResolution::BeatLevel:
                // Render only on beat boundaries
                return (BeatPositionInMeasure % TicksPerBeat) == 0;

            case TablatureResolution::HalfMeasure:
            {
                // For odd time signatures, show first beat and middle beat
                if (TotalBeats % 2 == 1)
                {
                    int MiddleBeat = TotalBeats / 2;
                    int BeatNumber = BeatPositionInMeasure / TicksPerBeat;
                    return BeatNumber == 0 || BeatNumber == MiddleBeat;
                }
                // For even time signatures, show beats at half-measure intervals
                else
                {
                    return (BeatPositionInMeasure % (TicksPerBeat * (TotalBeats / 2))) == 0;
                }
        }

        case TablatureResolution::MeasureLevel:
            // Show only first beat of each measure
            return BeatPositionInMeasure == 0;

        case TablatureResolution::FullDetail:
            return true;

        default:
            return true;
        }
    }

    bool Timeline_Direct2DRenderer::DrawTrackHeaders()
    {
        if (!_NativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0 || this->_Measures->Count == 0) {
            return false;
        }

        float TotalHeight = GetTotalTrackHeight();
		float X_Offset = (float)GetLeftPanelWidth();

        // Fill header background
        D2D1_RECT_F HeaderBackground = D2D1::RectF(
			X_Offset,						// Left
            HEADER_HEIGHT,					// Top
			X_Offset + TRACK_HEADER_WIDTH,	// Right
            TotalHeight						// Bottom
        );

        _NativeRenderer->FillRectangle(HeaderBackground, COLOR_TO_COLOR_F(m_ColorTheme.HeaderBackground));

        // Draw each track's header
        int CurrentY = HEADER_HEIGHT;

        for each(Track^ Trk in this->_Tracks)
        {
            // Calculate track bounds
            int TrackHeight = Trk->Height;
            int yPosition = CurrentY + this->_ScrollPosition->Y;

            System::Drawing::Rectangle TrackHeaderBounds = System::Drawing::Rectangle((int)X_Offset, yPosition, TRACK_HEADER_WIDTH, TrackHeight);

            // Draw header background (with selection highlight if track is selected)
            Color BGColor = Trk->IsSelected ? m_ColorTheme.SelectionHighlight : m_ColorTheme.HeaderBackground;
            _NativeRenderer->FillRectangle(RECT_TO_RECT_F(TrackHeaderBounds), COLOR_TO_COLOR_F(BGColor));


            // Draw track name if present
            if (!String::IsNullOrEmpty(Trk->Name))
            {
                // Calculate text bounds with padding
                float TextPadding = TRACK_PADDING;
                D2D1_RECT_F TextBounds = D2D1::RectF(TrackHeaderBounds.Left + TextPadding, TrackHeaderBounds.Top + TextPadding, TrackHeaderBounds.Right - TextPadding, TrackHeaderBounds.Bottom - TextPadding);

                // Draw the track name using the measure number format (for now)
                std::wstring TrackName = ConvertString(Trk->Name);
                _NativeRenderer->DrawText(TrackName, TextBounds, COLOR_TO_COLOR_F(m_ColorTheme.Text), _NativeRenderer->GetTrackHeaderFormat());
            }

            // Draw track buttons
            DrawTrackButtons(Trk, TrackHeaderBounds);

            // Draw borders
            _NativeRenderer->DrawRectangle(RECT_TO_RECT_F(TrackHeaderBounds), COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);
            //m_pNativeRenderer->DrawLine(TRACK_HEADER_WIDTH, headerBounds.top, TRACK_HEADER_WIDTH, headerBounds.bottom, COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);

            CurrentY += TrackHeight;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackButtons(Track^ track, System::Drawing::Rectangle trackHeaderBounds)
    {
        TrackButtonId HoveredButton = this->_ToolAccessDelegate->HoverButton();

        DrawTrackButtonText(trackHeaderBounds, 0, "T", track->ShowTablature, track == HoveredButton.Track && HoveredButton.ButtonIndex == 0, m_ColorTheme.HeaderBackground, m_ColorTheme.Text);

        if (track->IsDrumTrack)
        {
            System::Drawing::Image^ Image_Note = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Note_White")));
            DrawTrackButtonIcon(trackHeaderBounds, 1, Image_Note, track->ShowAsStandardNotation, track == HoveredButton.Track && HoveredButton.ButtonIndex == 1, m_ColorTheme.HeaderBackground, m_ColorTheme.Text);
        }
        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackButtonText(System::Drawing::Rectangle trackHeaderBounds, int buttonIndex, System::String^ text, bool isPressed, bool isHovered, Color baseColor, Color textColor)
    {
        // Calculate button position based on index (right to left)
        System::Drawing::Rectangle ButtonBounds = GetTrackButtonBounds(buttonIndex, trackHeaderBounds);
        
        DrawTrackButtonBase(ButtonBounds, isPressed, isHovered, baseColor);

        std::wstring ButtonText = ConvertString(text);
        _NativeRenderer->DrawText(ButtonText, RECT_TO_RECT_F(ButtonBounds), COLOR_TO_COLOR_F(m_ColorTheme.Text), _NativeRenderer->GetMeasureNumberFormat());

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackButtonIcon(System::Drawing::Rectangle trackHeaderBounds, int buttonIndex, System::Drawing::Image^ icon, bool isPressed, bool isHovered, Color baseColor, Color textColor)
    {
        // Calculate button position based on index(right to left)
        System::Drawing::Rectangle ButtonBounds = GetTrackButtonBounds(buttonIndex, trackHeaderBounds);

        DrawTrackButtonBase(ButtonBounds, isPressed, isHovered, baseColor);
        
        float Padding = 4.0f;
        D2D1_RECT_F IconRect = D2D1::RectF(
            ButtonBounds.X + Padding,
            ButtonBounds.Y + Padding,
            ButtonBounds.Right - Padding,
            ButtonBounds.Bottom - Padding
        );

        _NativeRenderer->DrawCachedBitmap(L"Note_White", IconRect);

        return true;
    }

    void Timeline_Direct2DRenderer::DrawTrackButtonBase(System::Drawing::Rectangle buttonBounds, bool isPressed, bool isHovered, System::Drawing::Color baseColor)
    {
        // Create rounded rectangle geometry
        const float CornerRadius = 6.0f;
        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(RECT_TO_RECT_F(buttonBounds), CornerRadius, CornerRadius);

        // Determine button color based on state
        D2D1_COLOR_F buttonColor;
        if (isHovered) {
            // Lighter for hover
            buttonColor = D2D1::ColorF(
                Math::Min(1.0f, baseColor.R / 255.0f + 0.1f),
                Math::Min(1.0f, baseColor.G / 255.0f + 0.1f),
                Math::Min(1.0f, baseColor.B / 255.0f + 0.1f),
                1.0f
            );
        }
        else if (isPressed) {
            // Darker for pressed
            buttonColor = D2D1::ColorF(
                Math::Min(1.0f, baseColor.R / 255.0f + 0.2f),
                Math::Min(1.0f, baseColor.G / 255.0f + 0.2f),
                Math::Min(1.0f, baseColor.B / 255.0f + 0.2f),
                1.0f
            );
        }
        else {
            buttonColor = COLOR_TO_COLOR_F(baseColor);
        }

        // Fill and stroke the rounded rectangle
        _NativeRenderer->FillRoundedRectangle(roundedRect, buttonColor);

        // Draw border with slight transparency
        _NativeRenderer->DrawRoundedRectangle(roundedRect, COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.3f), 1.0f);
    }

    bool Timeline_Direct2DRenderer::DrawTrackDividers(Track^ hoverTrack)
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate || this->_Tracks == nullptr || this->_Tracks->Count == 0 || this->_Measures->Count == 0) {
            return false;
        }

		// Get tool access interface
		ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
		if (!ToolAccess) {
			return false;
		}

        float CurrentY = HEADER_HEIGHT;

        // Draw dividers between tracks
        for each (Track^ T in this->_Tracks)
        {
            // Calculate divider Y position
            float DividerY = CurrentY + T->Height + this->_ScrollPosition->Y;

            // Only draw if divider is in visible range
            if (DividerY >= 0 && DividerY <= (float)this->_Control->Height)
            {
                // Draw differently if this is the hover track
                if (T == hoverTrack && !ToolAccess->IsDragging && !ToolAccess->IsSelecting)
                {
                    // Draw a highlighted divider across the full width
                    _NativeRenderer->DrawLine(
                        0.0f, DividerY,
                        (float)this->_Control->Width, DividerY,
                        COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight),
                        2.0f  // Thicker line for hover state
                    );
                }
                else
                {
                    // Draw normal divider
                    _NativeRenderer->DrawLine(0.0f, DividerY, (float)this->_Control->Width, DividerY, COLOR_TO_COLOR_F(m_ColorTheme.MeasureLine), 1.0f);
                }
            }

            // Draw vertical divider between header and content
            float headerBottom = CurrentY + T->Height;
            if (headerBottom >= this->_ScrollPosition->Y && CurrentY <= this->_ScrollPosition->Y + (float)this->_Control->Height)
            {
                _NativeRenderer->DrawLine((float)GetLeftPanelAndTrackHeaderWidth(), CurrentY, (float)GetLeftPanelAndTrackHeaderWidth(), headerBottom, COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);
            }

            CurrentY += T->Height;
        }

        return true;
    }

    void Timeline_Direct2DRenderer::DrawToolPreviewPointerTool()
    {
        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        if(ToolAccess->IsDragging && ToolAccess->SelectedBars->Count > 0) {
            DrawToolPreviewPointerToolMoving();
        }
		else if (ToolAccess->IsResizing && ToolAccess->SelectedBars->Count > 0) {
			DrawToolPreviewPointerToolResizing();
		}
		else if (ToolAccess->IsPasting && ToolAccess->PastePreviewBars != nullptr && ToolAccess->PastePreviewBars->Count > 0) {
			DrawToolPreviewPointerToolPasting();
		}

		// Draw selection rectangle if present
		DrawSelectionRectangle(ToolAccess->SelectionRect);
    }

    void Timeline_Direct2DRenderer::DrawToolPreviewPointerToolMoving()
    {
        if (!_NativeRenderer) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Calculate visible range in ticks
        int VisibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int VisibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + this->_Control->Width);

		for each (BarEvent^ Bar in ToolAccess->PreviewBars)
		{
			if (Bar->EndTick < VisibleStartTick || Bar->StartTick > VisibleEndTick) {
				continue;
			}

			System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Bar->ContainingTrack);
			TrackContentBounds.Y += this->_ScrollPosition->Y;

			DrawSelectedBar(Bar, TrackContentBounds);
		}
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewPointerToolResizing()
	{
		if (!_NativeRenderer) {
			return;
		}

		ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
		if (!ToolAccess) {
			return;
		}

		// Calculate visible range in ticks
		int VisibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
		int VisibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + this->_Control->Width);

		for each (BarEvent^ Event in ToolAccess->PreviewBars)
		{
			if (Event->EndTick < VisibleStartTick || Event->StartTick > VisibleEndTick) {
				continue;
			}

			System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Event->ContainingTrack);
			TrackContentBounds.Y += this->_ScrollPosition->Y;

			DrawSelectedBar(Event, TrackContentBounds);
			DrawResizeHandle(GetBarBounds(Event, TrackContentBounds), Event == ToolAccess->HoveredBar);
		}

		for each (BarEvent^ Event in ToolAccess->SelectedBars)
		{
			Track^ ContainingTrack = Event->ContainingTrack;

			if (ContainingTrack != nullptr)
			{
				System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(ContainingTrack);
				TrackContentBounds.Y += this->_ScrollPosition->Y;

				System::Drawing::Rectangle EventBounds = GetBarBounds(Event, TrackContentBounds);

				DrawBarGlowEffect(EventBounds, System::Drawing::Color::White, 1);
			}
		}
	}

	void Timeline_Direct2DRenderer::DrawToolPreviewPointerToolPasting()
	{
		if (!_NativeRenderer) {
			return;
		}

		ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
		if (!ToolAccess) {
			return;
		}

		if (ToolAccess->CurrentMousePosition.X > GetLeftPanelAndTrackHeaderWidth())
		{
			for each (BarEvent^ PasteBar in ToolAccess->PastePreviewBars)
			{
				System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(PasteBar->ContainingTrack);
				TrackContentBounds.Y += this->_ScrollPosition->Y;

				DrawPastePreviewBar(PasteBar, TrackContentBounds);
			}
		}
	}

    void Timeline_Direct2DRenderer::DrawToolPreviewDrawTool()
    {
        if (!_NativeRenderer) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Handle different draw tool modes
        switch (ToolAccess->CurrentMode)
        {
        case DrawToolMode::Draw:
            DrawToolPreviewDrawToolDraw();
            break;
        case DrawToolMode::Erase:
            DrawToolPreviewDrawToolErase();
            break;
        case DrawToolMode::Move:
            DrawToolPreviewDrawToolMove();
            break;
        case DrawToolMode::Resize:
            DrawToolPreviewDrawToolResize();
            break;
        }
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewDrawToolDraw()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Draw preview bar if one exists
        if (ToolAccess->PreviewBars->Count > 0)
        {
            DrawPreviewBar(ToolAccess->PreviewBars[0], nullptr, ToolAccess->CurrentMousePosition, BarPreviewType::Creation);
        }
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewDrawToolErase()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

		BarEvent^ HoveredBar = nullptr;
		List<BarEvent^>^ SelectedBars = ToolAccess->SelectedBars;
		if (SelectedBars->Count > 0) {
			HoveredBar = SelectedBars[0];
		}
		else {
			return;
		}

        // Get track containing hover bar
        Track^ ContainingTrack = HoveredBar->ContainingTrack;

        if (ContainingTrack != nullptr)
        {
            System::Drawing::Rectangle bounds = GetTrackContentBounds(ContainingTrack);
            bounds.Y += this->_ScrollPosition->Y;

            System::Drawing::Rectangle BarBounds = GetBarBounds(HoveredBar, bounds);

            // Draw delete preview with semi-transparent red
            D2D1_RECT_F DeleteRect = RECT_TO_RECT_F(BarBounds);
            D2D1_COLOR_F FillColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.4f);  // Red with 40% opacity
            D2D1_COLOR_F BorderColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% opacity

            // Fill rectangle
            _NativeRenderer->FillRectangle(DeleteRect, FillColor);

            // Draw border
            _NativeRenderer->DrawRectangle(DeleteRect, BorderColor, 2.0f);

            // Draw X
            _NativeRenderer->DrawLine((float)BarBounds.Left, (float)BarBounds.Top, (float)BarBounds.Right, (float)BarBounds.Bottom, BorderColor, 2.0f);
            _NativeRenderer->DrawLine((float)BarBounds.Left, (float)BarBounds.Bottom, (float)BarBounds.Right, (float)BarBounds.Top, BorderColor, 2.0f);
        }
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewDrawToolMove()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        List<BarEvent^>^ PreviewBars = ToolAccess->PreviewBars;

		if (PreviewBars->Count > 0)
		{
			System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(PreviewBars[0]->ContainingTrack);
			TrackContentBounds.Y += this->_ScrollPosition->Y;

			DrawSelectedBar(PreviewBars[0], TrackContentBounds);
		}
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewDrawToolResize()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

		List<BarEvent^>^ PreviewBars = ToolAccess->PreviewBars;
		if (PreviewBars->Count == 0 || !ToolAccess->IsResizing) {
			return;
		}

        // Get track containing hover bar
        Track^ ContainingTrack = PreviewBars[0]->ContainingTrack;
		if (ContainingTrack == nullptr) {
			return;
		}

		System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(ContainingTrack);
		TrackContentBounds.Y += this->_ScrollPosition->Y;

		//DrawGhostBar(HoveredBar, TrackContentBounds);
		DrawPreviewBar(PreviewBars[0], ContainingTrack, Point(), BarPreviewType::Duration);

		// Draw Resize Handle when Draw Tool is in Resize Mode
		DrawResizeHandle(GetBarBounds(PreviewBars[0], TrackContentBounds), true);
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewEraseTool()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Get the EraseTool's data
        List<BarEvent^>^ SelectedBars = ToolAccess->SelectedBars;
        BarEvent^ HoveredBar = ToolAccess->HoveredBar;

        // Check if we're hovering over a selected bar
        bool IsHoveringSelected = (HoveredBar != nullptr && SelectedBars->Contains(HoveredBar));

		if(SelectedBars->Count > 0)
		{
			// Draw mark deleted for all selected bars
			for each (BarEvent^ Bar in SelectedBars)
			{
				if (Bar->ContainingTrack != nullptr)
				{
					System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Bar->ContainingTrack);
					TrackContentBounds.Y += this->_ScrollPosition->Y;

					System::Drawing::Rectangle BarBounds = GetBarBounds(Bar, TrackContentBounds);

					DrawToolPreviewEraseToolDrawCross(BarBounds, IsHoveringSelected);
				}
			}
		}
		else if (HoveredBar != nullptr)
		{
			System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(HoveredBar->ContainingTrack);
			TrackContentBounds.Y += this->_ScrollPosition->Y;

			System::Drawing::Rectangle BarBounds = GetBarBounds(HoveredBar, TrackContentBounds);

			DrawToolPreviewEraseToolDrawCross(BarBounds, true);
		}

		// Draw selection rectangle if present
		DrawSelectionRectangle(ToolAccess->SelectionRect);
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewEraseToolDrawCross(System::Drawing::Rectangle barBounds, bool isHovered)
	{
		// Create D2D rect
		D2D1_RECT_F DeleteRect = RECT_TO_RECT_F(barBounds);

		// Fill with light red
		D2D1_COLOR_F FillColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.4f);  // Red with 40% opacity
		_NativeRenderer->FillRectangle(DeleteRect, FillColor);

		// If hovering over any selected bar, show delete preview for all selected bars
		if (isHovered)
		{
			// Draw border and X with darker red
			D2D1_COLOR_F BorderColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% opacity

			// Draw border
			_NativeRenderer->DrawRectangle(DeleteRect, BorderColor, 2.0f);

			// Draw X
			_NativeRenderer->DrawLine((float)barBounds.Left, (float)barBounds.Top, (float)barBounds.Right, (float)barBounds.Bottom, BorderColor, 2.0f);
			_NativeRenderer->DrawLine((float)barBounds.Left, (float)barBounds.Bottom, (float)barBounds.Right, (float)barBounds.Top, BorderColor, 2.0f);
		}
	}

	void Timeline_Direct2DRenderer::DrawToolPreviewDurationTool()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Handle preview if tool is active
		/*
        if (ToolAccess->PreviewBar != nullptr) {
            // Draw duration preview overlay
            DrawPreviewBar(ToolAccess->PreviewBar, ToolAccess->PreviewBar->ContainingTrack, Point(), BarPreviewType::Duration);
        }
		*/

        // Draw preview bars with enhanced visualization
        for each (BarEvent^ Event in ToolAccess->PreviewBars)
        {
            Track^ ContainingTrack = Event->ContainingTrack;

            if (ContainingTrack != nullptr)
            {
                System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(ContainingTrack);
                TrackContentBounds.Y += this->_ScrollPosition->Y;
                
                DrawPreviewBar(Event, ContainingTrack, Point(), BarPreviewType::Duration);
                DrawResizeHandle(GetBarBounds(Event, TrackContentBounds), Event == ToolAccess->HoveredBar);
            }
        }

		for each(BarEvent ^ Event in ToolAccess->SelectedBars)
		{
			Track^ ContainingTrack = Event->ContainingTrack;

			if (ContainingTrack != nullptr)
			{
				System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(ContainingTrack);
				TrackContentBounds.Y += this->_ScrollPosition->Y;

				System::Drawing::Rectangle EventBounds = GetBarBounds(Event, TrackContentBounds);

				DrawBarGlowEffect(EventBounds, System::Drawing::Color::White, 1);
			}
		}

        // Draw selection rectangle if active
        DrawSelectionRectangle(ToolAccess->SelectionRect);
    }

	void Timeline_Direct2DRenderer::DrawToolPreviewColorTool()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        // Get the ColorTool's specific data
        List<BarEvent^>^	SelectedBars	= ToolAccess->SelectedBars;
        BarEvent^			HoveredBar		= ToolAccess->HoveredBar;
        Color				CurrentColor	= ToolAccess->CurrentColor;

		float BarXHoverRatio = ToolAccess->BarXHoverRatio;

		if (SelectedBars->Count > 0)
		{
			// Draw all selected bars with a highlight
			for each (BarEvent ^ Bar in SelectedBars)
			{
				Track^ TargetTrack = Bar->ContainingTrack;

				if (TargetTrack != nullptr)
				{
					System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(TargetTrack);
					TrackContentBounds.Y += this->_ScrollPosition->Y;

					System::Drawing::Rectangle BarBounds = GetBarBounds(Bar, TrackContentBounds);
					D2D1_RECT_F BarRect = RECT_TO_RECT_F(BarBounds);

					DrawColorPreview(Bar, BarBounds, CurrentColor, 0.4f, 2.0f);

					// If this bar is being hovered over, show stronger highlight
					if (Bar == HoveredBar)
					{
						// Draw glow effect
						for (int i = 2; i >= 0; i--)
						{
							D2D1_RECT_F GlowRect = D2D1::RectF(
								BarRect.left - i,
								BarRect.top - i,
								BarRect.right + i,
								BarRect.bottom + i
							);

							D2D1_COLOR_F GlowColor = COLOR_TO_COLOR_F_A(CurrentColor, 0.3f - (i * 0.1f));
							_NativeRenderer->DrawRectangle(GlowRect, GlowColor, 1.0f);
						}
					}
				}
			}
		}
		else if (HoveredBar != nullptr)
		{
			System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(HoveredBar->ContainingTrack);
			TrackContentBounds.Y += this->_ScrollPosition->Y;

			System::Drawing::Rectangle BarBounds = GetBarBounds(HoveredBar, TrackContentBounds);

			DrawColorPreview(HoveredBar, BarBounds, CurrentColor, 0.3f, 1.0f);
		}

		// Draw selection rectangle if present
		DrawSelectionRectangle(ToolAccess->SelectionRect);
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewFadeTool()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return false;
        }

        List<BarEvent^>^	PreviewBars = ToolAccess->PreviewBars;
        System::Drawing::Point CurrentMousePos = ToolAccess->CurrentMousePosition;

        // Save current clip region
        D2D1_RECT_F ContentArea = D2D1::RectF(
			(float)GetLeftPanelAndTrackHeaderWidth(),
            HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        _NativeRenderer->PushAxisAlignedClip(ContentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try
        {
            if (PreviewBars->Count > 0)
            {
                // Draw single preview bar with creation effect
                DrawPreviewBar(PreviewBars[0], nullptr, CurrentMousePos, BarPreviewType::Creation);
            }
        }
        finally
        {
            // Restore clip region
            _NativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewStrobeTool()
    {
        if (!_NativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return false;
        }

        List<BarEvent^>^	PreviewBars = ToolAccess->PreviewBars;
        System::Drawing::Point CurrentMousePos = ToolAccess->CurrentMousePosition;

        // Save current clip region
        D2D1_RECT_F contentArea = D2D1::RectF(
			(float)GetLeftPanelAndTrackHeaderWidth(),
            HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        _NativeRenderer->PushAxisAlignedClip(contentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try
        {
            if (PreviewBars->Count > 0)
            {
                // Draw single preview bar with creation effect
                DrawPreviewBar(PreviewBars[0], nullptr, CurrentMousePos, BarPreviewType::Creation);
            }
        }
        finally
        {
            // Restore clip region
            _NativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }

	void Timeline_Direct2DRenderer::DrawLeftPanelExpanded(bool beingResized)
	{
		// Calculate panel dimensions - full height of the widget
		float Height = Math::Max(GetTotalTrackHeight(), (float)this->_Control->Height);

		// Create the rectangle for the panel area
		D2D1_RECT_F PanelArea = D2D1::RectF(
			0,							// Left
			HEADER_HEIGHT,				// Top
			(float)_Left_Panel->Width,	// Right
			Height						// Bottom
		);

		// Fill the panel rectangle with a color slightly darker than the header background
		D2D1_COLOR_F PanelColor = COLOR_TO_COLOR_F(m_ColorTheme.TrackBackground);
		// Make it slightly darker
		//PanelColor.r = Math::Max(0.0f, PanelColor.r - 0.05f);
		//PanelColor.g = Math::Max(0.0f, PanelColor.g - 0.05f);
		//PanelColor.b = Math::Max(0.0f, PanelColor.b - 0.05f);

		_NativeRenderer->FillRectangle(PanelArea, PanelColor);
		_NativeRenderer->DrawRectangle(PanelArea, COLOR_TO_COLOR_F(m_ColorTheme.MeasureLine), 2.0f);

		float HandleWidth = 3.0f;
		D2D1_RECT_F HandleRect = D2D1::RectF(this->_Left_Panel->Width - HandleWidth, HEADER_HEIGHT, (float)_Left_Panel->Width, Height);
		D2D1_COLOR_F HandleColor = COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBorder, 0.5f);

		if(beingResized) {
			HandleColor = COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight);
		}

		_NativeRenderer->FillRectangle(HandleRect, HandleColor);

		// Draw collapse/expand button at the top-right corner of the panel
		float ButtonX = (float)(_Left_Panel->Width - PANEL_BUTTON_SIZE - PANEL_BUTTON_MARGIN);
		DrawLeftPanelToggleButton(ButtonX, (float)PANEL_BUTTON_MARGIN, !this->_Left_Panel->IsExpanded);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelContent()
	{
		if (!_NativeRenderer || !_Left_Panel) {
			return;
		}

		const float SECTION_PADDING = 10.0f;
		const float ROW_HEIGHT = 20.0f;
		const float TITLE_HEIGHT = 26.0f;
		const float TITLE_PADDING = 12.0f;
		const float SECTION_MARGIN = 12.0f;
		const float SECTION_HEADER_HEIGHT = 22.0f;

		float Y = HEADER_HEIGHT + SECTION_PADDING;
		float panel_width = (float)_Left_Panel->Width;

		// Draw title with background
		D2D1_RECT_F TitleBackRect = D2D1::RectF(
			SECTION_PADDING,                 // Left
			Y,                               // Top
			panel_width - SECTION_PADDING,   // Right
			Y + TITLE_HEIGHT                 // Bottom
		);

		// Create rounded rectangle for title background
		D2D1_ROUNDED_RECT RoundedTitleRect = D2D1::RoundedRect(
			TitleBackRect,
			5.0f,  // Corner radius
			5.0f
		);

		// Draw title background
		D2D1_COLOR_F TitleBgColor = COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.4f);
		_NativeRenderer->FillRoundedRectangle(RoundedTitleRect, TitleBgColor);
		_NativeRenderer->DrawRoundedRectangle(RoundedTitleRect, COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.6f), 1.0f);

		// Title text rect
		D2D1_RECT_F TitleTextRect = D2D1::RectF(
			TitleBackRect.left + TITLE_PADDING,   // Left (with extra padding)
			TitleBackRect.top,                    // Top
			TitleBackRect.right - TITLE_PADDING,  // Right
			TitleBackRect.bottom                  // Bottom
		);

		std::wstring TitleText = L"Event Properties";
		_NativeRenderer->DrawText(TitleText, TitleTextRect, COLOR_TO_COLOR_F(m_ColorTheme.Text), _NativeRenderer->GetLeftPanelTitleFormat());

		Y += TITLE_HEIGHT + SECTION_MARGIN;

		if (_Left_Panel->SelectedEvent == nullptr &&
			(_Left_Panel->SelectedEvents == nullptr || _Left_Panel->SelectedEvents->Count == 0)) {
			// No events selected
			D2D1_RECT_F NoSelectionRect = D2D1::RectF(
				SECTION_PADDING,               // Left
				Y,                             // Top
				panel_width - SECTION_PADDING, // Right
				Y + ROW_HEIGHT                 // Bottom
			);

			std::wstring NoSelectionText = L"No events selected";
			D2D1_COLOR_F GrayTextColor = D2D1::ColorF(0.7f, 0.7f, 0.7f, 1.0f);
			_NativeRenderer->DrawText(NoSelectionText, NoSelectionRect, GrayTextColor, _NativeRenderer->GetLeftPanelTextFormat());

			return;
		}

		// Multiple events selected
		if (_Left_Panel->SelectedEvents->Count > 1) {
			D2D1_RECT_F MultiSelectionRect = D2D1::RectF(
				SECTION_PADDING,               // Left
				Y,                             // Top
				panel_width - SECTION_PADDING, // Right
				Y + ROW_HEIGHT                 // Bottom
			);

			std::wstring MultiSelectionText = L"Multiple events selected (" +
				std::to_wstring(_Left_Panel->SelectedEvents->Count) + L")";
			_NativeRenderer->DrawText(MultiSelectionText, MultiSelectionRect, COLOR_TO_COLOR_F(m_ColorTheme.Text), _NativeRenderer->GetLeftPanelTextFormat());

			Y += ROW_HEIGHT + TITLE_PADDING;

			// We could add common properties editing for multiple selections here
			DrawLeftPanelMultipleEventProperties(Y);
			return;
		}

		// Single event selected - draw event properties
		BarEvent^ event = _Left_Panel->SelectedEvent;
		if (event != nullptr) {
			// Draw section header for general properties
			DrawLeftPanelSectionHeader(L"General", Y);
			Y += SECTION_HEADER_HEIGHT + SECTION_PADDING;

			// Draw general properties
			DrawLeftPanelEventProperties(event, Y);
		}
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelHidden()
	{
		DrawLeftPanelToggleButton((float)PANEL_BUTTON_MARGIN, (float)PANEL_BUTTON_MARGIN, !this->_Left_Panel->IsExpanded);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelToggleButton(float x, float y, bool isCollapsed)
	{
		const float CornerRadius = 5.0f;

		D2D1_RECT_F ButtonRect = D2D1::RectF(
			(float)PANEL_BUTTON_MARGIN,
			(float)PANEL_BUTTON_MARGIN,
			(float)PANEL_BUTTON_MARGIN + PANEL_BUTTON_SIZE,
			(float)PANEL_BUTTON_MARGIN + PANEL_BUTTON_SIZE
		);

		D2D1_ROUNDED_RECT RoundedRect = D2D1::RoundedRect(ButtonRect, CornerRadius, CornerRadius);
		D2D1_COLOR_F FillColor = COLOR_TO_COLOR_F(m_ColorTheme.Background);

		// Draw button background with appropriate color based on state
		if (!isCollapsed) {
			FillColor = COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.8f);
		}

		// Draw button background
		_NativeRenderer->FillRoundedRectangle(RoundedRect, FillColor);

        if (isCollapsed) {
            _NativeRenderer->DrawRoundedRectangle(RoundedRect, COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.8f), 1.0f);
        }

		const float BarWidth	= PANEL_BUTTON_SIZE * 0.4f;
		const float BarHeight	= 3.0f;
		const float BarDist		= 5.0f;

		for (int i = 0 ; i < 3 ; i++)
		{
			D2D1_RECT_F BarRect = D2D1::RectF(
				(float)PANEL_BUTTON_MARGIN + PANEL_BUTTON_SIZE / 2 - BarWidth / 2,
				(float)PANEL_BUTTON_MARGIN + PANEL_BUTTON_SIZE / 2 + (i - 1) * BarDist + BarHeight / 2,
				(float)PANEL_BUTTON_MARGIN + PANEL_BUTTON_SIZE / 2 + BarWidth / 2,
				(float)PANEL_BUTTON_MARGIN + PANEL_BUTTON_SIZE / 2 + (i - 1) * BarDist - BarHeight / 2
			);
			_NativeRenderer->FillRectangle(BarRect, COLOR_TO_COLOR_F(m_ColorTheme.Text));
			
		}
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelEventProperties(BarEvent^ event, float startY)
	{
		if (!_NativeRenderer || event == nullptr) {
			return;
		}

		const float SECTION_PADDING = 10.0f;
		const float ROW_HEIGHT = 20.0f;
		const float ROW_SPACING = 6.0f;
		const float SECTION_MARGIN = 14.0f;
		const float SECTION_HEADER_HEIGHT = 22.0f;

		float Y = startY;

		// Common Properties
		DrawLeftPanelPropertyRow(L"Type:", GetEventTypeText(event->Type), Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		DrawLeftPanelPropertyRow(L"Start:", FormatTickPosition(event->StartTick), Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		DrawLeftPanelPropertyRow(L"End:", FormatTickPosition(event->EndTick), Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		DrawLeftPanelPropertyRow(L"Duration:", FormatTickDuration(event->Duration), Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		// Track name
		if (event->ContainingTrack != nullptr) {
			std::wstring trackName = ConvertString(event->ContainingTrack->Name);
			DrawLeftPanelPropertyRow(L"Track:", trackName, Y);
			Y += ROW_HEIGHT + ROW_SPACING;
		}

		// Add space after general properties
		Y += SECTION_MARGIN;

		// Type-specific properties
		switch (event->Type) {
		case BarEventType::Solid:
			DrawLeftPanelSectionHeader(L"Solid Properties", Y);
			Y += SECTION_HEADER_HEIGHT + SECTION_PADDING;
			DrawLeftPanelSolidEventProperties(event, Y);
			break;
		case BarEventType::Fade:
			DrawLeftPanelSectionHeader(L"Fade Properties", Y);
			Y += SECTION_HEADER_HEIGHT + SECTION_PADDING;
			DrawLeftPanelFadeEventProperties(event, Y);
			break;
		case BarEventType::Strobe:
			DrawLeftPanelSectionHeader(L"Strobe Properties", Y);
			Y += SECTION_HEADER_HEIGHT + SECTION_PADDING;
			DrawLeftPanelStrobeEventProperties(event, Y);
			break;
		}
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelSectionHeader(const std::wstring& headerText, float Y)
	{
		const float SECTION_PADDING = 10.0f;
		const float SECTION_HEADER_HEIGHT = 22.0f;
		const float SECTION_HEADER_PADDING = 8.0f;
		float panel_width = (float)_Left_Panel->Width;

		// Draw header background with gradient and bottom border
		D2D1_RECT_F HeaderRect = D2D1::RectF(
			SECTION_PADDING,               // Left
			Y,                             // Top
			panel_width - SECTION_PADDING, // Right
			Y + SECTION_HEADER_HEIGHT      // Bottom
		);

		// Draw gradient background
		D2D1_COLOR_F GradientStart = COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBackground, 0.7f);
		D2D1_COLOR_F GradientEnd = COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBorder, 0.2f);
		_NativeRenderer->FillRectangleGradient2(HeaderRect, GradientStart, GradientEnd);

		// Draw bottom border line
		float BorderY = Y + SECTION_HEADER_HEIGHT - 1.0f;
		_NativeRenderer->DrawLine(
			SECTION_PADDING, BorderY,
			panel_width - SECTION_PADDING, BorderY,
			COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBorder, 0.5f), 1.0f);

		// Draw header text
		D2D1_RECT_F TextRect = D2D1::RectF(
			HeaderRect.left + SECTION_HEADER_PADDING,   // Left
			HeaderRect.top,                             // Top
			HeaderRect.right - SECTION_HEADER_PADDING,  // Right
			HeaderRect.bottom                           // Bottom
		);

		_NativeRenderer->DrawText(
			headerText,
			TextRect,
			COLOR_TO_COLOR_F(m_ColorTheme.Text),
			_NativeRenderer->GetLeftPanelSectionFormat()
		);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelPropertyRow(const std::wstring& label, const std::wstring& value, float Y)
	{
		const float SECTION_PADDING = 10.0f;
		const float ROW_HEIGHT = 20.0f;
		const float VALUE_CONTAINER_WIDTH = 120.0f;
		const float CONTAINER_PADDING = 8.0f;
		const float CONTAINER_CORNER_RADIUS = 4.0f;
		float panel_width = (float)_Left_Panel->Width;

		// Label - left aligned
		D2D1_RECT_F LabelRect = D2D1::RectF(
			SECTION_PADDING,                     // Left
			Y,                                   // Top
			panel_width - VALUE_CONTAINER_WIDTH - SECTION_PADDING, // Right
			Y + ROW_HEIGHT                       // Bottom
		);

		// Value container - right aligned
		D2D1_RECT_F ContainerRect = D2D1::RectF(
			panel_width - VALUE_CONTAINER_WIDTH - SECTION_PADDING, // Left
			Y + (ROW_HEIGHT - ROW_HEIGHT * 0.8f) / 2,              // Top (slightly smaller than row height)
			panel_width - SECTION_PADDING,                         // Right
			Y + (ROW_HEIGHT - ROW_HEIGHT * 0.8f) / 2 + ROW_HEIGHT * 0.8f // Bottom
		);

		// Create the rounded rectangle for the container
		D2D1_ROUNDED_RECT RoundedContainerRect = D2D1::RoundedRect(
			ContainerRect,
			CONTAINER_CORNER_RADIUS,
			CONTAINER_CORNER_RADIUS
		);

		// Value text rect (inside container, with padding)
		D2D1_RECT_F ValueRect = D2D1::RectF(
			ContainerRect.left + CONTAINER_PADDING,    // Left
			ContainerRect.top,                         // Top (same as container)
			ContainerRect.right - CONTAINER_PADDING,   // Right
			ContainerRect.bottom                       // Bottom (same as container)
		);

		// Draw label
		_NativeRenderer->DrawText(label, LabelRect, COLOR_TO_COLOR_F(m_ColorTheme.Text), _NativeRenderer->GetLeftPanelTextFormat());

		// Draw container background
		D2D1_COLOR_F ContainerColor = D2D1::ColorF(
			m_ColorTheme.TrackBorder.R / 255.0f * 0.5f,  // Darker version of border color
			m_ColorTheme.TrackBorder.G / 255.0f * 0.5f,
			m_ColorTheme.TrackBorder.B / 255.0f * 0.5f,
			0.7f  // Semi-transparent
		);

		_NativeRenderer->FillRoundedRectangle(RoundedContainerRect, ContainerColor);

		// Draw container border
		_NativeRenderer->DrawRoundedRectangle(
			RoundedContainerRect,
			COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBorder, 0.4f),
			1.0f
		);

		// Draw text value inside container
		_NativeRenderer->DrawText(
			value,
			ValueRect,
			COLOR_TO_COLOR_F(m_ColorTheme.Text),
			_NativeRenderer->GetMeasureNumberFormat()
		);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelColorProperty(const std::wstring& label, System::Drawing::Color color, float Y)
	{
		const float SECTION_PADDING			= 10.0f;
		const float ROW_HEIGHT				= 20.0f;
		const float VALUE_CONTAINER_WIDTH	= 120.0f;
		const float CONTAINER_PADDING		= 8.0f;
		const float CONTAINER_CORNER_RADIUS = 4.0f;
		const float COLOR_SWATCH_SIZE		= 14.0f;
		const float COLOR_SWATCH_MARGIN		= 4.0f;
		float panel_width = (float)_Left_Panel->Width;

		// Label
		D2D1_RECT_F LabelRect = D2D1::RectF(
			SECTION_PADDING,                                       // Left
			Y,                                                     // Top
			panel_width - VALUE_CONTAINER_WIDTH - SECTION_PADDING, // Right
			Y + ROW_HEIGHT                                         // Bottom
		);

		// Value container - right aligned
		D2D1_RECT_F ContainerRect = D2D1::RectF(
			panel_width - VALUE_CONTAINER_WIDTH - SECTION_PADDING, // Left
			Y + (ROW_HEIGHT - ROW_HEIGHT * 0.8f) / 2,              // Top (slightly smaller than row height)
			panel_width - SECTION_PADDING,                         // Right
			Y + (ROW_HEIGHT - ROW_HEIGHT * 0.8f) / 2 + ROW_HEIGHT * 0.8f // Bottom
		);

		// Create the rounded rectangle for the container
		D2D1_ROUNDED_RECT RoundedContainerRect = D2D1::RoundedRect(
			ContainerRect,
			CONTAINER_CORNER_RADIUS,
			CONTAINER_CORNER_RADIUS
		);

		// Color swatch (inside container)
		float circleRadius = COLOR_SWATCH_SIZE / 2.0f;
		float circleCenterX = ContainerRect.left + CONTAINER_PADDING + circleRadius;
		float circleCenterY = ContainerRect.top + (ContainerRect.bottom - ContainerRect.top) / 2.0f;

		// Create ellipse for the swatch
		D2D1_ELLIPSE ColorSwatchEllipse = D2D1::Ellipse(
			D2D1::Point2F(circleCenterX, circleCenterY),
			circleRadius,
			circleRadius
		);

		// Color swatch (inside container)
		D2D1_RECT_F SwatchRect = D2D1::RectF(
			ContainerRect.left + CONTAINER_PADDING,                  // Left
			ContainerRect.top + (ContainerRect.bottom - ContainerRect.top - COLOR_SWATCH_SIZE) / 2, // Top (centered)
			ContainerRect.left + CONTAINER_PADDING + COLOR_SWATCH_SIZE, // Right
			ContainerRect.top + (ContainerRect.bottom - ContainerRect.top - COLOR_SWATCH_SIZE) / 2 + COLOR_SWATCH_SIZE // Bottom
		);

		// Color value text rect (inside container, next to swatch)
		D2D1_RECT_F ColorTextRect = D2D1::RectF(
			SwatchRect.right + COLOR_SWATCH_MARGIN,                 // Left
			ContainerRect.top,                                      // Top
			ContainerRect.right - CONTAINER_PADDING,                // Right
			ContainerRect.bottom                                    // Bottom
		);

		// Draw label
		_NativeRenderer->DrawText(
			label,
			LabelRect,
			COLOR_TO_COLOR_F(m_ColorTheme.Text),
			_NativeRenderer->GetLeftPanelTextFormat()
		);

		// Draw container background
		D2D1_COLOR_F ContainerColor = D2D1::ColorF(
			m_ColorTheme.TrackBorder.R / 255.0f * 0.5f,  // Darker version of border color
			m_ColorTheme.TrackBorder.G / 255.0f * 0.5f,
			m_ColorTheme.TrackBorder.B / 255.0f * 0.5f,
			0.7f  // Semi-transparent
		);

		_NativeRenderer->FillRoundedRectangle(RoundedContainerRect, ContainerColor);

		// Draw container border
		_NativeRenderer->DrawRoundedRectangle(
			RoundedContainerRect,
			COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBorder, 0.4f),
			1.0f
		);

		// Draw color swatch
		_NativeRenderer->FillEllipse(ColorSwatchEllipse, COLOR_TO_COLOR_F(color));
		_NativeRenderer->DrawEllipse(ColorSwatchEllipse, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.5f), 1.0f);

		// Draw color value text
		std::wstring colorText = L"#" +
			ToHexString(color.R) +
			ToHexString(color.G) +
			ToHexString(color.B);

		_NativeRenderer->DrawText(
			colorText,
			ColorTextRect,
			COLOR_TO_COLOR_F(m_ColorTheme.Text),
			_NativeRenderer->GetMeasureNumberFormat()
		);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelSolidEventProperties(BarEvent^ event, float startY)
	{
		float Y = startY;

		// Draw color property
		DrawLeftPanelColorProperty(L"Color:", event->Color, Y);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelFadeEventProperties(BarEvent^ event, float startY)
	{
		if (event->FadeInfo == nullptr)
			return;

		const float ROW_HEIGHT = 20.0f;
		const float ROW_SPACING = 4.0f;

		float Y = startY;

		// Fade type
		std::wstring fadeTypeText = (event->FadeInfo->Type == FadeType::Two_Colors) ? L"Two Colors" : L"Three Colors";
		DrawLeftPanelPropertyRow(L"Fade Type:", fadeTypeText, Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		// Quantization
		std::wstring quantText;
		// Try to get the friendly name from TimeSignatureLookup
		System::String^ friendlyName;
		if (TimeSignatures::TimeSignatureLookup->TryGetValue(event->FadeInfo->QuantizationTicks, friendlyName)) {
			quantText = ConvertString(friendlyName);
		}
		else {
			// Fallback to just the tick value if not found
			quantText = std::to_wstring(event->FadeInfo->QuantizationTicks);
		}
		DrawLeftPanelPropertyRow(L"Quantization:", quantText, Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		// Easing types
		std::wstring easeInText = GetEasingText(event->FadeInfo->EaseIn);
		DrawLeftPanelPropertyRow(L"Ease In:", easeInText, Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		std::wstring easeOutText = GetEasingText(event->FadeInfo->EaseOut);
		DrawLeftPanelPropertyRow(L"Ease Out:", easeOutText, Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		// Colors
		DrawLeftPanelColorProperty(L"Start Color:", event->FadeInfo->ColorStart, Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		if (event->FadeInfo->Type == FadeType::Three_Colors) {
			DrawLeftPanelColorProperty(L"Center Color:", event->FadeInfo->ColorCenter, Y);
			Y += ROW_HEIGHT + ROW_SPACING;
		}

		DrawLeftPanelColorProperty(L"End Color:", event->FadeInfo->ColorEnd, Y);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelStrobeEventProperties(BarEvent^ event, float startY)
	{
		if (event->StrobeInfo == nullptr)
			return;

		const float ROW_HEIGHT = 20.0f;
		const float ROW_SPACING = 4.0f;

		float Y = startY;

		// Quantization
		std::wstring quantText;
		// Try to get the friendly name from TimeSignatureLookup
		System::String^ friendlyName;
		if (TimeSignatures::TimeSignatureLookup->TryGetValue(event->StrobeInfo->QuantizationTicks, friendlyName)) {
			quantText = ConvertString(friendlyName);
		}
		else {
			// Fallback to just the tick value if not found
			quantText = std::to_wstring(event->StrobeInfo->QuantizationTicks);
		}
		DrawLeftPanelPropertyRow(L"Quantization:", quantText, Y);
		Y += ROW_HEIGHT + ROW_SPACING;

		// Color
		DrawLeftPanelColorProperty(L"Strobe Color:", event->StrobeInfo->ColorStrobe, Y);
	}

	void Timeline_Direct2DRenderer::DrawLeftPanelMultipleEventProperties(float startY)
	{
		// To be implemented for multi-selection properties
		// This would show common properties across all selected events
		// For now we just show a message
		const float SECTION_PADDING = 10.0f;
		const float ROW_HEIGHT = 20.0f;
		float panel_width = (float)_Left_Panel->Width;

		D2D1_RECT_F TextRect = D2D1::RectF(
			SECTION_PADDING,               // Left
			startY,                        // Top
			panel_width - SECTION_PADDING, // Right
			startY + ROW_HEIGHT            // Bottom
		);

		std::wstring Text = L"Common properties editing";
		D2D1_COLOR_F GrayTextColor = D2D1::ColorF(0.7f, 0.7f, 0.7f, 1.0f);
		_NativeRenderer->DrawText(Text, TextRect, GrayTextColor, _NativeRenderer->GetLeftPanelTextFormat());
	}

	std::wstring Timeline_Direct2DRenderer::GetEventTypeText(BarEventType type)
	{
		switch (type) {
			case BarEventType::Solid:	return L"Solid";
			case BarEventType::Fade:	return L"Fade";
			case BarEventType::Strobe:	return L"Strobe";
			default:					return L"Unknown";
		}
	}

	std::wstring Timeline_Direct2DRenderer::GetEasingText(Easing easing)
	{
		// Use the existing lookup dictionary from ContextMenuStrings
		for each (System::Collections::Generic::KeyValuePair<System::String^, FadeEasing> kvp in ContextMenuStrings::FadeEasings)
		{
			if (kvp.Value == easing) {
				return ConvertString(kvp.Key);
			}
		}
		return L"Unknown"; // Fallback if not found
	}

	std::wstring Timeline_Direct2DRenderer::FormatTickPosition(int tick)
	{
		// Format: MMMMM.QQ.EE.FF
		// MMMMM = Measure number (5 digits with leading zeros)
		// QQ = Quarter notes (2 digits with leading zeros)
		// EE = Eighth notes (2 digits with leading zeros)
		// FF = Fraction of eighth note (0-100, where 100 = full eighth note)

		// Find the measure containing this tick
		int currentTick = 0;
		int measureNumber = 1; // Measures are 1-based
		Measure^ containingMeasure = nullptr;

		for each (Measure ^ m in this->_Measures)
		{
			if (tick >= currentTick && tick < currentTick + m->Length) {
				containingMeasure = m;
				break;
			}
			currentTick += m->Length;
			measureNumber++;
		}

		// If we can't find the containing measure, format as zeros
		if (containingMeasure == nullptr) {
			return L"00000.00.00.00";
		}

		// Calculate ticks within the measure
		int ticksInMeasure = tick - currentTick;

		// Calculate full quarter notes
		int ticksPerQuarter = Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		int quarterNotes = ticksInMeasure / ticksPerQuarter;
		int remainderAfterQuarters = ticksInMeasure % ticksPerQuarter;

		// Calculate full eighth notes
		int ticksPerEighth = ticksPerQuarter / 2;
		int eighthNotes = remainderAfterQuarters / ticksPerEighth;
		int remainderAfterEighths = remainderAfterQuarters % ticksPerEighth;

		// Calculate fraction of eighth note (0-100)
		int fraction = (remainderAfterEighths * 100) / ticksPerEighth;

		// Format the components with leading zeros
		std::wstring formattedMeasure = std::to_wstring(measureNumber);
		formattedMeasure = std::wstring(5 - std::min(5, (int)formattedMeasure.length()), L'0') + formattedMeasure.substr(0, 5);

		std::wstring formattedQuarters = std::to_wstring(quarterNotes);
		formattedQuarters = std::wstring(2 - std::min(2, (int)formattedQuarters.length()), L'0') + formattedQuarters.substr(0, 2);

		std::wstring formattedEighths = std::to_wstring(eighthNotes);
		formattedEighths = std::wstring(2 - std::min(2, (int)formattedEighths.length()), L'0') + formattedEighths.substr(0, 2);

		std::wstring formattedFraction = std::to_wstring(fraction);
		formattedFraction = std::wstring(2 - std::min(2, (int)formattedFraction.length()), L'0') + formattedFraction.substr(0, 2);

		// Combine all components
		return formattedMeasure + L"." + formattedQuarters + L"." + formattedEighths + L"." + formattedFraction;
	}

	std::wstring Timeline_Direct2DRenderer::FormatTickDuration(int durationTicks)
	{
		// Format duration in MMMMM.QQ.EE.FF format
		// For durations, we consider MMMMM as "full measures" which may span multiple actual measures

		// Get ticks per full 4/4 measure
		int ticksPerQuarter = Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		int ticksPerFullMeasure = ticksPerQuarter * 4; // Assuming 4/4 for duration display

		// Calculate components
		int fullMeasures = durationTicks / ticksPerFullMeasure;
		int remainderAfterMeasures = durationTicks % ticksPerFullMeasure;

		int quarterNotes = remainderAfterMeasures / ticksPerQuarter;
		int remainderAfterQuarters = remainderAfterMeasures % ticksPerQuarter;

		int ticksPerEighth = ticksPerQuarter / 2;
		int eighthNotes = remainderAfterQuarters / ticksPerEighth;
		int remainderAfterEighths = remainderAfterQuarters % ticksPerEighth;

		// Calculate fraction of eighth note (0-100)
		int fraction = (remainderAfterEighths * 100) / ticksPerEighth;

		// Format the components with leading zeros
		std::wstring formattedMeasures = std::to_wstring(fullMeasures);
		formattedMeasures = std::wstring(5 - std::min(5, (int)formattedMeasures.length()), L'0') + formattedMeasures.substr(0, 5);

		std::wstring formattedQuarters = std::to_wstring(quarterNotes);
		formattedQuarters = std::wstring(2 - std::min(2, (int)formattedQuarters.length()), L'0') + formattedQuarters.substr(0, 2);

		std::wstring formattedEighths = std::to_wstring(eighthNotes);
		formattedEighths = std::wstring(2 - std::min(2, (int)formattedEighths.length()), L'0') + formattedEighths.substr(0, 2);

		std::wstring formattedFraction = std::to_wstring(fraction);
		formattedFraction = std::wstring(2 - std::min(2, (int)formattedFraction.length()), L'0') + formattedFraction.substr(0, 2);

		// Combine all components
		return formattedMeasures + L"." + formattedQuarters + L"." + formattedEighths + L"." + formattedFraction;
	}

	void Timeline_Direct2DRenderer::DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds)
	{
		if (!_NativeRenderer || !this->_ToolAccessDelegate) {
			return;
		}
		
		System::Drawing::Rectangle BarBounds = GetBarBounds(bar, trackContentBounds);

		switch (bar->Type)
		{
			case BarEventType::Solid:	DrawNormalBarSolid	(bar, BarBounds); break;
			case BarEventType::Fade:	DrawNormalBarFade	(bar, BarBounds); break;
			case BarEventType::Strobe:	DrawNormalBarStrobe	(bar, BarBounds); break;
		}

		// Add tool-specific enhancements
		DrawToolEnhancements(bar, BarBounds);
	}

	void Timeline_Direct2DRenderer::DrawNormalBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);

        _NativeRenderer->FillRectangle(BarRect, COLOR_TO_COLOR_F_A(bar->Color, 0.8f));        // Normal appearance with 80% opacity
        _NativeRenderer->DrawRectangle(BarRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.4f), 1.0f);  // Draw border with slight transparency
    }

	void Timeline_Direct2DRenderer::DrawNormalBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);
		
		if (bar->FadeInfo == nullptr) {
			return;
		}

		if(bar->FadeInfo->Type == FadeType::Two_Colors) {
			// Normal appearance with 80% opacity
			_NativeRenderer->FillRectangleGradient2(BarRect,
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorStart, 0.8f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorEnd, 0.8f));
		}
		else if (bar->FadeInfo->Type == FadeType::Three_Colors) {
			// Normal appearance with 80% opacity
			_NativeRenderer->FillRectangleGradient3(BarRect,
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorStart, 0.8f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorCenter, 0.8f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorEnd, 0.8f));
		}

        
        std::wstring NoteText = ConvertString(TimeSignatures::TimeSignatureLookup[bar->FadeInfo->QuantizationTicks]);

        const float Padding = 4.0f;
        D2D1_RECT_F TextRect = D2D1::RectF(
            BarRect.left + Padding,     // Left edge of text area
            BarRect.top + Padding,      // Top edge of text area
            BarRect.left + 40.0f,       // Right edge of text area
            BarRect.top + 20.0f         // Bottom edge of text area
        );

        // Draw text with semi-transparent white color
        D2D1_COLOR_F TextColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.7f);
        _NativeRenderer->DrawText(NoteText, TextRect, TextColor, _NativeRenderer->GetMeasureNumberFormat());

        bool ShouldDrawCurves = (barBounds.Width >= MIN_WIDTH_FOR_CURVES) &&
                                (this->_ZoomLevel >= MIN_ZOOM_FOR_CURVES) &&
                                (m_LevelOfDetail.TabResolution <= TablatureResolution::BeatLevel);

        if (ShouldDrawCurves)
        {
            // Draw easing curve indicators
            const float CurveHeight = barBounds.Height * 0.45f; // 45% of bar height

            // Draw ease-in curve
            std::vector<D2D1_POINT_2F> EaseInPoints;
            std::vector<D2D1_POINT_2F> EaseOutPoints;
            const int NumPoints = 20;
            float EaseInWidth = barBounds.Width * 0.5f; // 50% of bar width
            float EaseOutOffset = EaseInWidth;

            float Y_In  = (float)(barBounds.Top + barBounds.Height / 2 + CurveHeight);
            float Y_Out = (float)(barBounds.Top + barBounds.Height / 2);

            for (int i = 0; i <= NumPoints; i++)
            {
                float Ratio = (float)i / NumPoints;
                float X = barBounds.X + (Ratio * EaseInWidth);

                float EasedTIn  = Easings::ApplyEasing(Ratio, bar->FadeInfo->EaseIn);
                float EasedTOut = Easings::ApplyEasing(Ratio, bar->FadeInfo->EaseOut);

                EaseInPoints.push_back(D2D1::Point2F(X, Y_In - CurveHeight * EasedTIn));
                EaseOutPoints.push_back(D2D1::Point2F(X + EaseOutOffset, Y_Out - CurveHeight * EasedTOut));
            }

            // Draw the curves
            D2D1_COLOR_F curveColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.5f); // Semi-transparent white
            float CurveThickness = 2.0f;

            // Draw points as connected lines
            for (size_t i = 1; i < EaseInPoints.size(); i++)
            {
                _NativeRenderer->DrawLine(EaseInPoints[i - 1].x, EaseInPoints[i - 1].y, EaseInPoints[i].x, EaseInPoints[i].y, curveColor, CurveThickness);
                _NativeRenderer->DrawLine(EaseOutPoints[i - 1].x, EaseOutPoints[i - 1].y, EaseOutPoints[i].x, EaseOutPoints[i].y, curveColor, CurveThickness);
            }
        }
	}

	void Timeline_Direct2DRenderer::DrawNormalBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
        D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);

        float StrobeWidthPixels = TicksToPixels(bar->StrobeInfo->QuantizationTicks);

        // Normal appearance with 80% opacity
        _NativeRenderer->FillRectangleStripes(BarRect, COLOR_TO_COLOR_F_A(bar->Color, 0.8f), StrobeWidthPixels);    // Normal appearance with 80% opacity
        _NativeRenderer->DrawRectangle(BarRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.4f), 1.0f);                        // Draw border with slight transparency
	}

    void Timeline_Direct2DRenderer::DrawPreviewBar(BarEvent^ bar, Track^ track, System::Drawing::Point mousePos, BarPreviewType previewType)
    {
        // For Creation/Movement previews, we need to determine the track
        Track^ TargetTrack = track == nullptr ? GetTrackAtPoint(mousePos) : track;

        if (bar != nullptr && TargetTrack != nullptr)
        {
            // Get the track bounds for drawing
            System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(TargetTrack);
            TrackContentBounds.Y += this->_ScrollPosition->Y;

            System::Drawing::Rectangle BarBounds = GetBarBounds(bar, TrackContentBounds);

            // Different visual treatments based on preview type
            switch (previewType)
            {
            case BarPreviewType::Creation:
            case BarPreviewType::Movement:  DrawCreationMovementPreview(bar, BarBounds);    break;

            case BarPreviewType::Duration:  DrawDurationPreview(bar, BarBounds);            break;
            }
        }
    }

    void Timeline_Direct2DRenderer::DrawPreviewBarList(List<BarEvent^>^ bars, Track^ track)
    {
        if (bars == nullptr || track == nullptr) {
            return;
        }

        for each (BarEvent^ bar in bars)
        {
            // Get track bounds
            System::Drawing::Rectangle bounds = GetTrackContentBounds(track);
            bounds.Y += this->_ScrollPosition->Y;

            DrawPreviewBar(bar, track, Point(), BarPreviewType::Creation);
        }
    }

    void Timeline_Direct2DRenderer::DrawCreationMovementPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        switch (bar->Type)
        {
        case BarEventType::Solid:	DrawCreationMovementPreviewSolid(bar, barBounds); break;
        case BarEventType::Fade:	DrawCreationMovementPreviewFade(bar, barBounds); break;
        case BarEventType::Strobe:	DrawCreationMovementPreviewStrobe(bar, barBounds); break;
        }
        
        // Add subtle glow effect
        DrawBarGlowEffect(barBounds, Color::FromArgb(100, bar->Color), 2);
    }

    void Timeline_Direct2DRenderer::DrawCreationMovementPreviewSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        // Semi-transparent preview with border
        _NativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(bar->Color));
        _NativeRenderer->DrawRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(bar->Color, 0.7f), 1.0f);
    }

    void Timeline_Direct2DRenderer::DrawCreationMovementPreviewFade(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);

        if (bar->FadeInfo == nullptr) {
            return;
        }

        // Draw preview with enhanced brightness
        if (bar->FadeInfo->Type == FadeType::Two_Colors) {
            _NativeRenderer->FillRectangleGradient2(BarRect, COLOR_TO_COLOR_F(bar->FadeInfo->ColorStart), COLOR_TO_COLOR_F(bar->FadeInfo->ColorEnd));
        }
        else if (bar->FadeInfo->Type == FadeType::Three_Colors) {
            _NativeRenderer->FillRectangleGradient3(BarRect, COLOR_TO_COLOR_F(bar->FadeInfo->ColorStart), COLOR_TO_COLOR_F(bar->FadeInfo->ColorCenter), COLOR_TO_COLOR_F(bar->FadeInfo->ColorEnd));
        }
    }

    void Timeline_Direct2DRenderer::DrawCreationMovementPreviewStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        if (bar->StrobeInfo == nullptr) {
            return;
        }
        
        float StrobeWidthPixels = TicksToPixels(bar->StrobeInfo->QuantizationTicks);

        _NativeRenderer->FillRectangleStripes(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(bar->Color), StrobeWidthPixels);
    }

	void Timeline_Direct2DRenderer::DrawDurationPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
		switch (bar->Type)
		{
			case BarEventType::Solid:	DrawDurationPreviewSolid(bar, barBounds); break;
			case BarEventType::Fade:	DrawDurationPreviewFade(bar, barBounds); break;
			case BarEventType::Strobe:	DrawDurationPreviewStrobe(bar, barBounds); break;
		}
		
        // Add glow effect for the preview
        DrawBarGlowEffect(barBounds, this->m_ColorTheme.SelectionHighlight, 3);
    }

	void Timeline_Direct2DRenderer::DrawDurationPreviewSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		// Draw preview with enhanced brightness
		Color PreviewColor = Color::FromArgb(
			Math::Min(255, bar->Color.R + 10),
			Math::Min(255, bar->Color.G + 10),
			Math::Min(255, bar->Color.B + 10)
		);

		_NativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(PreviewColor));
	}

	void Timeline_Direct2DRenderer::DrawDurationPreviewFade(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);
		
		if (bar->FadeInfo == nullptr) {
			return;
		}

		// Draw preview with enhanced brightness
		Color ColorStart = Color::FromArgb(
			Math::Min(255, bar->FadeInfo->ColorStart.R + 10),
			Math::Min(255, bar->FadeInfo->ColorStart.G + 10),
			Math::Min(255, bar->FadeInfo->ColorStart.B + 10)
		);

		Color ColorEnd = Color::FromArgb(
			Math::Min(255, bar->FadeInfo->ColorEnd.R + 10),
			Math::Min(255, bar->FadeInfo->ColorEnd.G + 10),
			Math::Min(255, bar->FadeInfo->ColorEnd.B + 10)
		);

		if (bar->FadeInfo->Type == FadeType::Two_Colors) {
			_NativeRenderer->FillRectangleGradient2(BarRect,
													COLOR_TO_COLOR_F(ColorStart),
													COLOR_TO_COLOR_F(ColorEnd));
		}
		else if (bar->FadeInfo->Type == FadeType::Three_Colors) {
			Color ColorCenter = Color::FromArgb(
				Math::Min(255, bar->FadeInfo->ColorCenter.R + 10),
				Math::Min(255, bar->FadeInfo->ColorCenter.G + 10),
				Math::Min(255, bar->FadeInfo->ColorCenter.B + 10)
			);
			
			// Normal appearance with 80% opacity
			_NativeRenderer->FillRectangleGradient3(BarRect,
													COLOR_TO_COLOR_F(ColorStart),
													COLOR_TO_COLOR_F(ColorCenter),
													COLOR_TO_COLOR_F(ColorEnd));
		}
	}

	void Timeline_Direct2DRenderer::DrawDurationPreviewStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
        // Draw preview with enhanced brightness
        Color PreviewColor = Color::FromArgb(
            Math::Min(255, bar->Color.R + 10),
            Math::Min(255, bar->Color.G + 10),
            Math::Min(255, bar->Color.B + 10)
        );

        float StrobeWidthPixels = TicksToPixels(bar->StrobeInfo->QuantizationTicks);

        _NativeRenderer->FillRectangleStripes(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(PreviewColor), StrobeWidthPixels);
	}

    void Timeline_Direct2DRenderer::DrawColorPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds, System::Drawing::Color currentColor, float opacity, float borderWidth)
    {
        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return;
        }

        if (bar->Type == BarEventType::Solid) {
            _NativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(currentColor, opacity));  // Fill with semi-transparent preview color
            
            if(borderWidth > 0.0f) {
                _NativeRenderer->DrawRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(currentColor), borderWidth);    // Draw border
            }
        }
        else if (bar->Type == BarEventType::Fade)
        {
            float BarXHoverRatio = ToolAccess->BarXHoverRatio;
            
            Color ColorStart = bar->FadeInfo->ColorStart;
            Color ColorEnd = bar->FadeInfo->ColorEnd;

            if (bar->FadeInfo->Type == FadeType::Two_Colors) {
                BarXHoverRatio <= 0.5f ? ColorStart = currentColor : ColorEnd = currentColor;

                _NativeRenderer->FillRectangleGradient2(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(ColorStart, opacity), COLOR_TO_COLOR_F_A(ColorEnd, opacity));
            }
            else if (bar->FadeInfo->Type == FadeType::Three_Colors) {
                Color ColorCenter = bar->FadeInfo->ColorCenter;
                if (BarXHoverRatio <= 0.33f) {
                    ColorStart = currentColor;
                }
                else if (BarXHoverRatio <= 0.66f) {
                    ColorCenter = currentColor;
                }
                else {
                    ColorEnd = currentColor;
                }

                _NativeRenderer->FillRectangleGradient3(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(ColorStart, opacity), COLOR_TO_COLOR_F_A(ColorCenter, opacity), COLOR_TO_COLOR_F_A(ColorEnd, opacity));
            }
        }
        else if (bar->Type == BarEventType::Strobe)
        {
            float StrobeWidthPixels = TicksToPixels(bar->StrobeInfo->QuantizationTicks);
            
            _NativeRenderer->FillRectangleStripes(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(currentColor, opacity), StrobeWidthPixels);  // Fill with semi-transparent preview color

            if (borderWidth > 0.0f) {
                _NativeRenderer->DrawRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(currentColor), borderWidth);    // Draw border
            }
        }
    }

	void Timeline_Direct2DRenderer::DrawGhostBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds)
    {
        if (!_NativeRenderer) {
            return;
        }

		System::Drawing::Rectangle BarBounds = GetGhostBarBounds(bar, trackContentBounds);

		switch (bar->Type)
		{
		case BarEventType::Solid:	DrawGhostBarSolid(bar, BarBounds); break;
		case BarEventType::Fade:	DrawGhostBarFade(bar, BarBounds); break;
		case BarEventType::Strobe:	DrawGhostBarStrobe(bar, BarBounds); break;
		}
    }

	void Timeline_Direct2DRenderer::DrawGhostBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		// Draw with high transparency (ghost effect)
		_NativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(bar->Color, 0.3f));
	}

	void Timeline_Direct2DRenderer::DrawGhostBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);

		if (bar->FadeInfo == nullptr) {
			return;
		}

		if (bar->FadeInfo->Type == FadeType::Two_Colors) {
			// Draw with high transparency (ghost effect)
			_NativeRenderer->FillRectangleGradient2(BarRect,
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorStart, 0.3f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorEnd, 0.3f));
		}
		else if (bar->FadeInfo->Type == FadeType::Three_Colors) {
			// Draw with high transparency (ghost effect)
			_NativeRenderer->FillRectangleGradient3(BarRect,
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorStart, 0.3f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorCenter, 0.3f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorEnd, 0.3f));
		}
	}

	void Timeline_Direct2DRenderer::DrawGhostBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{

	}

    void Timeline_Direct2DRenderer::DrawSelectedBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds)
    {
        if (!_NativeRenderer) {
            return;
        }

		System::Drawing::Rectangle BarBounds = GetBarBounds(bar, trackContentBounds);

		switch (bar->Type)
		{
		case BarEventType::Solid:	DrawNormalBarSolid(bar, BarBounds); break;
		case BarEventType::Fade:	DrawNormalBarFade(bar, BarBounds); break;
		case BarEventType::Strobe:	DrawNormalBarStrobe(bar, BarBounds); break;
		}

        // Add glow effect
        DrawBarGlowEffect(BarBounds, m_ColorTheme.SelectionHighlight, 6);

		// Add tool-specific enhancements
		DrawToolEnhancements(bar, BarBounds);
    }

	void Timeline_Direct2DRenderer::DrawPastePreviewBar(BarEvent^ bar, System::Drawing::Rectangle trackContentBounds)
    {
        if (!_NativeRenderer) {
            return;
        }

        System::Drawing::Rectangle BarBounds = GetBarBounds(bar, trackContentBounds);

        D2D1_RECT_F BarRect = RECT_TO_RECT_F(BarBounds);

        // Draw shadow effect
        for (int i = 3; i >= 0; i--)
        {
            D2D1_RECT_F ShadowRect = D2D1::RectF(BarRect.left + i, BarRect.top + i, BarRect.right + i, BarRect.bottom + i);
            D2D1_COLOR_F ShadowColor = D2D1::ColorF(0, 0, 0, (0.4f - (i * 0.1f)));
            
            _NativeRenderer->FillRectangle(ShadowRect, ShadowColor);
        }

		switch (bar->Type)
		{
		case BarEventType::Solid:	DrawPastePreviewBarSolid(bar, BarBounds); break;
		case BarEventType::Fade:	DrawPastePreviewBarFade(bar, BarBounds); break;
		case BarEventType::Strobe:	DrawPastePreviewBarStrobe(bar, BarBounds); break;
		}

        // Draw glowing border
        DrawBarGlowEffect(BarBounds, Color::FromArgb(180, 255, 255, 255), 2);

        // Draw dashed outline
        _NativeRenderer->DrawRectangleDashed(BarRect, COLOR_TO_COLOR_F(Color::White), 1.0f, 4.0f, 4.0f);
    }

	void Timeline_Direct2DRenderer::DrawPastePreviewBarSolid(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		// Draw semi-transparent bar
		_NativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(bar->Color, 0.7f));
	}

	void Timeline_Direct2DRenderer::DrawPastePreviewBarFade(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		D2D1_RECT_F BarRect = RECT_TO_RECT_F(barBounds);

		if (bar->FadeInfo == nullptr) {
			return;
		}

		if (bar->FadeInfo->Type == FadeType::Two_Colors) {
			// Normal appearance with 70% opacity
			_NativeRenderer->FillRectangleGradient2(BarRect,
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorStart, 0.7f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorEnd, 0.7f));
		}
		else if (bar->FadeInfo->Type == FadeType::Three_Colors) {
			// Normal appearance with 70% opacity
			_NativeRenderer->FillRectangleGradient3(BarRect,
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorStart, 0.7f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorCenter, 0.7f),
													COLOR_TO_COLOR_F_A(bar->FadeInfo->ColorEnd, 0.7f));
		}
	}

	void Timeline_Direct2DRenderer::DrawPastePreviewBarStrobe(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
        if (bar->StrobeInfo == nullptr) {
            return;
        }

        float StrobeWidthPixels = TicksToPixels(bar->StrobeInfo->QuantizationTicks);

        // Normal appearance with 70% opacity
        _NativeRenderer->FillRectangleStripes(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(bar->Color, 0.7f), StrobeWidthPixels);
	}

    void Timeline_Direct2DRenderer::DrawBarGlowEffect(System::Drawing::Rectangle barBounds, System::Drawing::Color glowColor, int glowLevels)
    {
        if (!_NativeRenderer) {
            return;
        }

        for (int i = glowLevels; i >= 0; i--)
        {
            // Create inflated rectangle for glow
            D2D1_RECT_F glowRect = D2D1::RectF(
                (float)(barBounds.X - i),
                (float)(barBounds.Y - i),
                (float)(barBounds.Right + i),
                (float)(barBounds.Bottom + i)
            );

            // Create color with decreasing alpha for outer glow
            D2D1_COLOR_F glowD2DColor = D2D1::ColorF(
                glowColor.R / 255.0f,
                glowColor.G / 255.0f,
                glowColor.B / 255.0f,
                (0.3f - (i * 0.05f))  // Decreasing alpha for outer glow
            );

            _NativeRenderer->DrawRectangle(glowRect, glowD2DColor, 1.0f);
        }
    }

    void Timeline_Direct2DRenderer::DrawSelectionRectangle(System::Drawing::Rectangle selectionRectangle)
    {
        if (!_NativeRenderer) {
            return;
        }
        
        if (selectionRectangle.Width > 0 && selectionRectangle.Height > 0)
        {
            // Fill with semi-transparent selection color
            _NativeRenderer->FillRectangle(RECT_TO_RECT_F(selectionRectangle), COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.2f));

            // Draw border
            _NativeRenderer->DrawRectangle(RECT_TO_RECT_F(selectionRectangle), COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight), 1.0f);
        }
    }

	void Timeline_Direct2DRenderer::DrawToolEnhancements(BarEvent^ bar, System::Drawing::Rectangle barBounds)
	{
		TimelineToolType CurrentTool = this->_ToolAccessDelegate->CurrentToolType();

		// Add tool-specific enhancements
		// 1. Draw Resize Handles on each bar if Duration Tool is selected
		if (CurrentTool == TimelineToolType::Duration)
		{
			DrawResizeHandle(barBounds, false);
		}
		else if (CurrentTool == TimelineToolType::Pointer)
		{
			ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
			if (!ToolAccess) {
				return;
			}

			if (bar == ToolAccess->HoveredBar && ToolAccess->IsOverResizeHandle)
			{
				DrawResizeHandle(barBounds, false);
			}
			else if (ToolAccess->SelectedBars->Count > 0 && ToolAccess->SelectedBars->Contains(bar) && ToolAccess->IsOverResizeHandle)
			{
				DrawResizeHandle(barBounds, false);
			}
		}
	}

	void Timeline_Direct2DRenderer::DrawResizeHandle(System::Drawing::Rectangle barBounds, bool isTargeted)
	{
		if (!_NativeRenderer) {
			return;
		}

		System::Drawing::Rectangle HandleBounds(barBounds.Right - 5, barBounds.Y + 5, 3, barBounds.Height - 10);

		// Fill handle rectangle
		_NativeRenderer->FillRectangle(RECT_TO_RECT_F(HandleBounds), COLOR_TO_COLOR_F_A(Color::White, 0.8f));

		// Add glow effect for targeted handles
		if (isTargeted) {
			DrawBarGlowEffect(barBounds, m_ColorTheme.SelectionHighlight, 3);
		}
	}

    float Timeline_Direct2DRenderer::TicksToPixels(int ticks)
    {
        if (ticks == 0) {
            return 0.0f;
        }

        // Scale for display
        const float baseScale = 16.0f / (float)TICKS_PER_QUARTER;
        return (float)ticks * baseScale * this->_ZoomLevel;
    }

    float Timeline_Direct2DRenderer::PixelsToTicks(int pixels)
    {
        // Handle edge cases
        if (pixels == 0) return 0;

        // Use double for intermediate calculations to maintain precision
        double baseScale = TICKS_PER_QUARTER / 16.0;
        double scaledPixels = (double)pixels * baseScale;
        double result = scaledPixels / this->_ZoomLevel;

        // Round to nearest integer
        return (float)result;
    }

    float Timeline_Direct2DRenderer::GetSubdivisionLevel(float pixelsPerBeat)
    {
        float subdivLevel = pixelsPerBeat / (float)this->MIN_PIXELS_BETWEEN_GRIDLINES;

        // Extended subdivision levels for higher zoom
        if (subdivLevel >= 64) return 64;
        if (subdivLevel >= 32) return 32;
        if (subdivLevel >= 16) return 16;
        if (subdivLevel >= 8) return 8;
        if (subdivLevel >= 4) return 4;
        if (subdivLevel >= 2) return 2;

        return 1;
    }

    int Timeline_Direct2DRenderer::GetMeasureSkipFactor(float pixelsBetweenMeasures)
    {
        if (pixelsBetweenMeasures >= MIN_MEASURE_NUMBER_SPACING) return 1;
        if (pixelsBetweenMeasures * 2 >= MIN_MEASURE_NUMBER_SPACING) return 2;
        if (pixelsBetweenMeasures * 4 >= MIN_MEASURE_NUMBER_SPACING) return 4;
        if (pixelsBetweenMeasures * 8 >= MIN_MEASURE_NUMBER_SPACING) return 8;

        return 16;  // Maximum skip factor
    }

    int Timeline_Direct2DRenderer::GetTrackTop(Track^ track)
    {
        int top = HEADER_HEIGHT;

        for each (Track ^ t in this->_Tracks)
        {
            if (t == track) {
                break;
            }

            top += t->Height;
        }

        return top;
    }

	int Timeline_Direct2DRenderer::GetLeftPanelWidth()
	{
		if (this->_Left_Panel->IsExpanded) {
			return this->_Left_Panel->Width;
		}

		return 0;
	}

	int Timeline_Direct2DRenderer::GetLeftPanelAndTrackHeaderWidth()
	{
		return (GetLeftPanelWidth() + TRACK_HEADER_WIDTH);
	}

    float Timeline_Direct2DRenderer::GetTotalTrackHeight()
    {
        float TotalHeight = 0;

        for each (Track ^ track in this->_Tracks) {
            TotalHeight += track->Height;
        }

        return TotalHeight;
    }

    Track^ Timeline_Direct2DRenderer::GetTrackAtPoint(System::Drawing::Point point)
    {
		if (point.Y < HEADER_HEIGHT) {
			return nullptr;
		}

		if (_Left_Panel->IsExpanded && point.X < _Left_Panel->Width) {
			return nullptr;
		}

        int Y = HEADER_HEIGHT + this->_ScrollPosition->Y;
        for each(Track^ track in this->_Tracks)
        {
            int Height = track->Height;

            if (point.Y >= Y && point.Y < Y + Height)
            {
                return track;
            }
            Y += Height;
        }

        return nullptr;
    }

    Measure^ Timeline_Direct2DRenderer::GetMeasureAtTick(int tick)
    {
        int Accumulated = 0;
        
        for each(Measure^ Meas in this->_Measures)
        {
            if (tick >= Accumulated && tick < Accumulated + Meas->Length) {
                return Meas;
            }
            Accumulated += Meas->Length;
        }

        return nullptr;
    }

    BarEvent^ Timeline_Direct2DRenderer::GetBarAtPoint(System::Drawing::Point point)
    {
        Track^ Trk = GetTrackAtPoint(point);
        
        if (Trk == nullptr) {
            return nullptr;
        }

        // Convert point to tick position
        int ClickTick = (int)PixelsToTicks(point.X - GetLeftPanelAndTrackHeaderWidth() - this->_ScrollPosition->X);

        // Get track content bounds for vertical check
        System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Trk);
        TrackContentBounds.Y += this->_ScrollPosition->Y;

        // Check each bar in the track
        for each(BarEvent^ Bar in Trk->Events)
        {
            // First check if the click is within the bar's time range
            if (ClickTick >= Bar->StartTick && ClickTick <= Bar->StartTick + Bar->Duration)
            {
                // Then check if the click is within the track's vertical bounds
                if (point.Y >= TrackContentBounds.Y + TRACK_PADDING && point.Y <= TrackContentBounds.Y + TrackContentBounds.Height - TRACK_PADDING)
                {
                    return Bar;
                }
            }
        }

        return nullptr;
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetTrackBounds(Track^ track)
    {
        int Top = GetTrackTop(track);

        // Create full width bounds at the scrolled position
        return System::Drawing::Rectangle(0, Top, this->_Control->Width, track->Height);
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetTrackHeaderBounds(Track^ track)
    {
        System::Drawing::Rectangle Bounds = GetTrackBounds(track);
		Bounds.X = GetLeftPanelWidth();
        Bounds.Width = TRACK_HEADER_WIDTH;

        return Bounds;
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetTrackContentBounds(Track^ track)
    {
        int Top = GetTrackTop(track);
        int Height = track->Height;
		int LeftOffset = GetLeftPanelAndTrackHeaderWidth();

        return System::Drawing::Rectangle(
			LeftOffset,				// Left starts after Header and Panel
            Top,					// Top
            this->_Control->Width,	// Right is full width
            Height					// Bottom
        );
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetBarBounds(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        int X = (int)TicksToPixels(bar->StartTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth();
        int Width = (int)Math::Max(1.0f, TicksToPixels(bar->Duration)); // Ensure minimum width of 1 pixel

        System::Drawing::Rectangle BarBounds = System::Drawing::Rectangle(
            X,                                  // Left
            bounds.Top + TRACK_PADDING,         // Top
            Width,                              // Width
            bounds.Height - TRACK_PADDING * 2   // Height
        );

        return BarBounds;
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetGhostBarBounds(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        int X = (int)TicksToPixels(bar->OriginalStartTick) + this->_ScrollPosition->X + GetLeftPanelAndTrackHeaderWidth();
        int Width = (int)Math::Max(1.0f, TicksToPixels(bar->OriginalDuration)); // Ensure minimum width of 1 pixel

        System::Drawing::Rectangle BarBounds = System::Drawing::Rectangle(
            X,                                  // Left
            bounds.Top + TRACK_PADDING,         // Top
            Width,                              // Width
            bounds.Height - TRACK_PADDING * 2   // Height
        );

        return BarBounds;
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetTrackButtonBounds(int buttonIndex, System::Drawing::Rectangle trackHeaderBounds)
    {
        return System::Drawing::Rectangle(
            trackHeaderBounds.Right - (BUTTON_SIZE + BUTTON_MARGIN) * (buttonIndex + 1),
            trackHeaderBounds.Y + BUTTON_MARGIN,
            BUTTON_SIZE,
            BUTTON_SIZE
        );
    }

    std::wstring Timeline_Direct2DRenderer::ConvertString(System::String^ str)
    {
        if (str == nullptr) {
            return L"";
        }

        System::IntPtr ptr = System::Runtime::InteropServices::Marshal::StringToHGlobalUni(str);
        std::wstring result(static_cast<wchar_t*>(ptr.ToPointer()));
        System::Runtime::InteropServices::Marshal::FreeHGlobal(ptr);

        return result;
    }

	std::wstring Timeline_Direct2DRenderer::ToHexString(int value)
	{
		static const wchar_t* digits = L"0123456789ABCDEF";

		// Ensure the value is within valid range for a color component
		value = std::min(255, std::max(0, value));

		// Convert to two-digit hex
		wchar_t hex[3];
		hex[0] = digits[(value >> 4) & 0xF]; // First digit
		hex[1] = digits[value & 0xF];        // Second digit
		hex[2] = L'\0';                      // Null terminator

		return std::wstring(hex);
	}

    void Timeline_Direct2DRenderer::Cleanup()
    {
        if (!m_disposed)
        {
            if (_NativeRenderer)
            {
                _NativeRenderer->Cleanup();
                delete _NativeRenderer;
                _NativeRenderer = nullptr;
            }
            m_disposed = true;
        }
    }

} // namespace MIDILightDrawer
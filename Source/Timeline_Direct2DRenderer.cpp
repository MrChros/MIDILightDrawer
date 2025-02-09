#include "Timeline_Direct2DRenderer.h"
#include "Timeline_Direct2DRenderer_Native.h"

#include "Widget_Timeline_Common.h"
#include "Widget_Timeline_Classes.h"

#include <vcclr.h>
#include <msclr\marshal_cppstd.h>

#define COLOR_TO_RGBA(__color__)                    __color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, __color__.A / 255.0f
#define COLOR_TO_COLOR_F(__color__)                 D2D1::ColorF(__color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, __color__.A / 255.0f)
#define COLOR_TO_COLOR_F_A(__color__, __alpha__)    D2D1::ColorF(__color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, (__color__.A / 255.0f) * __alpha__)
#define RECT_TO_RECT_F(__rect__)                    D2D1::RectF((float)__rect__.Left, (float)__rect__.Top, (float)__rect__.Right, (float)__rect__.Bottom)

namespace MIDILightDrawer
{
    Timeline_Direct2DRenderer::Timeline_Direct2DRenderer(List<Track^>^ tracks, List<Measure^>^ measures, double zoomlevel, System::Drawing::Point^ scrollposition) : _NativeRenderer(nullptr), m_disposed(false), _ControlHandle(System::IntPtr::Zero), m_themeColorsCached(false)
    {
        this->_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
        
        // Create the native renderer
        this->_NativeRenderer = new Timeline_Direct2DRenderer_Native();

        this->_Control              = nullptr;
        this->_Tracks               = tracks;
        this->_Measures             = measures;
        this->_ToolAccessDelegate   = nullptr;

        SetZoomLevel(zoomlevel);
        SetScrollPositionReference(scrollposition);
    }

    Timeline_Direct2DRenderer::~Timeline_Direct2DRenderer()
    {
        this->!Timeline_Direct2DRenderer();
    }

    Timeline_Direct2DRenderer::!Timeline_Direct2DRenderer()
    {
        Cleanup();
    }

    bool Timeline_Direct2DRenderer::Initialize(System::Windows::Forms::Control^ control)
    {
        if (m_disposed || !control) {
            return false;
        }

        this->_Control = control;

        // Store control handle
        _ControlHandle = control->Handle;

        // Initialize the native renderer
        return _NativeRenderer->Initialize((HWND)_ControlHandle.ToPointer());
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

    void Timeline_Direct2DRenderer::PreloadImages()
    {
        // Draw the image with color transformation if needed

        System::Drawing::Imaging::ColorMatrix^ ColorMatrix = gcnew System::Drawing::Imaging::ColorMatrix(
            gcnew array<array<float>^> {
            gcnew array<float>{m_ColorTheme.Text.R / 255.0f, 0, 0, 0, 0},
                gcnew array<float>{0, m_ColorTheme.Text.G / 255.0f, 0, 0, 0},
                gcnew array<float>{0, 0, m_ColorTheme.Text.B / 255.0f, 0, 0},
                gcnew array<float>{0, 0, 0, m_ColorTheme.Text.A / 255.0f, 0},
                gcnew array<float>{0, 0, 0, 0, 1.0f}
        }
        );
        
        System::Drawing::Imaging::ImageAttributes^ Attributes = gcnew System::Drawing::Imaging::ImageAttributes();
        Attributes->SetColorMatrix(ColorMatrix);

        // Preload icons
        System::Drawing::Image^ NoteIcon = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Note_White")));
        System::Drawing::Bitmap^ TransformedIcon = gcnew System::Drawing::Bitmap(NoteIcon->Width, NoteIcon->Height);

        {
            System::Drawing::Graphics ^ g = System::Drawing::Graphics::FromImage(TransformedIcon);
            g->DrawImage(NoteIcon,
                System::Drawing::Rectangle(0, 0, NoteIcon->Width, NoteIcon->Height),  // dest rectangle
                0, 0, NoteIcon->Width, NoteIcon->Height,                             // source rectangle
                System::Drawing::GraphicsUnit::Pixel,
                Attributes);
            delete g;
        }
        
        this->_NativeRenderer->PreloadBitmap(L"Note_White", TransformedIcon);
    }

    bool Timeline_Direct2DRenderer::BeginDraw()
    {
        if (m_disposed || !_NativeRenderer)
            return false;

        return _NativeRenderer->BeginDraw();
    }

    bool Timeline_Direct2DRenderer::EndDraw()
    {
        if (m_disposed || !_NativeRenderer)
            return false;

        return _NativeRenderer->EndDraw();
    }

    bool Timeline_Direct2DRenderer::DrawWidgetBackground()
    {
        if (!_NativeRenderer) {
            return false;
        }

        // Create the rectangle for the tracks area
        D2D1_RECT_F widgetArea = D2D1::RectF(
            0,                              // Left
            0,                              // Top
            (float)this->_Control->Width,   // Right
            (float)this->_Control->Height   // Bottom
        );

        // Fill the rectangle using the native renderer
        return _NativeRenderer->FillRectangle(widgetArea, COLOR_TO_COLOR_F(m_ColorTheme.Background));
    }

    bool Timeline_Direct2DRenderer::DrawTrackBackground()
    {
        if (!_NativeRenderer) {
            return false;
        }

        // Create the rectangle for the tracks area
        D2D1_RECT_F tracksArea = D2D1::RectF(
            0,                              // Left
            HEADER_HEIGHT,                  // Top
            (float)this->_Control->Width,   // Right
            (float)this->_Control->Height   // Bottom
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
        D2D1_RECT_F headerRect = D2D1::RectF(0, 0, (float)this->_Control->Width, HEADER_HEIGHT);
        _NativeRenderer->FillRectangle(headerRect, COLOR_TO_COLOR_F(m_ColorTheme.HeaderBackground));

        // Constants for vertical positioning within header
        const float markerTextY = 2;
        const float timeSignatureY = markerTextY + 14 + 2;
        const float measureNumberY = timeSignatureY + 14 + 4;

        D2D1_COLOR_F d2dTextColor = COLOR_TO_COLOR_F(m_ColorTheme.Text);

        int accumulated = 0;
        int measureNumber = 1;

        // For each measure
        for each(MIDILightDrawer::Measure ^ measure in  this->_Measures)
        {
            // Convert tick position to pixels
            float x = TicksToPixels(accumulated) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;

            if (x >= TRACK_HEADER_WIDTH && x <= (float)this->_Control->Width)
            {
                // Draw vertical tick mark for measure start
                _NativeRenderer->DrawLine(
                    x, HEADER_HEIGHT - 5,
                    x, HEADER_HEIGHT - 0,
                    d2dTextColor,
                    1.0f
                );
                
                // Draw marker text if present
                if (!System::String::IsNullOrEmpty(measure->Marker_Text))
                {
                    D2D1_RECT_F markerRect = D2D1::RectF(x - 50, markerTextY, x + 50, markerTextY + 14);

                    std::wstring markerText = ConvertString(measure->Marker_Text);
                    _NativeRenderer->DrawText(markerText, markerRect, d2dTextColor, _NativeRenderer->GetMarkerTextFormat());
                }

                // Draw measure number
                D2D1_RECT_F numberRect = D2D1::RectF(x - 25, measureNumberY, x + 25, measureNumberY + 14);

                std::wstring numText = std::to_wstring(measureNumber);
                _NativeRenderer->DrawText(numText, numberRect, d2dTextColor, _NativeRenderer->GetMeasureNumberFormat());

                // Draw time signature
                D2D1_RECT_F sigRect = D2D1::RectF(x - 25, timeSignatureY, x + 25, timeSignatureY + 14);

                std::wstring timeSignature = std::to_wstring(measure->Numerator) + L"/" + std::to_wstring(measure->Denominator);

                _NativeRenderer->DrawText(timeSignature, sigRect, d2dTextColor, _NativeRenderer->GetTimeSignatureFormat());
            }

            // Calculate subdivision level based on time signature
            int ticksPerBeat = measure->Denominator == 8 ?
                TICKS_PER_QUARTER / 2 :  // 8th note-based measures
                TICKS_PER_QUARTER;       // Quarter note-based measures

            float pixelsPerBeat = TicksToPixels(ticksPerBeat);
            float subdivLevel = GetSubdivisionLevel(pixelsPerBeat);

            // Determine if we should show subdivisions
            bool showSubdivisions = (measure->Denominator == 8) ?
                (subdivLevel >= 3) :  // For 8th-based measures
                (subdivLevel >= 2);   // For quarter-based measures

            if (showSubdivisions)
            {
                DrawBeatNumbers(measure, x, measureNumberY, subdivLevel, measureNumber, ticksPerBeat);
            }

            accumulated += measure->Length;
            measureNumber++;
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
            (float)HEADER_HEIGHT,
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
            DrawTrackHeaders();
            DrawTrackDividers(hoverTrack);
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
            TRACK_HEADER_WIDTH,
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
            float beatX = x + TicksToPixels(beat * ticksPerBeat);

            // Only draw if beat marker is visible
            if (beatX >= TRACK_HEADER_WIDTH && beatX <= (float)this->_Control->Width)
            {
                // Create beat number text (e.g., "1.2" for measure 1, beat 2)
                std::wstring beatText = std::to_wstring(measureNumber) + L"." + std::to_wstring(beat + 1);

                // Define text rectangle
                D2D1_RECT_F beatRect = D2D1::RectF(
                    beatX - 25,
                    measureNumberY + 4, // Offset slightly below measure numbers
                    beatX + 25,
                    measureNumberY + 18
                );

                // Draw with smaller font size
                IDWriteTextFormat* quarterNoteFormat = _NativeRenderer->GetQuarterNoteFormat();
                _NativeRenderer->DrawText(beatText, beatRect, beatColor, quarterNoteFormat);

                // Draw tick mark
                _NativeRenderer->DrawLine(beatX, HEADER_HEIGHT - 2, beatX, HEADER_HEIGHT, beatColor, 1.0f);
            }
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawGridLines(float totalHeight)
    {
        // Calculate visible range in ticks
        int StartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int EndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + this->_Control->Width - TRACK_HEADER_WIDTH);

        // Draw grid lines in proper order (back to front)
        DrawSubdivisionLines(totalHeight, StartTick, EndTick);
        DrawBeatLines(totalHeight, StartTick, EndTick);
        DrawMeasureLines(totalHeight, StartTick, EndTick);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawSubdivisionLines(float totalHeight, int startTick, int endTick)
    {
        float SubdivLevel = GetSubdivisionLevel(TicksToPixels(TICKS_PER_QUARTER));
        if (SubdivLevel <= 1) {
            return true;
        }

        int Accumulated = 0;
        for each (Measure^ measure in this->_Measures)
        {
            int MeasureStart = Accumulated;
            int TicksPerBeat = TICKS_PER_QUARTER * 4 / measure->Denominator;
            int TicksPerSubdiv = TicksPerBeat / (int)SubdivLevel;

            // Calculate subdivisions for this measure
            int Subdivisions = (measure->Length / TicksPerSubdiv);

            for (int subdiv = 1; subdiv < Subdivisions; subdiv++)
            {
                int SubdivTick = MeasureStart + subdiv * TicksPerSubdiv;

                // Skip if this is already a beat or measure line
                if (SubdivTick % TicksPerBeat == 0) {
                    continue;
                }

                if (SubdivTick >= startTick && SubdivTick <= endTick)
                {
                    float x = TicksToPixels(SubdivTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;
                    _NativeRenderer->DrawLine(
                        x, HEADER_HEIGHT,
                        x, HEADER_HEIGHT + totalHeight,
                        COLOR_TO_RGBA(m_ColorTheme.SubdivisionLine),
                        1.0f
                    );
                }
            }

            Accumulated += measure->Length;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawBeatLines(float totalHeight, int startTick, int endTick)
    {
        int accumulated = 0;
        for each (MIDILightDrawer::Measure ^ measure in this->_Measures)
        {
            int measureStart = accumulated;
            int ticksPerBeat = TICKS_PER_QUARTER * 4 / measure->Denominator;

            // Draw lines for each beat except measure start
            for (int beat = 1; beat < measure->Numerator; beat++)
            {
                int beatTick = measureStart + beat * ticksPerBeat;

                if (beatTick >= startTick && beatTick <= endTick)
                {
                    float x = TicksToPixels(beatTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;
                    _NativeRenderer->DrawLine(
                        x, HEADER_HEIGHT,
                        x, HEADER_HEIGHT + totalHeight,
                        COLOR_TO_RGBA(m_ColorTheme.BeatLine),
                        1.0f
                    );
                }
            }

            accumulated += measure->Length;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawMeasureLines(float totalHeight, int startTick, int endTick)
    {
        int accumulated = 0;
        for each (MIDILightDrawer::Measure ^ measure in this->_Measures)
        {
            if (accumulated > endTick)
                break;

            if (accumulated + measure->Length >= startTick)
            {
                float x = TicksToPixels(accumulated) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;

                if (x >= TRACK_HEADER_WIDTH && x <= (float)this->_Control->Width)
                {
                    _NativeRenderer->DrawLine(x, HEADER_HEIGHT, x, HEADER_HEIGHT + totalHeight, COLOR_TO_RGBA(m_ColorTheme.MeasureLine), 1.0f);
                }
            }

            accumulated += measure->Length;
        }

        return true;
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

        // Early exit if zoom level is too small to be readable
        if (this->_ZoomLevel < 0.1) return true;

        float logScale = (float)Math::Log(this->_ZoomLevel + 1, 2);

        if (track->IsDrumTrack && track->ShowAsStandardNotation)
        {
            return DrawTrackTablatureDrum(track, trackContentBounds, logScale);
        }
        else
        {
            return DrawTrackTablatureRegular(track, trackContentBounds, logScale);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackTablatureDrum(Track^ track, System::Drawing::Rectangle trackContentBounds, float logScale)
    {
        if (!_NativeRenderer) {
            return false;
        }

        float availableHeight = (float)(trackContentBounds.Height - TRACK_PADDING * 2);
        TabStringInfo String_Info = DrawTablatureStrings(trackContentBounds, availableHeight, logScale, 5);

        if (String_Info.TotalHeight > availableHeight) {
            return true;
        }

        try {
            // Calculate visible range
            int visibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
            int visibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

            // Calculate required space for Beat Duration drawing
            const float BASE_DURATION_SPACE = 23.0f;
            const float DURATION_SCALE_FACTOR = 20.0f;
            float requiredSpace = BASE_DURATION_SPACE + (DURATION_SCALE_FACTOR * logScale);

            // Draw notes with symbols
            int measureStartTick = 0;

            for (int i = 0; i < track->Measures->Count; i++)
            {
                TrackMeasure^ measure = track->Measures[i];
                if (measure == nullptr) {
                    measureStartTick += measure->Length;
                    continue;
                }

                int measureEndTick = measureStartTick + measure->Length;

                // Skip if measure is out of visible range
                if (measureStartTick > visibleEndTick || measureEndTick < visibleStartTick)
                {
                    measureStartTick = measureEndTick;
                    continue;
                }

                for each(Beat^ beat in measure->Beats)
                {
                    if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0) {
                        continue;
                    }

                    if ((beat->Duration > 0) && (beat->Notes->Count > 0) && availableHeight > String_Info.TotalHeight + requiredSpace) {
                        DrawBeatDuration(beat, trackContentBounds, String_Info.StringYPositions);
                    }

                    float xPos = (float)(TicksToPixels(beat->StartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH);

                    for each(Note ^ note in beat->Notes)
                    {
                        DrumNoteInfo noteInfo = DrumNotationMap::GetNoteInfo(note->Value);

                        // Calculate Y position (can be between lines)
                        float yPos;
                        int lineIndex = (int)Math::Floor(noteInfo.StringPosition);
                        float fraction = noteInfo.StringPosition - lineIndex;

                        lineIndex -= 1;

                        if (fraction == 0.0f) { // On the line
                            yPos = String_Info.StringYPositions[lineIndex];
                        }
                        else if (lineIndex < 0) {
                            yPos = String_Info.StringYPositions[0] - (String_Info.StringYPositions[1] - String_Info.StringYPositions[0]) * fraction;
                        }
                        else { // Between lines
                            yPos = String_Info.StringYPositions[lineIndex] + (String_Info.StringYPositions[lineIndex + 1] - String_Info.StringYPositions[lineIndex]) * fraction;
                        }

                        DrawDrumSymbol(noteInfo.SymbolType, xPos, yPos, ((String_Info.StringYPositions[1] - String_Info.StringYPositions[0]) / 2.0f) - 1);
                    }
                }
                measureStartTick += measure->Length;
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

        // Calculate zoom-based scaling factors
        const float BASE_FONT_SIZE = 9.0f;
        const float FONT_SCALE_FACTOR = 2.0f;
        float scaledFontSize = BASE_FONT_SIZE + (logScale * FONT_SCALE_FACTOR);

        // Clamp font size between min and max values
        scaledFontSize = Math::Min(Math::Max(scaledFontSize, 4.0f), 18.0f);

        float availableHeight = (float)(trackContentBounds.Height - TRACK_PADDING * 2);

        TabStringInfo String_Info = DrawTablatureStrings(trackContentBounds, availableHeight, logScale, 6);

        if (String_Info.TotalHeight > availableHeight) {
            return true;
        }

        IDWriteTextFormat* TablatureTextFormat = _NativeRenderer->UpdateTablatureFormat(scaledFontSize);

        if (TablatureTextFormat == nullptr) {
            return false;
        }

        try
        {
            // Calculate visible tick range
            int visibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
            int visibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

            // Track measure position
            int measureStartTick = 0;

            // Process only visible measures
            for (int i = 0; i < track->Measures->Count; i++)
            {
                TrackMeasure^ measure = track->Measures[i];
                if (measure == nullptr) {
                    measureStartTick += measure->Length;
                    continue;
                }

                int measureEndTick = measureStartTick + measure->Length;

                // Skip if measure is out of visible range
                if (measureStartTick > visibleEndTick || measureEndTick < visibleStartTick)
                {
                    measureStartTick = measureEndTick;
                    continue;
                }

                // Draw beats in this measure
                for each (Beat ^ beat in measure->Beats)
                {
                    if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0) {
                        continue;
                    }

                    int beatTick = beat->StartTick;

                    // Skip if beat is outside visible range
                    if (beatTick > visibleEndTick || beatTick + beat->Duration < visibleStartTick) {
                        continue;
                    }

                    float beatX = (float)(TicksToPixels(beatTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH);

                    const float BASE_DURATION_SPACE = 23.0f;
                    const float DURATION_SCALE_FACTOR = 20.0f;
                    float requiredSpace = BASE_DURATION_SPACE + (DURATION_SCALE_FACTOR * logScale);

                    // Draw duration lines for beats with multiple notes
                    if ((beat->Duration > 0) && (beat->Notes->Count > 0) && availableHeight > String_Info.TotalHeight + requiredSpace)
                    {
                        DrawBeatDuration(beat, trackContentBounds, String_Info.StringYPositions);
                    }

                    // Draw the notes
                    for each (Note ^ note in beat->Notes)
                    {
                        if (note == nullptr || note->String < 1 || note->String > 6) {
                            continue;
                        }

                        std::wstring fretText = std::to_wstring(note->Value);

                        float textWidth = _NativeRenderer->MeasureTextWidth(fretText, TablatureTextFormat);

                        D2D1_RECT_F textRect;
                        textRect.left = beatX - (textWidth / 2.0f);
                        textRect.top = String_Info.StringYPositions[note->String - 1] - (scaledFontSize / 2.0f);
                        textRect.right = textRect.left + textWidth;
                        textRect.bottom = textRect.top + scaledFontSize;

                        // Draw background for better readability
                        D2D1_RECT_F bgRect = D2D1::RectF(textRect.left - 1, textRect.top, textRect.right + 1, textRect.bottom - 1);

                        D2D1_COLOR_F bgColor = COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBackground, 0.86f);

                        _NativeRenderer->FillRectangle(bgRect, bgColor);

                        // Draw the fret number
                        D2D1_COLOR_F textColor = COLOR_TO_COLOR_F(m_ColorTheme.Text);
                        _NativeRenderer->DrawText(fretText, textRect, textColor, TablatureTextFormat);
                    }
                }

                measureStartTick = measureEndTick;
            }

            DrawTieLines(track, trackContentBounds, String_Info.StringYPositions, scaledFontSize);

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

        // Calculate logarithmic scaling factor
        float logScale = (float)Math::Log(this->_ZoomLevel + 1, 2);

        // Scale base sizes logarithmically with limits
        const float BASE_STEM_LENGTH = 10.0f;
        const float BASE_LINE_LENGTH = 8.0f;
        const float BASE_LINE_SPACING = 3.0f;
        const float BASE_STEM_OFFSET = 8.0f;

        float scaledStemLength = Math::Min(BASE_STEM_LENGTH + (logScale * 6.0f), 35.0f);
        float scaledLineLength = Math::Min(BASE_LINE_LENGTH + (logScale * 2.0f), 15.0f);
        float scaledLineSpacing = Math::Min(BASE_LINE_SPACING + (logScale * 1.0f), 6.0f);
        float scaledLineThickness = Math::Min(1.0f + (logScale * 0.2f), 2.0f);
        float scaledStemOffset = Math::Min(BASE_STEM_OFFSET + (logScale * 2.0f), 20.0f);

        // Get bottom string Y position
        float bottomStringY = stringYPositions[stringYPositions->Length - 1];

        // Calculate x position centered on the note text
        float noteX = (float)(TicksToPixels(beat->StartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH);

        // Position stem below the bottom string with scaled offset
        float stemY = bottomStringY + scaledStemOffset;
        float stemEndY = stemY + scaledStemLength;

        // Calculate stem position - centered below the note
        float stemX = noteX;

        // Create base color with 70% opacity
        D2D1_COLOR_F stemColor = COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f);

        // Calculate duration details
        int duration = beat->Duration;
        int numLines = 0;

        if ((duration >= TICKS_PER_QUARTER * 2) && (duration < TICKS_PER_QUARTER * 4)) // Half note
        {
            // Draw only bottom half of stem
            float halfStemLength = scaledStemLength / 2;
            _NativeRenderer->DrawLine(stemX, stemY + halfStemLength, stemX, stemEndY, stemColor, scaledLineThickness);
        }
        else if (duration < TICKS_PER_QUARTER * 2) // Quarter note or shorter
        {
            // Draw full stem
            _NativeRenderer->DrawLine(stemX, stemY, stemX, stemEndY, stemColor, scaledLineThickness);

            if (duration <= TICKS_PER_QUARTER / 8)           numLines = 3; // 32nd note
            else if (duration <= TICKS_PER_QUARTER / 4)      numLines = 2; // 16th note
            else if (duration <= TICKS_PER_QUARTER / 2)      numLines = 1; // 8th note
            else numLines = 0; // Quarter note or longer

            // Draw horizontal lines
            if (numLines > 0)
            {
                for (int i = 0; i < numLines; i++)
                {
                    float lineY = stemEndY - (i * scaledLineSpacing);
                    float lineStartX = stemX;
                    float lineEndX = stemX + scaledLineLength;

                    _NativeRenderer->DrawLine(lineStartX, lineY, lineEndX, lineY, stemColor, scaledLineThickness);
                }
            }
        }

        if (beat->IsDotted)
        {
            float dotSize = Math::Min(2.0f + (logScale * 0.5f), 4.0f);
            float dotX = stemX + scaledLineLength + (dotSize * 2) - 10;
            float dotY = stemEndY - (numLines * scaledLineSpacing);

            D2D1_ELLIPSE dotEllipse = D2D1::Ellipse(D2D1::Point2F(dotX, dotY), dotSize, dotSize);
            _NativeRenderer->FillEllipse(dotEllipse, stemColor);
        }
        // Draw triplet indicator (small "3")
        else if (duration * 3 / 2 == TICKS_PER_QUARTER ||    // Triplet quarter
            duration * 3 / 2 == TICKS_PER_QUARTER / 2 ||     // Triplet eighth
            duration * 3 / 2 == TICKS_PER_QUARTER / 4)       // Triplet sixteenth
        {
            // Calculate position for the "3" text
            float textX = stemX + scaledLineLength - 10;
            float textY = stemEndY - (numLines * scaledLineSpacing) - 14; // -14 for approx text height

            // Draw the "3"
            D2D1_RECT_F textRect = D2D1::RectF(textX, textY, textX + 10, textY + 14);
            std::wstring tripletText = L"3";
            _NativeRenderer->DrawText(tripletText, textRect, stemColor, _NativeRenderer->GetMeasureNumberFormat());
        }
    }

    void Timeline_Direct2DRenderer::DrawTieLines(Track^ track, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions, float scaledFontSize)
    {
        if (!_NativeRenderer || !track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0) {
            return;
        }

        // Calculate visible range
        int visibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int visibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

        // Create color for tie lines with transparency
        D2D1_COLOR_F tieColor = COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f);

        // Track measure position
        int measureStartTick = 0;

        try {
            // For each measure, find tied notes and draw connections
            for (int i = 0; i < track->Measures->Count; i++)
            {
                TrackMeasure^ measure = track->Measures[i];
                if (measure == nullptr) {
                    measureStartTick += measure->Length;
                    continue;
                }

                int measureEndTick = measureStartTick + measure->Length;

                // Skip if measure is completely outside visible range
                if (measureStartTick > visibleEndTick || measureEndTick < visibleStartTick) {
                    measureStartTick = measureEndTick;
                    continue;
                }

                // Process each beat to find tied notes
                for each (Beat^ currentBeat in measure->Beats)
                {
                    if (currentBeat == nullptr || currentBeat->Notes == nullptr) {
                        continue;
                    }

                    // For each tied note in current beat, find its previous note
                    for each (Note^ currentNote in currentBeat->Notes)
                    {
                        if (!currentNote->IsTied) {
                            continue;
                        }

                        // Find the previous note with the same string and value
                        Beat^ previousBeat = nullptr;
                        Note^ previousNote = nullptr;

                        // Search in current measure first
                        for each (Beat ^ checkBeat in measure->Beats) {
                            if (checkBeat->StartTick >= currentBeat->StartTick) {
                                break;
                            }

                            // Look for matching note
                            for each (Note ^ checkNote in checkBeat->Notes) {
                                if (checkNote->String == currentNote->String && checkNote->Value == currentNote->Value)
                                {
                                    previousBeat = checkBeat;
                                    previousNote = checkNote;
                                }
                            }
                        }

                        // If not found and we're not in first measure, check previous measure
                        if (previousNote == nullptr && i > 0)
                        {
                            TrackMeasure^ prevMeasure = track->Measures[i - 1];

                            if (prevMeasure != nullptr && prevMeasure->Beats != nullptr) 
                            {
                                for each (Beat ^ checkBeat in prevMeasure->Beats) 
                                {
                                    for each (Note ^ checkNote in checkBeat->Notes)
                                    {
                                        if (checkNote->String == currentNote->String && checkNote->Value == currentNote->Value)
                                        {
                                            previousBeat = checkBeat;
                                            previousNote = checkNote;
                                        }
                                    }
                                }
                            }
                        }

                        // Draw tie line if we found the previous note
                        if (previousBeat != nullptr && previousNote != nullptr)
                        {
                            // Calculate note text dimensions
                            std::wstring prevNoteText = std::to_wstring(previousNote->Value);
                            std::wstring currentNoteText = std::to_wstring(currentNote->Value);

                            // Get text format with appropriate size
                            IDWriteTextFormat* textFormat = _NativeRenderer->UpdateTablatureFormat(scaledFontSize);
                            if (!textFormat) {
                                continue;
                            }

                            float prevNoteWidth = _NativeRenderer->MeasureTextWidth(prevNoteText, textFormat);
                            float currentNoteWidth = _NativeRenderer->MeasureTextWidth(currentNoteText, textFormat);

                            // Calculate x positions for both notes
                            float prevNoteX = (float)(TicksToPixels(previousBeat->StartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH);
                            float currentNoteX = (float)(TicksToPixels(currentBeat->StartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH);

                            // Get y position for the string
                            float stringY = stringYPositions[currentNote->String - 1];

                            // Calculate start and end points
                            float startX = prevNoteX + (prevNoteWidth / 2);
                            float endX = currentNoteX - (currentNoteWidth / 2);
                            float startY = stringY + (scaledFontSize / 2);
                            float endY = stringY + (scaledFontSize / 2);

                            // Calculate control points for Bezier curve that dips below
                            float controlHeight = 8.0f * (float)Math::Min(2.0f, Math::Max(0.5f, this->_ZoomLevel));

                            // Draw Bezier curve using Direct2D
                            _NativeRenderer->DrawTieLine(
                                D2D1::Point2F(startX, startY),                                          // Start point
                                D2D1::Point2F(startX + (endX - startX) / 3, startY + controlHeight),    // First control point
                                D2D1::Point2F(startX + (endX - startX) * 2 / 3, endY + controlHeight),  // Second control point
                                D2D1::Point2F(endX, endY),                                              // End point
                                tieColor,
                                1.5f  // Line thickness
                            );
                            
                        }
                    }
                }

                measureStartTick = measureEndTick;
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

        D2D1_COLOR_F symbolColor = COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f); // 70% opacity for symbols

        switch (symbolType)
        {
            case DrumNotationType::FilledDiamond:
            {
                D2D1_POINT_2F points[4] = {
                    D2D1::Point2F(x, y - size),      // Top
                    D2D1::Point2F(x + size, y),      // Right
                    D2D1::Point2F(x, y + size),      // Bottom
                    D2D1::Point2F(x - size, y)       // Left
                };
                _NativeRenderer->FillDiamond(points, 4, symbolColor);
                break;
            }

            case DrumNotationType::HollowDiamond:
            {
                D2D1_POINT_2F points[4] = {
                    D2D1::Point2F(x, y - size),
                    D2D1::Point2F(x + size, y),
                    D2D1::Point2F(x, y + size),
                    D2D1::Point2F(x - size, y)
                };
                _NativeRenderer->DrawPolygon(points, 4, symbolColor, 1.0f);
                break;
            }

            case DrumNotationType::CircledX:
            {
                // Draw circle
                _NativeRenderer->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), size, size), symbolColor, 1.0f);

                // Draw X
                float xSize = size * 0.7f;
                _NativeRenderer->DrawLine(x - xSize, y - xSize, x + xSize, y + xSize, symbolColor, 1.0f);
                _NativeRenderer->DrawLine(x - xSize, y + xSize, x + xSize, y - xSize, symbolColor, 1.0f);
                break;
            }

            case DrumNotationType::AccentedX:
            {
                // Draw X
                float xSize = size * 0.8f;
                _NativeRenderer->DrawLine(x - xSize, y - xSize, x + xSize, y + xSize, symbolColor, 2.5f);
                _NativeRenderer->DrawLine(x - xSize, y + xSize, x + xSize, y - xSize, symbolColor, 2.5f);

                // Draw accent mark (^)
                D2D1_POINT_2F points[3] = {
                    D2D1::Point2F(x - size, y - size * 1.5f),
                    D2D1::Point2F(x, y - size * 2.0f),
                    D2D1::Point2F(x + size, y - size * 1.5f)
                };
                _NativeRenderer->DrawLines(points, 3, symbolColor, 2.5f);
                break;
            }

            case DrumNotationType::RegularX:
            {
                float xSize = size * 0.8f;
                _NativeRenderer->DrawLine(x - xSize, y - xSize, x + xSize, y + xSize, symbolColor, 1.0f);
                _NativeRenderer->DrawLine(x - xSize, y + xSize, x + xSize, y - xSize, symbolColor, 1.0f);
                break;
            }

            case DrumNotationType::NoteEllipse:
            {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), size * 0.7f, size * 0.5f);

                _NativeRenderer->FillEllipse(ellipse, symbolColor);
                break;
            }

            case DrumNotationType::Unknown:
            {
                D2D1_COLOR_F errorColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% opacity
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), size * 0.7f, size * 0.5f);

                _NativeRenderer->FillEllipse(ellipse, errorColor);
                break;
            }
        }
    }

    TabStringInfo Timeline_Direct2DRenderer::DrawTablatureStrings(System::Drawing::Rectangle bounds, float availableHeight, float logScale, int numStrings)
    {
        TabStringInfo info;

        // Calculate spacing and dimensions
        const float BASE_STRING_SPACING = 10.0f;
        const float SPACING_SCALE_FACTOR = 3.0f;
        float scaledStringSpacing = BASE_STRING_SPACING + (logScale * SPACING_SCALE_FACTOR);
        scaledStringSpacing = Math::Min(Math::Max(scaledStringSpacing, 12.0f), 40.0f);

        float Total_Tab_Height = scaledStringSpacing * (numStrings - 1);
        float verticalOffset = bounds.Y + TRACK_PADDING + (availableHeight - Total_Tab_Height) / 2;

        // Store calculated values
        info.StringSpacing = scaledStringSpacing;
        info.TotalHeight = Total_Tab_Height;
        info.VerticalOffset = verticalOffset;
        info.StringYPositions = gcnew array<float>(numStrings);

        if (Total_Tab_Height < availableHeight)
        {
            for (int i = 0; i < numStrings; i++) {
                info.StringYPositions[i] = verticalOffset + (i * scaledStringSpacing);
                _NativeRenderer->DrawLine((float)bounds.X, info.StringYPositions[i], (float)bounds.Right, info.StringYPositions[i], COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f), 1.0f);
            }
        }

        return info;
    }

    bool Timeline_Direct2DRenderer::DrawTrackHeaders()
    {
        if (!_NativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0) {
            return false;
        }

        float totalHeight = GetTotalTrackHeight();

        // Fill header background
        D2D1_RECT_F headerBackground = D2D1::RectF(
            0,                              // Left
            HEADER_HEIGHT,                  // Top
            TRACK_HEADER_WIDTH,             // Right
            totalHeight                     // Bottom
        );

        _NativeRenderer->FillRectangle(headerBackground, COLOR_TO_COLOR_F(m_ColorTheme.HeaderBackground));

        // Draw each track's header
        int CurrentY = HEADER_HEIGHT;

        for each(Track^ track in this->_Tracks)
        {
            // Calculate track bounds
            int TrackHeight = track->Height;
            int yPosition = CurrentY + this->_ScrollPosition->Y;

            System::Drawing::Rectangle TrackHeaderBounds = System::Drawing::Rectangle(0, yPosition, TRACK_HEADER_WIDTH, TrackHeight);

            // Draw header background (with selection highlight if track is selected)
            Color bgColor = track->IsSelected ? m_ColorTheme.SelectionHighlight : m_ColorTheme.HeaderBackground;
            _NativeRenderer->FillRectangle(RECT_TO_RECT_F(TrackHeaderBounds), COLOR_TO_COLOR_F(bgColor));


            // Draw track name if present
            if (!String::IsNullOrEmpty(track->Name))
            {
                // Calculate text bounds with padding
                float textPadding = TRACK_PADDING;
                D2D1_RECT_F textBounds = D2D1::RectF(TrackHeaderBounds.Left + textPadding, TrackHeaderBounds.Top + textPadding, TrackHeaderBounds.Right - textPadding, TrackHeaderBounds.Bottom - textPadding);

                // Draw the track name using the measure number format (for now)
                std::wstring trackName = ConvertString(track->Name);
                _NativeRenderer->DrawText(trackName, textBounds, COLOR_TO_COLOR_F(m_ColorTheme.Text), _NativeRenderer->GetTrackHeaderFormat());
            }

            // Draw track buttons
            DrawTrackButtons(track, TrackHeaderBounds);

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
        if (!_NativeRenderer || !this->_ToolAccessDelegate || this->_Tracks == nullptr || this->_Tracks->Count == 0) {
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
            float SividerY = CurrentY + T->Height + this->_ScrollPosition->Y;

            // Only draw if divider is in visible range
            if (SividerY >= 0 && SividerY <= (float)this->_Control->Height)
            {
                // Draw differently if this is the hover track
                if (T == hoverTrack && !ToolAccess->IsDragging && !ToolAccess->IsSelecting)
                {
                    // Draw a highlighted divider across the full width
                    _NativeRenderer->DrawLine(
                        0.0f, SividerY,
                        (float)this->_Control->Width, SividerY,
                        COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight),
                        2.0f  // Thicker line for hover state
                    );
                }
                else
                {
                    // Draw normal divider
                    _NativeRenderer->DrawLine(0.0f, SividerY, (float)this->_Control->Width, SividerY, COLOR_TO_COLOR_F(m_ColorTheme.MeasureLine), 1.0f);
                }
            }

            // Draw vertical divider between header and content
            float headerBottom = CurrentY + T->Height;
            if (headerBottom >= this->_ScrollPosition->Y && CurrentY <= this->_ScrollPosition->Y + (float)this->_Control->Height)
            {
                _NativeRenderer->DrawLine(TRACK_HEADER_WIDTH, CurrentY, TRACK_HEADER_WIDTH, headerBottom, COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);
            }

            CurrentY += T->Height;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewPointerTool()
    {
        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return false;
        }

        if(ToolAccess->SelectedBars != nullptr && ToolAccess->SelectedBars->Count > 0) {
            DrawToolPreviewPointerToolMoving();
        }

        DrawToolPreviewPointerToolPasting();


        // Draw selection rectangle if present
        DrawSelectionRectangle(ToolAccess->SelectionRect);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewPointerToolMoving()
    {
        if (!_NativeRenderer) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return false;
        }

        // Calculate visible range in ticks
        int VisibleStartTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int VisibleEndTick = (int)PixelsToTicks(-this->_ScrollPosition->X + this->_Control->Width);

		
		//for each (BarEvent^ Bar in ToolAccess->SelectedBars)
		//{
		//	if (Bar->EndTick < VisibleStartTick || Bar->StartTick > VisibleEndTick) {
		//		continue;
		//	}

		//	if (ToolAccess->IsDragging)
		//	{
		//		System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Bar->OriginalContainingTrack);
		//		TrackContentBounds.Y += this->_ScrollPosition->Y;
		//		
		//		DrawGhostBar(Bar, TrackContentBounds);
		//	}
		//	else
		//	{
		//		System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Bar->ContainingTrack);
		//		TrackContentBounds.Y += this->_ScrollPosition->Y;

		//		DrawSelectedBar(Bar, TrackContentBounds);
		//	}
		//}

		for each (BarEvent^ Bar in ToolAccess->PreviewBars)
		{
			if (Bar->EndTick < VisibleStartTick || Bar->StartTick > VisibleEndTick) {
				continue;
			}

			System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(Bar->ContainingTrack);
			TrackContentBounds.Y += this->_ScrollPosition->Y;

			DrawSelectedBar(Bar, TrackContentBounds);
		}

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewPointerToolPasting()
    {
        if (!_NativeRenderer) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return false;
        }

        if (ToolAccess->IsPasting && ToolAccess->PastePreviewBars != nullptr && ToolAccess->PastePreviewBars->Count > 0)
        {
            if (ToolAccess->CurrentMousePosition.X > TRACK_HEADER_WIDTH)
            {
                for each (BarEvent^ PasteBar in ToolAccess->PastePreviewBars)
                {
                    System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(PasteBar->ContainingTrack);
                    
                    DrawPastePreviewBar(PasteBar, TrackContentBounds);
                }
            }
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewDrawTool()
    {
        if (!_NativeRenderer) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->ToolAccess();
        if (!ToolAccess) {
            return false;
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

        return true;
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
        for each (BarEvent^ Bar in ToolAccess->PreviewBars)
        {
            Track^ ContainingTrack = Bar->ContainingTrack;

            if (ContainingTrack != nullptr)
            {
                System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(ContainingTrack);
                TrackContentBounds.Y += this->_ScrollPosition->Y;
                
                //DrawGhostBar(Bar, TrackContentBounds);
                DrawPreviewBar(Bar, ContainingTrack, Point(), BarPreviewType::Duration);
                DrawResizeHandle(GetBarBounds(Bar, TrackContentBounds), Bar == ToolAccess->HoveredBar);
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

        Track^				TargetTrack = ToolAccess->TargetTrack;
        List<BarEvent^>^	PreviewBars = ToolAccess->PreviewBars;
        System::Drawing::Point CurrentMousePos = ToolAccess->CurrentMousePosition;

        // Save current clip region
        D2D1_RECT_F ContentArea = D2D1::RectF(
            TRACK_HEADER_WIDTH,
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

        Track^				TargetTrack = ToolAccess->TargetTrack;
        List<BarEvent^>^	PreviewBars = ToolAccess->PreviewBars;
        System::Drawing::Point CurrentMousePos = ToolAccess->CurrentMousePosition;

        // Save current clip region
        D2D1_RECT_F contentArea = D2D1::RectF(
            TRACK_HEADER_WIDTH,
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
		DrawToolEnhancements(BarBounds);
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
        DrawBarGlowEffect(BarBounds, m_ColorTheme.SelectionHighlight, 6); // Was 2

		// Add tool-specific enhancements
		DrawToolEnhancements(BarBounds);
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

	void Timeline_Direct2DRenderer::DrawToolEnhancements(System::Drawing::Rectangle barBounds)
	{
		TimelineToolType CurrentTool = this->_ToolAccessDelegate->CurrentToolType();

		// Add tool-specific enhancements
		// 1. Draw Resize Handles on each bar if Duration Tool is selected
		if (CurrentTool == TimelineToolType::Duration)
		{
			DrawResizeHandle(barBounds, false);
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

    float Timeline_Direct2DRenderer::GetTotalTrackHeight()
    {
        float totalHeight = 0;

        for each (Track ^ track in this->_Tracks) {
            totalHeight += track->Height;
        }

        return totalHeight;
    }

    Track^ Timeline_Direct2DRenderer::GetTrackAtPoint(System::Drawing::Point point)
    {
		if (point.Y < HEADER_HEIGHT) {
			return nullptr;
		}

        int y = HEADER_HEIGHT + this->_ScrollPosition->Y;
        for each(Track^ track in this->_Tracks)
        {
            int height = track->Height;

            if (point.Y >= y && point.Y < y + height)
            {
                return track;
            }
            y += height;
        }

        return nullptr;
    }

    Measure^ Timeline_Direct2DRenderer::GetMeasureAtTick(int tick)
    {
        int accumulated = 0;
        
        for each(Measure^ measure in this->_Measures)
        {
            if (tick >= accumulated && tick < accumulated + measure->Length) {
                return measure;
            }
            accumulated += measure->Length;
        }

        return nullptr;
    }

    BarEvent^ Timeline_Direct2DRenderer::GetBarAtPoint(System::Drawing::Point point)
    {
        Track^ track = GetTrackAtPoint(point);
        
        if (track == nullptr) {
            return nullptr;
        }

        // Convert point to tick position
        int clickTick = (int)PixelsToTicks(point.X - TRACK_HEADER_WIDTH - this->_ScrollPosition->X);

        // Get track content bounds for vertical check
        System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(track);
        TrackContentBounds.Y += this->_ScrollPosition->Y;

        // Check each bar in the track
        for each(BarEvent ^ bar in track->Events)
        {
            // First check if the click is within the bar's time range
            if (clickTick >= bar->StartTick && clickTick <= bar->StartTick + bar->Duration)
            {
                // Then check if the click is within the track's vertical bounds
                if (point.Y >= TrackContentBounds.Y + TRACK_PADDING && point.Y <= TrackContentBounds.Y + TrackContentBounds.Height - TRACK_PADDING)
                {
                    return bar;
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
        Bounds.Width = TRACK_HEADER_WIDTH;

        return Bounds;
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetTrackContentBounds(Track^ track)
    {
        int top = GetTrackTop(track);
        int height = track->Height;

        return System::Drawing::Rectangle(
            TRACK_HEADER_WIDTH,     // Left starts after header
            top,                    // Top
            this->_Control->Width,  // Right is full width
            height                  // Bottom
        );
    }

    System::Drawing::Rectangle Timeline_Direct2DRenderer::GetBarBounds(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        int X = (int)TicksToPixels(bar->StartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;
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
        int X = (int)TicksToPixels(bar->OriginalStartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;
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
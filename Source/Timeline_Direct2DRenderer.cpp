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
    Timeline_Direct2DRenderer::Timeline_Direct2DRenderer(List<Track^>^ tracks, List<Measure^>^ measures, double zoomlevel, System::Drawing::Point^ scrollposition) : m_pNativeRenderer(nullptr), m_disposed(false), m_controlHandle(System::IntPtr::Zero), m_themeColorsCached(false)
    {
        // Create the native renderer
        m_pNativeRenderer = new Timeline_Direct2DRenderer_Native();

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
        m_controlHandle = control->Handle;

        // Initialize the native renderer
        return m_pNativeRenderer->Initialize((HWND)m_controlHandle.ToPointer());
    }

    void Timeline_Direct2DRenderer::Resize(int width, int height)
    {
        if (m_disposed || !m_pNativeRenderer) {
            return;
        }

        m_pNativeRenderer->ResizeRenderTarget(width, height);
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

    bool Timeline_Direct2DRenderer::BeginDraw()
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->BeginDraw();
    }

    bool Timeline_Direct2DRenderer::EndDraw()
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->EndDraw();
    }

    bool Timeline_Direct2DRenderer::DrawWidgetBackground()
    {
        if (!m_pNativeRenderer) {
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
        return m_pNativeRenderer->FillRectangle(widgetArea, COLOR_TO_COLOR_F(m_ColorTheme.Background));
    }

    bool Timeline_Direct2DRenderer::DrawTrackBackground()
    {
        if (!m_pNativeRenderer) {
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
        return m_pNativeRenderer->FillRectangle(tracksArea, COLOR_TO_COLOR_F(m_ColorTheme.TrackBackground));
    }

    bool Timeline_Direct2DRenderer::DrawMeasureNumbers()
    {
        if (!m_pNativeRenderer || this->_Measures == nullptr || this->_Measures->Count == 0) {
            return false;
        }

        // Fill the header background
        D2D1_RECT_F headerRect = D2D1::RectF(0, 0, (float)this->_Control->Width, HEADER_HEIGHT);
        m_pNativeRenderer->FillRectangle(headerRect, COLOR_TO_COLOR_F(m_ColorTheme.HeaderBackground));

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
                m_pNativeRenderer->DrawLine(
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
                    m_pNativeRenderer->DrawText(markerText, markerRect, d2dTextColor, m_pNativeRenderer->GetMarkerTextFormat());
                }

                // Draw measure number
                D2D1_RECT_F numberRect = D2D1::RectF(x - 25, measureNumberY, x + 25, measureNumberY + 14);

                std::wstring numText = std::to_wstring(measureNumber);
                m_pNativeRenderer->DrawText(numText, numberRect, d2dTextColor, m_pNativeRenderer->GetMeasureNumberFormat());

                // Draw time signature
                D2D1_RECT_F sigRect = D2D1::RectF(x - 25, timeSignatureY, x + 25, timeSignatureY + 14);

                std::wstring timeSignature = std::to_wstring(measure->Numerator) + L"/" + std::to_wstring(measure->Denominator);

                m_pNativeRenderer->DrawText(timeSignature, sigRect, d2dTextColor, m_pNativeRenderer->GetTimeSignatureFormat());
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
        if (!m_pNativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0 || this->_Measures == nullptr || this->_Measures->Count == 0 || !this->_ToolAccessDelegate) {
            return false;
        }

        // Save current clipping region
        D2D1_RECT_F contentArea = D2D1::RectF(
            0,
            (float)HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        m_pNativeRenderer->PushAxisAlignedClip(contentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try {
            // 1. Draw grid lines first (these should be visible through track content)
            float totalHeight = GetTotalTrackHeight();

            DrawGridLines(totalHeight);

            TimelineToolType currentTool = this->_ToolAccessDelegate->GetCurrentToolType();

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
            m_pNativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreview()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        // Save the original clip region
        D2D1_RECT_F contentArea = D2D1::RectF(
            0,
            HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        m_pNativeRenderer->PushAxisAlignedClip(contentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try {
            TimelineToolType currentTool = this->_ToolAccessDelegate->GetCurrentToolType();

            // Get current tool
            switch (currentTool)
            {
            case TimelineToolType::Draw:
                DrawToolPreviewDrawTool();
                break;
            case TimelineToolType::Erase:
                DrawToolPreviewEraseTool();
                break;
            case TimelineToolType::Duration:
                DrawToolPreviewDurationTool();
                break;
            case TimelineToolType::Color:
                DrawToolPreviewColorTool();
                break;
            case TimelineToolType::Fade:
                DrawToolPreviewFadeTool();
                break;
            case TimelineToolType::Strobe:
                DrawToolPreviewStrobeTool();
                break;
            }
        }
        finally {
            m_pNativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }


    bool Timeline_Direct2DRenderer::DrawBeatNumbers(Measure^ measure, float x, float measureNumberY, float subdivLevel, int measureNumber, int ticksPerBeat)
    {
        if (!m_pNativeRenderer) {
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
                IDWriteTextFormat* quarterNoteFormat = m_pNativeRenderer->GetQuarterNoteFormat();
                m_pNativeRenderer->DrawText(beatText, beatRect, beatColor, quarterNoteFormat);

                // Draw tick mark
                m_pNativeRenderer->DrawLine(beatX, HEADER_HEIGHT - 2, beatX, HEADER_HEIGHT, beatColor, 1.0f);
            }
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawGridLines(float totalHeight)
    {
        // Calculate visible range in ticks
        int startTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int endTick = (int)PixelsToTicks(-this->_ScrollPosition->X + this->_Control->Width - TRACK_HEADER_WIDTH);

        // Draw grid lines in proper order (back to front)
        DrawSubdivisionLines(totalHeight, startTick, endTick);
        DrawBeatLines(totalHeight, startTick, endTick);
        DrawMeasureLines(totalHeight, startTick, endTick);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawSubdivisionLines(float totalHeight, int startTick, int endTick)
    {
        float subdivLevel = GetSubdivisionLevel(TicksToPixels(TICKS_PER_QUARTER));
        if (subdivLevel <= 1) {
            return true;
        }

        int accumulated = 0;
        for each (MIDILightDrawer::Measure ^ measure in this->_Measures)
        {
            int measureStart = accumulated;
            int ticksPerBeat = TICKS_PER_QUARTER * 4 / measure->Denominator;
            int ticksPerSubdiv = ticksPerBeat / (int)subdivLevel;

            // Calculate subdivisions for this measure
            int subdivisions = (measure->Length / ticksPerSubdiv);

            for (int subdiv = 1; subdiv < subdivisions; subdiv++)
            {
                int subdivTick = measureStart + subdiv * ticksPerSubdiv;

                // Skip if this is already a beat or measure line
                if (subdivTick % ticksPerBeat == 0) {
                    continue;
                }

                if (subdivTick >= startTick && subdivTick <= endTick)
                {
                    float x = TicksToPixels(subdivTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;
                    m_pNativeRenderer->DrawLine(
                        x, HEADER_HEIGHT,
                        x, HEADER_HEIGHT + totalHeight,
                        COLOR_TO_RGBA(m_ColorTheme.SubdivisionLine),
                        1.0f
                    );
                }
            }

            accumulated += measure->Length;
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
                    m_pNativeRenderer->DrawLine(
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
                    m_pNativeRenderer->DrawLine(x, HEADER_HEIGHT, x, HEADER_HEIGHT + totalHeight, COLOR_TO_RGBA(m_ColorTheme.MeasureLine), 1.0f);
                }
            }

            accumulated += measure->Length;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackEvents(Track^ track, System::Drawing::Rectangle trackContentBounds, TimelineToolType currentToolType)
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        // Get tool access interface
        ITimelineToolAccess^ toolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!toolAccess) {
            return false;
        }

        // Calculate visible range in ticks
        int startTick = (int)PixelsToTicks(-this->_ScrollPosition->X);
        int endTick = (int)PixelsToTicks(-this->_ScrollPosition->X + trackContentBounds.Width);

        // Track if this track has any selected bars
        bool hasSelectedBars = (toolAccess->SelectedBars != nullptr && toolAccess->SelectedBars->Count > 0);
        bool isDragging = toolAccess->IsDragging;
        bool isMultiTrackDrag = false;

        // Check if we're doing a multi-track drag
        if (isDragging && hasSelectedBars)
        {
            Track^ firstTrack = nullptr;
            for each (Track ^ t in this->_Tracks)
            {
                for each (BarEvent ^ bar in t->Events)
                {
                    if (toolAccess->SelectedBars->Contains(bar))
                    {
                        if (firstTrack == nullptr) {
                            firstTrack = t;
                        }
                        else if (t != firstTrack) {
                            isMultiTrackDrag = true;
                            break;
                        }
                    }
                }
            }
        }

        // STEP 1: Draw non-selected bars
        for each (BarEvent ^ bar in track->Events)
        {
            if ((bar->StartTick + bar->Duration < startTick) || (bar->StartTick > endTick)) {
                continue;
            }

            bool isSelected = hasSelectedBars && toolAccess->SelectedBars->Contains(bar);
            if (!isSelected) {
                DrawNormalBar(bar, trackContentBounds);
            }
        }

        // STEP 2: Draw ghost bars during drag operations
        if (isDragging && hasSelectedBars)
        {
            if (isMultiTrackDrag)
            {
                // For multi-track selection, show ghosts in original positions
                for each (BarEvent ^ bar in track->Events)
                {
                    if (toolAccess->SelectedBars->Contains(bar)) {
                        DrawGhostBar(bar, trackContentBounds);
                    }
                }
            }
            else if (track == toolAccess->DragSourceTrack)
            {
                // For single-track selection, only show ghosts in source track
                for each (BarEvent ^ bar in toolAccess->SelectedBars) {
                    DrawGhostBar(bar, trackContentBounds);
                }
            }
        }

        // STEP 3: Draw selected/dragged bars
        if (hasSelectedBars && isDragging)
        {
            Point mousePos = toolAccess->CurrentMousePosition;
            bool isOverHeader = mousePos.X <= TRACK_HEADER_WIDTH;

            if (!isOverHeader)
            {
                if (isMultiTrackDrag)
                {
                    // For multi-track selection, show dragged bars in their original tracks
                    for each (BarEvent ^ bar in track->Events)
                    {
                        if (bar->StartTick + bar->Duration < startTick || bar->StartTick > endTick) {
                            continue;
                        }

                        if (toolAccess->SelectedBars->Contains(bar)) {
                            DrawSelectedBar(bar, trackContentBounds);
                        }
                    }
                }
                else if (track == toolAccess->DragTargetTrack)
                {
                    // For single-track selection, show all dragged bars in target track
                    for each (BarEvent ^ bar in toolAccess->SelectedBars) {
                        DrawSelectedBar(bar, trackContentBounds);
                    }
                }
            }
        }
        else if (hasSelectedBars)
        {
            // When not dragging, draw selected bars in their current tracks
            for each (BarEvent ^ bar in track->Events)
            {
                if (bar->StartTick + bar->Duration < startTick ||
                    bar->StartTick > endTick) {
                    continue;
                }

                if (toolAccess->SelectedBars->Contains(bar)) {
                    DrawSelectedBar(bar, trackContentBounds);
                }
            }
        }

        // Handle paste preview if active
        if (toolAccess->IsPasting && toolAccess->PastePreviewBars != nullptr)
        {
            if (track != nullptr && toolAccess->CurrentMousePosition.X > TRACK_HEADER_WIDTH)
            {
                int currentTrackIndex = this->_Tracks->IndexOf(track);

                for each (BarEvent ^ previewBar in toolAccess->PastePreviewBars)
                {
                    // Check if this preview bar belongs to the current track
                    // We stored the target track index in OriginalStartTick
                    if (previewBar->OriginalStartTick == currentTrackIndex)
                    {
                        DrawPastePreviewBar(previewBar, trackContentBounds);
                    }
                }

                // Draw drop target indicator if this is the anchor track
                if (track == toolAccess->DragTargetTrack)
                {
                    DrawDropTargetIndicator(trackContentBounds);
                }
            }
        }

        return true;
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
        if (!m_pNativeRenderer) {
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
        if (!m_pNativeRenderer) {
            return false;
        }

        // Calculate zoom-based scaling factors
        const float BASE_FONT_SIZE = 6.0f;
        const float FONT_SCALE_FACTOR = 2.0f;
        float scaledFontSize = BASE_FONT_SIZE + (logScale * FONT_SCALE_FACTOR);

        // Clamp font size between min and max values
        scaledFontSize = Math::Min(Math::Max(scaledFontSize, 4.0f), 18.0f);

        float availableHeight = (float)(trackContentBounds.Height - TRACK_PADDING * 2);

        TabStringInfo String_Info = DrawTablatureStrings(trackContentBounds, availableHeight, logScale, 6);

        if (String_Info.TotalHeight > availableHeight) {
            return true;
        }

        IDWriteTextFormat* TablatureTextFormat = m_pNativeRenderer->UpdateTablatureFormat(scaledFontSize);

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

                        float textWidth = m_pNativeRenderer->MeasureTextWidth(fretText, TablatureTextFormat);

                        D2D1_RECT_F textRect;
                        textRect.left = beatX - (textWidth / 2.0f);
                        textRect.top = String_Info.StringYPositions[note->String - 1] - (scaledFontSize / 2.0f);
                        textRect.right = textRect.left + textWidth;
                        textRect.bottom = textRect.top + scaledFontSize;

                        // Draw background for better readability
                        D2D1_RECT_F bgRect = D2D1::RectF(textRect.left - 1, textRect.top, textRect.right + 1, textRect.bottom - 1);

                        D2D1_COLOR_F bgColor = COLOR_TO_COLOR_F_A(m_ColorTheme.TrackBackground, 0.86f);

                        m_pNativeRenderer->FillRectangle(bgRect, bgColor);

                        // Draw the fret number
                        D2D1_COLOR_F textColor = COLOR_TO_COLOR_F(m_ColorTheme.Text);
                        m_pNativeRenderer->DrawText(fretText, textRect, textColor, TablatureTextFormat);
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
        if (!m_pNativeRenderer || beat->Notes == nullptr || beat->Notes->Count == 0 || beat->Duration <= 0) {
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
            m_pNativeRenderer->DrawLine(stemX, stemY + halfStemLength, stemX, stemEndY, stemColor, scaledLineThickness);
        }
        else if (duration < TICKS_PER_QUARTER * 2) // Quarter note or shorter
        {
            // Draw full stem
            m_pNativeRenderer->DrawLine(stemX, stemY, stemX, stemEndY, stemColor, scaledLineThickness);

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

                    m_pNativeRenderer->DrawLine(lineStartX, lineY, lineEndX, lineY, stemColor, scaledLineThickness);
                }
            }
        }

        if (beat->IsDotted)
        {
            float dotSize = Math::Min(2.0f + (logScale * 0.5f), 4.0f);
            float dotX = stemX + scaledLineLength + (dotSize * 2) - 10;
            float dotY = stemEndY - (numLines * scaledLineSpacing);

            D2D1_ELLIPSE dotEllipse = D2D1::Ellipse(D2D1::Point2F(dotX, dotY), dotSize, dotSize);
            m_pNativeRenderer->FillEllipse(dotEllipse, stemColor);
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
            m_pNativeRenderer->DrawText(tripletText, textRect, stemColor, m_pNativeRenderer->GetMeasureNumberFormat());
        }
    }

    void Timeline_Direct2DRenderer::DrawTieLines(Track^ track, System::Drawing::Rectangle trackContentBounds, array<float>^ stringYPositions, float scaledFontSize)
    {
        if (!m_pNativeRenderer || !track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0) {
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
                            IDWriteTextFormat* textFormat = m_pNativeRenderer->UpdateTablatureFormat(scaledFontSize);
                            if (!textFormat) {
                                continue;
                            }

                            float prevNoteWidth = m_pNativeRenderer->MeasureTextWidth(prevNoteText, textFormat);
                            float currentNoteWidth = m_pNativeRenderer->MeasureTextWidth(currentNoteText, textFormat);

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
                            m_pNativeRenderer->DrawTieLine(
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
        if (!m_pNativeRenderer) {
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
                m_pNativeRenderer->FillDiamond(points, 4, symbolColor);
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
                m_pNativeRenderer->DrawPolygon(points, 4, symbolColor, 1.0f);
                break;
            }

            case DrumNotationType::CircledX:
            {
                // Draw circle
                m_pNativeRenderer->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), size, size), symbolColor, 1.0f);

                // Draw X
                float xSize = size * 0.7f;
                m_pNativeRenderer->DrawLine(x - xSize, y - xSize, x + xSize, y + xSize, symbolColor, 1.0f);
                m_pNativeRenderer->DrawLine(x - xSize, y + xSize, x + xSize, y - xSize, symbolColor, 1.0f);
                break;
            }

            case DrumNotationType::AccentedX:
            {
                // Draw X
                float xSize = size * 0.8f;
                m_pNativeRenderer->DrawLine(x - xSize, y - xSize, x + xSize, y + xSize, symbolColor, 2.5f);
                m_pNativeRenderer->DrawLine(x - xSize, y + xSize, x + xSize, y - xSize, symbolColor, 2.5f);

                // Draw accent mark (^)
                D2D1_POINT_2F points[3] = {
                    D2D1::Point2F(x - size, y - size * 1.5f),
                    D2D1::Point2F(x, y - size * 2.0f),
                    D2D1::Point2F(x + size, y - size * 1.5f)
                };
                m_pNativeRenderer->DrawLines(points, 3, symbolColor, 2.5f);
                break;
            }

            case DrumNotationType::RegularX:
            {
                float xSize = size * 0.8f;
                m_pNativeRenderer->DrawLine(x - xSize, y - xSize, x + xSize, y + xSize, symbolColor, 1.0f);
                m_pNativeRenderer->DrawLine(x - xSize, y + xSize, x + xSize, y - xSize, symbolColor, 1.0f);
                break;
            }

            case DrumNotationType::NoteEllipse:
            {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), size * 0.7f, size * 0.5f);

                m_pNativeRenderer->FillEllipse(ellipse, symbolColor);
                break;
            }

            case DrumNotationType::Unknown:
            {
                D2D1_COLOR_F errorColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% opacity
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), size * 0.7f, size * 0.5f);

                m_pNativeRenderer->FillEllipse(ellipse, errorColor);
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
                m_pNativeRenderer->DrawLine((float)bounds.X, info.StringYPositions[i], (float)bounds.Right, info.StringYPositions[i], COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.7f), 1.0f);
            }
        }

        return info;
    }

    bool Timeline_Direct2DRenderer::DrawTrackHeaders()
    {
        if (!m_pNativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0) {
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

        m_pNativeRenderer->FillRectangle(headerBackground, COLOR_TO_COLOR_F(m_ColorTheme.HeaderBackground));

        // Draw each track's header
        float currentY = HEADER_HEIGHT;

        for each(Track ^ track in this->_Tracks)
        {
            // Calculate track bounds
            float trackHeight = static_cast<float>(track->Height);
            float yPosition = currentY + this->_ScrollPosition->Y;

            System::Drawing::RectangleF headerBounds = System::Drawing::RectangleF(0, yPosition, TRACK_HEADER_WIDTH, trackHeight);

            // Draw header background (with selection highlight if track is selected)
            Color bgColor = track->IsSelected ? m_ColorTheme.SelectionHighlight : m_ColorTheme.HeaderBackground;
            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(headerBounds), COLOR_TO_COLOR_F(bgColor));


            // Draw track name if present
            if (!String::IsNullOrEmpty(track->Name))
            {
                // Calculate text bounds with padding
                float textPadding = TRACK_PADDING;
                D2D1_RECT_F textBounds = D2D1::RectF(headerBounds.Left + textPadding, headerBounds.Top + textPadding, headerBounds.Right - textPadding, headerBounds.Bottom - textPadding);

                // Draw the track name using the measure number format (for now)
                std::wstring trackName = ConvertString(track->Name);
                m_pNativeRenderer->DrawText(trackName, textBounds, COLOR_TO_COLOR_F(m_ColorTheme.Text), m_pNativeRenderer->GetTrackHeaderFormat());
            }

            // Draw track buttons
            DrawTrackButtons(track, headerBounds);

            // Draw borders
            m_pNativeRenderer->DrawRectangle(RECT_TO_RECT_F(headerBounds), COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);
            //m_pNativeRenderer->DrawLine(TRACK_HEADER_WIDTH, headerBounds.top, TRACK_HEADER_WIDTH, headerBounds.bottom, COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);

            currentY += trackHeight;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackButtons(Track^ track, System::Drawing::RectangleF headerBounds)
    {
        // Calculate button dimensions
        const float BUTTON_SIZE = 24.0f;
        const float BUTTON_MARGIN = 6.0f;

        // Draw tablature toggle button
        System::Drawing::RectangleF buttonBounds = System::Drawing::RectangleF(
            headerBounds.Right - (BUTTON_SIZE + BUTTON_MARGIN),
            headerBounds.Top + BUTTON_MARGIN,
            BUTTON_SIZE,
            BUTTON_SIZE
        );

        // Draw button background with rounded corners
        DrawRoundedButtonBackground(buttonBounds, track->ShowTablature, false, m_ColorTheme.HeaderBackground);

        // Draw "T" text centered in button
        std::wstring buttonText = L"T";
        m_pNativeRenderer->DrawText(buttonText, RECT_TO_RECT_F(buttonBounds), COLOR_TO_COLOR_F(m_ColorTheme.Text), m_pNativeRenderer->GetMeasureNumberFormat());

        // Draw notation button for drum tracks
        if (track->IsDrumTrack)
        {
            buttonBounds = System::Drawing::RectangleF(
                buttonBounds.X - (BUTTON_SIZE + BUTTON_MARGIN),
                buttonBounds.Y,
                BUTTON_SIZE,
                BUTTON_SIZE);

            DrawRoundedButtonBackground(buttonBounds, track->ShowAsStandardNotation, false, m_ColorTheme.HeaderBackground);

            // Draw note icon or text here
            std::wstring noteText = L"N";  // Placeholder - would need actual icon drawing
            m_pNativeRenderer->DrawText(noteText, D2D1::RectF(buttonBounds.Left, buttonBounds.Top, buttonBounds.Right, buttonBounds.Bottom), COLOR_TO_COLOR_F(m_ColorTheme.Text), m_pNativeRenderer->GetMeasureNumberFormat());
        }

        return true;
    }

    void Timeline_Direct2DRenderer::DrawRoundedButtonBackground(System::Drawing::RectangleF bounds, bool isPressed, bool isHovered, System::Drawing::Color baseColor)
    {
        // Create rounded rectangle geometry
        const float cornerRadius = 6.0f;
        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(RECT_TO_RECT_F(bounds), cornerRadius, cornerRadius);

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
        m_pNativeRenderer->FillRoundedRectangle(roundedRect, buttonColor);

        // Draw border with slight transparency
        m_pNativeRenderer->DrawRoundedRectangle(roundedRect, COLOR_TO_COLOR_F_A(m_ColorTheme.Text, 0.3f), 1.0f);
    }

    bool Timeline_Direct2DRenderer::DrawTrackDividers(Track^ hoverTrack)
    {
        if (!m_pNativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0) {
            return false;
        }

        float currentY = HEADER_HEIGHT;

        // Draw dividers between tracks
        for each (Track ^ track in this->_Tracks)
        {
            // Calculate divider Y position
            float dividerY = currentY + track->Height + this->_ScrollPosition->Y;

            // Only draw if divider is in visible range
            if (dividerY >= 0 && dividerY <= (float)this->_Control->Height)
            {
                // Draw differently if this is the hover track
                if (track == hoverTrack)
                {
                    // Draw a highlighted divider across the full width
                    m_pNativeRenderer->DrawLine(
                        0.0f, dividerY,
                        (float)this->_Control->Width, dividerY,
                        COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight),
                        2.0f  // Thicker line for hover state
                    );
                }
                else
                {
                    // Draw normal divider
                    m_pNativeRenderer->DrawLine(0.0f, dividerY, (float)this->_Control->Width, dividerY, COLOR_TO_COLOR_F(m_ColorTheme.MeasureLine), 1.0f);
                }
            }

            // Draw vertical divider between header and content
            float headerBottom = currentY + track->Height;
            if (headerBottom >= this->_ScrollPosition->Y && currentY <= this->_ScrollPosition->Y + (float)this->_Control->Height)
            {
                m_pNativeRenderer->DrawLine(TRACK_HEADER_WIDTH, currentY, TRACK_HEADER_WIDTH, headerBottom, COLOR_TO_COLOR_F(m_ColorTheme.TrackBorder), 1.0f);
            }

            currentY += track->Height;
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewDrawTool()
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
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

    bool Timeline_Direct2DRenderer::DrawToolPreviewDrawToolDraw()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        // Draw preview bar if one exists
        if (ToolAccess->PreviewBar != nullptr)
        {
            DrawPreviewBar(ToolAccess->PreviewBar, nullptr, ToolAccess->CurrentMousePosition, BarPreviewType::Creation);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewDrawToolErase()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        BarEvent^ hoverBar = ToolAccess->HoverBar;
        if (hoverBar == nullptr) {
            return true;
        }

        // Get track containing hover bar
        Track^ track = nullptr;
        for each (Track ^ t in this->_Tracks)
        {
            if (t->Events->Contains(hoverBar))
            {
                track = t;
                break;
            }
        }

        if (track != nullptr)
        {
            System::Drawing::Rectangle bounds = GetTrackContentBounds(track);
            bounds.Y += this->_ScrollPosition->Y;

            System::Drawing::Rectangle BarBounds = GetBarBounds(hoverBar, bounds);

            // Draw delete preview with semi-transparent red
            D2D1_RECT_F DeleteRect = RECT_TO_RECT_F(BarBounds);
            D2D1_COLOR_F FillColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.4f);  // Red with 40% opacity
            D2D1_COLOR_F BorderColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% opacity

            // Fill rectangle
            m_pNativeRenderer->FillRectangle(DeleteRect, FillColor);

            // Draw border
            m_pNativeRenderer->DrawRectangle(DeleteRect, BorderColor, 2.0f);

            // Draw X
            m_pNativeRenderer->DrawLine((float)BarBounds.Left, (float)BarBounds.Top, (float)BarBounds.Right, (float)BarBounds.Bottom, BorderColor, 2.0f);
            m_pNativeRenderer->DrawLine((float)BarBounds.Left, (float)BarBounds.Bottom, (float)BarBounds.Right, (float)BarBounds.Top, BorderColor, 2.0f);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewDrawToolMove()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        BarEvent^ HoverBar = ToolAccess->HoverBar;
        if (HoverBar == nullptr || ToolAccess->IsMoving) {
            return true;
        }

        // Get track containing hover bar
        Track^ track = nullptr;
        for each (Track ^ t in this->_Tracks)
        {
            if (t->Events->Contains(HoverBar))
            {
                track = t;
                break;
            }
        }

        if (track != nullptr)
        {
            System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(track);
            TrackContentBounds.Y += this->_ScrollPosition->Y;

            System::Drawing::Rectangle BarBounds = GetBarBounds(HoverBar, TrackContentBounds);

            // Draw move preview
            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(BarBounds), COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.4f));
            m_pNativeRenderer->DrawRectangle(RECT_TO_RECT_F(BarBounds), COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight), 2.0f);

            // Draw move handles
            DrawMoveHandles(BarBounds);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewDrawToolResize()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        BarEvent^ HoverBar = ToolAccess->HoverBar;
        if (HoverBar == nullptr || ToolAccess->IsMoving) {
            return true;
        }

        // Get track containing hover bar
        Track^ track = nullptr;
        for each (Track ^ t in this->_Tracks)
        {
            if (t->Events->Contains(HoverBar))
            {
                track = t;
                break;
            }
        }

        if (track != nullptr)
        {
            System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(track);
            TrackContentBounds.Y += this->_ScrollPosition->Y;

            System::Drawing::Rectangle BarBounds = GetBarBounds(HoverBar, TrackContentBounds);

            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(BarBounds), COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.4f));

            // Draw resize handle with enhanced visibility
            System::Drawing::Rectangle HandleBounds(BarBounds.Right - 5, BarBounds.Y, 5, BarBounds.Height);   
            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(HandleBounds), COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight));
        
            // Add grip lines
            for (int i = 0; i < 3; i++)
            {
                m_pNativeRenderer->DrawLine((float)(HandleBounds.X + i + 1), (float)(HandleBounds.Y + 2), (float)(HandleBounds.X + i + 1), (float)(HandleBounds.Bottom - 2), COLOR_TO_COLOR_F(System::Drawing::Color::White), 1.0f);
            }
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewEraseTool()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        // Get the EraseTool's data
        List<BarEvent^>^ SelectedBars = ToolAccess->SelectedBars;
        BarEvent^ HoverBar = ToolAccess->HoverBar;

        // Draw selection rectangle if present
        System::Drawing::Rectangle SelectionRectangle = ToolAccess->SelectionRect;
        DrawSelectionRectangle(SelectionRectangle);

        // Check if we're hovering over a selected bar
        bool IsHoveringSelected = (HoverBar != nullptr && SelectedBars->Contains(HoverBar));

        // Draw all selected bars
        for each (BarEvent^ bar in SelectedBars)
        {
            Track^ track = nullptr;

            // Find the track containing this bar
            for each (Track^ t in this->_Tracks)
            {
                if (t->Events->Contains(bar))
                {
                    track = t;
                    break;
                }
            }

            if (track != nullptr)
            {
                System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(track);
                TrackContentBounds.Y += this->_ScrollPosition->Y;

                System::Drawing::Rectangle BarBounds = GetBarBounds(bar, TrackContentBounds);

                // Create D2D rect
                D2D1_RECT_F DeleteRect = RECT_TO_RECT_F(BarBounds);

                // Fill with light red
                D2D1_COLOR_F FillColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.4f);  // Red with 40% opacity
                m_pNativeRenderer->FillRectangle(DeleteRect, FillColor);

                // If hovering over any selected bar, show delete preview for all selected bars
                if (IsHoveringSelected)
                {
                    // Draw border and X with darker red
                    D2D1_COLOR_F BorderColor = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% opacity

                    // Draw border
                    m_pNativeRenderer->DrawRectangle(DeleteRect, BorderColor, 2.0f);

                    // Draw X
                    m_pNativeRenderer->DrawLine((float)BarBounds.Left, (float)BarBounds.Top, (float)BarBounds.Right, (float)BarBounds.Bottom, BorderColor, 2.0f);
                    m_pNativeRenderer->DrawLine((float)BarBounds.Left, (float)BarBounds.Bottom, (float)BarBounds.Right, (float)BarBounds.Top, BorderColor, 2.0f);
                }
            }
        }
        
        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewDurationTool()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        // Draw selection rectangle if active
        System::Drawing::Rectangle SelectionRectangle = ToolAccess->SelectionRect;
        DrawSelectionRectangle(SelectionRectangle);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewColorTool()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        // Get the ColorTool's specific data
        List<BarEvent^>^ SelectedBars = ToolAccess->SelectedBars;
        BarEvent^ HoverBar = ToolAccess->HoverBar;
        Color CurrentColor = ToolAccess->CurrentColor;
        System::Drawing::Rectangle PreviewRect = ToolAccess->PreviewRect;

        // Draw selection rectangle if present
        System::Drawing::Rectangle SelectionRectangle = ToolAccess->SelectionRect;
        DrawSelectionRectangle(SelectionRectangle);

        // Draw all selected bars with a highlight
        for each (BarEvent ^ bar in SelectedBars)
        {
            Track^ track = nullptr;
            // Find the track containing this bar
            for each (Track ^ t in this->_Tracks)
            {
                if (t->Events->Contains(bar))
                {
                    track = t;
                    break;
                }
            }

            if (track != nullptr)
            {
                System::Drawing::Rectangle TrackContentBounds = GetTrackContentBounds(track);
                TrackContentBounds.Y += this->_ScrollPosition->Y;

                System::Drawing::Rectangle BarBounds = GetBarBounds(bar, TrackContentBounds);
                D2D1_RECT_F BarRect = RECT_TO_RECT_F(BarBounds);

                m_pNativeRenderer->FillRectangle(BarRect, COLOR_TO_COLOR_F_A(CurrentColor, 0.4f));  // Draw selection highlight
                m_pNativeRenderer->DrawRectangle(BarRect, COLOR_TO_COLOR_F(CurrentColor), 2.0f);    // Draw preview outline

                // If this bar is being hovered over, show stronger highlight
                if (bar == HoverBar)
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
                        m_pNativeRenderer->DrawRectangle(GlowRect, GlowColor, 1.0f);
                    }
                }
            }
        }

        // Draw preview rectangle for hovered bar
        if (!PreviewRect.IsEmpty && !SelectedBars->Contains(HoverBar))
        {
            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(PreviewRect), COLOR_TO_COLOR_F_A(CurrentColor, 0.3f));  // Fill with semi-transparent preview color
            m_pNativeRenderer->DrawRectangle(RECT_TO_RECT_F(PreviewRect), COLOR_TO_COLOR_F(CurrentColor), 1.0f);    // Draw border
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewFadeTool()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        Track^ TargetTrack = ToolAccess->TargetTrack;
        List<BarEvent^>^ PreviewBars = ToolAccess->PreviewBars;
        BarEvent^ SinglePreview = ToolAccess->PreviewBar;
        System::Drawing::Point CurrentMousePos = ToolAccess->CurrentMousePosition;

        // Save current clip region
        D2D1_RECT_F ContentArea = D2D1::RectF(
            TRACK_HEADER_WIDTH,
            HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        m_pNativeRenderer->PushAxisAlignedClip(ContentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try
        {
            if (PreviewBars != nullptr && PreviewBars->Count > 0)
            {
                DrawPreviewBarList(PreviewBars, TargetTrack);
            }
            else if (SinglePreview != nullptr)
            {
                // Draw single preview bar with creation effect
                DrawPreviewBar(SinglePreview, nullptr, CurrentMousePos, BarPreviewType::Creation);
            }
        }
        finally
        {
            // Restore clip region
            m_pNativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawToolPreviewStrobeTool()
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
        if (!ToolAccess) {
            return false;
        }

        Track^ TargetTrack = ToolAccess->TargetTrack;
        List<BarEvent^>^ PreviewBars = ToolAccess->PreviewBars;
        BarEvent^ SinglePreview = ToolAccess->PreviewBar;
        System::Drawing::Point CurrentMousePos = ToolAccess->CurrentMousePosition;

        // Save current clip region
        D2D1_RECT_F contentArea = D2D1::RectF(
            TRACK_HEADER_WIDTH,
            HEADER_HEIGHT,
            (float)this->_Control->Width,
            (float)this->_Control->Height
        );

        m_pNativeRenderer->PushAxisAlignedClip(contentArea, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        try
        {
            if (PreviewBars != nullptr && PreviewBars->Count > 0)
            {
                DrawPreviewBarList(PreviewBars, TargetTrack);
            }
            else if (SinglePreview != nullptr)
            {
                // Draw single preview bar with creation effect
                DrawPreviewBar(SinglePreview, nullptr, CurrentMousePos, BarPreviewType::Creation);
            }
        }
        finally
        {
            // Restore clip region
            m_pNativeRenderer->PopAxisAlignedClip();
        }

        return true;
    }


    bool Timeline_Direct2DRenderer::DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer || !this->_ToolAccessDelegate) {
            return false;
        }

        // Get the bar bounds
        System::Drawing::Rectangle BarBounds = GetBarBounds(bar, bounds);

        bool IsDurationTarget = false;

        TimelineToolType CurrentTool = this->_ToolAccessDelegate->GetCurrentToolType();

        // Check if this bar is a duration tool target
        if (CurrentTool == TimelineToolType::Duration)
        {
            ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();

            IsDurationTarget = (bar == ToolAccess->PreviewBar);

            // If this is a duration target and preview is visible, handle it differently
            if (IsDurationTarget && ToolAccess->IsPreviewVisible)
            {
                DrawPreviewBar(bar, nullptr, Point(), BarPreviewType::Duration);
                return true;
            }
        }

        // Draw the bar with semi-transparency
        D2D1_RECT_F BarRect = RECT_TO_RECT_F(BarBounds);

        if (IsDurationTarget)
        {
            // Highlight the bar being targeted for duration change
            // Make the color slightly brighter
            D2D1_COLOR_F highlightColor = D2D1::ColorF(
                Math::Min(1.0f, (bar->Color.R / 255.0f) + 0.12f),
                Math::Min(1.0f, (bar->Color.G / 255.0f) + 0.12f),
                Math::Min(1.0f, (bar->Color.B / 255.0f) + 0.12f),
                1.0f
            );
            m_pNativeRenderer->FillRectangle(BarRect, highlightColor);

            // Draw highlight border
            m_pNativeRenderer->DrawRectangle(BarRect, COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight), 2.0f);
        }
        else
        {
            // Normal appearance with 80% opacity
            m_pNativeRenderer->FillRectangle(BarRect, COLOR_TO_COLOR_F_A(bar->Color, 0.8f));

            // Draw border with slight transparency
            m_pNativeRenderer->DrawRectangle(BarRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.4f), 1.0f);
        }

        // Add tool-specific enhancements
        if (CurrentTool == TimelineToolType::Draw)
        {
            ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();
            
            if (ToolAccess->CurrentMode == DrawToolMode::Resize && bar == ToolAccess->HoverBar)
            {
                DrawResizeHandle(BarBounds, m_ColorTheme.SelectionHighlight, false);
            }
        }
        else if (CurrentTool == TimelineToolType::Duration)
        {
            // Create handle color based on target state
            Color handleColor = IsDurationTarget ?
                m_ColorTheme.SelectionHighlight :
                Color::FromArgb(
                    bar->Color.R * 8 / 10,
                    bar->Color.G * 8 / 10,
                    bar->Color.B * 8 / 10
                );

            DrawResizeHandle(BarBounds, handleColor, IsDurationTarget);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawPreviewBar(BarEvent^ bar, Track^ track, System::Drawing::Point mousePos, BarPreviewType previewType)
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
            case BarPreviewType::Movement:
                return DrawCreationMovementPreview(bar, BarBounds);

            case BarPreviewType::Duration:
                return DrawDurationPreview(bar, BarBounds);
            }
        }

        return false;
    }

    bool Timeline_Direct2DRenderer::DrawPreviewBarList(List<BarEvent^>^ bars, Track^ track)
    {
        if (bars == nullptr || track == nullptr) {
            return false;
        }

        for each (BarEvent^ bar in bars)
        {
            // Get track bounds
            System::Drawing::Rectangle bounds = GetTrackContentBounds(track);
            bounds.Y += this->_ScrollPosition->Y;

            DrawPreviewBar(bar, track, Point(), BarPreviewType::Creation);
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawCreationMovementPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        // Semi-transparent preview with border
        m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(bar->Color));
        m_pNativeRenderer->DrawRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F_A(bar->Color, 0.7f), 1.0f);
        
        // Add subtle glow effect
        DrawBarGlowEffect(barBounds, Color::FromArgb(100, bar->Color), 2);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawDurationPreview(BarEvent^ bar, System::Drawing::Rectangle barBounds)
    {
        // Draw original bar with reduced opacity
        System::Drawing::Rectangle OriginalBarBounds = barBounds;
        OriginalBarBounds.Width = (int)TicksToPixels(bar->Duration);
        
        m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(OriginalBarBounds), COLOR_TO_COLOR_F_A(bar->Color, 0.3f));

        // Draw preview with enhanced brightness
        Color PreviewColor = Color::FromArgb(
            Math::Min(255, bar->Color.R + 40),
            Math::Min(255, bar->Color.G + 40),
            Math::Min(255, bar->Color.B + 40)
        );

        m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(barBounds), COLOR_TO_COLOR_F(PreviewColor));

        // Add glow effect for the preview
        DrawBarGlowEffect(barBounds, this->m_ColorTheme.SelectionHighlight, 3);
        
        return true;
    }

    bool Timeline_Direct2DRenderer::DrawGhostBar(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        int x = (int)TicksToPixels(bar->OriginalStartTick) + _ScrollPosition->X + TRACK_HEADER_WIDTH;
        int width = (int)TicksToPixels(bar->Duration);

        // Ensure minimum width of 1 pixel
        width = Math::Max(1, width);

        D2D1_RECT_F barRect = D2D1::RectF((float)x, (float)(bounds.Y + TRACK_PADDING), (float)(x + width), (float)(bounds.Bottom - TRACK_PADDING));

        // Draw with high transparency (ghost effect)
        m_pNativeRenderer->FillRectangle(barRect, COLOR_TO_COLOR_F_A(bar->Color, 0.3f));

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawSelectedBar(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        // Calculate bar position and bounds
        System::Drawing::Rectangle BarBounds = GetBarBounds(bar, bounds);

        // Check for duration preview
        bool IsDurationTarget = false;
        bool IsDurationPreview = false;

        TimelineToolType CurrentTool = this->_ToolAccessDelegate->GetCurrentToolType();

        if (CurrentTool == TimelineToolType::Duration)
        {
            ITimelineToolAccess^ ToolAccess = this->_ToolAccessDelegate->GetToolAccess();

            IsDurationTarget = (bar == ToolAccess->PreviewBar);

            if (IsDurationTarget && ToolAccess->IsPreviewVisible)
            {
                int Width = (int)TicksToPixels(ToolAccess->PreviewLength);
                IsDurationPreview = true;

                BarBounds.Width = Width;
            }
        }

        if (IsDurationPreview)
        {
            DrawDurationPreview(bar, BarBounds);
        }
        else
        {
            // Draw bar with enhanced brightness
            D2D1_COLOR_F FillColor;
            if (IsDurationTarget)
            {
                // Brighter color for duration target
                FillColor = D2D1::ColorF(
                    Math::Min(1.0f, (bar->Color.R / 255.0f) + 0.15f),
                    Math::Min(1.0f, (bar->Color.G / 255.0f) + 0.15f),
                    Math::Min(1.0f, (bar->Color.B / 255.0f) + 0.15f),
                    1.0f
                );
            }
            else
            {
                FillColor = COLOR_TO_COLOR_F(bar->Color);
            }

            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(BarBounds), FillColor);                             // Fill the bar
            m_pNativeRenderer->DrawRectangle(RECT_TO_RECT_F(BarBounds), D2D1::ColorF(0, 0, 0, 0.4f), 1.0f);     // Draw border

            // Add glow effect
            DrawBarGlowEffect(BarBounds, m_ColorTheme.SelectionHighlight, IsDurationTarget ? 3 : 2);

            // Add duration tool handle if needed
            if (CurrentTool == TimelineToolType::Duration)
            {
                Color HandleColor = IsDurationTarget ?
                    this->m_ColorTheme.SelectionHighlight :
                    Color::FromArgb(
                        Math::Min(255, bar->Color.R * 13 / 10),
                        Math::Min(255, bar->Color.G * 13 / 10),
                        Math::Min(255, bar->Color.B * 13 / 10)
                    );

                DrawResizeHandle(BarBounds, HandleColor, IsDurationTarget);
            }
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawPastePreviewBar(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        int x = (int)TicksToPixels(bar->StartTick) + _ScrollPosition->X + TRACK_HEADER_WIDTH;
        int width = (int)TicksToPixels(bar->Duration);

        width = Math::Max(1, width);

        D2D1_RECT_F barRect = D2D1::RectF((float)x, (float)(bounds.Y + TRACK_PADDING), (float)(x + width), (float)(bounds.Bottom - TRACK_PADDING));

        // Draw shadow effect
        for (int i = 3; i >= 0; i--)
        {
            D2D1_RECT_F ShadowRect = D2D1::RectF(barRect.left + i, barRect.top + i, barRect.right + i, barRect.bottom + i);
            D2D1_COLOR_F ShadowColor = D2D1::ColorF(0, 0, 0, (0.4f - (i * 0.1f)));
            
            m_pNativeRenderer->FillRectangle(ShadowRect, ShadowColor);
        }

        // Draw semi-transparent bar
        D2D1_COLOR_F previewColor = D2D1::ColorF(
            bar->Color.R / 255.0f,
            bar->Color.G / 255.0f,
            bar->Color.B / 255.0f,
            0.7f
        );
        m_pNativeRenderer->FillRectangle(barRect, previewColor);

        // Draw glowing border
        DrawBarGlowEffect(System::Drawing::Rectangle(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2), Color::FromArgb(180, 255, 255, 255), 2);

        // Draw dashed outline
        D2D1_COLOR_F dashColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);

        // Create dashed stroke style
        float dashes[] = { 4.0f, 4.0f };
        m_pNativeRenderer->DrawLineDashed(barRect.left, barRect.top, barRect.right, barRect.top, dashColor, 1.0f, dashes, 2);
        m_pNativeRenderer->DrawLineDashed(barRect.right, barRect.top, barRect.right, barRect.bottom, dashColor, 1.0f, dashes, 2);
        m_pNativeRenderer->DrawLineDashed(barRect.right, barRect.bottom, barRect.left, barRect.bottom, dashColor, 1.0f, dashes, 2);
        m_pNativeRenderer->DrawLineDashed(barRect.left, barRect.bottom, barRect.left, barRect.top, dashColor, 1.0f, dashes, 2);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawDropTargetIndicator(System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        D2D1_RECT_F targetRect      = D2D1::RectF((float)bounds.X, (float)bounds.Y, (float)bounds.Right, (float)bounds.Bottom);
        D2D1_COLOR_F targetColor    = COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight);

        // Draw dashed border
        float dashes[] = { 6.0f, 3.0f };
        m_pNativeRenderer->DrawLineDashed(targetRect.left, targetRect.top, targetRect.right, targetRect.top, targetColor, 2.0f, dashes, 2);
        m_pNativeRenderer->DrawLineDashed(targetRect.right, targetRect.top, targetRect.right, targetRect.bottom, targetColor, 2.0f, dashes, 2);
        m_pNativeRenderer->DrawLineDashed(targetRect.right, targetRect.bottom, targetRect.left, targetRect.bottom, targetColor, 2.0f, dashes, 2);
        m_pNativeRenderer->DrawLineDashed(targetRect.left, targetRect.bottom, targetRect.left, targetRect.top, targetColor, 2.0f, dashes, 2);

        return true;
    }

    void Timeline_Direct2DRenderer::DrawMoveHandles(System::Drawing::Rectangle barBounds)
    {
        // Draw move arrows or handle indicators at the edges
        const float arrowSize = 6.0f;
        D2D1_COLOR_F HandleColor = COLOR_TO_COLOR_F(this->m_ColorTheme.SelectionHighlight);

        // Draw left arrow
        D2D1_POINT_2F leftArrow[3] = {
            D2D1::Point2F(barBounds.Left + arrowSize, barBounds.Top + (barBounds.Bottom - barBounds.Top) / 2 - arrowSize),
            D2D1::Point2F(barBounds.Left, barBounds.Top + (barBounds.Bottom - barBounds.Top) / 2),
            D2D1::Point2F(barBounds.Left + arrowSize, barBounds.Top + (barBounds.Bottom - barBounds.Top) / 2 + arrowSize)
        };

        // Draw right arrow
        D2D1_POINT_2F rightArrow[3] = {
            D2D1::Point2F(barBounds.Right - arrowSize, barBounds.Top + (barBounds.Bottom - barBounds.Top) / 2 - arrowSize),
            D2D1::Point2F(barBounds.Right, barBounds.Top + (barBounds.Bottom - barBounds.Top) / 2),
            D2D1::Point2F(barBounds.Right - arrowSize, barBounds.Top + (barBounds.Bottom - barBounds.Top) / 2 + arrowSize)
        };

        // Fill the arrows
        
        //////////
        // ToDo //
        //////////

        //m_pNativeRenderer->FillPolygon(leftArrow, 3, d2dHandleColor);
        //m_pNativeRenderer->FillPolygon(rightArrow, 3, d2dHandleColor);
    }

    void Timeline_Direct2DRenderer::DrawBarGlowEffect(System::Drawing::Rectangle barBounds, System::Drawing::Color glowColor, int glowLevels)
    {
        if (!m_pNativeRenderer) {
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

            m_pNativeRenderer->DrawRectangle(glowRect, glowD2DColor, 1.0f);
        }
    }

    void Timeline_Direct2DRenderer::DrawResizeHandle(System::Drawing::Rectangle barBounds, System::Drawing::Color handleColor, bool isTargeted)
    {
        if (!m_pNativeRenderer) {
            return;
        }

        System::Drawing::Rectangle HandleBounds(barBounds.Right - 5, barBounds.Y, 5, barBounds.Height);

        // Fill handle rectangle
        m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(HandleBounds), COLOR_TO_COLOR_F(handleColor));

        // Add grip lines
        D2D1_COLOR_F GripColor = isTargeted ?
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f) :  // White for targeted
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.8f);   // Slightly transparent white

        float StrokeWidth = isTargeted ? 2.0f : 1.0f;

        // Draw three vertical grip lines
        for (int i = 0; i < 3; i++)
        {
            float x = (float)(HandleBounds.Left + i + 1);
            m_pNativeRenderer->DrawLine(
                x, (float)(HandleBounds.Top + 2),
                x, (float)(HandleBounds.Bottom - 2),
                GripColor,
                StrokeWidth
            );
        }

        // Add glow effect for targeted handles
        if (isTargeted) {
            DrawBarGlowEffect(barBounds, m_ColorTheme.SelectionHighlight, 3);
        }
    }

    void Timeline_Direct2DRenderer::DrawSelectionRectangle(System::Drawing::Rectangle selectionRectangle)
    {
        if (selectionRectangle.Width > 0 && selectionRectangle.Height > 0)
        {
            // Fill with semi-transparent selection color
            m_pNativeRenderer->FillRectangle(RECT_TO_RECT_F(selectionRectangle), COLOR_TO_COLOR_F_A(m_ColorTheme.SelectionHighlight, 0.2f));

            // Draw border
            m_pNativeRenderer->DrawRectangle(RECT_TO_RECT_F(selectionRectangle), COLOR_TO_COLOR_F(m_ColorTheme.SelectionHighlight), 1.0f);
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
        if (point.Y < HEADER_HEIGHT) return nullptr;

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
            if (m_pNativeRenderer)
            {
                m_pNativeRenderer->Cleanup();
                delete m_pNativeRenderer;
                m_pNativeRenderer = nullptr;
            }
            m_disposed = true;
        }
    }

} // namespace MIDILightDrawer
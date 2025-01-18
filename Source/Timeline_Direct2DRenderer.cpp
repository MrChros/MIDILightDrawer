#include "Timeline_Direct2DRenderer.h"
#include "Timeline_Direct2DRenderer_Native.h"

#include "Widget_Timeline_Common.h"
#include "Widget_Timeline_Classes.h"

#include <vcclr.h>
#include <msclr\marshal_cppstd.h>

#define COLOR_TO_RGBA(__color__)        __color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, __color__.A / 255.0f
#define COLOR_TO_COLOR_F(__color__)     D2D1::ColorF(__color__.R / 255.0f, __color__.G / 255.0f, __color__.B / 255.0f, __color__.A / 255.0f)
#define RECT_TO_RECT_F(__rect__)        D2D1::RectF(__rect__.Left, __rect__.Top, __rect__.Right, __rect__.Bottom)

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
                int trackTop = GetTrackTop(track) + this->_ScrollPosition->Y;
                int trackHeight = track->Height;
                int trackBottom = trackTop + trackHeight;

                // Skip if track is not visible
                if (trackBottom < HEADER_HEIGHT || trackTop > this->_Control->Height) {
                    continue;
                }

                System::Drawing::Rectangle contentBounds(TRACK_HEADER_WIDTH, trackTop, this->_Control->Width - TRACK_HEADER_WIDTH, trackHeight);

                // Draw events and tablature
                DrawTrackEvents(track, contentBounds, currentTool);

                if (track->ShowTablature) {
                    //DrawTrackTablature(track, contentBounds, currentTheme.Text);
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

    bool Timeline_Direct2DRenderer::DrawTrackEvents(Track^ track, System::Drawing::Rectangle contentBounds, TimelineToolType currentToolType)
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
        int endTick = (int)PixelsToTicks(-this->_ScrollPosition->X + contentBounds.Width);

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
                DrawNormalBar(bar, contentBounds);
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
                        DrawGhostBar(bar, contentBounds);
                    }
                }
            }
            else if (track == toolAccess->DragSourceTrack)
            {
                // For single-track selection, only show ghosts in source track
                for each (BarEvent ^ bar in toolAccess->SelectedBars) {
                    DrawGhostBar(bar, contentBounds);
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
                            DrawSelectedBar(bar, contentBounds);
                        }
                    }
                }
                else if (track == toolAccess->DragTargetTrack)
                {
                    // For single-track selection, show all dragged bars in target track
                    for each (BarEvent ^ bar in toolAccess->SelectedBars) {
                        DrawSelectedBar(bar, contentBounds);
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
                    DrawSelectedBar(bar, contentBounds);
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
                        DrawPastePreviewBar(previewBar, contentBounds);
                    }
                }

                // Draw drop target indicator if this is the anchor track
                if (track == toolAccess->DragTargetTrack)
                {
                    DrawDropTargetIndicator(contentBounds);
                }
            }
        }

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawTrackTablature(Track^ track, System::Drawing::Rectangle contentBounds, System::Drawing::Color stringColor)
    {
        return true;
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

        for each (Track ^ track in this->_Tracks)
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
        D2D1_COLOR_F borderColor = COLOR_TO_COLOR_F(m_ColorTheme.Text);
        borderColor.a = 0.3f;    // 30% opacity
        ;
        m_pNativeRenderer->DrawRoundedRectangle(roundedRect, borderColor, 1.0f);
    }

    bool Timeline_Direct2DRenderer::DrawTrackDividers(Track^ hoverTrack)
    {
        if (!m_pNativeRenderer || this->_Tracks == nullptr || this->_Tracks->Count == 0)
            return false;

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

    bool Timeline_Direct2DRenderer::DrawNormalBar(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        // Calculate bar position and bounds
        int x = (int)TicksToPixels(bar->StartTick) + this->_ScrollPosition->X + TRACK_HEADER_WIDTH;
        int width = (int)TicksToPixels(bar->Duration);

        // Ensure minimum width of 1 pixel
        width = Math::Max(1, width);

        // Create bar rectangle
        D2D1_RECT_F barRect = D2D1::RectF((float)x, (float)(bounds.Y + TRACK_PADDING), (float)(x + width), (float)(bounds.Bottom - TRACK_PADDING));

        // Draw bar with semi-transparency
        D2D1_COLOR_F barColor = COLOR_TO_COLOR_F(bar->Color);
        barColor.a = 0.8f;  // 80% opacity

        // Fill bar
        m_pNativeRenderer->FillRectangle(barRect, barColor);

        // Draw border with slight transparency
        D2D1_COLOR_F borderColor = D2D1::ColorF(0, 0, 0, 0.4f);
        m_pNativeRenderer->DrawRectangle(barRect, borderColor, 1.0f);

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
        D2D1_COLOR_F ghostColor = COLOR_TO_COLOR_F(bar->Color);
        ghostColor.a = 0.3f;  // 30% opacity

        m_pNativeRenderer->FillRectangle(barRect, ghostColor);

        return true;
    }

    bool Timeline_Direct2DRenderer::DrawSelectedBar(BarEvent^ bar, System::Drawing::Rectangle bounds)
    {
        if (!m_pNativeRenderer) {
            return false;
        }

        int x = (int)TicksToPixels(bar->StartTick) + _ScrollPosition->X + TRACK_HEADER_WIDTH;
        int width = (int)TicksToPixels(bar->Duration);

        width = Math::Max(1, width);

        D2D1_RECT_F barRect = D2D1::RectF((float)x, (float)(bounds.Y + TRACK_PADDING), (float)(x + width), (float)(bounds.Bottom - TRACK_PADDING));

        // Draw bar with enhanced brightness
        D2D1_COLOR_F fillColor = D2D1::ColorF(
            Math::Min(1.0f, (bar->Color.R / 255.0f) + 0.2f),
            Math::Min(1.0f, (bar->Color.G / 255.0f) + 0.2f),
            Math::Min(1.0f, (bar->Color.B / 255.0f) + 0.2f),
            1.0f
        );

        m_pNativeRenderer->FillRectangle(barRect, fillColor);

        // Draw border
        D2D1_COLOR_F borderColor = D2D1::ColorF(0, 0, 0, 0.4f);
        m_pNativeRenderer->DrawRectangle(barRect, borderColor, 1.0f);

        // Add glow effect
        DrawBarGlowEffect(System::Drawing::Rectangle(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2), m_ColorTheme.SelectionHighlight, 2);

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
            D2D1_RECT_F shadowRect = D2D1::RectF(barRect.left + i, barRect.top + i, barRect.right + i, barRect.bottom + i);

            D2D1_COLOR_F shadowColor = D2D1::ColorF(0, 0, 0, (0.4f - (i * 0.1f)));
            m_pNativeRenderer->FillRectangle(shadowRect, shadowColor);
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

    bool Timeline_Direct2DRenderer::DrawLine(float x1, float y1, float x2, float y2, System::Drawing::Color color, float strokeWidth)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawLine(
            x1, y1, x2, y2,
            color.R / 255.0f,
            color.G / 255.0f,
            color.B / 255.0f,
            color.A / 255.0f,
            strokeWidth
        );
    }

    bool Timeline_Direct2DRenderer::DrawRectangle(System::Drawing::RectangleF rect, System::Drawing::Color color, float strokeWidth)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawRectangle(
            rect.Left, rect.Top, rect.Right, rect.Bottom,
            color.R / 255.0f,
            color.G / 255.0f,
            color.B / 255.0f,
            color.A / 255.0f,
            strokeWidth
        );
    }

    bool Timeline_Direct2DRenderer::FillRectangle(System::Drawing::RectangleF rect, System::Drawing::Color color)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->FillRectangle(
            rect.Left, rect.Top, rect.Right, rect.Bottom,
            color.R / 255.0f,
            color.G / 255.0f,
            color.B / 255.0f,
            color.A / 255.0f
        );
    }

    bool Timeline_Direct2DRenderer::DrawGridLine(float x1, float y1, float x2, float y2, System::Drawing::Color color, bool isDashed)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawLine(x1, y1, x2, y2, COLOR_TO_RGBA(color), 1.0f);
    }

    bool Timeline_Direct2DRenderer::DrawMeasureLine(float x, float y1, float y2, System::Drawing::Color color)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawLine(x, y1, x, y2, COLOR_TO_RGBA(color), 1.0f);
    }

    bool Timeline_Direct2DRenderer::DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float strokeWidth)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawLine(x1, y1, x2, y2, r, g, b, a, strokeWidth);
    }

    bool Timeline_Direct2DRenderer::DrawRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a, float strokeWidth)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawRectangle(left, top, right, bottom, r, g, b, a, strokeWidth);
    }

    bool Timeline_Direct2DRenderer::FillRectangle(float left, float top, float right, float bottom, float r, float g, float b, float a)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->FillRectangle(left, top, right, bottom, r, g, b, a);
    }

    bool Timeline_Direct2DRenderer::DrawGridLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, bool isDashed)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawLine(x1, y1, x2, y2, r, g, b, a, 1.0f);
    }

    bool Timeline_Direct2DRenderer::DrawMeasureLine(float x, float y1, float y2, float r, float g, float b, float a)
    {
        if (m_disposed || !m_pNativeRenderer)
            return false;

        return m_pNativeRenderer->DrawLine(x, y1, x, y2, r, g, b, a, 1.0f);
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
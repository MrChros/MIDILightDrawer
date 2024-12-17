#include "Widget_Timeline.h"
#include "Widget_Timeline_Tools.h"

namespace MIDILightDrawer
{
	// Widget_Timeline Implementation
	Widget_Timeline::Widget_Timeline()
	{
		InitializeComponent();

		this->SetStyle(ControlStyles::Selectable, true);
		this->TabStop = true;
		this->Focus();

		// Set up double buffering
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::OptimizedDoubleBuffer, true);

		// Initialize state
		currentTheme = CreateDarkTheme();
		zoomLevel = 1.0;
		scrollPosition = gcnew Point(0, 0);
		tracks = gcnew List<Track^>();
		measures = gcnew List<Measure^>();
		trackHeights = gcnew Dictionary<Track^, int>();

		InitializeToolSystem();
		InitializeTablatureResources();

		// Default time signature 4/4
		currentTimeSignatureNumerator = 4;
		currentTimeSignatureDenominator = 4;

		// Initialize fonts
		measureFont = gcnew Drawing::Font("Arial", 10);
		markerFont = gcnew Drawing::Font("Arial", 9);

		// Set background color
		this->BackColor = currentTheme.Background;

		// Buffer will be initialized in OnHandleCreated
		bufferContext = nullptr;
		graphicsBuffer = nullptr;

		viewState			= gcnew ViewState();
		resourceManager		= gcnew TimelineResourceManager();
		visibilityTracker	= gcnew EnhancedVisibilityTracker();
		performanceMetrics	= gcnew PerformanceMetrics();
	}

	Widget_Timeline::~Widget_Timeline() {
		delete cachedTabFont;
		delete cachedStringPen;
		delete cachedDurationPen;
		delete cachedTextBrush;
		delete viewState;
		delete visibilityTracker;
		delete performanceMetrics;
	}

	void Widget_Timeline::InitializeComponent()
	{
		// Initialize horizontal scrollbar
		hScrollBar = gcnew System::Windows::Forms::HScrollBar();
		hScrollBar->Dock = System::Windows::Forms::DockStyle::Bottom;
		hScrollBar->Height = System::Windows::Forms::SystemInformation::HorizontalScrollBarHeight;
		hScrollBar->Scroll += gcnew ScrollEventHandler(this, &Widget_Timeline::OnScroll);

		// Initialize vertical scrollbar
		vScrollBar = gcnew System::Windows::Forms::VScrollBar();
		vScrollBar->Dock = System::Windows::Forms::DockStyle::Right;
		vScrollBar->Width = System::Windows::Forms::SystemInformation::VerticalScrollBarWidth;
		vScrollBar->Scroll += gcnew ScrollEventHandler(this, &Widget_Timeline::OnVerticalScroll);

		// Add scrollbars to control
		this->Controls->Add(vScrollBar);
		this->Controls->Add(hScrollBar);
	}

	void Widget_Timeline::InitializeToolSystem()
	{
		tools = gcnew Dictionary<TimelineToolType, TimelineTool^>();

		// Create tools
		tools->Add(TimelineToolType::Pointer	, gcnew PointerTool	(this));
		tools->Add(TimelineToolType::Draw		, gcnew DrawTool	(this));
		tools->Add(TimelineToolType::Split		, gcnew SplitTool	(this));
		tools->Add(TimelineToolType::Erase		, gcnew EraseTool	(this));
		tools->Add(TimelineToolType::Duration	, gcnew DurationTool(this));
		tools->Add(TimelineToolType::Color		, gcnew ColorTool	(this));

		// Set default tool
		currentToolType = TimelineToolType::Pointer;
		currentTool = tools[currentToolType];
	}

	void Widget_Timeline::InitializeTablatureResources()
	{
		if (cachedTabFont != nullptr) delete cachedTabFont;
		if (cachedStringPen != nullptr) delete cachedStringPen;
		if (cachedDurationPen != nullptr) delete cachedDurationPen;
		if (cachedTextBrush != nullptr) delete cachedTextBrush;

		cachedTabFont = gcnew Drawing::Font("Arial", 10, FontStyle::Regular);
		cachedStringPen = gcnew Pen(Color::FromArgb(180, currentTheme.Text), 1.0f);
		cachedTextBrush = gcnew SolidBrush(currentTheme.Text);

		// Setup duration pen with dash pattern
		cachedDurationPen = gcnew Pen(Color::FromArgb(150, currentTheme.Text), 1.0f);
		array<float>^ dashPattern = { 2.0f, 2.0f };
		cachedDurationPen->DashPattern = dashPattern;
	}

	Rectangle Widget_Timeline::SelectionRectangle::get()
	{
		PointerTool^ pointerTool = (PointerTool^)tools[TimelineToolType::Pointer];
		return pointerTool->SelectionRect;
	}

	void Widget_Timeline::OnPaint(PaintEventArgs^ e)
	{
		performanceMetrics->StartFrame();
		
		if (graphicsBuffer != nullptr)
		{
			// Update visibility tracking before drawing
			UpdateVisibilityTracker(graphicsBuffer->Graphics);

			// Clear the background first
			graphicsBuffer->Graphics->Clear(currentTheme.Background);

			// Draw components in correct order
			DrawTrackBackground	(graphicsBuffer->Graphics);	// Draw track backgrounds first
			DrawTimeline		(graphicsBuffer->Graphics);	// Draw timeline (including grid lines)
			DrawTrackContent	(graphicsBuffer->Graphics);	// Draw track content and headers
			ToolPreview			(graphicsBuffer->Graphics);	// Draw tool preview last

			// Render the buffer
			graphicsBuffer->Render(e->Graphics);
		}
		else
		{
			// Direct drawing if buffer isn't ready
			UpdateVisibilityTracker(e->Graphics);

			e->Graphics->Clear(currentTheme.Background);
			DrawTrackBackground(e->Graphics);
			DrawTimeline(e->Graphics);
			DrawTrackContent(e->Graphics);
			ToolPreview(e->Graphics);
		}

		performanceMetrics->EndFrame();
		//LogPerformanceMetrics();
	}

	void Widget_Timeline::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);
		UpdateBuffer();
		UpdateScrollBarRange();
		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::OnMouseDown(MouseEventArgs^ e)
	{
		Track^ resizeTrack;
		if (IsOverTrackDivider(Point(e->X, e->Y), resizeTrack)) {
			BeginTrackResize(resizeTrack, e->Y);
			return;
		}

		// Normal mouse down handling
		this->Focus();
		Control::OnMouseDown(e);
		if (currentTool != nullptr) {
			currentTool->OnMouseDown(e);
		}
	}

	void Widget_Timeline::OnMouseMove(MouseEventArgs^ e)
	{
		Track^ hoverTrack;
		bool isOverDivider = IsOverTrackDivider(Point(e->X, e->Y), hoverTrack);

		if (trackBeingResized != nullptr) {
			// If we're actively resizing, update the track height
			UpdateTrackResize(e->Y);
		}
		else if (isOverDivider) {
			// Just hovering over a divider
			if (resizeHoverTrack != hoverTrack) {
				resizeHoverTrack = hoverTrack;
				this->Cursor = Cursors::SizeNS;
				Invalidate(); // Redraw to show hover state if needed
			}
		}
		else
		{
			// Not over a divider
			if (resizeHoverTrack != nullptr) {
				resizeHoverTrack = nullptr;
				this->Cursor = Cursors::Default;
				Invalidate(); // Redraw to remove hover state if needed
			}

			// Continue with normal mouse move handling
			Control::OnMouseMove(e);
			if (currentTool != nullptr) {
				currentTool->OnMouseMove(e);
			}
		}
	}

	void Widget_Timeline::OnMouseUp(MouseEventArgs^ e)
	{
		Control::OnMouseUp(e);
		
		if (trackBeingResized != nullptr) {
			EndTrackResize();
			return;
		}

		if (currentTool != nullptr) {
			currentTool->OnMouseUp(e);
		}
	}

	void Widget_Timeline::OnMouseWheel(MouseEventArgs^ e)
	{
		if (Control::ModifierKeys == Keys::Control)
		{
			double newZoom;
			if (e->Delta > 0) {
				if (zoomLevel < 1.0) {
					newZoom = zoomLevel * 1.2;
				}
				else {
					newZoom = zoomLevel * 1.05;
				}
			}
			else {
				if (zoomLevel > 1.0) {
					newZoom = zoomLevel / 1.05;
				}
				else {
					newZoom = zoomLevel / 1.2;
				}
			}
			SetZoomLevelAtPoint(newZoom, Point(e->X, e->Y));
		}
		else if (Control::ModifierKeys == Keys::Shift) {
			int scrollUnits = e->Delta > 0 ? -1 : 1;
			int newValue = Math::Min(Math::Max(hScrollBar->Value + scrollUnits,
				hScrollBar->Minimum),
				hScrollBar->Maximum - hScrollBar->LargeChange + 1);
			hScrollBar->Value = newValue;
			OnScroll(hScrollBar, gcnew ScrollEventArgs(ScrollEventType::ThumbPosition, newValue));
		}
		else
		{
			int scrollUnits = e->Delta > 0 ? -vScrollBar->SmallChange : vScrollBar->SmallChange;
			int newValue = Math::Min(Math::Max(
				vScrollBar->Value + scrollUnits,
				vScrollBar->Minimum),
				vScrollBar->Maximum - vScrollBar->LargeChange + 1);
			vScrollBar->Value = newValue;
			OnVerticalScroll(vScrollBar, gcnew ScrollEventArgs(ScrollEventType::ThumbPosition, newValue));
		}
	}

	void Widget_Timeline::OnKeyDown(KeyEventArgs^ e) {
		Control::OnKeyDown(e);
		if (currentTool != nullptr) {
			currentTool->OnKeyDown(e);
		}
	}

	void Widget_Timeline::OnKeyUp(KeyEventArgs^ e) {
		Control::OnKeyUp(e);
		if (currentTool != nullptr) {
			currentTool->OnKeyUp(e);
		}
	}

	void Widget_Timeline::OnHandleCreated(EventArgs^ e) {
		UserControl::OnHandleCreated(e);
		UpdateBuffer();
	}

	void Widget_Timeline::UpdateBuffer()
	{
		// Check if we already have a context, if not create one
		if (bufferContext == nullptr) {
			bufferContext = BufferedGraphicsManager::Current;
		}

		// Only create buffer if we have valid dimensions and visible
		if (Width > 0 && Height > 0 && this->Visible && bufferContext != nullptr) {
			try {
				// Delete old buffer if it exists
				if (graphicsBuffer != nullptr) {
					delete graphicsBuffer;
					graphicsBuffer = nullptr;
				}

				// Create new buffer
				graphicsBuffer = bufferContext->Allocate(this->CreateGraphics(), this->ClientRectangle);
			}
			catch (Exception^ ex) {
				// Handle any potential errors
				Console::WriteLine("Error in UpdateBuffer: " + ex->Message);
			}
		}
	}

	void Widget_Timeline::DrawTimeline(Graphics^ g)
	{
		if (measures->Count == 0) return;

		// Fill the header area with a solid color first
		Rectangle headerRect = Rectangle(0, 0, Width, HEADER_HEIGHT);
		g->FillRectangle(gcnew SolidBrush(currentTheme.HeaderBackground), headerRect);

		// Set up graphics for better quality
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		g->TextRenderingHint = System::Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		// Calculate content area that extends to full height
		int totalHeight = GetTotalTracksHeight();
		Rectangle contentRect(
			TRACK_HEADER_WIDTH,
			HEADER_HEIGHT,
			Width - TRACK_HEADER_WIDTH,
			totalHeight
		);

		// Save original clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Set clip to content area
		g->SetClip(contentRect);

		// Draw grid elements in correct order
		DrawSubdivisionLines(g, contentRect);
		DrawBeatLines(g, contentRect);
		DrawMeasureLines(g, contentRect);

		// Restore original clip for measure numbers
		g->Clip = originalClip;

		// Draw measure numbers in header area
		DrawMeasureNumbers(g, headerRect);
	}

	int Widget_Timeline::TicksToPixels(int ticks) {
		// Handle edge cases
		if (ticks == 0) return 0;

		// Use double for intermediate calculations to maintain precision
		// Split calculation to avoid overflow
		double baseScale = 16.0 / TICKS_PER_QUARTER;
		double scaledTicks = (double)ticks * baseScale;
		double result = scaledTicks * zoomLevel;

		// Protect against overflow
		if (result > Int32::MaxValue) return Int32::MaxValue;
		if (result < Int32::MinValue) return Int32::MinValue;

		//Console::WriteLine("TicksToPixels: {0}, Zoom Level: {1}", (int)Math::Round(result), zoomLevel);

		return (int)Math::Round(result);
	}

	int Widget_Timeline::PixelsToTicks(int pixels) {
		// Handle edge cases
		if (pixels == 0) return 0;

		// Use double for intermediate calculations to maintain precision
		double baseScale = TICKS_PER_QUARTER / 16.0;
		double scaledPixels = (double)pixels * baseScale;
		double result = scaledPixels / zoomLevel;

		// Round to nearest integer
		return (int)Math::Round(result);
	}

	void Widget_Timeline::ZoomIn()
	{
		double newZoom;
		if (zoomLevel < 1.0) {
			newZoom = zoomLevel * 1.2;
		}
		else {
			newZoom = zoomLevel * 1.05;
		}
		SetZoom(newZoom);
	}

	void Widget_Timeline::ZoomOut()
	{
		double newZoom;
		if (zoomLevel > 1.0) {
			newZoom = zoomLevel / 1.05;
		}
		else {
			newZoom = zoomLevel / 1.2;
		}
		SetZoom(newZoom);
	}

	void Widget_Timeline::SetZoom(double newZoom)
	{
		// Clamp zoom level to valid range
		newZoom = Math::Min(Math::Max(newZoom, MIN_ZOOM_LEVEL), MAX_ZOOM_LEVEL);

		// If no change in zoom, exit early
		if (Math::Abs(newZoom - zoomLevel) < 0.0001) return;

		// Calculate the center point of the visible area in ticks
		int visibleWidth = Width - TRACK_HEADER_WIDTH;
		int centerTick = PixelsToTicks(-scrollPosition->X + (visibleWidth / 2));

		// Store old zoom level and apply new zoom
		double oldZoom = zoomLevel;
		zoomLevel = newZoom;

		// Recalculate scroll position to maintain center
		int newCenterPixel = TicksToPixels(centerTick);
		scrollPosition->X = -(newCenterPixel - (visibleWidth / 2));

		// Ensure proper alignment
		UpdateScrollBounds();
		Invalidate();
	}

	void Widget_Timeline::ScrollTo(Point newPosition) {
		scrollPosition = newPosition;
		Invalidate();
	}

	void Widget_Timeline::AddTrack(String^ name, int octave)
	{
		Track^ track = gcnew Track(name, octave);
		tracks->Add(track);
		trackHeights[track] = DEFAULT_TRACK_HEIGHT;

		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::SetTrackHeight(Track^ track, int height)
	{
		if (track == nullptr) return;

		// Calculate minimum height based on whether tablature is shown
		int minHeight = track->ShowTablature ? (int)MIN_TRACK_HEIGHT_WITH_TAB : MINIMUM_TRACK_HEIGHT;

		// Ensure height is within acceptable bounds
		height = Math::Max(minHeight, height);

		// Update height in dictionary if track exists
		if (tracks->Contains(track)) {
			trackHeights[track] = height;

			// Recalculate total height
			int totalHeight = HEADER_HEIGHT;
			for each (Track ^ t in tracks) {
				totalHeight += GetTrackHeight(t);
			}

			// Update scrollbars and bounds
			UpdateVerticalScrollBarRange();
			UpdateScrollBounds();

			// Ensure current scroll position is still valid
			int viewportHeight = Height - HEADER_HEIGHT - hScrollBar->Height;
			if (-scrollPosition->Y + viewportHeight > totalHeight) {
				scrollPosition->Y = Math::Min(0, -(totalHeight - viewportHeight));
				vScrollBar->Value = -scrollPosition->Y;
			}

			Invalidate();
		}
	}

	void Widget_Timeline::AddBarToTrack(Track^ track, int startTick, int length, Color color)
	{
		if (tracks->Contains(track)) {
			track->AddBar(startTick, length, color);
			Invalidate();
		}
	}

	void Widget_Timeline::AddMeasure(int numerator, int denominator) {
		AddMeasure(numerator, denominator, "");
	}

	void Widget_Timeline::AddMeasure(int numerator, int denominator, String^ marker_text)
	{
		int startTick = TotalTicks;
		Measure^ newMeasure = gcnew Measure(startTick, numerator, denominator, marker_text);
		measures->Add(newMeasure);

		for each(Track ^ track in tracks) {
			track->Measures->Add(gcnew TrackMeasure(newMeasure));
		}

		UpdateDrawingForMeasures();
		UpdateScrollBarRange();
		Invalidate();
	}

	Beat^ Widget_Timeline::AddBeat(Track^ track, int measureIndex, int startTick, int durationInTicks)
	{
		if (track == nullptr || measureIndex < 0 || measureIndex >= track->Measures->Count)
			return nullptr;

		Beat^ beat = gcnew Beat();
		beat->StartTick = startTick;
		beat->Duration = durationInTicks;

		TrackMeasure^ measure = track->Measures[measureIndex];
		measure->AddBeat(beat);

		Invalidate();
		return beat;
	}

	void Widget_Timeline::AddNote(Beat^ beat, int stringNumber, int value)
	{
		if (beat == nullptr)
			return;

		Note^ note		= gcnew Note();
		note->String	= stringNumber;
		note->Value		= value;	// Fret/Pitch

		beat->Notes->Add(note);

		Invalidate();
	}

	Measure^ Widget_Timeline::GetMeasureAtTick(int tick) {
		int accumulated = 0;
		for each (Measure ^ measure in measures) {
			if (tick >= accumulated && tick < accumulated + measure->Length) {
				return measure;
			}
			accumulated += measure->Length;
		}
		return nullptr;
	}

	void Widget_Timeline::Clear()
	{
		for each(Track ^ track in tracks) {
			track->Measures->Clear();
		}
		
		// Clear all collections
		tracks->Clear();
		measures->Clear();
		trackHeights->Clear();

		// Reset selection state
		selectedTrack = nullptr;
		hoveredTrack = nullptr;
		selectedBar = nullptr;
		isDraggingTrackDivider = false;
		dragStartY = 0;

		// Reset scroll and zoom
		scrollPosition = gcnew Point(0, 0);
		zoomLevel = 1.0;

		// Reset time signature to default 4/4
		currentTimeSignatureNumerator = 4;
		currentTimeSignatureDenominator = 4;

		// Reset background color
		this->BackColor = currentTheme.Background;

		// Force redraw
		UpdateBuffer();
		Invalidate();
	}

	void Widget_Timeline::StartBarDrag(BarEvent^ bar, Track^ track, Point startPoint)
	{
		if (bar == nullptr || track == nullptr) return;

		isDraggingBar = true;
		draggedBar = bar;
		dragSourceTrack = track;
		dragTargetTrack = track;  // Initially, target is same as source
		dragStartPoint = startPoint;
		currentMousePoint = startPoint;
		dragStartTick = bar->StartTick;

		// Store original position for potential cancel
		bar->OriginalStartTick = bar->StartTick;

		Invalidate();
	}

	void Widget_Timeline::EndBarDrag()
	{
		if (!isDraggingBar || draggedBar == nullptr) return;

		// Final snap to grid
		draggedBar->StartTick = SnapTickToGrid(draggedBar->StartTick);

		// If we're dropping onto a different track
		if (dragTargetTrack != dragSourceTrack) {
			// Remove from source track
			dragSourceTrack->Events->Remove(draggedBar);

			// Add to target track
			dragTargetTrack->Events->Add(draggedBar);

			// Resort the events in the target track
			dragTargetTrack->Events->Sort(gcnew Comparison<BarEvent^>(&Track::CompareBarEvents));
		}

		// Reset drag state
		isDraggingBar = false;
		draggedBar = nullptr;
		dragSourceTrack = nullptr;
		dragTargetTrack = nullptr;
		dragStartPoint = nullptr;
		currentMousePoint = nullptr;

		Invalidate();
	}

	int Widget_Timeline::SnapTickToGrid(int tick)
	{
		// Get current subdivision level based on zoom
		float subdivLevel = GetSubdivisionLevel();

		// Calculate snap resolution based on subdivision level
		int snapResolution = TICKS_PER_QUARTER / (int)subdivLevel;

		// Round to nearest snap point
		return ((tick + (snapResolution / 2)) / snapResolution) * snapResolution;
	}

	void Widget_Timeline::ScrollToMeasure(int measureNumber) {
		// Validate measure number
		if (measureNumber < 1 || measureNumber > measures->Count)
		{
			throw gcnew ArgumentOutOfRangeException("measureNumber",
				"Measure number must be between 1 and " + measures->Count);
		}

		// Get the start tick of the requested measure
		int targetTick = GetMeasureStartTick(measureNumber);

		// Convert to pixels
		int targetPixel = TicksToPixels(targetTick);

		// Calculate scroll position to center the measure
		int viewportWidth = Width - TRACK_HEADER_WIDTH;
		int newScrollX = -targetPixel + (viewportWidth / 2);

		// Clamp scroll position to valid range
		double virtualWidth = GetVirtualWidth();
		double maxScroll = -(virtualWidth - viewportWidth);
		newScrollX = (int)Math::Round(Math::Max(maxScroll, Math::Min(0.0, (double)newScrollX)));

		// Update scroll position
		scrollPosition->X = newScrollX;

		// Update scrollbar
		UpdateScrollBarRange();

		// Request redraw
		Invalidate();
	}

	void Widget_Timeline::GetVisibleMeasureRange(int% firstMeasure, int% lastMeasure)
	{
		// Calculate visible tick range
		int startTick = PixelsToTicks(-scrollPosition->X);
		int endTick = PixelsToTicks(-scrollPosition->X + Width - TRACK_HEADER_WIDTH);

		// Find first visible measure
		int accumulatedTicks = 0;
		firstMeasure = 1;
		for (int i = 0; i < measures->Count; i++) {
			if (accumulatedTicks + measures[i]->Length > startTick) {
				firstMeasure = i + 1;
				break;
			}
			accumulatedTicks += measures[i]->Length;
		}

		// Find last visible measure
		lastMeasure = firstMeasure;
		for (int i = firstMeasure - 1; i < measures->Count; i++) {
			if (accumulatedTicks > endTick) {
				break;
			}
			lastMeasure = i + 1;
			accumulatedTicks += measures[i]->Length;
		}
	}

	int Widget_Timeline::GetMeasureStartTick(int measureNumber)
	{
		if (measureNumber < 1 || measureNumber > measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber",
				"Measure number must be between 1 and " + measures->Count);
		}

		int startTick = 0;
		// Sum up lengths of previous measures
		for (int i = 0; i < measureNumber - 1; i++) {
			startTick += measures[i]->Length;
		}
		return startTick;
	}

	int Widget_Timeline::GetMeasureLength(int measureNumber) {
		if (measureNumber < 1 || measureNumber > measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber",
				"Measure number must be between 1 and " + measures->Count);
		}

		return measures[measureNumber - 1]->Length;
	}

	bool Widget_Timeline::IsMeasureVisible(int measureNumber) {
		if (measureNumber < 1 || measureNumber > measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber",
				"Measure number must be between 1 and " + measures->Count);
		}

		int startTick = GetMeasureStartTick(measureNumber);
		int endTick = startTick + GetMeasureLength(measureNumber);

		// Calculate visible range in ticks
		int visibleStartTick = PixelsToTicks(-scrollPosition->X);
		int visibleEndTick = PixelsToTicks(-scrollPosition->X + Width - TRACK_HEADER_WIDTH);

		// Check if any part of the measure is visible
		return (startTick <= visibleEndTick && endTick >= visibleStartTick);
	}

	void Widget_Timeline::SetCurrentTool(TimelineToolType tool) {
		if (currentToolType != tool)
		{
			// Deactivate current tool if one exists
			if (currentTool != nullptr) {
				currentTool->Deactivate();
			}
			
			// Set new tool
			currentToolType = tool;
			currentTool = tools[tool];

			// Activate new tool
			currentTool->Activate();
			this->Cursor = currentTool->Cursor;
			Invalidate();
		}
	}

	void Widget_Timeline::UpdateCursor(System::Windows::Forms::Cursor^ cursor) {
		if (this->Cursor != cursor) {
			this->Cursor = cursor;
		}
	}

	void Widget_Timeline::UpdateBarDrag(Point newPoint)
	{
		if (!isDraggingBar || draggedBar == nullptr) return;

		currentMousePoint = newPoint;

		// Update target track based on current mouse position
		Track^ trackUnderMouse = GetTrackAtPoint(newPoint);
		if (trackUnderMouse != nullptr) {
			dragTargetTrack = trackUnderMouse;
		}

		// Calculate the delta in ticks
		int pixelDelta = newPoint.X - dragStartPoint->X;
		int tickDelta = PixelsToTicks(pixelDelta);

		// Calculate new position
		int newStartTick = dragStartTick + tickDelta;

		// Snap to grid
		int snappedTick = SnapTickToGrid(newStartTick);

		// Update bar position
		draggedBar->StartTick = snappedTick;

		Invalidate();
	}

	void Widget_Timeline::UpdateScrollPosition(Point mousePosition)
	{
		// TODO: Implement scroll position update logic
	}

	void Widget_Timeline::UpdateScrollBounds()
	{
		// Calculate total width in pixels
		double virtualWidth = GetVirtualWidth();
		int viewportWidth = Width - TRACK_HEADER_WIDTH;

		// Calculate maximum scroll position
		double maxScroll;
		if (virtualWidth > viewportWidth) {
			maxScroll = -(virtualWidth - viewportWidth);
		}
		else {
			maxScroll = 0;
		}

		// Calculate grid alignment based on zoom level
		int gridPixels;
		if (zoomLevel > 50.0) {
			gridPixels = TicksToPixels(TICKS_PER_QUARTER / 64); // Even finer grid at very high zoom
		}
		else if (zoomLevel > 20.0) {
			gridPixels = TicksToPixels(TICKS_PER_QUARTER / 32);
		}
		else if (zoomLevel > 10.0) {
			gridPixels = TicksToPixels(TICKS_PER_QUARTER / 16);
		}
		else if (zoomLevel > 5.0) {
			gridPixels = TicksToPixels(TICKS_PER_QUARTER / 8);
		}
		else {
			gridPixels = TicksToPixels(TICKS_PER_QUARTER / 4);
		}

		if (gridPixels > 0) {
			// Snap scroll position to grid
			scrollPosition->X = (int)Math::Round((double)scrollPosition->X / gridPixels) * gridPixels;
		}

		// Clamp scroll position
		scrollPosition->X = (int)Math::Round(Math::Max(maxScroll, Math::Min(0.0, (double)scrollPosition->X)));

		// Update scrollbar to reflect new position/range
		UpdateScrollBarRange();
	}

	double Widget_Timeline::GetRelativePositionInMeasure(int tick)
	{
		Measure^ measure = GetMeasureAtTick(tick);
		if (measure == nullptr) return 0.0;

		int measureStartTick = 0;
		for each (Measure ^ m in measures) {
			if (m == measure) break;
			measureStartTick += m->Length;
		}

		return (double)(tick - measureStartTick) / measure->Length;
	}

	void Widget_Timeline::SetZoomLevelAtPoint(double newZoom, Point referencePoint)
	{
		// Clamp zoom level to valid range
		newZoom = Math::Min(Math::Max(newZoom, MIN_ZOOM_LEVEL), MAX_ZOOM_LEVEL);

		// If no change in zoom, exit early
		if (Math::Abs(newZoom - zoomLevel) < 0.0001) return;

		// Get position relative to content area
		Point contentPos = Point(referencePoint.X - TRACK_HEADER_WIDTH, referencePoint.Y);

		// Find tick at reference point
		int tickAtPosition = PixelsToTicks(contentPos.X - scrollPosition->X);

		// Store old zoom level and apply new zoom
		zoomLevel = newZoom;

		// Maintain position of reference point
		int newPixelPosition = TicksToPixels(tickAtPosition);
		int positionOffset = referencePoint.X - TRACK_HEADER_WIDTH;
		scrollPosition->X = -(newPixelPosition - positionOffset);

		// Ensure proper alignment
		UpdateScrollBounds();
		Invalidate();
	}

	void Widget_Timeline::UpdateVisibilityTracker(Graphics^ g)
	{
		Rectangle viewport(0, 0, Width, Height);
		visibilityTracker->Update(viewport, zoomLevel, scrollPosition, tracks, measures, HEADER_HEIGHT, TRACK_HEADER_WIDTH, performanceMetrics);
	}

	void Widget_Timeline::DrawMeasureLines(Graphics^ g, Rectangle contentRect)
	{
		// Use alpha for measure lines to ensure visibility
		Color measureColor = Color::FromArgb(255, currentTheme.MeasureLine);
		Pen^ measurePen = gcnew Pen(measureColor, 1.0f);

		try {
			int startTick = PixelsToTicks(-scrollPosition->X);
			int endTick = PixelsToTicks(-scrollPosition->X + Width);

			int accumulated = 0;
			for each (Measure ^ measure in measures) {
				if (accumulated > endTick) break;
				if (accumulated + measure->Length < startTick) {
					accumulated += measure->Length;
					continue;
				}

				int x = TicksToPixels(accumulated) + scrollPosition->X + TRACK_HEADER_WIDTH;

				if (x >= TRACK_HEADER_WIDTH && x <= Width) {
					g->DrawLine(measurePen,
						x, contentRect.Y,
						x, contentRect.Y + contentRect.Height);
				}

				accumulated += measure->Length;
			}
		}
		finally {
			delete measurePen;
		}
	}

	void Widget_Timeline::DrawBeatLines(Graphics^ g, Rectangle^ clipRect)
	{
		if (measures->Count == 0) return;

		// Calculate total height including content area
		int totalHeight = GetTotalTracksHeight();

		// Create content area clipping region
		Rectangle contentRect(
			TRACK_HEADER_WIDTH,  // Start after header
			HEADER_HEIGHT,       // Start below header
			Width - TRACK_HEADER_WIDTH,
			totalHeight
		);

		// Save original clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create new clip region
		g->SetClip(contentRect);

		Pen^ beatPen = gcnew Pen(currentTheme.BeatLine);

		// Calculate visible range
		int startTick = PixelsToTicks(-scrollPosition->X);
		int endTick = PixelsToTicks(-scrollPosition->X + Width - TRACK_HEADER_WIDTH);

		// Draw beat lines within each measure
		int accumulated = 0;
		for each (Measure ^ measure in measures) {
			int measureStart = accumulated;
			int ticksPerBeat = TICKS_PER_QUARTER * 4 / measure->Denominator;

			// Draw lines for each beat except measure start
			for (int beat = 1; beat < measure->Numerator; beat++) {
				int beatTick = measureStart + beat * ticksPerBeat;

				if (beatTick >= startTick && beatTick <= endTick) {
					int x = TicksToPixels(beatTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
					g->DrawLine(beatPen, x, HEADER_HEIGHT, x, HEADER_HEIGHT + totalHeight);
				}
			}

			accumulated += measure->Length;
		}

		delete beatPen;
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawSubdivisionLines(Graphics^ g, Rectangle^ clipRect)
	{
		if (measures->Count == 0) return;

		// Calculate subdivision level based on zoom
		float subdivLevel = GetSubdivisionLevel();
		if (subdivLevel <= 1) return;

		// Calculate total height including content area
		int totalHeight = GetTotalTracksHeight();

		// Create content area clipping region
		Rectangle contentRect(
			TRACK_HEADER_WIDTH,  // Start after header
			HEADER_HEIGHT,       // Start below header
			Width - TRACK_HEADER_WIDTH,
			totalHeight
		);

		// Save original clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create new clip region
		g->SetClip(contentRect);

		Pen^ subdivPen = gcnew Pen(currentTheme.SubdivisionLine);

		// Calculate visible range
		int startTick = PixelsToTicks(-scrollPosition->X);
		int endTick = PixelsToTicks(-scrollPosition->X + Width - TRACK_HEADER_WIDTH);

		// Draw subdivision lines
		int accumulated = 0;
		for each (Measure ^ measure in measures) {
			int measureStart = accumulated;
			int ticksPerBeat = TICKS_PER_QUARTER * 4 / measure->Denominator;
			int ticksPerSubdiv = ticksPerBeat / (int)subdivLevel;

			// Calculate subdivisions for this measure
			int subdivisions = (measure->Length / ticksPerSubdiv);

			for (int subdiv = 1; subdiv < subdivisions; subdiv++) {
				int subdivTick = measureStart + subdiv * ticksPerSubdiv;

				// Skip if this is already a beat or measure line
				if (subdivTick % ticksPerBeat == 0) continue;

				if (subdivTick >= startTick && subdivTick <= endTick) {
					int x = TicksToPixels(subdivTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
					g->DrawLine(subdivPen, x, HEADER_HEIGHT, x, HEADER_HEIGHT + totalHeight);
				}
			}

			accumulated += measure->Length;
		}

		delete subdivPen;
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawMeasureNumbers(Graphics^ g, Rectangle^ clipRect)
	{
		if (!measureFont || measures->Count == 0) return;

		SolidBrush^ textBrush = gcnew SolidBrush(currentTheme.Text);

		// Calculate visible range in ticks
		int startTick = PixelsToTicks(-scrollPosition->X);
		int endTick = PixelsToTicks(-scrollPosition->X + clipRect->Width - TRACK_HEADER_WIDTH);

		// Draw measure numbers and time signatures
		int accumulated = 0;
		int measureNumber = 1;

		// Constants for vertical positioning - updated for marker text
		const int markerTextY = 2;  // Marker text at the top
		const int measureNumberY = markerTextY + markerFont->Height + 2;  // Measure number below marker
		const int timeSignatureY = measureNumberY + measureFont->Height + 2;  // Time signature at the bottom

		for each (Measure ^ measure in measures) {
			// Convert tick position to pixels
			int x = TicksToPixels(accumulated) + scrollPosition->X + TRACK_HEADER_WIDTH;

			if (x >= TRACK_HEADER_WIDTH && x <= Width) {
				// Draw marker text if present
				if (!String::IsNullOrEmpty(measure->Marker_Text)) {
					String^ markerText = measure->Marker_Text;
					SizeF markerSize = g->MeasureString(markerText, markerFont);
					float markerX = x - (markerSize.Width / 2);

					// Ensure text doesn't get cut off at the left edge
					markerX = Math::Max(markerX, (float)TRACK_HEADER_WIDTH + 2);

					// Draw with a slightly transparent background for better readability
					RectangleF markerBg = RectangleF(markerX - 2, markerTextY,
						markerSize.Width + 4, markerSize.Height);
					g->FillRectangle(gcnew SolidBrush(Color::FromArgb(180, currentTheme.HeaderBackground)),
						markerBg);
					g->DrawString(markerText, markerFont, textBrush, markerX, markerTextY);
				}

				// Draw measure number
				String^ numText = measureNumber.ToString();
				SizeF numSize = g->MeasureString(numText, measureFont);
				float textX = x - (numSize.Width / 2);

				// Ensure text doesn't get cut off at the left edge
				textX = Math::Max(textX, (float)TRACK_HEADER_WIDTH + 2);

				g->DrawString(numText, measureFont, textBrush, textX, (float)measureNumberY);

				// Draw time signature if different from previous measure
				if (measureNumber == 1 ||
					measures[measureNumber - 2]->Numerator != measure->Numerator ||
					measures[measureNumber - 2]->Denominator != measure->Denominator) {

					String^ timeSignature = measure->ToString();
					SizeF sigSize = g->MeasureString(timeSignature, markerFont);
					float sigX = x - (sigSize.Width / 2);

					// Ensure time signature doesn't get cut off
					sigX = Math::Max(sigX, (float)TRACK_HEADER_WIDTH + 2);

					g->DrawString(timeSignature, markerFont, textBrush, sigX, (float)timeSignatureY);
				}
			}

			accumulated += measure->Length;
			measureNumber++;
		}

		delete textBrush;
	}

	void Widget_Timeline::ToolPreview(Graphics^ g)
	{
		// Save the original clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create a clipping region for the content area
		Rectangle contentArea(0, HEADER_HEIGHT, Width, Height - HEADER_HEIGHT);
		g->SetClip(contentArea);
		
		// Get current tool
		if (currentToolType == TimelineToolType::Draw)
		{
			DrawTool^ drawTool = (DrawTool^)currentTool;

			// Handle different draw tool modes
			switch (drawTool->CurrentMode)
			{
			case DrawTool::DrawMode::Draw:
			{
				if (drawTool->PreviewBar != nullptr)
				{
					// Get the track under the current mouse position
					Point mousePos = drawTool->CurrentMousePosition;
					Track^ previewTrack = this->GetTrackAtPoint(mousePos);

					if (previewTrack != nullptr)
					{
						// Get the track bounds for drawing
						Rectangle bounds = GetTrackContentBounds(previewTrack);
						bounds.Y += scrollPosition->Y;

						// Calculate bar position and width
						int x = TicksToPixels(drawTool->PreviewBar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
						int width = TicksToPixels(drawTool->PreviewBar->Length);

						// Create rectangle for preview bar
						Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

						// Draw semi-transparent preview
						Color previewColor = drawTool->PreviewBar->Color;
						g->FillRectangle(gcnew SolidBrush(previewColor), barBounds);
						g->DrawRectangle(gcnew Pen(Color::FromArgb(180, previewColor)), barBounds);
					}
				}
				break;
			}

			case DrawTool::DrawMode::Erase:
			{
				BarEvent^ hoverBar = drawTool->HoverBar;
				if (hoverBar != nullptr)
				{
					// Get track containing hover bar
					Track^ track = nullptr;
					for each(Track ^ t in tracks) {
						if (t->Events->Contains(hoverBar)) {
							track = t;
							break;
						}
					}

					if (track != nullptr)
					{
						Rectangle bounds = GetTrackContentBounds(track);
						int x = TicksToPixels(hoverBar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
						int width = TicksToPixels(hoverBar->Length);

						Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

						// Draw delete preview
						Color deleteColor = Color::FromArgb(100, 255, 0, 0); // Light red
						g->FillRectangle(gcnew SolidBrush(deleteColor), barBounds);

						// Draw crossed-out effect
						Pen^ deletePen = gcnew Pen(Color::FromArgb(180, 255, 0, 0), 2);
						g->DrawRectangle(deletePen, barBounds);
						g->DrawLine(deletePen,
							barBounds.Left, barBounds.Top,
							barBounds.Right, barBounds.Bottom);
						g->DrawLine(deletePen,
							barBounds.Left, barBounds.Bottom,
							barBounds.Right, barBounds.Top);
						delete deletePen;
					}
				}
				break;
			}

			case DrawTool::DrawMode::Move:
			{
				BarEvent^ hoverBar = drawTool->HoverBar;
				if (hoverBar != nullptr && !drawTool->IsMoving)
				{
					// Get track containing hover bar
					Track^ track = nullptr;
					for each(Track ^ t in tracks)
					{
						if (t->Events->Contains(hoverBar)) {
							track = t;
							break;
						}
					}

					if (track != nullptr)
					{
						Rectangle bounds = GetTrackContentBounds(track);
						int x = TicksToPixels(hoverBar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
						int width = TicksToPixels(hoverBar->Length);

						Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

						// Draw move preview (highlight)
						Color moveColor = Color::FromArgb(100, currentTheme.SelectionHighlight);
						g->FillRectangle(gcnew SolidBrush(moveColor), barBounds);
						Pen^ movePen = gcnew Pen(currentTheme.SelectionHighlight, 2);
						g->DrawRectangle(movePen, barBounds);
						delete movePen;

						// Draw move arrows or handles
						DrawMoveHandles(g, barBounds);
					}
				}
				break;
			}

			case DrawTool::DrawMode::Resize:
			{
				BarEvent^ hoverBar = drawTool->HoverBar;
				if (hoverBar != nullptr && !drawTool->IsResizing)
				{
					// Get track containing hover bar
					Track^ track = nullptr;
					for each(Track ^ t in tracks) {
						if (t->Events->Contains(hoverBar)) {
							track = t;
							break;
						}
					}

					if (track != nullptr)
					{
						Rectangle bounds = GetTrackContentBounds(track);
						int x = TicksToPixels(hoverBar->StartTick) +
							scrollPosition->X + TRACK_HEADER_WIDTH;
						int width = TicksToPixels(hoverBar->Length);

						Rectangle barBounds(x, bounds.Y + TRACK_PADDING,
							width, bounds.Height - TRACK_PADDING * 2);

						// Draw resize preview
						Color resizeColor = Color::FromArgb(100, currentTheme.SelectionHighlight);
						g->FillRectangle(gcnew SolidBrush(resizeColor), barBounds);

						// Draw resize handle with enhanced visibility
						Rectangle handleBounds(
							barBounds.Right - 5,
							barBounds.Y,
							5,
							barBounds.Height
						);

						g->FillRectangle(gcnew SolidBrush(currentTheme.SelectionHighlight),
							handleBounds);

						// Add grip lines
						Pen^ gripPen = gcnew Pen(Color::White, 1);
						for (int i = 0; i < 3; i++) {
							g->DrawLine(gripPen,
								handleBounds.X + i + 1,
								handleBounds.Y + 2,
								handleBounds.X + i + 1,
								handleBounds.Bottom - 2);
						}
						delete gripPen;
					}
				}
				break;
			}
			}
		}
		else if (currentToolType == TimelineToolType::Erase)
		{
			EraseTool^ eraseTool = (EraseTool^)currentTool;

			// Draw selection rectangle if present
			Rectangle selRect = eraseTool->SelectionRect;
			if (selRect.Width > 0 && selRect.Height > 0) {
				Color selectionColor = Color::FromArgb(50, currentTheme.SelectionHighlight);
				g->FillRectangle(gcnew SolidBrush(selectionColor), selRect);
				g->DrawRectangle(gcnew Pen(currentTheme.SelectionHighlight), selRect);
			}

			// Check if we're hovering over a selected bar
			bool isHoveringSelected = eraseTool->HoverBar != nullptr &&
				eraseTool->SelectedBars->Contains(eraseTool->HoverBar);

			// Draw all selected bars
			for each(BarEvent ^ bar in eraseTool->SelectedBars) {
				Track^ track = nullptr;
				// Find the track containing this bar
				for each(Track ^ t in Tracks) {
					if (t->Events->Contains(bar)) {
						track = t;
						break;
					}
				}

				if (track != nullptr) {
					Rectangle bounds = GetTrackContentBounds(track);
					int x = TicksToPixels(bar->StartTick) +
						ScrollPosition->X +
						TRACK_HEADER_WIDTH;

					Rectangle barRect(
						x,
						bounds.Y + TRACK_PADDING,
						TicksToPixels(bar->Length),
						bounds.Height - (TRACK_PADDING * 2)
					);

					// Draw selection highlight
					Color highlightColor = Color::FromArgb(100, 255, 0, 0); // Light red
					g->FillRectangle(gcnew SolidBrush(highlightColor), barRect);

					// If hovering over any selected bar, show delete preview for all selected bars
					if (isHoveringSelected) {
						Color previewColor = Color::FromArgb(180, 255, 0, 0); // Semi-transparent red
						Pen^ previewPen = gcnew Pen(previewColor, 2);

						// Draw crossed-out effect
						g->DrawRectangle(previewPen, barRect);
						g->DrawLine(previewPen,
							barRect.Left, barRect.Top,
							barRect.Right, barRect.Bottom);
						g->DrawLine(previewPen,
							barRect.Left, barRect.Bottom,
							barRect.Right, barRect.Top);

						delete previewPen;
					}
				}
			}
		}
		else if (currentToolType == TimelineToolType::Duration)
		{
			DurationTool^ durationTool = GetDurationTool();

			// Draw selection rectangle if active
			Rectangle selRect = durationTool->SelectionRect;
			if (selRect.Width > 0 && selRect.Height > 0) {
				Color selectionColor = Color::FromArgb(50, currentTheme.SelectionHighlight);
				g->FillRectangle(gcnew SolidBrush(selectionColor), selRect);
				g->DrawRectangle(gcnew Pen(currentTheme.SelectionHighlight), selRect);
			}
		}
		else if (currentToolType == TimelineToolType::Color)
		{
			ColorTool^ colorTool = GetColorTool();

			// Draw selection rectangle if present
			Rectangle selRect = colorTool->SelectionRect;
			if (selRect.Width > 0 && selRect.Height > 0) {
				Color selectionColor = Color::FromArgb(50, currentTheme.SelectionHighlight);
				g->FillRectangle(gcnew SolidBrush(selectionColor), selRect);
				g->DrawRectangle(gcnew Pen(currentTheme.SelectionHighlight), selRect);
			}

			// Draw all selected bars with a highlight
			for each(BarEvent ^ bar in colorTool->SelectedBars) {
				Track^ track = nullptr;
				// Find the track containing this bar
				for each(Track ^ t in Tracks) {
					if (t->Events->Contains(bar)) {
						track = t;
						break;
					}
				}

				if (track != nullptr) {
					Rectangle bounds = GetTrackContentBounds(track);
					int x = TicksToPixels(bar->StartTick) +
						ScrollPosition->X +
						TRACK_HEADER_WIDTH;

					Rectangle barRect(
						x,
						bounds.Y + TRACK_PADDING,
						TicksToPixels(bar->Length),
						bounds.Height - (TRACK_PADDING * 2)
					);

					// Draw selection highlight
					Color highlightColor = Color::FromArgb(100, colorTool->CurrentColor);
					g->FillRectangle(gcnew SolidBrush(highlightColor), barRect);

					// Draw preview outline
					Pen^ previewPen = gcnew Pen(colorTool->CurrentColor, 2);
					g->DrawRectangle(previewPen, barRect);
					delete previewPen;

					// If this bar is being hovered over, show stronger highlight
					if (bar == colorTool->HoverBar) {
						for (int i = 2; i >= 0; i--) {
							Rectangle glowBounds = barRect;
							glowBounds.Inflate(i, i);
							Pen^ glowPen = gcnew Pen(Color::FromArgb(60 - i * 15, colorTool->CurrentColor), 1);
							g->DrawRectangle(glowPen, glowBounds);
							delete glowPen;
						}
					}
				}
			}

			// Draw preview rectangle for hovered bar
			Rectangle previewRect = colorTool->PreviewRect;
			if (!previewRect.IsEmpty && !colorTool->SelectedBars->Contains(colorTool->HoverBar)) {
				Color previewColor = Color::FromArgb(80, colorTool->CurrentColor);
				g->FillRectangle(gcnew SolidBrush(previewColor), previewRect);
				g->DrawRectangle(gcnew Pen(colorTool->CurrentColor), previewRect);
			}
		}

		// Restore original clip region
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawNormalBar(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		int x = TicksToPixels(bar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
		int width = TicksToPixels(bar->Length);

		// Check for duration preview
		bool isPreview = false;
		bool isDurationTarget = false;
		if (currentToolType == TimelineToolType::Duration) {
			DurationTool^ durationTool = GetDurationTool();
			isDurationTarget = (bar == durationTool->PreviewBar);
			if (isDurationTarget && durationTool->IsPreviewVisible) {
				width = TicksToPixels(durationTool->PreviewLength);
				isPreview = true;
			}
		}

		Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

		// Draw the bar with appropriate transparency
		if (isPreview) {
			// Draw original bar with higher transparency
			g->FillRectangle(gcnew SolidBrush(Color::FromArgb(80, bar->Color)),
				Rectangle(x, bounds.Y + TRACK_PADDING, TicksToPixels(bar->Length), bounds.Height - TRACK_PADDING * 2));

			// Draw preview length with normal transparency and highlight
			Color highlightColor = Color::FromArgb(
				Math::Min(255, bar->Color.R + 40),
				Math::Min(255, bar->Color.G + 40),
				Math::Min(255, bar->Color.B + 40)
			);
			g->FillRectangle(gcnew SolidBrush(highlightColor), barBounds);

			// Draw preview outline with glow effect
			Color glowColor = currentTheme.SelectionHighlight;
			for (int i = 3; i >= 0; i--) {
				Rectangle glowBounds = barBounds;
				glowBounds.Inflate(i, i);
				Pen^ glowPen = gcnew Pen(Color::FromArgb(60 - i * 15, glowColor), 1);
				g->DrawRectangle(glowPen, glowBounds);
				delete glowPen;
			}
		}
		else {
			// Normal drawing
			if (isDurationTarget) {
				// Highlight the bar being targeted for duration change
				Color highlightColor = Color::FromArgb(
					Math::Min(255, bar->Color.R + 30),
					Math::Min(255, bar->Color.G + 30),
					Math::Min(255, bar->Color.B + 30)
				);
				g->FillRectangle(gcnew SolidBrush(highlightColor), barBounds);
				Pen^ highlightPen = gcnew Pen(currentTheme.SelectionHighlight, 2);
				g->DrawRectangle(highlightPen, barBounds);
				delete highlightPen;
			}
			else {
				g->FillRectangle(gcnew SolidBrush(Color::FromArgb(200, bar->Color)), barBounds);
				g->DrawRectangle(gcnew Pen(Color::FromArgb(100, 0, 0, 0)), barBounds);
			}
		}

		// Add DrawTool-specific resize handle visualization
		if (currentToolType == TimelineToolType::Draw) {
			DrawTool^ drawTool = GetDrawTool();
			if (drawTool->CurrentMode == DrawTool::DrawMode::Resize &&
				bar == drawTool->HoverBar) {
				// Draw enhanced resize handle
				Rectangle handleBounds(
					barBounds.Right - 4,
					barBounds.Y,
					4,
					barBounds.Height
				);

				// Draw handle with highlight color
				Color handleColor = currentTheme.SelectionHighlight;
				g->FillRectangle(gcnew SolidBrush(handleColor), handleBounds);

				// Add grip lines
				Pen^ gripPen = gcnew Pen(Color::White, 1);
				for (int i = 0; i < 3; i++) {
					g->DrawLine(gripPen,
						handleBounds.X + i,
						handleBounds.Y + 2,
						handleBounds.X + i,
						handleBounds.Bottom - 2);
				}
				delete gripPen;
			}
		}

		// Draw resize handle if Duration tool is active
		if (currentToolType == TimelineToolType::Duration) {
			// Create handle color (brighter if this is the target bar)
			Color handleColor;
			if (isDurationTarget) {
				handleColor = currentTheme.SelectionHighlight;
			}
			else {
				handleColor = Color::FromArgb(
					bar->Color.R * 8 / 10,
					bar->Color.G * 8 / 10,
					bar->Color.B * 8 / 10
				);
			}

			// Draw handle at right edge with increased visibility
			Rectangle handleBounds(
				barBounds.Right - 4,
				barBounds.Y,
				4,
				barBounds.Height
			);

			g->FillRectangle(gcnew SolidBrush(handleColor), handleBounds);

			// Add more visible grip lines
			Color gripColor = isDurationTarget ? Color::White : Color::FromArgb(180, 255, 255, 255);
			Pen^ gripPen = gcnew Pen(gripColor, 1);
			for (int i = 0; i < 3; i++) {
				g->DrawLine(gripPen,
					handleBounds.X + i,
					handleBounds.Y + 2,
					handleBounds.X + i,
					handleBounds.Bottom - 2);
			}
			delete gripPen;

			// Add subtle glow to handle if this is the target
			if (isDurationTarget) {
				for (int i = 2; i >= 0; i--) {
					Rectangle glowBounds = handleBounds;
					glowBounds.Inflate(i, i);
					Pen^ glowPen = gcnew Pen(Color::FromArgb(40 - i * 10, currentTheme.SelectionHighlight), 1);
					g->DrawRectangle(glowPen, glowBounds);
					delete glowPen;
				}
			}
		}
	}

	void Widget_Timeline::DrawGhostBar(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		int x = TicksToPixels(bar->OriginalStartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
		int width = TicksToPixels(bar->Length);
		Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

		// Draw with high transparency
		g->FillRectangle(gcnew SolidBrush(Color::FromArgb(80, bar->Color)), barBounds);
	}

	void Widget_Timeline::DrawSelectedBar(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		int x = TicksToPixels(bar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
		int width = TicksToPixels(bar->Length);

		// Check for duration preview
		bool isPreview = false;
		bool isDurationTarget = false;
		if (currentToolType == TimelineToolType::Duration) {
			DurationTool^ durationTool = GetDurationTool();
			isDurationTarget = (bar == durationTool->PreviewBar);
			if (isDurationTarget && durationTool->IsPreviewVisible) {
				width = TicksToPixels(durationTool->PreviewLength);
				isPreview = true;
			}
		}

		Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

		if (isPreview) {
			// Draw original bar with reduced opacity
			Rectangle originalBounds(x, bounds.Y + TRACK_PADDING,
				TicksToPixels(bar->Length), bounds.Height - TRACK_PADDING * 2);
			g->FillRectangle(gcnew SolidBrush(Color::FromArgb(120, bar->Color)), originalBounds);

			// Draw preview with enhanced brightness and opacity
			Color previewColor = Color::FromArgb(
				Math::Min(255, bar->Color.R + 50),
				Math::Min(255, bar->Color.G + 50),
				Math::Min(255, bar->Color.B + 50)
			);
			g->FillRectangle(gcnew SolidBrush(Color::FromArgb(255, previewColor)), barBounds);

			// Draw animated glow effect
			for (int i = 4; i >= 0; i--) {
				Rectangle glowBounds = barBounds;
				glowBounds.Inflate(i, i);
				Pen^ glowPen = gcnew Pen(Color::FromArgb(80 - i * 15, currentTheme.SelectionHighlight), 1);
				g->DrawRectangle(glowPen, glowBounds);
				delete glowPen;
			}
		}
		else {
			// Normal selected bar drawing with enhanced highlight if being targeted
			if (isDurationTarget) {
				Color enhancedColor = Color::FromArgb(
					Math::Min(255, bar->Color.R + 40),
					Math::Min(255, bar->Color.G + 40),
					Math::Min(255, bar->Color.B + 40)
				);
				g->FillRectangle(gcnew SolidBrush(enhancedColor), barBounds);
			}
			else {
				g->FillRectangle(gcnew SolidBrush(Color::FromArgb(255, bar->Color)), barBounds);
			}

			g->DrawRectangle(gcnew Pen(Color::FromArgb(100, 0, 0, 0)), barBounds);

			// Draw selection border with enhanced glow if being targeted
			Pen^ highlightPen = gcnew Pen(currentTheme.SelectionHighlight, isDurationTarget ? 3.0f : 2.0f);
			Rectangle highlightBounds = barBounds;
			highlightBounds.Inflate(-1, -1);
			g->DrawRectangle(highlightPen, highlightBounds);
			delete highlightPen;
		}

		// Draw resize handle with enhanced visibility for duration tool
		if (currentToolType == TimelineToolType::Duration) {
			Color handleColor = isDurationTarget ?
				currentTheme.SelectionHighlight :
				Color::FromArgb(
					Math::Min(255, bar->Color.R * 13 / 10),
					Math::Min(255, bar->Color.G * 13 / 10),
					Math::Min(255, bar->Color.B * 13 / 10)
				);

			// Draw larger handle for selected bars
			Rectangle handleBounds(
				barBounds.Right - 5,
				barBounds.Y,
				5,
				barBounds.Height
			);

			g->FillRectangle(gcnew SolidBrush(handleColor), handleBounds);

			// Add bright grip lines
			Color gripColor = isDurationTarget ? Color::White : Color::FromArgb(200, 255, 255, 255);
			Pen^ gripPen = gcnew Pen(gripColor, isDurationTarget ? 2.0f : 1.0f);
			for (int i = 0; i < 3; i++) {
				g->DrawLine(gripPen,
					handleBounds.X + i + 1,
					handleBounds.Y + 2,
					handleBounds.X + i + 1,
					handleBounds.Bottom - 2);
			}
			delete gripPen;

			// Add glow effect to handle if targeted
			if (isDurationTarget) {
				for (int i = 3; i >= 0; i--) {
					Rectangle glowBounds = handleBounds;
					glowBounds.Inflate(i, i);
					Pen^ glowPen = gcnew Pen(Color::FromArgb(50 - i * 10, currentTheme.SelectionHighlight), 1);
					g->DrawRectangle(glowPen, glowBounds);
					delete glowPen;
				}
			}
		}
	}

	void Widget_Timeline::DrawMoveHandles(Graphics^ g, Rectangle barBounds)
	{
		// Draw move arrows or handle indicators at the edges
		int arrowSize = 6;
		Color handleColor = currentTheme.SelectionHighlight;

		array<Point>^ leftArrow = gcnew array<Point>(3) {
			Point(barBounds.Left + arrowSize, barBounds.Top + barBounds.Height / 2 - arrowSize),
				Point(barBounds.Left, barBounds.Top + barBounds.Height / 2),
				Point(barBounds.Left + arrowSize, barBounds.Top + barBounds.Height / 2 + arrowSize)
		};

		array<Point>^ rightArrow = gcnew array<Point>(3) {
			Point(barBounds.Right - arrowSize, barBounds.Top + barBounds.Height / 2 - arrowSize),
				Point(barBounds.Right, barBounds.Top + barBounds.Height / 2),
				Point(barBounds.Right - arrowSize, barBounds.Top + barBounds.Height / 2 + arrowSize)
		};

		g->FillPolygon(gcnew SolidBrush(handleColor), leftArrow);
		g->FillPolygon(gcnew SolidBrush(handleColor), rightArrow);
	}

	int Widget_Timeline::GetTicksPerMeasure()
	{
		return TICKS_PER_QUARTER * 4 * currentTimeSignatureNumerator / currentTimeSignatureDenominator;
	}

	int Widget_Timeline::GetTicksPerBeat()
	{
		return TICKS_PER_QUARTER * 4 / currentTimeSignatureDenominator;
	}

	float Widget_Timeline::GetSubdivisionLevel()
	{
		// Calculate how many subdivisions we can fit based on zoom level
		int pixelsPerBeat = TicksToPixels(GetTicksPerBeat());
		float subdivLevel = pixelsPerBeat / (float)MIN_PIXELS_BETWEEN_GRIDLINES;

		// Extended subdivision levels for higher zoom
		if (subdivLevel >= 64) return 64;
		if (subdivLevel >= 32) return 32;
		if (subdivLevel >= 16) return 16;
		if (subdivLevel >= 8) return 8;
		if (subdivLevel >= 4) return 4;
		if (subdivLevel >= 2) return 2;
		return 1;
	}

	void Widget_Timeline::DrawTrackHeaders(Graphics^ g)
	{
		if (tracks->Count == 0) return;

		// Fill header background
		Rectangle headerBackground(0, HEADER_HEIGHT, TRACK_HEADER_WIDTH, Height - HEADER_HEIGHT);
		g->FillRectangle(resourceManager->GetBrush(currentTheme.HeaderBackground), headerBackground);

		for each(Track ^ track in tracks)
		{
			if (!visibilityTracker->IsTrackVisible(track)) {
				continue;
			}

			Rectangle trackBounds = GetTrackBounds(track);
			Rectangle headerBounds(0, trackBounds.Y, TRACK_HEADER_WIDTH, trackBounds.Height);

			headerBounds.Y += scrollPosition->Y;

			// Draw header background
			Color headerBg = track->IsSelected ? currentTheme.SelectionHighlight : currentTheme.HeaderBackground;
			g->FillRectangle(resourceManager->GetBrush(headerBg), headerBounds);

			DrawTrackName(g, track, headerBounds);

			// Draw borders
			Pen^ borderPen = resourceManager->GetGridPen(currentTheme.TrackBorder, 1.0f);
			g->DrawRectangle(borderPen, headerBounds);
			g->DrawLine(borderPen, TRACK_HEADER_WIDTH, headerBounds.Y, TRACK_HEADER_WIDTH, headerBounds.Bottom);
		}
	}

	void Widget_Timeline::DrawTrackContent(Graphics^ g)
	{
		if (tracks->Count == 0 || measures->Count == 0) return;

		// Save the original clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create a clipping region for the content area
		Rectangle contentArea(0, HEADER_HEIGHT, Width, Height - HEADER_HEIGHT);
		g->SetClip(contentArea);

		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		g->TextRenderingHint = System::Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		// Draw in correct layer order:

		// 1. First draw track backgrounds
		for each (Track ^ track in tracks)
		{
			// Skip if track is not visible
			int trackTop	= GetTrackTop(track) + scrollPosition->Y;
			int trackHeight = GetTrackHeight(track);
			int trackBottom = trackTop + trackHeight;

			if (trackBottom < HEADER_HEIGHT || trackTop > Height) continue;

			Rectangle contentBounds = GetTrackContentBounds(track);
			contentBounds.Y = trackTop;
			g->FillRectangle(gcnew SolidBrush(currentTheme.TrackBackground), contentBounds);
		}

		// 2. Draw grid lines (these should be visible through track content)
		DrawGridLines(g);

		// 3. Draw track content (events and tablature)
		for each (Track ^ track in tracks)
		{
			int trackTop = GetTrackTop(track) + scrollPosition->Y;
			int trackHeight = GetTrackHeight(track);
			int trackBottom = trackTop + trackHeight;

			if (trackBottom < HEADER_HEIGHT || trackTop > Height) continue;

			//Rectangle trackBounds = GetTrackBounds(track);
			//trackBounds.Y = trackTop;

			Rectangle contentBounds = GetTrackContentBounds(track);
			contentBounds.Y = trackTop;

			// Draw events and tablature
			DrawTrackEvents(g, track, contentBounds);
			if (track->ShowTablature)
			{
				DrawTrackTablature(g, track, contentBounds);
			}
		}

		// 4. Draw track headers and borders (these should be on top)
		DrawTrackHeaders(g);
		DrawTrackDividers(g);

		// 5. Draw selection rectangles and tool previews
		DrawSelectionAndPreviews(g);

		// Restore original clip region
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawTrackBackground(Graphics^ g)
	{
		if (tracks->Count == 0 || measures->Count == 0) return;

		// Set up the graphics context
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		g->TextRenderingHint = System::Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		// Calculate the total available area for tracks
		Rectangle tracksArea(0, HEADER_HEIGHT, Width, Height - HEADER_HEIGHT);

		// Draw background for tracks area
		g->FillRectangle(gcnew SolidBrush(currentTheme.TrackBackground), tracksArea);

		// Draw each track's background
		for each (Track ^ track in tracks) {
			Rectangle contentBounds = GetTrackContentBounds(track);
			g->FillRectangle(gcnew SolidBrush(currentTheme.TrackBackground), contentBounds);
		}
	}

	void Widget_Timeline::DrawTrackDividers(Graphics^ g)
	{
		Pen^ dividerPen = gcnew Pen(currentTheme.TrackBorder);
		int y = HEADER_HEIGHT;

		for each (Track ^ track in tracks) {
			Rectangle trackContentBounds = GetTrackContentBounds(track);
			int dividerY = trackContentBounds.Bottom + scrollPosition->Y;

			if (dividerY >= 0 && dividerY <= this->Height) {
				// Draw differently if this is the hover track
				if (track == resizeHoverTrack) {
					// Draw a highlighted divider across the full width
					Pen^ hoverPen = gcnew Pen(currentTheme.SelectionHighlight, 2);
					g->DrawLine(hoverPen, 0, dividerY, Width, dividerY);
					delete hoverPen;
				}
				else {
					g->DrawLine(dividerPen, 0, dividerY, Width, dividerY);
				}
			}

			// Draw vertical divider between header and content
			int headerBottom = y + GetTrackHeight(track);
			if (headerBottom >= scrollPosition->Y && y <= scrollPosition->Y + this->Height) {
				g->DrawLine(dividerPen, TRACK_HEADER_WIDTH, y, TRACK_HEADER_WIDTH, headerBottom);
			}

			y += GetTrackHeight(track);
		}

		delete dividerPen;
	}

	void Widget_Timeline::DrawTrackName(Graphics^ g, Track^ track, Rectangle headerBounds)
	{
		// Only draw if we have a track name
		if (!String::IsNullOrEmpty(track->Name))
		{
			Rectangle textBounds = headerBounds;
			textBounds.X += TRACK_PADDING;
			textBounds.Y += TRACK_PADDING;
			textBounds.Width -= (TRACK_PADDING * 2);
			textBounds.Height -= (TRACK_PADDING * 2);

			StringFormat^ format = gcnew StringFormat();
			format->Alignment		= StringAlignment::Near;
			format->LineAlignment	= StringAlignment::Center;
			format->Trimming		= StringTrimming::EllipsisCharacter;

			g->DrawString(track->Name, measureFont, resourceManager->GetBrush(currentTheme.Text), textBounds, format);

			delete format;
		}
	}

	void Widget_Timeline::DrawTrackBorders(Graphics^ g, Track^ track, Rectangle bounds)
	{
		Pen^ borderPen = gcnew Pen(currentTheme.TrackBorder);

		// Draw outer rectangle
		g->DrawRectangle(borderPen, bounds);

		// Draw vertical separator between header and content
		g->DrawLine(borderPen,
			TRACK_HEADER_WIDTH, bounds.Y,
			TRACK_HEADER_WIDTH, bounds.Bottom);

		// Draw resize handle if this is not the last track
		if (track != tracks[tracks->Count - 1]) {
			int handleY = bounds.Bottom - TRACK_RESIZE_HANDLE_HEIGHT;
			g->DrawLine(borderPen,
				0, handleY,
				Width, handleY);
		}

		delete borderPen;
	}

	void Widget_Timeline::DrawGridLines(Graphics^ g)
	{
		// Calculate total height for grid lines
		int totalHeight = GetTotalTracksHeight();
		Rectangle contentRect(
			TRACK_HEADER_WIDTH,
			HEADER_HEIGHT,
			Width - TRACK_HEADER_WIDTH,
			totalHeight
		);

		// Draw grid lines in order
		DrawSubdivisionLines(g, contentRect);
		DrawBeatLines		(g, contentRect);
		DrawMeasureLines	(g, contentRect);
	}

	void Widget_Timeline::DrawSelectionAndPreviews(Graphics^ g)
	{
		// Draw selection rectangle if active
		Rectangle selRect;
		if (currentToolType == TimelineToolType::Pointer) {
			PointerTool^ pointerTool = (PointerTool^)tools[TimelineToolType::Pointer];
			selRect = pointerTool->SelectionRect;
		}
		else if (currentToolType == TimelineToolType::Duration) {
			DurationTool^ durationTool = (DurationTool^)tools[TimelineToolType::Duration];
			selRect = durationTool->SelectionRect;
		}

		if (selRect.Width > 0 && selRect.Height > 0) {
			Color selectionColor = Color::FromArgb(50, currentTheme.SelectionHighlight);
			g->FillRectangle(gcnew SolidBrush(selectionColor), selRect);
			g->DrawRectangle(gcnew Pen(currentTheme.SelectionHighlight), selRect);
		}

		// Draw tool preview overlays
		ToolPreview(g);
	}

	void Widget_Timeline::DrawTrackEvents(Graphics^ g, Track^ track, Rectangle bounds)
	{
		int startTick = PixelsToTicks(-scrollPosition->X);
		int endTick = PixelsToTicks(-scrollPosition->X + Width - TRACK_HEADER_WIDTH);

		PointerTool^ pointerTool = (PointerTool^)tools[TimelineToolType::Pointer];
		DurationTool^ durationTool = (DurationTool^)tools[TimelineToolType::Duration];
		bool isDragging = pointerTool->IsDragging;
		bool isMultiTrackSelection = pointerTool->IsMultiTrackSelection;

		DrawTool^ drawTool = (DrawTool^)tools[TimelineToolType::Draw];
		bool isDrawToolMoving = (currentToolType == TimelineToolType::Draw &&
			drawTool->CurrentMode == DrawTool::DrawMode::Move &&
			drawTool->IsMoving);

		// STEP 1: Draw non-selected bars
		for each(BarEvent ^ bar in track->Events) {
			if (bar->StartTick + bar->Length < startTick || bar->StartTick > endTick) continue;

			bool isSelected = false;
			if (currentToolType == TimelineToolType::Duration) {
				isSelected = durationTool->SelectedBars->Contains(bar);
			}
			else if (currentToolType == TimelineToolType::Pointer) {
				isSelected = pointerTool->SelectedBars->Contains(bar);
			}

			if (!isSelected && (!isDrawToolMoving || bar != drawTool->HoverBar)) {
				DrawNormalBar(g, bar, bounds);
			}
		}

		// STEP 2: If dragging with pointer tool, draw ghost bars in their original tracks
		if (isDragging && currentToolType == TimelineToolType::Pointer) {
			// For multi-track selection, show ghosts in original positions
			if (isMultiTrackSelection) {
				for each(BarEvent ^ bar in track->Events) {
					if (pointerTool->SelectedBars->Contains(bar)) {
						DrawGhostBar(g, bar, bounds);
					}
				}
			}
			// For single-track selection, only show ghosts in source track
			else if (track == pointerTool->DragSourceTrack) {
				for each(BarEvent ^ bar in pointerTool->SelectedBars) {
					DrawGhostBar(g, bar, bounds);
				}
			}
		}
		else if (isDrawToolMoving && drawTool->HoverBar != nullptr) {
			if (track == drawTool->SourceTrack) {
				DrawGhostBar(g, drawTool->HoverBar, bounds);
			}
		}

		// STEP 3: Draw selected/dragged bars
		if (currentToolType == TimelineToolType::Pointer && isDragging) {
			Point mousePos = pointerTool->CurrentMousePosition;
			bool isOverHeader = mousePos.X <= TRACK_HEADER_WIDTH;

			if (!isOverHeader) {  // Only show preview if not over header
				if (isMultiTrackSelection) {
					// For multi-track selection, show dragged bars in their original tracks
					for each(BarEvent ^ bar in track->Events) {
						if (bar->StartTick + bar->Length < startTick || bar->StartTick > endTick) continue;
						if (pointerTool->SelectedBars->Contains(bar)) {
							DrawSelectedBar(g, bar, bounds);
						}
					}
				}
				else if (track == pointerTool->DragTargetTrack) {
					// For single-track selection, show all dragged bars in target track
					for each(BarEvent ^ bar in pointerTool->SelectedBars) {
						DrawSelectedBar(g, bar, bounds);
					}
				}
			}
		}
		else if (isDrawToolMoving && drawTool->HoverBar != nullptr) {
			if (track == drawTool->TargetTrack) {
				DrawSelectedBar(g, drawTool->HoverBar, bounds);
			}
		}
		else {
			// When not dragging, draw selected bars in their current tracks
			for each(BarEvent ^ bar in track->Events) {
				if (bar->StartTick + bar->Length < startTick || bar->StartTick > endTick) continue;

				bool isSelected = false;
				if (currentToolType == TimelineToolType::Duration) {
					isSelected = durationTool->SelectedBars->Contains(bar);
				}
				else if (currentToolType == TimelineToolType::Pointer) {
					isSelected = pointerTool->SelectedBars->Contains(bar);
				}

				if (isSelected) {
					DrawSelectedBar(g, bar, bounds);
				}
			}
		}

		// Handle paste preview if active
		if (pointerTool->IsPasting && pointerTool->PastePreviewBars != nullptr) {
			if (track != nullptr && pointerTool->CurrentMousePosition.X > TRACK_HEADER_WIDTH) {
				int currentTrackIndex = Tracks->IndexOf(track);

				// Draw preview bars that belong to this track
				for each(BarEvent ^ previewBar in pointerTool->PastePreviewBars) {
					// Check if this preview bar belongs to the current track
					// We stored the target track index in OriginalStartTick
					if (previewBar->OriginalStartTick == currentTrackIndex) {
						int x = TicksToPixels(previewBar->StartTick) +
							scrollPosition->X + TRACK_HEADER_WIDTH;
						int width = TicksToPixels(previewBar->Length);

						Rectangle barBounds(x, bounds.Y + TRACK_PADDING,
							width, bounds.Height - TRACK_PADDING * 2);

						// Create shadow effect
						for (int i = 3; i >= 0; i--) {
							Rectangle shadowBounds = barBounds;
							shadowBounds.Offset(i, i);
							Color shadowColor = Color::FromArgb(40 - (i * 10), 0, 0, 0);
							g->FillRectangle(gcnew SolidBrush(shadowColor), shadowBounds);
						}

						// Draw semi-transparent bar with glowing edge
						Color previewColor = Color::FromArgb(180, previewBar->Color);
						g->FillRectangle(gcnew SolidBrush(previewColor), barBounds);

						// Draw glowing border
						for (int i = 2; i >= 0; i--) {
							Rectangle glowBounds = barBounds;
							glowBounds.Inflate(-i, -i);
							Color glowColor = Color::FromArgb(180 - (i * 40), 255, 255, 255);
							g->DrawRectangle(gcnew Pen(glowColor), glowBounds);
						}

						// Draw dashed outline
						Pen^ dashPen = gcnew Pen(Color::White, 1);
						array<float>^ dashPattern = { 4.0f, 4.0f };
						dashPen->DashPattern = dashPattern;
						g->DrawRectangle(dashPen, barBounds);
						delete dashPen;
					}
				}

				// Draw drop target indicator if this is the anchor track
				if (track == pointerTool->DragTargetTrack) {
					Rectangle trackBounds = bounds;
					Pen^ targetPen = gcnew Pen(currentTheme.SelectionHighlight, 2);
					array<float>^ dashPattern = { 6.0f, 3.0f };
					targetPen->DashPattern = dashPattern;
					g->DrawRectangle(targetPen, trackBounds);
					delete targetPen;
				}
			}
		}
	}

	void Widget_Timeline::DrawTrackTablature(Graphics^ g, Track^ track, Rectangle bounds)
	{
		if (!track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0)
			return;

		// Early exit if zoom level makes notes unreadable
		if (zoomLevel < 0.1) return;

		// Use fixed string spacing
		const float FIXED_STRING_SPACING = 12.0f; // Distance between strings
		const float TOTAL_TAB_HEIGHT = FIXED_STRING_SPACING * 5; // Height needed for 6 strings

		// Calculate vertical centering offset
		float availableHeight = (float)(bounds.Height - TRACK_PADDING * 2);
		float verticalOffset = bounds.Y + TRACK_PADDING + (availableHeight - TOTAL_TAB_HEIGHT) / 2;

		// Pre-calculate string Y positions with fixed spacing but centered in available height
		array<float>^ stringYPositions = gcnew array<float>(6);
		for (int i = 0; i < 6; i++) {
			stringYPositions[i] = verticalOffset + (i * FIXED_STRING_SPACING);
		}

		float fontSize = Math::Min(FIXED_STRING_SPACING * 0.7f, 10.0f);
		if (fontSize < 4.0f) return;  // Too small to be readable

		try {
			// Draw the strings with fixed spacing
			for (int i = 0; i < 6; i++) {
				g->DrawLine(cachedStringPen,
					(float)bounds.X, stringYPositions[i],
					(float)bounds.Right, stringYPositions[i]);
			}

			// Calculate visible tick range
			int visibleStartTick = PixelsToTicks(-scrollPosition->X);
			int visibleEndTick = PixelsToTicks(-scrollPosition->X + bounds.Width);

			// Track measure position
			int measureStartTick = 0;

			// Process only visible measures
			for (int i = 0; i < track->Measures->Count; i++) {
				TrackMeasure^ measure = track->Measures[i];
				if (measure == nullptr) {
					measureStartTick += measure->Length;
					continue;
				}

				int measureEndTick = measureStartTick + measure->Length;

				// Skip if measure is out of visible range
				if (measureStartTick > visibleEndTick || measureEndTick < visibleStartTick) {
					measureStartTick = measureEndTick;
					continue;
				}

				// Draw beats in this measure
				for each (Beat ^ beat in measure->Beats) {
					if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0)
						continue;

					int beatTick = measureStartTick + beat->StartTick;

					// Skip if beat is outside visible range
					if (beatTick > visibleEndTick || beatTick + beat->Duration < visibleStartTick)
						continue;

					float beatX = TicksToPixels(beatTick) + scrollPosition->X + TRACK_HEADER_WIDTH;

					// Draw duration lines for beats with multiple notes
					if (beat->Duration > 0 && beat->Notes->Count > 0) {
						DrawBeatDuration(g, beat, bounds, stringYPositions);
					}

					// Draw the notes
					for each (Note ^ note in beat->Notes) {
						if (note == nullptr || note->String < 1 || note->String > 6)
							continue;

						String^ fretText = note->Value.ToString();
						float textWidth = g->MeasureString(fretText, cachedTabFont).Width;
						float textX = beatX - (textWidth / 2.0f);
						float textY = stringYPositions[note->String - 1] - (cachedTabFont->Height / 2.0f);

						// Draw background for better readability
						RectangleF bgRect(textX - 1, textY, textWidth + 2, cachedTabFont->Height);
						g->FillRectangle(gcnew SolidBrush(Color::FromArgb(220, currentTheme.TrackBackground)),
							bgRect);

						// Draw the fret number
						g->DrawString(fretText, cachedTabFont, cachedTextBrush, textX, textY);
					}
				}

				measureStartTick = measureEndTick;
			}
		}
		finally {
			delete stringYPositions;
		}
	}

	void Widget_Timeline::DrawBeatNotes(Graphics^ g, Beat^ beat, Rectangle bounds, int beatTick, array<int>^ stringYPositions)
	{
		// Calculate x position once for all notes in this beat
		int x = TicksToPixels(beatTick) + scrollPosition->X + TRACK_HEADER_WIDTH;

		// Pre-calculate font metrics
		static float cachedCharWidth = g->MeasureString("0", cachedTabFont).Width;

		for each(Note ^ note in beat->Notes) {
			if (note == nullptr || note->String < 1 || note->String > 6) continue;

			String^ fretText = note->Value.ToString();
			float textX = x - (fretText->Length * cachedCharWidth / 2);
			float textY = stringYPositions[note->String - 1] - (cachedTabFont->Height / 2);

			g->DrawString(fretText, cachedTabFont, cachedTextBrush, textX, textY);
		}
	}

	void Widget_Timeline::DrawBeatDuration(Graphics^ g, Beat^ beat, Rectangle bounds, array<float>^ stringYPositions)
	{
		// Find the range of strings used in this beat
		int minString = 6;
		int maxString = 1;
		for each (Note ^ note in beat->Notes) {
			if (note == nullptr) continue;
			minString = Math::Min(minString, note->String);
			maxString = Math::Max(maxString, note->String);
		}

		// Calculate positions
		float startX = TicksToPixels(beat->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
		float endX = TicksToPixels(beat->StartTick + beat->Duration) + scrollPosition->X + TRACK_HEADER_WIDTH;

		// Draw vertical lines
		g->DrawLine(cachedDurationPen, startX, stringYPositions[minString - 1],
			startX, stringYPositions[maxString - 1]);
		g->DrawLine(cachedDurationPen, endX, stringYPositions[minString - 1],
			endX, stringYPositions[maxString - 1]);

		// Draw horizontal lines
		g->DrawLine(cachedDurationPen, startX, stringYPositions[minString - 1],
			endX, stringYPositions[minString - 1]);
		g->DrawLine(cachedDurationPen, startX, stringYPositions[maxString - 1],
			endX, stringYPositions[maxString - 1]);
	}

	Rectangle Widget_Timeline::GetTrackBounds(Track^ track)
	{
		int top = GetTrackTop(track);
		int height = GetTrackHeight(track);

		// Create full width bounds at the scrolled position
		return Rectangle(0, top, Width, height);
	}

	Rectangle Widget_Timeline::GetTrackHeaderBounds(Track^ track)
	{
		Rectangle bounds = GetTrackBounds(track);
		bounds.Width = TRACK_HEADER_WIDTH;
		return bounds;
	}

	SizeF Widget_Timeline::GetCachedTextSize(String^ text, Drawing::Font^ font, Graphics^ g) {
		SizeF size;
		if (!viewState->TextMeasurements->TryGetValue(text, size)) {
			size = g->MeasureString(text, font);
			viewState->TextMeasurements->Add(text, size);
		}
		return size;
	}

	void Widget_Timeline::SetTrackHeightPreset(Track^ track, TrackHeightPreset preset) {
		if (tracks->Contains(track)) {
			trackHeights[track] = static_cast<int>(preset);
			UpdateScrollBounds();
			Invalidate();
		}
	}

	void Widget_Timeline::SetAllTracksHeightPreset(TrackHeightPreset preset) {
		for each(Track ^ track in tracks) {
			trackHeights[track] = static_cast<int>(preset);
		}
		UpdateScrollBounds();
		Invalidate();
	}

	bool Widget_Timeline::IsOverTrackDivider(Point mousePoint, Track^% outTrack)
	{
		// Adjust mouse position for scroll
		int adjustedY = mousePoint.Y - scrollPosition->Y;
		outTrack = nullptr;

		// Start from header height
		int y = HEADER_HEIGHT;

		// Check each track except the last one (no divider after last track)
		for (int i = 0; i < tracks->Count; i++) {
			Track^ track = tracks[i];
			int height = GetTrackHeight(track);
			int dividerY = y + height;

			// Check if mouse is within the divider area (using adjusted Y position)
			if (adjustedY >= dividerY - TRACK_RESIZE_HANDLE_HEIGHT &&
				adjustedY <= dividerY + TRACK_RESIZE_HANDLE_HEIGHT) {
				outTrack = track;
				return true;
			}

			y += height;
		}

		return false;
	}

	void Widget_Timeline::BeginTrackResize(Track^ track, int mouseY)
	{
		trackBeingResized = track;
		resizeStartY = mouseY;
		initialTrackHeight = GetTrackHeight(track);

		// Change cursor to resize cursor
		this->Cursor = Cursors::SizeNS;
	}

	void Widget_Timeline::UpdateTrackResize(int mouseY)
	{
		if (trackBeingResized != nullptr) {
			int delta = mouseY - resizeStartY;
			int newHeight = Math::Max(MINIMUM_TRACK_HEIGHT, initialTrackHeight + delta);

			// Store previous total height
			int oldTotalHeight = GetTotalTracksHeight();

			// Update the track height
			SetTrackHeight(trackBeingResized, newHeight);

			// Get new total height
			int newTotalHeight = GetTotalTracksHeight();

			// If this changes total content height, ensure scroll position is still valid
			if (oldTotalHeight != newTotalHeight) {
				int viewportHeight = Height - HEADER_HEIGHT - hScrollBar->Height;

				// If we're scrolled to bottom, maintain bottom alignment
				if (-scrollPosition->Y + viewportHeight >= oldTotalHeight) {
					scrollPosition->Y = Math::Min(0, -(newTotalHeight - viewportHeight));
					if (vScrollBar != nullptr) {
						vScrollBar->Value = -scrollPosition->Y;
					}
				}

				// Update scrollbar range
				UpdateVerticalScrollBarRange();
			}

			Invalidate();
		}
	}

	void Widget_Timeline::EndTrackResize()
	{
		trackBeingResized = nullptr;

		// Check if we're still over a divider
		Track^ hoverTrack;
		if (IsOverTrackDivider(Point(this->PointToClient(Control::MousePosition)), hoverTrack)) {
			this->Cursor = Cursors::SizeNS;
		}
		else {
			this->Cursor = Cursors::Default;
		}
	}

	List<BarEvent^>^ Widget_Timeline::GetSelectedBars()
	{
		PointerTool^ pointerTool = (PointerTool^)tools[TimelineToolType::Pointer];
		return pointerTool->SelectedBars;
	}

	bool Widget_Timeline::IsBarSelected(BarEvent^ bar)
	{
		List<BarEvent^>^ selectedBars = GetSelectedBars();
		return selectedBars != nullptr && selectedBars->Contains(bar);
	}

	String^ Widget_Timeline::SaveBarEventsToFile(String^ filePath)
	{
		try
		{
			List<String^>^ lines = gcnew List<String^>();

			// Header with version
			lines->Add("MIDILightDrawer_BarEvents_v1.0");

			// Save pattern information - number of measures
			lines->Add(measures->Count.ToString());

			// Save time signatures for all measures
			for each(Measure ^ measure in measures) {
				lines->Add(String::Format("{0},{1}",
					measure->Numerator,
					measure->Denominator));
			}

			// Calculate total number of bars across all tracks
			int totalBars = 0;
			for each(Track ^ track in tracks) {
				totalBars += track->Events->Count;
			}
			lines->Add(totalBars.ToString());

			// Save each bar's data with track name
			for each(Track ^ track in tracks) {
				for each(BarEvent ^ bar in track->Events) {
					String^ barData = String::Format("{0},{1},{2},{3},{4},{5},{6}",
						bar->StartTick,
						bar->Length,
						tracks->IndexOf(track),
						bar->Color.R,
						bar->Color.G,
						bar->Color.B,
						track->Name
					);
					lines->Add(barData);
				}
			}

			System::IO::File::WriteAllLines(filePath, lines->ToArray());
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error saving bar events: {0}", ex->Message);
		}
	}

	String^ Widget_Timeline::LoadBarEventsFromFile(String^ filePath)
	{
		try
		{
			array<String^>^ lines = System::IO::File::ReadAllLines(filePath);
			if (lines->Length < 2 || !lines[0]->StartsWith("MIDILightDrawer_BarEvents_v1.0"))
				return "Invalid or unsupported file format version";

			// Parse number of measures
			int fileMeasureCount;
			if (!Int32::TryParse(lines[1], fileMeasureCount))
				return "Invalid measure count";

			// Verify measure count matches
			if (fileMeasureCount != measures->Count)
				return String::Format(
					"Pattern mismatch: File has {0} measures, but timeline has {1} measures",
					fileMeasureCount, measures->Count);

			// Verify time signatures for each measure
			for (int i = 0; i < measures->Count; i++)
			{
				String^ fileMeasureInfo = lines[2 + i];
				array<String^>^ parts = fileMeasureInfo->Split(',');

				if (parts->Length != 2)
					return String::Format("Invalid time signature format in measure {0}", i + 1);

				int fileNumerator, fileDenominator;
				if (!Int32::TryParse(parts[0], fileNumerator) ||
					!Int32::TryParse(parts[1], fileDenominator))
					return String::Format("Invalid time signature numbers in measure {0}", i + 1);

				if (fileNumerator != measures[i]->Numerator ||
					fileDenominator != measures[i]->Denominator)
					return String::Format(
						"Time signature mismatch at measure {0}: File has {1}/{2}, but timeline has {3}/{4}",
						i + 1, fileNumerator, fileDenominator,
						measures[i]->Numerator, measures[i]->Denominator);
			}

			// Create mapping of track names to indices
			Dictionary<String^, Track^>^ trackMap = gcnew Dictionary<String^, Track^>();
			for each(Track ^ track in tracks) {
				trackMap[track->Name] = track;
			}

			// Clear existing bars from all tracks
			for each(Track ^ track in tracks) {
				track->Events->Clear();
			}

			int barsStartLine = 2 + measures->Count;
			int barCount;
			if (!Int32::TryParse(lines[barsStartLine], barCount))
				return "Invalid bar count";

			// Load each bar
			for (int i = 0; i < barCount; i++)
			{
				String^ barData = lines[barsStartLine + 1 + i];
				array<String^>^ parts = barData->Split(',');
				if (parts->Length != 7)
					continue; // Skip invalid format

				String^ trackName = parts[6];
				if (!trackMap->ContainsKey(trackName))
					continue; // Skip if track doesn't exist

				Track^ targetTrack = trackMap[trackName];

				int startTick, length, r, g, b;
				if (!Int32::TryParse(parts[0], startTick) ||
					!Int32::TryParse(parts[1], length) ||
					!Int32::TryParse(parts[3], r) ||
					!Int32::TryParse(parts[4], g) ||
					!Int32::TryParse(parts[5], b))
					continue;

				BarEvent^ bar = gcnew BarEvent(
					startTick,
					length,
					Color::FromArgb(r, g, b)
				);

				targetTrack->Events->Add(bar);
			}

			// Sort events in each track
			for each(Track ^ track in tracks) {
				track->Events->Sort(Track::barComparer);
			}

			this->Invalidate();
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error loading bar events: {0}", ex->Message);
		}
	}

	void Widget_Timeline::LogPerformanceMetrics()
	{
		String^ report = performanceMetrics->GetReport();
		Console::WriteLine(report);
		performanceMetrics->Reset();
	}

	Rectangle Widget_Timeline::GetTrackContentBounds(Track^ track)
	{
		Rectangle bounds = GetTrackBounds(track);
		bounds.X = TRACK_HEADER_WIDTH;
		bounds.Width -= TRACK_HEADER_WIDTH;
		return bounds;
	}

	int Widget_Timeline::GetTrackTop(Track^ track)
	{
		int top = HEADER_HEIGHT;
		for each (Track ^ t in tracks) {
			if (t == track) break;
			top += GetTrackHeight(t);
		}

		return top;
	}

	int Widget_Timeline::GetTrackHeight(Track^ track)
	{
		// Get track height from parent's dictionary or use default
		int height;

		if (trackHeights->TryGetValue(track, height))
		{
			return height;
		}

		return Widget_Timeline::DEFAULT_TRACK_HEIGHT;
	}

	int Widget_Timeline::GetTotalTracksHeight()
	{
		int totalHeight = 0;
		for each (Track ^ track in tracks)
		{
			totalHeight += GetTrackHeight(track);
		}
		return totalHeight;
	}

	Track^ Widget_Timeline::GetTrackAtPoint(Point p)
	{
		if (p.Y < HEADER_HEIGHT) return nullptr;

		int y = HEADER_HEIGHT + ScrollPosition->Y;
		for each (Track ^ track in tracks)
		{
			int height = GetTrackHeight(track);

			if (p.Y >= y && p.Y < y + height)
			{
				return track;
			}
			y += height;
		}
		return nullptr;
	}

	BarEvent^ Widget_Timeline::GetBarAtPoint(Point p)
	{
		Track^ track = GetTrackAtPoint(p);
		if (track == nullptr) return nullptr;

		// Convert point to tick position
		int clickTick = PixelsToTicks(p.X - TRACK_HEADER_WIDTH - scrollPosition->X);

		// Get track content bounds for vertical check
		Rectangle trackBounds = GetTrackContentBounds(track);
		trackBounds.Y += scrollPosition->Y;

		// Check each bar in the track
		for each (BarEvent ^ bar in track->Events)
		{
			// First check if the click is within the bar's time range
			if (clickTick >= bar->StartTick && clickTick <= bar->StartTick + bar->Length)
			{
				// Then check if the click is within the track's vertical bounds
				if (p.Y >= trackBounds.Y + TRACK_PADDING && p.Y <= trackBounds.Y + trackBounds.Height - TRACK_PADDING)
				{
					return bar;
				}
			}
		}

		return nullptr;
	}

	void Widget_Timeline::RecalculateMeasurePositions() {
		int currentTick = 0;
		for each (Measure ^ measure in measures) {
			measure->StartTick = currentTick;
			currentTick += measure->Length;
		}
	}

	void Widget_Timeline::UpdateDrawingForMeasures()
	{
		RecalculateMeasurePositions();
		// Update any visual elements that depend on measure positions
	}

	double Widget_Timeline::GetVirtualWidth()
	{
		// Calculate total width based on musical content
		double tickScale = 16.0 / TICKS_PER_QUARTER;
		return (double)TotalTicks * tickScale * zoomLevel;
	}

	int Widget_Timeline::GetScrollUnits(double width)
	{
		// Convert pixel width to scroll units
		return (int)Math::Ceiling(width / SCROLL_UNIT);
	}

	void Widget_Timeline::UpdateVerticalScrollBarRange()
	{
		int totalHeight = GetTotalTracksHeight();
		// Ensure viewportHeight is at least 0
		int viewportHeight = Math::Max(0, Height - HEADER_HEIGHT - hScrollBar->Height);

		// Enable/update scrollbar if content exceeds viewport
		if (totalHeight > viewportHeight)
		{
			vScrollBar->Minimum = 0;
			vScrollBar->Maximum = totalHeight + 20;  // Use full total height as Maximum plus a little extra
			// Ensure LargeChange is at least 1
			vScrollBar->LargeChange = Math::Max(1, viewportHeight);
			vScrollBar->SmallChange = Math::Max(1, viewportHeight / 20); // Adjust scroll speed

			// Ensure value stays within valid range
			int maxValue = Math::Max(0, totalHeight - viewportHeight);
			int currentValue = Math::Min(-scrollPosition->Y, maxValue);
			vScrollBar->Value = Math::Max(0, currentValue);
		}
		else
		{
			// Content fits in viewport, but keep scrollbar visible
			vScrollBar->Minimum = 0;
			vScrollBar->Maximum = 0;
			vScrollBar->LargeChange = 1;
			vScrollBar->Value = 0;
			scrollPosition->Y = 0;
		}

		vScrollBar->Visible = true;
	}

	void Widget_Timeline::OnVerticalScroll(Object^ sender, ScrollEventArgs^ e)
	{
		scrollPosition->Y = -e->NewValue;
		Invalidate();
	}

	void Widget_Timeline::UpdateScrollBarRange()
	{
		// Calculate virtual width
		double virtualWidth = GetVirtualWidth();

		// Calculate viewport width (excluding header)
		int viewportWidth = Width - TRACK_HEADER_WIDTH;

		// Convert to scroll units
		int totalUnits = GetScrollUnits(virtualWidth);
		int viewportUnits = GetScrollUnits(viewportWidth);

		// Ensure viewportUnits is at least 1
		viewportUnits = Math::Max(1, viewportUnits);

		// Update scrollbar
		hScrollBar->Minimum = 0;
		hScrollBar->Maximum = Math::Max(0, totalUnits);
		hScrollBar->LargeChange = viewportUnits;
		hScrollBar->SmallChange = 1;

		// Ensure current position is valid
		int currentUnit = GetScrollUnits(-scrollPosition->X);
		hScrollBar->Value = Math::Min(Math::Max(currentUnit, hScrollBar->Minimum),
			hScrollBar->Maximum - hScrollBar->LargeChange + 1);
	}

	void Widget_Timeline::OnScroll(Object^ sender, ScrollEventArgs^ e) {
		// Convert scroll units to pixels
		double newScrollX = -(double)e->NewValue * SCROLL_UNIT;

		// Update scroll position
		scrollPosition->X = (int)Math::Round(newScrollX);

		// Request redraw
		Invalidate();
	}


	//////////////////////////
	// Track Implementation //
	//////////////////////////
	Track::Track(String^ trackName, int octave)
	{
		this->name			= trackName;
		this->octave		= octave;
		this->events		= gcnew List<BarEvent^>();
		this->isSelected	= false;
		this->Measures		= gcnew List<TrackMeasure^>();
		this->ShowTablature = true;
	}

	void Track::AddBar(int startTick, int length, Color color)
	{
		BarEvent^ newBar = gcnew BarEvent(startTick, length, color);
		events->Add(newBar);

		// Sort using the static comparison delegate
		events->Sort(barComparer);
	}

	void Track::RemoveBar(BarEvent^ bar)
	{
		if (events->Contains(bar)) {
			events->Remove(bar);
		}
	}

	int Track::CompareBarEvents(BarEvent^ a, BarEvent^ b)
	{
		return a->StartTick.CompareTo(b->StartTick);
	}


	/////////////////////////////
	// BarEvent Implementation //
	/////////////////////////////
	BarEvent::BarEvent(int start, int len, System::Drawing::Color c)
	{
		startTick = start;
		OriginalStartTick = start;
		length = len;
		color = c;
	}
}
#include "Widget_Timeline.h"
#include "Widget_Timeline_Tools.h"

namespace MIDILightDrawer
{
	// Widget_Timeline Implementation
	Widget_Timeline::Widget_Timeline()
	{
		this->_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());

		InitializeComponent();

		this->SetStyle(ControlStyles::Selectable, true);
		this->TabStop = true;
		this->Focus();
		this->BackColor = currentTheme.TrackBackground;

		if (USE_DIRECT_X)
		{
			// Optimize for Direct2D
			this->SetStyle(
				ControlStyles::AllPaintingInWmPaint |
				ControlStyles::UserPaint |
				ControlStyles::Opaque, true);  // Add Opaque style

			// Explicitly disable styles that can cause flickering
			this->SetStyle(
				ControlStyles::DoubleBuffer |
				ControlStyles::OptimizedDoubleBuffer |
				ControlStyles::SupportsTransparentBackColor, false);

			this->DoubleBuffered = false;
		}
		else
		{
			// Original GDI+ settings
			this->DoubleBuffered = true;
			this->SetStyle(
				ControlStyles::AllPaintingInWmPaint |
				ControlStyles::UserPaint |
				ControlStyles::OptimizedDoubleBuffer, true);
		}

		// Initialize state
		currentTheme = Theme_Manager::Get_Instance()->GetTimelineTheme();
		zoomLevel = 1.0;
		scrollPosition = gcnew Point(0, 0);
		tracks = gcnew List<Track^>();
		measures = gcnew List<Measure^>();

		InitializeToolSystem();
		InitializeTablatureResources();

		// Initialize fonts
		measureFont = gcnew Drawing::Font("Arial", 10);
		markerFont = gcnew Drawing::Font("Arial", 9);

		// Buffer will be initialized in OnHandleCreated
		bufferContext = nullptr;
		graphicsBuffer = nullptr;

		D2DRenderer	= gcnew Timeline_Direct2DRenderer(tracks, measures, zoomLevel, scrollPosition);
		if (D2DRenderer->Initialize(this)) {
			D2DRenderer->Resize(this->Width, this->Height);
			D2DRenderer->SetThemeColors(currentTheme.Background, currentTheme.HeaderBackground, currentTheme.Text, currentTheme.MeasureLine, currentTheme.BeatLine, currentTheme.SubdivisionLine, currentTheme.SelectionHighlight, currentTheme.TrackBackground, currentTheme.TrackBackground);
			D2DRenderer->SetTimelineAccess(this);
		}

		resourceManager		= gcnew TimelineResourceManager();
		performanceMetrics	= gcnew PerformanceMetrics();
	}

	Widget_Timeline::~Widget_Timeline()
	{
		delete cachedTabFont;
		delete cachedStringPen;
		delete cachedDurationPen;
		delete cachedTextBrush;
		delete performanceMetrics;
	}

	void Widget_Timeline::AddTrack(String^ name, int octave)
	{
		Track^ track = gcnew Track(name, octave);
		track->Height = Widget_Timeline::DEFAULT_TRACK_HEIGHT;
		tracks->Add(track);

		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::AddMeasure(int numerator, int denominator, int tempo) {
		AddMeasure(numerator, denominator, tempo, "");
	}

	void Widget_Timeline::AddMeasure(int numerator, int denominator, int tempo, String^ marker_text)
	{
		int startTick = TotalTicks;
		Measure^ newMeasure = gcnew Measure(startTick, numerator, denominator, tempo, marker_text);
		measures->Add(newMeasure);

		for each (Track ^ track in tracks) {
			track->Measures->Add(gcnew TrackMeasure(newMeasure, track));
		}

		UpdateDrawingForMeasures();
		UpdateScrollBarRange();
		Invalidate();
	}

	Beat^ Widget_Timeline::AddBeat(Track^ track, int measureIndex, int startTick, int durationInTicks, bool is_dotted)
	{
		if (track == nullptr || measureIndex < 0 || measureIndex >= track->Measures->Count)
			return nullptr;

		Beat^ beat = gcnew Beat();
		beat->Track = track;
		beat->StartTick = startTick - INITIAL_TICK_OFFSET;
		beat->Duration = durationInTicks;
		beat->IsDotted = is_dotted;

		TrackMeasure^ measure = track->Measures[measureIndex];
		measure->AddBeat(beat);

		Invalidate();
		return beat;
	}

	void Widget_Timeline::AddNote(Beat^ beat, int stringNumber, int value, bool is_tied)
	{
		if (beat == nullptr)
			return;

		Note^ note = gcnew Note();
		note->String	= stringNumber;
		note->Value		= value;	// Fret/Pitch
		note->IsTied	= is_tied;

		beat->Notes->Add(note);

		Invalidate();
	}

	void Widget_Timeline::AddBarToTrack(Track^ track, int startTick, int length, Color color)
	{
		if (tracks->Contains(track)) {
			track->AddBar(startTick, length, color);
			Invalidate();
		}
	}

	void Widget_Timeline::Clear()
	{
		for each (Track ^ track in tracks) {
			track->Measures->Clear();
		}

		// Clear all collections
		tracks->Clear();
		measures->Clear();

		// Reset selection state
		selectedTrack = nullptr;
		hoveredTrack = nullptr;
		selectedBar = nullptr;

		// Reset scroll and zoom
		scrollPosition = gcnew Point(0, 0);
		zoomLevel = 1.0;

		if (D2DRenderer != nullptr) {
			D2DRenderer->SetZoomLevel(zoomLevel);
			D2DRenderer->SetScrollPositionReference(scrollPosition);
		}

		// Reset background color
		this->BackColor = currentTheme.Background;

		// Force redraw
		UpdateBuffer();
		Invalidate();
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

		if (D2DRenderer != nullptr) {
			D2DRenderer->SetZoomLevel(zoomLevel);
		}

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

	void Widget_Timeline::SetTrackHeight(Track^ track, int height)
	{
		if (track == nullptr) return;

		// Calculate minimum height based on whether tablature is shown
		int minHeight = track->ShowTablature ? (int)MIN_TRACK_HEIGHT_WITH_TAB : MINIMUM_TRACK_HEIGHT;

		// Ensure height is within acceptable bounds
		height = Math::Max(minHeight, height);

		track->Height = height;

		int totalHeight = HEADER_HEIGHT;
		for each (Track ^ t in tracks) {
			totalHeight += t->Height;
		}

		// Update scrollbars and bounds
		UpdateVerticalScrollBarRange();
		UpdateScrollBounds();

		// Ensure current scroll position is still valid
		int viewportHeight = Height - HEADER_HEIGHT - hScrollBar->Height;
		if (-scrollPosition->Y + viewportHeight > totalHeight)
		{
			scrollPosition->Y = Math::Min(0, -(totalHeight - viewportHeight));
			vScrollBar->Value = -scrollPosition->Y;
		}

		Invalidate();
	}

	void Widget_Timeline::SetAllTracksHeight(int height)
	{
		for each (Track ^ track in tracks)
		{
			SetTrackHeight(track, height);
		}
	}

	void Widget_Timeline::SetToolSnapping(SnappingType type)
	{
		this->snappingType = type;
	}

	void Widget_Timeline::SetCurrentTool(TimelineToolType tool)
	{
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

	int Widget_Timeline::SnapTickToGrid(int tick)
	{
		// Get current subdivision level based on zoom
		float subdivLevel = GetSubdivisionLevel();

		// Calculate snap resolution based on subdivision level
		int snapResolution = TICKS_PER_QUARTER / (int)subdivLevel;

		// Round to nearest snap point
		//return ((tick + (snapResolution / 2)) / snapResolution) * snapResolution;

		// Calculate the grid point to the left of the given tick
		return (tick / snapResolution) * snapResolution;
	}

	int Widget_Timeline::SnapTickBasedOnType(int tick, Point mousePos)
	{
		const int SNAP_THRESHOLD = 480; // Maximum distance in ticks for bar snapping

		switch (snappingType)
		{
		case SnappingType::Snap_None:
			return tick;

		case SnappingType::Snap_Grid:
			return SnapTickToGrid(tick);

		case SnappingType::Snap_Bars:
		{
			// Get the track under the mouse pointer
			Track^ currentTrack = GetTrackAtPoint(mousePos);
			if (currentTrack == nullptr) return tick;

			int closestEndTick = tick;
			int minEndDistance = SNAP_THRESHOLD;

			// Only check bars in the current track
			for each (BarEvent ^ bar in currentTrack->Events)
			{
				// Only check distance to bar end
				int endTick = bar->StartTick + bar->Duration;
				int endDistance = Math::Abs(endTick - tick);
				if (endDistance < minEndDistance)
				{
					minEndDistance = endDistance;
					closestEndTick = endTick;
				}
			}

			// Return the closest edge tick if within threshold
			if (minEndDistance < SNAP_THRESHOLD)
			{
				return closestEndTick;
			}

			// If no close bars found, return original tick
			return tick;
		}

		case SnappingType::Snap_Tablature:
		{
			// Get the track under the mouse pointer
			Track^ currentTrack = GetTrackAtPoint(mousePos);
			if (currentTrack == nullptr) return tick;

			// Only proceed if the track has tablature visible
			if (!currentTrack->ShowTablature) return tick;

			// Find the nearest left-side beat in the current track
			int nearestLeftBeatTick = -1;  // Initialize to invalid value
			int smallestDistance = Int32::MaxValue;

			// Get the measure containing this tick
			Measure^ measure = GetMeasureAtTick(tick);
			if (measure == nullptr) return tick;

			// Find corresponding track measure
			TrackMeasure^ trackMeasure = nullptr;
			for each (TrackMeasure ^ tm in currentTrack->Measures)
			{
				if (tm->StartTick == measure->StartTick)
				{
					trackMeasure = tm;
					break;
				}
			}

			if (trackMeasure == nullptr) return tick;

			// Look for the closest beat that's to the left of our current position
			for each (Beat ^ beat in trackMeasure->Beats)
			{
				if (beat->StartTick <= tick)  // Only consider beats to the left
				{
					int distance = tick - beat->StartTick;
					if (distance < smallestDistance)
					{
						smallestDistance = distance;
						nearestLeftBeatTick = beat->StartTick;
					}
				}
			}

			// If we found a beat to the left, use it, otherwise fallback to grid
			if (nearestLeftBeatTick != -1)
			{
				return nearestLeftBeatTick;
			}

			return tick;
		}

		default:
			return tick;
		}
	}

	void Widget_Timeline::ScrollToMeasure(int measureNumber)
	{
		// Validate measure number
		if (measureNumber < 1 || measureNumber > measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + measures->Count);
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

	int Widget_Timeline::GetMeasureStartTick(int measureNumber)
	{
		if (measureNumber < 1 || measureNumber > measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + measures->Count);
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
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + measures->Count);
		}

		return measures[measureNumber - 1]->Length;
	}

	void Widget_Timeline::UpdateCursor(System::Windows::Forms::Cursor^ cursor)
	{
		if (this->Cursor != cursor) {
			this->Cursor = cursor;
		}
	}

	bool Widget_Timeline::IsBarSelected(BarEvent^ bar)
	{
		List<BarEvent^>^ selectedBars = GetSelectedBars();
		return selectedBars != nullptr && selectedBars->Contains(bar);
	}

	int Widget_Timeline::TicksToPixels(int ticks)
	{
		if (this->D2DRenderer == nullptr) {
			return 0;
		}

		float Pixels = this->D2DRenderer->TicksToPixels(ticks);
		
		// Protect against overflow
		if (Pixels > Int32::MaxValue) return Int32::MaxValue;
		if (Pixels < Int32::MinValue) return Int32::MinValue;

		return (int)Math::Round(Pixels);
	}

	int Widget_Timeline::PixelsToTicks(int pixels)
	{
		if (this->D2DRenderer == nullptr) {
			return 0;
		}

		return (int)Math::Round(this->D2DRenderer->PixelsToTicks(pixels));
	}

	Track^ Widget_Timeline::GetTrackAtPoint(Point p)
	{
		if (D2DRenderer == nullptr) {
			return nullptr;
		}

		return D2DRenderer->GetTrackAtPoint(p);
	}

	Rectangle Widget_Timeline::GetTrackBounds(Track^ track)
	{
		if (D2DRenderer == nullptr) {
			return Rectangle();
		}

		return D2DRenderer->GetTrackBounds(track);
	}

	Rectangle Widget_Timeline::GetTrackHeaderBounds(Track^ track)
	{
		if (D2DRenderer == nullptr) {
			return Rectangle();
		}

		return D2DRenderer->GetTrackHeaderBounds(track);
	}

	Rectangle Widget_Timeline::GetTrackContentBounds(Track^ track)
	{
		if (D2DRenderer == nullptr) {
			return Rectangle();
		}
		
		return D2DRenderer->GetTrackContentBounds(track);
	}

	Measure^ Widget_Timeline::GetMeasureAtTick(int tick)
	{
		if (D2DRenderer == nullptr) {
			return nullptr;
		}
		
		return D2DRenderer->GetMeasureAtTick(tick);
	}

	BarEvent^ Widget_Timeline::GetBarAtPoint(Point p)
	{
		if (D2DRenderer == nullptr) {
			return nullptr;
		}

		return D2DRenderer->GetBarAtPoint(p);
	}

	List<BarEvent^>^ Widget_Timeline::GetSelectedBars()
	{
		PointerTool^ pointerTool = (PointerTool^)tools[TimelineToolType::Pointer];
		return pointerTool->SelectedBars;
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
			for each (Measure ^ measure in measures) {
				lines->Add(String::Format("{0},{1}",
					measure->Numerator,
					measure->Denominator));
			}

			// Calculate total number of bars across all tracks
			int totalBars = 0;
			for each (Track ^ track in tracks) {
				totalBars += track->Events->Count;
			}
			lines->Add(totalBars.ToString());

			// Save each bar's data with track name
			for each (Track ^ track in tracks) {
				for each (BarEvent ^ bar in track->Events) {
					String^ barData = String::Format("{0},{1},{2},{3},{4},{5},{6}",
						bar->StartTick,
						bar->Duration,
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
			for each (Track ^ track in tracks) {
				trackMap[track->Name] = track;
			}

			// Clear existing bars from all tracks
			for each (Track ^ track in tracks) {
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
			for each (Track ^ track in tracks) {
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


	///////////////////////
	// Protected Methods //
	///////////////////////
	void Widget_Timeline::OnPaint(PaintEventArgs^ e)
	{
		performanceMetrics->StartFrame();

		if (USE_DIRECT_X)
		{
			// Direct2D rendering path
			if (D2DRenderer->BeginDraw())
			{
				// Clear the background
				if(this->Measures->Count == 0) {
					D2DRenderer->DrawWidgetBackground();
				}
				
				D2DRenderer->DrawTrackBackground();
				D2DRenderer->DrawMeasureNumbers();
				D2DRenderer->DrawTrackContent(resizeHoverTrack);
				D2DRenderer->DrawToolPreview();

				D2DRenderer->EndDraw();
			}
		}
		else
		{
			// Original GDI+ rendering path
			if (graphicsBuffer != nullptr)
			{
				// Clear the background first
				graphicsBuffer->Graphics->Clear(currentTheme.Background);

				// Draw components in correct order
				DrawTrackBackground(graphicsBuffer->Graphics);
				DrawMeasureNumbers(graphicsBuffer->Graphics);
				DrawTrackContent(graphicsBuffer->Graphics);
				DrawToolPreview(graphicsBuffer->Graphics);

				// Render the buffer
				graphicsBuffer->Render(e->Graphics);
			}
			else
			{
				e->Graphics->Clear(currentTheme.Background);
				DrawTrackBackground(e->Graphics);
				DrawMeasureNumbers(e->Graphics);
				DrawTrackContent(e->Graphics);
				DrawToolPreview(e->Graphics);
			}
		}

		performanceMetrics->EndFrame();
	}

	void Widget_Timeline::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		if (USE_DIRECT_X && D2DRenderer != nullptr) {
			D2DRenderer->Resize(Width, Height);
		}

		UpdateBuffer();
		UpdateScrollBarRange();
		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::OnMouseDown(MouseEventArgs^ e)
	{
		if (hoveredButton.Track != nullptr)
		{
			switch (hoveredButton.ButtonIndex)
			{
				case 0: // Tablature toggle button
					hoveredButton.Track->ShowTablature = !hoveredButton.Track->ShowTablature;
					break;
				case 1: // Notation toggle button (only for drum tracks)
					if (hoveredButton.Track->IsDrumTrack) {
						hoveredButton.Track->ShowAsStandardNotation = !hoveredButton.Track->ShowAsStandardNotation;
					}
					break;
			}
			Invalidate();
			return;  // Don't process other mouse down logic
		}
		
		Track^ resizeTrack;
		if (IsOverTrackDivider(Point(e->X, e->Y), resizeTrack))
		{
			BeginTrackResize(resizeTrack, e->Y);
			return;
		}

		// Normal mouse down handling
		this->Focus();
		Control::OnMouseDown(e);

		if (currentTool != nullptr)
		{
			currentTool->OnMouseDown(e);
		}
	}

	void Widget_Timeline::OnMouseMove(MouseEventArgs^ e)
	{
		Track^ hoverTrack;
		bool isOverDivider = IsOverTrackDivider(Point(e->X, e->Y), hoverTrack);

		Track^ currentTrack = GetTrackAtPoint(Point(e->X, e->Y));
		TrackButtonId newHoveredButton;
		bool isOverAnyButton = false;

		if (currentTrack != nullptr) {
			// Check each button
			// Adjust number based on max buttons
			for (int i = 0; i < 2; i++)	
			{ 
				if (IsOverTrackButton(currentTrack, i, Point(e->X, e->Y)))
				{
					newHoveredButton.Track = currentTrack;
					newHoveredButton.ButtonIndex = i;
					isOverAnyButton = true;
					break;
				}
			}
		}

		// Update hover states
		if (newHoveredButton.Track != hoveredButton.Track ||
			newHoveredButton.ButtonIndex != hoveredButton.ButtonIndex) {
			hoveredButton = newHoveredButton;
			Invalidate();
		}

		if (isOverAnyButton) {
			this->Cursor = Cursors::Hand;
			return;  // Don't process other mouse move logic
		}

		if (trackBeingResized != nullptr)
		{
			// If we're actively resizing, update the track height
			UpdateTrackResize(e->Y);
		}
		else if (isOverDivider)
		{
			// Just hovering over a divider
			if (resizeHoverTrack != hoverTrack)
			{
				resizeHoverTrack = hoverTrack;
				Invalidate(); // Redraw to show hover state if needed
			}

			this->Cursor = Cursors::SizeNS;
		}
		else
		{
			// Not over a divider
			if (resizeHoverTrack != nullptr) {
				resizeHoverTrack = nullptr;
				Invalidate();
			}

			// Handle normal tool behavior
			Control::OnMouseMove(e);
			if (currentTool != nullptr)
			{
				currentTool->OnMouseMove(e);
				// Only update cursor if we're not in a special state
				if (!isOverAnyButton && !isOverDivider && trackBeingResized == nullptr) {
					this->Cursor = currentTool->Cursor;
				}
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

	void Widget_Timeline::OnKeyDown(KeyEventArgs^ e)
	{
		Control::OnKeyDown(e);
		if (currentTool != nullptr) {
			currentTool->OnKeyDown(e);
		}
	}

	void Widget_Timeline::OnKeyUp(KeyEventArgs^ e)
	{
		Control::OnKeyUp(e);
		if (currentTool != nullptr) {
			currentTool->OnKeyUp(e);
		}
	}

	void Widget_Timeline::OnHandleCreated(EventArgs^ e)
	{
		UserControl::OnHandleCreated(e);

		if (USE_DIRECT_X && D2DRenderer != nullptr)
		{
			System::Diagnostics::Debug::WriteLine("OnHandleCreated: Initializing D2D");
			if (D2DRenderer->Initialize(this))
			{
				D2DRenderer->Resize(Width, Height);
			}
		}

		UpdateBuffer();
	}

	/////////////////////
	// Private Methods //
	/////////////////////
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
		tools->Add(TimelineToolType::Fade		, gcnew FadeTool	(this));
		tools->Add(TimelineToolType::Strobe		, gcnew StrobeTool	(this));

		// Set default tool
		currentToolType = TimelineToolType::Pointer;
		currentTool = tools[currentToolType];
	}

	void Widget_Timeline::InitializeTablatureResources()
	{
		if (cachedTabFont		!= nullptr) delete cachedTabFont;
		if (cachedStringPen		!= nullptr) delete cachedStringPen;
		if (cachedDurationPen	!= nullptr) delete cachedDurationPen;
		if (cachedTextBrush		!= nullptr) delete cachedTextBrush;

		cachedTabFont	= gcnew Drawing::Font("Arial", 10, FontStyle::Regular);
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

	void Widget_Timeline::UpdateBuffer()
	{
		if (USE_DIRECT_X) {
			return;
		}
		
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

	void Widget_Timeline::DrawMeasureLines(Graphics^ g, Rectangle contentRect)
	{
		// Use alpha for measure lines to ensure visibility
		Color measureColor = Color::FromArgb(255, currentTheme.MeasureLine);
		Pen^ measurePen = gcnew Pen(measureColor, 1.0f);

		try {
			int startTick	= PixelsToTicks(-scrollPosition->X);
			int endTick		= PixelsToTicks(-scrollPosition->X + Width);

			// Pixels to extend upward
			const int measureLineExtension = 4;  

			// Save original clip region
			System::Drawing::Region^ originalClip = g->Clip;

			int accumulated = 0;
			for each (Measure ^ measure in measures)
			{
				if (accumulated > endTick) {
					break;
				}

				if (accumulated + measure->Length < startTick) {
					accumulated += measure->Length;
					continue;
				}

				int x = TicksToPixels(accumulated) + scrollPosition->X + TRACK_HEADER_WIDTH;

				/*if (x >= TRACK_HEADER_WIDTH && x <= Width) {
					g->DrawLine(measurePen, x, contentRect.Y, x, contentRect.Y + contentRect.Height);
				}*/

				// Draw the main content area line (with original clip)
				g->Clip = originalClip;
				g->DrawLine(measurePen, x, contentRect.Y, x, contentRect.Y + contentRect.Height);

				// Draw the extension in the header area with a different clip region
				Rectangle headerClip(TRACK_HEADER_WIDTH, contentRect.Y - measureLineExtension, Width - TRACK_HEADER_WIDTH, measureLineExtension);
				g->SetClip(headerClip);
				g->DrawLine(measurePen, x, contentRect.Y - measureLineExtension, x, contentRect.Y);

				accumulated += measure->Length;
			}

			// Restore original clip
			g->Clip = originalClip;
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

	void Widget_Timeline::DrawMeasureNumbers(Graphics^ g)
	{
		if (!measureFont || measures->Count == 0) {
			return;
		}

		// Fill the header area with a solid color first
		Rectangle headerRect = Rectangle(0, 0, Width, HEADER_HEIGHT);
		g->FillRectangle(gcnew SolidBrush(currentTheme.HeaderBackground), headerRect);

		// Set up graphics for better quality
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		g->TextRenderingHint = System::Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		SolidBrush^ textBrush = gcnew SolidBrush(currentTheme.Text);

		// Create font for quarter note numbers (smaller than measure numbers)
		Drawing::Font^ quarterNoteFont = gcnew Drawing::Font(measureFont->FontFamily, measureFont->Size * 0.8f);

		// Determine if we should show quarter notes based on zoom level
		float subdivLevel = GetSubdivisionLevel();

		// Constants for vertical positioning
		const int markerTextY = 2;												// Marker text at the top
		const int timeSignatureY = markerTextY + markerFont->Height + 2;	// Time signature below marker
		const int measureNumberY = timeSignatureY + measureFont->Height + 4;	// Measure number at the bottom

		// Calculate visible range in ticks
		int startTick = PixelsToTicks(-scrollPosition->X);
		int endTick = PixelsToTicks(-scrollPosition->X + headerRect.Width - TRACK_HEADER_WIDTH);

		// Draw measure numbers and time signatures
		int accumulated = 0;
		int measureNumber = 1;

		for each (Measure ^ measure in measures)
		{
			// Convert tick position to pixels
			int x = TicksToPixels(accumulated) + scrollPosition->X + TRACK_HEADER_WIDTH;

			if (x >= TRACK_HEADER_WIDTH && x <= Width)
			{
				// Draw marker text if present
				if (!String::IsNullOrEmpty(measure->Marker_Text))
				{
					String^ markerText = measure->Marker_Text;
					SizeF markerSize = g->MeasureString(markerText, markerFont);
					float markerX = x - (markerSize.Width / 2);

					// Ensure text doesn't get cut off at the left edge
					markerX = Math::Max(markerX, (float)TRACK_HEADER_WIDTH + 2);

					// Draw with a slightly transparent background for better readability
					RectangleF markerBg = RectangleF(markerX - 2, markerTextY, markerSize.Width + 4, markerSize.Height);
					g->FillRectangle(gcnew SolidBrush(Color::FromArgb(180, currentTheme.HeaderBackground)), markerBg);
					g->DrawString(markerText, markerFont, textBrush, markerX, markerTextY);
				}

				// Draw measure number
				String^ numText = measureNumber.ToString();
				SizeF numSize = g->MeasureString(numText, measureFont);
				float textX = x - (numSize.Width / 2);

				// Ensure text doesn't get cut off at the left edge
				textX = Math::Max(textX, (float)TRACK_HEADER_WIDTH + 2);

				g->DrawString(numText, measureFont, textBrush, textX, (float)measureNumberY);


				// Determine if we should show subdivisions based on time signature and zoom level
				bool showSubdivisions = (measure->Denominator == 8) ?
					(subdivLevel >= 3) :  // Eighth-based measures
					(subdivLevel >= 2);   // Quarter-based measures

				if (showSubdivisions)
				{
					// Calculate subdivisions based on time signature
					int ticksPerBeat;

					if (measure->Denominator == 8)
					{
						// For 8th note denominator (e.g. 5/8, 7/8)
						ticksPerBeat = TICKS_PER_QUARTER / 2;  // Half of a quarter note
					}
					else  // denominator == 4
					{
						// For 4th note denominator (e.g. 4/4, 3/4)
						ticksPerBeat = TICKS_PER_QUARTER;
					}

					// Draw beat numbers
					for (int beat = 1; beat < measure->Numerator; beat++)
					{
						int beatTick = accumulated + (beat * ticksPerBeat);
						int beatX = TicksToPixels(beatTick) + scrollPosition->X + TRACK_HEADER_WIDTH;

						if (beatX >= TRACK_HEADER_WIDTH && beatX <= Width)
						{
							String^ beatText = String::Format("{0}.{1}", measureNumber, beat + 1);

							SizeF beatSize = g->MeasureString(beatText, quarterNoteFont);
							float beatTextX = beatX - (beatSize.Width / 2);

							// Ensure beat number doesn't get cut off
							beatTextX = Math::Max(beatTextX, (float)TRACK_HEADER_WIDTH + 2);

							// Draw with slightly transparent color
							Color beatColor = Color::FromArgb(180, currentTheme.Text);
							g->DrawString(beatText, quarterNoteFont, gcnew SolidBrush(beatColor), beatTextX, (float)measureNumberY + 4);
						}
					}
				}



				String^ timeSignature = measure->ToString();
				SizeF sigSize = g->MeasureString(timeSignature, markerFont);
				float sigX = x - (sigSize.Width / 2);

				// Ensure time signature doesn't get cut off
				sigX = Math::Max(sigX, (float)TRACK_HEADER_WIDTH + 2);

				g->DrawString(timeSignature, markerFont, textBrush, sigX, (float)timeSignatureY);
			}

			accumulated += measure->Length;
			measureNumber++;
		}

		delete quarterNoteFont;
		delete textBrush;
	}

	void Widget_Timeline::DrawToolPreview(Graphics^ g)
	{
		if (tracks->Count == 0 || measures->Count == 0) return;

		// Save the original clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create a clipping region for the content area
		Rectangle contentArea(0, HEADER_HEIGHT, Width, Height - HEADER_HEIGHT);
		g->SetClip(contentArea);

		// Get current tool
		if (currentToolType == TimelineToolType::Draw)
		{
			DrawToolPreviewDrawTool(g);
		}
		else if (currentToolType == TimelineToolType::Erase)
		{
			DrawToolPreviewEraseTool(g);
		}
		else if (currentToolType == TimelineToolType::Duration)
		{
			DrawToolPreviewDurationTool(g);
		}
		else if (currentToolType == TimelineToolType::Color)
		{
			DrawToolPreviewColorTool(g);
		}
		else if (currentToolType == TimelineToolType::Fade)
		{
			DrawToolPreviewFadeTool(g);
		}
		else if (currentToolType == TimelineToolType::Strobe)
		{
			DrawToolPreviewStrobeTool(g);
		}

		// Restore original clip region
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawToolPreviewDrawTool(Graphics^ g)
	{
		DrawTool^ drawTool = (DrawTool^)currentTool;

		// Handle different draw tool modes
		switch (drawTool->CurrentMode)
		{
		case DrawToolMode::Draw:	DrawToolPreviewDrawToolDraw(g, drawTool);	break;
		case DrawToolMode::Erase:	DrawToolPreviewDrawToolErase(g, drawTool);	break;
		case DrawToolMode::Move:	DrawToolPreviewDrawToolMove(g, drawTool);	break;
		case DrawToolMode::Resize:	DrawToolPreviewDrawToolResize(g, drawTool);	break;
		}
	}

	void Widget_Timeline::DrawToolPreviewDrawToolDraw(Graphics^ g, DrawTool^ drawTool)
	{
		if (drawTool->PreviewBar != nullptr)
		{
			DrawPreviewBar(g, drawTool->PreviewBar, nullptr, drawTool->CurrentMousePosition, BarPreviewType::Creation);
		}
	}

	void Widget_Timeline::DrawToolPreviewDrawToolErase(Graphics^ g, DrawTool^ drawTool)
	{
		BarEvent^ hoverBar = drawTool->HoverBar;
		if (hoverBar != nullptr)
		{
			// Get track containing hover bar
			Track^ track = nullptr;
			for each (Track ^ t in tracks)
			{
				if (t->Events->Contains(hoverBar))
				{
					track = t;
					break;
				}
			}

			if (track != nullptr)
			{
				Rectangle bounds = GetTrackContentBounds(track);
				Rectangle BarBounds = CalculateBarBounds(hoverBar, bounds);

				// Draw delete preview
				Color deleteColor = Color::FromArgb(100, 255, 0, 0); // Light red
				g->FillRectangle(gcnew SolidBrush(deleteColor), BarBounds);

				// Draw crossed-out effect
				Pen^ deletePen = gcnew Pen(Color::FromArgb(180, 255, 0, 0), 2);
				g->DrawRectangle(deletePen, BarBounds);

				g->DrawLine(deletePen, BarBounds.Left, BarBounds.Top, BarBounds.Right, BarBounds.Bottom);
				g->DrawLine(deletePen, BarBounds.Left, BarBounds.Bottom, BarBounds.Right, BarBounds.Top);
				delete deletePen;
			}
		}
	}

	void Widget_Timeline::DrawToolPreviewDrawToolMove(Graphics^ g, DrawTool^ drawTool)
	{
		BarEvent^ hoverBar = drawTool->HoverBar;
		if (hoverBar != nullptr && !drawTool->IsMoving)
		{
			// Get track containing hover bar
			Track^ track = nullptr;
			for each (Track ^ t in tracks)
			{
				if (t->Events->Contains(hoverBar))
				{
					track = t;
					break;
				}
			}

			if (track != nullptr)
			{
				Rectangle bounds = GetTrackContentBounds(track);
				Rectangle BarBounds = CalculateBarBounds(hoverBar, bounds);

				// Draw move preview (highlight)
				Color moveColor = Color::FromArgb(100, currentTheme.SelectionHighlight);
				g->FillRectangle(gcnew SolidBrush(moveColor), BarBounds);

				Pen^ movePen = gcnew Pen(currentTheme.SelectionHighlight, 2);
				g->DrawRectangle(movePen, BarBounds);

				delete movePen;

				// Draw move arrows or handles
				DrawMoveHandles(g, BarBounds);
			}
		}
	}

	void Widget_Timeline::DrawToolPreviewDrawToolResize(Graphics^ g, DrawTool^ drawTool)
	{
		BarEvent^ hoverBar = drawTool->HoverBar;
		if (hoverBar != nullptr && !drawTool->IsResizing)
		{
			// Get track containing hover bar
			Track^ track = nullptr;
			for each (Track ^ t in tracks)
			{
				if (t->Events->Contains(hoverBar))
				{
					track = t;
					break;
				}
			}

			if (track != nullptr)
			{
				Rectangle bounds = GetTrackContentBounds(track);
				Rectangle BarBounds = CalculateBarBounds(hoverBar, bounds);

				// Draw resize preview
				Color resizeColor = Color::FromArgb(100, currentTheme.SelectionHighlight);
				g->FillRectangle(gcnew SolidBrush(resizeColor), BarBounds);

				// Draw resize handle with enhanced visibility
				Rectangle handleBounds(BarBounds.Right - 5, BarBounds.Y, 5, BarBounds.Height);

				g->FillRectangle(gcnew SolidBrush(currentTheme.SelectionHighlight), handleBounds);

				// Add grip lines
				Pen^ gripPen = gcnew Pen(Color::White, 1);
				for (int i = 0; i < 3; i++)
				{
					g->DrawLine(gripPen, handleBounds.X + i + 1, handleBounds.Y + 2, handleBounds.X + i + 1, handleBounds.Bottom - 2);
				}
				delete gripPen;
			}
		}
	}

	void Widget_Timeline::DrawToolPreviewEraseTool(Graphics^ g)
	{
		EraseTool^ eraseTool = (EraseTool^)currentTool;

		// Draw selection rectangle if present
		Rectangle selRect = eraseTool->SelectionRect;
		if (selRect.Width > 0 && selRect.Height > 0)
		{
			Color selectionColor = Color::FromArgb(50, currentTheme.SelectionHighlight);
			g->FillRectangle(gcnew SolidBrush(selectionColor), selRect);
			g->DrawRectangle(gcnew Pen(currentTheme.SelectionHighlight), selRect);
		}

		// Check if we're hovering over a selected bar
		bool isHoveringSelected = eraseTool->HoverBar != nullptr && eraseTool->SelectedBars->Contains(eraseTool->HoverBar);

		// Draw all selected bars
		for each (BarEvent ^ bar in eraseTool->SelectedBars)
		{
			Track^ track = nullptr;

			// Find the track containing this bar
			for each (Track ^ t in Tracks)
			{
				if (t->Events->Contains(bar))
				{
					track = t;
					break;
				}
			}

			if (track != nullptr) {
				Rectangle bounds = GetTrackContentBounds(track);
				int x = TicksToPixels(bar->StartTick) + ScrollPosition->X + TRACK_HEADER_WIDTH;

				Rectangle barRect(x, bounds.Y + TRACK_PADDING, TicksToPixels(bar->Duration), bounds.Height - (TRACK_PADDING * 2));

				// Draw selection highlight
				Color highlightColor = Color::FromArgb(100, 255, 0, 0); // Light red
				g->FillRectangle(gcnew SolidBrush(highlightColor), barRect);

				// If hovering over any selected bar, show delete preview for all selected bars
				if (isHoveringSelected)
				{
					Color previewColor = Color::FromArgb(180, 255, 0, 0); // Semi-transparent red
					Pen^ previewPen = gcnew Pen(previewColor, 2);

					// Draw crossed-out effect
					g->DrawRectangle(previewPen, barRect);
					g->DrawLine(previewPen, barRect.Left, barRect.Top, barRect.Right, barRect.Bottom);
					g->DrawLine(previewPen, barRect.Left, barRect.Bottom, barRect.Right, barRect.Top);

					delete previewPen;
				}
			}
		}
	}

	void Widget_Timeline::DrawToolPreviewDurationTool(Graphics^ g)
	{
		DurationTool^ durationTool = GetDurationTool();

		// Draw selection rectangle if active
		Rectangle selRect = durationTool->SelectionRect;
		if (selRect.Width > 0 && selRect.Height > 0)
		{
			Color selectionColor = Color::FromArgb(50, currentTheme.SelectionHighlight);

			g->FillRectangle(gcnew SolidBrush(selectionColor), selRect);
			g->DrawRectangle(gcnew Pen(currentTheme.SelectionHighlight), selRect);
		}
	}

	void Widget_Timeline::DrawToolPreviewColorTool(Graphics^ g)
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
		for each (BarEvent ^ bar in colorTool->SelectedBars)
		{
			Track^ track = nullptr;
			// Find the track containing this bar
			for each (Track ^ t in Tracks) {
				if (t->Events->Contains(bar)) {
					track = t;
					break;
				}
			}

			if (track != nullptr)
			{
				Rectangle bounds = GetTrackContentBounds(track);
				int x = TicksToPixels(bar->StartTick) + ScrollPosition->X + TRACK_HEADER_WIDTH;

				Rectangle barRect(x, bounds.Y + TRACK_PADDING, TicksToPixels(bar->Duration), bounds.Height - (TRACK_PADDING * 2));

				// Draw selection highlight
				Color highlightColor = Color::FromArgb(100, colorTool->CurrentColor);
				g->FillRectangle(gcnew SolidBrush(highlightColor), barRect);

				// Draw preview outline
				Pen^ previewPen = gcnew Pen(colorTool->CurrentColor, 2);
				g->DrawRectangle(previewPen, barRect);
				delete previewPen;

				// If this bar is being hovered over, show stronger highlight
				if (bar == colorTool->HoverBar)
				{
					DrawBarGlowEffect(g, barRect, colorTool->CurrentColor, 2);
				}
			}
		}

		// Draw preview rectangle for hovered bar
		Rectangle previewRect = colorTool->PreviewRect;

		if (!previewRect.IsEmpty && !colorTool->SelectedBars->Contains(colorTool->HoverBar))
		{
			Color previewColor = Color::FromArgb(80, colorTool->CurrentColor);
			g->FillRectangle(gcnew SolidBrush(previewColor), previewRect);
			g->DrawRectangle(gcnew Pen(colorTool->CurrentColor), previewRect);
		}
	}

	void Widget_Timeline::DrawToolPreviewFadeTool(Graphics^ g)
	{
		FadeTool^ Fade_Tool = (FadeTool^)currentTool;
		
		Track^				Target_Track	= Fade_Tool->TargetTrack;
		List<BarEvent^>^	Preview_Bars	= Fade_Tool->PreviewBars;
		BarEvent^			Single_Preview	= Fade_Tool->PreviewBar;
		
		// Save current clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create clipping region for content area
		Rectangle contentArea(
			Widget_Timeline::TRACK_HEADER_WIDTH,
			Widget_Timeline::HEADER_HEIGHT,
			Width - Widget_Timeline::TRACK_HEADER_WIDTH,
			Height - Widget_Timeline::HEADER_HEIGHT
		);
		g->SetClip(contentArea);

		if (Preview_Bars != nullptr && Preview_Bars->Count > 0)
		{
			DrawPreviewBarList(g, Preview_Bars, Target_Track);
		}
		else if (Single_Preview != nullptr)
		{
			DrawPreviewBar(g, Single_Preview, nullptr, Fade_Tool->CurrentMousePosition, BarPreviewType::Creation);
		}

		// Restore original clip region
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawToolPreviewStrobeTool(Graphics^ g)
	{
		StrobeTool^ Strobe_Tool = (StrobeTool^)currentTool;
		
		Track^				Target_Track	= Strobe_Tool->TargetTrack;
		List<BarEvent^>^	Preview_Bars	= Strobe_Tool->PreviewBars;
		BarEvent^			Single_Preview	= Strobe_Tool->PreviewBar;

		// Save current clip region
		System::Drawing::Region^ originalClip = g->Clip;

		// Create clipping region for content area
		Rectangle contentArea(
			Widget_Timeline::TRACK_HEADER_WIDTH,
			Widget_Timeline::HEADER_HEIGHT,
			Width - Widget_Timeline::TRACK_HEADER_WIDTH,
			Height - Widget_Timeline::HEADER_HEIGHT
		);
		g->SetClip(contentArea);

		if (Preview_Bars != nullptr && Preview_Bars->Count > 0)
		{
			DrawPreviewBarList(g, Preview_Bars, Target_Track);
		}
		else if (Single_Preview != nullptr)
		{
			DrawPreviewBar(g, Single_Preview, nullptr, Strobe_Tool->CurrentMousePosition, BarPreviewType::Creation);
		}

		// Restore original clip region
		g->Clip = originalClip;
	}

	void Widget_Timeline::DrawPreviewBar(Graphics^ g, BarEvent^ bar, Track^ track, Point mousePos, BarPreviewType previewType)
	{
		// For Creation/Movement previews, we need to determine the track
		Track^ targetTrack = track == nullptr ? GetTrackAtPoint(mousePos) : track;

		if (bar != nullptr && targetTrack != nullptr)
		{
			// Get the track bounds for drawing
			Rectangle bounds = GetTrackContentBounds(targetTrack);
			bounds.Y += scrollPosition->Y;

			Rectangle BarBounds = CalculateBarBounds(bar, bounds);

			// Different visual treatments based on preview type
			switch (previewType)
			{
			case BarPreviewType::Creation:
			case BarPreviewType::Movement:
				DrawCreationMovementPreview(g, bar, BarBounds);
				break;

			case BarPreviewType::Duration:
				DrawDurationPreview(g, bar, BarBounds);
				break;
			}
		}
		
		
		/*
		Track^ hoverTrack = GetTrackAtPoint(mousePos);

		if (bar != nullptr && hoverTrack != nullptr)
		{
			// Get the track bounds for drawing
			Rectangle bounds = GetTrackContentBounds(hoverTrack);
			bounds.Y += scrollPosition->Y;

			int x, width;
			Rectangle barBounds;
			CalculateBarBounds(bar, bounds, x, width, barBounds);

			// Draw semi-transparent preview
			Color previewColor = bar->Color;
			g->FillRectangle(gcnew SolidBrush(previewColor), barBounds);
			g->DrawRectangle(gcnew Pen(Color::FromArgb(180, previewColor)), barBounds);
		}
		*/
	}

	void Widget_Timeline::DrawPreviewBarList(Graphics^ g, List<BarEvent^>^ bar_list, Track^ track)
	{
		if (bar_list == nullptr || track == nullptr) { return; }

		for each (BarEvent ^ bar in bar_list)
		{
			// Get track bounds
			Rectangle bounds = GetTrackContentBounds(track);
			bounds.Y += scrollPosition->Y;

			DrawPreviewBar(g, bar, track, Point(), BarPreviewType::Creation);
		}
	}

	void Widget_Timeline::DrawCreationMovementPreview(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		// Semi-transparent preview with border
		Color previewColor = bar->Color;
		g->FillRectangle(gcnew SolidBrush(Color::FromArgb(180, previewColor)), bounds);
		g->DrawRectangle(gcnew Pen(Color::FromArgb(180, previewColor)), bounds);

		// Add subtle glow effect
		DrawBarGlowEffect(g, bounds, Color::FromArgb(100, previewColor), 2);
	}

	void Widget_Timeline::DrawDurationPreview(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		// Draw original bar with reduced opacity
		Rectangle originalBounds = bounds;
		originalBounds.Width = TicksToPixels(bar->Duration);
		g->FillRectangle(gcnew SolidBrush(Color::FromArgb(80, bar->Color)), originalBounds);

		// Draw preview with enhanced brightness
		Color previewColor = Color::FromArgb(
			Math::Min(255, bar->Color.R + 40),
			Math::Min(255, bar->Color.G + 40),
			Math::Min(255, bar->Color.B + 40)
		);
		g->FillRectangle(gcnew SolidBrush(previewColor), bounds);

		// Add glow effect for the preview
		DrawBarGlowEffect(g, bounds, currentTheme.SelectionHighlight, 3);
	}


	void Widget_Timeline::DrawNormalBar(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		Rectangle BarBounds = CalculateBarBounds(bar, bounds);
		
		bool isDurationTarget = false;

		// Check for duration preview
		if (currentToolType == TimelineToolType::Duration)
		{
			DurationTool^ durationTool = GetDurationTool();
			isDurationTarget = (bar == durationTool->PreviewBar);

			if (isDurationTarget && durationTool->IsPreviewVisible)
			{
				DrawPreviewBar(g, bar, nullptr, Point(), BarPreviewType::Duration);
				return;  // Important: early return if preview was drawn
			}
		}

		// Draw normal bar appearance
		if (isDurationTarget)
		{
			// Highlight the bar being targeted for duration change
			Color highlightColor = Color::FromArgb(
				Math::Min(255, bar->Color.R + 30),
				Math::Min(255, bar->Color.G + 30),
				Math::Min(255, bar->Color.B + 30)
			);
			g->FillRectangle(gcnew SolidBrush(highlightColor), BarBounds);

			Pen^ highlightPen = gcnew Pen(currentTheme.SelectionHighlight, 2);
			g->DrawRectangle(highlightPen, BarBounds);
			delete highlightPen;
		}
		else
		{
			// Normal appearance
			g->FillRectangle(gcnew SolidBrush(Color::FromArgb(200, bar->Color)), BarBounds);
			g->DrawRectangle(gcnew Pen(Color::FromArgb(100, 0, 0, 0)), BarBounds);
		}

		// Add tool-specific enhancements
		if (currentToolType == TimelineToolType::Draw)
		{
			DrawTool^ drawTool = GetDrawTool();
			if (drawTool->CurrentMode == DrawToolMode::Resize && bar == drawTool->HoverBar)
			{
				DrawResizeHandle(g, BarBounds, currentTheme.SelectionHighlight, false);
			}
		}
		else if (currentToolType == TimelineToolType::Duration)
		{
			// Create handle color based on target state
			Color handleColor = isDurationTarget ?
				currentTheme.SelectionHighlight :
				Color::FromArgb(bar->Color.R * 8 / 10, bar->Color.G * 8 / 10, bar->Color.B * 8 / 10);

			DrawResizeHandle(g, BarBounds, handleColor, isDurationTarget);
		}
	}

	void Widget_Timeline::DrawGhostBar(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		int x = TicksToPixels(bar->OriginalStartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
		int width = TicksToPixels(bar->Duration);

		// Ensure that every bar is drawn with at least one pixel width, independent from the zoom level
		if (width < 1) {
			width = 1;
		}

		Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

		// Draw with high transparency
		g->FillRectangle(gcnew SolidBrush(Color::FromArgb(80, bar->Color)), barBounds);
	}

	void Widget_Timeline::DrawSelectedBar(Graphics^ g, BarEvent^ bar, Rectangle bounds)
	{
		Rectangle BarBounds = CalculateBarBounds(bar, bounds);

		// Check for duration preview
		bool isDurationTarget	= false;
		bool isDurationPreview	= false;

		if (currentToolType == TimelineToolType::Duration)
		{
			DurationTool^ durationTool = GetDurationTool();
			isDurationTarget = (bar == durationTool->PreviewBar);

			if (isDurationTarget && durationTool->IsPreviewVisible)
			{
				int width = TicksToPixels(durationTool->PreviewLength);
				isDurationPreview = true;
				BarBounds.Width = width;
			}
		}

		if (isDurationPreview)
		{
			DrawDurationPreview(g, bar, BarBounds);
		}
		else
		{
			// Draw bar with enhanced brightness
			Color fillColor = isDurationTarget ?
				Color::FromArgb(
					Math::Min(255, bar->Color.R + 40),
					Math::Min(255, bar->Color.G + 40),
					Math::Min(255, bar->Color.B + 40)
				) :
				Color::FromArgb(255, bar->Color);

			g->FillRectangle(gcnew SolidBrush(fillColor), BarBounds);
			g->DrawRectangle(gcnew Pen(Color::FromArgb(100, 0, 0, 0)), BarBounds);

			DrawBarGlowEffect(g, BarBounds, currentTheme.SelectionHighlight, isDurationTarget ? 3 : 2);

			// Add duration tool handle if needed
			if (currentToolType == TimelineToolType::Duration)
			{
				Color handleColor = isDurationTarget ?
					currentTheme.SelectionHighlight :
					Color::FromArgb(
						Math::Min(255, bar->Color.R * 13 / 10),
						Math::Min(255, bar->Color.G * 13 / 10),
						Math::Min(255, bar->Color.B * 13 / 10)
					);

				DrawResizeHandle(g, BarBounds, handleColor, isDurationTarget);
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

	void Widget_Timeline::DrawBarGlowEffect(Graphics^ g, Rectangle barBounds, Color glowColor, int glowLevels)
	{
		for (int i = glowLevels; i >= 0; i--) {
			Rectangle glowBounds = barBounds;
			glowBounds.Inflate(i, i);

			Pen^ glowPen = gcnew Pen(Color::FromArgb(80 - i * 15, glowColor), 1);
			g->DrawRectangle(glowPen, glowBounds);
			delete glowPen;
		}
	}

	void Widget_Timeline::DrawResizeHandle(Graphics^ g, Rectangle barBounds, Color handleColor, bool isTargeted)
	{
		Rectangle handleBounds(barBounds.Right - 5, barBounds.Y, 5, barBounds.Height);
		g->FillRectangle(gcnew SolidBrush(handleColor), handleBounds);

		// Add grip lines
		Color gripColor = isTargeted ? Color::White : Color::FromArgb(200, 255, 255, 255);
		Pen^ gripPen = gcnew Pen(gripColor, isTargeted ? 2.0f : 1.0f);

		for (int i = 0; i < 3; i++) {
			g->DrawLine(gripPen,
				handleBounds.X + i + 1,
				handleBounds.Y + 2,
				handleBounds.X + i + 1,
				handleBounds.Bottom - 2);
		}
		delete gripPen;

		if (isTargeted) {
			DrawBarGlowEffect(g, handleBounds, currentTheme.SelectionHighlight, 3);
		}
	}

	void Widget_Timeline::DrawTrackHeaders(Graphics^ g)
	{
		if (tracks->Count == 0) return;

		// Fill header background
		Rectangle headerBackground(0, HEADER_HEIGHT, TRACK_HEADER_WIDTH, Height - HEADER_HEIGHT);
		g->FillRectangle(resourceManager->GetBrush(currentTheme.HeaderBackground), headerBackground);

		for each (Track ^ track in tracks)
		{
			Rectangle trackBounds = GetTrackBounds(track);
			Rectangle headerBounds(0, trackBounds.Y, TRACK_HEADER_WIDTH, trackBounds.Height);

			headerBounds.Y += scrollPosition->Y;

			// Draw header background
			Color headerBg = track->IsSelected ? currentTheme.SelectionHighlight : currentTheme.HeaderBackground;
			g->FillRectangle(resourceManager->GetBrush(headerBg), headerBounds);

			DrawTrackName(g, track, headerBounds);
			DrawTrackButtons(g, track, headerBounds);

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
		// This is already done by the DrawTrackBackground method directly called by OnPaint

		// 2. Draw grid lines (these should be visible through track content)
		DrawGridLines(g);

		// 3. Draw track content (events and tablature)
		for each (Track ^ track in tracks)
		{
			int trackTop	= GetTrackTop(track) + scrollPosition->Y;
			int trackHeight = track->Height;
			int trackBottom = trackTop + trackHeight;

			if (trackBottom < HEADER_HEIGHT || trackTop > Height) continue;

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

		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		g->TextRenderingHint = System::Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		Rectangle tracksArea(0, HEADER_HEIGHT, Width, Height - HEADER_HEIGHT);
		g->FillRectangle(gcnew SolidBrush(currentTheme.TrackBackground), tracksArea);
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
			int headerBottom = y + track->Height;
			if (headerBottom >= scrollPosition->Y && y <= scrollPosition->Y + this->Height) {
				g->DrawLine(dividerPen, TRACK_HEADER_WIDTH, y, TRACK_HEADER_WIDTH, headerBottom);
			}

			y += track->Height;
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
			format->Alignment = StringAlignment::Near;
			format->LineAlignment = StringAlignment::Center;
			format->Trimming = StringTrimming::EllipsisCharacter;

			g->DrawString(track->Name, measureFont, resourceManager->GetBrush(currentTheme.Text), textBounds, format);

			delete format;
		}
	}

	void Widget_Timeline::DrawTrackButtons(Graphics^ g, Track^ track, Rectangle headerBounds)
	{
		DrawTrackButtonText(g, headerBounds, 0, "T",
			track->ShowTablature,
			(track == hoveredButton.Track && hoveredButton.ButtonIndex == 0),
			currentTheme.HeaderBackground,
			currentTheme.Text);

		if (track->IsDrumTrack) {
			DrawTrackButtonIcon(g, headerBounds, 1, (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Note_White"))),  // Index 1 for second button
				track->ShowAsStandardNotation,
				(track == hoveredButton.Track && hoveredButton.ButtonIndex == 1),
				currentTheme.HeaderBackground,
				currentTheme.Text);
		}
	}

	void Widget_Timeline::DrawTrackButtonText(Graphics^ g, Rectangle headerBounds, int buttonIndex, String^ text, bool isPressed, bool isHovered, Color baseColor, Color textColor)
	{
		// Calculate button position based on index (right to left)
		Rectangle buttonBounds = GetTrackButtonBounds(headerBounds, buttonIndex);

		// Define colors for different states
		Color buttonColor;

		if (isHovered) {
			// Slightly lighter for hover
			buttonColor = Color::FromArgb(
				Math::Min(255, (int)baseColor.R + 20),
				Math::Min(255, (int)baseColor.G + 20),
				Math::Min(255, (int)baseColor.B + 20)
			);
		}
		else if (isPressed) {
			// Darker, solid color for active state
			buttonColor = Color::FromArgb(
				Math::Min(255, (int)baseColor.R + 40),
				Math::Min(255, (int)baseColor.G + 40),
				Math::Min(255, (int)baseColor.B + 40)
			);
		}
		else {
			// Normal state
			buttonColor = baseColor;
			textColor = Color::FromArgb(
				Math::Max(0, textColor.R - 40),
				Math::Max(0, textColor.G - 40),
				Math::Max(0, textColor.B - 40)
			);
		}

		// Draw button with rounded corners
		Drawing2D::GraphicsPath^ buttonPath = gcnew Drawing2D::GraphicsPath();
		int radius = 6;  // Rounded corner radius
		buttonPath->AddArc(buttonBounds.X, buttonBounds.Y, radius * 2, radius * 2, 180, 90);
		buttonPath->AddArc(buttonBounds.Right - (radius * 2), buttonBounds.Y, radius * 2, radius * 2, 270, 90);
		buttonPath->AddArc(buttonBounds.Right - (radius * 2), buttonBounds.Bottom - (radius * 2), radius * 2, radius * 2, 0, 90);
		buttonPath->AddArc(buttonBounds.X, buttonBounds.Bottom - (radius * 2), radius * 2, radius * 2, 90, 90);
		buttonPath->CloseFigure();

		// Draw button background
		g->FillPath(gcnew SolidBrush(buttonColor), buttonPath);

		// Draw border
		Color borderColor = Color::FromArgb(60, currentTheme.Text);
		g->DrawPath(gcnew Pen(borderColor, 1.0f), buttonPath);

		// Draw text
		Drawing::Font^ buttonFont = gcnew Drawing::Font("Segoe UI", 14, FontStyle::Bold);
		SizeF textSize = g->MeasureString(text, buttonFont);

		// Center text
		float textX = (float)(buttonBounds.X + 1 + (buttonBounds.Width - textSize.Width) / 2);
		float textY = (float)(buttonBounds.Y + 1 + (buttonBounds.Height - textSize.Height) / 2);

		g->DrawString(text, buttonFont, gcnew SolidBrush(textColor), textX, textY);

		delete buttonFont;
		delete buttonPath;
	}

	void Widget_Timeline::DrawTrackButtonIcon(Graphics^ g, Rectangle headerBounds, int buttonIndex, Image^ icon, bool isPressed, bool isHovered, Color baseColor, Color textColor)
	{
		// First draw the button background using existing method with empty text
		DrawTrackButtonText(g, headerBounds, buttonIndex, "", isPressed, isHovered, baseColor, textColor);

		// Get the button bounds for the icon
		Rectangle buttonBounds = GetTrackButtonBounds(headerBounds, buttonIndex);

		// Draw the icon with padding
		if (icon != nullptr) {
			int padding = 4;
			Rectangle iconBounds = Rectangle(
				buttonBounds.X + padding,
				buttonBounds.Y + padding,
				buttonBounds.Width - (padding * 2),
				buttonBounds.Height - (padding * 2)
			);

			// Draw the image with color transformation if needed
			System::Drawing::Imaging::ColorMatrix^ colorMatrix = nullptr;
			if (textColor != Color::White) {  // If we need to recolor the icon
				colorMatrix = gcnew System::Drawing::Imaging::ColorMatrix(
					gcnew array<array<float>^>{
					gcnew array<float>{textColor.R / 255.0f, 0, 0, 0, 0},
					gcnew array<float>{0, textColor.G / 255.0f, 0, 0, 0},
					gcnew array<float>{0, 0, textColor.B / 255.0f, 0, 0},
					gcnew array<float>{0, 0, 0, textColor.A / 255.0f, 0},
					gcnew array<float>{0, 0, 0, 0, 1.0f}
				}
				);
			}

			if (colorMatrix != nullptr)
			{
				System::Drawing::Imaging::ImageAttributes^ attributes = gcnew System::Drawing::Imaging::ImageAttributes();
				attributes->SetColorMatrix(colorMatrix);
				g->DrawImage(icon, iconBounds, 0, 0, icon->Width, icon->Height, GraphicsUnit::Pixel, attributes);
				delete attributes;
				delete colorMatrix;
			}
			else {
				g->DrawImage(icon, iconBounds);
			}
		}
	}

	void Widget_Timeline::DrawTrackBorders(Graphics^ g, Track^ track, Rectangle bounds)
	{
		Pen^ borderPen = gcnew Pen(currentTheme.TrackBorder);

		// Draw outer rectangle
		g->DrawRectangle(borderPen, bounds);

		// Draw vertical separator between header and content
		g->DrawLine(borderPen, TRACK_HEADER_WIDTH, bounds.Y, TRACK_HEADER_WIDTH, bounds.Bottom);

		// Draw resize handle if this is not the last track
		if (track != tracks[tracks->Count - 1]) {
			int handleY = bounds.Bottom - TRACK_RESIZE_HANDLE_HEIGHT;
			g->DrawLine(borderPen, 0, handleY, Width, handleY);
		}

		delete borderPen;
	}

	void Widget_Timeline::DrawGridLines(Graphics^ g)
	{
		// Calculate total height for grid lines
		int totalHeight = GetTotalTracksHeight();
		Rectangle contentRect(TRACK_HEADER_WIDTH, HEADER_HEIGHT, Width - TRACK_HEADER_WIDTH, totalHeight);

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
		DrawToolPreview(g);
	}

	void Widget_Timeline::DrawTrackEvents(Graphics^ g, Track^ track, Rectangle bounds)
	{
		int startTick	= PixelsToTicks(-scrollPosition->X);
		int endTick		= PixelsToTicks(-scrollPosition->X + Width - TRACK_HEADER_WIDTH);

		PointerTool^	pointerTool		= GetPointerTool();
		DrawTool^		drawTool		= GetDrawTool();
		DurationTool^	durationTool	= GetDurationTool();

		bool isDragging				= pointerTool->IsDragging;
		bool isMultiTrackSelection	= pointerTool->IsMultiTrackSelection;
		bool isDrawToolMoving		= (currentToolType == TimelineToolType::Draw && drawTool->CurrentMode == DrawToolMode::Move && drawTool->IsMoving);

		// STEP 1: Draw non-selected bars
		for each (BarEvent ^ bar in track->Events)
		{
			if ((bar->StartTick + bar->Duration < startTick) || (bar->StartTick > endTick)) {
				continue;
			}

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
		if (isDragging && currentToolType == TimelineToolType::Pointer)
		{
			// For multi-track selection, show ghosts in original positions
			if (isMultiTrackSelection)
			{
				for each (BarEvent ^ bar in track->Events)
				{
					if (pointerTool->SelectedBars->Contains(bar)) {
						DrawGhostBar(g, bar, bounds);
					}
				}
			}
			// For single-track selection, only show ghosts in source track
			else if (track == pointerTool->DragSourceTrack)
			{
				for each (BarEvent ^ bar in pointerTool->SelectedBars) {
					DrawGhostBar(g, bar, bounds);
				}
			}
		}
		else if (isDrawToolMoving && drawTool->HoverBar != nullptr)
		{
			if (track == drawTool->SourceTrack) {
				DrawGhostBar(g, drawTool->HoverBar, bounds);
			}
		}

		// STEP 3: Draw selected/dragged bars
		if (currentToolType == TimelineToolType::Pointer && isDragging)
		{
			Point mousePos = pointerTool->CurrentMousePosition;
			bool isOverHeader = mousePos.X <= TRACK_HEADER_WIDTH;

			if (!isOverHeader)	// Only show preview if not over header
			{  
				if (isMultiTrackSelection)
				{
					// For multi-track selection, show dragged bars in their original tracks
					for each (BarEvent ^ bar in track->Events)
					{
						if (bar->StartTick + bar->Duration < startTick || bar->StartTick > endTick) {
							continue;
						}

						if (pointerTool->SelectedBars->Contains(bar)) {
							DrawSelectedBar(g, bar, bounds);
						}
					}
				}
				else if (track == pointerTool->DragTargetTrack) {
					// For single-track selection, show all dragged bars in target track
					for each (BarEvent ^ bar in pointerTool->SelectedBars) {
						DrawSelectedBar(g, bar, bounds);
					}
				}
			}
		}
		else if (isDrawToolMoving && drawTool->HoverBar != nullptr)
		{
			if (track == drawTool->TargetTrack) {
				DrawSelectedBar(g, drawTool->HoverBar, bounds);
			}
		}
		else
		{
			// When not dragging, draw selected bars in their current tracks
			for each (BarEvent ^ bar in track->Events)
			{
				if (bar->StartTick + bar->Duration < startTick || bar->StartTick > endTick) {
					continue;
				}

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
		if (pointerTool->IsPasting && pointerTool->PastePreviewBars != nullptr)
		{
			if (track != nullptr && pointerTool->CurrentMousePosition.X > TRACK_HEADER_WIDTH)
			{
				int currentTrackIndex = Tracks->IndexOf(track);

				// Draw preview bars that belong to this track
				for each (BarEvent ^ previewBar in pointerTool->PastePreviewBars)
				{
					// Check if this preview bar belongs to the current track
					// We stored the target track index in OriginalStartTick
					if (previewBar->OriginalStartTick == currentTrackIndex)
					{
						int x = TicksToPixels(previewBar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
						int width = TicksToPixels(previewBar->Duration);

						Rectangle barBounds(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);

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
						DrawBarGlowEffect(g, barBounds, Color::FromArgb(180, 255, 255, 255), 2);
						
						// Draw dashed outline
						Pen^ dashPen = gcnew Pen(Color::White, 1);
						array<float>^ dashPattern = { 4.0f, 4.0f };
						dashPen->DashPattern = dashPattern;
						g->DrawRectangle(dashPen, barBounds);
						delete dashPen;
					}
				}

				// Draw drop target indicator if this is the anchor track
				if (track == pointerTool->DragTargetTrack)
				{
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

		float logScale = (float)Math::Log(zoomLevel + 1, 2);

		if (track->IsDrumTrack && track->ShowAsStandardNotation)
		{
			DrawTrackTablatureDrum(g, track, bounds, logScale);
		}
		else
		{
			DrawTrackTablatureRegular(g, track, bounds, logScale);
		}
	}

	void Widget_Timeline::DrawTrackTablatureDrum(Graphics^ g, Track^ track, Rectangle bounds, float logScale)
	{
		float availableHeight = (float)(bounds.Height - TRACK_PADDING * 2);
		
		TabStringInfo String_Info = DrawTablatureStrings(g, bounds, availableHeight, logScale, 5);

		if (String_Info.TotalHeight > availableHeight) {
			return;
		}

		try
		{
			// Calculate visible range
			int visibleStartTick = PixelsToTicks(-scrollPosition->X);
			int visibleEndTick = PixelsToTicks(-scrollPosition->X + bounds.Width);

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

				for each(Beat ^ beat in measure->Beats)
				{
					if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0) {
						continue;
					}

					if ((beat->Duration > 0) && (beat->Notes->Count > 0) && availableHeight > String_Info.TotalHeight + requiredSpace) {
						DrawBeatDuration(g, beat, bounds, String_Info.StringYPositions);
					}

					float xPos = (float)(TicksToPixels(beat->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH);

					for each(Note ^ note in beat->Notes)
					{
						DrumNoteInfo noteInfo = DrumNotationMap::GetNoteInfo(note->Value);

						// Calculate Y position (can be between lines)
						float yPos;
						int lineIndex = (int)Math::Floor(noteInfo.StringPosition);
						float fraction = noteInfo.StringPosition - lineIndex ;

						lineIndex -=1;

						if (fraction == 0.0f) { // On the line
							yPos = String_Info.StringYPositions[lineIndex];
						}
						else if (lineIndex < 0) {
							yPos = String_Info.StringYPositions[0] - (String_Info.StringYPositions[1] - String_Info.StringYPositions[0]) * fraction;
						}
						else { // Between lines
							yPos = String_Info.StringYPositions[lineIndex] + (String_Info.StringYPositions[lineIndex + 1] - String_Info.StringYPositions[lineIndex]) * fraction;
						}

						DrawDrumSymbol(g, noteInfo.SymbolType, xPos, yPos, ((String_Info.StringYPositions[1] - String_Info.StringYPositions[0]) / 2.0f) - 1);
					}
				}
				measureStartTick += measure->Length;
			}
		}
		finally {

		}
	}

	void Widget_Timeline::DrawTrackTablatureRegular(Graphics^ g, Track^ track, Rectangle bounds, float logScale)
	{
		//////////////////////////////////////////
		// Calculate zoom-based scaling factors //
		//////////////////////////////////////////
		const float BASE_FONT_SIZE = 6.0f;
		const float FONT_SCALE_FACTOR = 2.0f;
		float scaledFontSize = BASE_FONT_SIZE + (logScale * FONT_SCALE_FACTOR);

		// Clamp font size between min and max values
		scaledFontSize = Math::Min(Math::Max(scaledFontSize, 4.0f), 18.0f);

		float availableHeight = (float)(bounds.Height - TRACK_PADDING * 2);

		TabStringInfo String_Info = DrawTablatureStrings(g, bounds, availableHeight, logScale, 6);

		if (String_Info.TotalHeight > availableHeight) {
			return;
		}

		float fontSize = Math::Min(FIXED_STRING_SPACING * 0.7f, 10.0f);
		if (fontSize < 4.0f) return;	// Too small to be readable

		try
		{
			Drawing::Font^ scaledFont = gcnew Drawing::Font("Arial", scaledFontSize);

			// Calculate visible tick range
			int visibleStartTick = PixelsToTicks(-scrollPosition->X);
			int visibleEndTick = PixelsToTicks(-scrollPosition->X + bounds.Width);

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
				for each(Beat ^ beat in measure->Beats)
				{
					if (beat == nullptr || beat->Notes == nullptr || beat->Notes->Count == 0) {
						continue;
					}

					int beatTick = beat->StartTick;

					// Skip if beat is outside visible range
					if (beatTick > visibleEndTick || beatTick + beat->Duration < visibleStartTick) {
						continue;
					}

					float beatX = (float)(TicksToPixels(beatTick) + scrollPosition->X + TRACK_HEADER_WIDTH);

					const float BASE_DURATION_SPACE = 23.0f;
					const float DURATION_SCALE_FACTOR = 20.0f;
					float requiredSpace = BASE_DURATION_SPACE + (DURATION_SCALE_FACTOR * logScale);

					// Draw duration lines for beats with multiple notes
					if ((beat->Duration > 0) && (beat->Notes->Count > 0) && availableHeight > String_Info.TotalHeight + requiredSpace)
					{
						DrawBeatDuration(g, beat, bounds, String_Info.StringYPositions);
					}

					// Draw the notes
					for each(Note ^ note in beat->Notes)
					{
						if (note == nullptr || note->String < 1 || note->String > 6)
							continue;

						String^ fretText = note->Value.ToString();
						float textWidth = g->MeasureString(fretText, scaledFont).Width;
						float textX = beatX - (textWidth / 2.0f);
						float textY = String_Info.StringYPositions[note->String - 1] - (scaledFont->Height / 2.0f);

						// Draw background for better readability
						RectangleF bgRect(textX - 1, textY, textWidth + 2, (float)(scaledFont->Height - 1));
						g->FillRectangle(gcnew SolidBrush(Color::FromArgb(220, currentTheme.TrackBackground)), bgRect);

						// Draw the fret number with scaled font
						g->DrawString(fretText, scaledFont, cachedTextBrush, textX, textY);
					}
				}

				measureStartTick = measureEndTick;
			}

			DrawTieLines(g, track, bounds, String_Info.StringYPositions, fontSize);

			delete scaledFont;
		}
		finally {

		}
	}

	void Widget_Timeline::DrawBeatDuration(Graphics^ g, Beat^ beat, Rectangle bounds, array<float>^ stringYPositions)
	{
		// Skip if no notes or no duration
		if (beat->Notes == nullptr || beat->Notes->Count == 0 || beat->Duration <= 0)
			return;

		// Calculate logarithmic scaling factor
		float logScale = (float)Math::Log(zoomLevel + 1, 2);

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
		float noteX = (float)(TicksToPixels(beat->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH);

		// Position stem below the bottom string with scaled offset
		float stemY = bottomStringY + scaledStemOffset;
		float stemEndY = stemY + scaledStemLength;

		// Calculate stem position - centered below the note
		float stemX = noteX;

		// Create pen with scaled thickness
		Pen^ stemPen = gcnew Pen(Color::FromArgb(180, currentTheme.Text), scaledLineThickness);

		// Calculate number of lines based on duration
		int duration = beat->Duration;
		int numLines = 0;

		if ((duration >= TICKS_PER_QUARTER * 2) && (duration < TICKS_PER_QUARTER * 4)) // Half note
		{
			// Draw only bottom half of stem
			float halfStemLength = scaledStemLength / 2;
			g->DrawLine(stemPen, stemX, stemY + halfStemLength, stemX, stemEndY);
		}
		else if (duration < TICKS_PER_QUARTER * 2) // Quarter note or shorter
		{
			// Draw full stem
			g->DrawLine(stemPen, stemX, stemY, stemX, stemEndY);


			if (duration <= TICKS_PER_QUARTER / 8)	numLines = 3; // 32nd note
			else if (duration <= TICKS_PER_QUARTER / 4)	numLines = 2; // 16th note
			else if (duration <= TICKS_PER_QUARTER / 2)	numLines = 1; // 8th note
			else numLines = 0; // Quarter note or longer

			// Draw horizontal lines to the right of the stem
			if (numLines > 0)
			{
				for (int i = 0; i < numLines; i++)
				{
					float lineY = stemEndY - (i * scaledLineSpacing);
					float lineStartX = stemX;
					float lineEndX = stemX + scaledLineLength;

					g->DrawLine(stemPen, lineStartX, lineY, lineEndX, lineY);
				}
			}
		}

		if (beat->IsDotted)
		{
			float dotSize = Math::Min(2.0f + (logScale * 0.5f), 4.0f);
			float dotX = stemX + scaledLineLength + (dotSize * 2) - 10;
			float dotY = stemEndY - (numLines * scaledLineSpacing);

			g->FillEllipse(gcnew SolidBrush(Color::FromArgb(180, currentTheme.Text)), dotX - dotSize, dotY - dotSize, dotSize * 2, dotSize * 2);
		}
		// Draw triplet indicator (small "3")
		else if (duration * 3 / 2 == TICKS_PER_QUARTER ||	// Triplet quarter
			duration * 3 / 2 == TICKS_PER_QUARTER / 2 ||	// Triplet eighth
			duration * 3 / 2 == TICKS_PER_QUARTER / 4)		// Triplet sixteenth
		{
			// Calculate font size based on zoom level
			float tripletFontSize = Math::Min(4.0f + (logScale * 1.0f), 12.0f);
			Drawing::Font^ tripletFont = gcnew Drawing::Font("Arial", tripletFontSize);

			// Position the "3" above the duration lines
			float textX = stemX + scaledLineLength - 10;
			float textY = stemEndY - (numLines * scaledLineSpacing) - tripletFont->Height;

			// Draw the "3"
			g->DrawString("3", tripletFont, gcnew SolidBrush(Color::FromArgb(180, currentTheme.Text)), textX, textY);

			delete tripletFont;
		}

		delete stemPen;
	}

	void Widget_Timeline::DrawTieLines(Graphics^ g, Track^ track, Rectangle bounds, array<float>^ stringYPositions, float scaledFontSize)
	{
		if (!track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0)
			return;

		// Calculate visible range
		int visibleStartTick = PixelsToTicks(-scrollPosition->X);
		int visibleEndTick = PixelsToTicks(-scrollPosition->X + bounds.Width);

		// Track measure position
		int measureStartTick = 0;

		// Create pen for tie lines with transparency
		Pen^ tiePen = gcnew Pen(Color::FromArgb(180, currentTheme.Text), 1.5f);
		tiePen->StartCap = Drawing2D::LineCap::Round;
		tiePen->EndCap = Drawing2D::LineCap::Round;

		// Create font for measuring note text dimensions
		Drawing::Font^ noteFont = gcnew Drawing::Font("Arial", scaledFontSize);

		try {
			// For each measure, find tied notes and draw connections
			for (int i = 0; i < track->Measures->Count; i++) {
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
				for each (Beat ^ currentBeat in measure->Beats) {
					if (currentBeat == nullptr || currentBeat->Notes == nullptr)
						continue;

					// For each tied note in current beat, find its previous note
					for each (Note ^ currentNote in currentBeat->Notes) {
						if (!currentNote->IsTied)
							continue;

						// Find the previous note with the same string and value
						Beat^ previousBeat = nullptr;
						Note^ previousNote = nullptr;

						// Search in current measure first
						for each (Beat ^ checkBeat in measure->Beats) {
							if (checkBeat->StartTick >= currentBeat->StartTick)
								break;

							// Look for matching note
							for each (Note ^ checkNote in checkBeat->Notes) {
								if (checkNote->String == currentNote->String &&
									checkNote->Value == currentNote->Value) {
									previousBeat = checkBeat;
									previousNote = checkNote;
								}
							}
						}

						// If not found and we're not in first measure, check previous measure
						if (previousNote == nullptr && i > 0) {
							TrackMeasure^ prevMeasure = track->Measures[i - 1];
							if (prevMeasure != nullptr && prevMeasure->Beats != nullptr) {
								for each (Beat ^ checkBeat in prevMeasure->Beats) {
									for each (Note ^ checkNote in checkBeat->Notes) {
										if (checkNote->String == currentNote->String &&
											checkNote->Value == currentNote->Value) {
											previousBeat = checkBeat;
											previousNote = checkNote;
										}
									}
								}
							}
						}

						// Draw tie line if we found the previous note
						if (previousBeat != nullptr && previousNote != nullptr) {
							// Calculate note text dimensions
							String^ prevNoteText	= previousNote->Value.ToString();
							String^ currentNoteText = currentNote->Value.ToString();
							SizeF prevNoteSize		= g->MeasureString(prevNoteText, noteFont);
							SizeF currentNoteSize	= g->MeasureString(currentNoteText, noteFont);

							// Calculate x positions for both notes
							float prevNoteX = (float)(TicksToPixels(previousBeat->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH);
							float currentNoteX = (float)(TicksToPixels(currentBeat->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH);

							// Get y position for the string
							float stringY = stringYPositions[currentNote->String - 1];

							// Calculate start and end points at bottom corners of note text
							float startX	= prevNoteX		+ (prevNoteSize.Width		/ 2);	// Bottom right of previous note
							float endX		= currentNoteX	- (currentNoteSize.Width	/ 2);	// Bottom left of current note
							float startY	= stringY		+ (prevNoteSize.Height		/ 2);
							float endY		= stringY		+ (currentNoteSize.Height	/ 2);

							// Calculate control points for Bezier curve that dips below
							float controlHeight = 8.0f * (float)Math::Min(2.0, Math::Max(0.5, zoomLevel));
							Drawing2D::GraphicsPath^ tiePath = gcnew Drawing2D::GraphicsPath();

							array<Drawing::PointF>^ curvePoints = gcnew array<Drawing::PointF>(4) {
								Drawing::PointF(startX, startY),											// Start at bottom right of first note
								Drawing::PointF(startX + (endX - startX) / 3, startY + controlHeight),		// First control point
								Drawing::PointF(startX + (endX - startX) * 2 / 3, endY + controlHeight),	// Second control point
								Drawing::PointF(endX, endY)													// End at bottom left of second note
							};

							tiePath->AddBeziers(curvePoints);
							g->DrawPath(tiePen, tiePath);

							delete tiePath;
						}
					}
				}

				measureStartTick = measureEndTick;
			}
		}
		finally {
			delete tiePen;
			delete noteFont;
		}
	}

	void Widget_Timeline::DrawDrumSymbol(Graphics^ g, DrumNotationType symbolType, float x, float y, float size)
	{
		// Create pens and brushes for drawing
		Pen^ outlinePen = gcnew Pen(Color::FromArgb(180, currentTheme.Text), 1.5f);
		SolidBrush^ fillBrush = gcnew SolidBrush(Color::FromArgb(180, currentTheme.Text));

		try {
			switch (symbolType)
			{
				case DrumNotationType::FilledDiamond:
				{
					array<PointF>^ points = gcnew array<PointF>(4) {
						PointF(x, y - size),	// top
						PointF(x + size, y),	// right
						PointF(x, y + size),	// bottom
						PointF(x - size, y)		// left
					};
					g->FillPolygon(fillBrush, points);
					break;
				}

				case DrumNotationType::HollowDiamond:
				{
					array<PointF>^ points = gcnew array<PointF>(4) {
						PointF(x, y - size),
						PointF(x + size, y),
						PointF(x, y + size),
						PointF(x - size, y)
					};
					g->DrawPolygon(outlinePen, points);
					break;
				}

				case DrumNotationType::CircledX:
				{
					// Draw circle
					g->DrawEllipse(outlinePen, x - size, y - size, size * 2, size * 2);
					// Draw X
					float xSize = size * 0.7f;
					g->DrawLine(outlinePen, x - xSize, y - xSize, x + xSize, y + xSize);
					g->DrawLine(outlinePen, x - xSize, y + xSize, x + xSize, y - xSize);
					break;
				}

				case DrumNotationType::AccentedX:
				{
					Pen^ thickPen = gcnew Pen(Color::FromArgb(180, currentTheme.Text), 2.5f);
					
					// Draw X
					float xSize = size * 0.8f;
					g->DrawLine(thickPen, x - xSize, y - xSize, x + xSize, y + xSize);
					g->DrawLine(thickPen, x - xSize, y + xSize, x + xSize, y - xSize);
					// Draw accent mark (^)
					array<PointF>^ accentPoints = gcnew array<PointF>(3) {
						PointF(x - size, y - size * 1.5f),
						PointF(x, y - size * 2.0f),
						PointF(x + size, y - size * 1.5f)
					};
					g->DrawLines(thickPen, accentPoints);

					delete thickPen;
					break;
				}

				case DrumNotationType::RegularX:
				{
					float xSize = size * 0.8f;
					g->DrawLine(outlinePen, x - xSize, y - xSize, x + xSize, y + xSize);
					g->DrawLine(outlinePen, x - xSize, y + xSize, x + xSize, y - xSize);
					break;
				}

				case DrumNotationType::NoteEllipse:
				{
					g->FillEllipse(fillBrush, x - size * 0.7f, y - size * 0.5f, size * 1.4f, size);
					break;
				}

				case DrumNotationType::Unknown:
				{
					fillBrush->Color = Color::Red;
					g->FillEllipse(fillBrush, x - size * 0.7f, y - size * 0.5f, size * 1.4f, size);
					break;
				}
			}
		}
		finally {
			delete outlinePen;
			delete fillBrush;
		}
	}

	TabStringInfo Widget_Timeline::DrawTablatureStrings(Graphics^ g, Rectangle bounds, float availableHeight, float logScale, int numStrings)
	{
		TabStringInfo info;

		// Calculate spacing and dimensions
		const float BASE_STRING_SPACING = 10.0f;
		const float SPACING_SCALE_FACTOR = 3.0f;
		float scaledStringSpacing = BASE_STRING_SPACING + (logScale * SPACING_SCALE_FACTOR);
		scaledStringSpacing = Math::Min(Math::Max(scaledStringSpacing, 12.0f), 40.0f);

		float Total_Tab_Height	= scaledStringSpacing * (numStrings - 1);
		float verticalOffset	= bounds.Y + TRACK_PADDING + (availableHeight - Total_Tab_Height) / 2;

		// Store calculated values
		info.StringSpacing		= scaledStringSpacing;
		info.TotalHeight		= Total_Tab_Height;
		info.VerticalOffset		= verticalOffset;
		info.StringYPositions	= gcnew array<float>(numStrings);

		if(Total_Tab_Height < availableHeight) {
			// Draw string lines and calculate positions
			for (int i = 0; i < numStrings; i++) {
				info.StringYPositions[i] = verticalOffset + (i * scaledStringSpacing);
				g->DrawLine(cachedStringPen, (float)bounds.X, info.StringYPositions[i], (float)bounds.Right, info.StringYPositions[i]);
			}
		}

		return info;
	}

	float Widget_Timeline::GetSubdivisionLevel()
	{
		if (D2DRenderer == nullptr) {
			return 1;
		}
		
		return D2DRenderer->GetSubdivisionLevel((float)TicksToPixels(TICKS_PER_QUARTER));
	}

	int Widget_Timeline::GetTrackTop(Track^ track)
	{
		if (this->D2DRenderer == nullptr) {
			return 0;
		}
		
		return this->D2DRenderer->GetTrackTop(track);
	}

	int Widget_Timeline::GetTotalTracksHeight()
	{
		int totalHeight = 0;
		for each (Track ^ track in tracks)
		{
			totalHeight += track->Height;
		}
		return totalHeight;
	}

	void Widget_Timeline::BeginTrackResize(Track^ track, int mouseY)
	{
		trackBeingResized	= track;
		resizeStartY		= mouseY;
		initialTrackHeight	= track->Height;

		// Change cursor to resize cursor
		this->Cursor = Cursors::SizeNS;
	}

	void Widget_Timeline::UpdateTrackResize(int mouseY)
	{
		if (trackBeingResized != nullptr)
		{
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

	bool Widget_Timeline::IsOverTrackDivider(Point mousePoint, Track^% outTrack)
	{
		// Adjust mouse position for scroll
		int adjustedY = mousePoint.Y - scrollPosition->Y;
		outTrack = nullptr;

		// Start from header height
		int y = HEADER_HEIGHT;

		// Check each track except the last one (no divider after last track)
		for (int i = 0; i < tracks->Count; i++)
		{
			Track^ track = tracks[i];
			int height = track->Height;
			int dividerY = y + height;

			// Check if mouse is within the divider area (using adjusted Y position)
			if (adjustedY >= dividerY - TRACK_RESIZE_HANDLE_HEIGHT && adjustedY <= dividerY + TRACK_RESIZE_HANDLE_HEIGHT)
			{
				outTrack = track;
				return true;
			}

			y += height;
		}

		return false;
	}

	bool Widget_Timeline::IsOverTrackButton(Track^ track, int buttonIndex, Point mousePoint)
	{
		if (track == nullptr) return false;

		if (buttonIndex == 1) {
			// Notation button only exists for drum tracks
			if (!track->IsDrumTrack) return false;
		}

		Rectangle headerBounds = GetTrackHeaderBounds(track);
		headerBounds.Y += scrollPosition->Y;

		Rectangle buttonBounds = GetTrackButtonBounds(headerBounds, buttonIndex);
		return buttonBounds.Contains(mousePoint);
	}

	Rectangle Widget_Timeline::GetTrackButtonBounds(Rectangle headerBounds, int buttonIndex)
	{
		return Rectangle(
			headerBounds.Right - (BUTTON_SIZE + BUTTON_MARGIN) * (buttonIndex + 1),
			headerBounds.Y + BUTTON_MARGIN,
			BUTTON_SIZE,
			BUTTON_SIZE
		);
	}

	Rectangle Widget_Timeline::CalculateBarBounds(BarEvent^ bar, Rectangle bounds)
	{
		if (D2DRenderer == nullptr) {
			return bounds;
		}

		return D2DRenderer->GetBarBounds(bar, bounds);

		// Former implementation
		/*
		x = TicksToPixels(bar->StartTick) + scrollPosition->X + TRACK_HEADER_WIDTH;
		width = Math::Max(1, TicksToPixels(bar->Duration)); // Ensure minimum width of 1 pixel

		barBounds = Rectangle(x, bounds.Y + TRACK_PADDING, width, bounds.Height - TRACK_PADDING * 2);
		*/
	}

	void Widget_Timeline::RecalculateMeasurePositions()
	{
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

	double Widget_Timeline::GetVirtualWidth()
	{
		// Calculate total width based on musical content
		double tickScale = 16.0 / TICKS_PER_QUARTER;
		return (double)TotalTicks * tickScale * zoomLevel;
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

		if (D2DRenderer != nullptr) {
			D2DRenderer->SetZoomLevel(zoomLevel);
		}

		// Maintain position of reference point
		int newPixelPosition = TicksToPixels(tickAtPosition);
		int positionOffset = referencePoint.X - TRACK_HEADER_WIDTH;
		scrollPosition->X = -(newPixelPosition - positionOffset);

		// Ensure proper alignment
		UpdateScrollBounds();
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
		hScrollBar->Minimum		= 0;
		hScrollBar->Maximum		= Math::Max(0, totalUnits);
		hScrollBar->LargeChange = viewportUnits;
		hScrollBar->SmallChange = 1;

		// Ensure current position is valid
		int currentUnit = GetScrollUnits(-scrollPosition->X);
		hScrollBar->Value = Math::Min(Math::Max(currentUnit, hScrollBar->Minimum), hScrollBar->Maximum - hScrollBar->LargeChange + 1);
	}

	int Widget_Timeline::GetScrollUnits(double width)
	{
		// Convert pixel width to scroll units
		return (int)Math::Ceiling(width / SCROLL_UNIT);
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
	
	void Widget_Timeline::OnScroll(Object^ sender, ScrollEventArgs^ e) {
		// Convert scroll units to pixels
		double newScrollX = -(double)e->NewValue * SCROLL_UNIT;

		// Update scroll position
		scrollPosition->X = (int)Math::Round(newScrollX);

		// Request redraw
		Invalidate();
	}

	void Widget_Timeline::OnVerticalScroll(Object^ sender, ScrollEventArgs^ e)
	{
		scrollPosition->Y = -e->NewValue;
		Invalidate();
	}
}
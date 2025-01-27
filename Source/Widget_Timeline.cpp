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
		this->BackColor = _CurrentTheme.TrackBackground;

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
		

		// Initialize state
		_CurrentTheme = Theme_Manager::Get_Instance()->GetTimelineTheme();
		_ZoomLevel = 1.0;
		_ScrollPosition = gcnew Point(0, 0);
		_Tracks = gcnew List<Track^>();
		_Measures = gcnew List<Measure^>();

		InitializeToolSystem();

		_D2DRenderer	= gcnew Timeline_Direct2DRenderer(_Tracks, _Measures, _ZoomLevel, _ScrollPosition);
		if (_D2DRenderer->Initialize(this)) {
			_D2DRenderer->Resize(this->Width, this->Height);
			_D2DRenderer->SetThemeColors(_CurrentTheme.Background, _CurrentTheme.HeaderBackground, _CurrentTheme.Text, _CurrentTheme.MeasureLine, _CurrentTheme.BeatLine, _CurrentTheme.SubdivisionLine, _CurrentTheme.SelectionHighlight, _CurrentTheme.TrackBackground, _CurrentTheme.TrackBackground);
			_D2DRenderer->SetTimelineAccess(this);
			_D2DRenderer->PreloadImages();
		}

		_ResourceManager		= gcnew TimelineResourceManager();
		_PerformanceMetrics	= gcnew PerformanceMetrics();
	}

	Widget_Timeline::~Widget_Timeline()
	{
		delete _PerformanceMetrics;
	}

	void Widget_Timeline::AddTrack(String^ name, int octave)
	{
		Track^ track = gcnew Track(name, octave);
		track->Height = Widget_Timeline::DEFAULT_TRACK_HEIGHT;
		_Tracks->Add(track);

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
		_Measures->Add(newMeasure);

		for each (Track ^ track in _Tracks) {
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
		if (_Tracks->Contains(track)) {
			track->AddBar(startTick, length, color);
			Invalidate();
		}
	}

	void Widget_Timeline::Clear()
	{
		for each (Track ^ track in _Tracks) {
			track->Measures->Clear();
		}

		// Clear all collections
		_Tracks->Clear();
		_Measures->Clear();

		// Reset scroll and zoom
		_ScrollPosition = gcnew Point(0, 0);
		_ZoomLevel = 1.0;

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->SetZoomLevel(_ZoomLevel);
			_D2DRenderer->SetScrollPositionReference(_ScrollPosition);
		}

		// Reset background color
		this->BackColor = _CurrentTheme.Background;

		// Force redraw
		Invalidate();
	}

	void Widget_Timeline::ZoomIn()
	{
		double newZoom;
		if (_ZoomLevel < 1.0) {
			newZoom = _ZoomLevel * 1.2;
		}
		else {
			newZoom = _ZoomLevel * 1.05;
		}
		SetZoom(newZoom);
	}

	void Widget_Timeline::ZoomOut()
	{
		double newZoom;
		if (_ZoomLevel > 1.0) {
			newZoom = _ZoomLevel / 1.05;
		}
		else {
			newZoom = _ZoomLevel / 1.2;
		}
		SetZoom(newZoom);
	}

	void Widget_Timeline::SetZoom(double newZoom)
	{
		// Clamp zoom level to valid range
		newZoom = Math::Min(Math::Max(newZoom, MIN_ZOOM_LEVEL), MAX_ZOOM_LEVEL);

		// If no change in zoom, exit early
		if (Math::Abs(newZoom - _ZoomLevel) < 0.0001) return;

		// Calculate the center point of the visible area in ticks
		int visibleWidth = Width - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;
		int centerTick = PixelsToTicks(-_ScrollPosition->X + (visibleWidth / 2));

		// Store old zoom level and apply new zoom
		double oldZoom = _ZoomLevel;
		_ZoomLevel = newZoom;

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->SetZoomLevel(_ZoomLevel);
		}

		// Recalculate scroll position to maintain center
		int newCenterPixel = TicksToPixels(centerTick);
		_ScrollPosition->X = -(newCenterPixel - (visibleWidth / 2));

		// Ensure proper alignment
		UpdateScrollBounds();
		Invalidate();
	}

	void Widget_Timeline::ScrollTo(Point newPosition) {
		_ScrollPosition = newPosition;
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

		int totalHeight = Timeline_Direct2DRenderer::HEADER_HEIGHT;
		for each (Track ^ t in _Tracks) {
			totalHeight += t->Height;
		}

		// Update scrollbars and bounds
		UpdateVerticalScrollBarRange();
		UpdateScrollBounds();

		// Ensure current scroll position is still valid
		int viewportHeight = Height - Timeline_Direct2DRenderer::HEADER_HEIGHT - _HScrollBar->Height;
		if (-_ScrollPosition->Y + viewportHeight > totalHeight)
		{
			_ScrollPosition->Y = Math::Min(0, -(totalHeight - viewportHeight));
			_VScrollBar->Value = -_ScrollPosition->Y;
		}

		Invalidate();
	}

	void Widget_Timeline::SetAllTracksHeight(int height)
	{
		for each (Track ^ track in _Tracks)
		{
			SetTrackHeight(track, height);
		}
	}

	void Widget_Timeline::SetToolSnapping(SnappingType type)
	{
		this->_SnappingType = type;
	}

	void Widget_Timeline::SetCurrentTool(TimelineToolType tool)
	{
		if (_CurrentToolType != tool)
		{
			// Deactivate current tool if one exists
			if (_CurrentTool != nullptr) {
				_CurrentTool->Deactivate();
			}

			// Set new tool
			_CurrentToolType = tool;
			_CurrentTool = _Tools[tool];

			// Activate new tool
			_CurrentTool->Activate();
			this->Cursor = _CurrentTool->Cursor;
			Invalidate();
		}
	}

	int Widget_Timeline::SnapTickToGrid(int tick)
	{
		// Get current subdivision level based on zoom
		float subdivLevel = GetSubdivisionLevel();

		// Calculate snap resolution based on subdivision level
		int snapResolution = Timeline_Direct2DRenderer::TICKS_PER_QUARTER / (int)subdivLevel;

		// Round to nearest snap point
		//return ((tick + (snapResolution / 2)) / snapResolution) * snapResolution;

		// Calculate the grid point to the left of the given tick
		return (tick / snapResolution) * snapResolution;
	}

	int Widget_Timeline::SnapTickBasedOnType(int tick, Point mousePos)
	{
		const int SNAP_THRESHOLD = 480; // Maximum distance in ticks for bar snapping

		switch (_SnappingType)
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
		if (measureNumber < 1 || measureNumber > _Measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + _Measures->Count);
		}

		// Get the start tick of the requested measure
		int targetTick = GetMeasureStartTick(measureNumber);

		// Convert to pixels
		int targetPixel = TicksToPixels(targetTick);

		// Calculate scroll position to center the measure
		int viewportWidth = Width - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;
		int newScrollX = -targetPixel + (viewportWidth / 2);

		// Clamp scroll position to valid range
		double virtualWidth = GetVirtualWidth();
		double maxScroll = -(virtualWidth - viewportWidth);
		newScrollX = (int)Math::Round(Math::Max(maxScroll, Math::Min(0.0, (double)newScrollX)));

		// Update scroll position
		_ScrollPosition->X = newScrollX;

		// Update scrollbar
		UpdateScrollBarRange();

		// Request redraw
		Invalidate();
	}

	int Widget_Timeline::GetMeasureStartTick(int measureNumber)
	{
		if (measureNumber < 1 || measureNumber > _Measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + _Measures->Count);
		}

		int startTick = 0;
		// Sum up lengths of previous measures
		for (int i = 0; i < measureNumber - 1; i++) {
			startTick += _Measures[i]->Length;
		}
		return startTick;
	}

	int Widget_Timeline::GetMeasureLength(int measureNumber) {
		if (measureNumber < 1 || measureNumber > _Measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + _Measures->Count);
		}

		return _Measures[measureNumber - 1]->Length;
	}

	void Widget_Timeline::UpdateCursor(System::Windows::Forms::Cursor^ cursor)
	{
		if (this->Cursor != cursor) {
			this->Cursor = cursor;
		}
	}

	int Widget_Timeline::TicksToPixels(int ticks)
	{
		if (this->_D2DRenderer == nullptr) {
			return 0;
		}

		float Pixels = this->_D2DRenderer->TicksToPixels(ticks);
		
		// Protect against overflow
		if (Pixels > Int32::MaxValue) return Int32::MaxValue;
		if (Pixels < Int32::MinValue) return Int32::MinValue;

		return (int)Math::Round(Pixels);
	}

	int Widget_Timeline::PixelsToTicks(int pixels)
	{
		if (this->_D2DRenderer == nullptr) {
			return 0;
		}

		return (int)Math::Round(this->_D2DRenderer->PixelsToTicks(pixels));
	}

	Track^ Widget_Timeline::GetTrackAtPoint(Point p)
	{
		if (_D2DRenderer == nullptr) {
			return nullptr;
		}

		return _D2DRenderer->GetTrackAtPoint(p);
	}

	Rectangle Widget_Timeline::GetTrackBounds(Track^ track)
	{
		if (_D2DRenderer == nullptr) {
			return Rectangle();
		}

		return _D2DRenderer->GetTrackBounds(track);
	}

	Rectangle Widget_Timeline::GetTrackHeaderBounds(Track^ track)
	{
		if (_D2DRenderer == nullptr) {
			return Rectangle();
		}

		return _D2DRenderer->GetTrackHeaderBounds(track);
	}

	Rectangle Widget_Timeline::GetTrackContentBounds(Track^ track)
	{
		if (_D2DRenderer == nullptr) {
			return Rectangle();
		}
		
		return _D2DRenderer->GetTrackContentBounds(track);
	}

	Measure^ Widget_Timeline::GetMeasureAtTick(int tick)
	{
		if (_D2DRenderer == nullptr) {
			return nullptr;
		}
		
		return _D2DRenderer->GetMeasureAtTick(tick);
	}

	BarEvent^ Widget_Timeline::GetBarAtPoint(Point p)
	{
		if (_D2DRenderer == nullptr) {
			return nullptr;
		}

		return _D2DRenderer->GetBarAtPoint(p);
	}

	String^ Widget_Timeline::SaveBarEventsToFile(String^ filePath)
	{
		try
		{
			List<String^>^ lines = gcnew List<String^>();

			// Header with version
			lines->Add("MIDILightDrawer_BarEvents_v1.0");

			// Save pattern information - number of measures
			lines->Add(_Measures->Count.ToString());

			// Save time signatures for all measures
			for each (Measure ^ measure in _Measures) {
				lines->Add(String::Format("{0},{1}",
					measure->Numerator,
					measure->Denominator));
			}

			// Calculate total number of bars across all tracks
			int totalBars = 0;
			for each (Track ^ track in _Tracks) {
				totalBars += track->Events->Count;
			}
			lines->Add(totalBars.ToString());

			// Save each bar's data with track name
			for each (Track ^ track in _Tracks) {
				for each (BarEvent ^ bar in track->Events) {
					String^ barData = String::Format("{0},{1},{2},{3},{4},{5},{6}",
						bar->StartTick,
						bar->Duration,
						_Tracks->IndexOf(track),
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
			if (fileMeasureCount != _Measures->Count)
				return String::Format(
					"Pattern mismatch: File has {0} measures, but timeline has {1} measures",
					fileMeasureCount, _Measures->Count);

			// Verify time signatures for each measure
			for (int i = 0; i < _Measures->Count; i++)
			{
				String^ fileMeasureInfo = lines[2 + i];
				array<String^>^ parts = fileMeasureInfo->Split(',');

				if (parts->Length != 2)
					return String::Format("Invalid time signature format in measure {0}", i + 1);

				int fileNumerator, fileDenominator;
				if (!Int32::TryParse(parts[0], fileNumerator) ||
					!Int32::TryParse(parts[1], fileDenominator))
					return String::Format("Invalid time signature numbers in measure {0}", i + 1);

				if (fileNumerator != _Measures[i]->Numerator ||
					fileDenominator != _Measures[i]->Denominator)
					return String::Format(
						"Time signature mismatch at measure {0}: File has {1}/{2}, but timeline has {3}/{4}",
						i + 1, fileNumerator, fileDenominator,
						_Measures[i]->Numerator, _Measures[i]->Denominator);
			}

			// Create mapping of track names to indices
			Dictionary<String^, Track^>^ trackMap = gcnew Dictionary<String^, Track^>();
			for each (Track ^ track in _Tracks) {
				trackMap[track->Name] = track;
			}

			// Clear existing bars from all tracks
			for each (Track^ track in _Tracks) {
				track->Events->Clear();
			}

			int barsStartLine = 2 + _Measures->Count;
			int barCount;

			if (!Int32::TryParse(lines[barsStartLine], barCount)) {
				return "Invalid bar count";
			}

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

				Track^ TargetTrack = trackMap[trackName];

				int startTick, length, r, g, b;
				if (!Int32::TryParse(parts[0], startTick) ||
					!Int32::TryParse(parts[1], length) ||
					!Int32::TryParse(parts[3], r) ||
					!Int32::TryParse(parts[4], g) ||
					!Int32::TryParse(parts[5], b))
					continue;

				TargetTrack->AddBar(startTick, length, Color::FromArgb(r, g, b));
			}

			// Sort events in each track
			for each (Track ^ track in _Tracks) {
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
		String^ report = _PerformanceMetrics->GetReport();
		Console::WriteLine(report);
		_PerformanceMetrics->Reset();
	}


	///////////////////////
	// Protected Methods //
	///////////////////////
	void Widget_Timeline::OnPaint(PaintEventArgs^ e)
	{
		_PerformanceMetrics->StartFrame();

		if (_D2DRenderer->BeginDraw())
		{
			// Clear the background
			if(this->Measures->Count == 0) {
				_D2DRenderer->DrawWidgetBackground();
			}
				
			_D2DRenderer->DrawTrackBackground();
			_D2DRenderer->DrawMeasureNumbers();
			_D2DRenderer->DrawTrackContent(_ResizeHoverTrack);
			_D2DRenderer->DrawToolPreview();

			_D2DRenderer->EndDraw();
		}
		
		_PerformanceMetrics->EndFrame();
	}

	void Widget_Timeline::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->Resize(Width, Height);
		}

		UpdateScrollBarRange();
		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::OnMouseDown(MouseEventArgs^ e)
	{
		if (_HoveredButton.Track != nullptr)
		{
			switch (_HoveredButton.ButtonIndex)
			{
				case 0: // Tablature toggle button
					_HoveredButton.Track->ShowTablature = !_HoveredButton.Track->ShowTablature;
					break;
				case 1: // Notation toggle button (only for drum tracks)
					if (_HoveredButton.Track->IsDrumTrack) {
						_HoveredButton.Track->ShowAsStandardNotation = !_HoveredButton.Track->ShowAsStandardNotation;
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

		if (_CurrentTool != nullptr)
		{
			_CurrentTool->OnMouseDown(e);
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
		if (newHoveredButton.Track != _HoveredButton.Track || newHoveredButton.ButtonIndex != _HoveredButton.ButtonIndex) {
			_HoveredButton = newHoveredButton;
			Invalidate();
		}

		if (isOverAnyButton) {
			this->Cursor = Cursors::Hand;
			return;  // Don't process other mouse move logic
		}

		if (_TrackBeingResized != nullptr)
		{
			// If we're actively resizing, update the track height
			UpdateTrackResize(e->Y);
		}
		else if (isOverDivider)
		{
			// Just hovering over a divider
			if (_ResizeHoverTrack != hoverTrack)
			{
				_ResizeHoverTrack = hoverTrack;
				Invalidate(); // Redraw to show hover state if needed
			}

			this->Cursor = Cursors::SizeNS;
		}
		else
		{
			// Not over a divider
			if (_ResizeHoverTrack != nullptr) {
				_ResizeHoverTrack = nullptr;
				Invalidate();
			}

			// Handle normal tool behavior
			Control::OnMouseMove(e);
			if (_CurrentTool != nullptr)
			{
				_CurrentTool->OnMouseMove(e);
				// Only update cursor if we're not in a special state
				if (!isOverAnyButton && !isOverDivider && _TrackBeingResized == nullptr) {
					this->Cursor = _CurrentTool->Cursor;
				}
			}
		}
	}

	void Widget_Timeline::OnMouseUp(MouseEventArgs^ e)
	{
		Control::OnMouseUp(e);

		if (_TrackBeingResized != nullptr) {
			EndTrackResize();
			return;
		}

		if (_CurrentTool != nullptr) {
			_CurrentTool->OnMouseUp(e);
		}
	}

	void Widget_Timeline::OnMouseWheel(MouseEventArgs^ e)
	{
		if (Control::ModifierKeys == Keys::Control)
		{
			double newZoom;
			if (e->Delta > 0) {
				if (_ZoomLevel < 1.0) {
					newZoom = _ZoomLevel * 1.2;
				}
				else {
					newZoom = _ZoomLevel * 1.05;
				}
			}
			else {
				if (_ZoomLevel > 1.0) {
					newZoom = _ZoomLevel / 1.05;
				}
				else {
					newZoom = _ZoomLevel / 1.2;
				}
			}
			SetZoomLevelAtPoint(newZoom, Point(e->X, e->Y));
		}
		else if (Control::ModifierKeys == Keys::Shift) {
			int scrollUnits = e->Delta > 0 ? -1 : 1;
			int newValue = Math::Min(Math::Max(_HScrollBar->Value + scrollUnits,
				_HScrollBar->Minimum),
				_HScrollBar->Maximum - _HScrollBar->LargeChange + 1);
			_HScrollBar->Value = newValue;
			OnScroll(_HScrollBar, gcnew ScrollEventArgs(ScrollEventType::ThumbPosition, newValue));
		}
		else
		{
			int scrollUnits = e->Delta > 0 ? -_VScrollBar->SmallChange : _VScrollBar->SmallChange;
			int newValue = Math::Min(Math::Max(
				_VScrollBar->Value + scrollUnits,
				_VScrollBar->Minimum),
				_VScrollBar->Maximum - _VScrollBar->LargeChange + 1);
			_VScrollBar->Value = newValue;
			OnVerticalScroll(_VScrollBar, gcnew ScrollEventArgs(ScrollEventType::ThumbPosition, newValue));
		}
	}

	void Widget_Timeline::OnKeyDown(KeyEventArgs^ e)
	{
		Control::OnKeyDown(e);
		if (_CurrentTool != nullptr) {
			_CurrentTool->OnKeyDown(e);
		}
	}

	void Widget_Timeline::OnKeyUp(KeyEventArgs^ e)
	{
		Control::OnKeyUp(e);
		if (_CurrentTool != nullptr) {
			_CurrentTool->OnKeyUp(e);
		}
	}

	void Widget_Timeline::OnHandleCreated(EventArgs^ e)
	{
		UserControl::OnHandleCreated(e);

		if (_D2DRenderer != nullptr)
		{
			System::Diagnostics::Debug::WriteLine("OnHandleCreated: Initializing D2D");
			if (_D2DRenderer->Initialize(this))
			{
				_D2DRenderer->Resize(Width, Height);
			}
		}
	}

	/////////////////////
	// Private Methods //
	/////////////////////
	void Widget_Timeline::InitializeComponent()
	{
		// Initialize horizontal scrollbar
		_HScrollBar = gcnew System::Windows::Forms::HScrollBar();
		_HScrollBar->Dock = System::Windows::Forms::DockStyle::Bottom;
		_HScrollBar->Height = System::Windows::Forms::SystemInformation::HorizontalScrollBarHeight;
		_HScrollBar->Scroll += gcnew ScrollEventHandler(this, &Widget_Timeline::OnScroll);

		// Initialize vertical scrollbar
		_VScrollBar = gcnew System::Windows::Forms::VScrollBar();
		_VScrollBar->Dock = System::Windows::Forms::DockStyle::Right;
		_VScrollBar->Width = System::Windows::Forms::SystemInformation::VerticalScrollBarWidth;
		_VScrollBar->Scroll += gcnew ScrollEventHandler(this, &Widget_Timeline::OnVerticalScroll);

		// Add scrollbars to control
		this->Controls->Add(_VScrollBar);
		this->Controls->Add(_HScrollBar);
	}

	void Widget_Timeline::InitializeToolSystem()
	{
		_Tools = gcnew Dictionary<TimelineToolType, TimelineTool^>();

		// Create tools
		_Tools->Add(TimelineToolType::Pointer	, gcnew PointerTool	(this));
		_Tools->Add(TimelineToolType::Draw		, gcnew DrawTool	(this));
		_Tools->Add(TimelineToolType::Split		, gcnew SplitTool	(this));
		_Tools->Add(TimelineToolType::Erase		, gcnew EraseTool	(this));
		_Tools->Add(TimelineToolType::Duration	, gcnew DurationTool(this));
		_Tools->Add(TimelineToolType::Color		, gcnew ColorTool	(this));
		_Tools->Add(TimelineToolType::Fade		, gcnew FadeTool	(this));
		_Tools->Add(TimelineToolType::Strobe		, gcnew StrobeTool	(this));

		// Set default tool
		_CurrentToolType = TimelineToolType::Pointer;
		_CurrentTool = _Tools[_CurrentToolType];
	}

	float Widget_Timeline::GetSubdivisionLevel()
	{
		if (_D2DRenderer == nullptr) {
			return 1;
		}
		
		return _D2DRenderer->GetSubdivisionLevel((float)TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER));
	}

	int Widget_Timeline::GetTrackTop(Track^ track)
	{
		if (this->_D2DRenderer == nullptr) {
			return 0;
		}
		
		return this->_D2DRenderer->GetTrackTop(track);
	}

	int Widget_Timeline::GetTotalTracksHeight()
	{
		int totalHeight = 0;
		for each (Track ^ track in _Tracks)
		{
			totalHeight += track->Height;
		}
		return totalHeight;
	}

	void Widget_Timeline::BeginTrackResize(Track^ track, int mouseY)
	{
		_TrackBeingResized	= track;
		_ResizeStartY		= mouseY;
		_InitialTrackHeight	= track->Height;

		// Change cursor to resize cursor
		this->Cursor = Cursors::SizeNS;
	}

	void Widget_Timeline::UpdateTrackResize(int mouseY)
	{
		if (_TrackBeingResized != nullptr)
		{
			int delta = mouseY - _ResizeStartY;
			int newHeight = Math::Max(MINIMUM_TRACK_HEIGHT, _InitialTrackHeight + delta);

			// Store previous total height
			int oldTotalHeight = GetTotalTracksHeight();

			// Update the track height
			SetTrackHeight(_TrackBeingResized, newHeight);

			// Get new total height
			int newTotalHeight = GetTotalTracksHeight();

			// If this changes total content height, ensure scroll position is still valid
			if (oldTotalHeight != newTotalHeight) {
				int viewportHeight = Height - Timeline_Direct2DRenderer::HEADER_HEIGHT - _HScrollBar->Height;

				// If we're scrolled to bottom, maintain bottom alignment
				if (-_ScrollPosition->Y + viewportHeight >= oldTotalHeight) {
					_ScrollPosition->Y = Math::Min(0, -(newTotalHeight - viewportHeight));
					if (_VScrollBar != nullptr) {
						_VScrollBar->Value = -_ScrollPosition->Y;
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
		_TrackBeingResized = nullptr;

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
		int adjustedY = mousePoint.Y - _ScrollPosition->Y;
		outTrack = nullptr;

		// Start from header height
		int y = Timeline_Direct2DRenderer::HEADER_HEIGHT;

		// Check each track except the last one (no divider after last track)
		for (int i = 0; i < _Tracks->Count; i++)
		{
			Track^ track = _Tracks[i];
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
		headerBounds.Y += _ScrollPosition->Y;

		Rectangle buttonBounds = GetTrackButtonBounds(headerBounds, buttonIndex);
		return buttonBounds.Contains(mousePoint);
	}

	Rectangle Widget_Timeline::GetTrackButtonBounds(Rectangle headerBounds, int buttonIndex)
	{
		return Rectangle(
			headerBounds.Right - (Timeline_Direct2DRenderer::BUTTON_SIZE + Timeline_Direct2DRenderer::BUTTON_MARGIN) * (buttonIndex + 1),
			headerBounds.Y + Timeline_Direct2DRenderer::BUTTON_MARGIN,
			Timeline_Direct2DRenderer::BUTTON_SIZE,
			Timeline_Direct2DRenderer::BUTTON_SIZE
		);
	}

	Rectangle Widget_Timeline::CalculateBarBounds(BarEvent^ bar, Rectangle bounds)
	{
		if (_D2DRenderer == nullptr) {
			return bounds;
		}

		return _D2DRenderer->GetBarBounds(bar, bounds);

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
		for each (Measure ^ measure in _Measures) {
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
		for each (Measure ^ m in _Measures) {
			if (m == measure) break;
			measureStartTick += m->Length;
		}

		return (double)(tick - measureStartTick) / measure->Length;
	}

	double Widget_Timeline::GetVirtualWidth()
	{
		// Calculate total width based on musical content
		double tickScale = 16.0 / Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		return (double)TotalTicks * tickScale * _ZoomLevel;
	}

	void Widget_Timeline::SetZoomLevelAtPoint(double newZoom, Point referencePoint)
	{
		// Clamp zoom level to valid range
		newZoom = Math::Min(Math::Max(newZoom, MIN_ZOOM_LEVEL), MAX_ZOOM_LEVEL);

		// If no change in zoom, exit early
		if (Math::Abs(newZoom - _ZoomLevel) < 0.0001) return;

		// Get position relative to content area
		Point contentPos = Point(referencePoint.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH, referencePoint.Y);

		// Find tick at reference point
		int tickAtPosition = PixelsToTicks(contentPos.X - _ScrollPosition->X);

		// Store old zoom level and apply new zoom
		_ZoomLevel = newZoom;

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->SetZoomLevel(_ZoomLevel);
		}

		// Maintain position of reference point
		int newPixelPosition = TicksToPixels(tickAtPosition);
		int positionOffset = referencePoint.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;
		_ScrollPosition->X = -(newPixelPosition - positionOffset);

		// Ensure proper alignment
		UpdateScrollBounds();
		Invalidate();
	}

	void Widget_Timeline::UpdateScrollBarRange()
	{
		// Calculate virtual width
		double virtualWidth = GetVirtualWidth();

		// Calculate viewport width (excluding header)
		int viewportWidth = Width - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

		// Convert to scroll units
		int totalUnits = GetScrollUnits(virtualWidth);
		int viewportUnits = GetScrollUnits(viewportWidth);

		// Ensure viewportUnits is at least 1
		viewportUnits = Math::Max(1, viewportUnits);

		// Update scrollbar
		_HScrollBar->Minimum		= 0;
		_HScrollBar->Maximum		= Math::Max(0, totalUnits);
		_HScrollBar->LargeChange = viewportUnits;
		_HScrollBar->SmallChange = 1;

		// Ensure current position is valid
		int currentUnit = GetScrollUnits(-_ScrollPosition->X);
		_HScrollBar->Value = Math::Min(Math::Max(currentUnit, _HScrollBar->Minimum), _HScrollBar->Maximum - _HScrollBar->LargeChange + 1);
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
		int viewportWidth = Width - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

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
		if (_ZoomLevel > 50.0) {
			gridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 64); // Even finer grid at very high zoom
		}
		else if (_ZoomLevel > 20.0) {
			gridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 32);
		}
		else if (_ZoomLevel > 10.0) {
			gridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 16);
		}
		else if (_ZoomLevel > 5.0) {
			gridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 8);
		}
		else {
			gridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 4);
		}

		if (gridPixels > 0) {
			// Snap scroll position to grid
			_ScrollPosition->X = (int)Math::Round((double)_ScrollPosition->X / gridPixels) * gridPixels;
		}

		// Clamp scroll position
		_ScrollPosition->X = (int)Math::Round(Math::Max(maxScroll, Math::Min(0.0, (double)_ScrollPosition->X)));

		// Update scrollbar to reflect new position/range
		UpdateScrollBarRange();
	}
	
	void Widget_Timeline::UpdateVerticalScrollBarRange()
	{
		int totalHeight = GetTotalTracksHeight();
		// Ensure viewportHeight is at least 0
		int viewportHeight = Math::Max(0, Height - Timeline_Direct2DRenderer::HEADER_HEIGHT - _HScrollBar->Height);

		// Enable/update scrollbar if content exceeds viewport
		if (totalHeight > viewportHeight)
		{
			_VScrollBar->Minimum = 0;
			_VScrollBar->Maximum = totalHeight + 20;  // Use full total height as Maximum plus a little extra
			// Ensure LargeChange is at least 1
			_VScrollBar->LargeChange = Math::Max(1, viewportHeight);
			_VScrollBar->SmallChange = Math::Max(1, viewportHeight / 20); // Adjust scroll speed

			// Ensure value stays within valid range
			int maxValue = Math::Max(0, totalHeight - viewportHeight);
			int currentValue = Math::Min(-_ScrollPosition->Y, maxValue);
			_VScrollBar->Value = Math::Max(0, currentValue);
		}
		else
		{
			// Content fits in viewport, but keep scrollbar visible
			_VScrollBar->Minimum = 0;
			_VScrollBar->Maximum = 0;
			_VScrollBar->LargeChange = 1;
			_VScrollBar->Value = 0;
			_ScrollPosition->Y = 0;
		}

		_VScrollBar->Visible = true;
	}
	
	void Widget_Timeline::OnScroll(Object^ sender, ScrollEventArgs^ e) {
		// Convert scroll units to pixels
		double newScrollX = -(double)e->NewValue * SCROLL_UNIT;

		// Update scroll position
		_ScrollPosition->X = (int)Math::Round(newScrollX);

		// Request redraw
		Invalidate();
	}

	void Widget_Timeline::OnVerticalScroll(Object^ sender, ScrollEventArgs^ e)
	{
		_ScrollPosition->Y = -e->NewValue;
		Invalidate();
	}
}
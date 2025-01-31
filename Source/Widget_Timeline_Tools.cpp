#include "Widget_Timeline_Tools.h"
#include "Widget_Timeline.h"

namespace MIDILightDrawer
{
	// Base TimelineTool Implementation
	TimelineTool::TimelineTool(Widget_Timeline^ timeline) : _Timeline(timeline)
	{
		_IsActive = false;
		_LastMousePos = Point(0, 0);
		_ModifierKeys = Keys::None;
	}

	void TimelineTool::Cursor::set(System::Windows::Forms::Cursor^ value)
	{
		_CurrentCursor = value;

		if (_Timeline != nullptr) {
			_Timeline->Cursor = value;
		}
	}

	bool TimelineTool::HasOverlappingBarsOnBarTrack(BarEvent^ bar)
	{
		Track^ TargetTrack = bar->ContainingTrack;

		if (TargetTrack == nullptr) {
			return false;
		}

		for each (BarEvent^ TrackBar in TargetTrack->Events)
		{
			// Skip comparing with itself (important for drag operations)
			if (bar == TrackBar) {
				continue;
			}

			// Check for time overlap
			bool TimeOverlap = bar->StartTick < TrackBar->EndTick && bar->EndTick > TrackBar->StartTick;

			if (TimeOverlap) {
				return true;
			}
		}

		return false;
	}

	bool TimelineTool::HasOverlappingBarsOnBarTrack(List<BarEvent^>^ barList)
	{
		if (barList == nullptr || barList->Count == 0) {
			return false;
		}

		// Check each preview bar against existing bars in the target track
		for each (BarEvent^ Bar in barList)
		{
			bool TimeOverlap = HasOverlappingBarsOnBarTrack(Bar);

			if (TimeOverlap) {
				return true;
			}
		}

		return false;
	}

	bool TimelineTool::HasOverlappingBarsOnSpecificTrack(Track^ track, BarEvent^ bar)
	{
		for each (BarEvent ^ TrackBar in track->Events)
		{
			// Skip comparing with itself (important for drag operations)
			if (bar == TrackBar) {
				continue;
			}

			// Check for time overlap
			bool TimeOverlap = bar->StartTick < TrackBar->EndTick && bar->EndTick > TrackBar->StartTick;

			if (TimeOverlap) {
				return true;
			}
		}

		return false;
	}

	bool TimelineTool::HasOverlappingBarsOnSpecificTrack(Track^ track, List<BarEvent^>^ barList)
	{
		if (track == nullptr || barList == nullptr || barList->Count == 0) {
			return false;
		}

		for each (BarEvent^ Bar in barList)
		{
			// Check for time overlap
			bool TimeOverlap = HasOverlappingBarsOnSpecificTrack(track, Bar);

			if (TimeOverlap) {
				return true;
			}
		}

		return false;
	}

	////////////////////////////////
	// PointerTool Implementation //
	////////////////////////////////
	PointerTool::PointerTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);

		_IsDragging = false;
		_DragStart = nullptr;
		_DragSourceTrack = nullptr;
		_DragTargetTrack = nullptr;
		_SelectedBars = gcnew List<BarEvent^>();
		_OriginalBarStartTicks = gcnew List<int>();
	}

	void PointerTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button != Windows::Forms::MouseButtons::Left) {
			return;
		}

		if (_IsPasting)
		{
			Track^ track = _Timeline->GetTrackAtPoint(Point(e->X, e->Y));
			if (track != nullptr && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
				FinishPaste();
			}
			return;
		}

		///////////////////////////////////////////////////
		// Handle different click scenarios form here on //
		///////////////////////////////////////////////////

		_DragStart = gcnew Point(e->X, e->Y);

		// Click in track header - select track
		if (e->X <= Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH || e->Y <= Timeline_Direct2DRenderer::HEADER_HEIGHT)
		{
			_SelectedBars->Clear();
			_Timeline->Invalidate();
			return;
		}

		// Click in track content - handle bars
		BarEvent^ Bar = _Timeline->GetBarAtPoint(*_DragStart);

		if (Bar == nullptr) {
			// Start selection rectangle if clicking empty space
			if (!Control::ModifierKeys.HasFlag(Keys::Control)) {
				_SelectedBars->Clear();
			}
			StartSelection(*_DragStart);

			_Timeline->Invalidate();
			return;
		}

		if (Control::ModifierKeys == Keys::Control)
		{
			// Toggle selection with Ctrl key
			if (_SelectedBars->Contains(Bar)) {
				_SelectedBars->Remove(Bar);
			}
			else {
				_SelectedBars->Add(Bar);
			}

			_Timeline->Invalidate();
			return;
		}

		// Check if clicking on an already selected bar
		if (_SelectedBars != nullptr && _SelectedBars->Contains(Bar))
		{
			StartMoving(*_DragStart);	// Start group drag
		}
		else
		{
			// Clear previous selection and select only this bar
			_SelectedBars->Clear();
			_SelectedBars->Add(Bar);

			StartMoving(*_DragStart);	// Start group drag
		}

		_Timeline->Invalidate();
	}

	void PointerTool::OnMouseMove(MouseEventArgs^ e)
	{
		_LastMousePos = Point(e->X, e->Y);  // Store current mouse position

		if (_IsPasting) {
			UpdatePaste(_LastMousePos);
		}
		else if (_IsDragging)
		{
			UpdateMoving(_LastMousePos);
		}
		else if (_IsSelecting)
		{
			UpdateSelection(_LastMousePos);
		}

		UpdateCursor(_LastMousePos);	// Update cursor based on position

		_Timeline->Invalidate();
	}

	void PointerTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsDragging)
		{
			FinishMoving(Point(e->X, e->Y));
		}
		else if (_IsSelecting) {
			EndSelection();
		}

		_DragStart = nullptr;
		_Timeline->Invalidate();
	}

	void PointerTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->Control)
		{
			if (e->KeyCode == Keys::C) {
				HandleCopy();
			}
			else if (e->KeyCode == Keys::V) {
				StartPaste();
			}
		}
		else if (e->KeyCode == Keys::Escape) {
			if(_IsPasting) {
				CancelPaste();
			}
			else if (_IsDragging) {
				CancelMoving();
			}

			_Timeline->Invalidate();
		}
		else if (e->KeyCode == Keys::Delete) {
			EraseSelectedBars();
		}
	}

	void PointerTool::OnKeyUp(KeyEventArgs^ e) { }

	void PointerTool::StartMoving(Point mousePos)
	{
		StoreOriginalPositions();

		_DragSourceTrack = _Timeline->GetTrackAtPoint(mousePos);
		_IsDragging = true;
	}

	void PointerTool::UpdateMoving(Point mousePos)
	{
		if (_SelectedBars == nullptr || _SelectedBars->Count == 0) {
			return;
		}
		
		// Calculate movement in ticks
		int PixelDelta = mousePos.X - _DragStart->X;
		int TickDelta = _Timeline->PixelsToTicks(PixelDelta);

		/*
		int EarliestTick = Int32::MaxValue;

		for each(BarEvent ^ Bar in _SelectedBars)
		{
			int Tick = Int32::MaxValue;
			if (_SelectedBars->IndexOf(Bar) == 0) {
				Tick = _Timeline->SnapTickBasedOnType(Bar->OriginalStartTick + TickDelta, mousePos);
			}
			else {
				int TickOffsetToFirstBar = Bar->OriginalStartTick - _SelectedBars[0]->OriginalStartTick;
				Tick = _SelectedBars[0]->StartTick + TickOffsetToFirstBar;
			}

			if (Tick < EarliestTick) {
				EarliestTick = Tick;
			}
		}

		
		if (EarliestTick < 0) {
			TickDelta -= EarliestTick;
		}
		*/

		// Store original positions
		for each (BarEvent^ bar in _SelectedBars) {
			_OriginalBarStartTicks->Add(bar->StartTick);
		}

		// Update positions of selected bars
		for each (BarEvent^ Bar in _SelectedBars)
		{
			if (_SelectedBars->IndexOf(Bar) == 0) {
				// Use SnapTickBasedOnType for the first bar
				Bar->StartTick = _Timeline->SnapTickBasedOnType(Bar->OriginalStartTick + TickDelta, mousePos);
			}
			else {
				int TickOffsetToFirstBar = Bar->OriginalStartTick - _SelectedBars[0]->OriginalStartTick;
				Bar->StartTick = _SelectedBars[0]->StartTick + TickOffsetToFirstBar;
			}
		}

		// Only update target track if not over header area and not multi-track selection
		if (!IsMultiTrackSelection && mousePos.X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			_DragTargetTrack = _Timeline->GetTrackAtPoint(mousePos);

			if ((_DragTargetTrack != nullptr) && (_DragTargetTrack != _DragSourceTrack) && !HasOverlappingBarsOnSpecificTrack(_DragTargetTrack, _SelectedBars))
			{
				for each (BarEvent^ Bar in _SelectedBars)
				{
					_DragSourceTrack->RemoveBar(Bar);
					_DragTargetTrack->AddBar(Bar);
				}

				_DragSourceTrack = _DragTargetTrack;
			}
		}

		// Restore original positions if there's an overlap
		if (HasOverlappingBarsOnBarTrack(_SelectedBars))
		{
			for (int i = 0; i < _SelectedBars->Count; i++) {
				_SelectedBars[i]->StartTick = _OriginalBarStartTicks[i];
			}
		}

		_OriginalBarStartTicks->Clear();
	}

	void PointerTool::FinishMoving(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (TargetTrack == nullptr) {
			CancelMoving();
			return;
		}

		// Don't allow track changes for multi-track selection
		if (!IsMultiTrackSelection)
		{
			if (TargetTrack != _DragSourceTrack)
			{
				// First remove bars from source track,  then add them to target track
				for each (BarEvent ^ Bar in _SelectedBars)
				{
					_DragSourceTrack->RemoveBar(Bar);
					TargetTrack->AddBar(Bar);
				}
			}
		}
		else
		{
			// For multi-track selection, just update positions if mouse button has been release inside the track content area
			for each (BarEvent ^ Bar in _SelectedBars) {
				Bar->StartTick = _Timeline->SnapTickToGrid(Bar->StartTick);
			}
		}

		_IsDragging = false;
		_DragSourceTrack = nullptr;
		_DragTargetTrack = nullptr;
	}

	void PointerTool::CancelMoving()
	{
		// Restore original tick position and containing track
		for each (BarEvent^ Bar in _SelectedBars) {
			Bar->StartTick = Bar->OriginalStartTick;
			Bar->ContainingTrack = Bar->OriginalContainingTrack;
		}
		
		_IsDragging = false;
		_DragSourceTrack = nullptr;
		_DragTargetTrack = nullptr;
		_OriginalBarStartTicks->Clear();
	}

	void PointerTool::StartSelection(Point start)
	{
		_SelectionRect = Rectangle(start.X, start.Y, 0, 0);
		_SelectedBars->Clear();

		_IsSelecting = true;
	}

	void PointerTool::UpdateSelection(Point current)
	{
		if (_DragStart == nullptr) {
			return;
		}

		// Calculate rectangle from start point to current point
		int X		= Math::Min(_DragStart->X, current.X);
		int Y		= Math::Min(_DragStart->Y, current.Y);
		int Width	= Math::Abs(current.X - _DragStart->X);
		int Height	= Math::Abs(current.Y - _DragStart->Y);

		_SelectionRect = Rectangle(X, Y, Width, Height);

		// Update selection if dragging
		if (Width > 0 && Height > 0) {
			SelectBarsInRegion(_SelectionRect);
		}
	}

	void PointerTool::EndSelection()
	{
		// Clear the selection rectangle
		_SelectionRect = Rectangle(0, 0, 0, 0);

		_IsSelecting = false;
	}

	void PointerTool::SelectBarsInRegion(Rectangle region)
	{
		_SelectedBars->Clear();

		// Convert selection rectangle to tick range
		int StartTick = _Timeline->PixelsToTicks(region.Left - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int EndTick = _Timeline->PixelsToTicks(region.Right - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track^ track in _Timeline->Tracks)
		{
			Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(track);

			if (TrackContentBounds.IntersectsWith(region))
			{
				for each (BarEvent^ bar in track->Events)
				{
					// Check if bar intersects with selection
					if (bar->EndTick >= StartTick && bar->StartTick <= EndTick)
					{
						_SelectedBars->Add(bar);
					}
				}
			}
		}
	}

	void PointerTool::HandleCopy()
	{
		if (_SelectedBars == nullptr || _SelectedBars->Count == 0) {
			return;
		}

		// Clear existing clipboard content
		TimelineClipboardManager::Clear();

		// Find the earliest start tick among selected bars
		int EarliestTick = Int32::MaxValue;

		for each(BarEvent^ Bar in _SelectedBars) {
			EarliestTick = Math::Min(EarliestTick, Bar->StartTick);
		}

		// Create copies of selected bars with relative positions and track information
		for each(Track^ Track in _Timeline->Tracks)
		{
			for each(BarEvent^ Bar in Track->Events)
			{
				if (_SelectedBars->Contains(Bar))
				{
					BarEvent^ CopiedBar = CreateBarCopy(Bar, Bar->StartTick - EarliestTick, false);
					TimelineClipboardManager::Content->Add(CopiedBar);
				}
			}
		}
	}

	void PointerTool::StartPaste()
	{
		if (TimelineClipboardManager::Content->Count == 0) {
			return;
		}

		// Create preview bars
		_PastePreviewBars = gcnew List<BarEvent^>();

		// Calculate paste position based on mouse position
		int MouseTick = _Timeline->PixelsToTicks(_LastMousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		Track^ TargetTrack = nullptr;
		bool CopiedBarsFroMultipleTracks = IsMultiTrackList(TimelineClipboardManager::Content);

		if (!CopiedBarsFroMultipleTracks && _LastMousePos.Y > Timeline_Direct2DRenderer::HEADER_HEIGHT) {
			TargetTrack = _Timeline->GetTrackAtPoint(_LastMousePos);
		}

		// Create preview bars
		for each(BarEvent^ CopiedBar in TimelineClipboardManager::Content)
		{
			int BarTick = _Timeline->SnapTickBasedOnType(MouseTick, _LastMousePos);
			
			if (_PastePreviewBars->Count > 0) {
				BarTick = _PastePreviewBars[0]->StartTick + CopiedBar->OriginalStartTick;
			}

			BarEvent^ PasteBar = CreateBarCopy(CopiedBar, BarTick, true);
			PasteBar->OriginalStartTick = CopiedBar->OriginalStartTick;

			if (!CopiedBarsFroMultipleTracks && TargetTrack != nullptr) {
				PasteBar->ContainingTrack = TargetTrack;
			}

			_PastePreviewBars->Add(PasteBar);
		}

		// Enter paste mode
		_IsPasting = true;

		// Clear existing selection
		_SelectedBars->Clear();

		// Update cursor
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

		_Timeline->Invalidate();
	}

	void PointerTool::UpdatePaste(Point mousePos)
	{
		if (!_IsPasting || _PastePreviewBars == nullptr || _PastePreviewBars->Count == 0) {
			return;
		}

		int MouseTick = _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		
		Track^ TargetTrack = nullptr;
		bool PastedBarsFromMultipleTracks = IsMultiTrackList(_PastePreviewBars);

		if(!PastedBarsFromMultipleTracks && mousePos.Y > Timeline_Direct2DRenderer::HEADER_HEIGHT) {
			TargetTrack = _Timeline->GetTrackAtPoint(mousePos);
		}

		// Store original positions
		for each (BarEvent^ bar in _PastePreviewBars) {
			_OriginalBarStartTicks->Add(bar->StartTick);
		}

		for each(BarEvent^ PasteBar in _PastePreviewBars)
		{
			if (_PastePreviewBars->IndexOf(PasteBar) == 0) {
				// Use SnapTickBasedOnType for the first bar
				PasteBar->StartTick = _Timeline->SnapTickBasedOnType(MouseTick, mousePos);
			}
			else {
				int TickOffsetToFirstBar = PasteBar->OriginalStartTick - _PastePreviewBars[0]->OriginalStartTick;
				PasteBar->StartTick = _PastePreviewBars[0]->StartTick + TickOffsetToFirstBar;
			}
		}

		if (!PastedBarsFromMultipleTracks && !HasOverlappingBarsOnSpecificTrack(TargetTrack, _PastePreviewBars) && TargetTrack != nullptr) {
			for each(BarEvent^ PasteBar in _PastePreviewBars) {
				PasteBar->ContainingTrack = TargetTrack;
			}
		}

		if (HasOverlappingBarsOnBarTrack(_PastePreviewBars)) {
			for (int i = 0; i < _PastePreviewBars->Count; i++) {
				_PastePreviewBars[i]->StartTick = _OriginalBarStartTicks[i];
			}
		}

		_OriginalBarStartTicks->Clear();
	}

	void PointerTool::FinishPaste()
	{
		if (!_IsPasting && _PastePreviewBars != nullptr && _PastePreviewBars->Count > 0) {
			CancelPaste();
			return;
		}

		if (!HasOverlappingBarsOnBarTrack(_PastePreviewBars)) {
			// Clear existing selection
			_SelectedBars->Clear();

			for each(BarEvent^ PasteBar in _PastePreviewBars)
			{
				PasteBar->ContainingTrack->AddBar(PasteBar);
			}

			// Exit paste mode
			CancelPaste();
		}
	}

	void PointerTool::CancelPaste()
	{
		if (_PastePreviewBars != nullptr) {
			_PastePreviewBars->Clear();
		}
		
		_IsPasting = false;
		_PastePreviewBars = nullptr;
		_PasteTargetTrack = nullptr;
		_OriginalBarStartTicks->Clear();
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
	}

	bool PointerTool::IsMultiTrackList(List<BarEvent^>^ list)
	{
		if (list == nullptr || list->Count <= 1) {
			return false;
		}

		Track^ FirstTrack = nullptr;

		for each(BarEvent^ Bar in list)
		{
			if (FirstTrack == nullptr) {
				FirstTrack = Bar->ContainingTrack;
			}
			else if (Bar->ContainingTrack != FirstTrack) {
				return true;
			}
		}

		return false;
	}

	void PointerTool::StoreOriginalPositions()
	{
		for each (BarEvent^ bar in _SelectedBars) {
			bar->OriginalStartTick			= bar->StartTick;
			bar->OriginalContainingTrack	= bar->ContainingTrack;
		}
	}

	void PointerTool::UpdateGroupPosition(int tickDelta, bool allowTrackChange)
	{
		for each (BarEvent ^ bar in _SelectedBars) {
			int newPosition = bar->OriginalStartTick + tickDelta;
			bar->StartTick = _Timeline->SnapTickToGrid(newPosition);
		}
	}

	void PointerTool::MoveSelectedBarsToTrack(Track^ targetTrack)
	{
		// First, remove bars from source track
		for each (BarEvent^ Bar in _SelectedBars) {
			_DragSourceTrack->Events->Remove(Bar);
		}

		// Add bars to target track
		for each (BarEvent^ Bar in _SelectedBars) {
			targetTrack->Events->Add(Bar);
		}

		// Resort the events in the target track
		targetTrack->Events->Sort(Track::barComparer);
	}

	void PointerTool::UpdateCursor(Point mousePos)
	{
		if (mousePos.X <= Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		}
		else {
			BarEvent^ hoverBar = _Timeline->GetBarAtPoint(mousePos);
			Cursor = (hoverBar != nullptr) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		}
	}

	void PointerTool::EraseSelectedBars()
	{
		if (_SelectedBars == nullptr || _SelectedBars->Count == 0) {
			return;
		}

		for each(BarEvent ^ bar in _SelectedBars)
		{
			Track^ track = nullptr;
			// Find the track containing this bar
			for each(Track ^ t in _Timeline->Tracks)
			{
				if (t->Events->Contains(bar)) {
					track = t;
					break;
				}
			}
			if (track != nullptr) {
				track->RemoveBar(bar);
			}
		}

		_SelectedBars->Clear();

		_Timeline->Invalidate();
		this->UpdateCursor(_LastMousePos);
	}

	BarEvent^ PointerTool::CreateBarCopy(BarEvent^ sourceBar, int startTick, bool isPreview)
	{
		BarEvent^ CopiedBar = nullptr;

		// Create appropriate bar based on type
		switch (sourceBar->Type)
		{
			case BarEventType::Solid:
				CopiedBar = gcnew BarEvent(
					sourceBar->ContainingTrack,
					startTick,
					sourceBar->Duration,
					isPreview ? Color::FromArgb(180, sourceBar->Color) : sourceBar->Color);
				break;

			case BarEventType::Fade:
			{
				// Deep copy the Fade Info
				BarEventFadeInfo^ OriginalFade = sourceBar->FadeInfo;
				BarEventFadeInfo^ CopiedFade;

				if (OriginalFade->Type == FadeType::Two_Colors) {
					CopiedFade = gcnew BarEventFadeInfo(
						OriginalFade->QuantizationTicks,
						OriginalFade->ColorStart,
						OriginalFade->ColorEnd);
				}
				else {
					CopiedFade = gcnew BarEventFadeInfo(
						OriginalFade->QuantizationTicks,
						OriginalFade->ColorStart,
						OriginalFade->ColorCenter,
						OriginalFade->ColorEnd);
				}

				CopiedBar = gcnew BarEvent(
					sourceBar->ContainingTrack,
					startTick,
					sourceBar->Duration,
					CopiedFade);
			}
			break;

			case BarEventType::Strobe:
			{
				// Deep copy the Strobe Info
				BarEventStrobeInfo^ OriginalStrobe = sourceBar->StrobeInfo;
				BarEventStrobeInfo^ CopiedStrobe = gcnew BarEventStrobeInfo(OriginalStrobe->QuantizationTicks, OriginalStrobe->ColorStrobe);

				CopiedBar = gcnew BarEvent(
					sourceBar->ContainingTrack,
					startTick,
					sourceBar->Duration,
					CopiedStrobe);
			}
			break;
		}

		return CopiedBar;
	}


	/////////////////////////////
	// DrawTool Implementation //
	/////////////////////////////
	DrawTool::DrawTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		
		_TargetTrack		= nullptr;
		_SourceTrack		= nullptr;
		_PreviewBar		= nullptr;
		_CurrentColor	= Color::FromArgb(200, 100, 100, 255);
		_DrawTickLength	= 960;
		_UseAutoLength	= false;

		_IsPainting		= false;
		_IsErasing		= false;
		_IsMoving		= false;
		_IsResizing		= false;
		_LastPaintedTick = 0;
		_HoverBar		= nullptr;
		_CurrentMode		= DrawToolMode::Draw;

		timeline->UpdateCursor(_CurrentCursor);
	}

	void DrawTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point mousePos(e->X, e->Y);
			_TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

			if (_TargetTrack != nullptr) {
				switch (_CurrentMode) {
				case DrawToolMode::Draw:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
					StartPainting(mousePos);
					break;

				case DrawToolMode::Erase:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
					StartErasing(mousePos);
					break;

				case DrawToolMode::Move:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
					StartMoving(mousePos);
					break;

				case DrawToolMode::Resize:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
					StartResizing(mousePos);
					break;
				}
			}
		}
	}

	void DrawTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);
		_LastMousePos = mousePos;

		if (e->X <= Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			ClearPreview();
			return;
		}

		if (e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			// Handle active operations
			if (e->Button == Windows::Forms::MouseButtons::Left) {
				if (_IsResizing) {
					_CurrentMode = DrawToolMode::Resize;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
					UpdateResizing(mousePos);
				}
				else if (_IsMoving) {
					_CurrentMode = DrawToolMode::Move;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
					UpdateMoving(mousePos);
				}
				else {
					switch (_CurrentMode) {
					case DrawToolMode::Draw:
						UpdatePainting(mousePos);
						break;
					case DrawToolMode::Erase:
						UpdateErasing(mousePos);
						break;
					}
				}
			}
			else {
				// Update mode and cursor for hover state
				UpdateCurrentMode(mousePos);
				UpdatePreview(mousePos);
			}
		}
	}

	void DrawTool::OnMouseUp(MouseEventArgs^ e)
	{
		switch (_CurrentMode)
		{
		case DrawToolMode::Move:
			FinishMoving(Point(e->X, e->Y));
			break;

		case DrawToolMode::Resize:
			FinishResizing();
			break;
		}

		_IsPainting	= false;
		_IsErasing	= false;
		_IsMoving	= false;
		_IsResizing	= false;

		UpdateCurrentMode(Point(e->X, e->Y));
	}

	void DrawTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Escape) {
			if (_IsMoving) { 
				CancelMoving();
				_Timeline->Invalidate();
			}
			else if (_IsResizing) {
				CancelResizing();
				_Timeline->Invalidate();
			}
		}
	}

	void DrawTool::OnKeyUp(KeyEventArgs^ e) {}

	void DrawTool::UpdatePreview(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (TargetTrack != nullptr && mousePos.X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			int Raw_Tick		= _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
			int Start_Tick		= _Timeline->SnapTickBasedOnType(Raw_Tick, mousePos);
			int Length_In_Ticks = _DrawTickLength;

			if (_UseAutoLength) {
				Length_In_Ticks = GetBeatLength(TargetTrack, Raw_Tick);
			}

			switch (_CurrentMode)
			{
				case DrawToolMode::Draw:
					if (!HasOverlappingBars(TargetTrack, Start_Tick, Length_In_Ticks))
					{
						if (_PreviewBar == nullptr)
						{
							_PreviewBar = gcnew BarEvent(nullptr, Start_Tick, Length_In_Ticks, Color::FromArgb(100, (int)_CurrentColor.R, (int)_CurrentColor.G, (int)_CurrentColor.B));
						}
						else {
							_PreviewBar->StartTick	= Start_Tick;
							_PreviewBar->Duration	= Length_In_Ticks;
						}
					}
					else {
						_PreviewBar = nullptr;
					}
					break;

				case DrawToolMode::Move:
				case DrawToolMode::Erase:
				case DrawToolMode::Resize:
					_PreviewBar = nullptr;
					break;
			}

			_Timeline->Invalidate();
		}
		else {
			ClearPreview();
		}
	}

	void DrawTool::ClearPreview()
	{
		_PreviewBar = nullptr;
		_Timeline->Invalidate();
	}

	void DrawTool::UpdateCurrentMode(Point mousePos)
	{
		if (_IsResizing || _IsMoving || _IsPainting) {
			UpdateActiveModesCursor();
			return;
		}

		Track^ track = _Timeline->GetTrackAtPoint(mousePos);
		BarEvent^ bar = nullptr;

		if (track != nullptr) {
			bar = _Timeline->GetBarAtPoint(mousePos);
		}

		_HoverBar = bar;

		// Determine new mode based on context
		DrawToolMode newMode;
		System::Windows::Forms::Cursor^ newCursor;

		if (bar != nullptr) {
			// Check resize handle first - this should take priority
			if (IsOverResizeHandle(mousePos, bar, track)) {
				newMode = DrawToolMode::Resize;
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
			}
			else if (Control::ModifierKeys == Keys::Control) {
				newMode = DrawToolMode::Move;
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
			}
			else {
				newMode = DrawToolMode::Erase;
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
		}
		else {
			newMode = DrawToolMode::Draw;
			if (!HasOverlappingBars(track,
				_Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X),
				_DrawTickLength)) {
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);	// For drawing (when position is valid)
			}
			else {
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);	// For drawing (when position would cause overlap)
			}
		}

		// Only update mode if it changed
		if (newMode != _CurrentMode || Cursor != newCursor) {
			_CurrentMode = newMode;
			Cursor = newCursor;
			_Timeline->Invalidate();	// Refresh preview
		}
	}

	void DrawTool::UpdateActiveModesCursor()
	{
		switch (_CurrentMode)
		{
		case DrawToolMode::Draw:
			this->Cursor = _IsPainting ? TimelineCursorHelper::GetCursor(TimelineCursor::Cross) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			break;
		case DrawToolMode::Move:
			this->Cursor = _IsMoving ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			break;
		case DrawToolMode::Resize:
			this->Cursor = _IsResizing ? TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			break;
		case DrawToolMode::Erase:
			this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			break;
		}
	}

	bool DrawTool::IsOverResizeHandle(Point mousePos, BarEvent^ bar, Track^ track)
	{
		if (bar == nullptr || track == nullptr) return false;

		Rectangle bounds = _Timeline->GetTrackContentBounds(track);

		// Calculate bar end position in pixels
		int barEndX = _Timeline->TicksToPixels(bar->StartTick + bar->Duration) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

		// Define handle region - make it a bit larger for easier interaction
		const int HANDLE_WIDTH = 5;

		// Check if mouse is within handle area
		bool isInXRange = mousePos.X >= barEndX - HANDLE_WIDTH && mousePos.X <= barEndX + HANDLE_WIDTH;

		bool isInYRange = mousePos.Y >= bounds.Y + Timeline_Direct2DRenderer::TRACK_PADDING && mousePos.Y <= bounds.Bottom - Timeline_Direct2DRenderer::TRACK_PADDING;

		return isInXRange && isInYRange;
	}

	void DrawTool::StartPainting(Point mousePos)
	{
		if (_TargetTrack == nullptr) {
			return;
		}
		
		_IsPainting = true;
		_CurrentMode = DrawToolMode::Draw;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

		int Raw_Tick		= _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int Start_Tick		= _Timeline->SnapTickBasedOnType(Raw_Tick, mousePos);
		int Length_In_Ticks = _DrawTickLength;

		if (_UseAutoLength) {
			Length_In_Ticks = GetBeatLength(_TargetTrack, Raw_Tick);
		}

		_LastPaintedTick = Start_Tick;

		// Create initial bar
		if (!HasOverlappingBars(_TargetTrack, Start_Tick, Length_In_Ticks)) {
			_Timeline->AddBarToTrack(_TargetTrack, Start_Tick, Length_In_Ticks, _CurrentColor);
			_Timeline->Invalidate();
		}
	}

	void DrawTool::UpdatePainting(Point mousePos)
	{
		if (!_IsPainting || _TargetTrack == nullptr) {
			return;
		}

		int Raw_Tick		= _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int Start_Tick		= _Timeline->SnapTickBasedOnType(Raw_Tick, mousePos);
		int Length_In_Ticks = _DrawTickLength;

		if (_UseAutoLength) {
			Length_In_Ticks = GetBeatLength(_TargetTrack, Raw_Tick);
		}

		// Only create new bar if we've moved to a new position and there's no overlap
		if (Start_Tick > _LastPaintedTick)
		{
			if (!HasOverlappingBars(_TargetTrack, Start_Tick, Length_In_Ticks))
			{
				_Timeline->AddBarToTrack(_TargetTrack, Start_Tick, Length_In_Ticks, _CurrentColor);
				_LastPaintedTick = Start_Tick;
				_Timeline->Invalidate();
			}
		}
	}

	bool DrawTool::HasOverlappingBars(Track^ track, int startTick, int length)
	{
		if (track == nullptr) return false;

		int endTick = startTick + length;

		for each(BarEvent ^ existingBar in track->Events) {
			// Skip checking against the bar we're currently moving
			if (existingBar == _HoverBar) continue;

			int existingEnd = existingBar->StartTick + existingBar->Duration;

			// Check for any overlap
			if (startTick < existingEnd && endTick > existingBar->StartTick) {
				return true;
			}
		}

		return false;
	}

	void DrawTool::StartErasing(Point mousePos)
	{
		_IsErasing = true; 
		if (_HoverBar != nullptr) {
			_TargetTrack->RemoveBar(_HoverBar);
			_Timeline->Invalidate();
		}
	}

	void DrawTool::UpdateErasing(Point mousePos)
	{
		if (!_IsErasing) return;

		Track^ track = _Timeline->GetTrackAtPoint(mousePos);
		if (track != nullptr) {
			// Get the bar under the mouse cursor
			BarEvent^ bar = _Timeline->GetBarAtPoint(mousePos);

			// Only erase if we find a bar and haven't already erased it
			if (bar != nullptr) {
				track->RemoveBar(bar);
				_Timeline->Invalidate();
			}
		}
	}

	void DrawTool::StartMoving(Point mousePos)
	{
		if (_HoverBar != nullptr) {
			_IsMoving = true;
			_CurrentMode = DrawToolMode::Move;
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
			_DragStartPoint = mousePos;
			_DragStartTick = _HoverBar->StartTick;
			_TargetTrack = _Timeline->GetTrackAtPoint(mousePos);
			_SourceTrack = _Timeline->GetTrackAtPoint(mousePos);

			_HoverBar->OriginalStartTick = _HoverBar->StartTick;
			_HoverBar->OriginalContainingTrack = _HoverBar->ContainingTrack;
		}
	}

	void DrawTool::UpdateMoving(Point mousePos)
	{
		if (!_IsMoving || _HoverBar == nullptr || _TargetTrack == nullptr) {
			return;
		}

		// Calculate movement in ticks
		int PixelDelta = mousePos.X - _DragStartPoint.X;
		int TickDelta = _Timeline->PixelsToTicks(PixelDelta);

		// Calculate new position with grid snapping
		int NewStartTick = _Timeline->SnapTickToGrid(_DragStartTick + TickDelta);

		// Store original position for overlap check
		int OriginalStart = _HoverBar->StartTick;

		// Temporarily move to new position
		_HoverBar->StartTick = NewStartTick;

		// Check for overlaps with other bars
		bool HasOverlap = HasOverlappingBars(_TargetTrack, NewStartTick, _HoverBar->Duration);

		if (HasOverlap) {
			// If overlap detected, revert to original position
			_HoverBar->StartTick = OriginalStart;
		}
		else {
			// If no overlap, check if we're over a different track
			Track^ TrackUnderMouse = _Timeline->GetTrackAtPoint(mousePos);

			if (TrackUnderMouse != nullptr && TrackUnderMouse != _TargetTrack) {
				// Ensure the new track doesn't have overlapping bars
				if (!HasOverlappingBars(TrackUnderMouse, NewStartTick, _HoverBar->Duration))
				{
					_TargetTrack->RemoveBar(_HoverBar);
					TrackUnderMouse->AddBar(_HoverBar);

					_TargetTrack = TrackUnderMouse;
				}
				else {
					// If overlap in new track, revert position
					_HoverBar->StartTick = OriginalStart;
				}
			}
		}

		_Timeline->Invalidate();
	}

	void DrawTool::FinishMoving(Point mousePos)
	{
		Track^ FinalTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (FinalTrack == nullptr) {
			CancelMoving();
			return;
		}
		
		if (_IsMoving && _HoverBar != nullptr)
		{
			// Ensure final position is properly snapped
			_HoverBar->StartTick = _Timeline->SnapTickToGrid(_HoverBar->StartTick);
			
			_HoverBar->OriginalStartTick = _HoverBar->StartTick;
			_HoverBar->OriginalContainingTrack = _HoverBar->ContainingTrack;

			// Resort the events in the final track
			if (_TargetTrack != nullptr) {
				_TargetTrack->Events->Sort(Track::barComparer);
			}
		}

		_IsMoving = false;
		_HoverBar = nullptr;
		_TargetTrack = nullptr;
		_SourceTrack = nullptr;
	}

	void DrawTool::CancelMoving()
	{
		if (_IsMoving && _HoverBar != nullptr)
		{
			_HoverBar->StartTick = _HoverBar->OriginalStartTick;
			_HoverBar->ContainingTrack = _HoverBar->OriginalContainingTrack;
		}
		
		_IsMoving = false;
		_HoverBar = nullptr;
		_TargetTrack = nullptr;
		_SourceTrack = nullptr;
	}

	void DrawTool::StartResizing(Point mousePos)
	{
		if (_HoverBar != nullptr) {
			_IsResizing = true;
			_CurrentMode = DrawToolMode::Resize;
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
			_DragStartPoint = mousePos;
			_DragStartTick = _HoverBar->Duration;
			_TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

			HoverBar->OriginalDuration = HoverBar->Duration;
		}
	}

	void DrawTool::UpdateResizing(Point mousePos)
	{
		if (!_IsResizing || _HoverBar == nullptr || _TargetTrack == nullptr) {
			return;
		}

		// Calculate mouse position in ticks
		int MouseTickPosition = _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Calculate new length (from bar start to mouse position)
		int NewLength = MouseTickPosition - _HoverBar->StartTick;

		// Snap the length to grid
		int SnappedLength = _Timeline->SnapTickToGrid(NewLength);

		// Ensure minimum length (e.g., 1/32 note)
		const int MIN_LENGTH = 120; // 1/32 note
		SnappedLength = Math::Max(SnappedLength, MIN_LENGTH);

		// Check for overlaps with other bars
		bool HasOverlap = false;
		for each(BarEvent^ existingBar in _TargetTrack->Events)
		{
			if (existingBar != _HoverBar && existingBar->StartTick < (_HoverBar->StartTick + SnappedLength) && (existingBar->StartTick + existingBar->Duration) > _HoverBar->StartTick)
			{
				HasOverlap = true;
				break;
			}
		}

		if (!HasOverlap) {
			_HoverBar->Duration = SnappedLength;
			_Timeline->Invalidate();
		}
	}

	void DrawTool::FinishResizing()
	{
		if (_IsResizing && _HoverBar != nullptr)
		{
			_HoverBar->OriginalDuration = _HoverBar->Duration;
		}
		
		_IsResizing = false;
		_HoverBar = nullptr;
		_TargetTrack = nullptr;
	}

	void DrawTool::CancelResizing()
	{
		if (_IsResizing && _HoverBar != nullptr)
		{
			_HoverBar->Duration = _HoverBar->OriginalDuration;
		}
		
		_IsResizing = false;
		_HoverBar = nullptr;
		_TargetTrack = nullptr;
	}

	int DrawTool::GetBeatLength(Track^ track, int currentTick)
	{
		// Set default length
		int length = _DrawTickLength;

		// If track doesn't have tablature or measures, return default
		if (!track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0) {
			return length;
		}

		// Find the current measure
		Measure^ currentMeasure = _Timeline->GetMeasureAtTick(currentTick);
		if (currentMeasure == nullptr) {
			return length;
		}

		// Find the corresponding track measure
		TrackMeasure^ trackMeasure = nullptr;
		for each(TrackMeasure ^ measure in track->Measures)
		{
			if (measure->StartTick == currentMeasure->StartTick)
			{
				trackMeasure = measure;
				break;
			}
		}

		if (trackMeasure == nullptr || trackMeasure->Beats == nullptr || trackMeasure->Beats->Count == 0) {
			return length;
		}

		// Find the last beat before current tick in this measure
		Beat^ lastBeat = nullptr;
		for each(Beat ^ beat in trackMeasure->Beats)
		{
			// Skip beats without notes
			if (beat->Notes == nullptr || beat->Notes->Count == 0) {
				continue;
			}

			// Skip beats after our cursor position
			if (beat->StartTick > currentTick) {
				break;
			}

			// Found a valid beat before our position
			lastBeat = beat;
		}

		// If we found a valid beat before our position, use its duration
		if (lastBeat != nullptr)
		{
			length = lastBeat->Duration;
		}

		return length;
	}

	void DrawTool::DrawColor::set(Color value)
	{
		_CurrentColor = value;
		
		if (_PreviewBar != nullptr)
		{
			_PreviewBar->Color = Color::FromArgb(100, (int)value.R, (int)value.G, (int)value.B);
			_Timeline->Invalidate();
		}
	}

	void DrawTool::DrawTickLength::set(int value)
	{
		_DrawTickLength = value;

		if (_PreviewBar != nullptr)
		{
			_PreviewBar->Duration = value;
			_Timeline->Invalidate();
		}
	}


	//////////////////////////////
	// SplitTool Implementation //
	//////////////////////////////
	SplitTool::SplitTool(Widget_Timeline^ timeline) : TimelineTool(timeline) {}

	void SplitTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
			Track^ track = _Timeline->GetTrackAtPoint(Point(e->X, e->Y));
			if (track != nullptr) {
				BarEvent^ bar = _Timeline->GetBarAtPoint(Point(e->X, e->Y));
				if (bar != nullptr) {
					// Calculate split point
					int splitTick = _Timeline->SnapTickToGrid(
						_Timeline->PixelsToTicks(e->X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));

					if (splitTick > bar->StartTick && splitTick < bar->StartTick + bar->Duration) {
						// Create two new bars
						int firstLength = splitTick - bar->StartTick;
						int secondLength = bar->Duration - firstLength;

						_Timeline->AddBarToTrack(track, bar->StartTick, firstLength, bar->Color);
						_Timeline->AddBarToTrack(track, splitTick, secondLength, bar->Color);

						// Remove original bar
						track->RemoveBar(bar);
						_Timeline->Invalidate();
					}
				}
			}
		}
	}

	void SplitTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);
		Track^ track = _Timeline->GetTrackAtPoint(mousePos);

		if (track != nullptr && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
			BarEvent^ bar = _Timeline->GetBarAtPoint(mousePos);
			if (bar != nullptr) {
				hoverTrack = track;
				hoverBar = bar;

				splitPreviewTick = _Timeline->SnapTickToGrid(
					_Timeline->PixelsToTicks(e->X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH -
						_Timeline->ScrollPosition->X));

				if (splitPreviewTick > bar->StartTick &&
					splitPreviewTick < bar->StartTick + bar->Duration) {
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::HSplit);
				}
				else {
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
				}

				_Timeline->Invalidate();
			}
			else {
				ClearSplitPreview();
			}
		}
	}

	void SplitTool::OnMouseUp(MouseEventArgs^ e) {}
	void SplitTool::OnKeyDown(KeyEventArgs^ e) {}
	void SplitTool::OnKeyUp(KeyEventArgs^ e) {}

	void SplitTool::UpdateSplitPreview(Point mousePos)
	{
		if (hoverBar != nullptr) {
			// Calculate split position
			splitPreviewTick = _Timeline->SnapTickToGrid(
				_Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH -
					_Timeline->ScrollPosition->X));

			// Only show preview if split position is valid
			if (splitPreviewTick > hoverBar->StartTick &&
				splitPreviewTick < hoverBar->StartTick + hoverBar->Duration) {
				// The actual preview drawing is handled in the widget's paint routine
				_Timeline->Invalidate();
			}
		}
	}

	void SplitTool::ClearSplitPreview()
	{
		splitPreviewTick	= -1;
		hoverTrack			= nullptr;
		hoverBar			= nullptr;
		_Timeline->Invalidate();
	}

	//////////////////////////////
	// EraseTool Implementation //
	//////////////////////////////
	EraseTool::EraseTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor				= TimelineCursorHelper::GetCursor(TimelineCursor::No);
		_IsErasing			= false;
		_IsSelecting			= false;
		_SelectionStart		= nullptr;
		_SelectedBars		= gcnew List<BarEvent^>();
		_ErasedBars			= gcnew List<BarEvent^>();
		_HoverBar			= nullptr;
		_HoverTrack			= nullptr;
		_SelectionRect		= Rectangle(0, 0, 0, 0);
		_ErasePreviewRect	= Rectangle(0, 0, 0, 0);
	}

	void EraseTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point clickPoint(e->X, e->Y);
			BarEvent^ clickedBar = _Timeline->GetBarAtPoint(clickPoint);

			// If clicking on a selected bar, delete all selected bars
			if (clickedBar != nullptr && _SelectedBars->Contains(clickedBar))
			{
				StartErasing();
				// Erase all selected bars
				for each(BarEvent ^ bar in _SelectedBars)
				{
					Track^ track = nullptr;
					// Find the track containing this bar
					for each(Track ^ t in _Timeline->Tracks)
					{
						if (t->Events->Contains(bar)) {
							track = t;
							break;
						}
					}
					if (track != nullptr) {
						track->RemoveBar(bar);
						_ErasedBars->Add(bar);
					}
				}
				ClearSelection();
				EndErasing();
			}
			// If clicking on an unselected bar, clear selection and select only this bar
			else if (clickedBar != nullptr) {
				ClearSelection();
				_SelectedBars->Add(clickedBar);
				UpdateHoverPreview(clickPoint);
			}
			// If clicking empty space, start selection rectangle
			else {
				ClearSelection();
				StartSelection(clickPoint);
			}

			_Timeline->Invalidate();
		}
	}

	void EraseTool::OnMouseMove(MouseEventArgs^ e) {
		Point mousePos(e->X, e->Y);

		if (_IsSelecting) {
			UpdateSelection(mousePos);
		}
		else {
			UpdateHoverPreview(mousePos);
		}

		_Timeline->Invalidate();
	}

	void EraseTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsSelecting) {
			EndSelection();
		}
		UpdateHoverPreview(Point(e->X, e->Y));
	}

	void EraseTool::OnKeyDown(KeyEventArgs^ e) { }
	void EraseTool::OnKeyUp(KeyEventArgs^ e) { }

	void EraseTool::StartSelection(Point start)
	{
		_IsSelecting = true;
		_SelectionStart = start;
		_SelectionRect = Rectangle(start.X, start.Y, 0, 0);
	}

	void EraseTool::UpdateSelection(Point current)
	{
		if (!_IsSelecting || _SelectionStart == nullptr) return;

		// Calculate selection rectangle
		int x = Math::Min(_SelectionStart->X, current.X);
		int y = Math::Min(_SelectionStart->Y, current.Y);
		int width = Math::Abs(current.X - _SelectionStart->X);
		int height = Math::Abs(current.Y - _SelectionStart->Y);

		_SelectionRect = Rectangle(x, y, width, height);

		// Update selected bars
		SelectBarsInRegion(_SelectionRect);
	}

	void EraseTool::EndSelection()
	{
		_IsSelecting = false;
		_SelectionStart = nullptr;
		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
	}

	void EraseTool::SelectBarsInRegion(Rectangle region)
	{
		_SelectedBars->Clear();

		// Convert selection rectangle to tick range
		int startTick = _Timeline->PixelsToTicks(
			region.Left - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int endTick = _Timeline->PixelsToTicks(
			region.Right - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each(Track ^ track in _Timeline->Tracks) {
			Rectangle trackBounds = _Timeline->GetTrackContentBounds(track);
			if (trackBounds.IntersectsWith(region)) {
				for each(BarEvent ^ bar in track->Events) {
					// Check if bar intersects with selection
					if (bar->StartTick + bar->Duration >= startTick &&
						bar->StartTick <= endTick) {
						_SelectedBars->Add(bar);
					}
				}
			}
		}
	}

	void EraseTool::ClearSelection()
	{
		_SelectedBars->Clear();
		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
	}

	void EraseTool::StartErasing()
	{
		_IsErasing = true;
		if (_ErasedBars == nullptr) {
			_ErasedBars = gcnew List<BarEvent^>();
		}
		else {
			_ErasedBars->Clear();
		}
	}

	void EraseTool::UpdateErasing(Point mousePos)
	{
		Track^ track = _Timeline->GetTrackAtPoint(mousePos);
		if (track != nullptr)
		{
			BarEvent^ bar = _Timeline->GetBarAtPoint(mousePos);
			if (bar != nullptr && !_ErasedBars->Contains(bar))
			{
				track->RemoveBar(bar);
				_ErasedBars->Add(bar);
				_Timeline->Invalidate();
			}
		}
	}

	void EraseTool::EndErasing()
	{
		_IsErasing = false;
		_ErasedBars->Clear();
	}

	void EraseTool::UpdateHoverPreview(Point mousePos)
	{
		Track^ track = _Timeline->GetTrackAtPoint(mousePos);
		BarEvent^ bar = nullptr;

		if (track != nullptr && mousePos.X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
			bar = _Timeline->GetBarAtPoint(mousePos);
		}

		// Update hover state
		if (bar != _HoverBar || track != _HoverTrack) {
			_HoverBar = bar;
			_HoverTrack = track;

			if (bar != nullptr) {
				// Calculate preview rectangle
				int x = _Timeline->TicksToPixels(bar->StartTick) +
					_Timeline->ScrollPosition->X +
					Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

				Rectangle trackBounds = _Timeline->GetTrackContentBounds(track);

				_ErasePreviewRect = Rectangle(
					x,
					trackBounds.Y + Timeline_Direct2DRenderer::TRACK_PADDING,
					_Timeline->TicksToPixels(bar->Duration),
					trackBounds.Height - (Timeline_Direct2DRenderer::TRACK_PADDING * 2)
				);

				// Update cursor based on whether the bar is selected
				Cursor = _SelectedBars->Contains(bar) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else {
				_ErasePreviewRect = Rectangle(0, 0, 0, 0);
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross); // Use cross cursor for selection
			}

			_Timeline->Invalidate();
		}
	}

	void EraseTool::ClearHoverPreview()
	{
		if (_HoverBar != nullptr || !_ErasePreviewRect.IsEmpty) {
			_HoverBar = nullptr;
			_HoverTrack = nullptr;
			_ErasePreviewRect = Rectangle(0, 0, 0, 0);
			_Timeline->Invalidate();
		}
	}

	void EraseTool::Activate() 
	{
		_IsActive = true;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
	}

	/////////////////////////////////
	// DurationTool Implementation //
	/////////////////////////////////
	DurationTool::DurationTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
		_IsDragging			= false;
		_IsShowingPreview	= false;
		_IsSelecting			= false;
		_TargetBar			= nullptr;
		_TargetTrack			= nullptr;
		_OriginalLength		= 0;
		_DragStartX			= 0;
		_PreviewLength		= 0;
		_ChangeTickLength	= Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		_SelectedBars		= gcnew List<BarEvent^>();
		_OriginalLengths	= gcnew Dictionary<BarEvent^, int>();
		_SelectionStart		= nullptr;
		_SelectionRect		= Rectangle(0, 0, 0, 0);
	}

	void DurationTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point mousePos(e->X, e->Y);
			Track^ track = _Timeline->GetTrackAtPoint(mousePos);
			BarEvent^ bar = _Timeline->GetBarAtPoint(mousePos);

			if (track != nullptr && bar != nullptr)
			{
				// If clicking on a handle of a selected bar
				if (IsOverHandle(mousePos, bar, track) && _SelectedBars->Contains(bar))
				{
					_IsDragging = true;
					_TargetBar = bar;
					_TargetTrack = track;
					StoreOriginalLengths();
					_DragStartX = e->X;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
				}
				// If clicking on a handle of an unselected bar
				else if (IsOverHandle(mousePos, bar, track))
				{
					ClearSelection();
					_SelectedBars->Add(bar);
					_IsDragging = true;
					_TargetBar = bar;
					_TargetTrack = track;
					StoreOriginalLengths();
					_DragStartX = e->X;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
				}
				// If clicking on a bar (not handle)
				else if (!IsOverHandle(mousePos, bar, track))
				{
					ClearSelection();
					StartSelection(mousePos);
				}
			}
			else
			{
				// Clicking on empty space
				ClearSelection();
				StartSelection(mousePos);
			}
			_Timeline->Invalidate();
		}
	}

	void DurationTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);

		if (_IsDragging && _TargetBar != nullptr && _SelectedBars->Count > 0)
		{
			// Calculate length change
			int deltaPixels = e->X - _DragStartX;
			int deltaTicks = _Timeline->PixelsToTicks(deltaPixels);

			// Calculate the scale factor for the length change
			double scaleFactor = 1.0;
			if (_OriginalLengths[_TargetBar] > 0)
			{
				scaleFactor = (double)(_OriginalLengths[_TargetBar] + deltaTicks) / _OriginalLengths[_TargetBar];
			}

			// Apply length change to all selected bars
			for each (BarEvent ^ bar in _SelectedBars)
			{
				int originalLength = _OriginalLengths[bar];
				int newLength;

				if (Control::ModifierKeys == Keys::Control)
				{
					// Snap to grid
					newLength = _Timeline->SnapTickToGrid(originalLength + deltaTicks);
				}
				else
				{
					// Proportional change
					newLength = (int)(originalLength * scaleFactor);
					// Round to nearest changeTickLength
					newLength = ((newLength + _ChangeTickLength / 2) / _ChangeTickLength) * _ChangeTickLength;
				}

				// Ensure minimum length
				bar->Duration = Math::Max(newLength, MIN_LENGTH_TICKS);
			}

			_Timeline->Invalidate();
		}
		else if (_IsSelecting)
		{
			UpdateSelection(mousePos);
		}
		else
		{
			Track^ track = _Timeline->GetTrackAtPoint(mousePos);
			BarEvent^ bar = _Timeline->GetBarAtPoint(mousePos);

			if (track != nullptr && bar != nullptr)
			{
				Cursor = IsOverHandle(mousePos, bar, track) ? TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			}
			else
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			}
		}
	}

	void DurationTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsDragging)
		{
			for each (BarEvent ^ bar in _SelectedBars)
			{
				bar->OriginalDuration = bar->Duration;
			}
			
			_IsDragging = false;
			_TargetBar = nullptr;
			_TargetTrack = nullptr;
			_OriginalLengths->Clear();
		}
		else if (_IsSelecting)
		{
			EndSelection();
		}
	}

	void DurationTool::OnKeyDown(KeyEventArgs^ e) {}
	void DurationTool::OnKeyUp(KeyEventArgs^ e) {}

	bool DurationTool::IsOverHandle(Point mousePos, BarEvent^ bar, Track^ track)
	{
		if (bar == nullptr || track == nullptr) { 
			return false;
		}

		Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(track);
		int BarEndX = _Timeline->TicksToPixels(bar->StartTick + bar->Duration) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

		// Check if mouse is within handle area at end of bar
		return	mousePos.X >= BarEndX - HANDLE_WIDTH &&
				mousePos.X <= BarEndX + HANDLE_WIDTH &&
				mousePos.Y >= TrackContentBounds.Y		+ Timeline_Direct2DRenderer::TRACK_PADDING &&
				mousePos.Y <= TrackContentBounds.Bottom - Timeline_Direct2DRenderer::TRACK_PADDING;
	}

	void DurationTool::UpdateLengthPreview(Point mousePos)
	{
		if (_TargetBar == nullptr || !_IsDragging)
		{
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);
			BarEvent^ Bar = _Timeline->GetBarAtPoint(mousePos);

			if (TargetTrack != nullptr && Bar != nullptr && IsOverHandle(mousePos, Bar, TargetTrack))
			{
				if (Control::ModifierKeys == Keys::Control)
				{
					int BarEndX = _Timeline->TicksToPixels(Bar->StartTick + Bar->Duration) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

					int DeltaPixels = mousePos.X - BarEndX;
					int DeltaTicks = _Timeline->PixelsToTicks(DeltaPixels);
					_PreviewLength = _Timeline->SnapTickToGrid(Bar->Duration + DeltaTicks);
				}
				else
				{
					// Calculate total desired length from mouse position
					int BarEndX = _Timeline->TicksToPixels(Bar->StartTick + Bar->Duration) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;
					int DeltaPixels = mousePos.X - BarEndX;
					int DeltaTicks = _Timeline->PixelsToTicks(DeltaPixels);

					int TotalDesiredLength = Bar->Duration + DeltaTicks;
					int NumUnits = Math::Max(1, (TotalDesiredLength + (_ChangeTickLength / 2)) / _ChangeTickLength);
					_PreviewLength = NumUnits * _ChangeTickLength;
				}

				// Ensure minimum length
				_PreviewLength = Math::Max(_PreviewLength, MIN_LENGTH_TICKS);

				_IsShowingPreview = true;
				_TargetBar = Bar;
				_TargetTrack = TargetTrack;

				_Timeline->Invalidate();
			}
			else {
				ClearPreview();
			}
		}
	}

	void DurationTool::ClearPreview()
	{
		if (_IsShowingPreview) {
			_IsShowingPreview = false;

			if (!_IsDragging) {
				_TargetBar = nullptr;
				_TargetTrack = nullptr;
			}

			_Timeline->Invalidate();
		}
	}

	void DurationTool::StartSelection(Point start)
	{
		_IsSelecting = true;
		_SelectionStart = start;
		_SelectionRect = Rectangle(start.X, start.Y, 0, 0);
	}

	void DurationTool::UpdateSelection(Point current)
	{
		if (!_IsSelecting || _SelectionStart == nullptr) {
			return;
		}

		// Calculate selection rectangle
		int X = Math::Min(_SelectionStart->X, current.X);
		int Y = Math::Min(_SelectionStart->Y, current.Y);
		int Width = Math::Abs(current.X - _SelectionStart->X);
		int Height = Math::Abs(current.Y - _SelectionStart->Y);

		_SelectionRect = Rectangle(X, Y, Width, Height);

		// Update selected bars
		SelectBarsInRegion(_SelectionRect);
		_Timeline->Invalidate();
	}

	void DurationTool::EndSelection()
	{
		_IsSelecting = false;
		_SelectionStart = nullptr;
		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
	}

	void DurationTool::SelectBarsInRegion(Rectangle region)
	{
		_SelectedBars->Clear();

		// Convert selection rectangle to tick range
		int startTick = _Timeline->PixelsToTicks(region.Left - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int endTick = _Timeline->PixelsToTicks(region.Right - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track^ T in _Timeline->Tracks)
		{
			Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(T);
			if (TrackContentBounds.IntersectsWith(region))
			{
				for each (BarEvent^ Bar in T->Events)
				{
					if (Bar->StartTick + Bar->Duration >= startTick && Bar->StartTick <= endTick)
					{
						_SelectedBars->Add(Bar);
					}
				}
			}
		}
	}

	void DurationTool::ClearSelection()
	{
		_SelectedBars->Clear();
		_OriginalLengths->Clear();
		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
	}

	void DurationTool::StoreOriginalLengths()
	{
		_OriginalLengths->Clear();
		for each (BarEvent^ Bar in _SelectedBars)
		{
			Bar->OriginalDuration = Bar->Duration;
			_OriginalLengths->Add(Bar, Bar->Duration);
		}
	}


	//////////////////////////////
	// ColorTool Implementation //
	//////////////////////////////
	ColorTool::ColorTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor			= TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		_IsSelecting	= false;
		_SelectionStart	= nullptr;
		_SelectedBars	= gcnew List<BarEvent^>();
		_HoverBar		= nullptr;
		_HoverTrack		= nullptr;
		_SelectionRect	= Rectangle(0, 0, 0, 0);
		_PreviewRect	= Rectangle(0, 0, 0, 0);
		_CurrentColor	= Color::FromArgb(255, 100, 100, 255); // Default color
	}

	void ColorTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point ClickPoint(e->X, e->Y);

			BarEvent^ ClickedBar = _Timeline->GetBarAtPoint(ClickPoint);

			// If clicking on a selected bar, apply color to all selected bars
			if (ClickedBar != nullptr && _SelectedBars->Contains(ClickedBar)) {
				ApplyColorToSelection();
				ClearSelection();
				ClearHoverPreview();
			}
			// If clicking on an unselected bar, clear selection and select only this bar
			else if (ClickedBar != nullptr)
			{
				ClearSelection();
				_SelectedBars->Add(ClickedBar);
				UpdateHoverPreview(ClickPoint);
			}
			// If clicking empty space, start selection rectangle
			else
			{
				ClearSelection();
				StartSelection(ClickPoint);
			}

			_Timeline->Invalidate();
		}
	}

	void ColorTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);

		if (_IsSelecting) {
			UpdateSelection(MousePos);
		}
		else {
			UpdateHoverPreview(MousePos);
		}

		_Timeline->Invalidate();
	}

	void ColorTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsSelecting) {
			EndSelection();
		}

		UpdateHoverPreview(Point(e->X, e->Y));
	}

	void ColorTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Escape)
		{
			ClearSelection();
			ClearHoverPreview();

			// Update cursor and view
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
			_Timeline->Invalidate();
		}
	}
	
	void ColorTool::OnKeyUp(KeyEventArgs^ e) {}

	void ColorTool::StartSelection(Point start)
	{
		_IsSelecting = true;
		_SelectionStart = start;
		_SelectionRect = Rectangle(start.X, start.Y, 0, 0);
	}

	void ColorTool::UpdateSelection(Point current)
	{
		if (!_IsSelecting || _SelectionStart == nullptr) { 
			return;
		}

		// Calculate selection rectangle
		int X = Math::Min(_SelectionStart->X, current.X);
		int Y = Math::Min(_SelectionStart->Y, current.Y);
		int Width = Math::Abs(current.X - _SelectionStart->X);
		int Height = Math::Abs(current.Y - _SelectionStart->Y);

		_SelectionRect = Rectangle(X, Y, Width, Height);

		// Update selected bars
		SelectBarsInRegion(_SelectionRect);
	}

	void ColorTool::EndSelection()
	{
		_IsSelecting = false;
		_SelectionStart = nullptr;
		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
	}

	void ColorTool::SelectBarsInRegion(Rectangle region)
	{
		_SelectedBars->Clear();

		// Convert selection rectangle to tick range
		int StartTick = _Timeline->PixelsToTicks(region.Left - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int EndTick = _Timeline->PixelsToTicks(region.Right - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track^ T in _Timeline->Tracks)
		{
			Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(T);
		
			if (TrackContentBounds.IntersectsWith(region))
			{
				for each (BarEvent^ Bar in T->Events)
				{
					// Check if bar intersects with selection
					if (Bar->StartTick + Bar->Duration >= StartTick && Bar->StartTick <= EndTick)
					{
						_SelectedBars->Add(Bar);
					}
				}
			}
		}
	}

	void ColorTool::ClearSelection()
	{
		_SelectedBars->Clear();
		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
	}

	void ColorTool::UpdateHoverPreview(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);
		BarEvent^ Bar = nullptr;

		if (TargetTrack != nullptr && mousePos.X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
			Bar = _Timeline->GetBarAtPoint(mousePos);
		}

		if (Bar == nullptr) {
			this->_HoverBar = nullptr;
			this->_HoverTrack = nullptr;
			this->_PreviewRect = Rectangle(0, 0, 0, 0);
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross); // Use cross cursor for selection

			return;
		}
		

		// Calculate preview rectangle
		int XStart = _Timeline->TicksToPixels(Bar->StartTick) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;
		int XEnd = _Timeline->TicksToPixels(Bar->EndTick) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

		this->_BarXHoverRatio = ((float)(mousePos.X - XStart)) / ((float)(XEnd - XStart));


		// Update hover state
		if (Bar != _HoverBar || TargetTrack != _HoverTrack)
		{
			this->_HoverBar = Bar;
			this->_HoverTrack = TargetTrack;

			Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(TargetTrack);

			this->_PreviewRect = Rectangle(
				XStart,
				TrackContentBounds.Y + Timeline_Direct2DRenderer::TRACK_PADDING,
				_Timeline->TicksToPixels(Bar->Duration),
				TrackContentBounds.Height - (Timeline_Direct2DRenderer::TRACK_PADDING * 2)
			);

			// Update cursor based on whether the bar is selected
			Cursor = this->_SelectedBars->Contains(Bar) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		}

		_Timeline->Invalidate();
	}

	void ColorTool::ClearHoverPreview()
	{
		if (_HoverBar != nullptr || !_PreviewRect.IsEmpty) {
			_HoverBar		= nullptr;
			_HoverTrack		= nullptr;
			_PreviewRect	= Rectangle(0, 0, 0, 0);

			_Timeline->Invalidate();
		}
	}

	void ColorTool::ApplyColorToSelection()
	{
		if (_SelectedBars->Count == 1 && _SelectedBars[0]->Type == BarEventType::Fade)
		{
			BarEvent^ Bar = _SelectedBars[0];
			
			Color ColorStart	= Bar->FadeInfo->ColorStart;
			Color ColorCenter	= Bar->FadeInfo->ColorCenter;
			Color ColorEnd		= Bar->FadeInfo->ColorEnd;

			if (Bar->FadeInfo->Type == FadeType::Two_Colors)
			{
				this->_BarXHoverRatio <= 0.5f ? ColorStart = CurrentColor : ColorEnd = CurrentColor;

			}
			else if (Bar->FadeInfo->Type == FadeType::Three_Colors)
			{
				if (this->_BarXHoverRatio <= 0.33f) {
					ColorStart = CurrentColor;
				}
				else if (this->_BarXHoverRatio <= 0.66f) {
					ColorCenter = CurrentColor;
				}
				else {
					ColorEnd = CurrentColor;
				}
			}

			Bar->FadeInfo->ColorStart	= ColorStart;
			Bar->FadeInfo->ColorCenter	= ColorCenter;
			Bar->FadeInfo->ColorEnd		= ColorEnd;
		}
		else
		{
			for each (BarEvent^ Bar in _SelectedBars)
			{
				if (Bar->Type == BarEventType::Fade) {
					continue;
				}
				
				Bar->Color = _CurrentColor;
			}
		}

		_Timeline->Invalidate();
	}

	void ColorTool::CurrentColor::set(Color value)
	{
		_CurrentColor = value;

		if (_SelectedBars->Count > 0) {
			ApplyColorToSelection();
		}

		_Timeline->Invalidate();
	}


	/////////////////////////////
	// FadeTool Implementation //
	/////////////////////////////
	FadeTool::FadeTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		_IsDrawing		= false;
		_DrawStart		= nullptr;
		_TargetTrack	= nullptr;
		_PreviewBar		= nullptr;

		TickLength	= Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION;
		Type		= FadeType::Two_Colors;
		ColorStart	= Color::Red;
		ColorEnd	= Color::Green;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
	}

	void FadeTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point MousePos(e->X, e->Y);
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(MousePos);

			if (TargetTrack != nullptr) {
				// Calculate start tick with grid snap
				_StartTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));

				// Create initial preview bar with fade info
				BarEventFadeInfo^ FadeInfo;
				if (this->Type == FadeType::Two_Colors) {
					FadeInfo = gcnew BarEventFadeInfo(this->TickLength, this->ColorStart, this->ColorEnd);
				}
				else {
					FadeInfo = gcnew BarEventFadeInfo(this->TickLength, this->ColorStart, this->ColorCenter, this->ColorEnd);
				}

				_PreviewBar = gcnew BarEvent(nullptr, _StartTick, this->TickLength, FadeInfo);

				if (!HasOverlappingBarsOnSpecificTrack(TargetTrack, _PreviewBar))
				{
					_IsDrawing = true;
					_DrawStart = MousePos;
					_TargetTrack = TargetTrack;
					_Timeline->Invalidate();
				}
				else
				{
					_PreviewBar = nullptr;
				}	
			}
		}
	}

	void FadeTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);
		_LastMousePos = MousePos;

		if (_IsDrawing && _TargetTrack != nullptr && _PreviewBar != nullptr)
		{
			// Calculate total distance in ticks
			int CurrentTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));
			int TotalTicks = CurrentTick - _StartTick;

			if (TotalTicks > 0)
			{
				int OriginalDuration = _PreviewBar->Duration;
				
				// Update preview bar duration
				_PreviewBar->Duration = TotalTicks;

				if (HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBar)) {
					// If there's an overlap, revert to the previous duration
					_PreviewBar->Duration = OriginalDuration;
				}

				_Timeline->Invalidate();
			}
		}
		else if (e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(MousePos);
			if (TargetTrack != nullptr)
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
				UpdatePreview(MousePos);
			}
			else
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
				ClearPreviews();
			}
		}
		else
		{
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		}
	}

	void FadeTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsDrawing && _PreviewBar != nullptr)
		{
			Point MousePos(e->X, e->Y);
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(MousePos);

			// Only add bars if released over the same track
			if (TargetTrack == _TargetTrack) {
				AddBarToTrack();
			}

			// Reset state
			_IsDrawing		= false;
			_DrawStart		= nullptr;
			_TargetTrack	= nullptr;

			ClearPreviews();

			_Timeline->Invalidate();
		}
	}

	void FadeTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Escape)
		{
			// Cancel active drawing operation
			if (_IsDrawing)
			{
				_IsDrawing = false;
				_DrawStart = nullptr;
				_TargetTrack = nullptr;
			}

			// Clear any previews
			ClearPreviews();

			// Update cursor and view
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
			_Timeline->Invalidate();
		}
	}

	void FadeTool::OnKeyUp(KeyEventArgs^ e) {}

	void FadeTool::AddBarToTrack()
	{
		if (_PreviewBar == nullptr || _TargetTrack == nullptr) {
			return;
		}

		if (!HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBar))
		{
			BarEventFadeInfo^ FadeInfo = nullptr;
			if (this->Type == FadeType::Two_Colors) {
				FadeInfo = gcnew BarEventFadeInfo(this->TickLength, this->ColorStart, this->ColorEnd);
			}
			else if (this->Type == FadeType::Three_Colors) {
				FadeInfo = gcnew BarEventFadeInfo(this->TickLength, this->ColorStart, this->ColorCenter, this->ColorEnd);
			}

			BarEvent^ FadeBar = gcnew BarEvent(_TargetTrack, _PreviewBar->StartTick, _PreviewBar->Duration, FadeInfo);
			_TargetTrack->AddBar(FadeBar);
		}
	}

	void FadeTool::UpdatePreview(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (TargetTrack != nullptr && !_IsDrawing)
		{
			int SnapTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));

			BarEvent^ TempBar = gcnew BarEvent(TargetTrack, SnapTick, this->TickLength, this->ColorStart);

			if (HasOverlappingBarsOnBarTrack(TempBar))
			{
				// If there would be an overlap, clear any existing preview and show "no" cursor
				if (_PreviewBar != nullptr) {
					_PreviewBar = nullptr;
					_Timeline->Invalidate();
				}
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else
			{
				if (_PreviewBar == nullptr) {
					_PreviewBar = gcnew BarEvent(nullptr, SnapTick, this->TickLength, this->ColorStart);
				}
				else {
					_PreviewBar->StartTick = SnapTick;
					_PreviewBar->Duration = this->TickLength;
					_PreviewBar->Color = this->ColorStart;
				}
			}

			_Timeline->Invalidate();
		}
		else
		{
			ClearPreviews();
		}
	}

	void FadeTool::ClearPreviews()
	{
		if (_PreviewBar != nullptr)
		{
			_PreviewBar = nullptr;
			_Timeline->Invalidate();
		}
	}


	///////////////////////////////
	// StrobeTool Implementation //
	///////////////////////////////
	StrobeTool::StrobeTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		_IsDrawing		= false;
		_DrawStart		= nullptr;
		_TargetTrack	= nullptr;
		_PreviewBar		= nullptr;

		TickLength	= Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION;
		ColorStrobe = Color::Red;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
	}

	void StrobeTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point MousePos(e->X, e->Y);
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(MousePos);

			if (TargetTrack != nullptr) {
				// Calculate start tick with grid snap
				_StartTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));
				
				// Create initial preview bar with fade info
				BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(this->TickLength, this->ColorStrobe);

				_PreviewBar = gcnew BarEvent(nullptr, _StartTick, this->TickLength, StrobeInfo);

				if (!HasOverlappingBarsOnSpecificTrack(TargetTrack, _PreviewBar))
				{
					_IsDrawing = true;
					_DrawStart = MousePos;
					_TargetTrack = TargetTrack;
					_Timeline->Invalidate();
				}
				else
				{
					_PreviewBar = nullptr;
				}
			}
		}
	}

	void StrobeTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);
		_LastMousePos = MousePos;

		if (_IsDrawing && _TargetTrack != nullptr && _PreviewBar != nullptr)
		{
			// Calculate total distance in ticks
			int CurrentTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));
			int TotalTicks = CurrentTick - _StartTick;
			
			if (TotalTicks > 0)
			{
				int OriginalDuration = _PreviewBar->Duration;

				// Update preview bar duration
				_PreviewBar->Duration = TotalTicks;

				if (HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBar)) {
					// If there's an overlap, revert to the previous duration
					_PreviewBar->Duration = OriginalDuration;
				}

				_Timeline->Invalidate();
			}
		}
		else if (e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(MousePos);
			if (TargetTrack != nullptr)
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
				UpdateSinglePreview(MousePos);
			}
			else
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
				ClearPreviews();
			}
		}
		else
		{
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		}
	}

	void StrobeTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsDrawing && _PreviewBar != nullptr)
		{
			Point MousePos(e->X, e->Y);
			Track^ TargetTrack = _Timeline->GetTrackAtPoint(MousePos);

			// Only add bars if released over the same track
			if (TargetTrack == _TargetTrack) {
				AddBarToTrack();
			}

			// Reset state
			_IsDrawing		= false;
			_DrawStart		= nullptr;
			_TargetTrack	= nullptr;

			ClearPreviews();

			_Timeline->Invalidate();
		}
	}

	void StrobeTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Escape)
		{
			// Cancel active drawing operation
			if (_IsDrawing)
			{
				_IsDrawing = false;
				_DrawStart = nullptr;
				_TargetTrack = nullptr;
			}

			// Clear any previews
			ClearPreviews();

			// Update cursor and view
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
			_Timeline->Invalidate();
		}
	}

	void StrobeTool::OnKeyUp(KeyEventArgs^ e) {}

	void StrobeTool::AddBarToTrack()
	{
		if (_PreviewBar == nullptr || _TargetTrack == nullptr) {
			return;
		}

		if (!HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBar))
		{
			BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(this->TickLength, this->ColorStrobe);

			BarEvent^ StrobeBar = gcnew BarEvent(_TargetTrack, _PreviewBar->StartTick, _PreviewBar->Duration, StrobeInfo);
			_TargetTrack->AddBar(StrobeBar);
		}
	}

	void StrobeTool::UpdateSinglePreview(Point mousePos)
	{
		Track^ TragetTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (TragetTrack != nullptr && !_IsDrawing)
		{
			int SnapTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));

			BarEvent^ TempBar = gcnew BarEvent(TargetTrack, SnapTick, this->TickLength, this->ColorStrobe);

			if (HasOverlappingBarsOnBarTrack(TempBar))
			{
				// If there would be an overlap, clear any existing preview and show "no" cursor
				if (_PreviewBar != nullptr) {
					_PreviewBar = nullptr;
					_Timeline->Invalidate();
				}
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else
			{
				if (_PreviewBar == nullptr) {
					_PreviewBar = gcnew BarEvent(nullptr, SnapTick, this->TickLength, this->ColorStrobe);
				}
				else {
					_PreviewBar->StartTick	= SnapTick;
					_PreviewBar->Duration	= this->TickLength;
					_PreviewBar->Color		= this->ColorStrobe;
				}
			}

			_Timeline->Invalidate();
		}
		else
		{
			ClearPreviews();
		}
	}

	void StrobeTool::ClearPreviews()
	{
		if (_PreviewBar != nullptr)
		{
			_PreviewBar = nullptr;
			_Timeline->Invalidate();
		}
	}
}
#include "Widget_Timeline_Tools.h"
#include "Widget_Timeline.h"

namespace MIDILightDrawer
{
	// Base TimelineTool Implementation
	TimelineTool::TimelineTool(Widget_Timeline^ timeline) : _Timeline(timeline)
	{
		_IsActive = false;
		_CanSelectWithRectangle = false;
		_IsSelecting = false;
		_LastMousePos = Point(0, 0);
		_SelectionStart = Point(0, 0);
		_ModifierKeys = Keys::None;

		_SelectedBars = gcnew List<BarEvent^>;
		_PreviewBars = gcnew List<BarEvent^>;
	}

	void TimelineTool::OnMouseRightClick(MouseEventArgs^ e)
	{
		if (e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point ClickPoint(e->X, e->Y);
			BarEvent^ ClickedBar = _Timeline->GetBarAtPoint(ClickPoint);

			if (ClickedBar != nullptr) {
				_Timeline->ShowContextMenu(ClickedBar, ClickPoint);
			}
		}
	}

	void TimelineTool::StartSelection(Point start)
	{
		if (!_CanSelectWithRectangle) {
			return;
		}
		
		_SelectionStart = start;
		_SelectionRect = Rectangle(start.X, start.Y, 0, 0);
		_SelectedBars->Clear();

		_IsSelecting = true;
	}

	void TimelineTool::UpdateSelection(Point current)
	{
		if (!_CanSelectWithRectangle) {
			return;
		}

		// Calculate rectangle from start point to current point
		int X = Math::Min(_SelectionStart.X, current.X);
		int Y = Math::Min(_SelectionStart.Y, current.Y);
		int Width = Math::Abs(current.X - _SelectionStart.X);
		int Height = Math::Abs(current.Y - _SelectionStart.Y);

		_SelectionRect = Rectangle(X, Y, Width, Height);

		// Update selection if dragging
		if (Width > 0 && Height > 0) {
			SelectBarsInRegion(_SelectionRect);
		}
	}

	void TimelineTool::EndSelection()
	{
		// Clear the selection rectangle
		_SelectionRect = Rectangle(0, 0, 0, 0);

		_IsSelecting = false;
	}

	void TimelineTool::SelectBarsInRegion(Rectangle region)
	{
		if (!_CanSelectWithRectangle) {
			return;
		}
		
		_SelectedBars->Clear();

		// Convert selection rectangle to tick range
		int StartTick = _Timeline->PixelsToTicks(region.Left - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);
		int EndTick = _Timeline->PixelsToTicks(region.Right - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track ^ Trk in _Timeline->Tracks)
		{
			Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(Trk);

			if (TrackContentBounds.IntersectsWith(region))
			{
				for each (BarEvent ^ Bar in Trk->Events)
				{
					// Check if bar intersects with selection
					if (Bar->EndTick >= StartTick && Bar->StartTick <= EndTick)
					{
						_SelectedBars->Add(Bar);
					}
				}
			}
		}

		OnSelectionChanged();
	}

	void TimelineTool::ClearSelection()
	{
		if (_SelectedBars->Count > 0)
		{
			for each (BarEvent ^ Bar in _SelectedBars) {
				Bar->IgnoreForOverlap = false;
			}

			_PreviewBars->Clear();
			_SelectedBars->Clear();

			OnSelectionChanged();
		}

		_SelectionRect = Rectangle(0, 0, 0, 0);
		_Timeline->Invalidate();
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

			if (TimeOverlap && !TrackBar->IgnoreForOverlap) {
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
		for each (BarEvent^ TrackBar in track->Events)
		{
			// Skip comparing with itself (important for drag operations)
			if (bar == TrackBar) {
				continue;
			}

			// Check for time overlap
			bool TimeOverlap = bar->StartTick < TrackBar->EndTick && bar->EndTick > TrackBar->StartTick;

			if (TimeOverlap && !TrackBar->IgnoreForOverlap) {
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

	void TimelineTool::OnSelectionChanged() {
		SelectionChanged(this, EventArgs::Empty);
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
		_PreviewBars = gcnew List<BarEvent^>();
		_OriginalBarStartTicks = gcnew List<int>();

		_CanSelectWithRectangle = true;
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
			ClearSelection();
			return;
		}

		// Click in track content - handle bars
		BarEvent^ Bar = _Timeline->GetBarAtPoint(*_DragStart);

		if (Bar == nullptr) {
			// Start selection rectangle if clicking empty space
			if (!Control::ModifierKeys.HasFlag(Keys::Control)) {
				ClearSelection();
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
		if (_SelectedBars->Contains(Bar))
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

	void PointerTool::OnCommandStateChanged()
	{
		// Create a new list to store bars that still exist
		List<BarEvent^>^ ExistingBars = gcnew List<BarEvent^>();

		for each (BarEvent^ Bar in _SelectedBars)
		{
			if (_Timeline->DoesBarExist(Bar))
			{
				ExistingBars->Add(Bar);
			}
		}

		// Update selection
		_SelectedBars = ExistingBars;

		_Timeline->Invalidate();
	}

	void PointerTool::StartMoving(Point mousePos)
	{
		StoreOriginalPositions();

		_PreviewBars->Clear();

		for each (BarEvent^ Bar in _SelectedBars)
		{
			BarEvent^ PreviewBar = TimelineCommandManager::CreateBarCopy(Bar, Bar->StartTick, false);
			_PreviewBars->Add(PreviewBar);

			Bar->IgnoreForOverlap = true;
		}

		_DragSourceTrack = _Timeline->GetTrackAtPoint(mousePos);
		_IsDragging = true;
	}

	void PointerTool::UpdateMoving(Point mousePos)
	{
		if (_PreviewBars->Count == 0) {
			return;
		}
		
		// Calculate movement in ticks
		int PixelDelta = mousePos.X - _DragStart->X;
		int TickDelta = _Timeline->PixelsToTicks(PixelDelta);

		// Store original positions
		for each (BarEvent^ Bar in _PreviewBars) {
			_OriginalBarStartTicks->Add(Bar->StartTick);
		}

		// Update positions of selected bars
		for each (BarEvent^ Bar in _PreviewBars)
		{
			if (_PreviewBars->IndexOf(Bar) == 0) {
				// Use SnapTickBasedOnType for the first bar
				Bar->StartTick = _Timeline->SnapTickBasedOnType(Bar->OriginalStartTick + TickDelta, mousePos);
			}
			else {
				int TickOffsetToFirstBar = Bar->OriginalStartTick - _SelectedBars[0]->OriginalStartTick;
				Bar->StartTick = _PreviewBars[0]->StartTick + TickOffsetToFirstBar;
			}
		}

		// Only update target track if not over header area and not multi-track selection
		if (!IsMultiTrackSelection && mousePos.X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			_DragTargetTrack = _Timeline->GetTrackAtPoint(mousePos);

			if ((_DragTargetTrack != nullptr) && !HasOverlappingBarsOnSpecificTrack(_DragTargetTrack, _PreviewBars))
			{
				for each (BarEvent^ Bar in _PreviewBars)
				{
					Bar->ContainingTrack = _DragTargetTrack;
				}
			}
		}

		// Restore original positions if there's an overlap
		if (HasOverlappingBarsOnBarTrack(_PreviewBars))
		{
			for (int i = 0; i < _PreviewBars->Count; i++) {
				_PreviewBars[i]->StartTick = _OriginalBarStartTicks[i];
			}
		}

		_OriginalBarStartTicks->Clear();
		_Timeline->Invalidate();
	}

	void PointerTool::FinishMoving(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (TargetTrack == nullptr) {
			CancelMoving();
			return;
		}

		for (int i = 0; i < _SelectedBars->Count; i++)
		{
			_SelectedBars[i]->StartTick			= _PreviewBars[i]->StartTick;
			_SelectedBars[i]->ContainingTrack	= _PreviewBars[i]->ContainingTrack;

			_SelectedBars[i]->IgnoreForOverlap = false;
		}

		if (IsMultiTrackSelection)
		{
			// Create a compound command for multiple bar moves
			CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Move Multiple Bars");

			for each (BarEvent^ Bar in _SelectedBars)
			{
				MoveBarCommand^ Cmd = gcnew MoveBarCommand(_Timeline, Bar, Bar->OriginalContainingTrack, Bar->ContainingTrack, Bar->OriginalStartTick, _Timeline->SnapTickToGrid(Bar->StartTick));
				CompoundCmd->AddCommand(Cmd);
			}

			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}
		else
		{
			if (_SelectedBars->Count == 1)
			{
				MoveBarCommand^ Cmd = gcnew MoveBarCommand(_Timeline, _SelectedBars[0], _SelectedBars[0]->OriginalContainingTrack, TargetTrack, _SelectedBars[0]->OriginalStartTick, _SelectedBars[0]->StartTick);
				_Timeline->CommandManager()->ExecuteCommand(Cmd);
			}
			else
			{
				// Single track or grouped bars moving to same track
				if (TargetTrack != _DragSourceTrack)
				{
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Move Bars to Different Track");

					for each (BarEvent ^ Bar in _SelectedBars)
					{
						MoveBarCommand^ Cmd = gcnew MoveBarCommand(_Timeline, Bar, Bar->OriginalContainingTrack, TargetTrack, Bar->OriginalStartTick, Bar->StartTick);
						CompoundCmd->AddCommand(Cmd);
					}

					_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);

				}
				else
				{
					// Moving within same track
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Move Bars");

					for each (BarEvent ^ bar in _SelectedBars)
					{
						MoveBarCommand^ cmd = gcnew MoveBarCommand(_Timeline, bar, _DragSourceTrack, _DragSourceTrack, bar->OriginalStartTick, bar->StartTick);
						CompoundCmd->AddCommand(cmd);
					}

					_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
				}
			}
		}

		_IsDragging = false;
		_DragSourceTrack = nullptr;
		_DragTargetTrack = nullptr;

		_PreviewBars->Clear();
	}

	void PointerTool::CancelMoving()
	{
		// Restore original tick position and containing track
		// TODO: I think this is not needed anymore
		for each (BarEvent^ Bar in _SelectedBars) {
			Bar->StartTick = Bar->OriginalStartTick;
			Bar->ContainingTrack = Bar->OriginalContainingTrack;
		}
		
		_IsDragging = false;
		_DragSourceTrack = nullptr;
		_DragTargetTrack = nullptr;
		_OriginalBarStartTicks->Clear();
		_PreviewBars->Clear();
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
					BarEvent^ CopiedBar = TimelineCommandManager::CreateBarCopy(Bar, Bar->StartTick - EarliestTick, false);
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

			BarEvent^ PasteBar = TimelineCommandManager::CreateBarCopy(CopiedBar, BarTick, true);
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

		if (!HasOverlappingBarsOnBarTrack(_PastePreviewBars))
		{
			// Clear existing selection
			_SelectedBars->Clear();

			List<BarEvent^>^ BarsToPaste = gcnew List<BarEvent^>();
			List<Track^>^ TargetTracks = gcnew List<Track^>();

			for each (BarEvent^ PreviewBar in _PastePreviewBars)
			{
				BarsToPaste->Add(PreviewBar);
				TargetTracks->Add(PreviewBar->ContainingTrack);
			}

			// Create and execute the paste command
			PasteBarCommand^ Cmd = gcnew PasteBarCommand(_Timeline, BarsToPaste, TargetTracks);
			_Timeline->CommandManager()->ExecuteCommand(Cmd);

			// Update selection to include newly pasted bars
			_SelectedBars->AddRange(Cmd->CreatedBars);

			// Implementation before the TimelineCommandManager
			/*
			for each(BarEvent^ PasteBar in _PastePreviewBars)
			{
				PasteBar->ContainingTrack->AddBar(PasteBar);
			}
			*/

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

		CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Delete Bars");

		for each (BarEvent^ Bar in _SelectedBars)
		{
			Track^ Trk = Bar->ContainingTrack;
			if (Trk != nullptr) {
				DeleteBarCommand^ Cmd = gcnew DeleteBarCommand(_Timeline, Trk, Bar);
				CompoundCmd->AddCommand(Cmd);
			}
		}

		_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);

		// Implementation before the TimelineCommandManager
		/*
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
		*/
		_SelectedBars->Clear();

		_Timeline->Invalidate();
		this->UpdateCursor(_LastMousePos);
	}


	/////////////////////////////
	// DrawTool Implementation //
	/////////////////////////////
	DrawTool::DrawTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		
		_TargetTrack	= nullptr;
		_SourceTrack	= nullptr;
		_CurrentColor	= Color::FromArgb(200, 100, 100, 255);
		_DrawTickLength	= 960;
		_UseAutoLength	= false;

		_IsPainting		= false;
		_IsErasing		= false;
		_IsDragging		= false;
		_IsResizing		= false;
		_LastPaintedTick = 0;
		_CurrentMode	= DrawToolMode::Draw;

		timeline->UpdateCursor(_CurrentCursor);
	}

	void DrawTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point MousePos(e->X, e->Y);
			_TargetTrack = _Timeline->GetTrackAtPoint(MousePos);

			if (_TargetTrack != nullptr) {
				switch (_CurrentMode) {
				case DrawToolMode::Draw:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
					StartPainting(MousePos);
					break;

				case DrawToolMode::Erase:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
					StartErasing(MousePos);
					break;

				case DrawToolMode::Move:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
					StartMoving(MousePos);
					break;

				case DrawToolMode::Resize:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
					StartResizing(MousePos);
					break;
				}
			}
		}
	}

	void DrawTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);
		_LastMousePos = MousePos;

		if (e->X <= Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			ClearPreview();
			return;
		}

		if (e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			// Handle active operations
			if (e->Button == Windows::Forms::MouseButtons::Left)
			{
				if (_IsResizing)
				{
					_CurrentMode = DrawToolMode::Resize;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
					UpdateResizing(MousePos);
				}
				else if (_IsDragging)
				{
					_CurrentMode = DrawToolMode::Move;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
					UpdateMoving(MousePos);
				}
				else
				{
					switch (_CurrentMode)
					{
					case DrawToolMode::Draw:	UpdatePainting(MousePos);	break;
					case DrawToolMode::Erase:	UpdateErasing(MousePos);	break;
					}
				}
			}
			else
			{
				// Update mode and cursor for hover state
				UpdateCurrentMode(MousePos);
				UpdatePreview(MousePos);
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
		_IsDragging	= false;
		_IsResizing	= false;

		UpdateCurrentMode(Point(e->X, e->Y));
	}

	void DrawTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Escape)
		{
			if (_IsDragging) { 
				CancelMoving();
				_Timeline->Invalidate();
			}
			else if (_IsResizing) {
				CancelResizing();
				_Timeline->Invalidate();
			}
		}
	}

	void DrawTool::OnKeyUp(KeyEventArgs^ e)
	{
		
	}

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
						if (_PreviewBars->Count == 0)
						{
							BarEvent^ PreviewBar = gcnew BarEvent(nullptr, Start_Tick, Length_In_Ticks, Color::FromArgb(100, (int)_CurrentColor.R, (int)_CurrentColor.G, (int)_CurrentColor.B));
							_PreviewBars->Add(PreviewBar);
						}
						else {
							_PreviewBars[0]->StartTick	= Start_Tick;
							_PreviewBars[0]->Duration	= Length_In_Ticks;
						}
					}
					else {
						_PreviewBars->Clear();
					}
					break;

				case DrawToolMode::Move:
				case DrawToolMode::Erase:
				case DrawToolMode::Resize:
					_PreviewBars->Clear();
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
		_PreviewBars->Clear();
		_Timeline->Invalidate();
	}

	void DrawTool::UpdateCurrentMode(Point mousePos)
	{
		if (_IsResizing || _IsDragging || _IsPainting) {
			UpdateActiveModesCursor();
			return;
		}

		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);
		BarEvent^ Bar = nullptr;

		if (TargetTrack != nullptr) {
			Bar = _Timeline->GetBarAtPoint(mousePos);
		}

		// Determine new mode based on context
		DrawToolMode NewMode;
		System::Windows::Forms::Cursor^ NewCursor;

		if (Bar != nullptr)
		{
			_SelectedBars->Clear();
			_SelectedBars->Add(Bar);
			
			// Check resize handle first - this should take priority
			if (IsOverResizeHandle(mousePos, Bar, TargetTrack)) {
				NewMode = DrawToolMode::Resize;
				NewCursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
			}
			else if (Control::ModifierKeys == Keys::Control) {
				NewMode = DrawToolMode::Move;
				NewCursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
			}
			else {
				NewMode = DrawToolMode::Erase;
				NewCursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
		}
		else
		{
			NewMode = DrawToolMode::Draw;

			if (!HasOverlappingBars(TargetTrack, _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X), _DrawTickLength)) {
				NewCursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);	// For drawing (when position is valid)
			}
			else {
				NewCursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);	// For drawing (when position would cause overlap)
			}
		}

		// Only update mode if it changed
		if (NewMode != _CurrentMode || Cursor != NewCursor)
		{
			Cursor = NewCursor;

			_CurrentMode = NewMode;
			_Timeline->Invalidate();
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
			this->Cursor = _IsDragging ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
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
		if (bar == nullptr || track == nullptr) {
			return false;
		}

		Rectangle TrackContentBounds = _Timeline->GetTrackContentBounds(track);

		// Calculate bar end position in pixels
		int BarEndX = _Timeline->TicksToPixels(bar->StartTick + bar->Duration) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

		// Define handle region - make it a bit larger for easier interaction
		const int HANDLE_WIDTH = 5;

		// Check if mouse is within handle area
		bool IsInXRange = mousePos.X >= BarEndX - HANDLE_WIDTH && mousePos.X <= BarEndX + HANDLE_WIDTH;

		bool IsInYRange = mousePos.Y >= TrackContentBounds.Y + Timeline_Direct2DRenderer::TRACK_PADDING && mousePos.Y <= TrackContentBounds.Bottom - Timeline_Direct2DRenderer::TRACK_PADDING;

		return IsInXRange && IsInYRange;
	}

	bool DrawTool::HasOverlappingBars(Track^ track, int startTick, int length)
	{
		if (track == nullptr) {
			return false;
		}

		int EndTick = startTick + length;

		for each (BarEvent^ ExistingBar in track->Events)
		{
			// Skip checking against the bar we're currently moving
			if (_SelectedBars->Contains(ExistingBar)) {
				continue;
			}

			int ExistingEnd = ExistingBar->StartTick + ExistingBar->Duration;

			// Check for any overlap
			if (startTick < ExistingEnd && EndTick > ExistingBar->StartTick) {
				return true;
			}
		}

		return false;
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

		BarEvent^ NewBar = gcnew BarEvent(_TargetTrack, Start_Tick, Length_In_Ticks, _CurrentColor);

		if (!HasOverlappingBarsOnBarTrack(NewBar))
		{
			AddBarCommand^ Cmd = gcnew AddBarCommand(_Timeline, _TargetTrack, NewBar);
			_Timeline->CommandManager()->ExecuteCommand(Cmd);
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
			BarEvent^ NewBar = gcnew BarEvent(_TargetTrack, Start_Tick, Length_In_Ticks, _CurrentColor);
			
			if (!HasOverlappingBarsOnBarTrack(NewBar))
			{
				// Create and execute the command
				AddBarCommand^ Cmd = gcnew AddBarCommand(_Timeline, _TargetTrack, NewBar);
				_Timeline->CommandManager()->ExecuteCommand(Cmd);
			
				_LastPaintedTick = Start_Tick;
			}
		}
	}
	
	void DrawTool::StartErasing(Point mousePos)
	{
		_IsErasing = true; 

		if (_SelectedBars->Count > 0)
		{
			DeleteBarCommand^ Cmd = gcnew DeleteBarCommand(_Timeline, _TargetTrack, _SelectedBars[0]);
			_Timeline->CommandManager()->ExecuteCommand(Cmd);
		}
	}

	void DrawTool::UpdateErasing(Point mousePos)
	{
		if (!_IsErasing) {
			return;
		}

		Track^ Trk = _Timeline->GetTrackAtPoint(mousePos);

		if (Trk != nullptr)
		{
			// Get the bar under the mouse cursor
			BarEvent^ Bar = _Timeline->GetBarAtPoint(mousePos);

			// Only erase if we find a bar and haven't already erased it
			if (Bar != nullptr)
			{
				DeleteBarCommand^ cmd = gcnew DeleteBarCommand(_Timeline, Trk, Bar);
				_Timeline->CommandManager()->ExecuteCommand(cmd);
			}
		}
	}

	void DrawTool::StartMoving(Point mousePos)
	{
		if (_SelectedBars->Count > 0)
		{
			_IsDragging = true;
			_CurrentMode = DrawToolMode::Move;
			
			_DragStartPoint = mousePos;
			_DragStartTick	= _SelectedBars[0]->StartTick;

			_SourceTrack = _Timeline->GetTrackAtPoint(mousePos);
			_TargetTrack = _SourceTrack;

			_SelectedBars[0]->BasicInfoCopyWorkingToOriginal();
			_SelectedBars[0]->IgnoreForOverlap = true;

			BarEvent^ PreviewBar = TimelineCommandManager::CreateBarCopy(_SelectedBars[0], _SelectedBars[0]->StartTick, false);
			_PreviewBars->Clear();
			_PreviewBars->Add(PreviewBar);

			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
		}
	}

	void DrawTool::UpdateMoving(Point mousePos)
	{
		if (!_IsDragging || _PreviewBars->Count == 0 || _TargetTrack == nullptr) {
			return;
		}

		// Calculate movement in ticks
		int PixelDelta = mousePos.X - _DragStartPoint.X;
		int TickDelta = _Timeline->PixelsToTicks(PixelDelta);

		// Calculate new position with grid snapping
		int NewStartTick = _Timeline->SnapTickToGrid(_DragStartTick + TickDelta);

		// Store original position for overlap check
		int OriginalStart = _PreviewBars[0]->StartTick;

		// Temporarily move to new position
		_PreviewBars[0]->StartTick = NewStartTick;

		Track^ TrackUnderMouse = _Timeline->GetTrackAtPoint(mousePos);
		if (TrackUnderMouse != nullptr)
		{
			if (HasOverlappingBarsOnSpecificTrack(TrackUnderMouse, _PreviewBars[0])) {
				_PreviewBars[0]->StartTick = OriginalStart;
			}
			else if (TrackUnderMouse != _PreviewBars[0]->ContainingTrack)
			{
				_PreviewBars[0]->ContainingTrack = TrackUnderMouse;
			}
		}
		else
		{
			_PreviewBars[0]->StartTick = OriginalStart;
		}

		_Timeline->Invalidate();
	}

	void DrawTool::FinishMoving(Point mousePos)
	{
		if (!_IsDragging || _SelectedBars->Count == 0 || _PreviewBars->Count == 0) {
			return;
		}
		
		Track^ FinalTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (FinalTrack == nullptr) {
			CancelMoving();
			return;
		}

		_SelectedBars[0]->StartTick			= _PreviewBars[0]->StartTick;
		_SelectedBars[0]->ContainingTrack	= _PreviewBars[0]->ContainingTrack;
		_SelectedBars[0]->IgnoreForOverlap = false;
		
		MoveBarCommand^ Cmd = gcnew MoveBarCommand(_Timeline, _SelectedBars[0], _SourceTrack, FinalTrack, _SelectedBars[0]->OriginalStartTick, _Timeline->SnapTickToGrid(_SelectedBars[0]->StartTick));
		_Timeline->CommandManager()->ExecuteCommand(Cmd);

		_PreviewBars->Clear();
		_SelectedBars->Clear();
		_IsDragging = false;
		_TargetTrack = nullptr;
		_SourceTrack = nullptr;
	}

	void DrawTool::CancelMoving()
	{
		if (_IsDragging && _SelectedBars->Count > 0)
		{
			_SelectedBars[0]->IgnoreForOverlap = false;
		}
		
		_PreviewBars->Clear();
		_SelectedBars->Clear();
		_IsDragging = false;
		_TargetTrack = nullptr;
		_SourceTrack = nullptr;
	}

	void DrawTool::StartResizing(Point mousePos)
	{
		if (_SelectedBars->Count > 0)
		{
			_IsResizing = true;
			_CurrentMode = DrawToolMode::Resize;
			
			_DragStartPoint = mousePos;
			_DragStartTick = _SelectedBars[0]->Duration;

			_TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

			_SelectedBars[0]->BasicInfoCopyWorkingToOriginal();
			_SelectedBars[0]->IgnoreForOverlap = true;

			BarEvent^ PreviewBar = TimelineCommandManager::CreateBarCopy(_SelectedBars[0], _SelectedBars[0]->StartTick, false);
			_PreviewBars->Clear();
			_PreviewBars->Add(PreviewBar);

			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
			
		}
	}

	void DrawTool::UpdateResizing(Point mousePos)
	{
		if (!_IsResizing || _PreviewBars->Count == 0 || _TargetTrack == nullptr) {
			return;
		}

		// Calculate mouse position in ticks
		int MouseTickPosition = _Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X);

		// Calculate new length (from bar start to mouse position)
		int NewLength = MouseTickPosition - _PreviewBars[0]->StartTick;

		// Snap the length to grid
		int SnappedLength = _Timeline->SnapTickToGrid(NewLength);

		// Ensure minimum length (e.g., 1/32 note)
		const int MIN_LENGTH = 120; // 1/32 note
		SnappedLength = Math::Max(SnappedLength, MIN_LENGTH);

		// Check for overlaps with other bars
		bool HasOverlap = false;
		for each(BarEvent^ ExistingBar in _TargetTrack->Events)
		{
			if (!ExistingBar->IgnoreForOverlap && ExistingBar->StartTick < (_PreviewBars[0]->StartTick + SnappedLength) && (ExistingBar->StartTick + ExistingBar->Duration) > _PreviewBars[0]->StartTick)
			{
				HasOverlap = true;
				break;
			}
		}

		if (!HasOverlap) {
			_PreviewBars[0]->Duration = SnappedLength;
			_Timeline->Invalidate();
		}
	}

	void DrawTool::FinishResizing()
	{
		if (!_IsResizing || _SelectedBars->Count == 0 || _PreviewBars->Count == 0) {
			return;
		}
		
		_SelectedBars[0]->IgnoreForOverlap = false;

		ResizeBarCommand^ cmd = gcnew ResizeBarCommand(_Timeline, _SelectedBars[0], _SelectedBars[0]->Duration, _PreviewBars[0]->Duration);
		_Timeline->CommandManager()->ExecuteCommand(cmd);
		
		_PreviewBars->Clear();
		_SelectedBars->Clear();
		_IsResizing = false;
		_TargetTrack = nullptr;
	}

	void DrawTool::CancelResizing()
	{
		if (_IsResizing && _SelectedBars->Count > 0)
		{
			_SelectedBars[0]->IgnoreForOverlap = false;
		}
		
		_PreviewBars->Clear();
		_SelectedBars->Clear();
		_IsResizing = false;
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
		
		if (_PreviewBars->Count > 0)
		{
			_PreviewBars[0]->Color = Color::FromArgb(100, (int)value.R, (int)value.G, (int)value.B);
			_Timeline->Invalidate();
		}
	}

	void DrawTool::DrawTickLength::set(int value)
	{
		_DrawTickLength = value;

		if (_PreviewBars->Count > 0)
		{
			_PreviewBars[0]->Duration = value;
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
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);

		_HoverBar = nullptr;
		_CanSelectWithRectangle = true;
	}

	void EraseTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point ClickPoint(e->X, e->Y);
			BarEvent^ ClickedBar = _Timeline->GetBarAtPoint(ClickPoint);

			// If clicking on a selected bar, delete all selected bars
			if (ClickedBar != nullptr && _SelectedBars->Contains(ClickedBar))
			{
				EraseSelectedBars();
			}
			// If clicking on an unselected bar, clear selection and select only this bar
			else if (ClickedBar != nullptr && _SelectedBars->Count > 0)
			{
				ClearSelection();

				UpdateHoverPreview(ClickPoint);
				OnSelectionChanged();
			}
			// Clicking directly on the hovered bar -> delete directly, now double click needed
			else if (ClickedBar != nullptr && ClickedBar == _HoverBar)
			{
				ClearSelection();
				_SelectedBars->Add(ClickedBar);

				EraseSelectedBars();
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

	void EraseTool::OnMouseMove(MouseEventArgs^ e)
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

	void EraseTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsSelecting)
		{
			EndSelection();
		}

		UpdateHoverPreview(Point(e->X, e->Y));
	}

	void EraseTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Delete) {
			EraseSelectedBars();
		}
	}
	
	void EraseTool::OnKeyUp(KeyEventArgs^ e) { }

	void EraseTool::OnCommandStateChanged()
	{
		if (_SelectedBars != nullptr)
		{
			List<BarEvent^>^ ExistingBars = gcnew List<BarEvent^>();

			for each (BarEvent^ Bar in _SelectedBars)
			{
				if (_Timeline->DoesBarExist(Bar))
				{
					ExistingBars->Add(Bar);
				}
			}

			_SelectedBars = ExistingBars;
			_Timeline->Invalidate();
		}
	}

	void EraseTool::EraseSelectedBars()
	{
		if (_SelectedBars->Count == 0) {
			return;
		}

		if (_SelectedBars->Count == 1)
		{
			DeleteBarCommand^ Cmd = gcnew DeleteBarCommand(_Timeline, _SelectedBars[0]->ContainingTrack, _SelectedBars[0]);

			_Timeline->CommandManager()->ExecuteCommand(Cmd);
		}
		else
		{
			CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Delete Selected Bars");

			for each (BarEvent^ Bar in _SelectedBars)
			{
				Track^ Trk = Bar->ContainingTrack;
				if (Trk != nullptr)
				{
					DeleteBarCommand^ cmd = gcnew DeleteBarCommand(_Timeline, Trk, Bar);
					CompoundCmd->AddCommand(cmd);
				}
			}

			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}

		ClearSelection();
	}

	void EraseTool::UpdateHoverPreview(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);
		BarEvent^ Bar = nullptr;

		if (TargetTrack != nullptr && mousePos.X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH) {
			Bar = _Timeline->GetBarAtPoint(mousePos);
		}

		// Update hover state
		if (Bar != _HoverBar)
		{
			_HoverBar = Bar;

			if (Bar != nullptr)
			{
				// Update cursor based on whether the bar is selected
				Cursor = _SelectedBars->Contains(Bar) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross); // Use cross cursor for selection
			}

			_Timeline->Invalidate();
		}
	}

	void EraseTool::ClearHoverPreview()
	{
		_HoverBar = nullptr;
		_Timeline->Invalidate();
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
		_IsResizing			= false;
		_TargetBar			= nullptr;
		_TargetTrack		= nullptr;
		_OriginalLength		= 0;
		_DragStartX			= 0;
		_ChangeTickLength	= Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		_SelectedBars		= gcnew List<BarEvent^>();
		_OriginalLengths	= gcnew Dictionary<BarEvent^, int>();

		_CanSelectWithRectangle = true;
	}

	void DurationTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point MousePos(e->X, e->Y);
			Track^ TargetTrack	= _Timeline->GetTrackAtPoint(MousePos);
			BarEvent^ Bar		= _Timeline->GetBarAtPoint(MousePos);

			if (TargetTrack != nullptr && Bar != nullptr)
			{
				// If clicking on a handle of a selected bar
				if (IsOverHandle(MousePos, Bar, TargetTrack) && _SelectedBars->Contains(Bar))
				{
					_TargetBar		= Bar;
					_TargetTrack	= TargetTrack;

					StartResizing(MousePos);
				}
				// If clicking on a handle of an unselected bar
				else if (IsOverHandle(MousePos, Bar, TargetTrack))
				{
					ClearSelection();
					_SelectedBars->Add(Bar);

					_TargetBar		= Bar;
					_TargetTrack	= TargetTrack;

					StartResizing(MousePos);
				}
				// If clicking on a bar (not handle)
				else if (!IsOverHandle(MousePos, Bar, TargetTrack))
				{
					ClearSelection();
					StartSelection(MousePos);
				}
			}
			else
			{
				// Clicking on empty space
				ClearSelection();
				StartSelection(MousePos);
			}

			_Timeline->Invalidate();
		}
	}

	void DurationTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);

		if (_IsResizing && _TargetBar != nullptr && _PreviewBars->Count > 0)
		{
			UpdateResizing(MousePos);
		}
		else if (_IsSelecting)
		{
			UpdateSelection(MousePos);
		}
		else
		{
			Track^ Trk		= _Timeline->GetTrackAtPoint(MousePos);
			BarEvent^ Bar	= _Timeline->GetBarAtPoint(MousePos);

			if (Trk != nullptr && Bar != nullptr)
			{
				Cursor = IsOverHandle(MousePos, Bar, Trk) ? TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			}
			else
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			}
		}

		_Timeline->Invalidate();
	}

	void DurationTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (_IsResizing)
		{
			FinishResizing();
		}
		else if (_IsSelecting)
		{
			EndSelection();
		}
	}

	void DurationTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Escape)
		{
			if (_IsResizing)
			{
				CancelResizing();
			}
			else if (_IsSelecting)
			{
				EndSelection();
			}
		}
	}

	void DurationTool::OnKeyUp(KeyEventArgs^ e) {}

	void DurationTool::OnCommandStateChanged()
	{
		if (_SelectedBars != nullptr)
		{
			List<BarEvent^>^ ExistingBars = gcnew List<BarEvent^>();

			for each (BarEvent ^ Bar in _SelectedBars)
			{
				if (_Timeline->DoesBarExist(Bar))
				{
					ExistingBars->Add(Bar);
				}
			}

			_SelectedBars = ExistingBars;
			_Timeline->Invalidate();
		}
	}

	void DurationTool::StartResizing(Point mousePos)
	{
		_PreviewBars->Clear();

		for each (BarEvent^ Bar in _SelectedBars)
		{
			BarEvent^ PreviewBar = TimelineCommandManager::CreateBarCopy(Bar, Bar->StartTick, false);
			_PreviewBars->Add(PreviewBar);

			Bar->IgnoreForOverlap = true;
		}

		StoreOriginalLengths();

		_IsResizing = true;
		_DragStartX = mousePos.X;

		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
	}

	void DurationTool::UpdateResizing(Point mousePos)
	{
		// Calculate length change
		int DeltaPixels = mousePos.X - _DragStartX;
		int DeltaTicks = _Timeline->PixelsToTicks(DeltaPixels);

		// Apply length change to all selected bars
		for each (BarEvent^ PreviewBar in _PreviewBars)
		{
			int OriginalLength = _OriginalLengths[PreviewBar];
			int NewLength;

			if (Control::ModifierKeys == Keys::Control)
			{
				NewLength = _Timeline->SnapTickToGrid(OriginalLength + DeltaTicks);	// Snap to grid
			}
			else
			{
				NewLength = OriginalLength + DeltaTicks;													// Direct length change by delta
				NewLength = ((NewLength + _ChangeTickLength / 2) / _ChangeTickLength) * _ChangeTickLength;	// Round to nearest changeTickLength
			}

			
			NewLength = Math::Max(NewLength, MIN_LENGTH_TICKS);	// Ensure minimum length
			
			Track^ Trk = PreviewBar->ContainingTrack;			// Find maximum possible length before next bar

			if (Trk != nullptr)
			{
				int MaxLength = NewLength;
				for each (BarEvent^ ExistingBar in Trk->Events)
				{
					// Skip comparing with selected bars and itself
					if (_SelectedBars->Contains(ExistingBar)) {
						continue;
					}

					// If bar starts after our bar, check if it limits our length
					if (ExistingBar->StartTick > PreviewBar->StartTick)
					{
						int PossibleLength = ExistingBar->StartTick - PreviewBar->StartTick;
						if (PossibleLength < MaxLength)
						{
							// Snap the length to grid if Ctrl is held
							MaxLength = Control::ModifierKeys == Keys::Control ? _Timeline->SnapTickToGrid(PossibleLength) : ((PossibleLength + _ChangeTickLength / 2) / _ChangeTickLength) * _ChangeTickLength;
						}
					}
				}

				// Apply the maximum possible length
				PreviewBar->Duration = MaxLength;
			}
			else
			{
				PreviewBar->Duration = NewLength;
			}
		}

		_Timeline->Invalidate();
	}

	void DurationTool::FinishResizing()
	{
		for (int i = 0; i < _SelectedBars->Count; i++)
		{
			_SelectedBars[i]->Duration = _PreviewBars[i]->Duration;
			_SelectedBars[i]->IgnoreForOverlap = false;
		}
		
		if (_SelectedBars->Count > 1)
		{
			// Create a compound command for multiple bar resizes
			CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Resize Multiple Bars");

			for each (BarEvent^ Bar in _SelectedBars)
			{
				ResizeBarCommand^ Cmd = gcnew ResizeBarCommand(_Timeline, Bar, Bar->OriginalDuration, Bar->Duration);
				CompoundCmd->AddCommand(Cmd);
			}

			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}
		else if (_SelectedBars->Count == 1)
		{
			// Single bar resize
			BarEvent^ Bar = _SelectedBars[0];
			ResizeBarCommand^ Cmd = gcnew ResizeBarCommand(_Timeline, Bar, Bar->OriginalDuration, Bar->Duration);
			_Timeline->CommandManager()->ExecuteCommand(Cmd);
		}

		_IsResizing = false;
		_TargetBar = nullptr;
		_TargetTrack = nullptr;
		_OriginalLengths->Clear();
	}

	void DurationTool::CancelResizing()
	{
		for each(BarEvent^ Bar in _SelectedBars) {
			Bar->IgnoreForOverlap = false;
		}

		_PreviewBars->Clear();
		_SelectedBars->Clear();
		_OriginalLengths->Clear();
		_IsResizing		= false;
		_TargetBar		= nullptr;
		_TargetTrack	= nullptr;

		_Timeline->Invalidate();
	}

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
				mousePos.Y >= TrackContentBounds.Y + Timeline_Direct2DRenderer::TRACK_PADDING &&
				mousePos.Y <= TrackContentBounds.Bottom - Timeline_Direct2DRenderer::TRACK_PADDING;
	}

	void DurationTool::StoreOriginalLengths()
	{
		_OriginalLengths->Clear();

		for each (BarEvent^ Bar in _PreviewBars)
		{
			_OriginalLengths->Add(Bar, Bar->Duration);
		}
	}


	//////////////////////////////
	// ColorTool Implementation //
	//////////////////////////////
	ColorTool::ColorTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor			= TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

		_HoverBar		= nullptr;
		_CurrentColor	= Color::FromArgb(255, 100, 100, 255); // Default color

		_CanSelectWithRectangle = true;
	}

	void ColorTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH)
		{
			Point ClickPoint(e->X, e->Y);

			BarEvent^ ClickedBar = _Timeline->GetBarAtPoint(ClickPoint);

			// If clicking on a selected bar, apply color to all selected bars
			if (ClickedBar != nullptr && _SelectedBars->Contains(ClickedBar)) {
				ApplyColorToSelectedBars();

				ClearSelection();
				ClearHoverPreview();
			}
			// If clicking on an unselected bar, clear selection and select only this bar
			else if (ClickedBar != nullptr && _SelectedBars->Count > 0)
			{
				ClearSelection();

				UpdateHoverPreview(ClickPoint);
			}
			// Clicking directly on the hovered bar -> delete directly, now double click needed
			else if (ClickedBar != nullptr && ClickedBar == _HoverBar)
			{
				ClearSelection();
				_SelectedBars->Add(ClickedBar);

				ApplyColorToSelectedBars();
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

	void ColorTool::OnCommandStateChanged()
	{
		if (_SelectedBars != nullptr)
		{
			List<BarEvent^>^ ExistingBars = gcnew List<BarEvent^>();

			for each (BarEvent ^ Bar in _SelectedBars)
			{
				if (_Timeline->DoesBarExist(Bar))
				{
					ExistingBars->Add(Bar);
				}
			}

			_SelectedBars = ExistingBars;
			_Timeline->Invalidate();
		}
	}

	void ColorTool::ApplyColorToSelectedBars()
	{
		if (_SelectedBars->Count == 0) {
			return;
		}

		if (_SelectedBars->Count == 1)
		{
			if (_SelectedBars[0]->Type == BarEventType::Fade)
			{
				ChangeFadeBarColorCommand^ Cmd = ApplyColorToSelectedBarsFade(_SelectedBars[0]);

				if (Cmd != nullptr) {
					_Timeline->CommandManager()->ExecuteCommand(Cmd);
				}
			}
			else if (_SelectedBars[0]->Type == BarEventType::Solid || _SelectedBars[0]->Type == BarEventType::Strobe)
			{
				// Regular or Strobe bar - simple color change
				ChangeBarColorCommand^ Cmd = ApplyColorToSelectedBarsSolid(_SelectedBars[0]);

				_Timeline->CommandManager()->ExecuteCommand(Cmd);				
			}
		}
		else
		{
			// Create a compound command for multiple bars
			CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Colors");

			for each(BarEvent^ Bar in _SelectedBars)
			{
				if (Bar->Type == BarEventType::Fade)
				{
					ChangeFadeBarColorCommand^ Cmd = ApplyColorToSelectedBarsFade(Bar);

					if(Cmd != nullptr) {
						CompoundCmd->AddCommand(Cmd);
					}
				}
				else if (Bar->Type == BarEventType::Solid || Bar->Type == BarEventType::Strobe)
				{
					// Regular or Strobe bar - simple color change
					ChangeBarColorCommand^ Cmd = ApplyColorToSelectedBarsSolid(Bar);

					CompoundCmd->AddCommand(Cmd);
				}
			}

			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}

		ClearSelection();
	}

	ChangeBarColorCommand^ ColorTool::ApplyColorToSelectedBarsSolid(BarEvent^ bar)
	{
		ChangeBarColorCommand^ Cmd = gcnew ChangeBarColorCommand(_Timeline, bar, bar->Color, _CurrentColor);
		
		return Cmd;
	}

	ChangeFadeBarColorCommand^ ColorTool::ApplyColorToSelectedBarsFade(BarEvent^ bar)
	{
		ChangeFadeBarColorCommand^ Cmd = nullptr;
		
		if (bar->FadeInfo->Type == FadeType::Two_Colors)
		{
			if (_BarXHoverRatio <= 0.5f)
			{
				Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, bar, ChangeFadeBarColorCommand::ColorType::Start, bar->FadeInfo->ColorStart, CurrentColor);
			}
			else
			{
				Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, bar, ChangeFadeBarColorCommand::ColorType::End, bar->FadeInfo->ColorEnd, CurrentColor);
			}
		}
		else if (bar->FadeInfo->Type == FadeType::Three_Colors)
		{
			if (_BarXHoverRatio <= 0.33f)
			{
				Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, bar, ChangeFadeBarColorCommand::ColorType::Start, bar->FadeInfo->ColorStart, CurrentColor);
			}
			else if (_BarXHoverRatio <= 0.66f)
			{
				Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, bar, ChangeFadeBarColorCommand::ColorType::Center, bar->FadeInfo->ColorCenter, CurrentColor);
			}
			else
			{
				Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, bar, ChangeFadeBarColorCommand::ColorType::End, bar->FadeInfo->ColorEnd, CurrentColor);
			}
		}

		return Cmd;
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

			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross); // Use cross cursor for selection

			return;
		}

		// Calculate preview rectangle
		int XStart	= _Timeline->TicksToPixels(Bar->StartTick) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;
		int XEnd	= _Timeline->TicksToPixels(Bar->EndTick) + _Timeline->ScrollPosition->X + Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH;

		this->_BarXHoverRatio = ((float)(mousePos.X - XStart)) / ((float)(XEnd - XStart));

		// Update hover state
		if (Bar != _HoverBar)
		{
			this->_HoverBar = Bar;

			// Update cursor based on whether the bar is selected
			Cursor = this->_SelectedBars->Contains(Bar) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		}

		_Timeline->Invalidate();
	}

	void ColorTool::ClearHoverPreview()
	{
		_HoverBar = nullptr;
		_Timeline->Invalidate();
	}

	void ColorTool::CurrentColor::set(Color value)
	{
		_CurrentColor = value;

		if (_SelectedBars->Count > 0) {
			ApplyColorToSelectedBars();
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

				
				BarEvent^ PreviewBar = gcnew BarEvent(nullptr, _StartTick, this->TickLength, FadeInfo);
				_PreviewBars->Clear();
				_PreviewBars->Add(PreviewBar);

				if (!HasOverlappingBarsOnSpecificTrack(TargetTrack, _PreviewBars[0]))
				{
					_IsDrawing = true;
					_DrawStart = MousePos;
					_TargetTrack = TargetTrack;
					_Timeline->Invalidate();
				}
				else
				{
					_PreviewBars->Clear();
					//_PreviewBar = nullptr;
				}	
			}
		}
	}

	void FadeTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);
		_LastMousePos = MousePos;

		if (_IsDrawing && _TargetTrack != nullptr && _PreviewBars->Count > 0)
		{
			// Calculate total distance in ticks
			int CurrentTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));
			int TotalTicks = CurrentTick - _StartTick;

			if (TotalTicks > 0)
			{
				int OriginalDuration = _PreviewBars[0]->Duration;
				
				// Update preview bar duration
				_PreviewBars[0]->Duration = TotalTicks;

				if (HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBars[0])) {
					// If there's an overlap, revert to the previous duration
					_PreviewBars[0]->Duration = OriginalDuration;
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
		if (_IsDrawing && _PreviewBars->Count > 0)
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

			// Clear any previews and redraw
			ClearPreviews();
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

			// Update cursor
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

			// Clear any previews and redraw
			ClearPreviews();
		}
	}

	void FadeTool::OnKeyUp(KeyEventArgs^ e) {}

	void FadeTool::TickLength::set(int value)
	{
		_TickLength = value;
		
		if (_PreviewBars->Count > 0 && _PreviewBars[0]->FadeInfo != nullptr)
		{
			_PreviewBars[0]->FadeInfo->QuantizationTicks = value;
			_Timeline->Invalidate();
		}
	}

	void FadeTool::ColorStart::set(Color color)
	{
		_ColorStart = color;

		UpdatePreviewColors();
	}

	void FadeTool::ColorCenter::set(Color color)
	{
		_ColorCenter = color;

		UpdatePreviewColors();
	}

	void FadeTool::ColorEnd::set(Color color)
	{
		_ColorEnd = color;

		UpdatePreviewColors();
	}

	void FadeTool::Type::set(FadeType type)
	{
		_Type = type;

		UpdatePreviewColors();
	}

	void FadeTool::AddBarToTrack()
	{
		if (_PreviewBars->Count == 0 || _TargetTrack == nullptr) {
			return;
		}

		if (!HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBars[0]))
		{
			BarEventFadeInfo^ FadeInfo = nullptr;
			if (this->Type == FadeType::Two_Colors) {
				FadeInfo = gcnew BarEventFadeInfo(this->TickLength, this->ColorStart, this->ColorEnd);
			}
			else if (this->Type == FadeType::Three_Colors) {
				FadeInfo = gcnew BarEventFadeInfo(this->TickLength, this->ColorStart, this->ColorCenter, this->ColorEnd);
			}

			// Create and execute the command
			AddFadeBarCommand^ Cmd = gcnew AddFadeBarCommand(
				_Timeline,
				_TargetTrack,
				_PreviewBars[0]->StartTick,
				_PreviewBars[0]->Duration,
				FadeInfo
			);

			_Timeline->CommandManager()->ExecuteCommand(Cmd);
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
				if (_PreviewBars->Count > 0) {
					_PreviewBars->Clear();
					_Timeline->Invalidate();
				}
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else
			{
				if (_PreviewBars->Count == 0) {
					BarEvent^ PreviewBar = gcnew BarEvent(nullptr, SnapTick, this->TickLength, this->ColorStart);
					_PreviewBars->Add(PreviewBar);
				}
				else {
					_PreviewBars[0]->StartTick = SnapTick;
					_PreviewBars[0]->Duration = this->TickLength;
					_PreviewBars[0]->Color = this->ColorStart;
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
		if (_PreviewBars->Count > 0)
		{
			_PreviewBars->Clear();
			_Timeline->Invalidate();
		}
	}

	void FadeTool::UpdatePreviewColors()
	{
		if (_PreviewBars->Count > 0 && _PreviewBars[0]->FadeInfo != nullptr)
		{
			if (_Type == FadeType::Two_Colors)
			{
				_PreviewBars[0]->FadeInfo->ColorStart = _ColorStart;
				_PreviewBars[0]->FadeInfo->ColorEnd = _ColorEnd;
			}
			else
			{
				_PreviewBars[0]->FadeInfo->ColorStart = _ColorStart;
				_PreviewBars[0]->FadeInfo->ColorCenter = _ColorCenter;
				_PreviewBars[0]->FadeInfo->ColorEnd = _ColorEnd;
			}

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

			if (TargetTrack != nullptr)
			{
				// Calculate start tick with grid snap
				_StartTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));
				
				// Create initial preview bar with fade info
				BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(this->TickLength, this->ColorStrobe);

				BarEvent^ PreviewBar = gcnew BarEvent(nullptr, _StartTick, this->TickLength, StrobeInfo);
				_PreviewBars->Clear();
				_PreviewBars->Add(PreviewBar);

				if (!HasOverlappingBarsOnSpecificTrack(TargetTrack, _PreviewBars[0]))
				{
					_IsDrawing = true;
					_DrawStart = MousePos;
					_TargetTrack = TargetTrack;
					_Timeline->Invalidate();
				}
				else
				{
					_PreviewBars->Clear();
				}
			}
		}
	}

	void StrobeTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos(e->X, e->Y);
		_LastMousePos = MousePos;

		if (_IsDrawing && _TargetTrack != nullptr && _PreviewBars->Count > 0)
		{
			// Calculate total distance in ticks
			int CurrentTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(MousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));
			int TotalTicks = CurrentTick - _StartTick;
			
			if (TotalTicks > 0)
			{
				int OriginalDuration = _PreviewBars[0]->Duration;

				// Update preview bar duration
				_PreviewBars[0]->Duration = TotalTicks;

				if (HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBars[0])) {
					// If there's an overlap, revert to the previous duration
					_PreviewBars[0]->Duration = OriginalDuration;
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
		if (_IsDrawing && _PreviewBars->Count > 0)
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

			// Clear any previews and redraw
			ClearPreviews();
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

			// Update cursor
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

			// Clear any previews and redraw
			ClearPreviews();
		}
	}

	void StrobeTool::OnKeyUp(KeyEventArgs^ e) {}

	void StrobeTool::TickLength::set(int value)
	{
		_TickLength = value;

		if (_PreviewBars->Count > 0 && _PreviewBars[0]->StrobeInfo != nullptr)
		{
			_PreviewBars[0]->StrobeInfo->QuantizationTicks = value;
			_Timeline->Invalidate();
		}
	}

	void StrobeTool::ColorStrobe::set(Color color)
	{
		_ColorStrobe = color;

		UpdatePreviewColor();
	}

	void StrobeTool::AddBarToTrack()
	{
		if (_PreviewBars->Count == 0 || _TargetTrack == nullptr) {
			return;
		}

		if (!HasOverlappingBarsOnSpecificTrack(_TargetTrack, _PreviewBars[0]))
		{
			BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(this->TickLength, this->ColorStrobe);

			// Create and execute the command
			AddStrobeBarCommand^ Cmd = gcnew AddStrobeBarCommand(
				_Timeline,
				_TargetTrack,
				_PreviewBars[0]->StartTick,
				_PreviewBars[0]->Duration,
				StrobeInfo
			);

			_Timeline->CommandManager()->ExecuteCommand(Cmd);
		}
	}

	void StrobeTool::UpdateSinglePreview(Point mousePos)
	{
		Track^ TargetTrack = _Timeline->GetTrackAtPoint(mousePos);

		if (TargetTrack != nullptr && !_IsDrawing)
		{
			int SnapTick = _Timeline->SnapTickToGrid(_Timeline->PixelsToTicks(mousePos.X - Timeline_Direct2DRenderer::TRACK_HEADER_WIDTH - _Timeline->ScrollPosition->X));

			BarEvent^ TempBar = gcnew BarEvent(TargetTrack, SnapTick, this->TickLength, this->ColorStrobe);

			if (HasOverlappingBarsOnBarTrack(TempBar))
			{
				// If there would be an overlap, clear any existing preview and show "no" cursor
				if (_PreviewBars->Count > 0)
				{
					_PreviewBars->Clear();
					_Timeline->Invalidate();
				}
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else
			{
				if (_PreviewBars->Count == 0) {
					BarEvent^ PreviewBar = gcnew BarEvent(nullptr, SnapTick, this->TickLength, this->ColorStrobe);
					_PreviewBars->Add(PreviewBar);
				}
				else {
					_PreviewBars[0]->StartTick	= SnapTick;
					_PreviewBars[0]->Duration	= this->TickLength;
					_PreviewBars[0]->Color		= this->ColorStrobe;
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
		if (_PreviewBars->Count > 0)
		{
			_PreviewBars->Clear();
			_Timeline->Invalidate();
		}
	}

	void StrobeTool::UpdatePreviewColor()
	{
		if (_PreviewBars->Count > 0 && _PreviewBars[0]->StrobeInfo != nullptr)
		{
			_PreviewBars[0]->StrobeInfo->ColorStrobe = _ColorStrobe;
			_Timeline->Invalidate();
		}
	}
}
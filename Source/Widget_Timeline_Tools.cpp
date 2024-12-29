#include "Widget_Timeline_Tools.h"
#include "Widget_Timeline.h"

namespace MIDILightDrawer
{
	// Base TimelineTool Implementation
	TimelineTool::TimelineTool(Widget_Timeline^ timeline) : timeline(timeline)
	{
		isActive = false;
		lastMousePos = Point(0, 0);
		modifierKeys = Keys::None;
	}

	void TimelineTool::Cursor::set(System::Windows::Forms::Cursor^ value)
	{
		currentCursor = value;

		if (timeline != nullptr) {
			timeline->Cursor = value;
		}
	}

	////////////////////////////////
	// PointerTool Implementation //
	////////////////////////////////
	PointerTool::PointerTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);

		isDragging = false;
		dragStart = nullptr;
		dragSourceTrack = nullptr;
		dragTargetTrack = nullptr;
		selectedBars = gcnew List<BarEvent^>();
	}

	

	void PointerTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left)
		{
			if (isPasting)
			{
				Track^ track = timeline->GetTrackAtPoint(Point(e->X, e->Y));
				if (track != nullptr && e->X > Widget_Timeline::TRACK_HEADER_WIDTH) {
					FinalizePaste();
				}
				return;
			}
			
			dragStart = gcnew Point(e->X, e->Y);
			Track^ track = timeline->GetTrackAtPoint(*dragStart);

			// Handle different click scenarios
			if (track != nullptr)
			{
				// Click in track header - select track
				if (e->X <= Widget_Timeline::TRACK_HEADER_WIDTH)
				{
					timeline->SelectedTrack = track;
					timeline->SelectedBar = nullptr;
					selectedBars->Clear();
				}
				// Click in track content - handle bars
				else
				{
					BarEvent^ bar = timeline->GetBarAtPoint(*dragStart);
					if (bar != nullptr)
					{
						if (Control::ModifierKeys == Keys::Control)
						{
							// Toggle selection with Ctrl key
							if (selectedBars == nullptr) {
								selectedBars = gcnew List<BarEvent^>();
							}

							if (selectedBars->Contains(bar))
							{
								selectedBars->Remove(bar);
								if (timeline->SelectedBar == bar) {
									timeline->SelectedBar = selectedBars->Count > 0 ? selectedBars[0] : nullptr;
								}
							}
							else {
								selectedBars->Add(bar);
								timeline->SelectedBar = bar;
							}
						}
						else
						{
							// Check if clicking on an already selected bar
							if (selectedBars != nullptr && selectedBars->Contains(bar))
							{
								// Start group drag
								isDragging = true;
								dragSourceTrack = track;
								StoreOriginalPositions();
							}
							else
							{
								// Clear previous selection and select only this bar
								if (selectedBars == nullptr) {
									selectedBars = gcnew List<BarEvent^>();
								}

								selectedBars->Clear();
								selectedBars->Add(bar);
								timeline->SelectedBar = bar;
								isDragging = true;
								dragSourceTrack = track;
								StoreOriginalPositions();
							}
						}
					}
					else {
						// Start selection rectangle if clicking empty space
						if (!Control::ModifierKeys.HasFlag(Keys::Control)) {
							selectedBars->Clear();
							timeline->SelectedBar = nullptr;
						}
						StartSelection(*dragStart);
						isSelecting = true;
					}
				}
				timeline->Invalidate();
			}
		}
	}

	void PointerTool::OnMouseMove(MouseEventArgs^ e)
	{
		lastMousePos = Point(e->X, e->Y);  // Store current mouse position

		if (isPasting) {
			UpdatePastePreview(lastMousePos);
			return;
		}

		if (isDragging && selectedBars != nullptr && selectedBars->Count > 0)
		{
			// Calculate movement in ticks
			int pixelDelta	= e->X - dragStart->X;
			int tickDelta	= timeline->PixelsToTicks(pixelDelta);

			// Update positions of selected bars
			for each (BarEvent ^ bar in selectedBars)
			{
				if(selectedBars->IndexOf(bar) == 0) {
					// Use SnapTickBasedOnType for the first bar
					bar->StartTick = timeline->SnapTickBasedOnType(bar->OriginalStartTick + tickDelta,	Point(e->X, e->Y));
				}
				else {
					int TickOffsetToFirstBar = bar->OriginalStartTick - selectedBars[0]->OriginalStartTick;
					bar->StartTick = selectedBars[0]->StartTick + TickOffsetToFirstBar;
				}
			}

			// Only update target track if not over header area and not multi-track selection
			if (!IsMultiTrackSelection && e->X > Widget_Timeline::TRACK_HEADER_WIDTH) {
				dragTargetTrack = timeline->GetTrackAtPoint(Point(e->X, e->Y));
			}

			timeline->Invalidate();
		}
		else if (isSelecting)
		{
			UpdateSelection(Point(e->X, e->Y));
			timeline->Invalidate();
		}

		// Update cursor based on position
		UpdateCursor(Point(e->X, e->Y));
		timeline->Invalidate();
	}

	void PointerTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (isDragging)
		{
			// Don't allow track changes for multi-track selection
			if (!IsMultiTrackSelection)
			{
				Track^ targetTrack = timeline->GetTrackAtPoint(Point(e->X, e->Y));

				if (targetTrack != nullptr)
				{
					// First remove bars from source track
					for each (BarEvent ^ bar in selectedBars)
					{
						dragSourceTrack->Events->Remove(bar);
					}

					// Then add them to target track (whether it's the same or different)
					for each (BarEvent ^ bar in selectedBars) {
						targetTrack->Events->Add(bar);
					}

					// Ensure bars are properly sorted in the target track
					targetTrack->Events->Sort(Track::barComparer);
				}
				else
				{
					// If dropped outside a track, move bars back to original positions
					for each (BarEvent ^ bar in selectedBars) {
						bar->StartTick = bar->OriginalStartTick;
					}
				}
			}
			else {
				// For multi-track selection, just update positions
				for each (BarEvent ^ bar in selectedBars) {
					bar->StartTick = timeline->SnapTickToGrid(bar->StartTick);
				}
			}

			isDragging = false;
			dragSourceTrack = nullptr;
			dragTargetTrack = nullptr;
		}
		else if (isSelecting) {
			EndSelection();
			isSelecting = false;
		}

		dragStart = nullptr;
		timeline->Invalidate();
	}

	void PointerTool::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->Control) {
			if (e->KeyCode == Keys::C) {
				HandleCopy();
			}
			else if (e->KeyCode == Keys::V) {
				HandlePaste();
			}
		}
		else if (e->KeyCode == Keys::Escape && isPasting) {
			CancelPaste();
		}
	}

	void PointerTool::OnKeyUp(KeyEventArgs^ e)
	{
	
	}

	void PointerTool::StartSelection(Point start) {
		selectionRect = Rectangle(start.X, start.Y, 0, 0);

		if (selectedBars == nullptr) {
			selectedBars = gcnew List<BarEvent^>();
		}
		else {
			selectedBars->Clear();
		}
	}

	void PointerTool::UpdateSelection(Point current)
	{
		if (dragStart == nullptr) return;

		// Calculate rectangle from start point to current point
		int x		= Math::Min(dragStart->X, current.X);
		int y		= Math::Min(dragStart->Y, current.Y);
		int width	= Math::Abs(current.X - dragStart->X);
		int height	= Math::Abs(current.Y - dragStart->Y);

		selectionRect = Rectangle(x, y, width, height);

		// Update selection if dragging
		if (width > 0 && height > 0) {
			SelectBarsInRegion(selectionRect);
		}
	}

	void PointerTool::EndSelection()
	{
		// Finalize the selection
		if (selectedBars != nullptr && selectedBars->Count > 0) {
			// Set the last selected bar as the primary selection
			timeline->SelectedBar = selectedBars[selectedBars->Count - 1];
		}
		else {
			timeline->SelectedBar = nullptr;
		}

		// Clear the selection rectangle
		selectionRect = Rectangle(0, 0, 0, 0);
	}

	void PointerTool::SelectBarsInRegion(Rectangle region)
	{
		selectedBars->Clear();

		// Convert selection rectangle to tick range
		int startTick = timeline->PixelsToTicks(
			region.Left - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int endTick = timeline->PixelsToTicks(
			region.Right - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track ^ track in timeline->Tracks) {
			Rectangle trackBounds = timeline->GetTrackContentBounds(track);
			if (trackBounds.IntersectsWith(region)) {
				for each (BarEvent ^ bar in track->Events) {
					// Check if bar intersects with selection
					if (bar->StartTick + bar->Duration >= startTick &&
						bar->StartTick <= endTick) {
						selectedBars->Add(bar);
					}
				}
			}
		}
	}

	void PointerTool::HandleCopy()
	{
		if (selectedBars == nullptr || selectedBars->Count == 0) return;

		// Clear existing clipboard content
		TimelineClipboardManager::Clear();

		// Find the earliest start tick among selected bars
		int earliestTick = Int32::MaxValue;
		for each(BarEvent ^ bar in selectedBars) {
			earliestTick = Math::Min(earliestTick, bar->StartTick);
		}

		// Create copies of selected bars with relative positions and track information
		for each(Track ^ track in timeline->Tracks) {
			int trackIndex = timeline->Tracks->IndexOf(track);

			for each(BarEvent ^ bar in track->Events) {
				if (selectedBars->Contains(bar)) {
					BarEvent^ copy = gcnew BarEvent(
						bar->StartTick - earliestTick,  // Store relative position
						bar->Duration,
						bar->Color
					);

					TimelineClipboardManager::Content->Add(
						gcnew TimelineClipboardItem(copy, trackIndex)
					);
				}
			}
		}
	}

	void PointerTool::HandlePaste()
	{
		if (TimelineClipboardManager::Content->Count == 0) return;

		// Create preview bars
		pastePreviewBars = gcnew List<BarEvent^>();

		// Calculate paste position based on mouse position
		int mouseTick = timeline->PixelsToTicks(
			lastMousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		pasteStartTick = timeline->SnapTickToGrid(mouseTick);

		// Calculate track offset based on where we're pasting
		Track^ targetTrack = timeline->GetTrackAtPoint(lastMousePos);
		int targetTrackIndex = timeline->Tracks->IndexOf(targetTrack);

		// Create preview bars
		for each(TimelineClipboardItem ^ item in TimelineClipboardManager::Content) {
			// Calculate destination track index
			int destTrackIndex = targetTrackIndex + (item->TrackIndex -
				TimelineClipboardManager::Content[0]->TrackIndex);

			// Skip if destination track doesn't exist
			if (destTrackIndex < 0 || destTrackIndex >= timeline->Tracks->Count) {
				continue;
			}

			BarEvent^ previewBar = gcnew BarEvent(
				pasteStartTick + item->Bar->StartTick,
				item->Bar->Duration,
				Color::FromArgb(128, item->Bar->Color) // Semi-transparent
			);
			pastePreviewBars->Add(previewBar);
		}

		// Enter paste mode
		isPasting = true;
		pasteTargetTrack = targetTrack;

		// Clear existing selection
		selectedBars->Clear();

		// Update cursor
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

		timeline->Invalidate();
	}

	void PointerTool::UpdatePastePreview(Point mousePos)
	{
		if (!isPasting || pastePreviewBars == nullptr) return;

		// Calculate new target track and validate position
		Track^ newTargetTrack = timeline->GetTrackAtPoint(mousePos);
		if (newTargetTrack == nullptr || mousePos.X <= Widget_Timeline::TRACK_HEADER_WIDTH) {
			return;
		}

		// Calculate new position in ticks
		int mouseTick		= timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int newStartTick	= timeline->SnapTickBasedOnType(mouseTick, mousePos);

		// Calculate the track offset
		int targetTrackIndex = timeline->Tracks->IndexOf(newTargetTrack);
		int sourceTrackIndex = TimelineClipboardManager::Content[0]->TrackIndex;

		// Clear existing preview bars and create new ones
		pastePreviewBars->Clear();

		// Create preview bars for each clipboard item
		for each(TimelineClipboardItem ^ item in TimelineClipboardManager::Content)
		{
			// Calculate destination track index for this bar
			int destTrackIndex = targetTrackIndex + (item->TrackIndex - sourceTrackIndex);

			// Skip if destination track would be out of bounds
			if (destTrackIndex < 0 || destTrackIndex >= timeline->Tracks->Count) {
				continue;
			}

			// Create preview bar with track-specific information
			BarEvent^ previewBar = gcnew BarEvent(newStartTick + item->Bar->StartTick, item->Bar->Duration, Color::FromArgb(128, item->Bar->Color));

			// Store the target track index in the bar's OriginalStartTick
			// We'll use this for rendering the preview in the correct track
			previewBar->OriginalStartTick = destTrackIndex;

			pastePreviewBars->Add(previewBar);
		}

		// Update target track
		pasteTargetTrack = newTargetTrack;

		timeline->Invalidate();;
	}

	void PointerTool::CancelPaste() 
	{
		isPasting = false;
		pastePreviewBars = nullptr;
		pasteTargetTrack = nullptr;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		timeline->Invalidate();
	}

	void PointerTool::FinalizePaste()
	{
		if (!isPasting || pasteTargetTrack == nullptr) {
			CancelPaste();
			return;
		}

		// Clear existing selection
		selectedBars->Clear();

		// Calculate track offset
		int targetTrackIndex = timeline->Tracks->IndexOf(pasteTargetTrack);
		int sourceTrackIndex = TimelineClipboardManager::Content[0]->TrackIndex;

		// Create final bars
		int i = 0;
		for each(TimelineClipboardItem ^ item in TimelineClipboardManager::Content)
		{
			// Calculate destination track index
			int destTrackIndex = targetTrackIndex + (item->TrackIndex - sourceTrackIndex);

			// Skip if destination track doesn't exist
			if (destTrackIndex < 0 || destTrackIndex >= timeline->Tracks->Count) {
				continue;
			}

			Track^ destTrack = timeline->Tracks[destTrackIndex];
			BarEvent^ previewBar = pastePreviewBars[i++];

			BarEvent^ newBar = gcnew BarEvent(previewBar->StartTick, previewBar->Duration, Color::FromArgb(255, (int)previewBar->Color.R, (int)previewBar->Color.G, (int)previewBar->Color.B));

			// Add to appropriate track and selection
			destTrack->Events->Add(newBar);
			selectedBars->Add(newBar);
		}

		// Sort events in all affected tracks
		for each(Track ^ track in timeline->Tracks)
		{
			track->Events->Sort(Track::barComparer);
		}

		// Exit paste mode
		CancelPaste();
	}

	bool PointerTool::Is_Multi_Track_Selection()
	{
		if (selectedBars == nullptr || selectedBars->Count <= 1) return false;

		Track^ firstTrack = nullptr;
		for each (Track ^ track in timeline->Tracks)
		{
			for each (BarEvent ^ bar in track->Events)
			{
				if (selectedBars->Contains(bar))
				{
					if (firstTrack == nullptr) {
						firstTrack = track;
					}
					else if (track != firstTrack) {
						return true;
					}
				}
			}
		}
		return false;
	}

	void PointerTool::StoreOriginalPositions()
	{
		for each (BarEvent ^ bar in selectedBars) {
			bar->OriginalStartTick = bar->StartTick;
		}
	}

	void PointerTool::UpdateGroupPosition(int tickDelta, bool allowTrackChange)
	{
		for each (BarEvent ^ bar in selectedBars) {
			int newPosition = bar->OriginalStartTick + tickDelta;
			bar->StartTick = timeline->SnapTickToGrid(newPosition);
		}
	}

	void PointerTool::MoveSelectedBarsToTrack(Track^ targetTrack)
	{
		// First, remove bars from source track
		for each (BarEvent ^ bar in selectedBars) {
			dragSourceTrack->Events->Remove(bar);
		}

		// Add bars to target track
		for each (BarEvent ^ bar in selectedBars) {
			targetTrack->Events->Add(bar);
		}

		// Resort the events in the target track
		targetTrack->Events->Sort(Track::barComparer);
	}

	void PointerTool::UpdateCursor(Point mousePos)
	{
		if (mousePos.X <= Widget_Timeline::TRACK_HEADER_WIDTH) {
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		}
		else {
			BarEvent^ hoverBar = timeline->GetBarAtPoint(mousePos);
			Cursor = (hoverBar != nullptr) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
		}
	}

	/////////////////////////////
	// DrawTool Implementation //
	/////////////////////////////
	DrawTool::DrawTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		
		targetTrack		= nullptr;
		sourceTrack		= nullptr;
		previewBar		= nullptr;
		_Current_Color	= Color::FromArgb(200, 100, 100, 255);
		_Draw_Tick_Length	= 960;
		_Use_Auto_Length	= false;

		isPainting		= false;
		isErasing		= false;
		isMoving		= false;
		isResizing		= false;
		lastPaintedTick = 0;
		hoverBar		= nullptr;
		currentMode		= DrawMode::Draw;

		timeline->UpdateCursor(currentCursor);
	}

	void DrawTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Point mousePos(e->X, e->Y);
			targetTrack = timeline->GetTrackAtPoint(mousePos);

			if (targetTrack != nullptr) {
				switch (currentMode) {
				case DrawMode::Draw:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
					StartPainting(mousePos);
					break;

				case DrawMode::Erase:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
					StartErasing(mousePos);
					break;

				case DrawMode::Move:
					this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
					StartMoving(mousePos);
					break;

				case DrawMode::Resize:
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
		lastMousePos = mousePos;

		if (e->X <= Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			ClearPreview();
			return;
		}

		if (e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			// Handle active operations
			if (e->Button == Windows::Forms::MouseButtons::Left) {
				if (isResizing) {
					currentMode = DrawMode::Resize;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
					UpdateResizing(mousePos);
				}
				else if (isMoving) {
					currentMode = DrawMode::Move;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
					UpdateMoving(mousePos);
				}
				else {
					switch (currentMode) {
					case DrawMode::Draw:
						UpdatePainting(mousePos);
						break;
					case DrawMode::Erase:
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
		switch (currentMode)
		{
		case DrawMode::Move:
			FinishMoving();
			break;

		case DrawMode::Resize:
			FinishResizing();
			break;
		}

		isPainting	= false;
		isErasing	= false;
		isMoving	= false;
		isResizing	= false;

		UpdateCurrentMode(Point(e->X, e->Y));
	}

	void DrawTool::OnKeyDown(KeyEventArgs^ e) {}
	void DrawTool::OnKeyUp(KeyEventArgs^ e) {}

	void DrawTool::UpdatePreview(Point mousePos)
	{
		Track^ track = timeline->GetTrackAtPoint(mousePos);

		if (track != nullptr && mousePos.X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			int Raw_Tick		= timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
			int Start_Tick		= timeline->SnapTickBasedOnType(Raw_Tick, mousePos);
			int Length_In_Ticks = _Draw_Tick_Length;

			if (_Use_Auto_Length) {
				Length_In_Ticks = GetBeatLength(track, Raw_Tick);
			}

			switch (currentMode)
			{
				case DrawMode::Draw:
					if (!HasOverlappingBars(track, Start_Tick, Length_In_Ticks))
					{
						if (previewBar == nullptr)
						{
							previewBar = gcnew BarEvent(Start_Tick, Length_In_Ticks, Color::FromArgb(100, (int)_Current_Color.R, (int)_Current_Color.G, (int)_Current_Color.B));
						}
						else {
							previewBar->StartTick	= Start_Tick;
							previewBar->Duration	= Length_In_Ticks;
						}
					}
					else {
						previewBar = nullptr;
					}
					break;

				case DrawMode::Move:
				case DrawMode::Erase:
				case DrawMode::Resize:
					previewBar = nullptr;
					break;
			}

			timeline->Invalidate();
		}
		else {
			ClearPreview();
		}
	}

	void DrawTool::ClearPreview()
	{
		previewBar = nullptr;
		timeline->Invalidate();
	}

	void DrawTool::UpdateCurrentMode(Point mousePos)
	{
		if (isResizing || isMoving || isPainting) {
			UpdateActiveModesCursor();
			return;
		}

		Track^ track = timeline->GetTrackAtPoint(mousePos);
		BarEvent^ bar = nullptr;

		if (track != nullptr) {
			bar = timeline->GetBarAtPoint(mousePos);
		}

		hoverBar = bar;

		// Determine new mode based on context
		DrawMode newMode;
		System::Windows::Forms::Cursor^ newCursor;

		if (bar != nullptr) {
			// Check resize handle first - this should take priority
			if (IsOverResizeHandle(mousePos, bar, track)) {
				newMode = DrawMode::Resize;
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
			}
			else if (Control::ModifierKeys == Keys::Control) {
				newMode = DrawMode::Move;
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
			}
			else {
				newMode = DrawMode::Erase;
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
		}
		else {
			newMode = DrawMode::Draw;
			if (!HasOverlappingBars(track,
				timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X),
				_Draw_Tick_Length)) {
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);	// For drawing (when position is valid)
			}
			else {
				newCursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);	// For drawing (when position would cause overlap)
			}
		}

		// Only update mode if it changed
		if (newMode != currentMode || Cursor != newCursor) {
			currentMode = newMode;
			Cursor = newCursor;
			timeline->Invalidate();	// Refresh preview
		}
	}

	void DrawTool::UpdateActiveModesCursor()
	{
		switch (currentMode)
		{
		case DrawMode::Draw:
			this->Cursor = isPainting ? TimelineCursorHelper::GetCursor(TimelineCursor::Cross) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			break;
		case DrawMode::Move:
			this->Cursor = isMoving ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			break;
		case DrawMode::Resize:
			this->Cursor = isResizing ? TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE) : TimelineCursorHelper::GetCursor(TimelineCursor::Default);
			break;
		case DrawMode::Erase:
			this->Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
			break;
		}
	}

	bool DrawTool::IsOverResizeHandle(Point mousePos, BarEvent^ bar, Track^ track)
	{
		if (bar == nullptr || track == nullptr) return false;

		Rectangle bounds = timeline->GetTrackContentBounds(track);

		// Calculate bar end position in pixels
		int barEndX = timeline->TicksToPixels(bar->StartTick + bar->Duration) + timeline->ScrollPosition->X + Widget_Timeline::TRACK_HEADER_WIDTH;

		// Define handle region - make it a bit larger for easier interaction
		const int HANDLE_WIDTH = 5;

		// Check if mouse is within handle area
		bool isInXRange = mousePos.X >= barEndX - HANDLE_WIDTH && mousePos.X <= barEndX + HANDLE_WIDTH;

		bool isInYRange = mousePos.Y >= bounds.Y + Widget_Timeline::TRACK_PADDING && mousePos.Y <= bounds.Bottom - Widget_Timeline::TRACK_PADDING;

		return isInXRange && isInYRange;
	}

	void DrawTool::StartPainting(Point mousePos)
	{
		if (targetTrack == nullptr) {
			return;
		}
		
		isPainting = true;
		currentMode = DrawMode::Draw;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);

		int Raw_Tick		= timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int Start_Tick		= timeline->SnapTickBasedOnType(Raw_Tick, mousePos);
		int Length_In_Ticks = _Draw_Tick_Length;

		if (_Use_Auto_Length) {
			Length_In_Ticks = GetBeatLength(targetTrack, Raw_Tick);
		}

		lastPaintedTick = Start_Tick;

		// Create initial bar
		if (!HasOverlappingBars(targetTrack, Start_Tick, Length_In_Ticks)) {
			timeline->AddBarToTrack(targetTrack, Start_Tick, Length_In_Ticks, _Current_Color);
			timeline->Invalidate();
		}
	}

	void DrawTool::UpdatePainting(Point mousePos)
	{
		if (!isPainting || targetTrack == nullptr) {
			return;
		}

		int Raw_Tick		= timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int Start_Tick		= timeline->SnapTickBasedOnType(Raw_Tick, mousePos);
		int Length_In_Ticks = _Draw_Tick_Length;

		if (_Use_Auto_Length) {
			Length_In_Ticks = GetBeatLength(targetTrack, Raw_Tick);
		}

		// Only create new bar if we've moved to a new position and there's no overlap
		if (Start_Tick > lastPaintedTick)
		{
			if (!HasOverlappingBars(targetTrack, Start_Tick, Length_In_Ticks))
			{
				timeline->AddBarToTrack(targetTrack, Start_Tick, Length_In_Ticks, _Current_Color);
				lastPaintedTick = Start_Tick;
				timeline->Invalidate();
			}
		}
	}

	bool DrawTool::HasOverlappingBars(Track^ track, int startTick, int length)
	{
		if (track == nullptr) return false;

		int endTick = startTick + length;

		for each(BarEvent ^ existingBar in track->Events) {
			// Skip checking against the bar we're currently moving
			if (existingBar == hoverBar) continue;

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
		isErasing = true; 
		if (hoverBar != nullptr) {
			targetTrack->RemoveBar(hoverBar);
			timeline->Invalidate();
		}
	}

	void DrawTool::UpdateErasing(Point mousePos)
	{
		if (!isErasing) return;

		Track^ track = timeline->GetTrackAtPoint(mousePos);
		if (track != nullptr) {
			// Get the bar under the mouse cursor
			BarEvent^ bar = timeline->GetBarAtPoint(mousePos);

			// Only erase if we find a bar and haven't already erased it
			if (bar != nullptr) {
				track->RemoveBar(bar);
				timeline->Invalidate();
			}
		}
	}

	void DrawTool::StartMoving(Point mousePos)
	{
		if (hoverBar != nullptr) {
			isMoving = true;
			currentMode = DrawMode::Move;
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Hand);
			dragStartPoint = mousePos;
			dragStartTick = hoverBar->StartTick;
			hoverBar->OriginalStartTick = hoverBar->StartTick;
			targetTrack = timeline->GetTrackAtPoint(mousePos);
			sourceTrack = timeline->GetTrackAtPoint(mousePos);
		}
	}

	void DrawTool::UpdateMoving(Point mousePos)
	{
		if (!isMoving || hoverBar == nullptr || targetTrack == nullptr) {
			return;
		}

		// Calculate movement in ticks
		int pixelDelta = mousePos.X - dragStartPoint.X;
		int tickDelta = timeline->PixelsToTicks(pixelDelta);

		// Calculate new position with grid snapping
		int newStartTick = timeline->SnapTickToGrid(dragStartTick + tickDelta);

		// Store original position for overlap check
		int originalStart = hoverBar->StartTick;

		// Temporarily move to new position
		hoverBar->StartTick = newStartTick;

		// Check for overlaps with other bars
		bool hasOverlap = HasOverlappingBars(targetTrack, newStartTick, hoverBar->Duration);

		if (hasOverlap) {
			// If overlap detected, revert to original position
			hoverBar->StartTick = originalStart;
		}
		else {
			// If no overlap, check if we're over a different track
			Track^ trackUnderMouse = timeline->GetTrackAtPoint(mousePos);

			if (trackUnderMouse != nullptr && trackUnderMouse != targetTrack) {
				// Ensure the new track doesn't have overlapping bars
				if (!HasOverlappingBars(trackUnderMouse, newStartTick, hoverBar->Duration)) {
					// Remove from original track
					targetTrack->Events->Remove(hoverBar);

					// Add to new track
					trackUnderMouse->Events->Add(hoverBar);

					// Update target track
					targetTrack = trackUnderMouse;

					// Resort the events in the new track
					targetTrack->Events->Sort(Track::barComparer);
				}
				else {
					// If overlap in new track, revert position
					hoverBar->StartTick = originalStart;
				}
			}
		}

		timeline->Invalidate();
	}

	void DrawTool::StartResizing(Point mousePos)
	{
		if (hoverBar != nullptr) {
			isResizing = true;
			currentMode = DrawMode::Resize;
			Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
			dragStartPoint = mousePos;
			dragStartTick = hoverBar->Duration;
			targetTrack = timeline->GetTrackAtPoint(mousePos);
		}
	}

	void DrawTool::UpdateResizing(Point mousePos)
	{
		if (!isResizing || hoverBar == nullptr || targetTrack == nullptr) {
			return;
		}

		// Calculate mouse position in ticks
		int mouseTickPosition = timeline->PixelsToTicks(
			mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);

		// Calculate new length (from bar start to mouse position)
		int newLength = mouseTickPosition - hoverBar->StartTick;

		// Snap the length to grid
		int snappedLength = timeline->SnapTickToGrid(newLength);

		// Ensure minimum length (e.g., 1/32 note)
		const int MIN_LENGTH = 120; // 1/32 note
		snappedLength = Math::Max(snappedLength, MIN_LENGTH);

		// Check for overlaps with other bars
		bool hasOverlap = false;
		for each(BarEvent ^ existingBar in targetTrack->Events) {
			if (existingBar != hoverBar &&
				existingBar->StartTick < (hoverBar->StartTick + snappedLength) &&
				(existingBar->StartTick + existingBar->Duration) > hoverBar->StartTick) {
				hasOverlap = true;
				break;
			}
		}

		if (!hasOverlap) {
			hoverBar->Duration = snappedLength;
			timeline->Invalidate();
		}
	}

	void DrawTool::FinishMoving()
	{
		if (isMoving && hoverBar != nullptr) {
			// Ensure final position is properly snapped
			hoverBar->StartTick = timeline->SnapTickToGrid(hoverBar->StartTick);

			// Resort the events in the final track
			if (targetTrack != nullptr) {
				targetTrack->Events->Sort(Track::barComparer);
			}
		}

		isMoving = false;
		hoverBar = nullptr;
		targetTrack = nullptr;
		sourceTrack = nullptr;
	}

	void DrawTool::FinishResizing()
	{
		isResizing = false;
		hoverBar = nullptr;
		targetTrack = nullptr;
	}

	int DrawTool::GetBeatLength(Track^ track, int currentTick)
	{
		// Set default length
		int length = _Draw_Tick_Length;

		// If track doesn't have tablature or measures, return default
		if (!track->ShowTablature || track->Measures == nullptr || track->Measures->Count == 0) {
			return length;
		}

		// Find the current measure
		Measure^ currentMeasure = timeline->GetMeasureAtTick(currentTick);
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
		_Current_Color = value;
		
		if (previewBar != nullptr)
		{
			previewBar->Color = Color::FromArgb(100, (int)value.R, (int)value.G, (int)value.B);
			timeline->Invalidate();
		}
	}

	void DrawTool::DrawTickLength::set(int value)
	{
		_Draw_Tick_Length = value;

		if (previewBar != nullptr)
		{
			previewBar->Duration = value;
			timeline->Invalidate();
		}
	}


	//////////////////////////////
	// SplitTool Implementation //
	//////////////////////////////
	SplitTool::SplitTool(Widget_Timeline^ timeline) : TimelineTool(timeline) {}

	void SplitTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Widget_Timeline::TRACK_HEADER_WIDTH) {
			Track^ track = timeline->GetTrackAtPoint(Point(e->X, e->Y));
			if (track != nullptr) {
				BarEvent^ bar = timeline->GetBarAtPoint(Point(e->X, e->Y));
				if (bar != nullptr) {
					// Calculate split point
					int splitTick = timeline->SnapTickToGrid(
						timeline->PixelsToTicks(e->X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X));

					if (splitTick > bar->StartTick && splitTick < bar->StartTick + bar->Duration) {
						// Create two new bars
						int firstLength = splitTick - bar->StartTick;
						int secondLength = bar->Duration - firstLength;

						timeline->AddBarToTrack(track, bar->StartTick, firstLength, bar->Color);
						timeline->AddBarToTrack(track, splitTick, secondLength, bar->Color);

						// Remove original bar
						track->RemoveBar(bar);
						timeline->Invalidate();
					}
				}
			}
		}
	}

	void SplitTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);
		Track^ track = timeline->GetTrackAtPoint(mousePos);

		if (track != nullptr && e->X > Widget_Timeline::TRACK_HEADER_WIDTH) {
			BarEvent^ bar = timeline->GetBarAtPoint(mousePos);
			if (bar != nullptr) {
				hoverTrack = track;
				hoverBar = bar;

				splitPreviewTick = timeline->SnapTickToGrid(
					timeline->PixelsToTicks(e->X - Widget_Timeline::TRACK_HEADER_WIDTH -
						timeline->ScrollPosition->X));

				if (splitPreviewTick > bar->StartTick &&
					splitPreviewTick < bar->StartTick + bar->Duration) {
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::HSplit);
				}
				else {
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
				}

				timeline->Invalidate();
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
			splitPreviewTick = timeline->SnapTickToGrid(
				timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH -
					timeline->ScrollPosition->X));

			// Only show preview if split position is valid
			if (splitPreviewTick > hoverBar->StartTick &&
				splitPreviewTick < hoverBar->StartTick + hoverBar->Duration) {
				// The actual preview drawing is handled in the widget's paint routine
				timeline->Invalidate();
			}
		}
	}

	void SplitTool::ClearSplitPreview()
	{
		splitPreviewTick	= -1;
		hoverTrack			= nullptr;
		hoverBar			= nullptr;
		timeline->Invalidate();
	}

	//////////////////////////////
	// EraseTool Implementation //
	//////////////////////////////
	EraseTool::EraseTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor				= TimelineCursorHelper::GetCursor(TimelineCursor::No);
		isErasing			= false;
		isSelecting			= false;
		selectionStart		= nullptr;
		selectedBars		= gcnew List<BarEvent^>();
		erasedBars			= gcnew List<BarEvent^>();
		hoverBar			= nullptr;
		hoverTrack			= nullptr;
		selectionRect		= Rectangle(0, 0, 0, 0);
		erasePreviewRect	= Rectangle(0, 0, 0, 0);
	}

	void EraseTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left &&
			e->X > Widget_Timeline::TRACK_HEADER_WIDTH) {

			Point clickPoint(e->X, e->Y);
			BarEvent^ clickedBar = timeline->GetBarAtPoint(clickPoint);

			// If clicking on a selected bar, delete all selected bars
			if (clickedBar != nullptr && selectedBars->Contains(clickedBar)) {
				StartErasing();
				// Erase all selected bars
				for each(BarEvent ^ bar in selectedBars) {
					Track^ track = nullptr;
					// Find the track containing this bar
					for each(Track ^ t in timeline->Tracks) {
						if (t->Events->Contains(bar)) {
							track = t;
							break;
						}
					}
					if (track != nullptr) {
						track->RemoveBar(bar);
						erasedBars->Add(bar);
					}
				}
				ClearSelection();
				EndErasing();
			}
			// If clicking on an unselected bar, clear selection and select only this bar
			else if (clickedBar != nullptr) {
				ClearSelection();
				selectedBars->Add(clickedBar);
				UpdateHoverPreview(clickPoint);
			}
			// If clicking empty space, start selection rectangle
			else {
				ClearSelection();
				StartSelection(clickPoint);
			}

			timeline->Invalidate();
		}
	}

	void EraseTool::OnMouseMove(MouseEventArgs^ e) {
		Point mousePos(e->X, e->Y);

		if (isSelecting) {
			UpdateSelection(mousePos);
		}
		else {
			UpdateHoverPreview(mousePos);
		}

		timeline->Invalidate();
	}

	void EraseTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (isSelecting) {
			EndSelection();
		}
		UpdateHoverPreview(Point(e->X, e->Y));
	}

	void EraseTool::OnKeyDown(KeyEventArgs^ e) {}
	void EraseTool::OnKeyUp(KeyEventArgs^ e) {}

	void EraseTool::StartSelection(Point start)
	{
		isSelecting = true;
		selectionStart = start;
		selectionRect = Rectangle(start.X, start.Y, 0, 0);
	}

	void EraseTool::UpdateSelection(Point current)
	{
		if (!isSelecting || selectionStart == nullptr) return;

		// Calculate selection rectangle
		int x = Math::Min(selectionStart->X, current.X);
		int y = Math::Min(selectionStart->Y, current.Y);
		int width = Math::Abs(current.X - selectionStart->X);
		int height = Math::Abs(current.Y - selectionStart->Y);

		selectionRect = Rectangle(x, y, width, height);

		// Update selected bars
		SelectBarsInRegion(selectionRect);
	}

	void EraseTool::EndSelection()
	{
		isSelecting = false;
		selectionStart = nullptr;
		selectionRect = Rectangle(0, 0, 0, 0);
		timeline->Invalidate();
	}

	void EraseTool::SelectBarsInRegion(Rectangle region)
	{
		selectedBars->Clear();

		// Convert selection rectangle to tick range
		int startTick = timeline->PixelsToTicks(
			region.Left - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int endTick = timeline->PixelsToTicks(
			region.Right - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each(Track ^ track in timeline->Tracks) {
			Rectangle trackBounds = timeline->GetTrackContentBounds(track);
			if (trackBounds.IntersectsWith(region)) {
				for each(BarEvent ^ bar in track->Events) {
					// Check if bar intersects with selection
					if (bar->StartTick + bar->Duration >= startTick &&
						bar->StartTick <= endTick) {
						selectedBars->Add(bar);
					}
				}
			}
		}
	}

	void EraseTool::ClearSelection()
	{
		selectedBars->Clear();
		selectionRect = Rectangle(0, 0, 0, 0);
		timeline->Invalidate();
	}

	void EraseTool::StartErasing()
	{
		isErasing = true;
		if (erasedBars == nullptr) {
			erasedBars = gcnew List<BarEvent^>();
		}
		else {
			erasedBars->Clear();
		}
	}

	void EraseTool::UpdateErasing(Point mousePos)
	{
		Track^ track = timeline->GetTrackAtPoint(mousePos);
		if (track != nullptr)
		{
			BarEvent^ bar = timeline->GetBarAtPoint(mousePos);
			if (bar != nullptr && !erasedBars->Contains(bar))
			{
				track->RemoveBar(bar);
				erasedBars->Add(bar);
				if (timeline->SelectedBar == bar) {
					timeline->SelectedBar = nullptr;
				}
				timeline->Invalidate();
			}
		}
	}

	void EraseTool::EndErasing()
	{
		isErasing = false;
		erasedBars->Clear();
	}

	void EraseTool::UpdateHoverPreview(Point mousePos)
	{
		Track^ track = timeline->GetTrackAtPoint(mousePos);
		BarEvent^ bar = nullptr;

		if (track != nullptr && mousePos.X > Widget_Timeline::TRACK_HEADER_WIDTH) {
			bar = timeline->GetBarAtPoint(mousePos);
		}

		// Update hover state
		if (bar != hoverBar || track != hoverTrack) {
			hoverBar = bar;
			hoverTrack = track;

			if (bar != nullptr) {
				// Calculate preview rectangle
				int x = timeline->TicksToPixels(bar->StartTick) +
					timeline->ScrollPosition->X +
					Widget_Timeline::TRACK_HEADER_WIDTH;

				Rectangle trackBounds = timeline->GetTrackContentBounds(track);

				erasePreviewRect = Rectangle(
					x,
					trackBounds.Y + Widget_Timeline::TRACK_PADDING,
					timeline->TicksToPixels(bar->Duration),
					trackBounds.Height - (Widget_Timeline::TRACK_PADDING * 2)
				);

				// Update cursor based on whether the bar is selected
				Cursor = selectedBars->Contains(bar) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::No);
			}
			else {
				erasePreviewRect = Rectangle(0, 0, 0, 0);
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross); // Use cross cursor for selection
			}

			timeline->Invalidate();
		}
	}

	void EraseTool::ClearHoverPreview()
	{
		if (hoverBar != nullptr || !erasePreviewRect.IsEmpty) {
			hoverBar = nullptr;
			hoverTrack = nullptr;
			erasePreviewRect = Rectangle(0, 0, 0, 0);
			timeline->Invalidate();
		}
	}

	void EraseTool::Activate() 
	{
		isActive = true;
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::No);
	}

	/////////////////////////////////
	// DurationTool Implementation //
	/////////////////////////////////
	DurationTool::DurationTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
		isDragging			= false;
		isShowingPreview	= false;
		isSelecting			= false;
		targetBar			= nullptr;
		targetTrack			= nullptr;
		originalLength		= 0;
		dragStartX			= 0;
		previewLength		= 0;
		changeTickLength	= DEFAULT_TICK_LENGTH;
		selectedBars		= gcnew List<BarEvent^>();
		originalLengths		= gcnew Dictionary<BarEvent^, int>();
		selectionStart		= nullptr;
		selectionRect		= Rectangle(0, 0, 0, 0);
	}

	void DurationTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left &&
			e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Point mousePos(e->X, e->Y);
			Track^ track = timeline->GetTrackAtPoint(mousePos);
			BarEvent^ bar = timeline->GetBarAtPoint(mousePos);

			if (track != nullptr && bar != nullptr)
			{
				// If clicking on a handle of a selected bar
				if (IsOverHandle(mousePos, bar, track) && selectedBars->Contains(bar))
				{
					isDragging = true;
					targetBar = bar;
					targetTrack = track;
					StoreOriginalLengths();
					dragStartX = e->X;
					Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::SizeWE);
				}
				// If clicking on a handle of an unselected bar
				else if (IsOverHandle(mousePos, bar, track))
				{
					ClearSelection();
					selectedBars->Add(bar);
					isDragging = true;
					targetBar = bar;
					targetTrack = track;
					StoreOriginalLengths();
					dragStartX = e->X;
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
			timeline->Invalidate();
		}
	}

	void DurationTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);

		if (isDragging && targetBar != nullptr && selectedBars->Count > 0)
		{
			// Calculate length change
			int deltaPixels = e->X - dragStartX;
			int deltaTicks = timeline->PixelsToTicks(deltaPixels);

			// Calculate the scale factor for the length change
			double scaleFactor = 1.0;
			if (originalLengths[targetBar] > 0)
			{
				scaleFactor = (double)(originalLengths[targetBar] + deltaTicks) / originalLengths[targetBar];
			}

			// Apply length change to all selected bars
			for each (BarEvent ^ bar in selectedBars)
			{
				int originalLength = originalLengths[bar];
				int newLength;

				if (Control::ModifierKeys == Keys::Control)
				{
					// Snap to grid
					newLength = timeline->SnapTickToGrid(originalLength + deltaTicks);
				}
				else
				{
					// Proportional change
					newLength = (int)(originalLength * scaleFactor);
					// Round to nearest changeTickLength
					newLength = ((newLength + changeTickLength / 2) / changeTickLength) * changeTickLength;
				}

				// Ensure minimum length
				bar->Duration = Math::Max(newLength, (int)MIN_LENGTH_TICKS);
			}

			timeline->Invalidate();
		}
		else if (isSelecting)
		{
			UpdateSelection(mousePos);
		}
		else
		{
			Track^ track = timeline->GetTrackAtPoint(mousePos);
			BarEvent^ bar = timeline->GetBarAtPoint(mousePos);

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
		if (isDragging)
		{
			isDragging = false;
			targetBar = nullptr;
			targetTrack = nullptr;
			originalLengths->Clear();
		}
		else if (isSelecting)
		{
			EndSelection();
		}
	}

	void DurationTool::OnKeyDown(KeyEventArgs^ e) {}
	void DurationTool::OnKeyUp(KeyEventArgs^ e) {}

	bool DurationTool::IsOverHandle(Point mousePos, BarEvent^ bar, Track^ track)
	{
		if (bar == nullptr || track == nullptr) return false;

		Rectangle bounds = timeline->GetTrackContentBounds(track);
		int barEndX = timeline->TicksToPixels(bar->StartTick + bar->Duration) +
			timeline->ScrollPosition->X +
			Widget_Timeline::TRACK_HEADER_WIDTH;

		// Check if mouse is within handle area at end of bar
		return mousePos.X >= barEndX - HANDLE_WIDTH &&
			mousePos.X <= barEndX + HANDLE_WIDTH &&
			mousePos.Y >= bounds.Y + Widget_Timeline::TRACK_PADDING &&
			mousePos.Y <= bounds.Bottom - Widget_Timeline::TRACK_PADDING;
	}

	void DurationTool::UpdateLengthPreview(Point mousePos)
	{
		if (targetBar == nullptr || !isDragging) {
			Track^ track = timeline->GetTrackAtPoint(mousePos);
			BarEvent^ bar = timeline->GetBarAtPoint(mousePos);

			if (track != nullptr && bar != nullptr && IsOverHandle(mousePos, bar, track)) {
				if (Control::ModifierKeys == Keys::Control)
				{
					int barEndX = timeline->TicksToPixels(bar->StartTick + bar->Duration) +
						timeline->ScrollPosition->X +
						Widget_Timeline::TRACK_HEADER_WIDTH;

					int deltaPixels = mousePos.X - barEndX;
					int deltaTicks = timeline->PixelsToTicks(deltaPixels);
					previewLength = timeline->SnapTickToGrid(bar->Duration + deltaTicks);
				}
				else
				{
					// Calculate total desired length from mouse position
					int barEndX = timeline->TicksToPixels(bar->StartTick + bar->Duration) +
						timeline->ScrollPosition->X +
						Widget_Timeline::TRACK_HEADER_WIDTH;
					int deltaPixels = mousePos.X - barEndX;
					int deltaTicks = timeline->PixelsToTicks(deltaPixels);

					int totalDesiredLength = bar->Duration + deltaTicks;
					int numUnits = Math::Max(1, (totalDesiredLength + (changeTickLength / 2)) / changeTickLength);
					previewLength = numUnits * changeTickLength;
				}

				// Ensure minimum length
				previewLength = Math::Max(previewLength, (int)MIN_LENGTH_TICKS);

				isShowingPreview = true;
				targetBar = bar;
				targetTrack = track;

				timeline->Invalidate();
			}
			else {
				ClearPreview();
			}
		}
	}

	void DurationTool::ClearPreview()
	{
		if (isShowingPreview) {
			isShowingPreview = false;

			if (!isDragging) {
				targetBar = nullptr;
				targetTrack = nullptr;
			}

			timeline->Invalidate();
		}
	}

	void DurationTool::StartSelection(Point start)
	{
		isSelecting = true;
		selectionStart = start;
		selectionRect = Rectangle(start.X, start.Y, 0, 0);
	}

	void DurationTool::UpdateSelection(Point current)
	{
		if (!isSelecting || selectionStart == nullptr) return;

		// Calculate selection rectangle
		int x = Math::Min(selectionStart->X, current.X);
		int y = Math::Min(selectionStart->Y, current.Y);
		int width = Math::Abs(current.X - selectionStart->X);
		int height = Math::Abs(current.Y - selectionStart->Y);

		selectionRect = Rectangle(x, y, width, height);

		// Update selected bars
		SelectBarsInRegion(selectionRect);
		timeline->Invalidate();
	}

	void DurationTool::EndSelection()
	{
		isSelecting = false;
		selectionStart = nullptr;
		selectionRect = Rectangle(0, 0, 0, 0);
		timeline->Invalidate();
	}

	void DurationTool::SelectBarsInRegion(Rectangle region)
	{
		selectedBars->Clear();

		// Convert selection rectangle to tick range
		int startTick = timeline->PixelsToTicks(
			region.Left - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int endTick = timeline->PixelsToTicks(
			region.Right - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track ^ track in timeline->Tracks)
		{
			Rectangle trackBounds = timeline->GetTrackContentBounds(track);
			if (trackBounds.IntersectsWith(region))
			{
				for each (BarEvent ^ bar in track->Events)
				{
					if (bar->StartTick + bar->Duration >= startTick && bar->StartTick <= endTick)
					{
						selectedBars->Add(bar);
					}
				}
			}
		}
	}

	void DurationTool::ClearSelection()
	{
		selectedBars->Clear();
		originalLengths->Clear();
		selectionRect = Rectangle(0, 0, 0, 0);
		timeline->Invalidate();
	}

	void DurationTool::StoreOriginalLengths()
	{
		originalLengths->Clear();
		for each (BarEvent ^ bar in selectedBars)
		{
			originalLengths->Add(bar, bar->Duration);
		}
	}


	//////////////////////////////
	// ColorTool Implementation //
	//////////////////////////////
	ColorTool::ColorTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor			= TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		isSelecting		= false;
		selectionStart	= nullptr;
		selectedBars	= gcnew List<BarEvent^>();
		hoverBar		= nullptr;
		hoverTrack		= nullptr;
		selectionRect	= Rectangle(0, 0, 0, 0);
		previewRect		= Rectangle(0, 0, 0, 0);
		currentColor	= Color::FromArgb(255, 100, 100, 255); // Default color
	}

	void ColorTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left &&
			e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Point clickPoint(e->X, e->Y);
			BarEvent^ clickedBar = timeline->GetBarAtPoint(clickPoint);

			// If clicking on a selected bar, apply color to all selected bars
			if (clickedBar != nullptr && selectedBars->Contains(clickedBar)) {
				ApplyColorToSelection();
			}
			// If clicking on an unselected bar, clear selection and select only this bar
			else if (clickedBar != nullptr) {
				ClearSelection();
				selectedBars->Add(clickedBar);
				UpdateHoverPreview(clickPoint);
			}
			// If clicking empty space, start selection rectangle
			else {
				ClearSelection();
				StartSelection(clickPoint);
			}

			timeline->Invalidate();
		}
	}

	void ColorTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);

		if (isSelecting) {
			UpdateSelection(mousePos);
		}
		else {
			UpdateHoverPreview(mousePos);
		}

		timeline->Invalidate();
	}

	void ColorTool::OnMouseUp(MouseEventArgs^ e)
	{
		if (isSelecting) {
			EndSelection();
		}
		UpdateHoverPreview(Point(e->X, e->Y));
	}

	void ColorTool::OnKeyDown(KeyEventArgs^ e) {}
	void ColorTool::OnKeyUp(KeyEventArgs^ e) {}

	void ColorTool::StartSelection(Point start)
	{
		isSelecting = true;
		selectionStart = start;
		selectionRect = Rectangle(start.X, start.Y, 0, 0);
	}

	void ColorTool::UpdateSelection(Point current)
	{
		if (!isSelecting || selectionStart == nullptr) return;

		// Calculate selection rectangle
		int x = Math::Min(selectionStart->X, current.X);
		int y = Math::Min(selectionStart->Y, current.Y);
		int width = Math::Abs(current.X - selectionStart->X);
		int height = Math::Abs(current.Y - selectionStart->Y);

		selectionRect = Rectangle(x, y, width, height);

		// Update selected bars
		SelectBarsInRegion(selectionRect);
	}

	void ColorTool::EndSelection()
	{
		isSelecting = false;
		selectionStart = nullptr;
		selectionRect = Rectangle(0, 0, 0, 0);
		timeline->Invalidate();
	}

	void ColorTool::SelectBarsInRegion(Rectangle region)
	{
		selectedBars->Clear();

		// Convert selection rectangle to tick range
		int startTick = timeline->PixelsToTicks(
			region.Left - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int endTick = timeline->PixelsToTicks(
			region.Right - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);

		// Find all bars that intersect with the selection rectangle
		for each (Track ^ track in timeline->Tracks) {
			Rectangle trackBounds = timeline->GetTrackContentBounds(track);
			if (trackBounds.IntersectsWith(region)) {
				for each (BarEvent ^ bar in track->Events) {
					// Check if bar intersects with selection
					if (bar->StartTick + bar->Duration >= startTick &&
						bar->StartTick <= endTick) {
						selectedBars->Add(bar);
					}
				}
			}
		}
	}

	void ColorTool::ClearSelection()
	{
		selectedBars->Clear();
		selectionRect = Rectangle(0, 0, 0, 0);
		timeline->Invalidate();
	}

	void ColorTool::UpdateHoverPreview(Point mousePos)
	{
		Track^ track = timeline->GetTrackAtPoint(mousePos);
		BarEvent^ bar = nullptr;

		if (track != nullptr && mousePos.X > Widget_Timeline::TRACK_HEADER_WIDTH) {
			bar = timeline->GetBarAtPoint(mousePos);
		}

		// Update hover state
		if (bar != hoverBar || track != hoverTrack) {
			hoverBar = bar;
			hoverTrack = track;

			if (bar != nullptr) {
				// Calculate preview rectangle
				int x = timeline->TicksToPixels(bar->StartTick) +
					timeline->ScrollPosition->X +
					Widget_Timeline::TRACK_HEADER_WIDTH;

				Rectangle trackBounds = timeline->GetTrackContentBounds(track);

				previewRect = Rectangle(
					x,
					trackBounds.Y + Widget_Timeline::TRACK_PADDING,
					timeline->TicksToPixels(bar->Duration),
					trackBounds.Height - (Widget_Timeline::TRACK_PADDING * 2)
				);

				// Update cursor based on whether the bar is selected
				Cursor = selectedBars->Contains(bar) ? TimelineCursorHelper::GetCursor(TimelineCursor::Hand) : TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
			}
			else {
				previewRect = Rectangle(0, 0, 0, 0);
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross); // Use cross cursor for selection
			}

			timeline->Invalidate();
		}
	}

	void ColorTool::ClearHoverPreview()
	{
		if (hoverBar != nullptr || !previewRect.IsEmpty) {
			hoverBar = nullptr;
			hoverTrack = nullptr;
			previewRect = Rectangle(0, 0, 0, 0);
			timeline->Invalidate();
		}
	}

	void ColorTool::ApplyColorToSelection()
	{
		for each (BarEvent ^ bar in selectedBars) {
			bar->Color = currentColor;
		}
		timeline->Invalidate();
	}

	void ColorTool::CurrentColor::set(Color value)
	{
		currentColor = value;
		if (selectedBars->Count > 0) {
			ApplyColorToSelection();
		}
		timeline->Invalidate();
	}


	/////////////////////////////
	// FadeTool Implementation //
	/////////////////////////////
	FadeTool::FadeTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor		= TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		isDrawing	= false;
		drawStart	= nullptr;
		targetTrack	= nullptr;
		previewBars	= nullptr;
		previewBar	= nullptr;
		TickLength	= DEFAULT_TICK_LENGTH;
		ColorStart	= Color::Red;
		ColorEnd	= Color::Green;
	}

	void FadeTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Point mousePos(e->X, e->Y);
			Track^ track = timeline->GetTrackAtPoint(mousePos);

			if (track != nullptr) {
				isDrawing = true;
				drawStart = mousePos;
				targetTrack = track;

				// Calculate start tick with grid snap
				startTick = timeline->SnapTickToGrid(
					timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X));

				previewBar = nullptr;
				timeline->Invalidate();
			}
		}
	}

	void FadeTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);
		lastMousePos = mousePos;

		if (isDrawing && targetTrack != nullptr)
		{
			// Calculate pixels moved
			int deltaX = Math::Abs(mousePos.X - drawStart->X);

			// Only start creating bars after minimum drag distance
			if (deltaX >= MIN_DRAG_PIXELS)
			{
				CreatePreviewBars(mousePos);
				previewBar = nullptr;
				timeline->Invalidate();
			}
		}
		else if (e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Track^ track = timeline->GetTrackAtPoint(mousePos);
			if (track != nullptr)
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
				UpdateSinglePreview(mousePos);
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
		if (isDrawing)
		{
			Point mousePos(e->X, e->Y);
			Track^ track = timeline->GetTrackAtPoint(mousePos);

			// Only add bars if released over the same track
			if (track == targetTrack && previewBars != nullptr && previewBars->Count > 0) {
				AddBarsToTrack();
			}

			// Reset state
			isDrawing	= false;
			drawStart	= nullptr;
			targetTrack	= nullptr;
			ClearPreviews();

			timeline->Invalidate();
		}
	}

	void FadeTool::OnKeyDown(KeyEventArgs^ e) {}
	void FadeTool::OnKeyUp(KeyEventArgs^ e) {}

	Color FadeTool::InterpolateColor(Color start, Color end, float ratio)
	{
		int r = start.R + (int)((end.R - start.R) * ratio);
		int g = start.G + (int)((end.G - start.G) * ratio);
		int b = start.B + (int)((end.B - start.B) * ratio);
		return Color::FromArgb(255, r, g, b);
	}

	void FadeTool::CreatePreviewBars(Point currentPos)
	{
		if (previewBars == nullptr) {
			previewBars = gcnew List<BarEvent^>();
		}
		else {
			previewBars->Clear();
		}

		// Calculate total distance in ticks
		int currentTick = timeline->PixelsToTicks(
			currentPos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int totalTicks = currentTick - startTick;

		if (totalTicks <= 0) return;

		// Calculate number of bars needed
		int numBars = (int)Math::Ceiling((double)totalTicks / TickLength);
		if (numBars == 0) return;

		// Create bars with interpolated colors
		for (int i = 0; i < numBars; i++)
		{
			float ratio = (float)i / (numBars - 1);
			if (numBars == 1) ratio = 0; // Single bar case

			Color barColor = InterpolateColor(ColorStart, ColorEnd, ratio);

			int barStartTick = startTick + (i * TickLength);
			int barLength = TickLength;

			BarEvent^ bar = gcnew BarEvent(barStartTick, barLength, Color::FromArgb(180, barColor));	// Semi-transparent for preview
			previewBars->Add(bar);
		}
	}

	void FadeTool::AddBarsToTrack()
	{
		if (previewBars == nullptr || targetTrack == nullptr) return;

		// Create final bars with full opacity
		for each (BarEvent ^ previewBar in previewBars)
		{
			BarEvent^ finalBar = gcnew BarEvent(
				previewBar->StartTick,
				previewBar->Duration,
				Color::FromArgb(255,
					(int)previewBar->Color.R,
					(int)previewBar->Color.G,
					(int)previewBar->Color.B)
			);
			targetTrack->Events->Add(finalBar);
		}

		// Sort events in track
		targetTrack->Events->Sort(Track::barComparer);
	}

	void FadeTool::UpdateSinglePreview(Point mousePos)
	{
		Track^ track = timeline->GetTrackAtPoint(mousePos);

		if (track != nullptr && !isDrawing)
		{
			int snapTick = timeline->SnapTickToGrid(
				timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X));

			// Create or update single preview bar
			if (previewBar == nullptr) {
				previewBar = gcnew BarEvent(snapTick, TickLength, Color::FromArgb(100, (int)ColorStart.R, (int)ColorStart.G, (int)ColorStart.B));
			}
			else {
				previewBar->StartTick	= snapTick;
				previewBar->Duration	= TickLength;
			}

			timeline->Invalidate();
		}
		else
		{
			ClearPreviews();
		}
	}

	void FadeTool::ClearPreviews()
	{
		if (previewBar != nullptr || (previewBars != nullptr && previewBars->Count > 0))
		{
			previewBar = nullptr;
			if (previewBars != nullptr)
			{
				previewBars->Clear();
			}
			timeline->Invalidate();
		}
	}


	///////////////////////////////
	// StrobeTool Implementation //
	///////////////////////////////
	StrobeTool::StrobeTool(Widget_Timeline^ timeline) : TimelineTool(timeline)
	{
		Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
		isDrawing = false;
		drawStart = nullptr;
		targetTrack = nullptr;
		previewBars = nullptr;
		previewBar	= nullptr;
		TickLength	= DEFAULT_TICK_LENGTH;
		StrobeColor = Color::Red;
	}

	void StrobeTool::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left && e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Point mousePos(e->X, e->Y);
			Track^ track = timeline->GetTrackAtPoint(mousePos);

			if (track != nullptr) {
				isDrawing = true;
				drawStart = mousePos;
				targetTrack = track;

				// Calculate start tick with grid snap
				startTick = timeline->SnapTickToGrid(
					timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X));

				previewBar = nullptr;
				timeline->Invalidate();
			}
		}
	}

	void StrobeTool::OnMouseMove(MouseEventArgs^ e)
	{
		Point mousePos(e->X, e->Y);
		lastMousePos = mousePos;

		if (isDrawing && targetTrack != nullptr)
		{
			// Calculate pixels moved
			int deltaX = Math::Abs(mousePos.X - drawStart->X);

			// Only start creating bars after minimum drag distance
			if (deltaX >= MIN_DRAG_PIXELS)
			{
				CreatePreviewBars(mousePos);
				previewBar = nullptr;
				timeline->Invalidate();
			}
		}
		else if (e->X > Widget_Timeline::TRACK_HEADER_WIDTH)
		{
			Track^ track = timeline->GetTrackAtPoint(mousePos);
			if (track != nullptr)
			{
				Cursor = TimelineCursorHelper::GetCursor(TimelineCursor::Cross);
				UpdateSinglePreview(mousePos);
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
		if (isDrawing)
		{
			Point mousePos(e->X, e->Y);
			Track^ track = timeline->GetTrackAtPoint(mousePos);

			// Only add bars if released over the same track
			if (track == targetTrack && previewBars != nullptr && previewBars->Count > 0) {
				AddBarsToTrack();
			}

			// Reset state
			isDrawing	= false;
			drawStart	= nullptr;
			targetTrack	= nullptr;
			ClearPreviews();
			timeline->Invalidate();
		}
	}

	void StrobeTool::OnKeyDown(KeyEventArgs^ e) {}
	void StrobeTool::OnKeyUp(KeyEventArgs^ e) {}

	void StrobeTool::CreatePreviewBars(Point currentPos)
	{
		if (previewBars == nullptr) {
			previewBars = gcnew List<BarEvent^>();
		}
		else {
			previewBars->Clear();
		}

		// Calculate total distance in ticks
		int currentTick = timeline->PixelsToTicks(currentPos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X);
		int totalTicks = currentTick - startTick;

		if (totalTicks <= 0) return;

		// Calculate number of bars needed
		int numBars = (int)Math::Ceiling((double)totalTicks / TickLength);
		if (numBars == 0) return;

		// Create bars with interpolated colors
		for (int i = 0; i < numBars; i++)
		{
			if (i % 2 == 1) continue;

			int barStartTick = startTick + (i * TickLength);
			int barLength = TickLength;

			BarEvent^ bar = gcnew BarEvent(barStartTick, barLength, Color::FromArgb(180, StrobeColor));	// Semi-transparent for preview
			previewBars->Add(bar);
		}
	}

	void StrobeTool::AddBarsToTrack()
	{
		if (previewBars == nullptr || targetTrack == nullptr) return;

		// Create final bars with full opacity
		for each (BarEvent ^ previewBar in previewBars)
		{
			BarEvent^ finalBar = gcnew BarEvent(
				previewBar->StartTick,
				previewBar->Duration,
				Color::FromArgb(255,
					(int)previewBar->Color.R,
					(int)previewBar->Color.G,
					(int)previewBar->Color.B)
			);
			targetTrack->Events->Add(finalBar);
		}

		// Sort events in track
		targetTrack->Events->Sort(Track::barComparer);
	}

	void StrobeTool::UpdateSinglePreview(Point mousePos)
	{
		Track^ track = timeline->GetTrackAtPoint(mousePos);

		if (track != nullptr && !isDrawing)
		{
			int snapTick = timeline->SnapTickToGrid(
				timeline->PixelsToTicks(mousePos.X - Widget_Timeline::TRACK_HEADER_WIDTH - timeline->ScrollPosition->X));

			// Create or update single preview bar
			if (previewBar == nullptr) {
				previewBar = gcnew BarEvent(snapTick, TickLength, Color::FromArgb(100, (int)StrobeColor.R, (int)StrobeColor.G, (int)StrobeColor.B));
			}
			else {
				previewBar->StartTick = snapTick;
				previewBar->Duration = TickLength;
			}

			timeline->Invalidate();
		}
		else
		{
			ClearPreviews();
		}
	}

	void StrobeTool::ClearPreviews()
	{
		if (previewBar != nullptr || (previewBars != nullptr && previewBars->Count > 0))
		{
			previewBar = nullptr;
			if (previewBars != nullptr)
			{
				previewBars->Clear();
			}
			timeline->Invalidate();
		}
	}
}
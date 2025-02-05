#include "Timeline_Command_Manager.h"

#include "Widget_Timeline.h"

namespace MIDILightDrawer
{
	////////////////////////////
	// TimelineCommandManager //
	////////////////////////////
	TimelineCommandManager::TimelineCommandManager(Widget_Timeline^ timeline)
	{
		_Timeline = timeline;
		_UndoStack = gcnew Stack<ITimelineCommand^>();
		_RedoStack = gcnew Stack<ITimelineCommand^>();
	}

	void TimelineCommandManager::ExecuteCommand(ITimelineCommand^ command)
	{
		command->Execute();
		_UndoStack->Push(command);
		_RedoStack->Clear();  // Clear redo stack when new command is executed

		// Limit undo stack size
		while (_UndoStack->Count > MAX_UNDO_LEVELS) {
			_UndoStack->Pop();
		}

		_Timeline->ToolAccess()->OnCommandStateChanged();
		CommandStateChanged();
	}

	void TimelineCommandManager::Undo()
	{
		if (_UndoStack->Count > 0) {
			ITimelineCommand^ command = _UndoStack->Pop();
			command->Undo();
			_RedoStack->Push(command);

			// Notify current tool
			_Timeline->ToolAccess()->OnCommandStateChanged();
		}

		CommandStateChanged();
	}

	void TimelineCommandManager::Redo()
	{
		if (_RedoStack->Count > 0) {
			ITimelineCommand^ command = _RedoStack->Pop();

			command->Execute();

			_UndoStack->Push(command);

			// Notify current tool
			_Timeline->ToolAccess()->OnCommandStateChanged();
		}

		CommandStateChanged();
	}


	/////////////////////
	// CompoundCommand //
	/////////////////////
	CompoundCommand::CompoundCommand(String^ description)
	{
		_Commands = gcnew List<ITimelineCommand^>();
		_Description = description;
	}

	void CompoundCommand::AddCommand(ITimelineCommand^ command)
	{
		_Commands->Add(command);
	}

	void CompoundCommand::Execute()
	{
		for each (ITimelineCommand ^ command in _Commands){
			command->Execute();
		}
	}

	void CompoundCommand::Undo()
	{
		// Undo commands in reverse order
		for (int i = _Commands->Count - 1; i >= 0; i--) {
			_Commands[i]->Undo();
		}
	}

	String^ CompoundCommand::GetDescription()
	{
		return _Description;
	}


	///////////////////
	// AddBarCommand //
	///////////////////
	AddBarCommand::AddBarCommand(Widget_Timeline^ timeline, Track^ track, BarEvent^ bar)
	{
		_Timeline	= timeline;
		_Track		= track;
		_Bar		= bar;
	}

	void AddBarCommand::Execute()
	{
		_Track->AddBar(_Bar);
		_Timeline->Invalidate();
	}

	void AddBarCommand::Undo()
	{
		_Track->RemoveBar(_Bar);
		_Timeline->Invalidate();
	}

	String^ AddBarCommand::GetDescription()
	{
		return "Add Bar";
	}


	////////////////////
	// MoveBarCommand //
	////////////////////
	MoveBarCommand::MoveBarCommand(Widget_Timeline^ timeline, BarEvent^ bar, Track^ sourceTrack, Track^ targetTrack, int oldStartTick, int newStartTick)
	{
		_Timeline = timeline;
		_Bar = bar;
		_SourceTrack = sourceTrack;
		_TargetTrack = targetTrack;
		_OldStartTick = oldStartTick;
		_NewStartTick = newStartTick;
	}

	void MoveBarCommand::Execute() {
		if (_SourceTrack != _TargetTrack) {
			_SourceTrack->RemoveBar(_Bar);
			_TargetTrack->AddBar(_Bar);
		}
		_Bar->StartTick = _NewStartTick;
		_Timeline->Invalidate();
	}

	void MoveBarCommand::Undo() {
		if (_SourceTrack != _TargetTrack) {
			_TargetTrack->RemoveBar(_Bar);
			_SourceTrack->AddBar(_Bar);
		}
		_Bar->StartTick = _OldStartTick;
		_Timeline->Invalidate();
	}

	String^ MoveBarCommand::GetDescription() {
		return "Move Bar";
	}


	//////////////////////
	// ResizeBarCommand //
	//////////////////////
	ResizeBarCommand::ResizeBarCommand(Widget_Timeline^ timeline, BarEvent^ bar, int oldDuration, int newDuration)
	{
		_Timeline = timeline;
		_Bar = bar;
		_OldDuration = oldDuration;
		_NewDuration = newDuration;
	}

	void ResizeBarCommand::Execute()
	{
		_Bar->Duration			= _NewDuration;
		_Bar->OriginalDuration	= _NewDuration;
		_Timeline->Invalidate();
	}

	void ResizeBarCommand::Undo()
	{
		_Bar->Duration			= _OldDuration;
		_Bar->OriginalDuration	= _OldDuration;
		_Timeline->Invalidate();
	}

	String^ ResizeBarCommand::GetDescription()
	{
		return "Resize Bar";
	}


	///////////////////////////
	// ChangeBarColorCommand //
	///////////////////////////
	ChangeBarColorCommand::ChangeBarColorCommand(Widget_Timeline^ timeline, BarEvent^ bar, Color oldColor, Color newColor)
	{
		_Timeline = timeline;
		_Bar = bar;
		_OldColor = oldColor;
		_NewColor = newColor;
	}

	void ChangeBarColorCommand::Execute()
	{
		_Bar->Color = _NewColor;
		_Timeline->Invalidate();
	}

	void ChangeBarColorCommand::Undo()
	{
		_Bar->Color = _OldColor;
		_Timeline->Invalidate();
	}

	String^ ChangeBarColorCommand::GetDescription()
	{
		return "Change Bar Color";
	}


	////////////////////////////
	// Change Fade Bar Colors //
	////////////////////////////
	ChangeFadeBarColorCommand::ChangeFadeBarColorCommand(Widget_Timeline^ timeline, BarEvent^ bar, ColorType colorType, Color oldColor, Color newColor)
	{
		_Timeline = timeline;
		_Bar = bar;
		_ColorType = colorType;
		_OldColor = oldColor;
		_NewColor = newColor;
	}

	void ChangeFadeBarColorCommand::Execute()
	{
		switch (_ColorType)
		{
		case ColorType::Start:	_Bar->FadeInfo->ColorStart	= _NewColor; break;
		case ColorType::Center:	_Bar->FadeInfo->ColorCenter = _NewColor; break;
		case ColorType::End:	_Bar->FadeInfo->ColorEnd	= _NewColor; break;
		}

		_Timeline->Invalidate();
	}

	void ChangeFadeBarColorCommand::Undo()
	{
		switch (_ColorType)
		{
		case ColorType::Start:	_Bar->FadeInfo->ColorStart	= _OldColor; break;
		case ColorType::Center:	_Bar->FadeInfo->ColorCenter = _OldColor; break;
		case ColorType::End:	_Bar->FadeInfo->ColorEnd	= _OldColor; break;
		}

		_Timeline->Invalidate();
	}

	String^ ChangeFadeBarColorCommand::GetDescription()
	{
		return "Change Fade Color";
	}


	//////////////////////
	// DeleteBarCommand //
	//////////////////////
	DeleteBarCommand::DeleteBarCommand(Widget_Timeline^ timeline, Track^ track, BarEvent^ bar)
	{
		_Timeline = timeline;
		_Track = track;
		_Bar = bar;
	}

	void DeleteBarCommand::Execute()
	{
		_Track->RemoveBar(_Bar);
		_Timeline->Invalidate();
	}

	void DeleteBarCommand::Undo()
	{
		_Track->AddBar(_Bar);
		_Timeline->Invalidate();
	}

	String^ DeleteBarCommand::GetDescription()
	{
		return "Delete Bar";
	}


	/////////////////////
	// PasteBarCommand //
	/////////////////////
	PasteBarCommand::PasteBarCommand(Widget_Timeline^ timeline, List<BarEvent^>^ bars, List<Track^>^ targetTracks)
	{
		_Timeline = timeline;
		_Bars = bars;
		_Tracks = targetTracks;
		_CreatedBars = gcnew List<BarEvent^>();
	}

	void PasteBarCommand::Execute()
	{
		_CreatedBars->Clear();
		for (int i = 0; i < _Bars->Count; i++)
		{
			BarEvent^ OriginalBar = _Bars[i];
			Track^ TargetTrack = _Tracks[i];

			// Create a new bar with the same properties
			BarEvent^ newBar = CreateBarCopy(OriginalBar, OriginalBar->StartTick, false);
			TargetTrack->AddBar(newBar);
			_CreatedBars->Add(newBar);
		}
		_Timeline->Invalidate();
	}

	void PasteBarCommand::Undo()
	{
		for each (BarEvent^ Bar in _CreatedBars) {
			Bar->ContainingTrack->RemoveBar(Bar);
		}
		_Timeline->Invalidate();
	}

	String^ PasteBarCommand::GetDescription() {
		return "Paste Bars";
	}

	BarEvent^ PasteBarCommand::CreateBarCopy(BarEvent^ sourceBar, int startTick, bool isPreview)
	{
		BarEvent^ CopiedBar = nullptr;

		// Create appropriate bar based on type
		switch (sourceBar->Type)
		{
		case BarEventType::Solid:
			CopiedBar = gcnew BarEvent(sourceBar->ContainingTrack, startTick, sourceBar->Duration, isPreview ? Color::FromArgb(180, sourceBar->Color) : sourceBar->Color);
			break;

		case BarEventType::Fade:
		{
			// Deep copy the Fade Info
			BarEventFadeInfo^ OriginalFade = sourceBar->FadeInfo;
			BarEventFadeInfo^ CopiedFade;

			if (OriginalFade->Type == FadeType::Two_Colors) {
				CopiedFade = gcnew BarEventFadeInfo(OriginalFade->QuantizationTicks, OriginalFade->ColorStart, OriginalFade->ColorEnd);
			}
			else {
				CopiedFade = gcnew BarEventFadeInfo(OriginalFade->QuantizationTicks, OriginalFade->ColorStart, OriginalFade->ColorCenter, OriginalFade->ColorEnd);
			}

			CopiedBar = gcnew BarEvent(sourceBar->ContainingTrack, startTick, sourceBar->Duration, CopiedFade);
		}
		break;

		case BarEventType::Strobe:
		{
			// Deep copy the Strobe Info
			BarEventStrobeInfo^ OriginalStrobe = sourceBar->StrobeInfo;
			BarEventStrobeInfo^ CopiedStrobe = gcnew BarEventStrobeInfo(OriginalStrobe->QuantizationTicks, OriginalStrobe->ColorStrobe);

			CopiedBar = gcnew BarEvent(sourceBar->ContainingTrack, startTick, sourceBar->Duration, CopiedStrobe);
		}
		break;
		}

		return CopiedBar;
	}


	///////////////////////
	// AddFadeBarCommand //
	///////////////////////
	AddFadeBarCommand::AddFadeBarCommand(Widget_Timeline^ timeline, Track^ track, int startTick, int duration, BarEventFadeInfo^ fadeInfo)
	{
		_Timeline = timeline;
		_Track = track;
		_StartTick = startTick;
		_Duration = duration;
		_FadeInfo = fadeInfo;
		_Bar = nullptr;
	}

	void AddFadeBarCommand::Execute()
	{
		if (_Bar == nullptr) {
			_Bar = gcnew BarEvent(_Track, _StartTick, _Duration, _FadeInfo);
		}
		_Track->AddBar(_Bar);
		_Timeline->Invalidate();
	}

	void AddFadeBarCommand::Undo()
	{
		_Track->RemoveBar(_Bar);
		_Timeline->Invalidate();
	}

	String^ AddFadeBarCommand::GetDescription()
	{
		return "Add Fade Bar";
	}


	/////////////////////////
	// AddStrobeBarCommand //
	/////////////////////////
	AddStrobeBarCommand::AddStrobeBarCommand(Widget_Timeline^ timeline, Track^ track, int startTick, int duration, BarEventStrobeInfo^ strobeInfo)
	{
		_Timeline = timeline;
		_Track = track;
		_StartTick = startTick;
		_Duration = duration;
		_StrobeInfo = strobeInfo;
		_Bar = nullptr;
	}

	void AddStrobeBarCommand::Execute()
	{
		if (_Bar == nullptr) {
			_Bar = gcnew BarEvent(_Track, _StartTick, _Duration, _StrobeInfo);
		}
		_Track->AddBar(_Bar);
		_Timeline->Invalidate();
	}

	void AddStrobeBarCommand::Undo()
	{
		_Track->RemoveBar(_Bar);
		_Timeline->Invalidate();
	}

	String^ AddStrobeBarCommand::GetDescription()
	{
		return "Add Strobe Bar";
	}
}
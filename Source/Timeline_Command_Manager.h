#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	// Forward Declarations
	ref class Widget_Timeline;
	ref class Track;
	ref class BarEvent;
	ref class BarEventFadeInfo;
	ref class BarEventStrobeInfo;
	enum class FadeType;
	enum class Easing;

	
	// Base interface for all undoable commands
	public interface class ITimelineCommand {
		void Execute();
		void Undo();
		String^ GetDescription();
	};

	// Command manager that handles the undo/redo stacks
	public ref class TimelineCommandManager
	{
	public:
		delegate void CommandStateChangedHandler();
		event CommandStateChangedHandler^ CommandStateChanged;

	private:
		System::Collections::Generic::Stack<ITimelineCommand^>^ _UndoStack;
		System::Collections::Generic::Stack<ITimelineCommand^>^ _RedoStack;
		Widget_Timeline^ _Timeline;
		static const int MAX_UNDO_LEVELS = 100;

	public:
		TimelineCommandManager(Widget_Timeline^ timeline);

		List<ITimelineCommand^>^ GetCommands();
		int GetCurrentIndex();

		void ExecuteCommand(ITimelineCommand^ command);
		void Undo();
		void Redo();

		property bool CanUndo {
			bool get() { return _UndoStack->Count > 0; }
		}

		property bool CanRedo {
			bool get() { return _RedoStack->Count > 0; }
		}

		static BarEvent^ CreateBarCopy(BarEvent^ sourceBar, int startTick, bool isPreview);
	};

	// Compound command for operations affecting multiple bars
	public ref class CompoundCommand : public ITimelineCommand
	{
	private:
		List<ITimelineCommand^>^ _Commands;
		String^ _Description;

	public:
		CompoundCommand(String^ description);

		void AddCommand(ITimelineCommand^ command);
		int GetCommandCount();
		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Add Bar
	public ref class AddBarCommand : public ITimelineCommand
	{
	private:
		Track^ _Track;
		BarEvent^ _Bar;
		Widget_Timeline^ _Timeline;

	public:
		AddBarCommand(Widget_Timeline^ timeline, Track^ track, BarEvent^ bar);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Move Bar
	public ref class MoveBarCommand : public ITimelineCommand
	{
	private:
		BarEvent^ _Bar;
		Track^ _SourceTrack;
		Track^ _TargetTrack;
		int _OldStartTick;
		int _NewStartTick;
		Widget_Timeline^ _Timeline;

	public:
		MoveBarCommand(Widget_Timeline^ timeline, BarEvent^ bar, Track^ sourceTrack, Track^ targetTrack, int oldStartTick, int newStartTick);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Resize Bar
	public ref class ResizeBarCommand : public ITimelineCommand
	{
	private:
		BarEvent^ _Bar;
		int _OldDuration;
		int _NewDuration;
		Widget_Timeline^ _Timeline;

	public:
		ResizeBarCommand(Widget_Timeline^ timeline, BarEvent^ bar, int oldDuration, int newDuration);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Change Bar Color
	public ref class ChangeBarColorCommand : public ITimelineCommand
	{
	private:
		BarEvent^ _Bar;
		Color _OldColor;
		Color _NewColor;
		Widget_Timeline^ _Timeline;

	public:
		ChangeBarColorCommand(Widget_Timeline^ timeline, BarEvent^ bar, Color oldColor, Color newColor);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Change Fade Bar Colors
	public ref class ChangeFadeBarColorCommand : public ITimelineCommand
	{
	public:
		enum class ColorType {
			Start,
			Center,
			End
		};

	private:
		BarEvent^	_Bar;
		Color		_OldColor;
		Color		_NewColor;
		ColorType	_ColorType;
		Widget_Timeline^ _Timeline;

	public:
		ChangeFadeBarColorCommand(Widget_Timeline^ timeline, BarEvent^ bar, ColorType colorType, Color oldColor, Color newColor);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Delete/Erase Bar
	public ref class DeleteBarCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		Track^ _Track;
		BarEvent^ _Bar;

	public:
		DeleteBarCommand(Widget_Timeline^ timeline, Track^ track, BarEvent^ bar);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Paste Bars
	public ref class PasteBarCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		List<BarEvent^>^ _Bars;
		List<Track^>^ _Tracks;
		List<BarEvent^>^ _CreatedBars;

	public:
		PasteBarCommand(Widget_Timeline^ timeline, List<BarEvent^>^ bars, List<Track^>^ targetTracks);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();

		property List<BarEvent^>^ CreatedBars {
			List<BarEvent^>^ get() { return _CreatedBars; }
		}
	};

	// Add Fade Bar
	public ref class AddFadeBarCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		Track^ _Track;
		BarEvent^ _Bar;
		BarEventFadeInfo^ _FadeInfo;
		int _StartTick;
		int _Duration;

	public:
		AddFadeBarCommand(Widget_Timeline^ timeline, Track^ track, int startTick, int duration, BarEventFadeInfo^ fadeInfo);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Change Fade Type
	public ref class ChangeFadeTypeCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		BarEvent^ _Bar;
		FadeType _OldType;
		FadeType _NewType;
		Color _OldColorStart;
		Color _OldColorCenter;
		Color _OldColorEnd;
		Easing _OldEaseIn;
		Easing _OldEaseOut;

	public:
		ChangeFadeTypeCommand(Widget_Timeline^ timeline, BarEvent^ bar, FadeType newType);
		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Change Fade Easing
	public ref class ChangeFadeEasingCommand : public ITimelineCommand
	{
	public:
		enum class EasingType {
			InEasing,
			OutEasing
		};

	private:
		Widget_Timeline^ _Timeline;
		BarEvent^ _Bar;
		Easing _OldEasing;
		Easing _NewEasing;
		EasingType _EasingType;

	public:
		ChangeFadeEasingCommand(Widget_Timeline^ timeline, BarEvent^ bar, EasingType easingType, Easing oldEasing, Easing newEasing);
		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Change Fade Quantization
	public ref class ChangeFadeQuantizationCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		BarEvent^ _Bar;
		int _OldQuantization;
		int _NewQuantization;

	public:
		ChangeFadeQuantizationCommand(Widget_Timeline^ timeline, BarEvent^ bar, int oldQuantization, int newQuantization);
		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Add Strobe Bar
	public ref class AddStrobeBarCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		Track^ _Track;
		BarEvent^ _Bar;
		BarEventStrobeInfo^ _StrobeInfo;
		int _StartTick;
		int _Duration;

	public:
		AddStrobeBarCommand(Widget_Timeline^ timeline, Track^ track, int startTick, int duration, BarEventStrobeInfo^ strobeInfo);

		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};

	// Change Strobe Quantization
	public ref class ChangeStrobeQuantizationCommand : public ITimelineCommand
	{
	private:
		Widget_Timeline^ _Timeline;
		BarEvent^ _Bar;
		int _OldQuantization;
		int _NewQuantization;

	public:
		ChangeStrobeQuantizationCommand(Widget_Timeline^ timeline, BarEvent^ bar, int oldQuantization, int newQuantization);
		virtual void Execute();
		virtual void Undo();
		virtual String^ GetDescription();
	};
}
#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

#include "Timeline_Tool_Interface.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	ref class Widget_Timeline;
	ref class Track;
	ref class TrackMeasure;
	ref class BarEvent;
	ref class ChangeBarColorCommand;
	ref class ChangeFadeBarColorCommand;
	enum class DrawToolMode;

	// Base tool class
	public ref class TimelineTool abstract : public ITimelineToolAccess {
	public:
		event EventHandler^ SelectionChanged;

	protected:
		Widget_Timeline^	_Timeline;
		bool				_IsActive;
		bool				_CanSelectWithRectangle;
		bool				_IsSelecting;
		Point				_LastMousePos;
		Point				_SelectionStart;
		Keys				_ModifierKeys;

		Rectangle			_SelectionRect;
		List<BarEvent^>^	_SelectedBars;
		List<BarEvent^>^	_PreviewBars;

		System::Windows::Forms::Cursor^ _CurrentCursor;

	public:
		TimelineTool(Widget_Timeline^ timeline);

		virtual void Activate() { _IsActive = true; }
		virtual void Deactivate() { _IsActive = false; }
		
		virtual void OnMouseDown(MouseEventArgs^ e) = 0;
		virtual void OnMouseMove(MouseEventArgs^ e) = 0;
		virtual void OnMouseUp(MouseEventArgs^ e) = 0;
		virtual void OnKeyDown(KeyEventArgs^ e) = 0;
		virtual void OnKeyUp(KeyEventArgs^ e) = 0;

		virtual void OnCommandStateChanged() { };
		virtual void OnMouseRightClick(MouseEventArgs^ e);

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();

		property bool IsActive{
			bool get() { return _IsActive; }
		}

		property System::Windows::Forms::Cursor^ Cursor {
			virtual System::Windows::Forms::Cursor^ get() { return _CurrentCursor; }
			virtual void set(System::Windows::Forms::Cursor^ value);
		}

		virtual property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() { return _SelectedBars; }
		}

		virtual property List<BarEvent^>^ PreviewBars {
			List<BarEvent^> ^ get() { return _PreviewBars; }
		}

		virtual property bool IsSelecting {
			bool get() { return _IsSelecting && _CanSelectWithRectangle; }
		}

		virtual property Rectangle SelectionRect {
			Rectangle get() { return _SelectionRect; }
		}

		virtual property System::Drawing::Point CurrentMousePosition {
			Point get() { return _LastMousePos; }
		}

		virtual property bool IsDragging {
			bool get() { return false; }
		}

		virtual property bool IsMultiTrackSelection {
			bool get() { return false; }
		}

		virtual property bool IsPasting {
			bool get() { return false; }
		}
		
		virtual property List<BarEvent^>^ PastePreviewBars {
			List<BarEvent^>^ get() { return nullptr; }
		}

		virtual property Color DrawColor {
			Color get() { return Color(); }
		}

		virtual property int DrawTickLength {
			int get() { return 0; }
		}

		virtual property Track^ TargetTrack {
			Track^ get() { return nullptr; }
		}

		virtual property DrawToolMode CurrentMode {
			DrawToolMode get() { return DrawToolMode::Draw; }
		}

		virtual property bool IsResizing {
			bool get() { return false; }
		}

		virtual property BarEvent^ HoveredBar { 
			BarEvent^ get() { return nullptr; }
		}

		virtual property Color CurrentColor {
			Color get() { return Color(); }
		}

		virtual property float BarXHoverRatio {
			float get() { return 0.0f; }
		}

	protected:
		bool HasOverlappingBarsOnBarTrack(BarEvent^ bar);
		bool HasOverlappingBarsOnBarTrack(List<BarEvent^>^ barList);
		bool HasOverlappingBarsOnSpecificTrack(Track^ track, BarEvent^ bar);
		bool HasOverlappingBarsOnSpecificTrack(Track^ track, List<BarEvent^>^ barList);
		void OnSelectionChanged();
	};


	////////////////////////////////
	// PointerTool Implementation //
	////////////////////////////////
	public ref class TimelineClipboardManager abstract sealed {
	private:
		static List<BarEvent^>^ _ClipboardContent;

	public:
		static property List<BarEvent^>^ Content {
			List<BarEvent^> ^ get() {
				if (_ClipboardContent == nullptr) {
					_ClipboardContent = gcnew List<BarEvent^>();
				}
				return _ClipboardContent;
			}
		}

		static void Clear() {
			if (_ClipboardContent != nullptr) {
				_ClipboardContent->Clear();
			}
		}
	};

	public ref class PointerTool : public TimelineTool
	{
	private:
		int					_PasteStartTick;
		bool				_IsDragging;
		bool				_IsPasting;
		Point^				_DragStart;
		Track^				_DragSourceTrack;
		Track^				_DragTargetTrack;
		Track^				_PasteTargetTrack;
		List<BarEvent^>^	_PastePreviewBars;
		List<int>^			_OriginalBarStartTicks;

	public:
		PointerTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		virtual void OnCommandStateChanged() override;

		void StartMoving(Point mousePos);
		void UpdateMoving(Point mousePos);
		void FinishMoving(Point mousePos);
		void CancelMoving();
		
		void HandleCopy();
		void StartPaste();
		void UpdatePaste(Point mousePos);
		void FinishPaste();
		void CancelPaste();
		
		property bool IsDragging {
			bool get() override { return _IsDragging; }
		}

		property bool IsMultiTrackSelection {
			bool get() override { return IsMultiTrackList(_SelectedBars); }
		}

		property bool IsPasting {
			bool get() override { return _IsPasting; }
		}

		property List<BarEvent^>^ PastePreviewBars {
			List<BarEvent^>^ get() override { return _PastePreviewBars; }
		}

	private:
		bool IsMultiTrackList(List<BarEvent^>^ list);
		void StoreOriginalPositions();
		void UpdateGroupPosition(int tickDelta, bool allowTrackChange);
		void MoveSelectedBarsToTrack(Track^ targetTrack);
		void UpdateCursor(Point mousePos);
		void EraseSelectedBars();
	};


	/////////////////////////////
	// DrawTool Implementation //
	/////////////////////////////
	public ref class DrawTool : public TimelineTool
	{
	private:
		Track^		_TargetTrack;
		Track^		_SourceTrack;
		Color		_CurrentColor;
		int			_DrawTickLength;
		bool		_UseAutoLength;

		bool		_IsPainting;
		bool		_IsErasing;
		bool		_IsDragging;
		bool		_IsResizing;
		int			_LastPaintedTick;
		
		Point		_DragStartPoint;
		int			_DragStartTick;

		static const int RESIZE_HANDLE_WIDTH = 5;

		DrawToolMode _CurrentMode;

	public:
		DrawTool(Widget_Timeline^ timeline);

		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		void UpdatePreview(Point mousePos);
		void ClearPreview();
		void UpdateCurrentMode(Point mousePos);
		void UpdateActiveModesCursor();
		bool IsOverResizeHandle(Point mousePos, BarEvent^ bar, Track^ track);
		bool HasOverlappingBars(Track^ track, int startTick, int length);

		void StartPainting(Point mousePos);
		void UpdatePainting(Point mousePos);

		void StartErasing(Point mousePos);
		void UpdateErasing(Point mousePos);

		void StartMoving(Point mousePos);
		void UpdateMoving(Point mousePos);
		void FinishMoving(Point mousePos);
		void CancelMoving();

		void StartResizing(Point mousePos);
		void UpdateResizing(Point mousePos);
		void FinishResizing();
		void CancelResizing();

		int  GetBeatLength(Track^ track, int currentTick);

		property Color DrawColor {
			Color get() override { return _CurrentColor; }
			void set(Color value);
		}

		property int DrawTickLength {
			int get() override { return _DrawTickLength; }
			void set(int value);
		}

		property Track^ TargetTrack {
			Track^ get() override { return _TargetTrack; }
		}

		property DrawToolMode CurrentMode {
			DrawToolMode get() override { return _CurrentMode; }
		}

		property bool IsDragging {
			bool get() override { return _IsDragging; }
		}

		property bool IsResizing {
			bool get() override { return _IsResizing; }
		}

		property bool UseAutoLength {
			bool get() { return _UseAutoLength; }
			void set(bool value) { _UseAutoLength = value; }
		}
	};


	//////////////////////////////
	// SplitTool Implementation //
	//////////////////////////////
	public ref class SplitTool : public TimelineTool {
	private:
		int splitPreviewTick;
		Track^ hoverTrack;
		BarEvent^ hoverBar;

	public:
		SplitTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		void UpdateSplitPreview(Point mousePos);
		void ClearSplitPreview();
	};

	//////////////////////////////
	// EraseTool Implementation //
	//////////////////////////////
	public ref class EraseTool : public TimelineTool {
	private:
		BarEvent^	_HoverBar;

	public:
		EraseTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		virtual void OnCommandStateChanged() override;

		void EraseSelectedBars();

		void UpdateHoverPreview(Point mousePos);
		void ClearHoverPreview();

		void Activate() override;

		property BarEvent^ HoveredBar {
			BarEvent ^ get() override { return _HoverBar; }
		}
	};

	/////////////////////////////////
	// DurationTool Implementation //
	/////////////////////////////////
	public ref class DurationTool : public TimelineTool {
	private:
		bool		_IsResizing;
		BarEvent^	_TargetBar;
		Track^		_TargetTrack;
		int			_OriginalLength;
		int			_DragStartX;
		int			_ChangeTickLength;
		Dictionary<BarEvent^, int>^ _OriginalLengths;

		static const int HANDLE_WIDTH		= 5;
		static const int MIN_LENGTH_TICKS	= 120;	// Minimum 1/32 note

	public:
		DurationTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		virtual void OnCommandStateChanged() override;

		void StartResizing(Point mousePos);
		void UpdateResizing(Point mousePos);
		void FinishResizing();
		void CancelResizing();

		bool IsOverHandle(Point mousePos, BarEvent^ bar, Track^ track);
		void StoreOriginalLengths();
		
		virtual property bool IsResizing {
			bool get() override { return _IsResizing; }
		}

		virtual property BarEvent^ HoveredBar {
			BarEvent^ get()  override { return _TargetBar; }
		}

		property int ChangeTickLength {
			int get() { return _ChangeTickLength; }
			void set(int value) {_ChangeTickLength = Math::Max((int)MIN_LENGTH_TICKS, value); }
		}
	};

	//////////////////////////////
	// ColorTool Implementation //
	//////////////////////////////
	public ref class ColorTool : public TimelineTool
	{
	private:
		BarEvent^	_HoverBar;
		Color		_CurrentColor;
		float		_BarXHoverRatio;

	public:
		ColorTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		virtual void OnCommandStateChanged() override;

		void ApplyColorToSelectedBars();
		ChangeBarColorCommand^ ApplyColorToSelectedBarsSolid(BarEvent^ bar);
		ChangeFadeBarColorCommand^ ApplyColorToSelectedBarsFade(BarEvent^ bar);

		void UpdateHoverPreview(Point mousePos);
		void ClearHoverPreview();

		property BarEvent^ HoveredBar {
			BarEvent^ get() override { return _HoverBar; }
		}

		property Color CurrentColor {
			Color get() override { return _CurrentColor; }
			void set(Color value);
		}

		property float BarXHoverRatio {
			float get() override { return _BarXHoverRatio; }
		}
	};


	/////////////////////////////
	// FadeTool Implementation //
	/////////////////////////////
	public ref class FadeTool : public TimelineTool
	{
	private:
		bool		_IsDrawing;
		Point^		_DrawStart;
		Track^		_TargetTrack;
		int			_StartTick;

		int			_TickLength;
		Color		_ColorStart;
		Color		_ColorCenter;
		Color		_ColorEnd;
		FadeType	_Type;
		
		static const int MIN_DRAG_PIXELS = 5;
		
	public:
		FadeTool(Widget_Timeline^ timeline);

		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		property int TickLength {
			int get() { return _TickLength; }
			void set(int value);
		}

		property Color ColorStart {
			Color get() { return _ColorStart; }
			void set(Color color);
		}

		property Color ColorCenter {
			Color get() { return _ColorCenter; }
			void set(Color color);
		}

		property Color ColorEnd {
			Color get() { return _ColorEnd; }
			void set(Color color);
		}
		
		property FadeType Type {
			FadeType get() { return _Type; }
			void set(FadeType type);
		}

		property Track^ TargetTrack {
			Track^ get() override { return _TargetTrack; }
		}

		property Point CurrentMousePosition {
			Point get() override { return _LastMousePos; }
		}

	private:
		void AddBarToTrack();
		void UpdatePreview(Point mousePos);
		void ClearPreviews();
		void UpdatePreviewColors();
	};


	///////////////////////////////
	// StrobeTool Implementation //
	///////////////////////////////
	public ref class StrobeTool : public TimelineTool
	{
	private:
		bool		_IsDrawing;
		Point^		_DrawStart;
		Track^		_TargetTrack;
		int			_StartTick;

		int			_TickLength;
		Color		_ColorStrobe;

		static const int MIN_DRAG_PIXELS = 5;

	public:
		StrobeTool(Widget_Timeline^ timeline);

		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		property int TickLength {
			int get() { return _TickLength; }
			void set(int value);
		}

		property Color ColorStrobe {
			Color get() { return _ColorStrobe; }
			void set(Color color);
		}

		property Track^ TargetTrack {
			Track^ get() override { return _TargetTrack; }
		}

		virtual property Color CurrentColor {
			Color get() override { return ColorStrobe; }
		}

		property Point CurrentMousePosition {
			Point get() override { return _LastMousePos; }
		}

	private:
		void AddBarToTrack();
		void UpdateSinglePreview(Point mousePos);
		void ClearPreviews();
		void UpdatePreviewColor();
	};
}
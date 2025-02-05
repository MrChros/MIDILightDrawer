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
	enum class DrawToolMode;

	// Base tool class
	public ref class TimelineTool abstract : public ITimelineToolAccess {
	protected:
		Widget_Timeline^	_Timeline;
		bool				_IsActive;
		Point				_LastMousePos;
		Keys				_ModifierKeys;
		System::Windows::Forms::Cursor^ _CurrentCursor;

	public:
		TimelineTool(Widget_Timeline^ timeline);

		virtual void Activate() { _IsActive = true; }
		virtual void Deactivate() { _IsActive = false; }

		property bool IsActive {
			bool get() { return _IsActive; }
		}

		virtual void OnMouseDown(MouseEventArgs^ e) = 0;
		virtual void OnMouseMove(MouseEventArgs^ e) = 0;
		virtual void OnMouseUp(MouseEventArgs^ e) = 0;
		virtual void OnKeyDown(KeyEventArgs^ e) = 0;
		virtual void OnKeyUp(KeyEventArgs^ e) = 0;

		virtual void OnCommandStateChanged() {};

		property System::Windows::Forms::Cursor^ Cursor {
			virtual System::Windows::Forms::Cursor^ get() { return _CurrentCursor; }
			virtual void set(System::Windows::Forms::Cursor^ value);
		}

		virtual property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() { return nullptr; }
		}
		virtual property Rectangle SelectionRect {
			Rectangle get() { return Rectangle(); }
		}

		virtual property bool IsDragging {
			bool get() { return false; }
		}

		virtual property Track^ DragSourceTrack {
			Track^ get() { return nullptr; }
		}
		
		virtual property Track^ DragTargetTrack {
			Track^ get() { return nullptr; }
		}

		virtual property bool IsMultiTrackSelection {
			bool get() { return false; }
		}

		virtual property System::Drawing::Point CurrentMousePosition {
			Point get() { return Point(); }
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

		virtual property BarEvent^ PreviewBar {
			BarEvent^ get() { return nullptr; }
		}

		virtual property Track^ TargetTrack {
			Track^ get() { return nullptr; }
		}

		virtual property Track^ SourceTrack{
			Track ^ get() { return nullptr; }
		}

		virtual property DrawToolMode CurrentMode {
			DrawToolMode get() { return DrawToolMode::Draw; }
		}

		virtual property bool IsMoving {
			bool get() { return false; }
		}

		virtual property bool IsResizing {
			bool get() { return false; }
		}

		virtual property BarEvent^ HoverBar {
			BarEvent^ get() { return nullptr; }
		}

		virtual property Rectangle ErasePreviewRect {
			Rectangle get() { return Rectangle(); }
		}

		virtual property Rectangle PreviewRect {
			Rectangle get() { return Rectangle(); }
		}

		virtual property Color CurrentColor {
			Color get() { return Color(); }
		}

		virtual property float BarXHoverRatio {
			float get() { return 0.0f; }
		}

		virtual property List<BarEvent^>^ PreviewBars {
			List<BarEvent^>^ get() { return nullptr; }
		}

	protected:
		bool HasOverlappingBarsOnBarTrack(BarEvent^ bar);
		bool HasOverlappingBarsOnBarTrack(List<BarEvent^>^ barList);
		bool HasOverlappingBarsOnSpecificTrack(Track^ track, BarEvent^ bar);
		bool HasOverlappingBarsOnSpecificTrack(Track^ track, List<BarEvent^>^ barList);
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

	public ref class PointerTool : public TimelineTool {
	private:
		int					_PasteStartTick;
		bool				_IsDragging;
		bool				_IsSelecting;
		bool				_IsPasting;
		Point^				_DragStart;
		Track^				_DragSourceTrack;
		Track^				_DragTargetTrack;
		Track^				_PasteTargetTrack;
		Rectangle			_SelectionRect;
		List<BarEvent^>^	_SelectedBars;
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

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);

		void HandleCopy();
		void StartPaste();
		void UpdatePaste(Point mousePos);
		void FinishPaste();
		void CancelPaste();
		

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() override { return _SelectedBars; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override { return _SelectionRect; }
		}

		property bool IsDragging {
			bool get() override { return _IsDragging; }
		}

		property Track^ DragSourceTrack {
			Track^ get() override { return _DragSourceTrack; }
		}

		property Track^ DragTargetTrack {
			Track^ get() override { return _DragTargetTrack; }
		}

		property bool IsMultiTrackSelection {
			bool get() override { return IsMultiTrackList(_SelectedBars); }
		}

		property Point CurrentMousePosition {
			Point get() override { return _LastMousePos; }
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
		BarEvent^	_PreviewBar;
		Color		_CurrentColor;
		int			_DrawTickLength;
		bool		_UseAutoLength;

		bool		_IsPainting;
		bool		_IsErasing;
		bool		_IsMoving;
		bool		_IsResizing;
		int			_LastPaintedTick;
		BarEvent^	_HoverBar;
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

		property BarEvent^ PreviewBar {
			BarEvent^ get() override { return _PreviewBar; }
		}

		property Track^ TargetTrack {
			Track^ get() override { return _TargetTrack; }
		}

		property Track^ SourceTrack {
			Track ^ get() override { return _SourceTrack; }
		}

		property Track^ DragSourceTrack{
			Track ^ get() override { return _SourceTrack; }
		}

		property DrawToolMode CurrentMode {
			DrawToolMode get() override { return _CurrentMode; }
		}

		property bool IsMoving {
			bool get() override { return _IsMoving; }
		}

		property bool IsResizing {
			bool get() override { return _IsResizing; }
		}

		property BarEvent^ HoverBar {
			BarEvent^ get() override { return _HoverBar; }
		}

		property Point CurrentMousePosition {
			Point get() override { return _LastMousePos; }
		}

		property bool UseAutoLength{
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
		bool		_IsErasing;
		bool		_IsSelecting;
		Point^		_SelectionStart;
		Rectangle	_SelectionRect;
		List<BarEvent^>^ _SelectedBars;
		List<BarEvent^>^ _ErasedBars;
		BarEvent^	_HoverBar;
		Track^		_HoverTrack;
		Rectangle	_ErasePreviewRect;

	public:
		EraseTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		virtual void OnCommandStateChanged() override;

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();

		void StartErasing();
		void UpdateErasing(Point mousePos);
		void EndErasing();
		void EraseSelectedBars();
		void UpdateHoverPreview(Point mousePos);
		void ClearHoverPreview();
		void Activate() override;

		property Rectangle ErasePreviewRect {
			Rectangle get() override { return _ErasePreviewRect; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override { return _SelectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^> ^ get() override { return _SelectedBars; }
		}

		property BarEvent^ HoverBar {
			BarEvent ^ get() override { return _HoverBar; }
		}
	};

	/////////////////////////////////
	// DurationTool Implementation //
	/////////////////////////////////
	public ref class DurationTool : public TimelineTool {
	private:
		bool		_IsDragging;
		bool		_IsSelecting;
		Point^		_SelectionStart;
		Rectangle		_SelectionRect;
		List<BarEvent^>^ _SelectedBars;
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

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();

		void CancelResizing();

		bool IsOverHandle(Point mousePos, BarEvent^ bar, Track^ track);
		void StoreOriginalLengths();

		property BarEvent^ PreviewBar {
			BarEvent^ get() override { return _TargetBar; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override { return _SelectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() override { return _SelectedBars; }
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
		bool		_IsSelecting;
		Point^		_SelectionStart;
		Rectangle	_SelectionRect;
		Rectangle	_PreviewRect;
		List<BarEvent^>^ _SelectedBars;
		BarEvent^	_HoverBar;
		Track^		_HoverTrack;
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

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();

		void UpdateHoverPreview(Point mousePos);
		void ClearHoverPreview();

		void ApplyColorToSelection();

		property Rectangle PreviewRect {
			Rectangle get() override { return _PreviewRect; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override  { return _SelectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() override { return _SelectedBars; }
		}

		property BarEvent^ HoverBar {
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
		BarEvent^	_PreviewBar;
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

		property BarEvent^ PreviewBar {
			BarEvent^ get() override { return _PreviewBar; }
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
		BarEvent^	_PreviewBar;
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

		property BarEvent^ PreviewBar{
			BarEvent ^ get() override { return _PreviewBar; }
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
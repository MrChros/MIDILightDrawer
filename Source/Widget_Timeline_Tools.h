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
		Widget_Timeline^ _Timeline;
		bool	_IsActive;
		Point	_LastMousePos;
		Keys	_ModifierKeys;
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

		virtual property bool IsPreviewVisible {
				bool get() { return false; }
		}

		virtual property int PreviewLength {
			int get() { return 0; }
		}

		virtual property Rectangle PreviewRect {
			Rectangle get() { return Rectangle(); }
		}

		virtual property Color CurrentColor {
			Color get() { return Color(); }
		}

		virtual property List<BarEvent^>^ PreviewBars {
			List<BarEvent^>^ get() { return nullptr; }
		}
	};


	////////////////////////////////
	// PointerTool Implementation //
	////////////////////////////////
	public ref class TimelineClipboardItem {
	public:
		BarEvent^ Bar;
		int TrackIndex;  // Store track index instead of Track^ to avoid reference issues

		TimelineClipboardItem(BarEvent^ bar, int trackIdx) {
			Bar = bar;
			TrackIndex = trackIdx;
		}
	};

	public ref class TimelineClipboardManager abstract sealed {
	private:
		static List<TimelineClipboardItem^>^ _ClipboardContent;

	public:
		static property List<TimelineClipboardItem^>^ Content {
			List<TimelineClipboardItem^> ^ get() {
				if (_ClipboardContent == nullptr) {
					_ClipboardContent = gcnew List<TimelineClipboardItem^>();
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
		
		bool Is_Multi_Track_Selection();
		void StoreOriginalPositions();
		void UpdateGroupPosition(int tickDelta, bool allowTrackChange);
		void MoveSelectedBarsToTrack(Track^ targetTrack);
		void UpdateCursor(Point mousePos);
		void EraseSelectedBars();

	public:
		PointerTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		void StartMoving(Point mousePos);
		void UpdateMoving(Point mousePos);
		void FinishMoving(Point mousePos);
		void CancelMoving();

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);

		void HandleCopy();
		void HandlePaste();
		void UpdatePastePreview(Point mousePos);
		void CancelPaste();
		void FinalizePaste();

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
			bool get() override { return Is_Multi_Track_Selection(); }
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
		void StartPainting(Point mousePos);
		void UpdatePainting(Point mousePos);
		bool HasOverlappingBars(Track^ track, int startTick, int length);
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
		bool isErasing;
		bool isSelecting;
		Point^ selectionStart;
		Rectangle selectionRect;
		List<BarEvent^>^ selectedBars;
		List<BarEvent^>^ erasedBars;
		BarEvent^ hoverBar;
		Track^ hoverTrack;
		Rectangle erasePreviewRect;

	public:
		EraseTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();

		void StartErasing();
		void UpdateErasing(Point mousePos);
		void EndErasing();
		void UpdateHoverPreview(Point mousePos);
		void ClearHoverPreview();
		void Activate() override;

		property Rectangle ErasePreviewRect {
			Rectangle get() override { return erasePreviewRect; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override { return selectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^> ^ get() override { return selectedBars; }
		}

		property BarEvent^ HoverBar {
			BarEvent ^ get() override { return hoverBar; }
		}
	};

	/////////////////////////////////
	// DurationTool Implementation //
	/////////////////////////////////
	public ref class DurationTool : public TimelineTool {
	private:
		bool isDragging;
		bool isShowingPreview;
		bool isSelecting;
		Point^			selectionStart;
		Rectangle		selectionRect;
		List<BarEvent^>^ selectedBars;
		BarEvent^		targetBar;
		Track^			targetTrack;
		int originalLength;
		int dragStartX;
		int previewLength;
		int changeTickLength;
		Dictionary<BarEvent^, int>^ originalLengths;

		static const int	HANDLE_WIDTH		= 5;
		static const float	MIN_LENGTH_TICKS	= 120.0f;	// Minimum 1/32 note
		static const int	DEFAULT_TICK_LENGTH = 960;

	public:
		DurationTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		bool IsOverHandle(Point mousePos, BarEvent^ bar, Track^ track);
		void UpdateLengthPreview(Point mousePos);
		void ClearPreview();

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();
		void StoreOriginalLengths();

		property BarEvent^ PreviewBar {
			BarEvent^ get() override { return targetBar; }
		}

		property bool IsPreviewVisible {
			bool get() override { return isShowingPreview; }
		}

		property int PreviewLength {
			int get() override { return previewLength; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override { return selectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() override { return selectedBars; }
		}

		property int ChangeTickLength {
			int get() { return changeTickLength; }
			void set(int value) {
				changeTickLength = Math::Max((int)MIN_LENGTH_TICKS, value);
				// Update preview if active
				if (isShowingPreview && targetBar != nullptr) {
					UpdateLengthPreview(_LastMousePos);
				}
			}
		}
	};

	//////////////////////////////
	// ColorTool Implementation //
	//////////////////////////////
	public ref class ColorTool : public TimelineTool
	{
	private:
		bool isSelecting;
		Point^ selectionStart;
		Rectangle selectionRect;
		List<BarEvent^>^ selectedBars;
		BarEvent^ hoverBar;
		Track^ hoverTrack;
		Rectangle previewRect;
		Color currentColor;

	public:
		ColorTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		void StartSelection(Point start);
		void UpdateSelection(Point current);
		void EndSelection();
		void SelectBarsInRegion(Rectangle region);
		void ClearSelection();
		void UpdateHoverPreview(Point mousePos);
		void ClearHoverPreview();
		void ApplyColorToSelection();

		property Rectangle PreviewRect {
			Rectangle get() override { return previewRect; }
		}

		property Rectangle SelectionRect {
			Rectangle get() override  { return selectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() override { return selectedBars; }
		}

		property BarEvent^ HoverBar {
			BarEvent^ get() override { return hoverBar; }
		}

		property Color CurrentColor {
			Color get() override { return currentColor; }
			void set(Color value);
		}
	};


	/////////////////////////////
	// FadeTool Implementation //
	/////////////////////////////
	public ref class FadeTool : public TimelineTool
	{
	private:
		bool				isDrawing;
		Point^				drawStart;
		Track^				targetTrack;
		List<BarEvent^>^	previewBars;
		BarEvent^			previewBar;
		int					startTick;
		

		static const int DEFAULT_TICK_LENGTH	= 960 / 4; // 16th Note
		static const int MIN_DRAG_PIXELS		= 5;
		
	public:
		FadeTool(Widget_Timeline^ timeline);

		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		property int TickLength;
		property Color ColorStart;
		property Color ColorEnd;
		property Color ColorCenter;
		property FadeType Type;

		property List<BarEvent^>^ PreviewBars {
			List<BarEvent^>^ get() override { return previewBars; }
		}

		property Track^ TargetTrack {
			Track^ get() override { return targetTrack; }
		}

		property BarEvent^ PreviewBar {
			BarEvent^ get() override { return previewBar; }
		}

		property Point CurrentMousePosition {
			Point get() override { return _LastMousePos; }
		}

	private:
		Color InterpolateColor(Color start, Color end, float ratio);
		void CreatePreviewBars(Point currentPos);
		void AddBarsToTrack();
		void UpdateSinglePreview(Point mousePos);
		void ClearPreviews();
	};


	///////////////////////////////
	// StrobeTool Implementation //
	///////////////////////////////
	public ref class StrobeTool : public TimelineTool
	{
	private:
		bool				isDrawing;
		Point^				drawStart;
		Track^				targetTrack;
		List<BarEvent^>^	previewBars;
		BarEvent^			previewBar;
		int					startTick;

		static const int DEFAULT_TICK_LENGTH = 960 / 4; // 16th Note
		static const int MIN_DRAG_PIXELS = 5;

	public:
		StrobeTool(Widget_Timeline^ timeline);

		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

		property int TickLength;
		property Color StrobeColor;

		property List<BarEvent^>^ PreviewBars {
			List<BarEvent^>^ get() override { return previewBars; }
		}

		property Track^ TargetTrack {
			Track^ get() override { return targetTrack; }
		}

		property BarEvent^ PreviewBar{
			BarEvent ^ get() override { return previewBar; }
		}

		property Point CurrentMousePosition {
			Point get() override { return _LastMousePos; }
		}

	private:
		void CreatePreviewBars(Point currentPos);
		void AddBarsToTrack();
		void UpdateSinglePreview(Point mousePos);
		void ClearPreviews();
	};
}
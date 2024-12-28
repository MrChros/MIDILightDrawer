#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;


#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	ref class Widget_Timeline;
	ref class Track;
	ref class TrackMeasure;
	ref class BarEvent;

	// Base tool class
	public ref class TimelineTool abstract {
	protected:
		Widget_Timeline^ timeline;
		bool	isActive;
		Point	lastMousePos;
		Keys	modifierKeys;
		System::Windows::Forms::Cursor^ currentCursor;

	public:
		TimelineTool(Widget_Timeline^ timeline);

		virtual void Activate() { isActive = true; }
		virtual void Deactivate() { isActive = false; }
		property bool IsActive { bool get() { return isActive; } }

		virtual void OnMouseDown(MouseEventArgs^ e) = 0;
		virtual void OnMouseMove(MouseEventArgs^ e) = 0;
		virtual void OnMouseUp(MouseEventArgs^ e) = 0;
		virtual void OnKeyDown(KeyEventArgs^ e) = 0;
		virtual void OnKeyUp(KeyEventArgs^ e) = 0;

		property System::Windows::Forms::Cursor^ Cursor {
			virtual System::Windows::Forms::Cursor^ get() { return currentCursor; }
			virtual void set(System::Windows::Forms::Cursor^ value);
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
		static List<TimelineClipboardItem^>^ clipboardContent;

	public:
		static property List<TimelineClipboardItem^>^ Content {
			List<TimelineClipboardItem^> ^ get() {
				if (clipboardContent == nullptr) {
					clipboardContent = gcnew List<TimelineClipboardItem^>();
				}
				return clipboardContent;
			}
		}

		static void Clear() {
			if (clipboardContent != nullptr) {
				clipboardContent->Clear();
			}
		}
	};

	public ref class PointerTool : public TimelineTool {
	private:
		int					pasteStartTick;
		bool				isDragging;
		bool				isSelecting;
		bool				isPasting;
		Point^				dragStart;
		Track^				dragSourceTrack;
		Track^				dragTargetTrack;
		Track^				pasteTargetTrack;
		Rectangle			selectionRect;
		List<BarEvent^>^	selectedBars;
		List<BarEvent^>^	pastePreviewBars;
		
		bool Is_Multi_Track_Selection();
		void StoreOriginalPositions();
		void UpdateGroupPosition(int tickDelta, bool allowTrackChange);
		void MoveSelectedBarsToTrack(Track^ targetTrack);
		void UpdateCursor(Point mousePos);

	public:
		PointerTool(Widget_Timeline^ timeline);
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnKeyUp(KeyEventArgs^ e) override;

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
			List<BarEvent^>^ get() { return selectedBars; }
		}

		property Rectangle SelectionRect {
			Rectangle get() { return selectionRect; }
		}

		property bool IsDragging {
			bool get() { return isDragging; }
		}

		property Track^ DragSourceTrack {
			Track^ get() { return dragSourceTrack; }
		}

		property Track^ DragTargetTrack {
			Track^ get() { return dragTargetTrack; }
		}

		property bool IsMultiTrackSelection {
			bool get() { return Is_Multi_Track_Selection(); }
		}

		property Point CurrentMousePosition {
			Point get() { return lastMousePos; }
		}

		property bool IsPasting {
			bool get() { return isPasting; }
		}

		property List<BarEvent^>^ PastePreviewBars {
			List<BarEvent^>^ get() { return pastePreviewBars; }
		}
	};


	/////////////////////////////
	// DrawTool Implementation //
	/////////////////////////////
	public ref class DrawTool : public TimelineTool {
	public:
		enum class DrawMode {
			Draw,
			Erase,
			Move,
			Resize
		};

	private:
		Track^		targetTrack;
		Track^		sourceTrack;
		BarEvent^	previewBar;
		Color		_Current_Color;
		int			_Draw_Tick_Length;
		bool		_Use_Auto_Length;

		bool		isPainting;
		bool		isErasing;
		bool		isMoving;
		bool		isResizing;
		int			lastPaintedTick;
		BarEvent^	hoverBar;
		Point		dragStartPoint;
		int			dragStartTick;

		static const int RESIZE_HANDLE_WIDTH = 5;

		DrawMode currentMode;

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
		void StartResizing(Point mousePos);
		void UpdateResizing(Point mousePos);
		void FinishMoving();
		void FinishResizing();
		int  GetBeatLength(Track^ track, int currentTick);

		property Color DrawColor {
			Color get() { return _Current_Color; }
			void set(Color value);
		}

		property int DrawTickLength {
			int get() { return _Draw_Tick_Length; }
			void set(int value);
		}

		property BarEvent^ PreviewBar {
			BarEvent^ get() { return previewBar; }
		}

		property Track^ TargetTrack {
			Track^ get() { return targetTrack; }
		}

		property Track^ SourceTrack{
			Track ^ get() { return sourceTrack; }
		}

		property DrawMode CurrentMode {
			DrawMode get() { return currentMode; }
		}

		property bool IsMoving {
			bool get() { return isMoving; }
		}

		property bool IsResizing {
			bool get() { return isResizing; }
		}

		property BarEvent^ HoverBar {
			BarEvent^ get() { return hoverBar; }
		}

		property Point CurrentMousePosition {
			Point get() { return lastMousePos; }
		}

		property bool UseAutoLength{
			bool get() { return _Use_Auto_Length; }
			void set(bool value) { _Use_Auto_Length = value; }
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
		Rectangle get() { return erasePreviewRect; }
		}

		property Rectangle SelectionRect {
			Rectangle get() { return selectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^> ^ get() { return selectedBars; }
		}

		property BarEvent^ HoverBar {
			BarEvent ^ get() { return hoverBar; }
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
			BarEvent ^ get() { return targetBar; }
		}

		property bool IsPreviewVisible {
			bool get() { return isShowingPreview; }
		}

		property int PreviewLength {
			int get() { return previewLength; }
		}

		property Rectangle SelectionRect {
			Rectangle get() { return selectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() { return selectedBars; }
		}

		property int ChangeTickLength {
			int get() { return changeTickLength; }
			void set(int value) {
				changeTickLength = Math::Max((int)MIN_LENGTH_TICKS, value);
				// Update preview if active
				if (isShowingPreview && targetBar != nullptr) {
					UpdateLengthPreview(lastMousePos);
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
			Rectangle get() { return previewRect; }
		}

		property Rectangle SelectionRect {
			Rectangle get() { return selectionRect; }
		}

		property List<BarEvent^>^ SelectedBars {
			List<BarEvent^>^ get() { return selectedBars; }
		}

		property BarEvent^ HoverBar {
			BarEvent^ get() { return hoverBar; }
		}

		property Color CurrentColor {
			Color get() { return currentColor; }
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

		property List<BarEvent^>^ PreviewBars {
			List<BarEvent^>^ get() { return previewBars; }
		}

		property Track^ TargetTrack {
			Track^ get() { return targetTrack; }
		}

		property BarEvent^ PreviewBar {
			BarEvent^ get() { return previewBar; }
		}

		property Point CurrentMousePosition {
			Point get() { return lastMousePos; }
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
			List<BarEvent^>^ get() { return previewBars; }
		}

		property Track^ TargetTrack {
			Track^ get() { return targetTrack; }
		}

		property BarEvent^ PreviewBar{
			BarEvent ^ get() { return previewBar; }
		}

		property Point CurrentMousePosition {
			Point get() { return lastMousePos; }
		}

	private:
		void CreatePreviewBars(Point currentPos);
		void AddBarsToTrack();
		void UpdateSinglePreview(Point mousePos);
		void ClearPreviews();
	};
}
#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;

#include <set>
#include <cmath>

namespace MIDILightDrawer {

	public ref class Widget_Draw_Table : public Control
	{
	public:
		enum class NoteQuantization
		{
			Measure = 1,
			Half = 2,
			Quarter = 4,
			Eighth = 8,
			Sixteenth = 16,
			ThirtySecond = 32
		};

		enum class WidgetMode
		{
			Selection,
			Draw,
			Erase,
			Fade,
			Length,
			Color,
			Bucket
		};

		ref struct RGBBarInfo
		{
			int StartTick;
			int LengthInTicks;
			int Row;
			Color BarColor;
			Rectangle Bounds;
			bool IsSelected;

			RGBBarInfo() {
				StartTick = 0;
				LengthInTicks = TICKS_PER_QUARTER_NOTE;
				Row = 0;
				IsSelected = false;
			}
		};

	private:
		System::Resources::ResourceManager^ _Resources;

		// Structs
		ref struct ColumnHeaderText
		{
			String^ Line1;
			String^ Line2;
			String^ Line3;

			ColumnHeaderText()
			{
				Line1 = "";
				Line2 = "";
				Line3 = "";
			}
		};

		ref struct CellInfo
		{
			bool IsSelected;
			Rectangle Bounds;
		};

		ref struct ColumnInfo
		{
			ColumnHeaderText^ HeaderText;
			int StartTick;          // Starting tick position
			int LengthInTicks;      // Length in ticks
			int Width;
			int TimeSignatureNumerator;
			int TimeSignatureDenominator;
			Rectangle Bounds;
			array<CellInfo^>^ Cells;
			bool IsFirstInMeasure;
			int MeasureIndex;
			int SubdivisionIndex;
		};

		ref struct RowInfo
		{
			int Height;
			int TopOffset;
			List<String^>^ CellValues;
			String^ Label;
		};

		ref struct DraggedBarInfo {
			RGBBarInfo^ Bar;
			int OriginalRow;
			int OriginalStartTick;
			int TickOffset;  // Offset from first bar

			DraggedBarInfo(RGBBarInfo^ bar) {
				Bar = bar;
				OriginalRow = bar->Row;
				OriginalStartTick = bar->StartTick;
				TickOffset = 0;
			}
		};

		// Member variables
		List<ColumnInfo^>^ columns;
		List<RowInfo^>^ rows;
		List<ColumnInfo^>^ selectedColumns;
		List<RGBBarInfo^>^ rgbBars;
		int currentMeasureNumber;
		NoteQuantization currentQuantization;
		double horizontalScaleFactor;
		int currentPatternPower;

		int resizingRowIndex;
		Point mouseDownPoint;
		bool isSelecting;
		Point selectionStart;
		Rectangle selectionRectangle;

		WidgetMode currentMode;
		Color currentDrawColor;
		int drawLengthInTicks;
		int changeLengthInTicks;

		// Dragging Members
		bool isDraggingBar;
		RGBBarInfo^ draggedBar;
		Point dragOffset;
		int originalRow;
		int originalStartTick;

		bool isDraggingBars;
		List<DraggedBarInfo^>^ draggedBars;
		Point dragStart;
		bool isMultiRowDrag;


		// Copy and Paste
		List<RGBBarInfo^>^ clipboardBars;



		Point _CurrentScrollPosition;


		// Cursors
		System::Windows::Forms::Cursor^ penCursor;
		System::Windows::Forms::Cursor^ eraseCursor;
		System::Windows::Forms::Cursor^ selectCursor;
		System::Windows::Forms::Cursor^ fadeCursor;
		System::Windows::Forms::Cursor^ lengthCursor;
		System::Windows::Forms::Cursor^ colorCursor;
		System::Windows::Forms::Cursor^ bucketCursor;

		System::Drawing::Font^ rowLabelFont;

		// Constants
		static const int ROW_GAP = 20;
		static const int TITLE_HEIGHT = 75;
		static const int ROW_LABEL_WIDTH = 150;
		static const int HEADER_LINE_GAP = 2;
		static const int TICKS_PER_QUARTER_NOTE = 960;
		static const int PIXELS_PER_TICK_DIVISOR = 40;
		static const double MIN_SCALE_FACTOR = 0.1;
		static const double MAX_SCALE_FACTOR = 10.0;

	public:
		Widget_Draw_Table();

		// Public methods
		void SetQuantization(NoteQuantization quantization);
		void AddRow(String^ label);
		void UpdateRowLabel(int rowIndex, String^ newLabel);
		void AddColumn(int numerator, int denominator);
		void AddColumn(int numerator, int denominator, String^ customText);
		void Clear();
		void SetRowLabel(int rowIndex, String^ label);
		void SetHorizontalScale(double scaleFactor);
		double GetHorizontalScale();
		void SetMode(WidgetMode mode);
		void SetDrawColor(Color color);
		void SetDrawLengthInTicks(int lengthInTicks);
		void SetChangeLengthInTicks(int lengthInTicks);
		void DebugPrintRGBBars();
		int GetRowCount();
		int GetColumnCount();
		int GetRowLabelWidth();
		bool IsColumnFirstInMeasure(int index);
		int GetColumnWidth(int index);
		List<RGBBarInfo^>^ GetSelectedRGBBars();
		List<RGBBarInfo^>^ GetRGBBarsInTrack(String^ track_name);

		String^ Widget_Draw_Table::SaveRGBBarsToFile(String^ filePath);
		String^ Widget_Draw_Table::LoadRGBBarsFromFile(String^ filePath);

		void UpdateScrollPosition(Point scrollPos) { _CurrentScrollPosition = scrollPos; }
		void CenterOnMarker(int markerIndex);
				
		event EventHandler<int>^ RequestHorizontalScroll;

		property double TotalVirtualWidth
		{
			double get() { return CalculateTotalVirtualWidth(); }
		}

	protected:
		virtual void OnParentChanged(EventArgs^ e) override;
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;

	private:
		// Helper methods
		String^ FormatTimeSignature(int numerator, int denominator);
		String^ FormatMeasureNumber(int measureNumber);
		int CalculateMeasureTicks(int numerator, int denominator);
		int CalculateSubdivisionWidth(int numerator, int denominator, int subdivisions);
		int CalculateSubdivisions(int numerator, int denominator, NoteQuantization quantization);
		int GetTicksPerSubdivision(int numerator, int denominator, NoteQuantization quantization);
		int GetTicksAtColumn(int columnIndex);
		int GetNearestQuantizedTick(int tick, NoteQuantization quantization);
		double CalculateExactTickWidth(int ticks);
		System::Windows::Forms::Cursor^ CreateCursorFromResource(String^ resourceName);
		int FindColumnIndexByMarkerIndex(int markerIndex);
		int CalculateScrollPositionForColumn(int columnIndex);
		int FindCenteredMeasureColumn(int viewportCenterX);
		int CalculateScrollPositionForCenteredColumn(int columnIndex);

		// Event handlers
		void OnParentResize(Object^ sender, EventArgs^ e);
		void OnMouseMove(Object^ sender, MouseEventArgs^ e);
		void HandleMouseClick(MouseEventArgs^ e);
		void OnContextMenuOpening(Object^ sender, CancelEventArgs^ e);

		// Layout methods
		void RecalculateWidthAndLayout();
		void ApplyQuantization(NoteQuantization newQuantization);
		int FindColumnIndex(int x);
		int FindRowIndex(int y);
		void InitializeCursors();
		void UpdateCursor(Point mousePosition);
		double GetNearestAllowedScale(double requestedScale);
		double CalculateTotalVirtualWidth();
		Rectangle GetVisibleBounds();

		// Selection methods
		void ClearAllSelections();
		void UpdateCellSelection(MouseEventArgs^ e);

		// Rendering methods
		void RenderRowLabels(Graphics^ g);
		void RenderCells(Graphics^ g);
		void RenderColumnTitles(Graphics^ g);
		void RenderRGBBars(Graphics^ g);
		void RenderVisibleRGBBars(Graphics^ g, Rectangle visibleBounds);

		// RGB Bar methods - declarations only
		void AddRGBBar(int columnIndex, int rowIndex);
		void SelectRGBBar(int x, int y);
		void EraseRGBBar(int x, int y);
		void UpdateRGBBarBounds(RGBBarInfo^ bar);
		void UpdateRGBBarSelection(Point mousePoint, bool isSelectionFrame);
		void ClearRGBBarSelection();

		// Dragging Methods
		void StartDragging(RGBBarInfo^ bar, Point mousePoint);
		void UpdateDraggedBars(Point mousePoint);
		void EndDragging(Point mousePoint);
		RGBBarInfo^ GetBarAtPoint(Point point);
		int GetTickAtPoint(int x);

		// Copy and Paste
		void CopySelectedBars();
		void PasteFromClipboard(int columnIndex, int targetRow);
		void AddContextMenu();
	};
}
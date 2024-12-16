#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

#include <cmath>

namespace MIDILightDrawer {

	public ref class CustomTableWidget : public Control
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
			// Reserved for future modes
		};

	private:
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

		ref struct RGBBarInfo
		{
			int StartTick;      // Position in ticks
			int LengthInTicks;  // Length in ticks
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

		// Member variables
		List<ColumnInfo^>^ columns;
		List<RowInfo^>^ rows;
		List<ColumnInfo^>^ selectedColumns;
		List<RGBBarInfo^>^ rgbBars;
		int currentMeasureNumber;
		NoteQuantization currentQuantization;
		double horizontalScaleFactor;
		int currentPatternPower;

		bool isResizing;
		int resizingRowIndex;
		Point mouseDownPoint;
		Rectangle selectionRectangle;
		bool isSelecting;

		WidgetMode currentMode;
		Color currentDrawColor;
		double currentDrawWidth;

		System::Windows::Forms::Cursor^ penCursor;
		System::Windows::Forms::Cursor^ defaultCursor;

		// Constants
		static const int ROW_GAP = 20;
		static const int TITLE_HEIGHT = 75;
		static const int ROW_LABEL_WIDTH = 150;
		static const int HEADER_LINE_GAP = 2;
		static const int TICKS_PER_QUARTER_NOTE = 960;
		static const int PIXELS_PER_TICK_DIVISOR = 40;
		static const double MIN_SCALE_FACTOR = 0.1;
		static const double MAX_SCALE_FACTOR = 10.0;

		System::Drawing::Font^ rowLabelFont;

		// Helper Methods
		String^ FormatTimeSignature(int numerator, int denominator) {
			return String::Format("{0}/{1}", numerator, denominator);
		}

		String^ FormatMeasureNumber(int measureNumber) {
			return measureNumber.ToString();
		}

		int CalculateMeasureTicks(int numerator, int denominator) {
			double quarterNotesInMeasure = (4.0 * numerator) / denominator;
			return static_cast<int>(quarterNotesInMeasure * TICKS_PER_QUARTER_NOTE);
		}

		int CalculateSubdivisionWidth(int numerator, int denominator, int subdivisions) {
			if (subdivisions <= 0) {
				// Handle the error case by returning a minimum width
				return static_cast<int>(TICKS_PER_QUARTER_NOTE / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor);
			}

			int totalMeasureTicks = CalculateMeasureTicks(numerator, denominator);
			int ticksPerSubdivision = totalMeasureTicks / subdivisions;

			// Ensure we never return a width smaller than the minimum
			int calculatedWidth = static_cast<int>((ticksPerSubdivision / PIXELS_PER_TICK_DIVISOR) * horizontalScaleFactor);
			return calculatedWidth > 0 ? calculatedWidth : 1;
		}

		int CalculateSubdivisions(int numerator, int denominator, NoteQuantization quantization) {
			double quarterNotesInMeasure = (4.0 * numerator) / denominator;

			// For half note quantization request
			if (quantization == NoteQuantization::Half) {
				// Try half notes (2 quarter notes)
				if (std::fmod(quarterNotesInMeasure, 2.0) == 0.0) {
					return static_cast<int>(quarterNotesInMeasure / 2.0);
				}
				// Try quarter notes
				if (std::fmod(quarterNotesInMeasure, 1.0) == 0.0) {
					return static_cast<int>(quarterNotesInMeasure);
				}
				// Try eighth notes
				if (std::fmod(quarterNotesInMeasure, 0.5) == 0.0) {
					return static_cast<int>(quarterNotesInMeasure * 2.0);
				}
				// Fall back to sixteenth notes
				return static_cast<int>(std::ceil(quarterNotesInMeasure * 4.0));
			}

			// For quarter note quantization request
			if (quantization == NoteQuantization::Quarter) {
				// Try quarter notes
				if (std::fmod(quarterNotesInMeasure, 1.0) == 0.0) {
					return static_cast<int>(quarterNotesInMeasure);
				}
				// Try eighth notes
				if (std::fmod(quarterNotesInMeasure, 0.5) == 0.0) {
					return static_cast<int>(quarterNotesInMeasure * 2.0);
				}
				// Fall back to sixteenth notes
				return static_cast<int>(std::ceil(quarterNotesInMeasure * 4.0));
			}

			// For eighth note quantization request
			if (quantization == NoteQuantization::Eighth) {
				// Try eighth notes
				if (std::fmod(quarterNotesInMeasure, 0.5) == 0.0) {
					return static_cast<int>(quarterNotesInMeasure * 2.0);
				}
				// Fall back to sixteenth notes
				return static_cast<int>(std::ceil(quarterNotesInMeasure * 4.0));
			}

			// For sixteenth note quantization request
			if (quantization == NoteQuantization::Sixteenth) {
				return static_cast<int>(std::ceil(quarterNotesInMeasure * 4.0));
			}

			// Handle remaining cases
			switch (quantization) {
			case NoteQuantization::Measure:
				return 1;
			case NoteQuantization::ThirtySecond:
				return static_cast<int>(std::ceil(quarterNotesInMeasure * 8.0));
			default:
				return static_cast<int>(std::ceil(quarterNotesInMeasure));
			}
		}

		// Event Handlers
		void OnParentResize(Object^ sender, EventArgs^ e)
		{
			if (this->Parent != nullptr)
			{
				this->Height = this->Parent->ClientSize.Height;
				RecalculateWidthAndLayout();
			}
		}

		void OnMouseMove(Object^ sender, MouseEventArgs^ e)
		{
			UpdateCursor(e->Location);
			HandleMouseMove(e);
		}

		void HandleMouseMove(MouseEventArgs^ e)
		{
			if (e->Button == System::Windows::Forms::MouseButtons::Left && !isResizing)
			{
				UpdateCellSelection(e);
			}
			Invalidate();
		}

		/*void OnMouseDown(Object^ sender, MouseEventArgs^ e)
		{
			if (e->Button == System::Windows::Forms::MouseButtons::Left)
			{
				mouseDownPoint = e->Location;
				isSelecting = true;
				UpdateCellSelection(e);
			}
		}*/

		void OnMouseUp(Object^ sender, MouseEventArgs^ e)
		{
			if (e->Button == System::Windows::Forms::MouseButtons::Left)
			{
				isSelecting = false;
				isResizing = false;
				UpdateCellSelection(e);
			}
			Invalidate();
		}

		void ClearAllSelections()
		{
			for each (ColumnInfo ^ col in columns)
			{
				for each (CellInfo ^ cell in col->Cells)
				{
					cell->IsSelected = false;
				}
			}
		}

		void UpdateCellSelection(MouseEventArgs^ e)
		{
			selectionRectangle = Rectangle::FromLTRB(
				Math::Min(mouseDownPoint.X, e->X),
				Math::Min(mouseDownPoint.Y, e->Y),
				Math::Max(mouseDownPoint.X, e->X),
				Math::Max(mouseDownPoint.Y, e->Y)
			);

			ClearAllSelections();

			for each (ColumnInfo ^ col in columns)
			{
				for (int rowIndex = 0; rowIndex < rows->Count; rowIndex++)
				{
					if (col->Cells[rowIndex]->Bounds.IntersectsWith(selectionRectangle))
					{
						col->Cells[rowIndex]->IsSelected = true;
					}
				}
			}
		}

		// Layout Methods
		void RecalculateWidthAndLayout()
		{
			if (this->Parent == nullptr)
				return;

			int totalWidth = ROW_LABEL_WIDTH;
			for each (ColumnInfo ^ col in columns)
			{
				totalWidth += col->Width;
			}

			// Add extra space after the last measure (50 pixels)
			totalWidth += 50;

			this->Size = System::Drawing::Size(totalWidth, this->Parent->ClientSize.Height);

			// Rest of the method remains unchanged
			int xPosition = ROW_LABEL_WIDTH;
			for (int i = 0; i < columns->Count; i++)
			{
				ColumnInfo^ col = columns[i];
				col->Bounds = Rectangle(xPosition, TITLE_HEIGHT, col->Width, this->Height - TITLE_HEIGHT);
				xPosition += col->Width;

				for (int rowIndex = 0; rowIndex < rows->Count; rowIndex++)
				{
					RowInfo^ row = rows[rowIndex];
					col->Cells[rowIndex]->Bounds = Rectangle(
						col->Bounds.X,
						row->TopOffset,
						col->Width,
						row->Height
					);
				}
			}

			if (rows->Count > 0)
			{
				int availableHeight = this->Height - TITLE_HEIGHT;
				int totalGaps = (rows->Count * ROW_GAP);
				int rowHeight = (availableHeight - totalGaps) / rows->Count;

				int currentY = TITLE_HEIGHT;
				for (int i = 0; i < rows->Count; i++)
				{
					RowInfo^ row = rows[i];
					row->Height = rowHeight;
					row->TopOffset = currentY;
					currentY += rowHeight + ROW_GAP;

					for each (ColumnInfo ^ col in columns)
					{
						col->Cells[i]->Bounds = Rectangle(
							col->Bounds.X,
							row->TopOffset,
							col->Width,
							rowHeight
						);
					}
				}
			}

			for each(RGBBarInfo ^ bar in rgbBars) {
				UpdateRGBBarBounds(bar);
			}

			this->Invalidate();
		}

		// Rendering Methods
		void RenderRowLabels(Graphics^ g)
		{
			StringFormat^ format = gcnew StringFormat();
			format->Alignment = StringAlignment::Near;
			format->LineAlignment = StringAlignment::Center;

			for each (RowInfo ^ row in rows)
			{
				if (!String::IsNullOrEmpty(row->Label))
				{
					Rectangle labelBounds = Rectangle(
						0,
						row->TopOffset,
						ROW_LABEL_WIDTH - 5,
						row->Height
					);

					g->DrawString(
						row->Label,
						rowLabelFont,
						Brushes::Black,
						labelBounds,
						format
					);
				}
			}
		}

		void RenderCells(Graphics^ g)
		{
			int textLine1Bottom = static_cast<int>(TITLE_HEIGHT / 2 - (Font->Height * 1.5f + HEADER_LINE_GAP) + Font->Height);

			for each (ColumnInfo ^ col in columns)
			{
				for (int rowIndex = 0; rowIndex < rows->Count; rowIndex++)
				{
					CellInfo^ cell = col->Cells[rowIndex];

					// Draw the cell border
					Pen^ borderPen = gcnew Pen(Color::DarkGray, 1);
					g->DrawRectangle(borderPen, cell->Bounds);

					// Handle measure lines
					if (col->IsFirstInMeasure)
					{
						int measureIndex = col->MeasureIndex - 1;
						bool showText = (measureIndex % (1 << currentPatternPower)) == 0;

						if (!showText &&
							!String::IsNullOrEmpty(col->HeaderText->Line3) &&
							!String::IsNullOrEmpty(col->HeaderText->Line1) &&
							rowIndex == 0)
						{
							Pen^ dashedPen = gcnew Pen(borderPen->Color, borderPen->Width);
							dashedPen->DashStyle = Drawing2D::DashStyle::Dash;

							g->DrawLine(dashedPen,
								cell->Bounds.X,
								cell->Bounds.Y,
								cell->Bounds.X,
								textLine1Bottom
							);
						}
						else
						{
							g->DrawLine(borderPen,
								cell->Bounds.X,
								cell->Bounds.Y - 10,
								cell->Bounds.X,
								cell->Bounds.Y
							);
						}
					}

					// Draw cell selection only if we're not in RGB bar mode
					if (cell->IsSelected && currentMode != WidgetMode::Draw)
					{
						// Draw a semi-transparent selection overlay
						Color selectionColor = Color::FromArgb(40, 0, 120, 215);  // Light blue with alpha
						SolidBrush^ selectionBrush = gcnew SolidBrush(selectionColor);
						g->FillRectangle(selectionBrush, cell->Bounds);

						// Draw selection border
						Pen^ selectionPen = gcnew Pen(Color::FromArgb(0, 120, 215), 2);
						g->DrawRectangle(selectionPen, cell->Bounds);
					}
				}
			}
		}

		void RenderColumnTitles(Graphics^ g)
		{
			StringFormat^ format = gcnew StringFormat();
			format->Alignment = StringAlignment::Near;

			Rectangle lastTextBounds = Rectangle::Empty;
			int patternPower = 0;  // Start with 2^0 = 1 (show all)
			bool hasOverlap = true;

			// Keep increasing the pattern power until we find a pattern with no overlap
			while (hasOverlap && patternPower < 4)  // Limit to 2^4 = 16 as maximum skip
			{
				hasOverlap = false;
				lastTextBounds = Rectangle::Empty;
				int currentMeasure = 0;

				// Test current pattern for overlaps
				for each (ColumnInfo ^ col in columns)
				{
					if (!col->IsFirstInMeasure) continue;

					bool showText = (currentMeasure % (1 << patternPower)) == 0;

					if (showText && (col->HeaderText->Line2 != "" || col->HeaderText->Line3 != ""))
					{
						float y2 = TITLE_HEIGHT / 2 - Font->Height / 2;
						float y3 = TITLE_HEIGHT / 2 + (Font->Height / 2 + HEADER_LINE_GAP);

						float maxWidth = 0;
						if (!String::IsNullOrEmpty(col->HeaderText->Line2))
						{
							SizeF size2 = g->MeasureString(col->HeaderText->Line2, Font);
							maxWidth = Math::Max(maxWidth, size2.Width);
						}
						if (!String::IsNullOrEmpty(col->HeaderText->Line3))
						{
							SizeF size3 = g->MeasureString(col->HeaderText->Line3, Font);
							maxWidth = Math::Max(maxWidth, size3.Width);
						}

						Rectangle currentTextBounds = Rectangle(
							col->Bounds.X + 5,
							static_cast<int>(y2),
							static_cast<int>(maxWidth),
							static_cast<int>(y3 - y2 + Font->Height)
						);

						if (lastTextBounds != Rectangle::Empty &&
							currentTextBounds.IntersectsWith(lastTextBounds))
						{
							hasOverlap = true;
							break;
						}
						lastTextBounds = currentTextBounds;
					}
					currentMeasure++;
				}

				if (hasOverlap)
				{
					patternPower++;
				}
			}

			// Now render with the determined pattern
			lastTextBounds = Rectangle::Empty;
			int measureIndex = 0;

			for each (ColumnInfo ^ col in columns)
			{
				float y1 = TITLE_HEIGHT / 2 - (Font->Height * 1.5f + HEADER_LINE_GAP);
				float y2 = TITLE_HEIGHT / 2 - Font->Height / 2;
				float y3 = TITLE_HEIGHT / 2 + (Font->Height / 2 + HEADER_LINE_GAP);

				// Always render Line1 if it exists
				if (!String::IsNullOrEmpty(col->HeaderText->Line1))
				{
					g->DrawString(
						col->HeaderText->Line1,
						Font,
						Brushes::Black,
						col->Bounds.X + 5,
						y1
					);
				}

				// For Line2 and Line3, apply the determined pattern
				if (col->IsFirstInMeasure)
				{
					bool showText = (measureIndex % (1 << patternPower)) == 0;

					if (showText)
					{
						if (!String::IsNullOrEmpty(col->HeaderText->Line2))
						{
							g->DrawString(
								col->HeaderText->Line2,
								Font,
								Brushes::Black,
								col->Bounds.X + 5,
								y2
							);
						}

						if (!String::IsNullOrEmpty(col->HeaderText->Line3))
						{
							g->DrawString(
								col->HeaderText->Line3,
								Font,
								Brushes::Black,
								col->Bounds.X + 5,
								y3
							);
						}
					}
					measureIndex++;
				}
			}

			this->currentPatternPower = patternPower;
		}

		void RenderSelectionRectangle(Graphics^ g)
		{
			if (isSelecting)
			{
				Pen^ selectionPen = gcnew Pen(Color::Blue, 1);
				selectionPen->DashStyle = Drawing2D::DashStyle::Dash;
				g->DrawRectangle(selectionPen, selectionRectangle);
			}
		}

		void ApplyQuantization(NoteQuantization newQuantization)
		{
			if (columns->Count == 0) return;

			List<ColumnInfo^>^ newColumns = gcnew List<ColumnInfo^>();
			int measureIndex = 1;

			for each (ColumnInfo ^ originalCol in columns)
			{
				if (!originalCol->IsFirstInMeasure) continue;

				int subdivisions = CalculateSubdivisions(
					originalCol->TimeSignatureNumerator,
					originalCol->TimeSignatureDenominator,
					newQuantization
				);

				int subdivisionWidth = CalculateSubdivisionWidth(
					originalCol->TimeSignatureNumerator,
					originalCol->TimeSignatureDenominator,
					subdivisions
				);

				// subdivisionWidth now includes scaling factor from CalculateSubdivisionWidth

				for (int i = 0; i < subdivisions; i++)
				{
					ColumnInfo^ newCol = gcnew ColumnInfo();
					newCol->HeaderText = gcnew ColumnHeaderText();

					if (i == 0)
					{
						newCol->HeaderText->Line1 = originalCol->HeaderText->Line1;
						newCol->HeaderText->Line2 = measureIndex.ToString();
						newCol->HeaderText->Line3 = FormatTimeSignature(
							originalCol->TimeSignatureNumerator,
							originalCol->TimeSignatureDenominator
						);
						newCol->IsFirstInMeasure = true;
					}
					else
					{
						newCol->HeaderText->Line1 = "";
						newCol->HeaderText->Line2 = "";
						newCol->HeaderText->Line3 = "";
						newCol->IsFirstInMeasure = false;
					}

					newCol->TimeSignatureNumerator = originalCol->TimeSignatureNumerator;
					newCol->TimeSignatureDenominator = originalCol->TimeSignatureDenominator;
					newCol->Width = subdivisionWidth;
					newCol->MeasureIndex = measureIndex;
					newCol->SubdivisionIndex = i;
					newCol->Cells = gcnew array<CellInfo^>(rows->Count);

					for (int j = 0; j < rows->Count; j++)
					{
						newCol->Cells[j] = gcnew CellInfo();
						newCol->Cells[j]->IsSelected = false;
					}

					newColumns->Add(newCol);
				}
				measureIndex++;
			}

			columns = newColumns;
			currentQuantization = newQuantization;
			RecalculateWidthAndLayout();
		}

		int FindColumnIndex(int x)
		{
			int currentX = ROW_LABEL_WIDTH;
			for (int i = 0; i < columns->Count; i++)
			{
				if (x >= currentX && x < currentX + columns[i]->Width)
				{
					return i;
				}
				currentX += columns[i]->Width;
			}
			return -1;
		}

		int FindRowIndex(int y)
		{
			for (int i = 0; i < rows->Count; i++)
			{
				if (y >= rows[i]->TopOffset && y < rows[i]->TopOffset + rows[i]->Height)
				{
					return i;
				}
			}
			return -1;
		}

		void HandleMouseClick(MouseEventArgs^ e)
		{
			if (e->Button != System::Windows::Forms::MouseButtons::Left)
				return;

			int columnIndex = FindColumnIndex(e->X);
			int rowIndex = FindRowIndex(e->Y);

			switch (currentMode)
			{
			case WidgetMode::Draw:
				if (columnIndex >= 0 && rowIndex >= 0)
				{
					AddRGBBar(columnIndex, rowIndex);
				}
				break;

			case WidgetMode::Selection:
				SelectRGBBar(e->X, e->Y);
				break;

			case WidgetMode::Erase:
				EraseRGBBar(e->X, e->Y);
				break;
			}

			this->Invalidate();
		}

		int GetTicksPerSubdivision(int numerator, int denominator, NoteQuantization quantization) {
			int measureTicks = CalculateMeasureTicks(numerator, denominator);
			int subdivisions = CalculateSubdivisions(numerator, denominator, quantization);
			return measureTicks / subdivisions;
		}

		int GetTicksAtColumn(int columnIndex) {
			if (columnIndex < 0 || columnIndex >= columns->Count) return 0;

			int totalTicks = 0;
			int currentMeasureStartCol = 0;

			// Find start of current measure
			for (int i = columnIndex; i >= 0; i--) {
				if (columns[i]->IsFirstInMeasure) {
					currentMeasureStartCol = i;
					break;
				}
			}

			// Add ticks for complete measures
			for (int i = 0; i < currentMeasureStartCol; i++) {
				if (columns[i]->IsFirstInMeasure) {
					totalTicks += CalculateMeasureTicks(columns[i]->TimeSignatureNumerator,
						columns[i]->TimeSignatureDenominator);
				}
			}

			// Add ticks for subdivisions in current measure
			ColumnInfo^ measureCol = columns[currentMeasureStartCol];
			int ticksPerSubdivision = CalculateMeasureTicks(measureCol->TimeSignatureNumerator,
				measureCol->TimeSignatureDenominator) /
				CalculateSubdivisions(measureCol->TimeSignatureNumerator,
					measureCol->TimeSignatureDenominator,
					currentQuantization);

			totalTicks += (columnIndex - currentMeasureStartCol) * ticksPerSubdivision;

			return totalTicks;
		}

		void AddRGBBar(int columnIndex, int rowIndex) {
			// Get clicked column
			ColumnInfo^ col = columns[columnIndex];

			// Calculate start tick based on column's position and quantization
			int measureStartTick = 0;
			int subdivisionTick = 0;

			// Find measure start
			for (int i = 0; i < columns->Count; i++) {
				if (columns[i]->IsFirstInMeasure) {
					if (i > columnIndex) break;
					measureStartTick = GetTicksAtColumn(i);
				}
			}

			// Calculate subdivision tick offset
			int measureTicks = CalculateMeasureTicks(col->TimeSignatureNumerator, col->TimeSignatureDenominator);
			int ticksPerSubdivision = measureTicks / CalculateSubdivisions(col->TimeSignatureNumerator,
				col->TimeSignatureDenominator,
				currentQuantization);
			subdivisionTick = col->SubdivisionIndex * ticksPerSubdivision;

			// Calculate final start tick
			int startTick = measureStartTick + subdivisionTick;

			// Remove overlapping bars
			for (int i = rgbBars->Count - 1; i >= 0; i--) {
				RGBBarInfo^ existingBar = rgbBars[i];
				if (existingBar->Row == rowIndex) {
					int existingEnd = existingBar->StartTick + existingBar->LengthInTicks;
					int newEnd = startTick + currentDrawWidth;
					if ((startTick >= existingBar->StartTick && startTick < existingEnd) ||
						(newEnd > existingBar->StartTick && newEnd <= existingEnd)) {
						rgbBars->RemoveAt(i);
					}
				}
			}

			RGBBarInfo^ newBar = gcnew RGBBarInfo();
			newBar->StartTick = startTick;
			newBar->LengthInTicks = currentDrawWidth;
			newBar->Row = rowIndex;
			newBar->BarColor = currentDrawColor;

			UpdateRGBBarBounds(newBar);
			rgbBars->Add(newBar);
		}

		void SelectRGBBar(int x, int y)
		{
			// First, deselect all bars
			for each (RGBBarInfo ^ bar in rgbBars)
			{
				bar->IsSelected = false;
			}

			// Iterate through the bars in reverse order to select the topmost bar
			// when there are overlapping bars
			for (int i = rgbBars->Count - 1; i >= 0; i--)
			{
				RGBBarInfo^ bar = rgbBars[i];
				if (bar->Bounds.Contains(x, y))
				{
					bar->IsSelected = true;
					break; // Stop after selecting the first (topmost) bar that contains the point
				}
			}
		}

		void RenderRGBBars(Graphics^ g)
		{
			for each (RGBBarInfo ^ bar in rgbBars)
			{
				// Create semi-transparent brush for the bar
				SolidBrush^ barBrush = gcnew SolidBrush(Color::FromArgb(180,
					bar->BarColor.R, bar->BarColor.G, bar->BarColor.B));

				g->FillRectangle(barBrush, bar->Bounds);

				if (bar->IsSelected)
				{
					Pen^ selectionPen = gcnew Pen(Color::Yellow, 2);
					g->DrawRectangle(selectionPen, bar->Bounds);
				}
			}
		}

		void UpdateRGBBarBounds(RGBBarInfo^ bar) {
			// Find correct measure and pixel position based on ticks
			int currentTicks = 0;
			int startX = ROW_LABEL_WIDTH;

			for (int i = 0; i < columns->Count; i++) {
				if (columns[i]->IsFirstInMeasure) {
					int measureTicks = CalculateMeasureTicks(columns[i]->TimeSignatureNumerator,
						columns[i]->TimeSignatureDenominator);

					if (currentTicks <= bar->StartTick &&
						currentTicks + measureTicks > bar->StartTick) {
						// Found the measure containing our start tick
						double tickOffsetInMeasure = bar->StartTick - currentTicks;
						double measureWidth = measureTicks / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
						double offsetRatio = tickOffsetInMeasure / measureTicks;
						startX += static_cast<int>(measureWidth * offsetRatio);
						break;
					}

					startX += static_cast<int>(measureTicks / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor);
					currentTicks += measureTicks;
				}
			}

			// Calculate width based on ticks
			double width = (bar->LengthInTicks / PIXELS_PER_TICK_DIVISOR) * horizontalScaleFactor;

			RowInfo^ row = rows[bar->Row];
			int height = static_cast<int>(row->Height * 0.8);
			int yOffset = static_cast<int>(row->Height * 0.1);

			bar->Bounds = Rectangle(
				startX,
				row->TopOffset + yOffset,
				static_cast<int>(width),
				height
			);
		}

		void InitializeCursors()
		{
			// Create a custom cursor for draw mode using the pen symbol
			this->penCursor = Cursors::Cross;
			// Keep a reference to the default cursor
			this->defaultCursor = Cursors::Default;
		}

		void UpdateCursor(Point mousePosition)
		{
			bool isInTableArea = mousePosition.X > ROW_LABEL_WIDTH &&
				mousePosition.Y > TITLE_HEIGHT &&
				mousePosition.X < this->Width &&
				mousePosition.Y < this->Height;

			switch (currentMode)
			{
			case WidgetMode::Draw:
				this->Cursor = isInTableArea ? Cursors::Cross : Cursors::Default;
				break;

			case WidgetMode::Erase:
				this->Cursor = isInTableArea ? Cursors::Hand : Cursors::Default;
				break;

			default:
				this->Cursor = Cursors::Default;
				break;
			}
		}

		void EraseRGBBar(int x, int y)
		{
			for (int i = rgbBars->Count - 1; i >= 0; i--)
			{
				RGBBarInfo^ bar = rgbBars[i];
				if (bar->Bounds.Contains(x, y))
				{
					rgbBars->RemoveAt(i);
					this->Invalidate();
					break;
				}
			}
		}

	protected:
		virtual void OnParentChanged(EventArgs^ e) override
		{
			if (this->Parent != nullptr)
			{
				this->Parent->Resize -= gcnew EventHandler(this, &CustomTableWidget::OnParentResize);
				this->Parent->Resize += gcnew EventHandler(this, &CustomTableWidget::OnParentResize);
				this->Height = this->Parent->ClientSize.Height;
				RecalculateWidthAndLayout();
			}
			Control::OnParentChanged(e);
		}

		virtual void OnPaint(PaintEventArgs^ e) override
		{
			e->Graphics->Clear(Color::White);
			RenderRowLabels(e->Graphics);
			RenderCells(e->Graphics);
			RenderRGBBars(e->Graphics);
			RenderColumnTitles(e->Graphics);
			if (isSelecting)
				RenderSelectionRectangle(e->Graphics);
		}

		// Modify MouseDown event handler
		void OnMouseDown(Object^ sender, MouseEventArgs^ e)
		{
			HandleMouseClick(e);
		}

	public:
		CustomTableWidget(int rowCount) : Control()
		{
			currentMeasureNumber = 1;
			currentQuantization = NoteQuantization::Measure;
			horizontalScaleFactor = 1.0;
			columns = gcnew List<ColumnInfo^>();
			rows = gcnew List<RowInfo^>();
			selectedColumns = gcnew List<ColumnInfo^>();

			rgbBars = gcnew List<RGBBarInfo^>();
			currentMode = WidgetMode::Selection;
			currentDrawColor = Color::Red;
			currentDrawWidth = 1;

			rowLabelFont = gcnew System::Drawing::Font(
				this->Font->FontFamily,
				this->Font->Size + 2,
				FontStyle::Bold
			);

			for (int i = 0; i < rowCount; i++)
			{
				RowInfo^ row = gcnew RowInfo();
				row->Height = 30;
				row->CellValues = gcnew List<String^>();
				row->Label = "";
				rows->Add(row);
			}

			SetStyle(ControlStyles::OptimizedDoubleBuffer |
				ControlStyles::AllPaintingInWmPaint |
				ControlStyles::UserPaint |
				ControlStyles::ResizeRedraw, true);

			InitializeCursors();

			this->MouseMove += gcnew MouseEventHandler(this, &CustomTableWidget::OnMouseMove);
			this->MouseDown += gcnew MouseEventHandler(this, &CustomTableWidget::OnMouseDown);
			this->MouseUp += gcnew MouseEventHandler(this, &CustomTableWidget::OnMouseUp);

			this->BackColor = Color::White;
			this->ForeColor = Color::Black;
			this->DoubleBuffered = true;
			this->MinimumSize = System::Drawing::Size(100, 100);
			this->AutoSize = false;

			this->Padding = System::Windows::Forms::Padding(0);
			this->Margin = System::Windows::Forms::Padding(0);
		}

		void SetQuantization(NoteQuantization quantization)
		{
			ApplyQuantization(quantization);
		}

		void AddColumn(int numerator, int denominator)
		{
			AddColumn(numerator, denominator, "");
		}

		void AddColumn(int numerator, int denominator, String^ customText)
		{
			ColumnInfo^ newColumn = gcnew ColumnInfo();
			newColumn->HeaderText = gcnew ColumnHeaderText();
			newColumn->HeaderText->Line1 = customText;
			newColumn->HeaderText->Line2 = currentMeasureNumber.ToString();
			newColumn->HeaderText->Line3 = FormatTimeSignature(numerator, denominator);
			newColumn->TimeSignatureNumerator = numerator;
			newColumn->TimeSignatureDenominator = denominator;
			newColumn->IsFirstInMeasure = true;
			newColumn->MeasureIndex = currentMeasureNumber;
			newColumn->SubdivisionIndex = 0;

			int subdivisions = CalculateSubdivisions(numerator, denominator, currentQuantization);
			int subdivisionWidth = CalculateSubdivisionWidth(numerator, denominator, subdivisions);
			newColumn->Width = subdivisionWidth;

			newColumn->Cells = gcnew array<CellInfo^>(rows->Count);

			for (int i = 0; i < rows->Count; i++)
			{
				newColumn->Cells[i] = gcnew CellInfo();
				newColumn->Cells[i]->IsSelected = false;
			}

			columns->Add(newColumn);

			// Add additional subdivisions if needed
			for (int i = 1; i < subdivisions; i++)
			{
				ColumnInfo^ subdivisionColumn = gcnew ColumnInfo();
				subdivisionColumn->HeaderText = gcnew ColumnHeaderText();
				subdivisionColumn->TimeSignatureNumerator = numerator;
				subdivisionColumn->TimeSignatureDenominator = denominator;
				subdivisionColumn->Width = subdivisionWidth;
				subdivisionColumn->IsFirstInMeasure = false;
				subdivisionColumn->MeasureIndex = currentMeasureNumber;
				subdivisionColumn->SubdivisionIndex = i;
				subdivisionColumn->Cells = gcnew array<CellInfo^>(rows->Count);

				for (int j = 0; j < rows->Count; j++)
				{
					subdivisionColumn->Cells[j] = gcnew CellInfo();
					subdivisionColumn->Cells[j]->IsSelected = false;
				}

				columns->Add(subdivisionColumn);
			}

			currentMeasureNumber++;
			RecalculateWidthAndLayout();
		}

		void Clear()
		{
			columns->Clear();
			currentMeasureNumber = 1;
			this->Width = ROW_LABEL_WIDTH + MinimumSize.Width;
			RecalculateWidthAndLayout();
		}

		void SetRowLabel(int rowIndex, String^ label)
		{
			if (rowIndex >= 0 && rowIndex < rows->Count)
			{
				rows[rowIndex]->Label = label;
				this->Invalidate();
			}
		}

		// Add new method to set scale factor
		void SetHorizontalScale(double scaleFactor)
		{
			// Validate and clamp the scale factor
			if (scaleFactor < MIN_SCALE_FACTOR)
				scaleFactor = MIN_SCALE_FACTOR;
			if (scaleFactor > MAX_SCALE_FACTOR)
				scaleFactor = MAX_SCALE_FACTOR;

			// Only update if the scale actually changed
			if (horizontalScaleFactor != scaleFactor)
			{
				horizontalScaleFactor = scaleFactor;

				ApplyQuantization(currentQuantization);

				// Trigger layout update
				RecalculateWidthAndLayout();
			}
		}

		// Add getter for current scale
		double GetHorizontalScale()
		{
			return horizontalScaleFactor;
		}

		// Public methods for mode and drawing settings
		void SetMode(WidgetMode mode)
		{
			currentMode = mode;

			UpdateCursor(PointToClient(Control::MousePosition));
		}

		void SetDrawParameters(Color color, int lengthInTicks) {
			currentDrawColor = color;
			currentDrawWidth = lengthInTicks > 0 ? lengthInTicks : TICKS_PER_QUARTER_NOTE;
		}

		void DebugPrintRGBBars() {
			Console::WriteLine("\n=== RGB Bars Debug Info ===");
			for (int i = 0; i < rgbBars->Count; i++) {
				RGBBarInfo^ bar = rgbBars[i];
				Console::WriteLine(String::Format(
					"Bar {0}:\n  Row: {1}\n  StartTick: {2}\n  Length: {3} ticks\n  Bounds: X={4}, Y={5}, W={6}, H={7}\n  Color: R={8}, G={9}, B={10}",
					i,
					bar->Row,
					bar->StartTick,
					bar->LengthInTicks,
					bar->Bounds.X,
					bar->Bounds.Y,
					bar->Bounds.Width,
					bar->Bounds.Height,
					bar->BarColor.R,
					bar->BarColor.G,
					bar->BarColor.B
				));
			}
			Console::WriteLine("========================\n");
		}
	};

} // end namespace MIDILightDrawer
#include "Widget_Draw_Table.h"

namespace MIDILightDrawer {
	////////////////////
	// Public Methods //
	////////////////////
	void Widget_Draw_Table::DebugPrintRGBBars()
	{
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

	List<Widget_Draw_Table::RGBBarInfo^>^ Widget_Draw_Table::GetSelectedRGBBars()
	{
		List<RGBBarInfo^>^ selectedBars = gcnew List<RGBBarInfo^>();
		for each (RGBBarInfo ^ bar in rgbBars) {
			if (bar->IsSelected) {
				selectedBars->Add(bar);
			}
		}
		return selectedBars;
	}

	List<Widget_Draw_Table::RGBBarInfo^>^ Widget_Draw_Table::GetRGBBarsInTrack(String^ track_name)
	{
		int Row_Number = -1;
		for(int i=0;i<rows->Count;i++)
		{
			if (rows[i]->Label == track_name)
			{
				Row_Number = i;
				break;
			}
		}

		if (Row_Number == -1) {
			return nullptr;
		}
				
		List<RGBBarInfo^>^ Track_Bars = gcnew List<RGBBarInfo^>();

		for each (RGBBarInfo ^ bar in rgbBars) {
			if (bar->Row == Row_Number) {
				Track_Bars->Add(bar);
			}
		}

		return Track_Bars;
	}

	String^ Widget_Draw_Table::SaveRGBBarsToFile(String^ filePath)
	{
		try
		{
			List<String^>^ lines = gcnew List<String^>();

			// Header with version
			lines->Add("MIDILightDrawer_RGBBars_v1.2");

			// Save pattern information
			int measureCount = 0;
			for each(ColumnInfo ^ col in columns) {
				if (col->IsFirstInMeasure) measureCount++;
			}
			lines->Add(measureCount.ToString());

			// Save time signatures
			for each (ColumnInfo ^ col in columns) {
				if (col->IsFirstInMeasure) {
					lines->Add(String::Format("{0},{1}",
						col->TimeSignatureNumerator,
						col->TimeSignatureDenominator));
				}
			}

			lines->Add(rgbBars->Count.ToString());

			// Save each RGB bar's data with row label
			for each (RGBBarInfo ^ bar in rgbBars)
			{
				String^ barData = String::Format("{0},{1},{2},{3},{4},{5},{6}",
					bar->StartTick,
					bar->LengthInTicks,
					bar->Row,
					bar->BarColor.R,
					bar->BarColor.G,
					bar->BarColor.B,
					rows[bar->Row]->Label
				);
				lines->Add(barData);
			}

			System::IO::File::WriteAllLines(filePath, lines->ToArray());
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error saving RGB bars: {0}", ex->Message);
		}
	}

	String^ Widget_Draw_Table::LoadRGBBarsFromFile(String^ filePath)
	{
		try
		{
			array<String^>^ lines = System::IO::File::ReadAllLines(filePath);
			if (lines->Length < 2 || !lines[0]->StartsWith("MIDILightDrawer_RGBBars_v1.2"))
				return "Invalid or unsupported file format version";

			// Parse number of measures
			int filePatternMeasureCount;
			if (!Int32::TryParse(lines[1], filePatternMeasureCount))
			{
				return "Invalid measure count";
			}

			// Count current measures
			int currentMeasureCount = 0;
			for each(ColumnInfo ^ col in columns) {
				if (col->IsFirstInMeasure) currentMeasureCount++;
			}

			// Verify measure count matches
			if (filePatternMeasureCount != currentMeasureCount)
			{
				return String::Format(
					"Pattern mismatch: File has {0} measures, but widget has {1} measures",
					filePatternMeasureCount, currentMeasureCount);
			}

			// Verify time signatures for each measure
			int currentMeasureIndex = 0;
			for each(ColumnInfo ^ col in columns)
			{
				if (col->IsFirstInMeasure)
				{
					String^ fileMeasureInfo = lines[2 + currentMeasureIndex];
					array<String^>^ parts = fileMeasureInfo->Split(',');

					if (parts->Length != 2)
					{
						return String::Format(
							"Invalid time signature format in measure {0}",
							currentMeasureIndex + 1);
					}

					int fileNumerator, fileDenominator;
					if (!Int32::TryParse(parts[0], fileNumerator) ||
						!Int32::TryParse(parts[1], fileDenominator))
					{
						return String::Format(
							"Invalid time signature numbers in measure {0}",
							currentMeasureIndex + 1);
					}

					if (fileNumerator != col->TimeSignatureNumerator ||
						fileDenominator != col->TimeSignatureDenominator)
					{
						return String::Format(
							"Time signature mismatch at measure {0}: File has {1}/{2}, but widget has {3}/{4}",
							currentMeasureIndex + 1,
							fileNumerator, fileDenominator,
							col->TimeSignatureNumerator, col->TimeSignatureDenominator);
					}

					currentMeasureIndex++;
				}
			}

			// Create mapping of row labels to indices
			Dictionary<String^, int>^ rowLabelMap = gcnew Dictionary<String^, int>();
			for (int i = 0; i < rows->Count; i++) {
				rowLabelMap[rows[i]->Label] = i;
			}

			rgbBars->Clear();
			int rgbBarsStartLine = 2 + filePatternMeasureCount;
			int barCount;
			if (!Int32::TryParse(lines[rgbBarsStartLine], barCount))
				return "Invalid bar count";

			// Load each bar
			for (int i = 0; i < barCount; i++)
			{
				String^ barData = lines[rgbBarsStartLine + 1 + i];
				array<String^>^ parts = barData->Split(',');
				if (parts->Length != 7)
					continue; // Skip invalid format

				String^ rowLabel = parts[6];
				if (!rowLabelMap->ContainsKey(rowLabel))
					continue; // Skip if row label doesn't exist

				RGBBarInfo^ bar = gcnew RGBBarInfo();
				if (!Int32::TryParse(parts[0], bar->StartTick) ||
					!Int32::TryParse(parts[1], bar->LengthInTicks))
					continue;

				int r, g, b;
				if (!Int32::TryParse(parts[3], r) ||
					!Int32::TryParse(parts[4], g) ||
					!Int32::TryParse(parts[5], b))
					continue;

				bar->Row = rowLabelMap[rowLabel];
				bar->BarColor = Color::FromArgb(r, g, b);
				bar->IsSelected = false;

				UpdateRGBBarBounds(bar);
				rgbBars->Add(bar);
			}

			this->Invalidate();
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error loading RGB bars: {0}", ex->Message);
		}
	}

	/////////////////////
	// Private Methods //
	/////////////////////
	void Widget_Draw_Table::RenderRGBBars(Graphics^ g)
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

	void Widget_Draw_Table::RenderVisibleRGBBars(Graphics^ g, Rectangle visibleBounds)
	{
		if (rgbBars == nullptr || rgbBars->Count == 0)
			return;

		// Calculate tick range that's visible
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		int startTick = static_cast<int>((visibleBounds.X - ROW_LABEL_WIDTH) / pixelsPerTick);
		int endTick = static_cast<int>((visibleBounds.X + visibleBounds.Width - ROW_LABEL_WIDTH) / pixelsPerTick);

		// Only process bars that could be visible
		for each (RGBBarInfo ^ bar in rgbBars)
		{
			// Check if bar is in visible tick range
			int barEndTick = bar->StartTick + bar->LengthInTicks;
			if (bar->StartTick > endTick || barEndTick < startTick)
				continue;

			// Calculate bar bounds in current view
			double barStartX = (bar->StartTick * pixelsPerTick) + ROW_LABEL_WIDTH - visibleBounds.X;
			double barWidth = bar->LengthInTicks * pixelsPerTick;

			// Check if the row is visible
			if (bar->Row < rows->Count)
			{
				RowInfo^ row = rows[bar->Row];
				if (row->TopOffset < visibleBounds.Y + visibleBounds.Height &&
					row->TopOffset + row->Height > visibleBounds.Y)
				{
					// Update bar bounds for current view
					int height = static_cast<int>(row->Height * 0.8);
					int yOffset = static_cast<int>(row->Height * 0.1);

					bar->Bounds = Rectangle(
						static_cast<int>(barStartX),
						row->TopOffset + yOffset,
						static_cast<int>(barWidth),
						height
					);

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
		}
	}

	void Widget_Draw_Table::AddRGBBar(int columnIndex, int rowIndex)
	{
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

		// Store current scroll position
		Point currentScroll = _CurrentScrollPosition;

		// Calculate final start tick
		int startTick = measureStartTick + subdivisionTick;

		// Remove overlapping bars
		for (int i = rgbBars->Count - 1; i >= 0; i--) {
			RGBBarInfo^ existingBar = rgbBars[i];
			if (existingBar->Row == rowIndex) {
				int existingEnd = existingBar->StartTick + existingBar->LengthInTicks;
				int newEnd = startTick + drawLengthInTicks;
				if ((startTick >= existingBar->StartTick && startTick < existingEnd) ||
					(newEnd > existingBar->StartTick && newEnd <= existingEnd)) {
					rgbBars->RemoveAt(i);
				}
			}
		}

		RGBBarInfo^ newBar = gcnew RGBBarInfo();
		newBar->StartTick = startTick;
		newBar->LengthInTicks = drawLengthInTicks;
		newBar->Row = rowIndex;
		newBar->BarColor = currentDrawColor;

		UpdateRGBBarBounds(newBar);
		rgbBars->Add(newBar);

		if (this->Parent != nullptr) {
			Panel^ container = safe_cast<Panel^>(this->Parent);
			container->HorizontalScroll->Value = currentScroll.X;
			container->VerticalScroll->Value = currentScroll.Y;
			this->UpdateScrollPosition(currentScroll);
		}
	}

	void Widget_Draw_Table::SelectRGBBar(int x, int y)
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

	void Widget_Draw_Table::EraseRGBBar(int x, int y)
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

	void Widget_Draw_Table::UpdateRGBBarBounds(RGBBarInfo^ bar)
	{
		// Convert all start tick to pixels directly using the scale factor
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		double startX = ROW_LABEL_WIDTH + (bar->StartTick * pixelsPerTick);
		double width = bar->LengthInTicks * pixelsPerTick;

		// Only round at the final step when creating the Rectangle
		RowInfo^ row = rows[bar->Row];
		int height = static_cast<int>(row->Height * 0.8);
		int yOffset = static_cast<int>(row->Height * 0.1);

		bar->Bounds = Rectangle(
			static_cast<int>(Math::Round(startX)),
			row->TopOffset + yOffset,
			static_cast<int>(Math::Round(width)),
			height
		);
	}

	void Widget_Draw_Table::UpdateRGBBarSelection(Point mousePoint, bool isSelectionFrame) {
		if (!isSelectionFrame) {
			// Single click selection - clear previous selection first
			ClearRGBBarSelection();

			// Select the bar under the cursor
			for (int i = rgbBars->Count - 1; i >= 0; i--) {
				RGBBarInfo^ bar = rgbBars[i];
				if (bar->Bounds.Contains(mousePoint)) {
					bar->IsSelected = true;
					break;  // Select only the topmost bar
				}
			}
		}
		else {
			// Selection frame - select all bars that intersect with the frame
			for each (RGBBarInfo ^ bar in rgbBars) {
				bar->IsSelected = bar->Bounds.IntersectsWith(selectionRectangle);
			}
		}
	}

	void Widget_Draw_Table::ClearRGBBarSelection() {
		for each (RGBBarInfo ^ bar in rgbBars) {
			bar->IsSelected = false;
		}
	}
}
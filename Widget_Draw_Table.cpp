#include "Widget_Draw_Table.h"
#include "Widget_Draw_Table_Container.h"

namespace MIDILightDrawer {

	////////////////////
	// Public Methods //
	////////////////////
	Widget_Draw_Table::Widget_Draw_Table() : Control()
	{
		_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
		if (_Resources == nullptr) {
			Console::WriteLine("Failed to create ResourceManager");
		}

		currentMeasureNumber = 1;
		currentQuantization = NoteQuantization::Measure;
		horizontalScaleFactor = 1.0;
		columns = gcnew List<ColumnInfo^>();
		rows = gcnew List<RowInfo^>();
		selectedColumns = gcnew List<ColumnInfo^>();

		rgbBars = gcnew List<RGBBarInfo^>();
		currentMode = WidgetMode::Selection;
		currentDrawColor = Color::Red;
		drawLengthInTicks = TICKS_PER_QUARTER_NOTE;
		changeLengthInTicks = TICKS_PER_QUARTER_NOTE;

		isSelecting = false;

		rowLabelFont = gcnew System::Drawing::Font(
			this->Font->FontFamily,
			this->Font->Size + 2,
			FontStyle::Bold
		);

		SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::ResizeRedraw, true);

		UpdateStyles();

		InitializeCursors();
		AddContextMenu();

		this->SetStyle(ControlStyles::Selectable, true);
		this->TabStop = true;

		this->MouseMove += gcnew MouseEventHandler(this, &Widget_Draw_Table::OnMouseMove);

		this->BackColor = Color::White;
		this->ForeColor = Color::Black;
		this->DoubleBuffered = true;
		this->MinimumSize = System::Drawing::Size(100, 100);
		this->AutoSize = false;

		this->Padding = System::Windows::Forms::Padding(0);
		this->Margin = System::Windows::Forms::Padding(0);
	}

	void Widget_Draw_Table::SetQuantization(NoteQuantization quantization)
	{
		ApplyQuantization(quantization);
		this->Invalidate();
	}

	void Widget_Draw_Table::AddRow(String^ label)
	{
		RowInfo^ row = gcnew RowInfo();
		row->Height = 30;
		row->CellValues = gcnew List<String^>();
		row->Label = label;
		rows->Add(row);

		// Update column cells to match new row count
		for each (ColumnInfo ^ col in columns)
		{
			Array::Resize(col->Cells, rows->Count);
			col->Cells[rows->Count - 1] = gcnew CellInfo();
			col->Cells[rows->Count - 1]->IsSelected = false;
		}

		RecalculateWidthAndLayout();
		this->Invalidate();
	}

	void Widget_Draw_Table::UpdateRowLabel(int rowIndex, String^ newLabel)
	{
		if (rowIndex >= 0 && rowIndex < rows->Count)
		{
			rows[rowIndex]->Label = newLabel;
			this->Invalidate();
		}
	}

	void Widget_Draw_Table::AddColumn(int numerator, int denominator)
	{
		AddColumn(numerator, denominator, "");
	}

	void Widget_Draw_Table::AddColumn(int numerator, int denominator, String^ customText)
	{
		int currentTick = 0;
		// Find current total ticks
		for each (ColumnInfo ^ col in columns) {
			if (col->IsFirstInMeasure) {
				currentTick += CalculateMeasureTicks(col->TimeSignatureNumerator, col->TimeSignatureDenominator);
			}
		}

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

		int measureTicks = CalculateMeasureTicks(numerator, denominator);
		int subdivisions = CalculateSubdivisions(numerator, denominator, currentQuantization);
		int ticksPerSubdivision = measureTicks / subdivisions;

		newColumn->StartTick = currentTick;
		newColumn->LengthInTicks = ticksPerSubdivision;

		// Initialize cells for all existing rows
		if (rows->Count > 0) {
			newColumn->Cells = gcnew array<CellInfo^>(rows->Count);
			for (int i = 0; i < rows->Count; i++) {
				newColumn->Cells[i] = gcnew CellInfo();
				newColumn->Cells[i]->IsSelected = false;
			}
		}
		else {
			newColumn->Cells = gcnew array<CellInfo^>(0);
		}

		columns->Add(newColumn);

		// Add additional subdivisions
		for (int i = 1; i < subdivisions; i++) {
			ColumnInfo^ subdivisionColumn = gcnew ColumnInfo();
			subdivisionColumn->HeaderText = gcnew ColumnHeaderText();
			subdivisionColumn->TimeSignatureNumerator = numerator;
			subdivisionColumn->TimeSignatureDenominator = denominator;
			subdivisionColumn->IsFirstInMeasure = false;
			subdivisionColumn->MeasureIndex = currentMeasureNumber;
			subdivisionColumn->SubdivisionIndex = i;

			// Initialize cells for subdivision columns
			if (rows->Count > 0) {
				subdivisionColumn->Cells = gcnew array<CellInfo^>(rows->Count);
				for (int j = 0; j < rows->Count; j++) {
					subdivisionColumn->Cells[j] = gcnew CellInfo();
					subdivisionColumn->Cells[j]->IsSelected = false;
				}
			}
			else {
				subdivisionColumn->Cells = gcnew array<CellInfo^>(0);
			}

			subdivisionColumn->StartTick = currentTick + (i * ticksPerSubdivision);
			subdivisionColumn->LengthInTicks = ticksPerSubdivision;

			columns->Add(subdivisionColumn);
		}

		currentMeasureNumber++;
		RecalculateWidthAndLayout();
	}

	void Widget_Draw_Table::Clear()
	{
		columns->Clear();
		rows->Clear();
		rgbBars->Clear();
		currentMeasureNumber = 1;
		this->Width = ROW_LABEL_WIDTH + MinimumSize.Width;
		RecalculateWidthAndLayout();
	}

	void Widget_Draw_Table::SetRowLabel(int rowIndex, String^ label)
	{
		if (rowIndex >= 0 && rowIndex < rows->Count)
		{
			rows[rowIndex]->Label = label;
			this->Invalidate();
		}
	}

	// Add new method to set scale factor
	void Widget_Draw_Table::SetHorizontalScale(double scaleFactor)
	{
		double snappedScale = GetNearestAllowedScale(scaleFactor);

		if (horizontalScaleFactor != snappedScale)
		{
			horizontalScaleFactor = snappedScale;
			ApplyQuantization(currentQuantization);
			RecalculateWidthAndLayout();

			// Force layout update
			this->PerformLayout();
			this->Invalidate();

			// Width has changed - this will trigger the SizeChanged event
			this->OnSizeChanged(gcnew EventArgs());
		}
		//// Snap the scale factor to nearest allowed value
		//double snappedScale = GetNearestAllowedScale(scaleFactor);

		//// Only update if the snapped scale actually changed
		//if (horizontalScaleFactor != snappedScale && this->Parent != nullptr)
		//{
		//	Panel^ container = safe_cast<Panel^>(this->Parent);

		//	// Find the measure column closest to the center of the viewport
		//	int viewportCenterX = container->HorizontalScroll->Value + (container->ClientSize.Width / 2);
		//	int centerColumnIndex = FindCenteredMeasureColumn(viewportCenterX);

		//	if (centerColumnIndex >= 0)
		//	{
		//		// Store the old scale factor
		//		double oldScale = horizontalScaleFactor;

		//		// Update scale factor and recalculate layout
		//		horizontalScaleFactor = snappedScale;
		//		ApplyQuantization(currentQuantization);
		//		RecalculateWidthAndLayout();

		//		// Calculate new scroll position to keep the same measure centered
		//		int newScrollPosition = CalculateScrollPositionForCenteredColumn(centerColumnIndex);

		//		// Set the container's horizontal scroll position
		//		container->HorizontalScroll->Value = newScrollPosition;

		//		// Update the stored scroll position
		//		this->UpdateScrollPosition(Point(newScrollPosition, container->VerticalScroll->Value));
		//	}
		//	else
		//	{
		//		// If no measure column is found, just update the scale
		//		horizontalScaleFactor = snappedScale;
		//		ApplyQuantization(currentQuantization);
		//		RecalculateWidthAndLayout();
		//	}
		//}
	}

	// Add getter for current scale
	double Widget_Draw_Table::GetHorizontalScale()
	{
		return horizontalScaleFactor;
	}

	// Public methods for mode and drawing settings
	void Widget_Draw_Table::SetMode(WidgetMode mode)
	{
		currentMode = mode;

		UpdateCursor(PointToClient(Control::MousePosition));
	}

	void Widget_Draw_Table::SetDrawColor(Color color) {
		currentDrawColor = color;
	}

	void Widget_Draw_Table::SetDrawLengthInTicks(int lengthInTicks) {
		drawLengthInTicks = lengthInTicks > 0 ? lengthInTicks : TICKS_PER_QUARTER_NOTE;
	}

	void Widget_Draw_Table::SetChangeLengthInTicks(int lengthInTicks) {
		changeLengthInTicks = lengthInTicks > 0 ? lengthInTicks : TICKS_PER_QUARTER_NOTE;
	}

	int Widget_Draw_Table::GetRowCount()
	{
		return rows->Count;
	}

	int Widget_Draw_Table::GetRowLabelWidth() {
		return ROW_LABEL_WIDTH;
	}

	int Widget_Draw_Table::GetColumnCount() {
		return columns->Count;
	}

	bool Widget_Draw_Table::IsColumnFirstInMeasure(int index) {
		if (index >= 0 && index < columns->Count) {
			return columns[index]->IsFirstInMeasure;
		}
		return false;
	}

	int Widget_Draw_Table::GetColumnWidth(int index) {
		if (index >= 0 && index < columns->Count) {
			return columns[index]->Width;
		}
		return 0;
	}

	void Widget_Draw_Table::CenterOnMarker(int markerIndex)
	{
		if (markerIndex < 0) return;

		int columnIndex = FindColumnIndexByMarkerIndex(markerIndex);
		if (columnIndex >= 0 && this->Parent != nullptr)
		{
			Panel^ container = safe_cast<Panel^>(this->Parent);
			int scrollPosition = CalculateScrollPositionForColumn(columnIndex);

			// Set the container's horizontal scroll position
			container->HorizontalScroll->Value = scrollPosition;

			// Update the stored scroll position in the Draw Table
			this->UpdateScrollPosition(Point(scrollPosition, container->VerticalScroll->Value));

			// Update the last centered measure in the container
			Widget_Draw_Table_Container^ tableContainer = safe_cast<Widget_Draw_Table_Container^>(this->Parent);
			tableContainer->UpdateLastCenteredMeasure();
		}
	}

	///////////////////////
	// Protected Methods //
	///////////////////////
	void Widget_Draw_Table::OnParentChanged(EventArgs^ e) 
	{
		if (this->Parent != nullptr)
		{
			this->Parent->Resize -= gcnew EventHandler(this, &Widget_Draw_Table::OnParentResize);
			this->Parent->Resize += gcnew EventHandler(this, &Widget_Draw_Table::OnParentResize);
			this->Height = this->Parent->ClientSize.Height;
			RecalculateWidthAndLayout();
		}
		Control::OnParentChanged(e);
	}

	void Widget_Draw_Table::OnPaint(PaintEventArgs^ e) 
	{
		// Get the visible bounds
		Rectangle visible = GetVisibleBounds();

		// Clear the background
		e->Graphics->Clear(Color::White);

		// Calculate the scroll offset
		Point scrollOffset = _CurrentScrollPosition;

		// Calculate visible content range
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		int startTick = static_cast<int>((scrollOffset.X - ROW_LABEL_WIDTH) / pixelsPerTick);
		int endTick = static_cast<int>((scrollOffset.X + visible.Width - ROW_LABEL_WIDTH) / pixelsPerTick);

		// Find visible column range
		int startColumnIndex = -1;
		int endColumnIndex = -1;
		int accumulatedTicks = 0;

		for (int i = 0; i < columns->Count; i++)
		{
			if (accumulatedTicks >= startTick && startColumnIndex == -1)
				startColumnIndex = i;

			accumulatedTicks += columns[i]->LengthInTicks;

			if (accumulatedTicks > endTick && endColumnIndex == -1)
			{
				endColumnIndex = i + 1;
				break;
			}
		}

		if (startColumnIndex == -1) startColumnIndex = 0;
		if (endColumnIndex == -1) endColumnIndex = columns->Count;

		// Add buffer columns
		startColumnIndex = Math::Max(0, startColumnIndex - 2);
		endColumnIndex = Math::Min(columns->Count, endColumnIndex + 2);

		// Save current transform
		Drawing2D::Matrix^ originalTransform = e->Graphics->Transform;

		// First render fixed elements (row labels) without transform
		RenderRowLabels(e->Graphics);

		// Apply transform for scrollable content
		Drawing2D::Matrix^ scrollTransform = gcnew Drawing2D::Matrix();
		scrollTransform->Translate(-scrollOffset.X, 0);
		e->Graphics->Transform = scrollTransform;

		// Update visible column bounds
		double currentX = ROW_LABEL_WIDTH;
		for (int i = 0; i < columns->Count; i++)
		{
			ColumnInfo^ col = columns[i];
			double columnWidth = col->LengthInTicks * pixelsPerTick;

			if (i >= startColumnIndex && i < endColumnIndex)
			{
				// Update column bounds
				col->Bounds = Rectangle(
					static_cast<int>(currentX),
					TITLE_HEIGHT,
					static_cast<int>(Math::Ceiling(columnWidth)),
					this->Height - TITLE_HEIGHT
				);

				// Update cell bounds
				if (col->Cells != nullptr)
				{
					for (int rowIndex = 0; rowIndex < rows->Count && rowIndex < col->Cells->Length; rowIndex++)
					{
						RowInfo^ row = rows[rowIndex];
						col->Cells[rowIndex]->Bounds = Rectangle(
							static_cast<int>(currentX),
							row->TopOffset,
							static_cast<int>(Math::Ceiling(columnWidth)),
							row->Height
						);
					}
				}
			}

			currentX += columnWidth;
		}

		// Render content
		RenderColumnTitles(e->Graphics);
		RenderCells(e->Graphics);
		RenderVisibleRGBBars(e->Graphics, visible);

		// Restore original transform
		e->Graphics->Transform = originalTransform;
	}

	// Modify MouseDown event handler
	void Widget_Draw_Table::OnMouseDown(MouseEventArgs^ e)
	{
		this->Focus();
		
		if (e->Button == System::Windows::Forms::MouseButtons::Left) {
			if (currentMode == WidgetMode::Selection) {
				RGBBarInfo^ clickedBar = GetBarAtPoint(e->Location);
				if (clickedBar != nullptr) {
					StartDragging(clickedBar, e->Location);
				}
				else {
					// Existing selection rectangle logic
					isSelecting = true;
					selectionStart = e->Location;
					selectionRectangle = Rectangle(selectionStart, System::Drawing::Size(0, 0));
					UpdateRGBBarSelection(e->Location, false);
				}
				this->Invalidate();
			}
			else {
				HandleMouseClick(e);
			}
		}
		Control::OnMouseDown(e);
	}

	void Widget_Draw_Table::OnMouseUp(MouseEventArgs^ e)
	{
		if (e->Button == System::Windows::Forms::MouseButtons::Left) {
			if (isDraggingBars) {
				EndDragging(e->Location);
			}
			isSelecting = false;
			this->Invalidate();
		}
		Control::OnMouseUp(e);
	}

	void Widget_Draw_Table::OnKeyDown(KeyEventArgs^ e)
	{
		if (e->Control && e->KeyCode == Keys::C)
		{
			CopySelectedBars();
			e->Handled = true;
		}

		Control::OnKeyDown(e);
	}

	/////////////////////
	// Private Methods //
	/////////////////////
	String^ Widget_Draw_Table::FormatTimeSignature(int numerator, int denominator)
	{
		return String::Format("{0}/{1}", numerator, denominator);
	}

	String^ Widget_Draw_Table::FormatMeasureNumber(int measureNumber)
	{
		return measureNumber.ToString();
	}

	int Widget_Draw_Table::CalculateMeasureTicks(int numerator, int denominator)
	{
		double quarterNotesInMeasure = (4.0 * numerator) / denominator;
		return static_cast<int>(quarterNotesInMeasure * TICKS_PER_QUARTER_NOTE);
	}

	int Widget_Draw_Table::CalculateSubdivisionWidth(int numerator, int denominator, int subdivisions)
	{
		// Calculate total measure ticks first
		int totalMeasureTicks = CalculateMeasureTicks(numerator, denominator);

		if (subdivisions <= 0) {
			subdivisions = 1;  // Prevent division by zero
		}

		// Calculate ticks per subdivision
		double ticksPerSubdivision = static_cast<double>(totalMeasureTicks) / subdivisions;

		// Convert ticks to pixels using scale factor
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		double width = ticksPerSubdivision * pixelsPerTick;

		// Ensure minimum width of 1 pixel
		return static_cast<int>(Math::Max(1.0, Math::Round(width)));
	}

	int Widget_Draw_Table::CalculateSubdivisions(int numerator, int denominator, NoteQuantization quantization)
	{
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

	int Widget_Draw_Table::GetTicksPerSubdivision(int numerator, int denominator, NoteQuantization quantization)
	{
		int measureTicks = CalculateMeasureTicks(numerator, denominator);
		int subdivisions = CalculateSubdivisions(numerator, denominator, quantization);
		return measureTicks / subdivisions;
	}

	int Widget_Draw_Table::GetTicksAtColumn(int columnIndex)
	{
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

	int Widget_Draw_Table::GetNearestQuantizedTick(int tick, NoteQuantization quantization)
	{
		// Get the tick interval for current quantization
		int quantizationTicks;
		switch (quantization)
		{
		case NoteQuantization::Half:
			quantizationTicks = TICKS_PER_QUARTER_NOTE * 2;
			break;
		case NoteQuantization::Quarter:
			quantizationTicks = TICKS_PER_QUARTER_NOTE;
			break;
		case NoteQuantization::Eighth:
			quantizationTicks = TICKS_PER_QUARTER_NOTE / 2;
			break;
		case NoteQuantization::Sixteenth:
			quantizationTicks = TICKS_PER_QUARTER_NOTE / 4;
			break;
		case NoteQuantization::ThirtySecond:
			quantizationTicks = TICKS_PER_QUARTER_NOTE / 8;
			break;
		default:
			return tick; // For Measure, keep original position
		}

		// Round to nearest quantization tick
		return static_cast<int>(Math::Round(static_cast<double>(tick) / quantizationTicks) * quantizationTicks);
	}

	double Widget_Draw_Table::CalculateExactTickWidth(int ticks)
	{
		return (static_cast<double>(ticks) / PIXELS_PER_TICK_DIVISOR) * horizontalScaleFactor;
	}

	Cursor^ Widget_Draw_Table::CreateCursorFromResource(String^ resourceName)
	{
		try {
			array<Byte>^ cursorBytes = (array<Byte>^)_Resources->GetObject(resourceName);
			System::IO::MemoryStream^ memoryStream = gcnew System::IO::MemoryStream(cursorBytes);
			System::Windows::Forms::Cursor^ cursor = gcnew System::Windows::Forms::Cursor(memoryStream);
			memoryStream->Close();
			return cursor;
		}
		catch (Exception^ ex) {
			Console::WriteLine("Error loading custom cursor '" + resourceName + "': " + ex->Message);
			return Cursors::Cross;  // Fallback to default
		}
	}

	int Widget_Draw_Table::FindColumnIndexByMarkerIndex(int markerIndex)
	{
		if (markerIndex < 0)
			return -1;

		int currentMarkerCount = 0;
		for (int i = 0; i < columns->Count; i++)
		{
			if (columns[i]->HeaderText != nullptr &&
				!String::IsNullOrEmpty(columns[i]->HeaderText->Line1))
			{
				if (currentMarkerCount == markerIndex)
				{
					return i;
				}
				currentMarkerCount++;
			}
		}
		return -1;
	}

	int Widget_Draw_Table::CalculateScrollPositionForColumn(int columnIndex)
	{
		if (columnIndex < 0 || columnIndex >= columns->Count || this->Parent == nullptr)
			return 0;

		// Calculate the x-position of the target column
		int targetX = ROW_LABEL_WIDTH;
		for (int i = 0; i < columnIndex; i++)
		{
			targetX += columns[i]->Width;
		}

		// Get the parent container's viewport width
		int viewportWidth = this->Parent->ClientSize.Width;

		// Calculate the scroll position that will center the column
		int columnWidth = columns[columnIndex]->Width;
		int scrollPosition = targetX - ((viewportWidth - columnWidth) / 2);

		// Ensure scroll position stays within valid bounds
		scrollPosition = Math::Max(0, scrollPosition);
		scrollPosition = Math::Min(scrollPosition, this->Width - viewportWidth);

		return scrollPosition;
	}

	int Widget_Draw_Table::FindCenteredMeasureColumn(int viewportCenterX)
	{
		int currentX = ROW_LABEL_WIDTH;
		int closestColumnIndex = -1;
		int minDistance = INT_MAX;

		// Iterate through columns to find the measure column closest to the center
		for (int i = 0; i < columns->Count; i++)
		{
			if (columns[i]->IsFirstInMeasure)
			{
				// Calculate the center of this column
				int columnCenterX = currentX + (columns[i]->Width / 2);
				int distance = Math::Abs(columnCenterX - viewportCenterX);

				if (distance < minDistance)
				{
					minDistance = distance;
					closestColumnIndex = i;
				}
			}

			currentX += columns[i]->Width;
		}

		return closestColumnIndex;
	}

	int Widget_Draw_Table::CalculateScrollPositionForCenteredColumn(int columnIndex)
	{
		if (columnIndex < 0 || columnIndex >= columns->Count || this->Parent == nullptr)
			return 0;

		// Calculate the x-position of the target column
		int targetX = ROW_LABEL_WIDTH;
		for (int i = 0; i < columnIndex; i++)
		{
			targetX += columns[i]->Width;
		}

		// Get the parent container's viewport width
		int viewportWidth = this->Parent->ClientSize.Width;

		// Calculate the scroll position that will center the column
		int columnWidth = columns[columnIndex]->Width;
		int scrollPosition = targetX - ((viewportWidth - columnWidth) / 2);

		// Ensure scroll position stays within valid bounds
		scrollPosition = Math::Max(0, scrollPosition);
		scrollPosition = Math::Min(scrollPosition, this->Width - viewportWidth);

		return scrollPosition;
	}

	void Widget_Draw_Table::OnParentResize(Object^ sender, EventArgs^ e)
	{
		if (this->Parent != nullptr)
		{
			this->Height = this->Parent->ClientSize.Height;
			RecalculateWidthAndLayout();
		}
	}

	void Widget_Draw_Table::OnMouseMove(Object^ sender, MouseEventArgs^ e)
	{
		UpdateCursor(e->Location);

		// Only continue dragging if left mouse button is pressed
		if (isDraggingBars && draggedBars != nullptr &&
			(e->Button & System::Windows::Forms::MouseButtons::Left) == System::Windows::Forms::MouseButtons::Left) {
			UpdateDraggedBars(e->Location);
			this->Invalidate();
		}
		else if ((e->Button & System::Windows::Forms::MouseButtons::Left) != System::Windows::Forms::MouseButtons::Left) {
			// If left button is released during drag, end the drag operation
			if (isDraggingBars) {
				EndDragging(e->Location);
			}
		}
		else if (currentMode == WidgetMode::Selection && isSelecting &&
			(e->Button & System::Windows::Forms::MouseButtons::Left) == System::Windows::Forms::MouseButtons::Left) {
			int x = Math::Min(selectionStart.X, e->X);
			int y = Math::Min(selectionStart.Y, e->Y);
			int width = Math::Abs(e->X - selectionStart.X);
			int height = Math::Abs(e->Y - selectionStart.Y);
			selectionRectangle = Rectangle(x, y, width, height);
			UpdateRGBBarSelection(e->Location, true);
			this->Invalidate();
		}
	}

	void Widget_Draw_Table::HandleMouseClick(MouseEventArgs^ e)
	{
		if (e->Button != System::Windows::Forms::MouseButtons::Left)
			return;

		int columnIndex = FindColumnIndex(e->X);
		int rowIndex = FindRowIndex(e->Y);

		switch (currentMode) {
		case WidgetMode::Draw:
			if (columnIndex >= 0 && rowIndex >= 0) {
				AddRGBBar(columnIndex, rowIndex);
			}
			break;

		case WidgetMode::Erase:
			EraseRGBBar(e->X, e->Y);
			break;
		}

		this->Invalidate();
	}

	void Widget_Draw_Table::OnContextMenuOpening(Object^ sender, CancelEventArgs^ e)
	{
		// Cancel the default context menu
		e->Cancel = true;

		// Handle right-click as paste operation if we have copied bars
		if (clipboardBars != nullptr && clipboardBars->Count > 0)
		{
			Point mousePos = this->PointToClient(Control::MousePosition);
			int columnIndex = FindColumnIndex(mousePos.X);
			int rowIndex = FindRowIndex(mousePos.Y);

			if (columnIndex >= 0) {
				PasteFromClipboard(columnIndex, rowIndex);
				this->Invalidate();
			}
		}
	}

	void Widget_Draw_Table::RecalculateWidthAndLayout()
	{
		if (this->Parent == nullptr || rows == nullptr)
			return;

		// Calculate row heights and positions with fixed spacing
		int availableHeight = this->Height - TITLE_HEIGHT;
		int rowHeight = 30; // Fixed row height

		int currentY = TITLE_HEIGHT;
		for (int i = 0; i < rows->Count; i++)
		{
			RowInfo^ row = rows[i];
			row->Height = rowHeight;
			row->TopOffset = currentY;
			currentY += rowHeight + ROW_GAP;
		}

		// Calculate total ticks and convert to pixels
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		int totalTicks = 0;

		for each (ColumnInfo ^ col in columns)
		{
			totalTicks += col->LengthInTicks;
		}

		int totalWidth = ROW_LABEL_WIDTH + static_cast<int>(Math::Ceiling(totalTicks * pixelsPerTick));
		this->Width = Math::Max(totalWidth, ROW_LABEL_WIDTH + MinimumSize.Width);

		// Notify parent of size change to trigger scrollbar update
		OnSizeChanged(gcnew EventArgs());

		// Update RGB bar bounds
		for each (RGBBarInfo ^ bar in rgbBars)
		{
			UpdateRGBBarBounds(bar);
		}
	}

	void Widget_Draw_Table::ApplyQuantization(NoteQuantization newQuantization)
	{
		if (columns->Count == 0) return;

		List<ColumnInfo^>^ newColumns = gcnew List<ColumnInfo^>();
		int currentTick = 0;
		int measureIndex = 1;
		int rowCount = rows->Count;

		for each (ColumnInfo ^ originalCol in columns) {
			if (!originalCol->IsFirstInMeasure) continue;

			int measureTicks = CalculateMeasureTicks(
				originalCol->TimeSignatureNumerator,
				originalCol->TimeSignatureDenominator
			);

			int subdivisions = CalculateSubdivisions(
				originalCol->TimeSignatureNumerator,
				originalCol->TimeSignatureDenominator,
				newQuantization
			);

			int ticksPerSubdivision = measureTicks / subdivisions;

			for (int i = 0; i < subdivisions; i++) {
				ColumnInfo^ newCol = gcnew ColumnInfo();
				newCol->HeaderText = gcnew ColumnHeaderText();

				if (i == 0) {
					newCol->HeaderText->Line1 = originalCol->HeaderText->Line1;
					newCol->HeaderText->Line2 = measureIndex.ToString();
					newCol->HeaderText->Line3 = FormatTimeSignature(
						originalCol->TimeSignatureNumerator,
						originalCol->TimeSignatureDenominator
					);
					newCol->IsFirstInMeasure = true;
				}
				else {
					newCol->HeaderText->Line1 = "";
					newCol->HeaderText->Line2 = "";
					newCol->HeaderText->Line3 = "";
					newCol->IsFirstInMeasure = false;
				}

				newCol->StartTick = currentTick + (i * ticksPerSubdivision);
				newCol->LengthInTicks = ticksPerSubdivision;
				newCol->TimeSignatureNumerator = originalCol->TimeSignatureNumerator;
				newCol->TimeSignatureDenominator = originalCol->TimeSignatureDenominator;
				newCol->MeasureIndex = measureIndex;
				newCol->SubdivisionIndex = i;

				// Create cells array only if we have rows
				if (rowCount > 0) {
					newCol->Cells = gcnew array<CellInfo^>(rowCount);
					for (int j = 0; j < rowCount; j++) {
						newCol->Cells[j] = gcnew CellInfo();
						newCol->Cells[j]->IsSelected = false;
					}
				}
				else {
					newCol->Cells = gcnew array<CellInfo^>(0);
				}

				newColumns->Add(newCol);
			}

			currentTick += measureTicks;
			measureIndex++;
		}

		columns = newColumns;
		currentQuantization = newQuantization;
		RecalculateWidthAndLayout();
	}


	int Widget_Draw_Table::FindColumnIndex(int x)
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

	int Widget_Draw_Table::FindRowIndex(int y)
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

	void Widget_Draw_Table::InitializeCursors()
	{
		this->penCursor		= CreateCursorFromResource(L"Cursor_Pen_32");
		this->eraseCursor	= CreateCursorFromResource(L"Cursor_Eraser_32");
		this->selectCursor	= CreateCursorFromResource(L"Cursor_Select_32");
		this->fadeCursor	= CreateCursorFromResource(L"Cursor_Fade_32");
		this->lengthCursor	= CreateCursorFromResource(L"Cursor_Length_32");
		this->colorCursor	= CreateCursorFromResource(L"Cursor_Color_32");
		this->bucketCursor	= CreateCursorFromResource(L"Cursor_Bucket_32");
	}

	void Widget_Draw_Table::UpdateCursor(Point mousePosition)
	{
		bool isInTableArea = mousePosition.X > ROW_LABEL_WIDTH &&
			mousePosition.Y > TITLE_HEIGHT &&
			mousePosition.X < this->Width &&
			mousePosition.Y < this->Height;

		switch (currentMode)
		{
		case WidgetMode::Draw:		this->Cursor = isInTableArea ? this->penCursor		: Cursors::Default;	break;
		case WidgetMode::Erase:		this->Cursor = isInTableArea ? this->eraseCursor	: Cursors::Default;	break;
		case WidgetMode::Selection:	this->Cursor = isInTableArea ? this->selectCursor	: Cursors::Default;	break;
		case WidgetMode::Fade:		this->Cursor = isInTableArea ? this->fadeCursor		: Cursors::Default;	break;
		case WidgetMode::Length:	this->Cursor = isInTableArea ? this->lengthCursor	: Cursors::Default;	break;
		case WidgetMode::Color:		this->Cursor = isInTableArea ? this->colorCursor	: Cursors::Default;	break;
		case WidgetMode::Bucket:	this->Cursor = isInTableArea ? this->bucketCursor	: Cursors::Default;	break;

		default:
			this->Cursor = Cursors::Default;
			break;
		}
	}

	double Widget_Draw_Table::GetNearestAllowedScale(double requestedScale)
	{
		// Handle scale factors less than 1.0
		if (requestedScale < 1.0)
		{
			const array<double>^ allowedSmallScales = { 0.1, 0.25, 0.5, 0.75 };

			// Find the closest allowed small scale
			double closestScale = allowedSmallScales[0];
			double minDiff = Math::Abs(requestedScale - closestScale);

			for (int i = 1; i < allowedSmallScales->Length; i++)
			{
				double diff = Math::Abs(requestedScale - allowedSmallScales[i]);
				if (diff < minDiff)
				{
					minDiff = diff;
					closestScale = allowedSmallScales[i];
				}
			}

			return closestScale;
		}

		// Handle scale factors >= 1.0
		// Round to nearest integer and clamp to maximum
		int roundedScale = static_cast<int>(Math::Round(requestedScale));
		return Math::Min(roundedScale, static_cast<int>(MAX_SCALE_FACTOR));
	}

	double Widget_Draw_Table::CalculateTotalVirtualWidth()
	{
		double totalWidth = ROW_LABEL_WIDTH;

		// Calculate width based on music data
		for each (ColumnInfo ^ col in columns)
		{
			double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
			double columnWidth = col->LengthInTicks * pixelsPerTick;
			totalWidth += columnWidth;
		}

		return Math::Max(totalWidth, static_cast<double>(this->MinimumSize.Width));
	}

	Rectangle Widget_Draw_Table::GetVisibleBounds()
	{
		Rectangle clientRect = this->ClientRectangle;

		if (this->Parent != nullptr)
		{
			return Rectangle(
				_CurrentScrollPosition.X,
				0,
				this->Parent->ClientSize.Width,
				this->Parent->ClientSize.Height
			);
		}

		return clientRect;
	}

	void Widget_Draw_Table::ClearAllSelections()
	{
		for each (ColumnInfo ^ col in columns)
		{
			for each (CellInfo ^ cell in col->Cells)
			{
				cell->IsSelected = false;
			}
		}
	}

	void Widget_Draw_Table::UpdateCellSelection(MouseEventArgs^ e)
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

	void Widget_Draw_Table::RenderRowLabels(Graphics^ g)
	{
		if (rows == nullptr || rows->Count == 0)
			return;

		Rectangle visible = GetVisibleBounds();

		StringFormat^ format = gcnew StringFormat();
		format->Alignment = StringAlignment::Near;
		format->LineAlignment = StringAlignment::Center;

		SolidBrush^ brush = gcnew SolidBrush(Color::Black);

		// Calculate visible rows
		int startRow = Math::Max(0, (visible.Y - TITLE_HEIGHT) / (rows[0]->Height + ROW_GAP));
		int endRow = Math::Min(rows->Count,
			(visible.Y + visible.Height - TITLE_HEIGHT) / (rows[0]->Height + ROW_GAP) + 1);

		// Draw labels for visible rows
		for (int i = startRow; i < endRow; i++)
		{
			RowInfo^ row = rows[i];
			if (!String::IsNullOrEmpty(row->Label))
			{
				Rectangle labelBounds = Rectangle(
					0,
					row->TopOffset,
					ROW_LABEL_WIDTH - 5,
					row->Height
				);

				g->DrawString(row->Label, rowLabelFont, brush, labelBounds, format);
			}
		}

		// Draw vertical separator
		Pen^ separatorPen = gcnew Pen(Color::Black, 1);
		g->DrawLine(separatorPen,
			ROW_LABEL_WIDTH, visible.Y,
			ROW_LABEL_WIDTH, visible.Y + visible.Height);
	}

	void Widget_Draw_Table::RenderCells(Graphics^ g)
	{
		if (rows->Count == 0) return;

		int textLine1Bottom = static_cast<int>(TITLE_HEIGHT / 2 - (Font->Height * 1.5f + HEADER_LINE_GAP) + Font->Height);

		for each (ColumnInfo ^ col in columns)
		{
			if (col->Cells == nullptr) continue;

			for (int rowIndex = 0; rowIndex < rows->Count && rowIndex < col->Cells->Length; rowIndex++)
			{
				CellInfo^ cell = col->Cells[rowIndex];
				if (cell == nullptr) continue;

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

				// Draw cell selection
				if (cell->IsSelected && currentMode != WidgetMode::Draw)
				{
					Color selectionColor = Color::FromArgb(40, 0, 120, 215);
					SolidBrush^ selectionBrush = gcnew SolidBrush(selectionColor);
					g->FillRectangle(selectionBrush, cell->Bounds);

					Pen^ selectionPen = gcnew Pen(Color::FromArgb(0, 120, 215), 2);
					g->DrawRectangle(selectionPen, cell->Bounds);
				}
			}
		}
	}

	void Widget_Draw_Table::RenderColumnTitles(Graphics^ g)
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

	void Widget_Draw_Table::StartDragging(RGBBarInfo^ clickedBar, Point mousePoint)
	{
		isDraggingBars = true;
		dragStart = mousePoint;
		draggedBars = gcnew List<DraggedBarInfo^>();

		// Get all selected bars
		List<RGBBarInfo^>^ selectedBars = GetSelectedRGBBars();

		// If clicked bar isn't selected, clear selection and only drag clicked bar
		if (!clickedBar->IsSelected) {
			ClearRGBBarSelection();
			clickedBar->IsSelected = true;
			selectedBars->Clear();
			selectedBars->Add(clickedBar);
		}

		// Check if we're dragging from multiple rows
		List<int>^ uniqueRows = gcnew List<int>();

		for each (RGBBarInfo ^ bar in selectedBars) {
			if (!uniqueRows->Contains(bar->Row)) {
				uniqueRows->Add(bar->Row);
			}
		}

		isMultiRowDrag = uniqueRows->Count > 1;

		// Calculate tick offsets relative to clicked bar
		for each (RGBBarInfo ^ bar in selectedBars) {
			DraggedBarInfo^ dragInfo = gcnew DraggedBarInfo(bar);
			dragInfo->TickOffset = bar->StartTick - clickedBar->StartTick;
			draggedBars->Add(dragInfo);
		}

		// Calculate mouse offset based on clicked bar
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		dragOffset = Point(
			mousePoint.X - (ROW_LABEL_WIDTH + static_cast<int>(clickedBar->StartTick * pixelsPerTick)),
			mousePoint.Y - (rows[clickedBar->Row]->TopOffset + (rows[clickedBar->Row]->Height / 2))
		);
	}

	void Widget_Draw_Table::UpdateDraggedBars(Point mousePoint)
	{
		if (draggedBars == nullptr || draggedBars->Count == 0) return;

		// Find base column under mouse position considering dragOffset
		int columnIndex = FindColumnIndex(mousePoint.X - dragOffset.X);

		// Calculate minimum allowed tick position for the current drag operation
		int minAllowedTick = 0;
		for each (DraggedBarInfo ^ dragInfo in draggedBars) {
			minAllowedTick = Math::Min(minAllowedTick, dragInfo->TickOffset);
		}
		minAllowedTick = Math::Abs(minAllowedTick);  // Convert to positive value

		// If mouse position would place any bar before the first column, adjust column index
		if (columnIndex < 0) {
			columnIndex = 0;  // Snap to first column
		}

		// Calculate base tick position
		ColumnInfo^ col = columns[columnIndex];
		int measureStartTick = 0;
		for (int i = 0; i <= columnIndex; i++) {
			if (columns[i]->IsFirstInMeasure) {
				measureStartTick = GetTicksAtColumn(i);
			}
		}

		int measureTicks = CalculateMeasureTicks(
			col->TimeSignatureNumerator,
			col->TimeSignatureDenominator);

		int ticksPerSubdivision = measureTicks /
			CalculateSubdivisions(
				col->TimeSignatureNumerator,
				col->TimeSignatureDenominator,
				currentQuantization);

		int baseTick = measureStartTick + (col->SubdivisionIndex * ticksPerSubdivision);

		// Ensure baseTick is at least minAllowedTick to prevent any bars going negative
		baseTick = Math::Max(baseTick, minAllowedTick);

		// Use first dragged bar as reference for row calculations
		DraggedBarInfo^ firstDragInfo = draggedBars[0];

		// Update all selected bars
		for each (DraggedBarInfo ^ dragInfo in draggedBars) {
			// Update tick position relative to base position
			int newTick = baseTick + dragInfo->TickOffset;

			// Ensure no bar goes before the first column
			dragInfo->Bar->StartTick = Math::Max(0, newTick);

			// Update row position only if not in multi-row mode
			if (!isMultiRowDrag) {
				int newRow = FindRowIndex(mousePoint.Y);
				if (newRow >= 0 && newRow < rows->Count) {
					int rowOffset = dragInfo->OriginalRow - firstDragInfo->OriginalRow;
					int targetRow = newRow + rowOffset;
					if (targetRow >= 0 && targetRow < rows->Count) {
						dragInfo->Bar->Row = targetRow;
					}
				}
			}

			UpdateRGBBarBounds(dragInfo->Bar);
		}
	}

	void Widget_Draw_Table::EndDragging(Point mousePoint)
	{
		if (draggedBars != nullptr) {
			// Check if the final position is valid
			int columnIndex = FindColumnIndex(mousePoint.X);
			if (columnIndex < 0) {
				// If dropped outside valid columns, restore all original positions
				for each (DraggedBarInfo ^ dragInfo in draggedBars) {
					dragInfo->Bar->Row = dragInfo->OriginalRow;
					dragInfo->Bar->StartTick = dragInfo->OriginalStartTick;
					UpdateRGBBarBounds(dragInfo->Bar);
				}
			}
		}

		isDraggingBars = false;
		draggedBars = nullptr;
		isMultiRowDrag = false;
	}

	Widget_Draw_Table::RGBBarInfo^ Widget_Draw_Table::GetBarAtPoint(Point point)
	{
		for (int i = rgbBars->Count - 1; i >= 0; i--) {
			RGBBarInfo^ bar = rgbBars[i];
			if (bar->Bounds.Contains(point)) {
				return bar;
			}
		}
		return nullptr;
	}
	int Widget_Draw_Table::GetTickAtPoint(int x)
	{
		double pixelsPerTick = 1.0 / PIXELS_PER_TICK_DIVISOR * horizontalScaleFactor;
		int approximateTick = static_cast<int>((x - ROW_LABEL_WIDTH) / pixelsPerTick);
		return approximateTick >= 0 ? approximateTick : 0;
	}

	void Widget_Draw_Table::CopySelectedBars()
	{
		List<RGBBarInfo^>^ selectedBars = GetSelectedRGBBars();
		if (selectedBars->Count == 0) return;

		// Clear previous clipboard content
		clipboardBars = gcnew List<RGBBarInfo^>();

		// Find the leftmost bar's tick to use as reference
		int minTick = Int32::MaxValue;
		for each (RGBBarInfo ^ bar in selectedBars) {
			minTick = Math::Min(minTick, bar->StartTick);
		}

		// Copy all selected bars
		for each (RGBBarInfo ^ originalBar in selectedBars) {
			RGBBarInfo^ copiedBar = gcnew RGBBarInfo();
			copiedBar->BarColor = originalBar->BarColor;
			copiedBar->Row = originalBar->Row;
			copiedBar->LengthInTicks = originalBar->LengthInTicks;
			copiedBar->StartTick = originalBar->StartTick - minTick; // Store relative position
			clipboardBars->Add(copiedBar);
		}
	}

	void Widget_Draw_Table::PasteFromClipboard(int columnIndex, int targetRow)
	{
		if (clipboardBars == nullptr || clipboardBars->Count == 0) return;

		// Calculate base tick position for paste
		ColumnInfo^ col = columns[columnIndex];
		int measureStartTick = 0;
		for (int i = 0; i <= columnIndex; i++) {
			if (columns[i]->IsFirstInMeasure) {
				measureStartTick = GetTicksAtColumn(i);
			}
		}

		int measureTicks = CalculateMeasureTicks(
			col->TimeSignatureNumerator,
			col->TimeSignatureDenominator);

		int ticksPerSubdivision = measureTicks /
			CalculateSubdivisions(
				col->TimeSignatureNumerator,
				col->TimeSignatureDenominator,
				currentQuantization);

		int baseTick = measureStartTick + (col->SubdivisionIndex * ticksPerSubdivision);

		// For single bar copy, paste to clicked position
		if (clipboardBars->Count == 1) {
			RGBBarInfo^ newBar = gcnew RGBBarInfo();
			RGBBarInfo^ clipBar = clipboardBars[0];

			newBar->BarColor = clipBar->BarColor;
			newBar->LengthInTicks = clipBar->LengthInTicks;
			newBar->StartTick = baseTick;
			newBar->Row = targetRow >= 0 ? targetRow : clipBar->Row;

			UpdateRGBBarBounds(newBar);
			rgbBars->Add(newBar);
		}
		// For multiple bars, maintain relative positions and original rows
		else {
			// First, check if any bars would end up before the first column
			int minOffset = 0;
			for each (RGBBarInfo ^ clipBar in clipboardBars) {
				minOffset = Math::Min(minOffset, clipBar->StartTick);
			}

			// Adjust base tick if necessary to prevent negative positions
			baseTick = Math::Max(baseTick, Math::Abs(minOffset));

			// Create and add new bars
			for each (RGBBarInfo ^ clipBar in clipboardBars) {
				RGBBarInfo^ newBar = gcnew RGBBarInfo();
				newBar->BarColor = clipBar->BarColor;
				newBar->LengthInTicks = clipBar->LengthInTicks;
				newBar->StartTick = baseTick + clipBar->StartTick;
				newBar->Row = clipBar->Row;

				UpdateRGBBarBounds(newBar);
				rgbBars->Add(newBar);
			}
		}
	}

	void Widget_Draw_Table::AddContextMenu()
	{
		System::Windows::Forms::ContextMenuStrip^ menu = gcnew System::Windows::Forms::ContextMenuStrip();
		menu->Opening += gcnew CancelEventHandler(this, &Widget_Draw_Table::OnContextMenuOpening);
		this->ContextMenuStrip = menu;
	}
}
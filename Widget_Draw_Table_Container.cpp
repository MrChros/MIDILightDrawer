#include "Widget_Draw_Table_Container.h"

namespace MIDILightDrawer {

	Widget_Draw_Table_Container::Widget_Draw_Table_Container()
	{
		SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::ResizeRedraw, true);
		
		this->AutoScroll				= false;
		this->HorizontalScroll->Enabled = false;
		this->VerticalScroll->Enabled	= false;

		// Create table layout
		_Table_Layout = gcnew TableLayoutPanel();
		_Table_Layout->RowCount = 2;
		_Table_Layout->ColumnCount = 1;
		_Table_Layout->Dock = DockStyle::Fill;
		_Table_Layout->Margin = System::Windows::Forms::Padding(0);

		// Configure rows
		_Table_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		_Table_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 17)); // SCROLLBAR_HEIGHT

		// Configure column
		_Table_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));

		// Create and configure scrollbar
		_ScrollBar = gcnew Control_ScrollBar();
		_ScrollBar->Dock = DockStyle::Fill;
		_ScrollBar->ScrollPositionChanged += gcnew EventHandler<int>(this, &Widget_Draw_Table_Container::OnScrollPositionChanged);

		// Add scrollbar to table
		_Table_Layout->Controls->Add(_ScrollBar, 0, 1);

		this->Controls->Add(_Table_Layout);
	}

	void Widget_Draw_Table_Container::OnScrollPositionChanged(Object^ sender, int newPosition)
	{
		if (_Draw_Table != nullptr)
		{
			// Convert physical scroll position to virtual position
			double virtualPosition = _ScrollBar->PhysicalToVirtual(newPosition);

			// Update table position
			_Draw_Table->Location = Point(static_cast<int>(-virtualPosition), 0);
			_Draw_Table->UpdateScrollPosition(Point(static_cast<int>(virtualPosition), 0));
		}
	}

	void Widget_Draw_Table_Container::OnDrawTableSizeChanged(Object^ sender, EventArgs^ e)
	{
		if (_Draw_Table == nullptr) return;

		// Update virtual dimensions
		double virtualWidth = _Draw_Table->TotalVirtualWidth;
		double viewportWidth = static_cast<double>(this->ClientSize.Width);

		// Update scrollbar
		_ScrollBar->VirtualTotalWidth = virtualWidth;
		_ScrollBar->VirtualViewportWidth = viewportWidth;

		// Use fixed range for physical scrollbar
		const int PHYSICAL_RANGE = 10000;
		_ScrollBar->TotalWidth = PHYSICAL_RANGE;
		_ScrollBar->ViewportWidth = static_cast<int>((viewportWidth / virtualWidth) * PHYSICAL_RANGE);

		// Maintain scroll position ratio when content changes
		if (_ScrollBar->VirtualTotalWidth > _ScrollBar->VirtualViewportWidth)
		{
			double currentRatio = _ScrollBar->VirtualScrollPosition /
				(_ScrollBar->VirtualTotalWidth - _ScrollBar->VirtualViewportWidth);

			double newMaxScroll = virtualWidth - viewportWidth;
			_ScrollBar->VirtualScrollPosition = currentRatio * newMaxScroll;
		}

		_ScrollBar->SetEnabledState(virtualWidth > viewportWidth);
	}

	void Widget_Draw_Table_Container::OnSizeChanged(System::EventArgs^ e)
	{
		System::Windows::Forms::Panel::OnSizeChanged(e);

		if (_Draw_Table != nullptr) {
			_Draw_Table->Height = this->ClientSize.Height - 17;	// SCROLLBAR_HEIGHT;

			// Update scrollbar viewport
			_ScrollBar->ViewportWidth = this->ClientSize.Width;
			_ScrollBar->SetEnabledState(_Draw_Table->Width > this->ClientSize.Width);
		}
	}
	
	int Widget_Draw_Table_Container::FindCenteredMeasureColumn()
	{
		if (_Draw_Table == nullptr) return -1;

		// Calculate the viewport center considering scroll position
		int scrollX = this->HorizontalScroll->Value;
		int viewportWidth = this->ClientSize.Width;
		int viewportCenter = scrollX + (viewportWidth / 2);

		int currentX = _Draw_Table->GetRowLabelWidth();
		int lastMeasureIndex = -1;
		int lastMeasureX = currentX;

		// Find which measure contains or is closest to the viewport center
		for (int i = 0; i < _Draw_Table->GetColumnCount(); i++) {
			if (_Draw_Table->IsColumnFirstInMeasure(i)) {
				// If we've gone past the viewport center
				if (currentX > viewportCenter) {
					// Return the closer measure between current and last measure
					if (lastMeasureIndex >= 0) {
						int distToPrevious = Math::Abs(viewportCenter - lastMeasureX);
						int distToCurrent = Math::Abs(viewportCenter - currentX);
						return (distToPrevious < distToCurrent) ? lastMeasureIndex : i;
					}
					return i;
				}
				lastMeasureIndex = i;
				lastMeasureX = currentX;
			}
			currentX += _Draw_Table->GetColumnWidth(i);
		}

		// If we got here, return the last measure we found
		return lastMeasureIndex;
	}

	int Widget_Draw_Table_Container::GetMeasureNumberAtCenter()
	{
		if (_Draw_Table == nullptr) return 1;

		// Calculate the center point of the viewport
		int viewportCenter = this->HorizontalScroll->Value + (this->ClientSize.Width / 2);
		Console::WriteLine("Viewport Center: {0}", viewportCenter);

		// Find which measure this point is in
		int currentX = _Draw_Table->GetRowLabelWidth();
		int currentMeasure = 1;

		Console::WriteLine("Starting search at X: {0}", currentX);

		for (int i = 0; i < _Draw_Table->GetColumnCount(); i++) {
			if (_Draw_Table->IsColumnFirstInMeasure(i)) {
				int columnWidth = _Draw_Table->GetColumnWidth(i);
				Console::WriteLine("Measure {0} at X: {1}, Width: {2}",
					currentMeasure, currentX, columnWidth);

				if (currentX > viewportCenter) {
					Console::WriteLine("Found measure: {0} (previous)", currentMeasure - 1);
					return currentMeasure - 1;
				}
				currentMeasure++;
			}
			currentX += _Draw_Table->GetColumnWidth(i);
		}

		Console::WriteLine("Returning last measure: {0}", currentMeasure - 1);
		return currentMeasure - 1;
	}

	void Widget_Draw_Table_Container::CenterOnMeasure(int measureNumber)
	{
		if (_Draw_Table == nullptr) return;

		_Last_Centered_Measure = measureNumber;
		Console::WriteLine("Storing centered measure: {0}", measureNumber);

		// Find the column that starts the requested measure
		int columnIndex = -1;
		int currentMeasure = 1;

		for (int i = 0; i < _Draw_Table->GetColumnCount(); i++) {
			if (_Draw_Table->IsColumnFirstInMeasure(i)) {
				if (currentMeasure == measureNumber) {
					columnIndex = i;
					break;
				}
				currentMeasure++;
			}
		}

		if (columnIndex >= 0) {
			// Calculate position to center this column
			int targetX = _Draw_Table->GetRowLabelWidth();
			for (int i = 0; i < columnIndex; i++) {
				targetX += _Draw_Table->GetColumnWidth(i);
			}

			int columnWidth = _Draw_Table->GetColumnWidth(columnIndex);
			int newScrollPos = targetX - ((this->ClientSize.Width - columnWidth) / 2);

			// Update scroll position through the scrollbar property
			_ScrollBar->ScrollPosition = newScrollPos;
			_Last_Scroll_Position = Point(newScrollPos, 0);
		}
	}

	void Widget_Draw_Table_Container::SetDrawTable(Widget_Draw_Table^ table)
	{
		if (this->_Draw_Table != nullptr) {
			_Draw_Table->SizeChanged -= gcnew EventHandler(this, &Widget_Draw_Table_Container::OnDrawTableSizeChanged);
			_Table_Layout->Controls->Remove(this->_Draw_Table);
		}

		this->_Draw_Table = table;
		if (table != nullptr) {
			table->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Top);
			table->Location = Point(0, 0);
			table->Margin = System::Windows::Forms::Padding(0);
			_Table_Layout->Controls->Add(table, 0, 0);

			// Subscribe to size changes
			table->SizeChanged += gcnew EventHandler(this, &Widget_Draw_Table_Container::OnDrawTableSizeChanged);

			// Initial update of scrollbar
			_ScrollBar->TotalWidth = table->Width;
			_ScrollBar->ViewportWidth = this->ClientSize.Width;
			_ScrollBar->SetEnabledState(table->Width > this->ClientSize.Width);
		}
	}

	void Widget_Draw_Table_Container::SetScale(double newScale)
	{
		if (_Draw_Table == nullptr) return;

		int measureToCenter = _Last_Centered_Measure;
		_Draw_Table->SetHorizontalScale(newScale);

		// Update scroll bar
		_ScrollBar->TotalWidth = _Draw_Table->Width;
		_ScrollBar->ViewportWidth = this->ClientSize.Width - SystemInformation::VerticalScrollBarWidth;

		CenterOnMeasure(measureToCenter);
	}

	void Widget_Draw_Table_Container::UpdateLastCenteredMeasure()
	{
		if (_Draw_Table == nullptr) return;
		_Last_Centered_Measure = GetMeasureNumberAtCenter();
	}

	void Widget_Draw_Table_Container::UpdateVirtualScrollRange()
	{
		if (_Draw_Table == nullptr) return;

		// Get actual dimensions
		int totalWidth = _Draw_Table->Width;
		int viewportWidth = this->ClientSize.Width;

		Console::WriteLine("UpdateVirtualScrollRange: Total={0}, Viewport={1}", totalWidth, viewportWidth);

		// Update scrollbar dimensions
		_ScrollBar->TotalWidth = totalWidth;
		_ScrollBar->ViewportWidth = viewportWidth;
		_ScrollBar->SetEnabledState(totalWidth > viewportWidth);
	}
}
#include "Control_CheckedListBox.h"

namespace MIDILightDrawer
{
	Control_CheckedListBox::Control_CheckedListBox()
	{
		Initialize();
	}

	Control_CheckedListBox::~Control_CheckedListBox()
	{
		// Clean up resources
		if (_CheckedIcon != nullptr)
		{
			delete _CheckedIcon;
			_CheckedIcon = nullptr;
		}

		if (_UncheckedIcon != nullptr)
		{
			delete _UncheckedIcon;
			_UncheckedIcon = nullptr;
		}

		// Clear items
		_Items->Clear();
	}

	void Control_CheckedListBox::Initialize()
	{
		// Double buffering for smooth rendering
		this->DoubleBuffered = true;
		this->SetStyle(
			ControlStyles::UserPaint |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::OptimizedDoubleBuffer, true);

		this->BackColor = Color::Transparent;

		// Initialize item list
		_Items = gcnew List<CheckedListItem^>();
		_ItemHeight		= 36;  // Default item height
		_SelectedIndex	= -1;
		_HoverIndex		= -1;
		_ScrollOffset	= 0;
		_IsScrolling	= false;
		_ShowItemCount	= false;

		// Load checkbox icons
		try
		{
			// Create icons programmatically
			_CheckedIcon = CreateCheckIcon(true);
			_UncheckedIcon = CreateCheckIcon(false);
		}
		catch (Exception^ ex)
		{
			System::Diagnostics::Debug::WriteLine("Error loading checkbox icons: " + ex->Message);
		}

		// Set default size
		this->Size = System::Drawing::Size(200, 200);
	}

	Image^ Control_CheckedListBox::CreateCheckIcon(bool isChecked)
	{
		// Create a bitmap for the checkbox icon
		Bitmap^ IconBitmap = gcnew Bitmap(16, 16);
		Graphics^ G = Graphics::FromImage(IconBitmap);

		// Get theme colors
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();
		Color BackgroundColor = Theme->BackgroundLight;
		Color BorderColor = Theme->BorderStrong;
		Color CheckColor = Theme->AccentPrimary;

		// Enable anti-aliasing
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Draw checkbox background and border
		G->Clear(Color::Transparent);
		G->FillRectangle(gcnew SolidBrush(BackgroundColor), 1, 1, 14, 14);
		G->DrawRectangle(gcnew Pen(BorderColor), 1, 1, 13, 13);

		// If checked, draw the checkmark
		if (isChecked)
		{
			array<Point>^ checkPoints =
			{
				Point(3, 8),
				Point(6, 11),
				Point(12, 4)
			};

			G->DrawLines(gcnew Pen(CheckColor, 2), checkPoints);
		}

		delete G;
		return IconBitmap;
	}

	void Control_CheckedListBox::AddItem(String^ text)
	{
		AddItem(text, false, nullptr);
	}

	void Control_CheckedListBox::AddItem(String^ text, bool checked)
	{
		AddItem(text, checked, nullptr);
	}

	void Control_CheckedListBox::AddItem(String^ text, bool checked, Object^ tag)
	{
		CheckedListItem^ item = gcnew CheckedListItem(text, checked, tag);
		AddItem(item);
	}

	void Control_CheckedListBox::AddItem(CheckedListItem^ item)
	{
		_Items->Add(item);
		UpdateScrollThumbBounds();
		this->Invalidate();
	}

	void Control_CheckedListBox::RemoveItem(int index)
	{
		if (index >= 0 && index < _Items->Count)
		{
			_Items->RemoveAt(index);

			// Update selection if necessary
			if (_SelectedIndex == index)
			{
				_SelectedIndex = -1;
			}
			else if (_SelectedIndex > index)
			{
				_SelectedIndex--;
			}

			UpdateScrollThumbBounds();
			this->Invalidate();
		}
	}

	void Control_CheckedListBox::ClearItems()
	{
		_Items->Clear();
		_SelectedIndex = -1;
		_ScrollOffset = 0;

		UpdateScrollThumbBounds();
		this->Invalidate();
	}

	void Control_CheckedListBox::SetItemChecked(int index, bool checked)
	{
		if (index >= 0 && index < _Items->Count)
		{
			bool oldValue = _Items[index]->Checked;
			_Items[index]->Checked = checked;

			// Raise event if the checked state changed
			if (oldValue != checked)
			{
				ItemCheckedChanged(this, gcnew CheckedListItemEventArgs(index, checked, _Items[index]));
			}

			this->Invalidate();
		}
	}

	bool Control_CheckedListBox::GetItemChecked(int index)
	{
		if (index >= 0 && index < _Items->Count)
		{
			return _Items[index]->Checked;
		}
		return false;
	}

	int Control_CheckedListBox::ItemCount()
	{
		return _Items->Count;
	}

	void Control_CheckedListBox::SelectAll()
	{
		for (int i = 0; i < _Items->Count; i++)
		{
			bool oldValue = _Items[i]->Checked;
			_Items[i]->Checked = true;

			// Raise event if the checked state changed
			if (oldValue != true)
			{
				ItemCheckedChanged(this, gcnew CheckedListItemEventArgs(i, true, _Items[i]));
			}
		}

		this->Invalidate();
	}

	void Control_CheckedListBox::SelectNone()
	{
		for (int i = 0; i < _Items->Count; i++)
		{
			bool oldValue = _Items[i]->Checked;
			_Items[i]->Checked = false;

			// Raise event if the checked state changed
			if (oldValue != false)
			{
				ItemCheckedChanged(this, gcnew CheckedListItemEventArgs(i, false, _Items[i]));
			}
		}

		this->Invalidate();
	}

	void Control_CheckedListBox::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Render the control
		RenderBackground(G);
		RenderItems(G);
		RenderScrollBar(G);
	}

	void Control_CheckedListBox::RenderBackground(Graphics^ g)
	{
		// No background at all - transparent
	}

	void Control_CheckedListBox::RenderItems(Graphics^ g)
	{
		if (_Items->Count == 0)
		{
			// No items to display
			Theme_Manager^ Theme = Theme_Manager::Get_Instance();
			String^ EmptyText = "No items";

			// Measure text for centering
			Drawing::Font^ Font = this->Font;
			SizeF TextSize = g->MeasureString(EmptyText, Font);

			float X = (this->Width - TextSize.Width) / 2;
			float Y = (this->Height - TextSize.Height) / 2;

			// Draw text centered
			g->DrawString(EmptyText, Font, gcnew SolidBrush(Color::FromArgb(150, Theme->ForegroundText)), X, Y);
			return;
		}

		// Get theme for colors
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();
		Color SelectedColor = Theme->AccentPrimary;
		Color HoverColor = Color::FromArgb(50, Theme->AccentPrimary);
		Color AlternateRowColor = Theme->BackgroundAlt;
		Color TextColor = Theme->ForegroundText;
		Color DisabledTextColor = Color::FromArgb(150, Theme->ForegroundText);

		// Font for rendering text
		Drawing::Font^ NormalFont = this->Font;
		Drawing::Font^ SubtitleFont = gcnew Drawing::Font(this->Font->FontFamily, this->Font->Size - 1);

		// Calculate which items are visible
		int FirstVisibleIndex = Math::Max(0, _ScrollOffset / _ItemHeight);
		int LastVisibleIndex = Math::Min(_Items->Count - 1, FirstVisibleIndex + (this->Height / _ItemHeight) + 1);

		// Render visible items
		for (int i = FirstVisibleIndex; i <= LastVisibleIndex; i++)
		{
			Rectangle ItemBounds = GetItemBounds(i);

			// Skip if item is not visible
			if (ItemBounds.Bottom < 0 || ItemBounds.Top > this->Height) {
				continue;
			}

			CheckedListItem^ Item = _Items[i];

			// Draw alternating row background
			if (i % 2 == 1)
			{
				g->FillRectangle(gcnew SolidBrush(AlternateRowColor), ItemBounds);
			}

			// Draw selection background if selected
			if (i == _SelectedIndex)
			{
				g->FillRectangle(gcnew SolidBrush(Color::FromArgb(100, SelectedColor)), ItemBounds);
			}
			// Draw hover effect
			else if (i == _HoverIndex)
			{
				g->FillRectangle(gcnew SolidBrush(HoverColor), ItemBounds);
			}

			// Draw checkbox
			Rectangle CheckboxBounds = GetCheckBoxBounds(i);
			g->DrawImage(Item->Checked ? _CheckedIcon : _UncheckedIcon, CheckboxBounds);

			// Calculate text bounds
			Rectangle TextBounds = Rectangle(
				CheckboxBounds.Right + 8,
				ItemBounds.Y + 4,
				ItemBounds.Width - CheckboxBounds.Width - 24,
				ItemBounds.Height);

			// Draw item text
			g->DrawString(Item->Text,
				NormalFont,
				gcnew SolidBrush(TextColor),
				TextBounds,
				StringFormat::GenericTypographic);

			// Draw subtitle text if available
			if (!String::IsNullOrEmpty(Item->Subtitle))
			{
				Rectangle subtitleBounds = Rectangle(
					TextBounds.X,
					TextBounds.Y + (int)g->MeasureString(Item->Text, NormalFont).Height,
					TextBounds.Width,
					TextBounds.Height / 2);

				g->DrawString(Item->Subtitle,
					SubtitleFont,
					gcnew SolidBrush(DisabledTextColor),
					subtitleBounds,
					StringFormat::GenericTypographic);
			}
		}

		// Draw item count if enabled
		if (_ShowItemCount && _Items->Count > 0)
		{
			int CheckedCount = CheckedItems->Count;
			String^ CountText = String::Format("{0} of {1} selected", CheckedCount, _Items->Count);

			SizeF TextSize = g->MeasureString(CountText, SubtitleFont);
			float X = this->Width - TextSize.Width - 25; // Account for scrollbar
			float Y = this->Height - TextSize.Height - 5;

			// Draw with shadow for better visibility
			g->DrawString(CountText, SubtitleFont, gcnew SolidBrush(Color::FromArgb(50, Color::Black)), X + 1, Y + 1);
			g->DrawString(CountText, SubtitleFont, gcnew SolidBrush(DisabledTextColor), X, Y);
		}

		// Clean up fonts
		delete SubtitleFont;
	}

	void Control_CheckedListBox::RenderScrollBar(Graphics^ g)
	{
		// Only show scrollbar if needed
		if (_Items->Count * _ItemHeight <= this->Height) {
			return;
		}

		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		// Scrollbar track
		_ScrollBarBounds = Rectangle(this->Width - 16, 2, 14, this->Height - 4);
		g->FillRectangle(gcnew SolidBrush(Color::FromArgb(30, Theme->BorderStrong)), _ScrollBarBounds);

		// Calculate thumb size and position
		UpdateScrollThumbBounds();

		// Draw thumb
		Color ThumbColor = _IsScrolling ? Theme->AccentPrimary : Theme->BorderStrong;
		g->FillRectangle(gcnew SolidBrush(ThumbColor), _ScrollThumbBounds);

		// Draw thumb grip lines
		int ThumbMiddleY = _ScrollThumbBounds.Y + (_ScrollThumbBounds.Height / 2);
		Pen^ GripPen = gcnew Pen(Color::FromArgb(100, Theme->BackgroundLight), 1);

		g->DrawLine(GripPen, _ScrollThumbBounds.X + 4, ThumbMiddleY - 3, _ScrollThumbBounds.Right - 4, ThumbMiddleY - 3);
		g->DrawLine(GripPen, _ScrollThumbBounds.X + 4, ThumbMiddleY, _ScrollThumbBounds.Right - 4, ThumbMiddleY);
		g->DrawLine(GripPen, _ScrollThumbBounds.X + 4, ThumbMiddleY + 3, _ScrollThumbBounds.Right - 4, ThumbMiddleY + 3);
	}

	void Control_CheckedListBox::UpdateScrollThumbBounds()
	{
		// Calculate visible content ratio
		int VisibleHeight = this->Height;
		int TotalHeight = TotalItemHeight();

		if (TotalHeight <= VisibleHeight)
		{
			// No scrollbar needed
			_ScrollThumbBounds = Rectangle(0, 0, 0, 0);
			return;
		}

		// Calculate thumb size (proportional to visible/total ratio)
		int ThumbHeight = Math::Max(30, (int)((float)VisibleHeight / TotalHeight * _ScrollBarBounds.Height));

		// Calculate thumb position
		int TrackHeight = _ScrollBarBounds.Height - ThumbHeight;
		int ThumbY = _ScrollBarBounds.Y;

		if (TrackHeight > 0)
		{
			float scrollRatio = (float)_ScrollOffset / (TotalHeight - VisibleHeight);
			ThumbY += (int)(TrackHeight * scrollRatio);
		}

		// Update thumb bounds
		_ScrollThumbBounds = Rectangle(_ScrollBarBounds.X, ThumbY, _ScrollBarBounds.Width, ThumbHeight);
	}

	void Control_CheckedListBox::OnMouseMove(MouseEventArgs^ e)
	{
		UserControl::OnMouseMove(e);

		// Handle scrollbar dragging
		if (_IsScrolling)
		{
			int DeltaY = e->Y - _ScrollStartY;

			// Calculate the scroll track height
			int TrackHeight = _ScrollBarBounds.Height - _ScrollThumbBounds.Height;

			if (TrackHeight > 0)
			{
				// Calculate new scroll position
				float ScrollRatio = (float)DeltaY / TrackHeight;
				int TotalScrollableHeight = TotalItemHeight() - this->Height;
				int NewOffset = _ScrollStartOffset + (int)(ScrollRatio * TotalScrollableHeight);

				// Update scroll position
				SetScrollOffset(NewOffset);
			}

			return;
		}

		// Check if mouse is over the scrollbar thumb
		if (_ScrollThumbBounds.Contains(e->Location))
		{
			this->Cursor = Cursors::Hand;
		}
		// Check if mouse is over the scrollbar track
		else if (_ScrollBarBounds.Contains(e->Location))
		{
			this->Cursor = Cursors::Arrow;
		}
		else
		{
			this->Cursor = Cursors::Default;

			// Check if mouse is over an item
			int NewHoverIndex = GetItemIndexAt(e->Location);

			if (NewHoverIndex != _HoverIndex)
			{
				_HoverIndex = NewHoverIndex;
				this->Invalidate();
			}
		}
	}

	void Control_CheckedListBox::OnMouseLeave(EventArgs^ e)
	{
		UserControl::OnMouseLeave(e);

		// Clear hover state
		if (_HoverIndex != -1)
		{
			_HoverIndex = -1;
			this->Invalidate();
		}

		this->Cursor = Cursors::Default;
	}

	void Control_CheckedListBox::OnMouseDown(MouseEventArgs^ e)
	{
		UserControl::OnMouseDown(e);

		if (e->Button == System::Windows::Forms::MouseButtons::Left)
		{
			// Check if clicking on scrollbar thumb
			if (_ScrollThumbBounds.Contains(e->Location))
			{
				_IsScrolling = true;
				_ScrollStartY = e->Y;
				_ScrollStartOffset = _ScrollOffset;
				this->Capture = true;
				this->Invalidate();
				return;
			}

			// Check if clicking on scrollbar track
			if (_ScrollBarBounds.Contains(e->Location))
			{
				// Page up/down
				int NewOffset = _ScrollOffset;

				if (e->Y < _ScrollThumbBounds.Y)
				{
					// Page up
					NewOffset -= this->Height;
				}
				else if (e->Y > _ScrollThumbBounds.Bottom)
				{
					// Page down
					NewOffset += this->Height;
				}

				SetScrollOffset(NewOffset);
				return;
			}

			// Check if clicking on an item
			int ClickedIndex = GetItemIndexAt(e->Location);

			if (ClickedIndex >= 0 && ClickedIndex < _Items->Count)
			{
				// Toggle checkbox for the clicked item (regardless of where clicked)
				ToggleItemChecked(ClickedIndex);

				// Also select the item
				SelectedIndex = ClickedIndex;
			}
		}
	}

	void Control_CheckedListBox::OnMouseUp(MouseEventArgs^ e)
	{
		UserControl::OnMouseUp(e);

		if (_IsScrolling)
		{
			_IsScrolling = false;
			this->Capture = false;
			this->Invalidate();
		}
	}

	void Control_CheckedListBox::OnKeyDown(KeyEventArgs^ e)
	{
		UserControl::OnKeyDown(e);

		// Handle keyboard navigation
		switch (e->KeyCode)
		{
		case Keys::Up:
			if (_SelectedIndex > 0)
			{
				SelectedIndex = _SelectedIndex - 1;
			}
			e->Handled = true;
			break;

		case Keys::Down:
			if (_SelectedIndex < _Items->Count - 1)
			{
				SelectedIndex = _SelectedIndex + 1;
			}
			e->Handled = true;
			break;

		case Keys::Home:
			if (_Items->Count > 0)
			{
				SelectedIndex = 0;
			}
			e->Handled = true;
			break;

		case Keys::End:
			if (_Items->Count > 0)
			{
				SelectedIndex = _Items->Count - 1;
			}
			e->Handled = true;
			break;

		case Keys::PageUp:
			if (_Items->Count > 0)
			{
				int visibleItems = this->Height / _ItemHeight;
				SelectedIndex = Math::Max(0, _SelectedIndex - visibleItems);
			}
			e->Handled = true;
			break;

		case Keys::PageDown:
			if (_Items->Count > 0)
			{
				int VisibleItems = this->Height / _ItemHeight;
				SelectedIndex = Math::Min(_Items->Count - 1, _SelectedIndex + VisibleItems);
			}
			e->Handled = true;
			break;

		case Keys::Space:
			if (_SelectedIndex >= 0 && _SelectedIndex < _Items->Count)
			{
				ToggleItemChecked(_SelectedIndex);
			}
			e->Handled = true;
			break;
		}
	}

	int Control_CheckedListBox::GetItemIndexAt(Point p)
	{
		// Check if point is within item area
		if (p.X < 1 || p.X >= this->Width - 16 || p.Y < 1 || p.Y >= this->Height - 1) {
			return -1;
		}

		// Calculate item index based on Y position
		int AdjustedY = p.Y + _ScrollOffset;
		int Index = AdjustedY / _ItemHeight;

		// Validate index
		if (Index >= 0 && Index < _Items->Count) {
			return Index;
		}

		return -1;
	}

	Rectangle Control_CheckedListBox::GetItemBounds(int index)
	{
		if (index < 0 || index >= _Items->Count) {
			return Rectangle::Empty;
		}

		int Y = (index * _ItemHeight) - _ScrollOffset;
		return Rectangle(0, Y, this->Width, _ItemHeight);
	}

	Rectangle Control_CheckedListBox::GetCheckBoxBounds(int index)
	{
		Rectangle ItemBounds = GetItemBounds(index);

		// Position checkbox in the item bounds
		int CheckSize = 16;
		int X = ItemBounds.X + 8;
		int Y = ItemBounds.Y + (ItemBounds.Height - CheckSize) / 2;

		return Rectangle(X, Y, CheckSize, CheckSize);
	}

	void Control_CheckedListBox::EnsureVisible(int index)
	{
		if (index < 0 || index >= _Items->Count) {
			return;
		}

		Rectangle ItemBounds = GetItemBounds(index);

		// Check if item is already visible
		if (ItemBounds.Y >= 0 && ItemBounds.Bottom <= this->Height)
			return;

		// Adjust scroll offset to make item visible
		if (ItemBounds.Y < 0)
		{
			// Scroll up to show item at top
			SetScrollOffset(_ScrollOffset + ItemBounds.Y);
		}
		else if (ItemBounds.Bottom > this->Height)
		{
			// Scroll down to show item at bottom
			SetScrollOffset(_ScrollOffset + (ItemBounds.Bottom - this->Height));
		}
	}

	int Control_CheckedListBox::VisibleItemCount()
	{
		return Math::Min(_Items->Count, this->Height / _ItemHeight);
	}

	int Control_CheckedListBox::TotalItemHeight()
	{
		return _Items->Count * _ItemHeight;
	}

	void Control_CheckedListBox::SetScrollOffset(int offset)
	{
		// Calculate maximum scroll offset
		int MaxOffset = Math::Max(0, TotalItemHeight() - this->Height);

		// Clamp offset to valid range
		int NewOffset = Math::Max(0, Math::Min(offset, MaxOffset));

		if (NewOffset != _ScrollOffset)
		{
			_ScrollOffset = NewOffset;
			UpdateScrollThumbBounds();
			this->Invalidate();
		}
	}

	void Control_CheckedListBox::ToggleItemChecked(int index)
	{
		if (index >= 0 && index < _Items->Count)
		{
			bool NewValue = !_Items[index]->Checked;
			SetItemChecked(index, NewValue);
		}
	}

	CheckedListItem^ Control_CheckedListBox::SelectedItem::get()
	{
		return (_SelectedIndex >= 0 && _SelectedIndex < _Items->Count) ? _Items[_SelectedIndex] : nullptr;
	}

	int Control_CheckedListBox::SelectedIndex::get()
	{ 
		return _SelectedIndex; 
	}

	void Control_CheckedListBox::SelectedIndex::set(int value)
	{
		if (value >= -1 && value < _Items->Count && _SelectedIndex != value)
		{
			_SelectedIndex = value;

			// Ensure the selected item is visible
			if (_SelectedIndex >= 0)
			{
				EnsureVisible(_SelectedIndex);
			}

			// Raise selection changed event
			SelectionChanged(this, EventArgs::Empty);

			this->Invalidate();
		}
	}

	List<CheckedListItem^>^ Control_CheckedListBox::Items::get()
	{
		return _Items;
	}

	List<CheckedListItem^>^ Control_CheckedListBox::CheckedItems::get()
	{
		List<CheckedListItem^>^ Result = gcnew List<CheckedListItem^>();

		for each (CheckedListItem^ Item in _Items)
		{
			if (Item->Checked) {
				Result->Add(Item);
			}
		}

		return Result;
	}

	bool Control_CheckedListBox::ShowItemCount::get()
	{
		return _ShowItemCount; 
	}

	void Control_CheckedListBox::ShowItemCount::set(bool value) 
	{
		_ShowItemCount = value;
		Invalidate();
	}
}
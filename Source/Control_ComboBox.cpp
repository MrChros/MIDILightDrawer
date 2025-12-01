#include "Control_ComboBox.h"

namespace MIDILightDrawer
{
	Control_ComboBox::Control_ComboBox()
	{
		Initialize_Component();
	}

	Control_ComboBox::~Control_ComboBox()
	{
		this->!Control_ComboBox();
	}

	Control_ComboBox::!Control_ComboBox()
	{
		if (_Close_Timer != nullptr) {
			_Close_Timer->Stop();
			delete _Close_Timer;
			_Close_Timer = nullptr;
		}

		if (_Drop_Down_Panel != nullptr) {
			if (_Drop_Down_Panel->Parent != nullptr) {
				_Drop_Down_Panel->Parent->Controls->Remove(_Drop_Down_Panel);
			}
			delete _Drop_Down_Panel;
			_Drop_Down_Panel = nullptr;
		}

		if (_Scroll_Bar != nullptr) {
			delete _Scroll_Bar;
			_Scroll_Bar = nullptr;
		}

		if (_Edit_Box != nullptr) {
			delete _Edit_Box;
			_Edit_Box = nullptr;
		}
	}


	/////////////////////
	// Private Methods //
	/////////////////////
	void Control_ComboBox::Initialize_Component()
	{
		// Enable double buffering
		this->SetStyle(ControlStyles::AllPaintingInWmPaint, true);
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer, true);
		this->SetStyle(ControlStyles::ResizeRedraw, true);
		this->SetStyle(ControlStyles::UserPaint, true);
		this->SetStyle(ControlStyles::Selectable, true);

		// Initialize items list
		_Items = gcnew List<Object^>();

		// Initialize state
		_Selected_Index = -1;
		_Highlight_Index = -1;
		_Scroll_Offset = 0;
		_Is_Dropped = false;
		_Is_Hovered = false;
		_Is_Focused = false;
		_Is_Mouse_Over_Arrow = false;
		_Enabled = true;

		// Initialize style
		_Drop_Down_Style = ComboBoxStyle::DropDownList;
		_Max_Drop_Down_Items = 8;
		_Item_Height = 20;

		// Apply theme colors
		Apply_Theme();

		// Set default size
		this->Size = System::Drawing::Size(200, 24);

		// Calculate arrow bounds
		int Arrow_Size = 16;
		_Arrow_Bounds = Rectangle(Width - Arrow_Size - 4, (Height - Arrow_Size) / 2, Arrow_Size, Arrow_Size);

		// Initialize close timer
		_Close_Timer = gcnew Timer();
		_Close_Timer->Interval = 100;
		_Close_Timer->Tick += gcnew EventHandler(this, &Control_ComboBox::Handle_Close_Timer_Tick);

		// Initialize dropdown panel
		_Drop_Down_Panel = gcnew ComboBox_Drop_Down_Panel();
		_Drop_Down_Panel->Visible = false;
		_Drop_Down_Panel->BorderStyle = BorderStyle::None;
		_Drop_Down_Panel->Paint += gcnew PaintEventHandler(this, &Control_ComboBox::Paint_Drop_Down);
		_Drop_Down_Panel->MouseMove += gcnew MouseEventHandler(this, &Control_ComboBox::Handle_Drop_Down_Mouse_Move);
		_Drop_Down_Panel->MouseClick += gcnew MouseEventHandler(this, &Control_ComboBox::Handle_Drop_Down_Mouse_Click);
		_Drop_Down_Panel->MouseWheel += gcnew MouseEventHandler(this, &Control_ComboBox::Handle_Drop_Down_Mouse_Wheel);

		_Mouse_Leave_Handler = gcnew EventHandler(this, &Control_ComboBox::Handle_Drop_Down_Mouse_Leave);

		// Initialize scrollbar
		_Scroll_Bar = gcnew VScrollBar();
		_Scroll_Bar->Dock = DockStyle::Right;
		_Scroll_Bar->Width = 16;
		_Scroll_Bar->Visible = false;
		_Scroll_Bar->Scroll += gcnew ScrollEventHandler(this, &Control_ComboBox::Handle_Scroll_Bar_Scroll);
		_Drop_Down_Panel->Controls->Add(_Scroll_Bar);

		// Initialize edit box for DropDown style (not DropDownList)
		_Edit_Box = gcnew TextBox();
		_Edit_Box->BorderStyle = BorderStyle::None;
		_Edit_Box->Visible = false;
		_Edit_Box->TextChanged += gcnew EventHandler(this, &Control_ComboBox::Handle_Edit_Box_Text_Changed);
		_Edit_Box->KeyDown += gcnew KeyEventHandler(this, &Control_ComboBox::Handle_Edit_Box_Key_Down);
		this->Controls->Add(_Edit_Box);
	}

	void Control_ComboBox::Apply_Theme()
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		_Background_Color = Theme->BackgroundLight;
		_Background_Disabled_Color = Theme->Background;
		_Border_Color = Theme->BorderStrong;
		_Border_Focused_Color = Theme->AccentPrimary;
		_Arrow_Color = Theme->ForegroundText;
		_Text_Color = Theme->ForegroundText;
		_Text_Disabled_Color = Color::FromArgb(128, Theme->ForegroundText);
		_Dropdown_Background_Color = Theme->Background;
		_Dropdown_Border_Color = Theme->BorderPrimary;
		_Hover_Color = Theme->AccentPrimary;
		_Selected_Color = Color::FromArgb(80, (int)Theme->AccentPrimary.R, (int)Theme->AccentPrimary.G, (int)Theme->AccentPrimary.B);

		// Apply to edit box if exists
		if (_Edit_Box != nullptr) {
			_Edit_Box->BackColor = _Background_Color;
			_Edit_Box->ForeColor = _Text_Color;
		}

		this->Invalidate();
	}

	void Control_ComboBox::Show_Drop_Down()
	{
		if (_Items->Count == 0) return;

		// Add panel to form if not already
		if (_Drop_Down_Panel->Parent == nullptr && this->FindForm() != nullptr) {
			Form^ Parent_Form = this->FindForm();
			Parent_Form->Controls->Add(_Drop_Down_Panel);
			_Drop_Down_Panel->BringToFront();
		}

		// Update size and position
		Update_Drop_Down_Size();
		Update_Drop_Down_Position();
		Update_Scroll_Bar();

		// Ensure selected item is visible
		if (_Selected_Index >= 0) {
			int Visible_Count = Get_Visible_Item_Count();
			if (_Selected_Index < _Scroll_Offset) {
				_Scroll_Offset = _Selected_Index;
			}
			else if (_Selected_Index >= _Scroll_Offset + Visible_Count) {
				_Scroll_Offset = _Selected_Index - Visible_Count + 1;
			}
			_Scroll_Bar->Value = _Scroll_Offset;
		}

		// Show panel
		_Drop_Down_Panel->Visible = true;
		_Drop_Down_Panel->BringToFront();

		// Add mouse leave handlers
		this->MouseLeave += _Mouse_Leave_Handler;
		_Drop_Down_Panel->MouseLeave += _Mouse_Leave_Handler;

		_Is_Dropped = true;
		_Highlight_Index = _Selected_Index;

		this->Invalidate();

		// Raise event
		Drop_Down_Opened(this, EventArgs::Empty);
	}

	void Control_ComboBox::Close_Drop_Down()
	{
		_Close_Timer->Stop();

		if (_Drop_Down_Panel != nullptr) {
			this->MouseLeave -= _Mouse_Leave_Handler;
			_Drop_Down_Panel->MouseLeave -= _Mouse_Leave_Handler;

			_Drop_Down_Panel->Visible = false;
		}

		_Is_Dropped = false;
		_Highlight_Index = -1;

		this->Invalidate();

		// Raise event
		Drop_Down_Closed(this, EventArgs::Empty);
	}

	void Control_ComboBox::Update_Drop_Down_Position()
	{
		if (this->FindForm() == nullptr) return;

		// Get screen position below the control
		Point Location = this->Parent->PointToScreen(Point(this->Left, this->Bottom));
		Location = this->FindForm()->PointToClient(Location);

		// Check if there's enough space below, otherwise open above
		Form^ Parent_Form = this->FindForm();
		int Space_Below = Parent_Form->ClientSize.Height - Location.Y;
		int Panel_Height = _Drop_Down_Panel->Height;

		if (Space_Below < Panel_Height) {
			// Not enough space below, open above
			Location = this->Parent->PointToScreen(Point(this->Left, this->Top - Panel_Height));
			Location = this->FindForm()->PointToClient(Location);
		}

		_Drop_Down_Panel->Location = Location;
	}

	void Control_ComboBox::Update_Drop_Down_Size()
	{
		int Visible_Items = Math::Min(_Items->Count, _Max_Drop_Down_Items);
		int Panel_Height = Visible_Items * _Item_Height + 2; // +2 for border
		int Panel_Width = this->Width;

		_Drop_Down_Panel->Size = System::Drawing::Size(Panel_Width, Panel_Height);
	}

	void Control_ComboBox::Update_Scroll_Bar()
	{
		int Visible_Items = Get_Visible_Item_Count();

		if (_Items->Count > Visible_Items) {
			_Scroll_Bar->Visible = true;
			_Scroll_Bar->Minimum = 0;
			_Scroll_Bar->Maximum = _Items->Count - 1;
			_Scroll_Bar->LargeChange = Visible_Items;
			_Scroll_Bar->SmallChange = 1;
			_Scroll_Bar->Value = _Scroll_Offset;
		}
		else {
			_Scroll_Bar->Visible = false;
			_Scroll_Offset = 0;
		}
	}

	void Control_ComboBox::Paint_Drop_Down(Object^ sender, PaintEventArgs^ e)
	{
		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		G->TextRenderingHint = Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		Rectangle Bounds = _Drop_Down_Panel->ClientRectangle;

		// Fill background
		G->FillRectangle(gcnew SolidBrush(_Dropdown_Background_Color), Bounds);

		// Draw items
		int Visible_Count = Get_Visible_Item_Count();
		int Item_Area_Width = _Scroll_Bar->Visible ? Bounds.Width - _Scroll_Bar->Width : Bounds.Width;

		for (int i = 0; i < Visible_Count; i++) {
			int Item_Index = i + _Scroll_Offset;
			if (Item_Index >= _Items->Count) break;

			Rectangle Item_Rect = Rectangle(1, 1 + i * _Item_Height, Item_Area_Width - 2, _Item_Height);
			bool Is_Highlighted = (Item_Index == _Highlight_Index);
			bool Is_Selected = (Item_Index == _Selected_Index);

			Draw_Item(G, Item_Index, Item_Rect, Is_Highlighted, Is_Selected);
		}

		// Draw border
		G->DrawRectangle(gcnew Pen(_Dropdown_Border_Color), 0, 0, Bounds.Width - 1, Bounds.Height - 1);
	}

	void Control_ComboBox::Draw_Item(Graphics^ g, int index, Rectangle bounds, bool is_highlighted, bool is_selected)
	{
		if (index < 0 || index >= _Items->Count) return;

		// Draw background
		if (is_highlighted) {
			g->FillRectangle(gcnew SolidBrush(_Hover_Color), bounds);
		}
		else if (is_selected) {
			g->FillRectangle(gcnew SolidBrush(_Selected_Color), bounds);
		}

		// Get item text
		String^ Text = _Items[index]->ToString();

		// Draw text
		StringFormat^ Format = gcnew StringFormat();
		Format->Alignment = StringAlignment::Near;
		Format->LineAlignment = StringAlignment::Center;
		Format->Trimming = StringTrimming::EllipsisCharacter;
		Format->FormatFlags = StringFormatFlags::NoWrap;

		Rectangle Text_Rect = Rectangle(bounds.X + 4, bounds.Y, bounds.Width - 8, bounds.Height);

		Color Item_Text_Color = (is_highlighted) ? Color::White : _Text_Color;
		g->DrawString(Text, this->Font, gcnew SolidBrush(Item_Text_Color), Text_Rect, Format);

		delete Format;
	}

	void Control_ComboBox::Draw_Arrow(Graphics^ g, Rectangle bounds, Color color)
	{
		int Arrow_Width = 8;
		int Arrow_Height = 4;
		int Center_X = bounds.X + bounds.Width / 2;
		int Center_Y = bounds.Y + bounds.Height / 2;

		array<Point>^ Arrow_Points;

		if (_Is_Dropped) {
			// Up arrow when dropped
			Arrow_Points = gcnew array<Point> {
				Point(Center_X - Arrow_Width / 2, Center_Y + Arrow_Height / 2),
					Point(Center_X + Arrow_Width / 2, Center_Y + Arrow_Height / 2),
					Point(Center_X, Center_Y - Arrow_Height / 2)
			};
		}
		else {
			// Down arrow when closed
			Arrow_Points = gcnew array<Point> {
				Point(Center_X - Arrow_Width / 2, Center_Y - Arrow_Height / 2),
					Point(Center_X + Arrow_Width / 2, Center_Y - Arrow_Height / 2),
					Point(Center_X, Center_Y + Arrow_Height / 2)
			};
		}

		g->FillPolygon(gcnew SolidBrush(color), Arrow_Points);
	}

	void Control_ComboBox::Handle_Drop_Down_Mouse_Move(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Is_Dropped) return;

		int New_Highlight = Get_Item_At_Point(e->Location);

		if (New_Highlight != _Highlight_Index) {
			_Highlight_Index = New_Highlight;
			_Drop_Down_Panel->Invalidate();
		}
	}

	void Control_ComboBox::Handle_Drop_Down_Mouse_Click(Object^ sender, MouseEventArgs^ e)
	{
		if (e->Button != System::Windows::Forms::MouseButtons::Left) return;

		int Clicked_Index = Get_Item_At_Point(e->Location);

		if (Clicked_Index >= 0 && Clicked_Index < _Items->Count) {
			Selected_Index = Clicked_Index;
			Close_Drop_Down();
		}
	}

	void Control_ComboBox::Handle_Drop_Down_Mouse_Leave(Object^ sender, EventArgs^ e)
	{
		// Start timer to check if we should close
		_Close_Timer->Start();
	}

	void Control_ComboBox::Handle_Drop_Down_Mouse_Wheel(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Is_Dropped || !_Scroll_Bar->Visible) return;

		int Delta = e->Delta > 0 ? -3 : 3;
		int New_Offset = Math::Max(0, Math::Min(_Scroll_Offset + Delta, _Items->Count - Get_Visible_Item_Count()));

		if (New_Offset != _Scroll_Offset) {
			_Scroll_Offset = New_Offset;
			_Scroll_Bar->Value = _Scroll_Offset;
			_Drop_Down_Panel->Invalidate();
		}
	}

	void Control_ComboBox::Handle_Close_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		_Close_Timer->Stop();

		if (!Is_Mouse_Over_Controls()) {
			Close_Drop_Down();
		}
	}

	void Control_ComboBox::Handle_Scroll_Bar_Scroll(Object^ sender, ScrollEventArgs^ e)
	{
		_Scroll_Offset = e->NewValue;
		_Drop_Down_Panel->Invalidate();
	}

	void Control_ComboBox::Handle_Edit_Box_Text_Changed(Object^ sender, EventArgs^ e)
	{
		if (_Drop_Down_Style != ComboBoxStyle::DropDown) return;

		// Auto-complete logic - find matching item
		int Found_Index = Find_String_Start(_Edit_Box->Text);
		if (Found_Index >= 0) {
			_Highlight_Index = Found_Index;

			// Ensure visible
			int Visible_Count = Get_Visible_Item_Count();
			if (Found_Index < _Scroll_Offset) {
				_Scroll_Offset = Found_Index;
			}
			else if (Found_Index >= _Scroll_Offset + Visible_Count) {
				_Scroll_Offset = Found_Index - Visible_Count + 1;
			}

			if (_Scroll_Bar->Visible) {
				_Scroll_Bar->Value = _Scroll_Offset;
			}

			_Drop_Down_Panel->Invalidate();
		}
	}

	void Control_ComboBox::Handle_Edit_Box_Key_Down(Object^ sender, KeyEventArgs^ e)
	{
		// Forward key events
		OnKeyDown(e);
	}

	bool Control_ComboBox::Is_Mouse_Over_Controls()
	{
		Point Cursor_Pos = Control::MousePosition;

		// Check main control
		Rectangle Control_Rect = this->RectangleToScreen(this->ClientRectangle);
		if (Control_Rect.Contains(Cursor_Pos)) return true;

		// Check dropdown panel
		if (_Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible) {
			Rectangle Panel_Rect = _Drop_Down_Panel->RectangleToScreen(_Drop_Down_Panel->ClientRectangle);
			if (Panel_Rect.Contains(Cursor_Pos)) return true;
		}

		return false;
	}

	int Control_ComboBox::Get_Item_At_Point(Point point)
	{
		int Visible_Index = (point.Y - 1) / _Item_Height;
		int Item_Index = Visible_Index + _Scroll_Offset;

		if (Item_Index >= 0 && Item_Index < _Items->Count) {
			return Item_Index;
		}

		return -1;
	}

	int Control_ComboBox::Get_Visible_Item_Count()
	{
		return Math::Min(_Items->Count, _Max_Drop_Down_Items);
	}

	Rectangle Control_ComboBox::Get_Item_Bounds(int visible_index)
	{
		int Item_Area_Width = _Scroll_Bar->Visible ? _Drop_Down_Panel->Width - _Scroll_Bar->Width : _Drop_Down_Panel->Width;
		return Rectangle(1, 1 + visible_index * _Item_Height, Item_Area_Width - 2, _Item_Height);
	}

	int Control_ComboBox::Find_String_Start(String^ text)
	{
		if (String::IsNullOrEmpty(text)) return -1;

		for (int i = 0; i < _Items->Count; i++) {
			if (_Items[i]->ToString()->StartsWith(text, StringComparison::OrdinalIgnoreCase)) {
				return i;
			}
		}

		return -1;
	}


	///////////////////////
	// Protected Methods //
	///////////////////////
	void Control_ComboBox::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		G->TextRenderingHint = Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		Rectangle Bounds = this->ClientRectangle;

		// Draw background
		Color Bg_Color = this->Enabled ? _Background_Color : _Background_Disabled_Color;
		G->FillRectangle(gcnew SolidBrush(Bg_Color), Bounds);

		// Draw border
		Color Current_Border_Color = (_Is_Focused || _Is_Dropped) ? _Border_Focused_Color : _Border_Color;
		Pen^ Border_Pen = gcnew Pen(Current_Border_Color, 1);
		G->DrawRectangle(Border_Pen, 0, 0, Bounds.Width - 1, Bounds.Height - 1);
		delete Border_Pen;

		// Calculate text area (leave space for arrow)
		int Arrow_Area_Width = 20;
		Rectangle Text_Rect = Rectangle(4, 2, Bounds.Width - Arrow_Area_Width - 4, Bounds.Height - 4);

		// Draw selected item text (only if DropDownList or edit box is not visible)
		if (_Drop_Down_Style == ComboBoxStyle::DropDownList || !_Edit_Box->Visible) {
			if (_Selected_Index >= 0 && _Selected_Index < _Items->Count) {
				StringFormat^ Format = gcnew StringFormat();
				Format->Alignment = StringAlignment::Near;
				Format->LineAlignment = StringAlignment::Center;
				Format->Trimming = StringTrimming::EllipsisCharacter;
				Format->FormatFlags = StringFormatFlags::NoWrap;

				Color Current_Text_Color = this->Enabled ? _Text_Color : _Text_Disabled_Color;
				G->DrawString(_Items[_Selected_Index]->ToString(), this->Font, gcnew SolidBrush(Current_Text_Color), Text_Rect, Format);

				delete Format;
			}
		}

		// Draw arrow
		_Arrow_Bounds = Rectangle(Bounds.Width - Arrow_Area_Width, 0, Arrow_Area_Width, Bounds.Height);

		// Arrow separator line
		Pen^ Separator_Pen = gcnew Pen(_Border_Color, 1);
		G->DrawLine(Separator_Pen, _Arrow_Bounds.Left, 4, _Arrow_Bounds.Left, Bounds.Height - 4);
		delete Separator_Pen;

		// Arrow color
		Color Current_Arrow_Color = this->Enabled ? _Arrow_Color : _Text_Disabled_Color;
		if (_Is_Mouse_Over_Arrow && this->Enabled) {
			Current_Arrow_Color = _Border_Focused_Color;
		}

		Draw_Arrow(G, _Arrow_Bounds, Current_Arrow_Color);

		Control::OnPaint(e);
	}

	void Control_ComboBox::OnMouseClick(MouseEventArgs^ e)
	{
		if (!this->Enabled) return;

		if (e->Button == System::Windows::Forms::MouseButtons::Left) {
			if (_Drop_Down_Style == ComboBoxStyle::DropDownList) {
				// Entire control toggles dropdown
				if (_Is_Dropped) {
					Close_Drop_Down();
				}
				else {
					Show_Drop_Down();
				}
			}
			else {
				// Only arrow area toggles dropdown for editable styles
				if (_Arrow_Bounds.Contains(e->Location)) {
					if (_Is_Dropped) {
						Close_Drop_Down();
					}
					else {
						Show_Drop_Down();
					}
				}
			}
		}

		Control::OnMouseClick(e);
	}

	void Control_ComboBox::OnMouseDown(MouseEventArgs^ e)
	{
		this->Focus();
		Control::OnMouseDown(e);
	}

	void Control_ComboBox::OnMouseMove(MouseEventArgs^ e)
	{
		bool Was_Over_Arrow = _Is_Mouse_Over_Arrow;
		_Is_Mouse_Over_Arrow = _Arrow_Bounds.Contains(e->Location);

		if (Was_Over_Arrow != _Is_Mouse_Over_Arrow) {
			this->Invalidate();
		}

		Control::OnMouseMove(e);
	}

	void Control_ComboBox::OnMouseEnter(EventArgs^ e)
	{
		_Is_Hovered = true;
		this->Invalidate();
		Control::OnMouseEnter(e);
	}

	void Control_ComboBox::OnMouseLeave(EventArgs^ e)
	{
		_Is_Hovered = false;
		_Is_Mouse_Over_Arrow = false;
		this->Invalidate();

		if (_Is_Dropped) {
			_Close_Timer->Start();
		}

		Control::OnMouseLeave(e);
	}

	void Control_ComboBox::OnMouseWheel(MouseEventArgs^ e)
	{
		if (!this->Enabled) return;

		if (_Is_Dropped && _Scroll_Bar->Visible) {
			// Scroll the dropdown list
			Handle_Drop_Down_Mouse_Wheel(this, e);
		}
		else if (!_Is_Dropped) {
			// Change selection when closed
			if (e->Delta > 0) {
				Select_Previous();
			}
			else {
				Select_Next();
			}
		}

		Control::OnMouseWheel(e);
	}

	void Control_ComboBox::OnGotFocus(EventArgs^ e)
	{
		_Is_Focused = true;
		this->Invalidate();
		Control::OnGotFocus(e);
	}

	void Control_ComboBox::OnLostFocus(EventArgs^ e)
	{
		_Is_Focused = false;

		// Close dropdown if open
		if (_Is_Dropped) {
			Close_Drop_Down();
		}

		this->Invalidate();
		Control::OnLostFocus(e);
	}

	void Control_ComboBox::OnKeyDown(KeyEventArgs^ e)
	{
		if (!this->Enabled) return;

		switch (e->KeyCode) {
		case Keys::Down:
			if (!_Is_Dropped) {
				if (e->Alt) {
					Show_Drop_Down();
				}
				else {
					Select_Next();
				}
			}
			else {
				// Move highlight down
				if (_Highlight_Index < _Items->Count - 1) {
					_Highlight_Index++;

					// Ensure visible
					int Visible_Count = Get_Visible_Item_Count();
					if (_Highlight_Index >= _Scroll_Offset + Visible_Count) {
						_Scroll_Offset = _Highlight_Index - Visible_Count + 1;
						_Scroll_Bar->Value = _Scroll_Offset;
					}

					_Drop_Down_Panel->Invalidate();
				}
			}
			e->Handled = true;
			break;

		case Keys::Up:
			if (!_Is_Dropped) {
				if (e->Alt) {
					Show_Drop_Down();
				}
				else {
					Select_Previous();
				}
			}
			else {
				// Move highlight up
				if (_Highlight_Index > 0) {
					_Highlight_Index--;

					// Ensure visible
					if (_Highlight_Index < _Scroll_Offset) {
						_Scroll_Offset = _Highlight_Index;
						_Scroll_Bar->Value = _Scroll_Offset;
					}

					_Drop_Down_Panel->Invalidate();
				}
			}
			e->Handled = true;
			break;

		case Keys::Enter:
			if (_Is_Dropped && _Highlight_Index >= 0) {
				Selected_Index = _Highlight_Index;
				Close_Drop_Down();
			}
			e->Handled = true;
			break;

		case Keys::Escape:
			if (_Is_Dropped) {
				Close_Drop_Down();
			}
			e->Handled = true;
			break;

		case Keys::Home:
			if (_Is_Dropped) {
				_Highlight_Index = 0;
				_Scroll_Offset = 0;
				_Scroll_Bar->Value = 0;
				_Drop_Down_Panel->Invalidate();
			}
			else {
				Selected_Index = 0;
			}
			e->Handled = true;
			break;

		case Keys::End:
			if (_Is_Dropped) {
				_Highlight_Index = _Items->Count - 1;
				_Scroll_Offset = Math::Max(0, _Items->Count - Get_Visible_Item_Count());
				_Scroll_Bar->Value = _Scroll_Offset;
				_Drop_Down_Panel->Invalidate();
			}
			else {
				Selected_Index = _Items->Count - 1;
			}
			e->Handled = true;
			break;

		case Keys::PageUp:
			if (_Is_Dropped) {
				int Page_Size = Get_Visible_Item_Count();
				_Highlight_Index = Math::Max(0, _Highlight_Index - Page_Size);
				_Scroll_Offset = Math::Max(0, _Scroll_Offset - Page_Size);
				_Scroll_Bar->Value = _Scroll_Offset;
				_Drop_Down_Panel->Invalidate();
			}
			e->Handled = true;
			break;

		case Keys::PageDown:
			if (_Is_Dropped) {
				int Page_Size = Get_Visible_Item_Count();
				_Highlight_Index = Math::Min(_Items->Count - 1, _Highlight_Index + Page_Size);
				_Scroll_Offset = Math::Min(_Items->Count - Page_Size, _Scroll_Offset + Page_Size);
				if (_Scroll_Offset < 0) _Scroll_Offset = 0;
				_Scroll_Bar->Value = _Scroll_Offset;
				_Drop_Down_Panel->Invalidate();
			}
			e->Handled = true;
			break;

		case Keys::F4:
			// Toggle dropdown
			if (_Is_Dropped) {
				Close_Drop_Down();
			}
			else {
				Show_Drop_Down();
			}
			e->Handled = true;
			break;
		}

		Control::OnKeyDown(e);
	}

	void Control_ComboBox::OnSizeChanged(EventArgs^ e)
	{
		// Update arrow bounds
		int Arrow_Area_Width = 20;
		_Arrow_Bounds = Rectangle(Width - Arrow_Area_Width, 0, Arrow_Area_Width, Height);

		// Update edit box bounds if visible
		if (_Edit_Box != nullptr && _Edit_Box->Visible) {
			_Edit_Box->SetBounds(4, (Height - _Edit_Box->Height) / 2, Width - Arrow_Area_Width - 8, _Edit_Box->Height);
		}

		Control::OnSizeChanged(e);
	}

	void Control_ComboBox::OnEnabledChanged(EventArgs^ e)
	{
		if (_Edit_Box != nullptr) {
			_Edit_Box->Enabled = this->Enabled;
		}

		this->Invalidate();
		Control::OnEnabledChanged(e);
	}


	////////////////////
	// Public Methods //
	////////////////////
	void Control_ComboBox::Add_Item(Object^ item)
	{
		_Items->Add(item);
		this->Invalidate();
	}

	void Control_ComboBox::Add_Items(array<Object^>^ items)
	{
		for each (Object ^ Item in items) {
			_Items->Add(Item);
		}
		this->Invalidate();
	}

	void Control_ComboBox::Insert_Item(int index, Object^ item)
	{
		if (index >= 0 && index <= _Items->Count) {
			_Items->Insert(index, item);

			// Adjust selected index if needed
			if (_Selected_Index >= index) {
				_Selected_Index++;
			}

			this->Invalidate();
		}
	}

	void Control_ComboBox::Remove_Item(Object^ item)
	{
		int Index = _Items->IndexOf(item);
		if (Index >= 0) {
			Remove_At(Index);
		}
	}

	void Control_ComboBox::Remove_At(int index)
	{
		if (index >= 0 && index < _Items->Count) {
			_Items->RemoveAt(index);

			// Adjust selected index
			if (_Selected_Index == index) {
				_Selected_Index = Math::Min(_Selected_Index, _Items->Count - 1);
			}
			else if (_Selected_Index > index) {
				_Selected_Index--;
			}

			this->Invalidate();
		}
	}

	void Control_ComboBox::Clear_Items()
	{
		_Items->Clear();
		_Selected_Index = -1;
		_Highlight_Index = -1;
		_Scroll_Offset = 0;

		if (_Is_Dropped) {
			Close_Drop_Down();
		}

		this->Invalidate();
	}

	int Control_ComboBox::Find_String(String^ text)
	{
		return Find_String_Start(text);
	}

	int Control_ComboBox::Find_String_Exact(String^ text)
	{
		for (int i = 0; i < _Items->Count; i++) {
			if (_Items[i]->ToString()->Equals(text, StringComparison::OrdinalIgnoreCase)) {
				return i;
			}
		}
		return -1;
	}

	void Control_ComboBox::Select_Next()
	{
		if (_Selected_Index < _Items->Count - 1) {
			Selected_Index = _Selected_Index + 1;
		}
	}

	void Control_ComboBox::Select_Previous()
	{
		if (_Selected_Index > 0) {
			Selected_Index = _Selected_Index - 1;
		}
	}

	void Control_ComboBox::Begin_Update()
	{
		// Could implement update locking for performance
	}

	void Control_ComboBox::End_Update()
	{
		this->Invalidate();
	}


	//////////////////////
	// Property Setters //
	//////////////////////
	void Control_ComboBox::Selected_Index::set(int value)
	{
		if (value >= -1 && value < _Items->Count && value != _Selected_Index) {
			int Old_Index = _Selected_Index;
			_Selected_Index = value;

			// Update edit box text for editable styles
			if (_Drop_Down_Style == ComboBoxStyle::DropDown && _Edit_Box != nullptr) {
				if (_Selected_Index >= 0) {
					_Edit_Box->Text = _Items[_Selected_Index]->ToString();
				}
			}

			this->Invalidate();

			// Raise event
			if (_Selected_Index >= 0) {
				Control_ComboBox_Selection_Changed_Event_Args^ Args =
					gcnew Control_ComboBox_Selection_Changed_Event_Args(_Selected_Index, _Items[_Selected_Index]);
				Selection_Changed(this, Args);
			}
		}
	}

	Object^ Control_ComboBox::Selected_Item::get()
	{
		if (_Selected_Index >= 0 && _Selected_Index < _Items->Count) {
			return _Items[_Selected_Index];
		}
		return nullptr;
	}

	void Control_ComboBox::Selected_Item::set(Object^ value)
	{
		int Index = _Items->IndexOf(value);
		if (Index >= 0) {
			Selected_Index = Index;
		}
	}

	String^ Control_ComboBox::Selected_Text::get()
	{
		if (_Selected_Index >= 0 && _Selected_Index < _Items->Count) {
			return _Items[_Selected_Index]->ToString();
		}
		return String::Empty;
	}

	void Control_ComboBox::Max_Drop_Down_Items::set(int value)
	{
		_Max_Drop_Down_Items = Math::Max(1, value);
	}

	void Control_ComboBox::Item_Height::set(int value)
	{
		_Item_Height = Math::Max(16, value);
	}

	void Control_ComboBox::Drop_Down_Style::set(ComboBoxStyle value)
	{
		if (_Drop_Down_Style != value) {
			_Drop_Down_Style = value;

			// Show/hide edit box based on style
			if (_Edit_Box != nullptr) {
				if (_Drop_Down_Style == ComboBoxStyle::DropDown) {
					_Edit_Box->Visible = true;
					int Arrow_Area_Width = 20;
					_Edit_Box->SetBounds(4, (Height - _Edit_Box->Height) / 2, Width - Arrow_Area_Width - 8, _Edit_Box->Height);
				}
				else {
					_Edit_Box->Visible = false;
				}
			}

			this->Invalidate();
		}
	}

	void Control_ComboBox::Background_Color::set(Color value)
	{
		_Background_Color = value;
		this->Invalidate();
	}

	void Control_ComboBox::Border_Color::set(Color value)
	{
		_Border_Color = value;
		this->Invalidate();
	}

	void Control_ComboBox::Text_Color::set(Color value)
	{
		_Text_Color = value;
		this->Invalidate();
	}

}
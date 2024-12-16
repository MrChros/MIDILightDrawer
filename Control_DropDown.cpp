#include "Control_DropDown.h"

namespace MIDILightDrawer {

	Control_DropDown::Control_DropDown() {
		Initialize_Component();
	}

	void Control_DropDown::Set_Title_Color(Color color) {
		if (_Title_Color != color) {
			_Title_Color = color;
			this->Invalidate(); // Redraw with new color
		}
	}

	void Control_DropDown::Initialize_Component() {
		_Selected_Index = -1;
		_Is_Dropped = false;
		_Highlight_Index = -1;
		_Tile_Width = 150;
		_Tile_Height = 50;
		_Columns = 3;
		_Title_Color = Color::Black;
		_Open_Above = false;  // Default to opening below
		_Horizontal_Alignment = Panel_Horizontal_Alignment::Left;

		// Calculate optimal height
		int optimal_height = 20 + 20;  // title height + content height
		this->Size = System::Drawing::Size(200, optimal_height);
		this->BackColor = Color::White;

		// Adjust arrow position for new height
		_Arrow_Bounds = Rectangle(Width - 15, optimal_height - 15, 8, 6);

		_Drop_Down_Panel = gcnew Double_Buffered_Panel();
		_Drop_Down_Panel->Visible = false;
		_Drop_Down_Panel->BorderStyle = BorderStyle::FixedSingle;
		_Drop_Down_Panel->BackColor = Color::White;

		_Drop_Down_Panel->Paint += gcnew PaintEventHandler(this, &Control_DropDown::Paint_Drop_Down);
		_Drop_Down_Panel->MouseMove += gcnew MouseEventHandler(this, &Control_DropDown::Handle_Mouse_Move);
		_Drop_Down_Panel->MouseClick += gcnew MouseEventHandler(this, &Control_DropDown::Handle_Mouse_Click);
	}

	void Control_DropDown::Show_Drop_Down() {
		if (_Drop_Down_Panel->Parent == nullptr && this->FindForm() != nullptr) {
			Form^ parent_form = this->FindForm();

			// Create dictionary to store scroll positions
			Dictionary<ScrollableControl^, Point>^ scroll_positions = gcnew Dictionary<ScrollableControl^, Point>();

			// Recursively find all scrollable controls in the form
			Find_Scrollable_Controls(parent_form, scroll_positions);

			// Add dropdown panel
			parent_form->Click += gcnew EventHandler(this, &Control_DropDown::Form_Click);
			parent_form->Controls->Add(_Drop_Down_Panel);
			_Drop_Down_Panel->BringToFront();

			// Restore scroll positions
			Restore_Scroll_Positions(scroll_positions);
		}

		int total_rows = Get_Row_Count();
		_Drop_Down_Panel->Size = System::Drawing::Size(_Tile_Width * _Columns, _Tile_Height * total_rows);

		Update_Panel_Position();

		_Drop_Down_Panel->BackColor = Color::White;
		_Drop_Down_Panel->BorderStyle = BorderStyle::FixedSingle;
		_Drop_Down_Panel->Visible = true;
		_Drop_Down_Panel->BringToFront();
		_Is_Dropped = true;
	}

	int Control_DropDown::Get_Row_Count() {
		if (_First_Lines == nullptr) return 0;
		return (int)Math::Ceiling((double)_First_Lines->Length / _Columns);
	}

	void Control_DropDown::Update_Panel_Position() {
		Point location;
		int panel_width = _Tile_Width * _Columns;
		int x_position;

		// Calculate horizontal position
		switch (_Horizontal_Alignment) {
		case Panel_Horizontal_Alignment::Left:
			x_position = this->Left;
			break;
		case Panel_Horizontal_Alignment::Right:
			x_position = this->Left + (this->Width - panel_width);
			break;
		default:
			x_position = this->Left;
			break;
		}

		if (_Open_Above) {
			int total_rows = Get_Row_Count();
			int panel_height = _Tile_Height * total_rows;
			// Position above the control
			location = this->Parent->PointToScreen(Point(x_position, this->Top - panel_height));
		}
		else {
			// Position below the control
			location = this->Parent->PointToScreen(Point(x_position, this->Bottom));
		}

		location = this->FindForm()->PointToClient(location);
		_Drop_Down_Panel->Location = location;
	}

	void Control_DropDown::Form_Click(Object^ sender, EventArgs^ e) {
		if (_Is_Dropped) {
			Point mouse_pos = this->FindForm()->PointToClient(Control::MousePosition);
			if (!_Drop_Down_Panel->Bounds.Contains(mouse_pos)) {
				Close_Drop_Down(nullptr, nullptr);
			}
		}
	}

	int Control_DropDown::Find_Index_By_Value(int value) {
		if (_Values == nullptr) return -1;
		for (int i = 0; i < _Values->Length; i++) {
			if (_Values[i] == value) return i;
		}
		return -1;
	}

	void Control_DropDown::Find_Scrollable_Controls(Control^ control, Dictionary<ScrollableControl^, Point>^ scroll_positions) {
		// Check if current control is scrollable
		if (ScrollableControl^ scrollable = dynamic_cast<ScrollableControl^>(control)) {
			if (!scroll_positions->ContainsKey(scrollable)) {
				scroll_positions->Add(scrollable, Point(scrollable->HorizontalScroll->Value,
					scrollable->VerticalScroll->Value));
			}
		}

		// Recursively check all child controls
		for each (Control ^ child in control->Controls) {
			Find_Scrollable_Controls(child, scroll_positions);
		}
	}

	void Control_DropDown::Restore_Scroll_Positions(Dictionary<ScrollableControl^, Point>^ scroll_positions) {
		for each (KeyValuePair<ScrollableControl^, Point> position in scroll_positions) {
			try {
				ScrollableControl^ scrollable = position.Key;
				if (scrollable != nullptr && !scrollable->IsDisposed) {
					// Restore horizontal scroll
					if (scrollable->HorizontalScroll->Visible) {
						scrollable->HorizontalScroll->Value = Math::Min(position.Value.X,
							scrollable->HorizontalScroll->Maximum);
					}

					// Restore vertical scroll
					if (scrollable->VerticalScroll->Visible) {
						scrollable->VerticalScroll->Value = Math::Min(position.Value.Y,
							scrollable->VerticalScroll->Maximum);
					}
				}
			}
			catch (...) {
				// Silently continue if we can't restore a particular control's scroll position
				continue;
			}
		}
	}

	Rectangle Control_DropDown::Get_Tile_Bounds(int index) {
		int row = index / _Columns;
		int col = index % _Columns;
		return Rectangle(col * _Tile_Width, row * _Tile_Height, _Tile_Width, _Tile_Height);
	}

	void Control_DropDown::Set_Items(array<String^>^ first_lines, array<String^>^ second_lines, array<int>^ values) {
		_First_Lines = first_lines;
		_Second_Lines = second_lines;
		_Values = values;
		_First_Line_Colors = gcnew array<Color>(_First_Lines->Length);
		_Second_Line_Colors = gcnew array<Color>(_First_Lines->Length);

		// Initialize with default colors
		for (int i = 0; i < _First_Lines->Length; i++) {
			_First_Line_Colors[i] = Color::Black;
			_Second_Line_Colors[i] = Color::Black;
		}

		if (_Selected_Index == -1 && _First_Lines->Length > 0) {
			_Selected_Index = 0;
		}
		this->Invalidate();
	}

	void Control_DropDown::Set_First_Line_Color(int index, Color color) {
		if (index >= 0 && index < _First_Line_Colors->Length) {
			_First_Line_Colors[index] = color;
			this->Invalidate();
		}
	}

	void Control_DropDown::Set_Second_Line_Color(int index, Color color) {
		if (index >= 0 && index < _Second_Line_Colors->Length) {
			_Second_Line_Colors[index] = color;
			this->Invalidate();
		}
	}

	void Control_DropDown::Set_Tile_Layout(int tile_width, int tile_height, int columns) {
		_Tile_Width = tile_width;
		_Tile_Height = tile_height;
		_Columns = Math::Max(1, columns);  // Ensure at least 1 column

		// If dropdown is currently shown, update its size and position
		if (_Is_Dropped && _Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible) {
			int total_rows = Get_Row_Count();
			_Drop_Down_Panel->Size = System::Drawing::Size(_Tile_Width * _Columns, _Tile_Height * total_rows);
			Point location = this->Parent->PointToScreen(Point(this->Left, this->Bottom));
			location = this->FindForm()->PointToClient(location);
			_Drop_Down_Panel->Location = location;
		}

		this->Invalidate(); // Refresh the main control display
	}

	void Control_DropDown::Set_Open_Direction(bool open_above) {
		if (_Open_Above != open_above) {
			_Open_Above = open_above;
			if (_Is_Dropped && _Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible) {
				Update_Panel_Position();
			}
		}
	}

	void Control_DropDown::Set_Horizontal_Alignment(Panel_Horizontal_Alignment alignment) {
		if (_Horizontal_Alignment != alignment) {
			_Horizontal_Alignment = alignment;
			if (_Is_Dropped && _Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible) {
				Update_Panel_Position();
			}
		}
	}

	bool Control_DropDown::Select_Next() {
		if (_First_Lines == nullptr || _First_Lines->Length == 0) return false;
		if (_Selected_Index < _First_Lines->Length - 1) {
			Selected_Index = _Selected_Index + 1;
			return true;
		}
		return false;
	}

	bool Control_DropDown::Select_Previous() {
		if (_First_Lines == nullptr || _First_Lines->Length == 0) return false;
		if (_Selected_Index > 0) {
			Selected_Index = _Selected_Index - 1;
			return true;
		}
		return false;
	}

	bool Control_DropDown::Select_By_Value(int value) {
		int index = Find_Index_By_Value(value);
		if (index != -1) {
			Selected_Index = index;
			return true;
		}
		return false;
	}

	void Control_DropDown::Clear()
	{
		// Clear all arrays
		_First_Lines = nullptr;
		_Second_Lines = nullptr;
		_Values = nullptr;
		//_First_Line_Colors = nullptr;
		//_Second_Line_Colors = nullptr;

		// Reset selection and highlight
		_Selected_Index = -1;
		_Highlight_Index = -1;

		// Close dropdown if it's open
		if (_Is_Dropped) {
			Close_Drop_Down(nullptr, nullptr);
		}

		// Refresh the display
		this->Invalidate();
	}

	void Control_DropDown::OnPaint(PaintEventArgs^ e) {
		Graphics^ g = e->Graphics;

		// Draw 3D border like a ComboBox
		ControlPaint::DrawBorder3D(g, 0, 0, Width, Height, Border3DStyle::Flat);

		// Fixed title height
		const int title_height = 20;

		if (!String::IsNullOrEmpty(_Title_Text)) {
			// Draw title
			Rectangle title_rect(2, 2, Width - 4, title_height);  // Adjusted for border
			StringFormat^ format = gcnew StringFormat();
			format->Alignment = StringAlignment::Center;
			format->LineAlignment = StringAlignment::Center;
			g->DrawString(_Title_Text, this->Font, gcnew SolidBrush(_Title_Color), title_rect, format);
		}

		// Draw selected item immediately below title with adjusted margins for border
		if (_Selected_Index >= 0) {
			Rectangle content_rect = Rectangle(2, title_height, Width - 4, Height - title_height - 2);
			Draw_Item(g, _Selected_Index, content_rect, false, true);
		}

		// Draw arrow
		array<Point>^ arrow_points = gcnew array<Point>{
			Point(_Arrow_Bounds.Left, _Arrow_Bounds.Top),
				Point(_Arrow_Bounds.Right, _Arrow_Bounds.Top),
				Point(_Arrow_Bounds.Left + _Arrow_Bounds.Width / 2, _Arrow_Bounds.Bottom)
		};
		g->FillPolygon(Brushes::Black, arrow_points);
	}

	void Control_DropDown::Draw_Item(Graphics^ g, int index, Rectangle bounds, bool is_highlighted, bool is_main_control) {
		if (index < 0 || index >= _First_Lines->Length) return;

		// Draw highlight without spacing
		if (is_highlighted) {
			g->FillRectangle(Brushes::LightBlue, bounds);
			g->DrawRectangle(Pens::Red, bounds.X, bounds.Y, bounds.Width - 1, bounds.Height - 1);
		}

		// Create string format for center alignment
		StringFormat^ format = gcnew StringFormat();
		format->Alignment = StringAlignment::Center;

		if (is_main_control) {
			// For main control, combine both lines into one and align center
			format->LineAlignment = StringAlignment::Near;  // Align to top
			String^ combined_text;
			if (!String::IsNullOrEmpty(_Second_Lines[index])) {
				combined_text = _First_Lines[index] + " " + _Second_Lines[index];
			}
			else {
				combined_text = _First_Lines[index];
			}
			g->DrawString(combined_text, this->Font, gcnew SolidBrush(_First_Line_Colors[index]), bounds, format);
		}
		else {
			// Normal two-line drawing for dropdown panel
			Rectangle first_line_bounds;
			Rectangle second_line_bounds;

			if (String::IsNullOrEmpty(_Second_Lines[index])) {
				// Center single line vertically in the whole bounds
				first_line_bounds = bounds;
				format->LineAlignment = StringAlignment::Center;
				g->DrawString(_First_Lines[index], this->Font, gcnew SolidBrush(_First_Line_Colors[index]), first_line_bounds, format);
			}
			else {
				// Increased overlap between lines
				float first_line_height = bounds.Height * 0.55f;
				first_line_bounds = Rectangle(bounds.X, bounds.Y, bounds.Width, (int)first_line_height);
				// Move second line up more and increase overlap
				second_line_bounds = Rectangle(bounds.X, bounds.Y + (int)first_line_height - 12, bounds.Width, bounds.Height - (int)first_line_height + 12);

				format->LineAlignment = StringAlignment::Center;
				g->DrawString(_First_Lines[index], this->Font, gcnew SolidBrush(_First_Line_Colors[index]), first_line_bounds, format);

				Drawing::Font^ smaller_font = gcnew Drawing::Font(this->Font->FontFamily, this->Font->Size * 0.8f, this->Font->Style);
				g->DrawString(_Second_Lines[index], smaller_font, gcnew SolidBrush(_Second_Line_Colors[index]), second_line_bounds, format);
				delete smaller_font;
			}
		}
	}

	void Control_DropDown::Paint_Drop_Down(Object^ sender, PaintEventArgs^ e) {
		Graphics^ g = e->Graphics;

		for (int i = 0; i < _First_Lines->Length; i++) {
			Rectangle tile_bounds = Get_Tile_Bounds(i);

			// Adjust the bounds to account for the panel border
			if (tile_bounds.Right == _Drop_Down_Panel->Width - 1) {
				tile_bounds.Width += 1;
			}
			if (tile_bounds.Bottom == _Drop_Down_Panel->Height - 1) {
				tile_bounds.Height += 1;
			}

			Draw_Item(g, i, tile_bounds, i == _Highlight_Index, false);  // false for dropdown panel
		}
	}

	void Control_DropDown::OnMouseClick(MouseEventArgs^ e) {
		if (e->Button == Windows::Forms::MouseButtons::Left) {
			if (!_Is_Dropped) {
				Show_Drop_Down();
			}
			else {
				Close_Drop_Down(nullptr, nullptr);
			}
		}
		Control::OnMouseClick(e);
	}

	void Control_DropDown::Handle_Mouse_Move(Object^ sender, MouseEventArgs^ e) {
		if (!_Is_Dropped) return;

		int col = e->X / _Tile_Width;
		int row = e->Y / _Tile_Height;
		int index = (row * _Columns) + col;

		// Only redraw if we're hovering over a valid tile and the highlight has changed
		if (index != _Highlight_Index && index >= 0 && index < _First_Lines->Length) {
			_Highlight_Index = index;
			_Drop_Down_Panel->Invalidate();
		}
	}

	void Control_DropDown::Handle_Mouse_Click(Object^ sender, MouseEventArgs^ e) {
		if (_Highlight_Index >= 0 && _Highlight_Index < _First_Lines->Length) {
			Selected_Index = _Highlight_Index;

			// Raise the event with the index and value
			Control_DropDown_Item_Selected_Event_Args^ args =
				gcnew Control_DropDown_Item_Selected_Event_Args(_Selected_Index, _Values[_Selected_Index]);
			Item_Selected(this, args);

			Close_Drop_Down(nullptr, nullptr);
		}
	}

	void Control_DropDown::Close_Drop_Down(Object^ sender, EventArgs^ e) {
		if (_Drop_Down_Panel != nullptr) {
			Form^ parent_form = this->FindForm();
			if (parent_form != nullptr) {
				// Store scroll positions of all scrollable controls recursively
				Dictionary<ScrollableControl^, Point>^ scroll_positions = gcnew Dictionary<ScrollableControl^, Point>();
				Find_Scrollable_Controls(parent_form, scroll_positions);

				// Hide and remove dropdown
				_Drop_Down_Panel->Visible = false;
				parent_form->Click -= gcnew EventHandler(this, &Control_DropDown::Form_Click);

				// Restore scroll positions
				Restore_Scroll_Positions(scroll_positions);
			}
		}
		_Is_Dropped = false;
		_Highlight_Index = -1;
	}

	void Control_DropDown::OnSizeChanged(EventArgs^ e) {
		_Arrow_Bounds = Rectangle(Width - 15, Height - 15, 8, 6);
		Control::OnSizeChanged(e);
	}

	void Control_DropDown::Selected_Index::set(int value) {
		if (value >= -1 && value < _First_Lines->Length && value != _Selected_Index) {
			_Selected_Index = value;

			// Raise the event if this is a valid selection
			if (_Selected_Index >= 0) {
				Control_DropDown_Item_Selected_Event_Args^ args =
					gcnew Control_DropDown_Item_Selected_Event_Args(_Selected_Index, _Values[_Selected_Index]);
				Item_Selected(this, args);
			}

			this->Invalidate();
		}
	}
}
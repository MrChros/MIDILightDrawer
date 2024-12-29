#include "Control_DropDown.h"

namespace MIDILightDrawer
{
	Control_DropDown::Control_DropDown()
	{
		Initialize_Component();

		_Close_Delay_Ms = 1000;
	}

	void Control_DropDown::Initialize_Component()
	{
		_Selected_Index		= -1;
		_Is_Dropped			= false;
		_Highlight_Index	= -1;
		_Tile_Width			= 150;
		_Tile_Height		= 50;
		_Columns			= 3;

		_Title_Color			= Color::Black;
		_Open_Above				= false;	// Default to opening below
		_Horizontal_Alignment	= Panel_Horizontal_Alignment::Left;

		// Apply theme colors
		Apply_Theme();

		// Calculate optimal height
		int optimal_height = 20 + 20;	// Title Height + Content Height
		this->Size = System::Drawing::Size(200, optimal_height);
		this->BackColor = Color::White;

		// Initialize timer
		_Close_Timer = gcnew System::Windows::Forms::Timer();
		_Close_Timer->Tick += gcnew EventHandler(this, &Control_DropDown::Handle_Close_Timer_Tick);
		_Close_Timer->Interval = 1; // Will be set when used

		// Adjust arrow position for new height
		_Arrow_Bounds = Rectangle(Width - 20, (optimal_height - 8) / 2, 10, 8);

		_Drop_Down_Panel = gcnew Double_Buffered_Panel();
		_Drop_Down_Panel->Visible		= false;
		_Drop_Down_Panel->BorderStyle	= BorderStyle::None;

		_Drop_Down_Panel->Paint			+= gcnew PaintEventHandler(this, &Control_DropDown::Paint_Drop_Down);
		_Drop_Down_Panel->MouseMove		+= gcnew MouseEventHandler(this, &Control_DropDown::Handle_Mouse_Move);
		_Drop_Down_Panel->MouseClick	+= gcnew MouseEventHandler(this, &Control_DropDown::Handle_Mouse_Click);

		_Mouse_Leave_Handler = gcnew EventHandler(this, &Control_DropDown::Handle_Mouse_Leave);
	}

	void Control_DropDown::Show_Drop_Down()
	{
		if (_Drop_Down_Panel->Parent == nullptr && this->FindForm() != nullptr)
		{
			Form^ parent_form = this->FindForm();
			parent_form->Controls->Add(_Drop_Down_Panel);
			_Drop_Down_Panel->BringToFront();
		}

		int total_rows = Get_Row_Count();
		_Drop_Down_Panel->Size = System::Drawing::Size(_Tile_Width * _Columns + 3, _Tile_Height * total_rows + 3);

		Update_Panel_Position();

		_Drop_Down_Panel->BackColor = Color::White;
		_Drop_Down_Panel->BorderStyle = BorderStyle::FixedSingle;
		_Drop_Down_Panel->Visible = true;
		_Drop_Down_Panel->BringToFront();

		// Add mouse leave handlers
		this->MouseLeave += _Mouse_Leave_Handler;
		_Drop_Down_Panel->MouseLeave += _Mouse_Leave_Handler;

		_Is_Dropped = true;
	}
	
	void Control_DropDown::Close_Drop_Down(Object^ sender, EventArgs^ e)
	{
		// Stop the timer if it's running
		_Close_Timer->Stop();

		if (_Drop_Down_Panel != nullptr) {
			this->MouseLeave -= _Mouse_Leave_Handler;
			_Drop_Down_Panel->MouseLeave -= _Mouse_Leave_Handler;

			Form^ parent_form = this->FindForm();
			if (parent_form != nullptr)
			{
				_Drop_Down_Panel->Visible = false;
			}
		}
		_Is_Dropped = false;
		_Highlight_Index = -1;
	}

	void Control_DropDown::Paint_Drop_Down(Object^ sender, PaintEventArgs^ e)
	{
		Graphics^ g = e->Graphics;
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Draw panel background and border
		Rectangle bounds = _Drop_Down_Panel->ClientRectangle;

		// Fill background
		g->FillRectangle(gcnew SolidBrush(_Dropdown_Background), bounds);

		// Draw simple border
		g->DrawRectangle(gcnew Pen(_Dropdown_Border), 0, 0, bounds.Width - 1, bounds.Height - 1);

		// Draw items
		for (int i = 0; i < _First_Lines->Length; i++) {
			Rectangle tile_bounds = Get_Tile_Bounds(i);
			tile_bounds.Inflate(-2, -2);  // Add padding

			// Draw highlight for hovered item
			if (i == _Highlight_Index) {
				Rectangle highlight_bounds = tile_bounds;
				highlight_bounds.Inflate(-2, -2);
				g->FillRectangle(gcnew SolidBrush(_Hover_Color), highlight_bounds);
			}

			// Draw selection for selected item
			if (i == _Selected_Index) {
				Rectangle select_bounds = tile_bounds;
				select_bounds.Inflate(-2, -2);
				g->FillRectangle(gcnew SolidBrush(_Selected_Color), select_bounds);
			}

			Draw_Item(g, i, tile_bounds, i == _Highlight_Index, false);
		}
	}

	void Control_DropDown::Draw_Item(Graphics^ g, int index, Rectangle bounds, bool is_highlighted, bool is_main_control)
	{
		if (index < 0 || index >= _First_Lines->Length) return;

		// Create string format for center alignment
		StringFormat^ format = gcnew StringFormat();
		format->Alignment = StringAlignment::Center;

		if (is_main_control) {
			// For main control, combine both lines into one and align center
			format->LineAlignment = StringAlignment::Center;
			String^ combined_text;
			if (!String::IsNullOrEmpty(_Second_Lines[index])) {
				combined_text = _First_Lines[index] + " " + _Second_Lines[index];
			}
			else {
				combined_text = _First_Lines[index];
			}
			g->DrawString(combined_text, this->Font, gcnew SolidBrush(_Title_Color), bounds, format);
		}
		else
		{
			// Normal two-line drawing for dropdown panel
			Rectangle first_line_bounds;
			Rectangle second_line_bounds;

			if (String::IsNullOrEmpty(_Second_Lines[index]))
			{
				// Center single line vertically in the whole bounds
				first_line_bounds = bounds;
				format->LineAlignment = StringAlignment::Center;
				g->DrawString(_First_Lines[index], this->Font, gcnew SolidBrush(_Title_Color), first_line_bounds, format);
			}
			else
			{
				// Increased overlap between lines
				float first_line_height = bounds.Height * 0.55f;
				first_line_bounds = Rectangle(bounds.X, bounds.Y, bounds.Width, (int)first_line_height);
				// Move second line up more and increase overlap
				second_line_bounds = Rectangle(bounds.X, bounds.Y + (int)first_line_height - 12, bounds.Width, bounds.Height - (int)first_line_height + 12);

				format->LineAlignment = StringAlignment::Center;
				g->DrawString(_First_Lines[index], this->Font, gcnew SolidBrush(_Title_Color), first_line_bounds, format);

				Drawing::Font^ smaller_font = gcnew Drawing::Font(this->Font->FontFamily, this->Font->Size * 0.8f, this->Font->Style);
				g->DrawString(_Second_Lines[index], smaller_font, gcnew SolidBrush(_Title_Color), second_line_bounds, format);
				delete smaller_font;
			}
		}
	}

	void Control_DropDown::Handle_Mouse_Move(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Is_Dropped) return;

		int col = e->X / _Tile_Width;
		int row = e->Y / _Tile_Height;
		int index = (row * _Columns) + col;

		// Only redraw if we're hovering over a valid tile and the highlight has changed
		if (index != _Highlight_Index && index >= 0 && index < _First_Lines->Length)
		{
			_Highlight_Index = index;
			_Drop_Down_Panel->Invalidate();
		}
	}

	void Control_DropDown::Handle_Mouse_Click(Object^ sender, MouseEventArgs^ e)
	{
		if (_Highlight_Index >= 0 && _Highlight_Index < _First_Lines->Length)
		{
			Selected_Index = _Highlight_Index;

			// Raise the event with the index and value
			Control_DropDown_Item_Selected_Event_Args^ args =
				gcnew Control_DropDown_Item_Selected_Event_Args(_Selected_Index, _Values[_Selected_Index]);
			Item_Selected(this, args);

			Close_Drop_Down(nullptr, nullptr);
		}
	}

	void Control_DropDown::Handle_Mouse_Leave(Object^ sender, EventArgs^ e)
	{
		Application::DoEvents();

		if (!Is_Mouse_Over_Controls())
		{
			if (_Close_Delay_Ms > 0)
			{
				// Stop any existing timer
				_Close_Timer->Stop();

				// Set the interval and start the timer
				_Close_Timer->Interval = _Close_Delay_Ms;
				_Close_Timer->Start();
			}
			else
			{
				// No delay, close immediately
				Close_Drop_Down(nullptr, nullptr);
			}
		}
	}

	void Control_DropDown::Handle_Close_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		_Close_Timer->Stop();
		if (!Is_Mouse_Over_Controls())
		{
			Close_Drop_Down(nullptr, nullptr);
		}
	}

	bool Control_DropDown::Is_Mouse_Over_Controls()
	{
		Point mouse_pos = Control::MousePosition;

		// Get screen bounds for both controls
		Rectangle control_bounds = Get_Control_Screen_Bounds();
		Rectangle panel_bounds = Get_Panel_Screen_Bounds();

		// Check if mouse is over either control in screen coordinates
		return (control_bounds.Contains(mouse_pos) ||
			(_Drop_Down_Panel->Visible && panel_bounds.Contains(mouse_pos)));
	}
	
	void Control_DropDown::Update_Panel_Position()
	{
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
			int panel_height = _Tile_Height * total_rows + 3 + 1;
			// Position above the control
			location = this->Parent->PointToScreen(Point(x_position, this->Top - panel_height));
		}
		else {
			// Position below the control
			location = this->Parent->PointToScreen(Point(x_position, this->Bottom + 1));
		}

		location = this->FindForm()->PointToClient(location);
		_Drop_Down_Panel->Location = location;
	}

	int Control_DropDown::Get_Row_Count()
	{
		if (_First_Lines == nullptr) return 0;
		return (int)Math::Ceiling((double)_First_Lines->Length / _Columns);
	}

	int Control_DropDown::Find_Index_By_Value(int value)
	{
		if (_Values == nullptr) return -1;
		for (int i = 0; i < _Values->Length; i++) {
			if (_Values[i] == value) return i;
		}
		return -1;
	}

	Rectangle Control_DropDown::Get_Control_Screen_Bounds()
	{
		// Get the control's bounds in screen coordinates
		return this->RectangleToScreen(this->ClientRectangle);
	}

	Rectangle Control_DropDown::Get_Panel_Screen_Bounds()
	{
		if (_Drop_Down_Panel == nullptr || !_Drop_Down_Panel->Visible)
			return Rectangle::Empty;

		// Get the panel's bounds in screen coordinates
		return _Drop_Down_Panel->RectangleToScreen(_Drop_Down_Panel->ClientRectangle);
	}

	Rectangle Control_DropDown::Get_Tile_Bounds(int index)
	{
		//const int panel_padding = 10;
		
		int row = index / _Columns;
		int col = index % _Columns;

		//int effective_tile_width = (_Drop_Down_Panel->Width - (2 * panel_padding)) / _Columns;

		return Rectangle(col * _Tile_Width, row * _Tile_Height, _Tile_Width, _Tile_Height);
	}

	void Control_DropDown::Apply_Theme()
	{
		auto theme = Theme_Manager::Get_Instance();
		_Background_Color	= theme->BackgroundLight;
		_Border_Color		= theme->BorderStrong;
		_Hover_Color		= theme->AccentPrimary;

		_Selected_Color = Color::FromArgb(80, (int)theme->AccentPrimary.R, (int)theme->AccentPrimary.G, (int)theme->AccentPrimary.B);
		
		_Dropdown_Background	= theme->Background;	// Darker background for dropdown
		_Dropdown_Border		= theme->BorderPrimary;
		_Title_Color			= theme->ForegroundText;

		if (_Drop_Down_Panel != nullptr) {
			_Drop_Down_Panel->BackColor = _Dropdown_Background;
		}
		this->BackColor = _Background_Color;
		this->ForeColor = _Title_Color;
	}


	///////////////////////
	// Protected Methods //
	///////////////////////
	void Control_DropDown::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ g = e->Graphics;
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		Rectangle bounds = this->ClientRectangle;

		// Fill background
		g->FillRectangle(gcnew SolidBrush(_Background_Color), bounds);

		// Draw simple border
		g->DrawRectangle(gcnew Pen(_Border_Color), 0, 0, Width - 1, Height - 1);

		// Draw title text
		if (!String::IsNullOrEmpty(_Title_Text))
		{
			Rectangle title_rect(8, 2, Width - 30, Height / 2);
			StringFormat^ title_format = gcnew StringFormat();
			title_format->Alignment = StringAlignment::Center;
			title_format->LineAlignment = StringAlignment::Center;
			g->DrawString(_Title_Text, this->Font, gcnew SolidBrush(_Title_Color), title_rect, title_format);
		}

		// Draw selected item
		if (_Selected_Index >= 0)
		{
			Rectangle value_rect = Rectangle(8, Height / 2, Width - 30, Height / 2);
			Draw_Item(g, _Selected_Index, value_rect, false, true);
		}

		// Draw arrow based on _Open_Above
		array<Point>^ Arrow_Points;
		if (_Open_Above)
		{
			// Draw upward arrow with same distance from top as downward arrow is from bottom
			int Arrow_Height = 6;	// Height of the arrow
			int Border_Distance = 7;	// Distance from border (matching _Arrow_Bounds bottom distance)

			Arrow_Points = gcnew array<Point> {
				Point(Width - 15, Border_Distance + Arrow_Height),	// Bottom point
					Point(Width - 11, Border_Distance),					// Top left
					Point(Width - 7, Border_Distance + Arrow_Height)	// Bottom right
			};
		}
		else {
			// Draw downward arrow
			Arrow_Points = gcnew array<Point> {
				Point(_Arrow_Bounds.Left, _Arrow_Bounds.Top),
					Point(_Arrow_Bounds.Right, _Arrow_Bounds.Top),
					Point(_Arrow_Bounds.Left + _Arrow_Bounds.Width / 2, _Arrow_Bounds.Bottom)
			};
		}

		g->FillPolygon(gcnew SolidBrush(_Title_Color), Arrow_Points);
	}

	void Control_DropDown::OnMouseClick(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left)
		{
			if (!_Is_Dropped) {
				Show_Drop_Down();
			}
			else {
				Close_Drop_Down(nullptr, nullptr);
			}
		}
		Control::OnMouseClick(e);
	}

	void Control_DropDown::OnSizeChanged(EventArgs^ e)
	{
		_Arrow_Bounds = Rectangle(Width - 15, Height - 15, 8, 6);
		Control::OnSizeChanged(e);
	}



	////////////////////
	// Public Methods //
	////////////////////
	void Control_DropDown::Set_Title_Color(Color color)
	{
		if (_Title_Color != color)
		{
			_Title_Color = color;
			this->Invalidate(); // Redraw with new color
		}
	}

	void Control_DropDown::Set_Items(array<String^>^ first_lines, array<String^>^ second_lines, array<int>^ values)
	{
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

	void Control_DropDown::Set_First_Line_Color(int index, Color color)
	{
		if (index >= 0 && index < _First_Line_Colors->Length) {
			_First_Line_Colors[index] = color;
			this->Invalidate();
		}
	}

	void Control_DropDown::Set_Second_Line_Color(int index, Color color)
	{
		if (index >= 0 && index < _Second_Line_Colors->Length) {
			_Second_Line_Colors[index] = color;
			this->Invalidate();
		}
	}

	void Control_DropDown::Set_Tile_Layout(int tile_width, int tile_height, int columns)
	{
		_Tile_Width = tile_width;
		_Tile_Height = tile_height;
		_Columns = Math::Max(1, columns);  // Ensure at least 1 column

		// If dropdown is currently shown, update its size and position
		if (_Is_Dropped && _Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible)
		{
			int total_rows = Get_Row_Count();
			_Drop_Down_Panel->Size = System::Drawing::Size(_Tile_Width * _Columns, _Tile_Height * total_rows);
			Point location = this->Parent->PointToScreen(Point(this->Left, this->Bottom));
			location = this->FindForm()->PointToClient(location);
			_Drop_Down_Panel->Location = location;
		}

		this->Invalidate(); // Refresh the main control display
	}

	void Control_DropDown::Set_Open_Direction(bool open_above)
	{
		if (_Open_Above != open_above) {
			_Open_Above = open_above;
			if (_Is_Dropped && _Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible) {
				Update_Panel_Position();
			}
		}
	}

	void Control_DropDown::Set_Horizontal_Alignment(Panel_Horizontal_Alignment alignment)
	{
		if (_Horizontal_Alignment != alignment) {
			_Horizontal_Alignment = alignment;
			if (_Is_Dropped && _Drop_Down_Panel != nullptr && _Drop_Down_Panel->Visible) {
				Update_Panel_Position();
			}
		}
	}

	bool Control_DropDown::Select_Next()
	{
		if (_First_Lines == nullptr || _First_Lines->Length == 0) return false;
		if (_Selected_Index < _First_Lines->Length - 1) {
			Selected_Index = _Selected_Index + 1;
			return true;
		}
		return false;
	}

	bool Control_DropDown::Select_Previous()
	{
		if (_First_Lines == nullptr || _First_Lines->Length == 0) return false;
		if (_Selected_Index > 0) {
			Selected_Index = _Selected_Index - 1;
			return true;
		}
		return false;
	}

	bool Control_DropDown::Select_By_Value(int value)
	{
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
		_First_Lines	= nullptr;
		_Second_Lines	= nullptr;
		_Values			= nullptr;
		//_First_Line_Colors = nullptr;
		//_Second_Line_Colors = nullptr;

		// Reset selection and highlight
		_Selected_Index		= -1;
		_Highlight_Index	= -1;

		// Close dropdown if it's open
		if (_Is_Dropped) {
			Close_Drop_Down(nullptr, nullptr);
		}

		// Refresh the display
		this->Invalidate();
	}

	void Control_DropDown::Selected_Index::set(int value)
	{
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
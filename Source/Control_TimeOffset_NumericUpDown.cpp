#include "Control_TimeOffset_NumericUpDown.h"

namespace MIDILightDrawer
{
	Control_TimeOffset_NumericUpDown::Control_TimeOffset_NumericUpDown()
	{
		_Total_Milliseconds = 0.0;
		_Minimum_ms = -60000.0;
		_Maximum_ms = 60000.0;
		_Is_Editing = false;
		_Is_Hovering = false;
		_Is_Dragging_Value = false;
		_Drag_Start_X = 0;
		_Drag_Start_Value = 0.0;
		_Is_Hovering_Decrement = false;
		_Is_Hovering_Increment = false;
		_Is_Pressing_Decrement = false;
		_Is_Pressing_Increment = false;

		Initialize_Components();
		Update_Layout();
	}

	Control_TimeOffset_NumericUpDown::~Control_TimeOffset_NumericUpDown()
	{
		if (_Repeat_Timer != nullptr) {
			_Repeat_Timer->Stop();
			delete _Repeat_Timer;
		}
	}

	void Control_TimeOffset_NumericUpDown::Initialize_Components()
	{
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::ResizeRedraw |
			ControlStyles::Selectable, true);

		this->Size = System::Drawing::Size(180, CONTROL_HEIGHT);
		this->MinimumSize = System::Drawing::Size(120, CONTROL_HEIGHT);
		this->MaximumSize = System::Drawing::Size(300, CONTROL_HEIGHT);
		this->BackColor = Color::Transparent;
		this->Cursor = Cursors::Default;

		// Create hidden textbox for editing
		_TextBox_Input = gcnew TextBox();
		_TextBox_Input->Visible = false;
		_TextBox_Input->BorderStyle = System::Windows::Forms::BorderStyle::None;
		_TextBox_Input->TextAlign = HorizontalAlignment::Center;
		_TextBox_Input->Font = gcnew System::Drawing::Font("Consolas", 10.0f, FontStyle::Bold);
		_TextBox_Input->KeyPress += gcnew KeyPressEventHandler(this, &Control_TimeOffset_NumericUpDown::On_TextBox_KeyPress);
		_TextBox_Input->KeyDown += gcnew KeyEventHandler(this, &Control_TimeOffset_NumericUpDown::On_TextBox_KeyDown);
		_TextBox_Input->LostFocus += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_TextBox_LostFocus);
		this->Controls->Add(_TextBox_Input);

		// Setup repeat timer for held buttons
		_Repeat_Timer = gcnew Timer();
		_Repeat_Timer->Interval = 50;
		_Repeat_Timer->Tick += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Repeat_Timer_Tick);
	}

	void Control_TimeOffset_NumericUpDown::Update_Layout()
	{
		int Height_Val = this->Height;
		int Width_Val = this->Width;

		_Rect_Decrement = Rectangle(0, 0, BUTTON_WIDTH, Height_Val);
		_Rect_Display = Rectangle(BUTTON_WIDTH, 0, Width_Val - BUTTON_WIDTH * 2, Height_Val);
		_Rect_Increment = Rectangle(Width_Val - BUTTON_WIDTH, 0, BUTTON_WIDTH, Height_Val);

		// Update textbox position
		if(_TextBox_Input) {
			Theme_Manager^ Theme = Theme_Manager::Get_Instance();
			_TextBox_Input->BackColor = Theme->BackgroundAlt;
			_TextBox_Input->ForeColor = Color::White;
			_TextBox_Input->Location = Point(_Rect_Display.X + 4, (_Rect_Display.Height - _TextBox_Input->Height) / 2);
			_TextBox_Input->Width = _Rect_Display.Width - 8;
		}
	}

	void Control_TimeOffset_NumericUpDown::Value_ms::set(double value)
	{
		// Clamp to min/max
		if (value < _Minimum_ms) value = _Minimum_ms;
		if (value > _Maximum_ms) value = _Maximum_ms;

		// Round to nearest integer
		value = Math::Round(value);

		if (Math::Abs(_Total_Milliseconds - value) < 0.001) {
			return; // No change
		}

		_Total_Milliseconds = value;
		Invalidate();

		Value_Changed(this, EventArgs::Empty);
	}

	String^ Control_TimeOffset_NumericUpDown::Format_Time(double milliseconds)
	{
		bool Is_Negative = milliseconds < 0;
		double Abs_Ms = Math::Abs(milliseconds);

		String^ Sign = Is_Negative ? "-" : "+";

		// Format based on magnitude
		//if (Abs_Ms >= 1000.0) {
		//	double Seconds = Abs_Ms / 1000.0;
		//	return String::Format("{0}{1:F0} ms ({2:F2}s)", Sign, Abs_Ms, Seconds);
		//}
		//else {
		//	return String::Format("{0}{1:F0} ms", Sign, Abs_Ms);
		//}

		return String::Format("{0}{1:F0} ms", Sign, Abs_Ms);
	}

	double Control_TimeOffset_NumericUpDown::Parse_Time_Input(String^ input)
	{
		input = input->Trim();

		// Remove content in parentheses
		int Paren_Start = input->IndexOf('(');
		if (Paren_Start >= 0) {
			input = input->Substring(0, Paren_Start)->Trim();
		}

		// Remove units
		input = input->Replace("ms", "")->Replace("s", "")->Trim();

		bool Is_Negative = input->StartsWith("-");
		if (Is_Negative) {
			input = input->Substring(1)->Trim();
		}
		else if (input->StartsWith("+")) {
			input = input->Substring(1)->Trim();
		}

		// Parse as number
		double Result = 0;
		if (!Double::TryParse(input, Result)) {
			return _Total_Milliseconds;
		}

		if (Is_Negative) {
			Result = -Result;
		}

		// Clamp
		if (Result < _Minimum_ms) Result = _Minimum_ms;
		if (Result > _Maximum_ms) Result = _Maximum_ms;

		return Result;
	}

	void Control_TimeOffset_NumericUpDown::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ g = e->Graphics;
		g->SmoothingMode = SmoothingMode::AntiAlias;
		g->PixelOffsetMode = PixelOffsetMode::HighQuality;
		g->TextRenderingHint = System::Drawing::Text::TextRenderingHint::ClearTypeGridFit;

		// Clear background
		if (Parent != nullptr) {
			g->Clear(Parent->BackColor);
		}
		else {
			g->Clear(Color::Transparent);
		}

		// Draw components
		Draw_Button(g, _Rect_Decrement, false, _Is_Hovering_Decrement, _Is_Pressing_Decrement);
		Draw_Display(g, _Rect_Display);
		Draw_Button(g, _Rect_Increment, true, _Is_Hovering_Increment, _Is_Pressing_Increment);
	}

	void Control_TimeOffset_NumericUpDown::Draw_Button(Graphics^ g, Rectangle rect, bool is_increment, bool is_hovering, bool is_pressing)
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		// Determine colors based on state
		Color Bg_Color;
		if (is_pressing) {
			Bg_Color = Theme->AccentPrimary;
		}
		else if (is_hovering) {
			Bg_Color = Color::FromArgb(70, 70, 75);
		}
		else {
			Bg_Color = Theme->BackgroundAlt;
		}

		Color Border_Color = is_hovering || is_pressing ? Theme->AccentPrimary : Theme->BorderStrong;
		Color Symbol_Color = is_pressing ? Color::White : (is_hovering ? Theme->AccentSecondary : Color::FromArgb(180, 180, 180));

		// Create path for rounded corners on appropriate side
		GraphicsPath^ Path;
		if (is_increment) {
			Path = Create_Right_Rounded_Rect(rect, CORNER_RADIUS);
		}
		else {
			Path = Create_Left_Rounded_Rect(rect, CORNER_RADIUS);
		}

		// Fill background
		SolidBrush^ Bg_Brush = gcnew SolidBrush(Bg_Color);
		g->FillPath(Bg_Brush, Path);
		delete Bg_Brush;

		// Draw border
		Pen^ Border_Pen = gcnew Pen(Border_Color, 1.0f);
		g->DrawPath(Border_Pen, Path);
		delete Border_Pen;

		// Draw symbol (+/-)
		int Center_X = rect.X + rect.Width / 2;
		int Center_Y = rect.Y + rect.Height / 2;
		int Symbol_Size = 8;

		Pen^ Symbol_Pen = gcnew Pen(Symbol_Color, 2.0f);
		Symbol_Pen->StartCap = LineCap::Round;
		Symbol_Pen->EndCap = LineCap::Round;

		// Horizontal line (for both + and -)
		g->DrawLine(Symbol_Pen, Center_X - Symbol_Size / 2, Center_Y, Center_X + Symbol_Size / 2, Center_Y);

		// Vertical line (only for +)
		if (is_increment) {
			g->DrawLine(Symbol_Pen, Center_X, Center_Y - Symbol_Size / 2, Center_X, Center_Y + Symbol_Size / 2);
		}

		delete Symbol_Pen;
		delete Path;
	}

	void Control_TimeOffset_NumericUpDown::Draw_Display(Graphics^ g, Rectangle rect)
	{
		if (_Is_Editing) {
			return; // TextBox is visible, don't draw
		}

		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		// Background
		Color Bg_Color = Theme->BackgroundAlt;
		if (_Is_Dragging_Value) {
			Bg_Color = Color::FromArgb(50, 50, 55);
		}
		else if (_Is_Hovering && _Rect_Display.Contains(PointToClient(MousePosition))) {
			Bg_Color = Color::FromArgb(45, 45, 50);
		}

		SolidBrush^ Bg_Brush = gcnew SolidBrush(Bg_Color);
		g->FillRectangle(Bg_Brush, rect);
		delete Bg_Brush;

		// Border
		Pen^ Border_Pen = gcnew Pen(Theme->BorderStrong, 1.0f);
		g->DrawRectangle(Border_Pen, rect.X, rect.Y, rect.Width - 1, rect.Height - 1);
		delete Border_Pen;

		// Determine text color based on value
		Color Text_Color;
		if (_Total_Milliseconds > 0) {
			Text_Color = Color::FromArgb(100, 200, 100);  // Green for positive
		}
		else if (_Total_Milliseconds < 0) {
			Text_Color = Color::FromArgb(220, 100, 100);  // Red for negative
		}
		else {
			Text_Color = Color::FromArgb(180, 180, 180);  // Gray for zero
		}

		// Draw value text
		String^ Display_Text = Format_Time(_Total_Milliseconds);
		System::Drawing::Font^ Display_Font = gcnew System::Drawing::Font("Consolas", 10.0f, FontStyle::Bold);

		StringFormat^ Sf = gcnew StringFormat();
		Sf->Alignment = StringAlignment::Center;
		Sf->LineAlignment = StringAlignment::Center;

		SolidBrush^ Text_Brush = gcnew SolidBrush(Text_Color);
		g->DrawString(Display_Text, Display_Font, Text_Brush, RectangleF((float)rect.X, (float)rect.Y, (float)rect.Width, (float)rect.Height), Sf);

		delete Text_Brush;
		delete Display_Font;
		delete Sf;

		// Draw drag hint indicator when hovering
		if (_Is_Hovering && !_Is_Dragging_Value && _Rect_Display.Contains(PointToClient(MousePosition))) {
			// Small horizontal arrows on sides
			Pen^ Arrow_Pen = gcnew Pen(Color::FromArgb(80, 255, 255, 255), 1.0f);
			int Arrow_Y = rect.Y + rect.Height / 2;

			// Left arrow
			g->DrawLine(Arrow_Pen, rect.X + 4, Arrow_Y, rect.X + 8, Arrow_Y - 3);
			g->DrawLine(Arrow_Pen, rect.X + 4, Arrow_Y, rect.X + 8, Arrow_Y + 3);

			// Right arrow
			g->DrawLine(Arrow_Pen, rect.Right - 5, Arrow_Y, rect.Right - 9, Arrow_Y - 3);
			g->DrawLine(Arrow_Pen, rect.Right - 5, Arrow_Y, rect.Right - 9, Arrow_Y + 3);

			delete Arrow_Pen;
		}
	}

	GraphicsPath^ Control_TimeOffset_NumericUpDown::Create_Rounded_Rect(Rectangle rect, int radius)
	{
		GraphicsPath^ Path = gcnew GraphicsPath();
		int Diameter = radius * 2;

		Path->AddArc(rect.X, rect.Y, Diameter, Diameter, 180, 90);
		Path->AddArc(rect.Right - Diameter, rect.Y, Diameter, Diameter, 270, 90);
		Path->AddArc(rect.Right - Diameter, rect.Bottom - Diameter, Diameter, Diameter, 0, 90);
		Path->AddArc(rect.X, rect.Bottom - Diameter, Diameter, Diameter, 90, 90);
		Path->CloseFigure();

		return Path;
	}

	GraphicsPath^ Control_TimeOffset_NumericUpDown::Create_Left_Rounded_Rect(Rectangle rect, int radius)
	{
		GraphicsPath^ Path = gcnew GraphicsPath();
		int Diameter = radius * 2;

		Path->AddArc(rect.X, rect.Y, Diameter, Diameter, 180, 90);
		Path->AddLine(rect.Right, rect.Y, rect.Right, rect.Bottom);
		Path->AddArc(rect.X, rect.Bottom - Diameter, Diameter, Diameter, 90, 90);
		Path->CloseFigure();

		return Path;
	}

	GraphicsPath^ Control_TimeOffset_NumericUpDown::Create_Right_Rounded_Rect(Rectangle rect, int radius)
	{
		GraphicsPath^ Path = gcnew GraphicsPath();
		int Diameter = radius * 2;

		Path->AddLine(rect.X, rect.Y, rect.Right - radius, rect.Y);
		Path->AddArc(rect.Right - Diameter, rect.Y, Diameter, Diameter, 270, 90);
		Path->AddArc(rect.Right - Diameter, rect.Bottom - Diameter, Diameter, Diameter, 0, 90);
		Path->AddLine(rect.Right - radius, rect.Bottom, rect.X, rect.Bottom);
		Path->CloseFigure();

		return Path;
	}

	void Control_TimeOffset_NumericUpDown::OnMouseDown(MouseEventArgs^ e)
	{
		UserControl::OnMouseDown(e);

		if (e->Button != System::Windows::Forms::MouseButtons::Left) {
			return;
		}

		this->Focus();

		if (_Rect_Decrement.Contains(e->Location)) {
			_Is_Pressing_Decrement = true;
			_Repeat_Increment = false;
			Decrement_Value(1);
			_Repeat_Timer->Interval = 400;  // Initial delay
			_Repeat_Timer->Start();
			Invalidate();
		}
		else if (_Rect_Increment.Contains(e->Location)) {
			_Is_Pressing_Increment = true;
			_Repeat_Increment = true;
			Increment_Value(1);
			_Repeat_Timer->Interval = 400;  // Initial delay
			_Repeat_Timer->Start();
			Invalidate();
		}
		else if (_Rect_Display.Contains(e->Location)) {
			// Double-click to edit
			// For now, start drag mode
			_Is_Dragging_Value = true;
			_Drag_Start_X = e->X;
			_Drag_Start_Value = _Total_Milliseconds;
			this->Cursor = Cursors::SizeWE;
			Invalidate();
		}
	}

	void Control_TimeOffset_NumericUpDown::OnMouseUp(MouseEventArgs^ e)
	{
		UserControl::OnMouseUp(e);

		_Repeat_Timer->Stop();

		bool Was_Pressing = _Is_Pressing_Decrement || _Is_Pressing_Increment;
		_Is_Pressing_Decrement = false;
		_Is_Pressing_Increment = false;

		if (_Is_Dragging_Value) {
			// Check if it was just a click (minimal movement) -> enter edit mode
			int Delta_X = Math::Abs(e->X - _Drag_Start_X);
			if (Delta_X < 3) {
				Enter_Edit_Mode();
			}
			_Is_Dragging_Value = false;
			this->Cursor = Cursors::Default;
		}

		if (Was_Pressing) {
			Invalidate();
		}
	}

	void Control_TimeOffset_NumericUpDown::OnMouseMove(MouseEventArgs^ e)
	{
		UserControl::OnMouseMove(e);

		if (_Is_Dragging_Value) {
			int Delta_X = e->X - _Drag_Start_X;
			double Delta_Ms = (double)Delta_X / DRAG_STEP_PIXELS;
			Value_ms = _Drag_Start_Value + Delta_Ms;
		}
		else {
			// Update hover states
			bool Was_Hovering_Dec = _Is_Hovering_Decrement;
			bool Was_Hovering_Inc = _Is_Hovering_Increment;

			_Is_Hovering_Decrement = _Rect_Decrement.Contains(e->Location);
			_Is_Hovering_Increment = _Rect_Increment.Contains(e->Location);

			// Update cursor for display area
			if (_Rect_Display.Contains(e->Location)) {
				this->Cursor = Cursors::IBeam;
			}
			else if (_Rect_Decrement.Contains(e->Location) || _Rect_Increment.Contains(e->Location)) {
				this->Cursor = Cursors::Hand;
			}
			else {
				this->Cursor = Cursors::Default;
			}

			if (Was_Hovering_Dec != _Is_Hovering_Decrement || Was_Hovering_Inc != _Is_Hovering_Increment) {
				Invalidate();
			}
		}
	}

	void Control_TimeOffset_NumericUpDown::OnMouseEnter(EventArgs^ e)
	{
		UserControl::OnMouseEnter(e);
		_Is_Hovering = true;
		Invalidate();
	}

	void Control_TimeOffset_NumericUpDown::OnMouseLeave(EventArgs^ e)
	{
		UserControl::OnMouseLeave(e);
		_Is_Hovering = false;
		_Is_Hovering_Decrement = false;
		_Is_Hovering_Increment = false;
		this->Cursor = Cursors::Default;
		Invalidate();
	}

	void Control_TimeOffset_NumericUpDown::OnMouseWheel(MouseEventArgs^ e)
	{
		UserControl::OnMouseWheel(e);

		// Each wheel notch is typically 120 delta
		int Steps = e->Delta / 120;

		if (Steps > 0) {
			Increment_Value(Steps);
		}
		else if (Steps < 0) {
			Decrement_Value(-Steps);
		}
	}

	void Control_TimeOffset_NumericUpDown::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);
		Update_Layout();
	}

	void Control_TimeOffset_NumericUpDown::On_TextBox_KeyPress(Object^ sender, KeyPressEventArgs^ e)
	{
		// Allow digits, backspace, minus, plus, decimal point
		if (!Char::IsDigit(e->KeyChar) &&
			e->KeyChar != (Char)Keys::Back &&
			e->KeyChar != '-' &&
			e->KeyChar != '+' &&
			e->KeyChar != '.') {
			e->Handled = true;
		}

		// Only allow +/- at the beginning
		if ((e->KeyChar == '-' || e->KeyChar == '+') && _TextBox_Input->SelectionStart != 0) {
			e->Handled = true;
		}
	}

	void Control_TimeOffset_NumericUpDown::On_TextBox_KeyDown(Object^ sender, KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Enter || e->KeyCode == Keys::Return) {
			e->Handled = true;
			e->SuppressKeyPress = true;
			Exit_Edit_Mode(true);
		}
		else if (e->KeyCode == Keys::Escape) {
			e->Handled = true;
			e->SuppressKeyPress = true;
			Exit_Edit_Mode(false);
		}
		else if (e->KeyCode == Keys::Up) {
			Increment_Value(1);
			e->Handled = true;
		}
		else if (e->KeyCode == Keys::Down) {
			Decrement_Value(1);
			e->Handled = true;
		}
	}

	void Control_TimeOffset_NumericUpDown::On_TextBox_LostFocus(Object^ sender, EventArgs^ e)
	{
		if (_Is_Editing) {
			Exit_Edit_Mode(true);
		}
	}

	void Control_TimeOffset_NumericUpDown::On_Repeat_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		// Speed up after initial delay
		if (_Repeat_Timer->Interval > 50) {
			_Repeat_Timer->Interval = 50;
		}

		if (_Repeat_Increment && _Is_Pressing_Increment) {
			Increment_Value(1);
		}
		else if (!_Repeat_Increment && _Is_Pressing_Decrement) {
			Decrement_Value(1);
		}
		else {
			_Repeat_Timer->Stop();
		}
	}

	void Control_TimeOffset_NumericUpDown::Enter_Edit_Mode()
	{
		if (_Is_Editing) return;

		_Is_Editing = true;

		// Prepare textbox
		bool Is_Negative = _Total_Milliseconds < 0;
		double Abs_Ms = Math::Abs(_Total_Milliseconds);
		String^ Sign = Is_Negative ? "-" : "+";
		_TextBox_Input->Text = String::Format("{0}{1:F0}", Sign, Abs_Ms);

		_TextBox_Input->Visible = true;
		_TextBox_Input->Focus();
		_TextBox_Input->SelectAll();

		Invalidate();
	}

	void Control_TimeOffset_NumericUpDown::Exit_Edit_Mode(bool apply_changes)
	{
		if (!_Is_Editing) return;

		_Is_Editing = false;

		if (apply_changes) {
			Value_ms = Parse_Time_Input(_TextBox_Input->Text);
		}

		_TextBox_Input->Visible = false;

		// Move focus to parent or next control to properly exit edit mode
		if (this->Parent != nullptr) {
			this->Parent->Focus();

			// If parent can't take focus, try to select next control
			if (!this->Parent->Focused) {
				Control^ Next_Control = this->Parent->GetNextControl(this, true);
				if (Next_Control != nullptr) {
					Next_Control->Focus();
				}
			}
		}

		Invalidate();
	}

	void Control_TimeOffset_NumericUpDown::Increment_Value(int steps)
	{
		Value_ms = _Total_Milliseconds + (STEP_SIZE_MS * steps);
	}

	void Control_TimeOffset_NumericUpDown::Decrement_Value(int steps)
	{
		Value_ms = _Total_Milliseconds - (STEP_SIZE_MS * steps);
	}
}
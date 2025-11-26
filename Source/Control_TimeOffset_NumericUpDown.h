#pragma once

#include "Theme_Manager.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;

namespace MIDILightDrawer
{
	/// <summary>
	/// Modern time offset control with ms format
	/// Features: Click to edit, +/- buttons, mouse wheel support, drag-to-adjust
	/// Color-coded display (green for positive, red for negative)
	/// </summary>
	public ref class Control_TimeOffset_NumericUpDown : UserControl
	{
	private:
		// Value tracking
		double _Total_Milliseconds;
		double _Minimum_ms;
		double _Maximum_ms;
		bool _Is_Editing;

		// Interaction states
		bool _Is_Hovering;
		bool _Is_Dragging_Value;
		int _Drag_Start_X;
		double _Drag_Start_Value;
		bool _Is_Hovering_Decrement;
		bool _Is_Hovering_Increment;
		bool _Is_Pressing_Decrement;
		bool _Is_Pressing_Increment;

		// Edit mode
		TextBox^ _TextBox_Input;

		// Layout rectangles
		Rectangle _Rect_Decrement;
		Rectangle _Rect_Display;
		Rectangle _Rect_Increment;

		// Constants
		literal int STEP_SIZE_MS = 10;
		literal int DRAG_STEP_PIXELS = 2;		// Pixels of mouse movement per ms change
		literal int BUTTON_WIDTH = 28;
		literal int CONTROL_HEIGHT = 28;
		literal int CORNER_RADIUS = 4;

		// Repeat timer for held buttons
		Timer^ _Repeat_Timer;
		bool _Repeat_Increment;

	public:
		Control_TimeOffset_NumericUpDown();
		~Control_TimeOffset_NumericUpDown();

		// Events
		event EventHandler^ Value_Changed;

		// Properties
		property double Value_ms
		{
			double get() { return _Total_Milliseconds; }
			void set(double value);
		}

			property double Minimum_ms
		{
			double get() { return _Minimum_ms; }
			void set(double value) { _Minimum_ms = value; Invalidate(); }
		}

			property double Maximum_ms
		{
			double get() { return _Maximum_ms; }
			void set(double value) { _Maximum_ms = value; Invalidate(); }
		}

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseEnter(EventArgs^ e) override;
		virtual void OnMouseLeave(EventArgs^ e) override;
		virtual void OnMouseWheel(MouseEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;

	private:
		void Initialize_Components();
		void Update_Layout();
		String^ Format_Time(double milliseconds);
		double Parse_Time_Input(String^ input);

		// Drawing helpers
		void Draw_Button(Graphics^ g, Rectangle rect, bool is_increment, bool is_hovering, bool is_pressing);
		void Draw_Display(Graphics^ g, Rectangle rect);
		GraphicsPath^ Create_Rounded_Rect(Rectangle rect, int radius);
		GraphicsPath^ Create_Left_Rounded_Rect(Rectangle rect, int radius);
		GraphicsPath^ Create_Right_Rounded_Rect(Rectangle rect, int radius);

		// Event handlers
		void On_TextBox_KeyPress(Object^ sender, KeyPressEventArgs^ e);
		void On_TextBox_KeyDown(Object^ sender, KeyEventArgs^ e);
		void On_TextBox_LostFocus(Object^ sender, EventArgs^ e);
		void On_Repeat_Timer_Tick(Object^ sender, EventArgs^ e);

		void Enter_Edit_Mode();
		void Exit_Edit_Mode(bool apply_changes);

		void Increment_Value(int steps);
		void Decrement_Value(int steps);
	};
}
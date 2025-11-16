#pragma once

#include "Theme_Manager.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;

namespace MIDILightDrawer
{
	/// <summary>
	/// Modern time offset control with ss:fff ms format
	/// Click to edit, +/- buttons for 10ms increments
	/// </summary>
	public ref class Control_TimeOffset_NumericUpDown : UserControl
	{
	private:
		// Components
		TextBox^ _TextBox_Input;
		Button^ _Button_Increment;
		Button^ _Button_Decrement;
		Panel^ _Display_Panel;
		Label^ _Label_Display;

		// Value tracking
		double _Total_Milliseconds;
		double _Minimum_ms;
		double _Maximum_ms;
		bool _Is_Editing;

		// Constants
		literal int STEP_SIZE_MS = 10;
		literal int BUTTON_WIDTH = 20;
		literal int DISPLAY_HEIGHT = 26;

	public:
		Control_TimeOffset_NumericUpDown();

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
			void set(double value) { _Minimum_ms = value; }
		}

		property double Maximum_ms
		{
			double get() { return _Maximum_ms; }
			void set(double value) { _Maximum_ms = value; }
		}

	private:
		void Initialize_Components();
		void Update_Display();
		String^ Format_Time(double milliseconds);
		double Parse_Time_Input(String^ input);

		// Event handlers
		void On_Display_Click(Object^ sender, EventArgs^ e);
		void On_TextBox_KeyPress(Object^ sender, KeyPressEventArgs^ e);
		void On_TextBox_KeyDown(Object^ sender, KeyEventArgs^ e);
		void On_TextBox_LostFocus(Object^ sender, EventArgs^ e);
		void On_Increment_Click(Object^ sender, EventArgs^ e);
		void On_Decrement_Click(Object^ sender, EventArgs^ e);

		void Enter_Edit_Mode();
		void Exit_Edit_Mode(bool apply_changes);
	};
}

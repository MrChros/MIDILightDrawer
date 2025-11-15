#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	/// <summary>
	/// Custom numeric up/down control for time offset in "ss:fff ms" format
	/// where ss = seconds, fff = milliseconds
	/// </summary>
	public ref class Control_TimeOffset_NumericUpDown : UserControl
	{
	private:
		// Components
		NumericUpDown^ _NumericUpDown_Seconds;
		NumericUpDown^ _NumericUpDown_Milliseconds;
		Label^ _Label_Separator;
		Label^ _Label_Unit;

		// Value tracking
		double _Total_Milliseconds;
		bool _Updating_From_Total;

		// Styling
		Color _Background_Color;
		Color _Foreground_Color;
		Color _Border_Color;

	public:
		Control_TimeOffset_NumericUpDown();

		// Events
		event EventHandler^ ValueChanged;

		// Properties
		property double Value_ms
		{
			double get() { return _Total_Milliseconds; }
			void set(double value);
		}

		property double Minimum_ms
		{
			double get();
			void set(double value);
		}

		property double Maximum_ms
		{
			double get();
			void set(double value);
		}

		property Color BackColor_Custom
		{
			Color get() { return _Background_Color; }
			void set(Color value);
		}

		property Color ForeColor_Custom
		{
			Color get() { return _Foreground_Color; }
			void set(Color value);
		}

	private:
		void Initialize_Components();
		void Update_From_Total_Milliseconds();
		void Update_Total_From_Components();
		void On_Component_ValueChanged(Object^ sender, EventArgs^ e);
		void Apply_Theme();
	};
}
#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Drawing::Drawing2D;
using namespace System::Drawing::Imaging;

#include "Theme_Manager.h"
#include "Control_ColorSlider.h"

namespace MIDILightDrawer
{
	public ref class Control_ColorPicker : public UserControl {
	public:
		Control_ColorPicker();

	private:
		Bitmap^					_Wheel_Bitmap;
		Control_ColorSlider^	_Saturation_Slider;
		Control_ColorSlider^	_Value_Slider;
		TextBox^ _TextBox_Red;
		TextBox^ _TextBox_Green;
		TextBox^ _TextBox_Blue;
		TextBox^ _TextBox_Hex;
		Label^	_Label_Red;
		Label^	_Label_Green;
		Label^	_Label_Blue;
		Label^	_Label_Hex;
		float	_Radius_Wheel;
		float	_Current_Hue;
		float	_Current_Saturation;
		float	_Current_Value;
		bool	_Is_Dragging_Wheel;
		bool	_Updating_Text_Boxes;

		static int SLIDER_HEIGHT		= 20;
		static int SPACING				= 10;
		static int TEXT_BOX_WIDTH		= 50;
		static int TEXT_BOX_HEIGHT		= 20;
		static const int RING_WIDTH		= 10;

	protected:
		void Initialize_Sliders();
		void Initialize_Text_Boxes();
		void Create_Wheel_Bitmap();
		void Validate_RGBTextBox_Input(TextBox^ textBox);
		void Validate_HexTextBox_Input(TextBox^ textBox);
		String^ Get_Valid_Hex_Color(String^ input);

		void Update_Color_From_Mouse(int x, int y);
		void Update_TextBox_Positions();
		void Update_Slider_Positions();
		void Update_Text_Boxes();
		void Update_From_RGB(int r, int g, int b);
		void Update_Sliders_Hue();


		virtual void OnPaint(PaintEventArgs^ e) override;
		void OnMouseDown(Object^ sender, MouseEventArgs^ e);
		void OnMouseMove(Object^ sender, MouseEventArgs^ e);
		void OnMouseUp(Object^ sender, MouseEventArgs^ e);
		void OnResize(Object^ sender, EventArgs^ e);

		void OnColorChanged();
		void OnSliderValueChanged(Object^ sender, EventArgs^ e);
		void OnRGBTextBoxValueChanged(Object^ sender, EventArgs^ e);
		void OnHexTextBoxValueChanged(Object^ sender, EventArgs^ e);
		
		static Color ColorFromHSV(float hue, float saturation, float value);
		static void RGBtoHSV(int r, int g, int b, float% h, float% s, float% v);

	public:
		property Color SelectedColor {
			Color get();
			void set(Color color);
		}

		property bool IsTyping {
			bool get();
		}

		event EventHandler^ ColorChanged;
	};
}
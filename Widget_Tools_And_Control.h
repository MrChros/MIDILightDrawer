#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

#include "Control_ColorPicker.h"

#include "Widget_Toolbar.h"
#include "Widget_Draw_Options.h"
#include "Widget_Fade_Options.h"
#include "Widget_Length_Options.h"
#include "Widget_Color_Options.h"
#include "Widget_Bucket_Options.h"

namespace MIDILightDrawer {

	public ref class Widget_Tools_And_Control : public System::Windows::Forms::UserControl
	{
	public:
		Widget_Tools_And_Control(void);

		void Select_Color_From_Preset(int color_index);
		void Quantization_Up(void);
		void Quantization_Down(void);

		Widget_Toolbar^			Get_Widget_Toolbar(void);
		Widget_Draw_Options^	Get_Widget_Draw_Options(void);
		Widget_Fade_Options^	Get_Widget_Fade_Options(void);
		Widget_Length_Options^	Get_Widget_Length_Options(void);
		Widget_Color_Options^	Get_Widget_Color_Options(void);
		Widget_Bucket_Options^	Get_Widget_Bucket_Options(void);

	protected:
		~Widget_Tools_And_Control();

	private:
		Widget_Toolbar^				_Toolbar;

		Control_ColorPicker^		_Color_Picker;
		TableLayoutPanel^			_Options_Container;

		Widget_Draw_Options^		_Draw_Options;
		Widget_Fade_Options^		_Fade_Options;
		Widget_Length_Options^		_Length_Options;
		Widget_Color_Options^		_Color_Options;
		Widget_Bucket_Options^		_Bucket_Options;

		void UpdateOptionsVisibility(Widget_Toolbar::ToolType tool);
		void OnToolbar_ModeChanged(System::Object^ sender, MIDILightDrawer::Widget_Toolbar::ToolType e);
	};
}

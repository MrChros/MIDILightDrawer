#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;

#include "Settings.h"
#include "Control_GroupBox.h"
#include "Control_DropDown.h"
#include "Control_ColorPicker.h"
#include "Control_FadePreview.h"
#include "Widget_Draw_Options.h"

namespace MIDILightDrawer
{ 
	public delegate void FadeModeChangedHandler(FadeType mode);
	public delegate void EasingsChangedHandler(FadeEasing easeIn, FadeEasing easeOut);
	
	public ref class Widget_Fade_Options : public System::Windows::Forms::UserControl
	{
	public:
		Widget_Fade_Options(Control_ColorPicker^ color_picker);

	private:
		System::ComponentModel::Container^ _Components;
		Control_GroupBox^		_GroupBox;
		Control_DropDown^		_DropDown_Fade_Quantization;
		Control_DropDown^		_DropDown_Ease_In;
		Control_DropDown^		_DropDown_Ease_Out;
		Control_ColorPicker^	_Color_Picker;
		Control_FadePreview^	_Fade_Preview;
		
		void Initialize_Component(void);
		void DropDown_Fade_Quantization_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void DropDown_Easings_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e);
		void Fade_Preview_OnPreviewSideSelected(System::Drawing::Color color);
		void Button_Switch_Colors_OnClick(System::Object^ sender, System::EventArgs^ e);
		void Button_Fade_Mode_OnClick(System::Object^ sender, System::EventArgs^ e);

	protected:
		~Widget_Fade_Options();

		virtual void OnVisibleChanged(EventArgs^ e) override;

	public:
		void Select_Next_Fade_Value(void);
		void Select_Previous_Fade_Value(void);

		property int TickLength {
			int get();
			void set(int value);
		}

		property FadeType FadeMode {
			FadeType get();
		}

		property Color StartColor {
			Color get();
		}

		property Color EndColor {
			Color get();
		}

		property Color CenterColor {
			Color get();
		}

		property FadeEasing EaseIn {
			FadeEasing get();
		}

		property FadeEasing EaseOut {
			FadeEasing get();
		}

		event QuantizationChangedHandler^ QuantizationChanged;
		event ColorChangedHandler^ ColorStartChanged;
		event ColorChangedHandler^ ColorEndChanged;
		event ColorChangedHandler^ ColorCenterChanged;
		event FadeModeChangedHandler^ FadeModeChanged;
		event EasingsChangedHandler^ EasingsChanged;
	};
}
#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;

#include "Widget_Draw_Options.h"

#include "Control_DropDown.h"

namespace MIDILightDrawer
{
	public ref class Widget_Length_Options : public System::Windows::Forms::UserControl
	{
	public:
		Widget_Length_Options(void);

	private:
		System::ComponentModel::Container^ _Components;
		GroupBox^			_GroupBox;
		Control_DropDown^	_DropDown_Length_Quantization;

		int					_Length_Quantization_Ticks;

		void Initialize_Component(void);
		void DropDown_Length_Quantization_OnItem_Selected(System::Object^ sender, MIDILightDrawer::Control_DropDown_Item_Selected_Event_Args^ e);

	public:
		void Select_Next_Length_Value(void);
		void Select_Previous_Length_Value(void);

		property int Value {
			int get();
			void set(int value);
		}

		event QuantizationChangedHandler^ QuantizationChanged;
	};
}
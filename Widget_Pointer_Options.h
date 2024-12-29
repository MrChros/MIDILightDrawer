#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;

#include "Settings.h"
#include "Control_DropDown.h"
#include "Control_GroupBox.h"
#include "Widget_Draw_Options.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	public ref class Widget_Pointer_Options : public UserControl
	{
	public:
		Widget_Pointer_Options();

	private:
		System::ComponentModel::Container^ _Components;
		Control_GroupBox^ _GroupBox;
		Control_DropDown^ _DropDown_Draw_Snapping;

		void Initialize_Component(void);
		void DropDown_Draw_Snapping_OnItem_Selected(System::Object^ sender, MIDILightDrawer::Control_DropDown_Item_Selected_Event_Args^ e);

	protected:
		~Widget_Pointer_Options();

	public:
		void Select_Pointer_Snapping_Next(void);
		void Select_Pointer_Snapping_Previous(void);

		property int PointerSnapping {
			int get();
			void set(int value);
		}

		event QuantizationChangedHandler^ SnappingChanged;
	};
}
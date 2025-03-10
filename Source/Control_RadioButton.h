#pragma once

#include "Theme_Manager.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Control_RadioButton : public Button
	{
	private:
		System::Resources::ResourceManager^ _Resources;
		System::Drawing::Image^ _TickIcon;
		String^ _OptionText;

		bool _IsSelected;
		int _GroupId;

	public:
		event EventHandler^ SelectedChanged;

		Control_RadioButton();
		System::Drawing::Image^ GetTickIcon();

	private:
		void ApplyTheme();
		void UpdateAppearance();
		
		void OnClick(Object^ sender, EventArgs^ e);
		System::Drawing::Image^ Control_RadioButton::CreateTickIcon();

	public:
		// Group ID allows for grouping radio buttons
		property int GroupId
		{
			int get();
			void set(int value);
		}

		property bool Selected
		{
			bool get();
			void set(bool value);
		}

		property String^ OptionText
		{
			String^ get();
			void set(String^ value);
		}

		
	};
}
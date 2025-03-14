#pragma once

#include "Theme_Manager.h"
#include "Control_GroupBox.h"
#include "Control_CheckedListBox.h"
#include "Widget_Timeline_Classes.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer {

	// Class to hold track information for the import dialog
	public ref class LightTrackInfo
	{
	public:
		LightTrackInfo(String^ name, int eventCount, List<BarEvent^>^ events)
		{
			Name = name;
			EventCount = eventCount;
			Events = events;
		}

		property String^ Name;
		property int EventCount;
		property List<BarEvent^>^ Events;
	};
	
	public ref class Form_Light_Import : public System::Windows::Forms::Form
	{
	public:
		Form_Light_Import(List<LightTrackInfo^>^ availableTracks);

	protected:
		~Form_Light_Import();

	private:
		System::ComponentModel::Container^ components;
		Control_CheckedListBox^ _CheckListTracks;
		Button^ _ButtonOk;
		Button^ _ButtonCancel;
		Button^ _ButtonSelectAll;
		Button^ _ButtonSelectNone;
		GroupBox^ _GroupBoxTracks;

		List<LightTrackInfo^>^ _AvailableTracks;
		List<LightTrackInfo^>^ _SelectedTracks;

		void InitializeComponent(void);
		void ApplyTheme();
		void PopulateTrackList();
		void ButtonOk_Click(System::Object^ sender, System::EventArgs^ e);
		void ButtonSelectAll_Click(System::Object^ sender, System::EventArgs^ e);
		void ButtonSelectNone_Click(System::Object^ sender, System::EventArgs^ e);

	public:
		property List<LightTrackInfo^>^ SelectedTracks {
			List<LightTrackInfo^>^ get();
		}
	};

	
}
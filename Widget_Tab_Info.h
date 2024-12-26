#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	public ref class Widget_Tab_Info : public System::Windows::Forms::UserControl
	{
	public:
		Widget_Tab_Info(void);

		void Update_Info(String^ file_path, String^ song_name, UInt32 count_measures, UInt32 count_tracks);
		void Add_Track_Title(String^ track_tilte);

		String^ Get_Song_Name(void);

	protected:
		~Widget_Tab_Info();

	private:
		Label^ _Label_Song_Name;
		Label^ _Label_File_Path;
		Label^ _Label_Measures_Count;
		Label^ _Label_Tracks_Count;
		ToolTip^ _Track_Tooltip;

		String^ _Song_Name;
		List<String^>^ _Track_Titles;

		Label^ Create_Text_Label(String^ text);
	};
}

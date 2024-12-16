#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace MIDILightDrawer {

	public ref class Widget_Tab_Info : public System::Windows::Forms::UserControl
	{
	public:
		Widget_Tab_Info(void);

		void Update_Info(String^ song_name, UInt32 count_measures, UInt32 count_tracks);

	protected:
		~Widget_Tab_Info();

	private:
		Label^ _Label_Song_Name;
		Label^ _Label_Measures_Count;
		Label^ _Label_Tracks_Count;

		Label^ Create_Text_Label(String^ text);
	};
}

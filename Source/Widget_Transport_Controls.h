#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	ref class Playback_Manager;

	public ref class Widget_Transport_Controls : public UserControl
	{
	private:
		System::Resources::ResourceManager^ _Resources;

		Playback_Manager^ _Playback_Manager;

		// Buttons
		Button^ _Button_Move_To_Start;
		Button^ _Button_Rewind;
		Button^ _Button_Play_Pause;
		Button^ _Button_Fast_Forward;
		Button^ _Button_Move_To_End;

		// Rewind/Fast Forward state
		bool _Is_Rewinding;
		bool _Is_Fast_Forwarding;
		bool _Moved_To_Start;
		bool _Moved_To_End;
		Timer^ _Seek_Timer;

	public:
		Widget_Transport_Controls();

		void Set_Playback_Manager(Playback_Manager^ playback_manager);
		void Update_State(bool is_playing);

	private:
		void Initialize_Components();
		void Attach_Event_Handlers();

		// Button event handlers
		void On_Move_To_Start_Click(Object^ sender, EventArgs^ e);
		void On_Rewind_Mouse_Down(Object^ sender, MouseEventArgs^ e);
		void On_Rewind_Mouse_Up(Object^ sender, MouseEventArgs^ e);
		void On_Play_Pause_Click(Object^ sender, EventArgs^ e);
		void On_Fast_Forward_Mouse_Down(Object^ sender, MouseEventArgs^ e);
		void On_Fast_Forward_Mouse_Up(Object^ sender, MouseEventArgs^ e);
		void On_Move_To_End_Click(Object^ sender, EventArgs^ e);

		// Seek timer
		void On_Seek_Timer_Tick(Object^ sender, EventArgs^ e);

		// Button appearance
		void Update_Play_Pause_Button();

	public:
		property bool Is_Playing {
			bool get();
		}

		property bool Is_Rewinding {
			bool get();
		}

		property bool Is_Fast_Forwarding {
			bool get();
		}

		property bool Moved_To_Start {
			bool get();
		}

		property bool Moved_To_End {
			bool get();
		}
	};
}
#pragma once

#include "Control_Label.h"

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
		Button^ _Button_AutoScroll;

		Control_Label^ _Label_Time;

		// Rewind/Fast Forward state
		bool _Is_Rewinding;
		bool _Is_Fast_Forwarding;
		bool _Moved_To_Start;
		bool _Moved_To_End;
		bool _Was_Playing_Before_Seek;
		bool _AutoScroll_Enabled;
		Int64 _Current_Time_ms;
		Int64 _Last_Time_ms;
		Timer^ _Seek_Timer;
		Timer^ _Update_Time_Timer;

	public:
		Widget_Transport_Controls();

		void Set_Playback_Manager(Playback_Manager^ playback_manager);
		void Update_State();

		void Trigger_Play_Pause();
		void Trigger_Move_To_Start();
		void Trigger_Move_To_End();
		void Trigger_Rewind_Start();
		void Trigger_Rewind_Stop();
		void Trigger_Fast_Forward_Start();
		void Trigger_Fast_Forward_Stop();

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
		void On_AutoScroll_Click(System::Object^ sender, System::EventArgs^ e);

		// Timer Callbacks
		void On_Seek_Timer_Tick(Object^ sender, EventArgs^ e);
		void On_Update_Time_Timer_Tick(Object^ sender, EventArgs^ e);

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

		property bool AutoScroll_Enabled {
			bool get();
		}
	};
}
#include "Widget_Transport_Controls.h"
#include "Playback_Manager.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	Widget_Transport_Controls::Widget_Transport_Controls()
	{
		_Playback_Manager = nullptr;
		_Is_Rewinding = false;
		_Is_Fast_Forwarding = false;
		_Is_Playing = false;

		Initialize_Components();
		Setup_Layout();
		Attach_Event_Handlers();
	}

	void Widget_Transport_Controls::Initialize_Components()
	{
		// Initialize buttons
		_Button_Move_To_Start = gcnew Button();
		_Button_Rewind = gcnew Button();
		_Button_Play_Pause = gcnew Button();
		_Button_Fast_Forward = gcnew Button();
		_Button_Move_To_End = gcnew Button();

		// Set button text (or icons if available)
		_Button_Move_To_Start->Text = "|<";
		_Button_Rewind->Text = "<<";
		_Button_Play_Pause->Text = ">";
		_Button_Fast_Forward->Text = ">>";
		_Button_Move_To_End->Text = ">|";

		// Set button sizes
		int Button_Width = 50;
		int Button_Height = 30;

		_Button_Move_To_Start->Size = System::Drawing::Size(Button_Width, Button_Height);
		_Button_Rewind->Size = System::Drawing::Size(Button_Width, Button_Height);
		_Button_Play_Pause->Size = System::Drawing::Size(Button_Width, Button_Height);
		_Button_Fast_Forward->Size = System::Drawing::Size(Button_Width, Button_Height);
		_Button_Move_To_End->Size = System::Drawing::Size(Button_Width, Button_Height);

		// Set tooltips
		ToolTip^ Tooltip = gcnew ToolTip();
		Tooltip->SetToolTip(_Button_Move_To_Start, "Move to Start (Home)");
		Tooltip->SetToolTip(_Button_Rewind, "Rewind (J)");
		Tooltip->SetToolTip(_Button_Play_Pause, "Play/Pause (Space)");
		Tooltip->SetToolTip(_Button_Fast_Forward, "Fast Forward (L)");
		Tooltip->SetToolTip(_Button_Move_To_End, "Move to End (End)");

		// Apply theme
		//Color Background_Color = Theme_Manager::Get_Instance()->BackgroundAlt;
		//Color Button_Color = Theme_Manager::Get_Color(Theme_Color::Button_Primary);
		//Color Text_Color = Theme_Manager::Get_Instance()->ForegroundText;

		this->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;

		for each (Button ^ Btn in gcnew array<Button^>{_Button_Move_To_Start, _Button_Rewind, _Button_Play_Pause, _Button_Fast_Forward, _Button_Move_To_End })
		{
			Theme_Manager::Get_Instance()->ApplyThemeToButton(Btn);
		}

		// Initialize seek timer
		_Seek_Timer = gcnew Timer();
		_Seek_Timer->Interval = 100; // 100ms interval for seeking
		_Seek_Timer->Tick += gcnew EventHandler(this, &Widget_Transport_Controls::On_Seek_Timer_Tick);

		// Add controls
		this->Controls->Add(_Button_Move_To_Start);
		this->Controls->Add(_Button_Rewind);
		this->Controls->Add(_Button_Play_Pause);
		this->Controls->Add(_Button_Fast_Forward);
		this->Controls->Add(_Button_Move_To_End);
	}

	void Widget_Transport_Controls::Setup_Layout()
	{
		// Horizontal layout with padding
		int Padding = 5;
		int X_Position = Padding;
		int Y_Position = Padding;

		_Button_Move_To_Start->Location = Point(X_Position, Y_Position);
		X_Position += _Button_Move_To_Start->Width + Padding;

		_Button_Rewind->Location = Point(X_Position, Y_Position);
		X_Position += _Button_Rewind->Width + Padding;

		_Button_Play_Pause->Location = Point(X_Position, Y_Position);
		X_Position += _Button_Play_Pause->Width + Padding;

		_Button_Fast_Forward->Location = Point(X_Position, Y_Position);
		X_Position += _Button_Fast_Forward->Width + Padding;

		_Button_Move_To_End->Location = Point(X_Position, Y_Position);
		X_Position += _Button_Move_To_End->Width + Padding;

		// Set control size
		this->Size = System::Drawing::Size(X_Position, _Button_Play_Pause->Height + 2 * Padding);
		this->MinimumSize = this->Size;
		this->MaximumSize = this->Size;
	}

	void Widget_Transport_Controls::Attach_Event_Handlers()
	{
		_Button_Move_To_Start->Click += gcnew EventHandler(this, &Widget_Transport_Controls::On_Move_To_Start_Click);
		_Button_Rewind->MouseDown += gcnew MouseEventHandler(this, &Widget_Transport_Controls::On_Rewind_Mouse_Down);
		_Button_Rewind->MouseUp += gcnew MouseEventHandler(this, &Widget_Transport_Controls::On_Rewind_Mouse_Up);
		_Button_Play_Pause->Click += gcnew EventHandler(this, &Widget_Transport_Controls::On_Play_Pause_Click);
		_Button_Fast_Forward->MouseDown += gcnew MouseEventHandler(this, &Widget_Transport_Controls::On_Fast_Forward_Mouse_Down);
		_Button_Fast_Forward->MouseUp += gcnew MouseEventHandler(this, &Widget_Transport_Controls::On_Fast_Forward_Mouse_Up);
		_Button_Move_To_End->Click += gcnew EventHandler(this, &Widget_Transport_Controls::On_Move_To_End_Click);
	}

	void Widget_Transport_Controls::On_Move_To_Start_Click(Object^ sender, EventArgs^ e)
	{
		if (_Playback_Manager)
		{
			_Playback_Manager->Seek_To_Position(0.0);
		}
	}

	void Widget_Transport_Controls::On_Rewind_Mouse_Down(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		_Is_Rewinding = true;
		_Playback_Manager->Set_Playback_Speed(-2.0); // Rewind at 2x speed
		_Seek_Timer->Start();
	}

	void Widget_Transport_Controls::On_Rewind_Mouse_Up(Object^ sender, MouseEventArgs^ e)
	{
		_Is_Rewinding = false;
		_Seek_Timer->Stop();

		if (_Playback_Manager)
		{
			_Playback_Manager->Set_Playback_Speed(1.0);
		}
	}

	void Widget_Transport_Controls::On_Play_Pause_Click(Object^ sender, EventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		if (_Playback_Manager->Is_Playing())
		{
			_Playback_Manager->Pause();
			_Is_Playing = false;
		}
		else
		{
			_Playback_Manager->Play();
			_Is_Playing = true;
		}

		Update_Play_Pause_Button();
	}

	void Widget_Transport_Controls::On_Fast_Forward_Mouse_Down(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		_Is_Fast_Forwarding = true;
		_Playback_Manager->Set_Playback_Speed(2.0); // Fast forward at 2x speed
		_Seek_Timer->Start();
	}

	void Widget_Transport_Controls::On_Fast_Forward_Mouse_Up(Object^ sender, MouseEventArgs^ e)
	{
		_Is_Fast_Forwarding = false;
		_Seek_Timer->Stop();

		if (_Playback_Manager)
		{
			_Playback_Manager->Set_Playback_Speed(1.0);
		}
	}

	void Widget_Transport_Controls::On_Move_To_End_Click(Object^ sender, EventArgs^ e)
	{
		if (_Playback_Manager)
		{
			double Duration = _Playback_Manager->Get_Audio_Duration_Ms();
			if (Duration > 0.0)
			{
				_Playback_Manager->Seek_To_Position(Duration);
			}
		}
	}

	void Widget_Transport_Controls::On_Seek_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		double Current_Position = _Playback_Manager->Get_Current_Position_Ms();
		double Seek_Amount = 500.0; // Seek 500ms per tick

		if (_Is_Rewinding)
		{
			double New_Position = Current_Position - Seek_Amount;
			if (New_Position < 0.0) {
				New_Position = 0.0;
			}
			_Playback_Manager->Seek_To_Position(New_Position);
		}
		else if (_Is_Fast_Forwarding)
		{
			double Duration = _Playback_Manager->Get_Audio_Duration_Ms();
			double New_Position = Current_Position + Seek_Amount;
			if (Duration > 0.0 && New_Position > Duration) {
				New_Position = Duration;
			}
			_Playback_Manager->Seek_To_Position(New_Position);
		}
	}

	void Widget_Transport_Controls::Update_Play_Pause_Button()
	{
		if (_Is_Playing)
		{
			_Button_Play_Pause->Text = "||"; // Pause symbol
		}
		else
		{
			_Button_Play_Pause->Text = ">"; // Play symbol
		}
	}

	void Widget_Transport_Controls::Set_Playback_Manager(Playback_Manager^ playback_manager)
	{
		this->_Playback_Manager = playback_manager;
	}

	void Widget_Transport_Controls::Update_State(bool is_playing)
	{
		_Is_Playing = is_playing;
		Update_Play_Pause_Button();
	}
}
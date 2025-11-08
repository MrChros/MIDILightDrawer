#include "Widget_Transport_Controls.h"
#include "Playback_Manager.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	Widget_Transport_Controls::Widget_Transport_Controls()
	{
		_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());

		_Playback_Manager = nullptr;
		_Is_Rewinding = false;
		_Is_Fast_Forwarding = false;
		_Moved_To_Start = false;
		_Moved_To_End = false;
		_Was_Playing_Before_Seek = false;
		_AutoScroll_Enabled = false;
		_Current_Time_ms = 0;
		_Last_Time_ms = -1;

		Initialize_Components();
		Attach_Event_Handlers();
	}

	void Widget_Transport_Controls::Initialize_Components()
	{
		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 9;
		Layout->RowCount = 1;

		const int BUTTON = 50;
		const int SPACER = 30;

		// Configure column styles
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 200));	// Time
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON));	// Auto Follow
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, SPACER));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON));	// Move to Start
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON));	// Rewind
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON));	// Play/Pause
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON));	// Fast-Forward
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON));	// Move to End
		
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.f));

		// Initialize buttons
		_Button_Move_To_Start = gcnew Button();
		_Button_Rewind = gcnew Button();
		_Button_Play_Pause = gcnew Button();
		_Button_Fast_Forward = gcnew Button();
		_Button_Move_To_End = gcnew Button();
		_Button_AutoScroll = gcnew Button();

		_Button_Move_To_Start->Image	= (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_MoveToStart_24")));
		_Button_Rewind->Image			= (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_Rewind_24"		)));
		_Button_Play_Pause->Image		= (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_Play_24"		)));
		_Button_Fast_Forward->Image		= (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_FastForward_24")));
		_Button_Move_To_End->Image		= (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_MoveToEnd_24"	)));
		_Button_AutoScroll->Image		= (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_AutoScroll_24"	)));

		Drawing::Font^ Label_Font = gcnew Drawing::Font("Segoe UI Bold", 14.0f);
		_Label_Time = gcnew Control_Label();
		_Label_Time->Dock = DockStyle::Fill;
		_Label_Time->TextAlign = ContentAlignment::MiddleCenter;
		_Label_Time->ForeColor = Color::White;
		_Label_Time->BackColor = Color::Transparent;
		_Label_Time->Font = Label_Font;

		// Set tooltips
		ToolTip^ Tooltip = gcnew ToolTip();
		Tooltip->SetToolTip(_Button_Move_To_Start	, "Move to Start");
		Tooltip->SetToolTip(_Button_Rewind			, "Rewind");
		Tooltip->SetToolTip(_Button_Play_Pause		, "Play/Pause");
		Tooltip->SetToolTip(_Button_Fast_Forward	, "Fast Forward");
		Tooltip->SetToolTip(_Button_Move_To_End		, "Move to End");
		Tooltip->SetToolTip(_Button_AutoScroll		, "Toggle Auto Scroll");

		this->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;

		for each (Button ^ Btn in gcnew array<Button^>{_Button_Move_To_Start, _Button_Rewind, _Button_Play_Pause, _Button_Fast_Forward, _Button_Move_To_End, _Button_AutoScroll })
		{
			Btn->Dock = DockStyle::Fill;
			Btn->ImageAlign = ContentAlignment::MiddleCenter;
			Btn->TextAlign = ContentAlignment::MiddleCenter;
			Btn->TextImageRelation = TextImageRelation::ImageBeforeText;
			Btn->Padding = System::Windows::Forms::Padding(0);
			Theme_Manager::Get_Instance()->ApplyThemeToButton(Btn);
		}

		// Initialize seek timer
		_Seek_Timer = gcnew Timer();
		_Seek_Timer->Interval = 100; // 100ms interval for seeking
		_Seek_Timer->Tick += gcnew EventHandler(this, &Widget_Transport_Controls::On_Seek_Timer_Tick);

		_Update_Time_Timer = gcnew Timer();
		_Update_Time_Timer->Interval = 100;
		_Update_Time_Timer->Tick += gcnew EventHandler(this, &Widget_Transport_Controls::On_Update_Time_Timer_Tick);

		// Add controls
		Layout->Controls->Add(_Label_Time			, 1, 0);
		Layout->Controls->Add(_Button_AutoScroll, 3-1, 0);
		Layout->Controls->Add(_Button_Move_To_Start	, 5-1, 0);
		Layout->Controls->Add(_Button_Rewind		, 6-1, 0);
		Layout->Controls->Add(_Button_Play_Pause	, 7-1, 0);
		Layout->Controls->Add(_Button_Fast_Forward	, 8-1, 0);
		Layout->Controls->Add(_Button_Move_To_End	, 9-1, 0);

		this->Controls->Add(Layout);

		_Update_Time_Timer->Start();
	}

	void Widget_Transport_Controls::Attach_Event_Handlers()
	{
		_Button_Move_To_Start->Click	+= gcnew EventHandler		(this, &Widget_Transport_Controls::On_Move_To_Start_Click);
		_Button_Rewind->MouseDown		+= gcnew MouseEventHandler	(this, &Widget_Transport_Controls::On_Rewind_Mouse_Down);
		_Button_Rewind->MouseUp			+= gcnew MouseEventHandler	(this, &Widget_Transport_Controls::On_Rewind_Mouse_Up);
		_Button_Play_Pause->Click		+= gcnew EventHandler		(this, &Widget_Transport_Controls::On_Play_Pause_Click);
		_Button_Fast_Forward->MouseDown += gcnew MouseEventHandler	(this, &Widget_Transport_Controls::On_Fast_Forward_Mouse_Down);
		_Button_Fast_Forward->MouseUp	+= gcnew MouseEventHandler	(this, &Widget_Transport_Controls::On_Fast_Forward_Mouse_Up);
		_Button_Move_To_End->Click		+= gcnew EventHandler		(this, &Widget_Transport_Controls::On_Move_To_End_Click);
		_Button_AutoScroll->Click		+= gcnew EventHandler		(this, &Widget_Transport_Controls::On_AutoScroll_Click);
	}

	void Widget_Transport_Controls::On_Move_To_Start_Click(Object^ sender, EventArgs^ e)
	{
		if (_Playback_Manager)
		{
			_Playback_Manager->Seek_To_Position(0.0);
			_Moved_To_Start = true;
		}
	}

	void Widget_Transport_Controls::On_Rewind_Mouse_Down(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Playback_Manager || _Is_Rewinding) {
			return;
		}

		// Remember if we were playing
		_Was_Playing_Before_Seek = _Playback_Manager->Is_Playing();

		// Pause playback if playing
		if (_Was_Playing_Before_Seek) {
			_Playback_Manager->Pause();
		}

		_Is_Rewinding = true;
		_Playback_Manager->Set_Playback_Speed(-2.0); // Rewind at 2x speed
		_Seek_Timer->Start();
	}

	void Widget_Transport_Controls::On_Rewind_Mouse_Up(Object^ sender, MouseEventArgs^ e)
	{
		_Is_Rewinding = false;
		_Seek_Timer->Stop();

		if (_Playback_Manager) {
			_Playback_Manager->Set_Playback_Speed(1.0);
		}

		// Resume playback if we were playing before
		if (_Playback_Manager && _Was_Playing_Before_Seek)
		{
			_Playback_Manager->Play();
			_Was_Playing_Before_Seek = false;
		}
	}

	void Widget_Transport_Controls::On_Play_Pause_Click(Object^ sender, EventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		if (_Playback_Manager->Is_Playing()) {
			_Playback_Manager->Pause();
		}
		else {
			_Playback_Manager->Play();
		}

		Update_Play_Pause_Button();
	}

	void Widget_Transport_Controls::On_Fast_Forward_Mouse_Down(Object^ sender, MouseEventArgs^ e)
	{
		if (!_Playback_Manager || _Is_Fast_Forwarding) {
			return;
		}

		// Remember if we were playing
		_Was_Playing_Before_Seek = _Playback_Manager->Is_Playing();

		// Pause playback if playing
		if (_Was_Playing_Before_Seek) {
			_Playback_Manager->Pause();
		}

		_Is_Fast_Forwarding = true;
		_Playback_Manager->Set_Playback_Speed(2.0); // Fast forward at 2x speed
		_Seek_Timer->Start();
	}

	void Widget_Transport_Controls::On_Fast_Forward_Mouse_Up(Object^ sender, MouseEventArgs^ e)
	{
		_Is_Fast_Forwarding = false;
		_Seek_Timer->Stop();

		if (_Playback_Manager) {
			_Playback_Manager->Set_Playback_Speed(1.0);
		}

		// Resume playback if we were playing before
		if (_Playback_Manager && _Was_Playing_Before_Seek)
		{
			_Playback_Manager->Play();
			_Was_Playing_Before_Seek = false;
		}
	}

	void Widget_Transport_Controls::On_Move_To_End_Click(Object^ sender, EventArgs^ e)
	{
		if (_Playback_Manager) {
			double Duration = _Playback_Manager->Get_Audio_Duration_Ms();
			if (Duration > 0.0)
			{
				_Playback_Manager->Seek_To_Position(Duration);
				_Moved_To_End = true;
			}
		}
	}

	void Widget_Transport_Controls::On_AutoScroll_Click(System::Object^ sender, System::EventArgs^ e)
	{
		_AutoScroll_Enabled = !_AutoScroll_Enabled;

		if (_AutoScroll_Enabled) {
			_Button_AutoScroll->BackColor = Theme_Manager::Get_Instance()->AccentPrimary;
		}
		else {
			_Button_AutoScroll->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
		}
	}

	void Widget_Transport_Controls::On_Seek_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		double Current_Position = _Playback_Manager->Get_Playback_Position_ms();
		double Seek_Amount = 500.0; // Seek 500ms per tick

		if (_Is_Rewinding) {
			double New_Position = Current_Position - Seek_Amount;
			if (New_Position < 0.0) {
				New_Position = 0.0;
			}
			_Playback_Manager->Seek_To_Position(New_Position);
		}
		else if (_Is_Fast_Forwarding) {
			double Duration = _Playback_Manager->Get_Audio_Duration_Ms();
			double New_Position = Current_Position + Seek_Amount;
			if (Duration > 0.0 && New_Position > Duration) {
				New_Position = Duration;
			}
			_Playback_Manager->Seek_To_Position(New_Position);
		}
	}

	void Widget_Transport_Controls::On_Update_Time_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		if (!_Playback_Manager) {
			return;
		}

		Int64 _Current_Time_ms = (Int64)_Playback_Manager->Get_Playback_Position_ms();
		
		if (_Current_Time_ms != _Last_Time_ms) {
			_Last_Time_ms = _Current_Time_ms;
			Int64 Time_ms = _Current_Time_ms;
			String^ String_Min = (Time_ms / 60000).ToString(L"D")->PadLeft(2, L'0');
			Time_ms %= 60000;
			String^ String_Sec = (Time_ms / 1000).ToString(L"D")->PadLeft(2, L'0');
			Time_ms %= 1000;
			String^ String_MSec = Time_ms.ToString(L"D")->PadLeft(3, L'0');

			_Label_Time->Text = String_Min + ":" + String_Sec + ":" + String_MSec;
		}
	}

	void Widget_Transport_Controls::Update_Play_Pause_Button()
	{
		if (_Playback_Manager->Is_Playing()) {
			_Button_Play_Pause->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_Stop_24")));
		}
		else {
			_Button_Play_Pause->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject("Transport_Controls_Play_24")));
		}
	}

	void Widget_Transport_Controls::Set_Playback_Manager(Playback_Manager^ playback_manager)
	{
		this->_Playback_Manager = playback_manager;
	}

	void Widget_Transport_Controls::Update_State(bool is_playing)
	{
		Update_Play_Pause_Button();
	}

	void Widget_Transport_Controls::Trigger_Play_Pause()
	{
		On_Play_Pause_Click(nullptr, EventArgs::Empty);
	}

	void Widget_Transport_Controls::Trigger_Move_To_Start()
	{
		On_Move_To_Start_Click(nullptr, EventArgs::Empty);
	}

	void Widget_Transport_Controls::Trigger_Move_To_End()
	{
		On_Move_To_End_Click(nullptr, EventArgs::Empty);
	}

	void Widget_Transport_Controls::Trigger_Rewind_Start()
	{
		On_Rewind_Mouse_Down(this, gcnew MouseEventArgs(System::Windows::Forms::MouseButtons::Right, 0, 0, 0, 0));
	}

	void Widget_Transport_Controls::Trigger_Rewind_Stop()
	{
		On_Rewind_Mouse_Up(this, gcnew MouseEventArgs(System::Windows::Forms::MouseButtons::Right, 0, 0, 0, 0));
	}

	void Widget_Transport_Controls::Trigger_Fast_Forward_Start()
	{
		On_Fast_Forward_Mouse_Down(this, gcnew MouseEventArgs(System::Windows::Forms::MouseButtons::Right, 0, 0, 0, 0));
	}

	void Widget_Transport_Controls::Trigger_Fast_Forward_Stop()
	{
		On_Fast_Forward_Mouse_Up(this, gcnew MouseEventArgs(System::Windows::Forms::MouseButtons::Right, 0, 0, 0, 0));
	}

	bool Widget_Transport_Controls::Is_Playing::get()
	{
		return _Playback_Manager->Is_Playing();
	}

	bool Widget_Transport_Controls::Is_Rewinding::get()
	{
		return _Is_Rewinding;
	}

	bool Widget_Transport_Controls::Is_Fast_Forwarding::get()
	{
		return _Is_Fast_Forwarding;
	}

	bool Widget_Transport_Controls::Moved_To_Start::get()
	{
		bool Return_Value = _Moved_To_Start;

		_Moved_To_Start = false;

		return Return_Value;
	}

	bool Widget_Transport_Controls::Moved_To_End::get()
	{
		bool Return_Value = _Moved_To_End;

		_Moved_To_End = false;

		return Return_Value;
	}

	bool Widget_Transport_Controls::AutoScroll_Enabled::get()
	{
		return _AutoScroll_Enabled;
	}
}


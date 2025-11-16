#include "Widget_Audio_Container.h"

#include "Theme_Manager.h"
#include "Widget_Timeline.h"
#include "Playback_Manager.h"

namespace MIDILightDrawer
{
	Widget_Audio_Container::Widget_Audio_Container()
	{
		this->_Timeline = nullptr;
		this->_Playback_Manager = nullptr;
		this->_Marker_Timestamps = gcnew List<double>;

		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->Dock = DockStyle::Fill;

		Table_Layout_Main->RowCount = 3;
		Table_Layout_Main->ColumnCount = 9;

		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 1));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 130));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 70));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 120));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));

		Drawing::Font^ Font_Label = gcnew Drawing::Font("Segoe UI Semibold", 9.5f);
		Label^ Label_Offset = gcnew Label();
		Label_Offset->Text = "Audio Offset:";
		
		Label^ Label_Volume = gcnew Label();
		Label_Volume->Text = "Volume:";


		// Create Audio Offset TextBox
		_TimeOffset_Audio_Offset = gcnew Control_TimeOffset_NumericUpDown();
		_TimeOffset_Audio_Offset->Dock = DockStyle::Fill;
		_TimeOffset_Audio_Offset->Value_ms = 0.0;
		_TimeOffset_Audio_Offset->Minimum_ms = -60000.0; // -60 seconds
		_TimeOffset_Audio_Offset->Maximum_ms = 60000.0;  // +60 seconds
		_TimeOffset_Audio_Offset->Value_Changed += gcnew System::EventHandler(this, &Widget_Audio_Container::On_Audio_Offset_ValueChanged);
		//_TimeOffset_Audio_Offset->Margin = System::Windows::Forms::Padding(0, 5, 0, 0);

		// Create Volume Slider
		_VolumeSlider = gcnew Control_VolumeSlider();
		_VolumeSlider->Minimum = 0;
		_VolumeSlider->Maximum = 100;
		_VolumeSlider->Value = 100;
		_VolumeSlider->Dock = DockStyle::Fill;
		_VolumeSlider->ValueChanged += gcnew EventHandler(this, &Widget_Audio_Container::On_Volume_ValueChanged);

		_Label_Audio_Info_Length		= gcnew Label();
		_Label_Audio_Info_Channels		= gcnew Label();
		_Label_Audio_Info_Resolution	= gcnew Label();
		_Label_Audio_Info_Freqeuncy		= gcnew Label();
		_Label_Audio_Info_File			= gcnew Label();

		for each (Label ^ Lbl in gcnew array<Label^>{Label_Offset, Label_Volume, _Label_Audio_Info_Length, _Label_Audio_Info_Channels, _Label_Audio_Info_Resolution, _Label_Audio_Info_Freqeuncy, _Label_Audio_Info_File })
		{
			Lbl->Dock		= DockStyle::Fill;
			Lbl->TextAlign	= ContentAlignment::MiddleCenter;
			Lbl->ForeColor	= Color::White;
			Lbl->BackColor	= Color::Transparent;
		}
		_Label_Audio_Info_File->TextAlign = ContentAlignment::MiddleLeft;

		Table_Layout_Main->Controls->Add(Label_Offset, 0, 0);
		Table_Layout_Main->Controls->Add(_TimeOffset_Audio_Offset, 1, 0);
		Table_Layout_Main->Controls->Add(Label_Volume, 2, 0);
		Table_Layout_Main->Controls->Add(_VolumeSlider, 3, 0);

		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Length		, 4, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Channels		, 5, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Resolution	, 6, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Freqeuncy	, 7, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_File			, 8, 0);


		Label^ Label_Line = gcnew Label();
		Label_Line->Dock = DockStyle::Fill;
		Label_Line->BackColor = Theme_Manager::Get_Instance()->AccentPrimary;

		Table_Layout_Main->Controls->Add(Label_Line, 0, 1);
		Table_Layout_Main->SetColumnSpan(Label_Line, Table_Layout_Main->ColumnCount);

		_Audio_Waveform = gcnew Widget_Audio_Waveform(_Marker_Timestamps);
		_Audio_Waveform->Dock = DockStyle::Fill;
		_Audio_Waveform->OnCursorPositionChanged += gcnew System::EventHandler<double>(this, &Widget_Audio_Container::On_Cursor_Position_Changed);

		Table_Layout_Main->Controls->Add(_Audio_Waveform, 0, 2);
		Table_Layout_Main->SetColumnSpan(_Audio_Waveform, Table_Layout_Main->ColumnCount);

		this->Controls->Add(Table_Layout_Main);

		Load_Audio_Data();
	}

	void Widget_Audio_Container::Set_Widget_Timeline(Widget_Timeline^ timeline)
	{
		if (timeline) {
			this->_Timeline = timeline;
		}
	}

	void Widget_Audio_Container::Set_Playback_Manager(Playback_Manager^ playback_manager)
	{
		if (playback_manager) {
			this->_Playback_Manager = playback_manager;
		}
	}

	void Widget_Audio_Container::Load_Audio_Data()
	{
		if(this->_Playback_Manager) {
			this->_Audio_Waveform->Set_Audio_Duration_ms(this->_Playback_Manager->Get_Audio_Duration_ms());
			this->_Audio_Waveform->Set_Waveform_Data(this->_Playback_Manager->Audio_Waveform_Data);
		}

		this->_Audio_Waveform->Invalidate();

		Update_Audio_Information();
	}

	void Widget_Audio_Container::Unload_Audio_Data()
	{
		if (this->_Playback_Manager) {
			this->_Audio_Waveform->Set_Audio_Duration_ms(Widget_Audio_Waveform::NO_AUDIO);
			this->_Audio_Waveform->Set_Waveform_Data(nullptr);
		}

		this->_Audio_Waveform->Invalidate();

		Update_Audio_Information();
	}

	void Widget_Audio_Container::Load_MIDI_Information()
	{
		_Marker_Timestamps->Clear();

		if (this->_Playback_Manager) {
			this->_Audio_Waveform->Set_MIDI_Duration_ms(this->_Playback_Manager->Get_MIDI_Duration_ms());
		}
			
		if (this->_Timeline) {
			for each(Measure^ M in _Timeline->Measures)
			{
				if (M->Marker_Text->Length > 0) {
					_Marker_Timestamps->Add(M->StartTime_ms);
				}
			}
		}

		this->_Audio_Waveform->Invalidate();
	}

	void Widget_Audio_Container::Update_Cursor()
	{
		if(this->_Playback_Manager) {
			this->_Audio_Waveform->Set_Cursor_Position_ms(this->_Playback_Manager->Get_Playback_Position_ms());
		}
	}

	Widget_Audio_Waveform^ Widget_Audio_Container::Waveform()
	{
		return _Audio_Waveform;
	}

	void Widget_Audio_Container::Update_Audio_Information()
	{
		if (!this->_Playback_Manager || !this->_Playback_Manager->Is_Audio_Loaded) {
			_Label_Audio_Info_Length->Text = "";
			_Label_Audio_Info_Channels->Text = "";
			_Label_Audio_Info_Resolution->Text = "";
			_Label_Audio_Info_Freqeuncy->Text = "";
			_Label_Audio_Info_File->Text = "File: <No Audio file loaded>";
			return;
		}

		TimeSpan Time_Span = TimeSpan::FromMilliseconds(this->_Playback_Manager->Audio_Duration_ms);
		String^ Formatted_Time = Time_Span.ToString("mm\\:ss\\.fff");

		String^ Audio_Channel = "Mono";
		if(this->_Playback_Manager->Audio_Channel_Count == 2) {
			Audio_Channel = "Stereo";
		}
		else if (this->_Playback_Manager->Audio_Channel_Count > 2) {
			Audio_Channel = "Stereo+";
		}

		String^ Audio_Info_Length		= String::Format("Length:\n{0}"				, Formatted_Time);
		String^ Audio_Info_Channels		= String::Format("Channels:\n{0}"			, Audio_Channel);
		String^ Audio_Info_Resolution	= String::Format("Resolution:\n {0} bits"	, this->_Playback_Manager->Audio_Bit_Rate);
		String^ Audio_Info_Freqeuncy	= String::Format("Frequency:\n{0} Hz"		, this->_Playback_Manager->Audio_Sample_Rate_File);
		String^ Audio_Info_File			= String::Format("File:\n{0}"				, this->_Playback_Manager->Audio_File_Path);

		_Label_Audio_Info_Length->Text		= Audio_Info_Length;
		_Label_Audio_Info_Channels->Text	= Audio_Info_Channels;
		_Label_Audio_Info_Resolution->Text	= Audio_Info_Resolution;
		_Label_Audio_Info_Freqeuncy->Text	= Audio_Info_Freqeuncy;
		_Label_Audio_Info_File->Text		= Audio_Info_File;
	}

	void Widget_Audio_Container::On_Audio_Offset_ValueChanged(Object^ sender, EventArgs^ e)
	{
		if (!this->_Playback_Manager) {
			return;
		}

		double Audio_Offset_ms = _TimeOffset_Audio_Offset->Value_ms;

		_Playback_Manager->Set_Audio_Offset(Audio_Offset_ms);
	}

	void Widget_Audio_Container::On_Volume_ValueChanged(Object^ sender, EventArgs^ e)
	{
		if (!this->_Playback_Manager) {
			return;
		}

		double Volume_Percent = _VolumeSlider->Value / 100.0;
		_Playback_Manager->Set_Volume(Volume_Percent);
	}

	void Widget_Audio_Container::On_Cursor_Position_Changed(Object^ sender, double cursor_position_ms)
	{
		if (!this->_Playback_Manager) {
			return;
		}

		_Playback_Manager->Seek_To_Position(cursor_position_ms);
	}
}
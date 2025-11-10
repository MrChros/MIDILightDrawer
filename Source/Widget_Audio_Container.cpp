#include "Widget_Audio_Container.h"

#include "Theme_Manager.h"
#include "Playback_Manager.h"
#include "Playback_Audio_File_Manager.h"

namespace MIDILightDrawer
{
	Widget_Audio_Container::Widget_Audio_Container()
	{
		this->_Playback_Manager = nullptr;
		
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->Dock = DockStyle::Fill;

		Table_Layout_Main->RowCount = 3;
		Table_Layout_Main->ColumnCount = 7;

		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 1));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));

		Drawing::Font^ Label_Font = gcnew Drawing::Font("Segoe UI Semibold", 9.5f);
		Label^ Label_Offset = gcnew Label();
		Label_Offset->Dock = DockStyle::Fill;
		Label_Offset->TextAlign = ContentAlignment::MiddleCenter;
		Label_Offset->ForeColor = Color::White;
		Label_Offset->BackColor = Color::Transparent;
		Label_Offset->Text = "Audio Offset:";
		//Label_Offset->Font = Label_Font;

		Table_Layout_Main->Controls->Add(Label_Offset, 0, 0);

		_Label_Audio_Info_Length		= gcnew Label();
		_Label_Audio_Info_Channels		= gcnew Label();
		_Label_Audio_Info_Resolution	= gcnew Label();
		_Label_Audio_Info_Freqeuncy		= gcnew Label();
		_Label_Audio_Info_File			= gcnew Label();

		for each (Label ^ Lbl in gcnew array<Label^>{_Label_Audio_Info_Length, _Label_Audio_Info_Channels, _Label_Audio_Info_Resolution, _Label_Audio_Info_Freqeuncy, _Label_Audio_Info_File })
		{
			Lbl->Dock		= DockStyle::Fill;
			Lbl->TextAlign	= ContentAlignment::MiddleCenter;
			Lbl->ForeColor	= Color::White;
			Lbl->BackColor	= Color::Transparent;
		}

		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Length		, 2, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Channels		, 3, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Resolution	, 4, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_Freqeuncy	, 5, 0);
		Table_Layout_Main->Controls->Add(_Label_Audio_Info_File			, 6, 0);


		Label^ Label_Line = gcnew Label();
		Label_Line->Dock = DockStyle::Fill;
		Label_Line->BackColor = Theme_Manager::Get_Instance()->AccentPrimary;

		Table_Layout_Main->Controls->Add(Label_Line, 0, 1);
		Table_Layout_Main->SetColumnSpan(Label_Line, Table_Layout_Main->ColumnCount);

		_Audio_Waveform = gcnew Widget_Audio_Waveform();
		_Audio_Waveform->Dock = DockStyle::Fill;

		Table_Layout_Main->Controls->Add(_Audio_Waveform, 0, 2);
		Table_Layout_Main->SetColumnSpan(_Audio_Waveform, Table_Layout_Main->ColumnCount);

		this->Controls->Add(Table_Layout_Main);

		Set_Audio_File_Manager(nullptr);
	}

	void Widget_Audio_Container::Set_Playback_Manager(Playback_Manager^ playback_manager)
	{
		if (playback_manager) {
			this->_Playback_Manager = playback_manager;
		}
	}

	void Widget_Audio_Container::Set_Audio_File_Manager(Playback_Audio_File_Manager^ audio_file_manager)
	{
		this->_Audio_File_Manager = audio_file_manager;
		this->_Audio_Waveform->Set_Audio_File_Manager(audio_file_manager);

		this->_Audio_Waveform->Invalidate();

		Update_Audio_Information();
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
		if (this->_Audio_File_Manager == nullptr) {
			_Label_Audio_Info_Length->Text = "";
			_Label_Audio_Info_Channels->Text = "";
			_Label_Audio_Info_Resolution->Text = "";
			_Label_Audio_Info_Freqeuncy->Text = "";
			_Label_Audio_Info_File->Text = "File: <No Audio file loaded>";
			return;
		}

		TimeSpan Time_Span = TimeSpan::FromMilliseconds(this->_Audio_File_Manager->DurationMilliseconds);
		String^ Formatted_Time = Time_Span.ToString("mm\\:ss\\.fff");

		String^ Audio_Info_Length		= String::Format("Length: {0}"			, Formatted_Time);
		String^ Audio_Info_Channels		= String::Format("Channels: {0}"		, this->_Audio_File_Manager->ChannelCount);
		String^ Audio_Info_Resolution	= String::Format("Resolution: {0} bits"	, this->_Audio_File_Manager->BitDepth);
		String^ Audio_Info_Freqeuncy	= String::Format("Frequency: {0} Hz"	, this->_Audio_File_Manager->SampleRate);
		String^ Audio_Info_File			= String::Format("File: {0}"			, this->_Audio_File_Manager->FilePath);

		_Label_Audio_Info_Length->Text		= Audio_Info_Length;
		_Label_Audio_Info_Channels->Text	= Audio_Info_Channels;
		_Label_Audio_Info_Resolution->Text	= Audio_Info_Resolution;
		_Label_Audio_Info_Freqeuncy->Text	= Audio_Info_Freqeuncy;
		_Label_Audio_Info_File->Text		= Audio_Info_File;
	}
}
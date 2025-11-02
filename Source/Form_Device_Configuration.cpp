#include "Form_Device_Configuration.h"
#include "Settings.h"

using namespace System::ComponentModel;

namespace MIDILightDrawer
{
	Form_Device_Configuration::Form_Device_Configuration()
	{
		_Device_Manager = gcnew Device_Manager();
		Initialize_Component();
		Load_Current_Settings();
	}

	void Form_Device_Configuration::Initialize_Component()
	{
		this->Text = "Device Configuration";
		this->Size = System::Drawing::Size(600, 450);
		this->StartPosition = FormStartPosition::CenterParent;
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->ShowIcon = false;

		// Create tooltip
		_Tool_Tip = gcnew ToolTip();

		// Main layout
		_Main_Layout = gcnew TableLayoutPanel();
		_Main_Layout->ColumnCount = 1;
		_Main_Layout->RowCount = 4;
		_Main_Layout->Dock = DockStyle::Fill;
		_Main_Layout->Padding = System::Windows::Forms::Padding(10);
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));

		// MIDI Output Group Box
		_Group_Box_MIDI_Output = gcnew Control_GroupBox();
		_Group_Box_MIDI_Output->Text = "MIDI Output";
		_Group_Box_MIDI_Output->Dock = DockStyle::Fill;
		_Group_Box_MIDI_Output->Padding = System::Windows::Forms::Padding(10);

		_MIDI_Layout = gcnew TableLayoutPanel();
		_MIDI_Layout->ColumnCount = 3;
		_MIDI_Layout->RowCount = 2;
		_MIDI_Layout->Dock = DockStyle::Fill;
		_MIDI_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		_MIDI_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		_MIDI_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		_Label_MIDI_Device = gcnew Label();
		_Label_MIDI_Device->Text = "Device:";
		_Label_MIDI_Device->AutoSize = true;
		_Label_MIDI_Device->Anchor = AnchorStyles::Left;

		_Combo_Box_MIDI_Device = gcnew ComboBox();
		_Combo_Box_MIDI_Device->DropDownStyle = ComboBoxStyle::DropDownList;
		_Combo_Box_MIDI_Device->Dock = DockStyle::Fill;

		_Button_Refresh_MIDI = gcnew Button();
		_Button_Refresh_MIDI->Text = "Refresh";
		_Button_Refresh_MIDI->AutoSize = true;
		_Button_Refresh_MIDI->Click += gcnew EventHandler(this, &Form_Device_Configuration::Button_Refresh_MIDI_Click);

		_Label_MIDI_Channel = gcnew Label();
		_Label_MIDI_Channel->Text = "Channel:";
		_Label_MIDI_Channel->AutoSize = true;
		_Label_MIDI_Channel->Anchor = AnchorStyles::Left;

		_Combo_Box_MIDI_Channel = gcnew ComboBox();
		_Combo_Box_MIDI_Channel->DropDownStyle = ComboBoxStyle::DropDownList;
		_Combo_Box_MIDI_Channel->Dock = DockStyle::Fill;

		_MIDI_Layout->Controls->Add(_Label_MIDI_Device, 0, 0);
		_MIDI_Layout->Controls->Add(_Combo_Box_MIDI_Device, 1, 0);
		_MIDI_Layout->Controls->Add(_Button_Refresh_MIDI, 2, 0);
		_MIDI_Layout->Controls->Add(_Label_MIDI_Channel, 0, 1);
		_MIDI_Layout->Controls->Add(_Combo_Box_MIDI_Channel, 1, 1);

		_Group_Box_MIDI_Output->Controls->Add(_MIDI_Layout);

		// Audio Output Group Box
		_Group_Box_Audio_Output = gcnew Control_GroupBox();
		_Group_Box_Audio_Output->Text = "Audio Output";
		_Group_Box_Audio_Output->Dock = DockStyle::Fill;
		_Group_Box_Audio_Output->Padding = System::Windows::Forms::Padding(10);

		_Audio_Layout = gcnew TableLayoutPanel();
		_Audio_Layout->ColumnCount = 3;
		_Audio_Layout->RowCount = 2;
		_Audio_Layout->Dock = DockStyle::Fill;
		_Audio_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		_Audio_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		_Audio_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		_Label_Audio_Device = gcnew Label();
		_Label_Audio_Device->Text = "Device:";
		_Label_Audio_Device->AutoSize = true;
		_Label_Audio_Device->Anchor = AnchorStyles::Left;

		_Combo_Box_Audio_Device = gcnew ComboBox();
		_Combo_Box_Audio_Device->DropDownStyle = ComboBoxStyle::DropDownList;
		_Combo_Box_Audio_Device->Dock = DockStyle::Fill;

		_Button_Refresh_Audio = gcnew Button();
		_Button_Refresh_Audio->Text = "Refresh";
		_Button_Refresh_Audio->AutoSize = true;
		_Button_Refresh_Audio->Click += gcnew EventHandler(this, &Form_Device_Configuration::Button_Refresh_Audio_Click);

		_Label_Buffer_Size = gcnew Label();
		_Label_Buffer_Size->Text = "Buffer Size:";
		_Label_Buffer_Size->AutoSize = true;
		_Label_Buffer_Size->Anchor = AnchorStyles::Left;

		_Combo_Box_Buffer_Size = gcnew ComboBox();
		_Combo_Box_Buffer_Size->DropDownStyle = ComboBoxStyle::DropDownList;
		_Combo_Box_Buffer_Size->Dock = DockStyle::Fill;

		_Audio_Layout->Controls->Add(_Label_Audio_Device, 0, 0);
		_Audio_Layout->Controls->Add(_Combo_Box_Audio_Device, 1, 0);
		_Audio_Layout->Controls->Add(_Button_Refresh_Audio, 2, 0);
		_Audio_Layout->Controls->Add(_Label_Buffer_Size, 0, 1);
		_Audio_Layout->Controls->Add(_Combo_Box_Buffer_Size, 1, 1);

		_Group_Box_Audio_Output->Controls->Add(_Audio_Layout);

		// Buttons
		TableLayoutPanel^ Button_Panel = gcnew TableLayoutPanel();
		Button_Panel->ColumnCount = 4;
		Button_Panel->RowCount = 1;
		Button_Panel->Dock = DockStyle::Fill;
		Button_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		Button_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		Button_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		Button_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		_Button_OK = gcnew Button();
		_Button_OK->Text = "OK";
		_Button_OK->Size = System::Drawing::Size(80, 30);
		_Button_OK->Click += gcnew EventHandler(this, &Form_Device_Configuration::Button_OK_Click);

		_Button_Apply = gcnew Button();
		_Button_Apply->Text = "Apply";
		_Button_Apply->Size = System::Drawing::Size(80, 30);
		_Button_Apply->Click += gcnew EventHandler(this, &Form_Device_Configuration::Button_Apply_Click);

		_Button_Cancel = gcnew Button();
		_Button_Cancel->Text = "Cancel";
		_Button_Cancel->Size = System::Drawing::Size(80, 30);
		_Button_Cancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
		_Button_Cancel->Click += gcnew EventHandler(this, &Form_Device_Configuration::Button_Cancel_Click);

		Button_Panel->Controls->Add(_Button_OK, 1, 0);
		Button_Panel->Controls->Add(_Button_Apply, 2, 0);
		Button_Panel->Controls->Add(_Button_Cancel, 3, 0);

		// Add to main layout
		_Main_Layout->Controls->Add(_Group_Box_MIDI_Output, 0, 0);
		_Main_Layout->Controls->Add(_Group_Box_Audio_Output, 0, 1);
		_Main_Layout->Controls->Add(Button_Panel, 0, 3);

		this->Controls->Add(_Main_Layout);

		// Populate controls
		Populate_MIDI_Channels();
		Populate_Buffer_Sizes();
		Populate_MIDI_Devices();
		Populate_Audio_Devices();

		Theme_Manager::Get_Instance()->ApplyTheme(this);
	}

	void Form_Device_Configuration::Load_Current_Settings()
	{
		Settings^ Current_Settings = Settings::Get_Instance();

		// TODO: Load from Settings
		// Select MIDI device
		// Select MIDI channel
		// Select Audio device
		// Select Buffer size
	}

	void Form_Device_Configuration::Save_Settings()
	{
		Settings^ Current_Settings = Settings::Get_Instance();

		// TODO: Save to Settings
		// Save selected MIDI device
		// Save selected MIDI channel
		// Save selected Audio device
		// Save selected buffer size

		Current_Settings->Save_To_File();

		On_Device_Settings_Changed();
	}

	void Form_Device_Configuration::Populate_MIDI_Devices()
	{
		_Device_Manager->Enumerate_MIDI_Devices();
		_Combo_Box_MIDI_Device->Items->Clear();

		List<MIDI_Device_Info^>^ Devices = _Device_Manager->Get_MIDI_Devices();
		for each (MIDI_Device_Info ^ Device in Devices)
		{
			_Combo_Box_MIDI_Device->Items->Add(Device->Device_Name);
		}

		if (_Combo_Box_MIDI_Device->Items->Count > 0)
			_Combo_Box_MIDI_Device->SelectedIndex = 0;
	}

	void Form_Device_Configuration::Populate_Audio_Devices()
	{
		_Device_Manager->Enumerate_Audio_Devices();
		_Combo_Box_Audio_Device->Items->Clear();

		List<Audio_Device_Info^>^ Devices = _Device_Manager->Get_Audio_Devices();
		for each (Audio_Device_Info ^ Device in Devices)
		{
			_Combo_Box_Audio_Device->Items->Add(Device->Device_Name);
		}

		if (_Combo_Box_Audio_Device->Items->Count > 0)
			_Combo_Box_Audio_Device->SelectedIndex = 0;
	}

	void Form_Device_Configuration::Populate_MIDI_Channels()
	{
		_Combo_Box_MIDI_Channel->Items->Clear();
		for (int i = 1; i <= 16; ++i)
		{
			_Combo_Box_MIDI_Channel->Items->Add(i.ToString());
		}
		_Combo_Box_MIDI_Channel->SelectedIndex = 0; // Channel 1
	}

	void Form_Device_Configuration::Populate_Buffer_Sizes()
	{
		_Combo_Box_Buffer_Size->Items->Clear();
		_Combo_Box_Buffer_Size->Items->Add("256 samples");
		_Combo_Box_Buffer_Size->Items->Add("512 samples");
		_Combo_Box_Buffer_Size->Items->Add("1024 samples");
		_Combo_Box_Buffer_Size->Items->Add("2048 samples");
		_Combo_Box_Buffer_Size->Items->Add("4096 samples");
		_Combo_Box_Buffer_Size->SelectedIndex = 2; // 1024 default
	}

	void Form_Device_Configuration::Button_OK_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Save_Settings();
		this->DialogResult = System::Windows::Forms::DialogResult::OK;
		this->Close();
	}

	void Form_Device_Configuration::Button_Apply_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Save_Settings();
	}

	void Form_Device_Configuration::Button_Cancel_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->Close();
	}

	void Form_Device_Configuration::Button_Refresh_MIDI_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Populate_MIDI_Devices();
	}

	void Form_Device_Configuration::Button_Refresh_Audio_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Populate_Audio_Devices();
	}
}

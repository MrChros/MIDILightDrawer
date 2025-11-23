#include "Form_MIDI_Log.h"
#include "Theme_Manager.h"


#include <vcclr.h>

namespace MIDILightDrawer
{
	Form_MIDI_Log::Form_MIDI_Log()
	{
		_Log_Entries = gcnew List<MIDI_Log_Entry^>();
		_Event_Buffer = gcnew List<MIDI_Log_Entry^>();
		_Buffer_Lock = gcnew Object();

		Initialize_Components();
		Setup_Grid_Columns();
		Attach_Event_Handlers();

		// Setup update timer
		_Update_Timer = gcnew System::Windows::Forms::Timer();
		_Update_Timer->Interval = UPDATE_INTERVAL_MS;
		_Update_Timer->Tick += gcnew EventHandler(this, &Form_MIDI_Log::On_Update_Timer_Tick);
		_Update_Timer->Start();
	}

	void Form_MIDI_Log::Add_Log_Entry(MIDI_Log_Entry^ entry)
	{
		if (!entry) {
			return;
		}

		// Thread-safe add to buffer
		System::Threading::Monitor::Enter(_Buffer_Lock);
		try
		{
			_Event_Buffer->Add(entry);
		}
		finally
		{
			System::Threading::Monitor::Exit(_Buffer_Lock);
		}
	}

	void Form_MIDI_Log::On_Update_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		Flush_Event_Buffer();
	}

	void Form_MIDI_Log::Flush_Event_Buffer()
	{
		if (!this->IsHandleCreated || this->Disposing || this->IsDisposed) {
			return;
		}

		// Get buffered events
		List<MIDI_Log_Entry^>^ Events_To_Process = nullptr;

		System::Threading::Monitor::Enter(_Buffer_Lock);
		try
		{
			if (_Event_Buffer->Count == 0) {
				return;
			}

			Events_To_Process = _Event_Buffer;
			_Event_Buffer = gcnew List<MIDI_Log_Entry^>();
		}
		finally
		{
			System::Threading::Monitor::Exit(_Buffer_Lock);
		}

		if (Events_To_Process == nullptr || Events_To_Process->Count == 0) {
			return;
		}

		try
		{
			// Suspend layout for batch update
			_Grid_Log->SuspendLayout();

			for each (MIDI_Log_Entry^ entry in Events_To_Process)
			{
				Add_Entry_To_Grid(entry);
			}

			// Trim excess entries efficiently (from the beginning)
			int Excess_Count = _Log_Entries->Count - MAX_LOG_ENTRIES;
			if (Excess_Count > 0)
			{
				_Log_Entries->RemoveRange(0, Excess_Count);
			}

			// Trim excess grid rows
			Excess_Count = _Grid_Log->RowCount - MAX_LOG_ENTRIES;
			if (Excess_Count > 0)
			{
				for (int i = 0; i < Excess_Count; i++)
				{
					_Grid_Log->Rows->RemoveAt(0);
				}
			}

			// Auto-scroll to bottom
			if (_Grid_Log->RowCount > 0) {
				_Grid_Log->FirstDisplayedScrollingRowIndex = _Grid_Log->RowCount - 1;
			}

			_Grid_Log->ResumeLayout();
		}
		catch (System::Exception^ ex)
		{
			_Grid_Log->ResumeLayout();
			System::Diagnostics::Debug::WriteLine("MIDI Log Error: " + ex->Message);
		}
	}

	void Form_MIDI_Log::Add_Entry_To_Grid(MIDI_Log_Entry^ entry)
	{
		// Add to internal list
		_Log_Entries->Add(entry);

		int Row_Index = _Grid_Log->Rows->Add();
		DataGridViewRow^ Row = _Grid_Log->Rows[Row_Index];

		Row->Cells["Timestamp"]->Value = Format_Timestamp(entry->Timestamp_Ms);
		Row->Cells["Track"]->Value = entry->Track_Number.ToString();
		Row->Cells["Channel"]->Value = (entry->MIDI_Channel + 1).ToString();
		Row->Cells["Command"]->Value = entry->Command_Type;
		Row->Cells["Data"]->Value = entry->Data;
		Row->Cells["Description"]->Value = entry->Description;
	}

	void Form_MIDI_Log::Add_MIDI_Event(double timestamp_ms, int track, int channel, unsigned char command, unsigned char data1, unsigned char data2)
	{
		MIDI_Log_Entry^ Entry = gcnew MIDI_Log_Entry();
		Entry->Timestamp_Ms = timestamp_ms;
		Entry->Track_Number = track;
		Entry->MIDI_Channel = channel;

		unsigned char Command_Type = command & 0xF0;

		switch (Command_Type)
		{
		case 0x80:
			Entry->Command_Type = "Note Off";
			break;
		case 0x90:
			Entry->Command_Type = (data2 == 0) ? "Note Off" : "Note On";
			break;
		case 0xA0:
			Entry->Command_Type = "Aftertouch";
			break;
		case 0xB0:
			Entry->Command_Type = "Control Change";
			break;
		case 0xC0:
			Entry->Command_Type = "Program Change";
			break;
		case 0xD0:
			Entry->Command_Type = "Channel Pressure";
			break;
		case 0xE0:
			Entry->Command_Type = "Pitch Bend";
			break;
		default:
			Entry->Command_Type = "Unknown";
			break;
		}

		Entry->Data = String::Format("0x{0:X2}, {1}, {2}", command, data1, data2);
		Entry->Description = Format_MIDI_Command(command, data1, data2);

		Add_Log_Entry(Entry);
	}

	void Form_MIDI_Log::Clear_Log()
	{
		// Clear buffer
		System::Threading::Monitor::Enter(_Buffer_Lock);
		try
		{
			_Event_Buffer->Clear();
		}
		finally
		{
			System::Threading::Monitor::Exit(_Buffer_Lock);
		}

		_Log_Entries->Clear();
		_Grid_Log->Rows->Clear();
	}

	void Form_MIDI_Log::Export_Log_To_File(String^ file_path)
	{
		try
		{
			System::IO::StreamWriter^ Writer = gcnew System::IO::StreamWriter(file_path);

			// Write header
			Writer->WriteLine("MIDI Log");
			Writer->WriteLine("========");
			Writer->WriteLine();
			Writer->WriteLine("Timestamp\tTrack\tChannel\tCommand\tData\tDescription");
			Writer->WriteLine("-------------------------------------------------------------------");

			// Write entries
			for each (MIDI_Log_Entry ^ Entry in _Log_Entries)
			{
				Writer->WriteLine("{0}\t{1}\t{2}\t{3}\t{4}\t{5}",
					Format_Timestamp(Entry->Timestamp_Ms),
					Entry->Track_Number,
					Entry->MIDI_Channel + 1,
					Entry->Command_Type,
					Entry->Data,
					Entry->Description);
			}

			Writer->Close();
		}
		catch (Exception^ ex)
		{
			MessageBox::Show("Failed to export log:\n" + ex->Message, "Export Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		}
	}

	void Form_MIDI_Log::Initialize_Components()
	{
		// Form properties
		this->Text = "MIDI Log";
		this->Size = System::Drawing::Size(900, 500);
		this->MinimumSize = System::Drawing::Size(600, 300);
		this->StartPosition = FormStartPosition::CenterScreen;
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::Sizable;

		// Apply theme
		this->BackColor = Theme_Manager::Get_Instance()->Background;
		this->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;

		// Create data grid
		_Grid_Log = gcnew Control_DataGrid();
		_Grid_Log->Location = Point(10, 10);
		_Grid_Log->Size = System::Drawing::Size(this->ClientSize.Width - 20, this->ClientSize.Height - 60);
		_Grid_Log->Anchor = AnchorStyles::Top | AnchorStyles::Bottom | AnchorStyles::Left | AnchorStyles::Right;
		
		_Grid_Log->ReadOnly = true;
		_Grid_Log->MultiSelect = false;
		_Grid_Log->RowHeadersVisible = false;
		
		_Grid_Log->BackgroundColor = Theme_Manager::Get_Instance()->BackgroundAlt;
		_Grid_Log->ForeColor = Theme_Manager::Get_Instance()->Background;
		_Grid_Log->GridColor = Theme_Manager::Get_Instance()->BorderPrimary;
		
		_Grid_Log->ScrollBars = ScrollBars::Vertical;
		_Grid_Log->SelectionMode = DataGridViewSelectionMode::FullRowSelect;

		_Grid_Log->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;
		_Grid_Log->ColumnHeadersHeight = 32;
		_Grid_Log->ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode::DisableResizing;
		_Grid_Log->RowHeadersWidthSizeMode = DataGridViewRowHeadersWidthSizeMode::DisableResizing;

		_Grid_Log->AllowUserToAddRows = false;
		_Grid_Log->AllowUserToDeleteRows = false;
		_Grid_Log->AllowUserToResizeRows = false;
		_Grid_Log->AllowUserToResizeColumns = false;
		_Grid_Log->AllowUserToOrderColumns = false;
		_Grid_Log->AutoGenerateColumns = false;
		

		// Create buttons
		_Button_Clear = gcnew Button();
		_Button_Clear->Text = "Clear Log";
		_Button_Clear->Size = System::Drawing::Size(100, 30);
		_Button_Clear->Location = Point(this->ClientSize.Width - 220, this->ClientSize.Height - 40);
		_Button_Clear->Anchor = AnchorStyles::Bottom | AnchorStyles::Right;
		_Button_Clear->FlatStyle = FlatStyle::Flat;
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Clear);

		_Button_Export = gcnew Button();
		_Button_Export->Text = "Export Log";
		_Button_Export->Size = System::Drawing::Size(100, 30);
		_Button_Export->Location = Point(this->ClientSize.Width - 110, this->ClientSize.Height - 40);
		_Button_Export->Anchor = AnchorStyles::Bottom | AnchorStyles::Right;
		_Button_Export->FlatStyle = FlatStyle::Flat;
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Export);

		// Add controls
		this->Controls->Add(_Grid_Log);
		this->Controls->Add(_Button_Clear);
		this->Controls->Add(_Button_Export);
	}

	void Form_MIDI_Log::Setup_Grid_Columns()
	{
		_Grid_Log->Columns->Clear();

		// Add columns
		_Grid_Log->Columns->Add("Timestamp", "Timestamp");
		_Grid_Log->Columns->Add("Track", "Track");
		_Grid_Log->Columns->Add("Channel", "Channel");
		_Grid_Log->Columns->Add("Command", "Command");
		_Grid_Log->Columns->Add("Data", "Data");
		_Grid_Log->Columns->Add("Description", "Description");

		// Set column widths (proportional)
		_Grid_Log->Columns["Timestamp"]->FillWeight = 15;
		_Grid_Log->Columns["Track"]->FillWeight = 10;
		_Grid_Log->Columns["Channel"]->FillWeight = 10;
		_Grid_Log->Columns["Command"]->FillWeight = 15;
		_Grid_Log->Columns["Data"]->FillWeight = 20;
		_Grid_Log->Columns["Description"]->FillWeight = 30;
	}

	void Form_MIDI_Log::Attach_Event_Handlers()
	{
		_Button_Clear->Click += gcnew EventHandler(this, &Form_MIDI_Log::On_Clear_Click);
		_Button_Export->Click += gcnew EventHandler(this, &Form_MIDI_Log::On_Export_Click);
		this->FormClosing += gcnew FormClosingEventHandler(this, &Form_MIDI_Log::On_Form_Closing);
	}

	void Form_MIDI_Log::On_Clear_Click(Object^ sender, EventArgs^ e)
	{
		Clear_Log();
	}

	void Form_MIDI_Log::On_Export_Click(Object^ sender, EventArgs^ e)
	{
		SaveFileDialog^ Save_Dialog = gcnew SaveFileDialog();
		Save_Dialog->Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*";
		Save_Dialog->DefaultExt = "txt";
		Save_Dialog->FileName = "MIDI_Log.txt";

		if (Save_Dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			Export_Log_To_File(Save_Dialog->FileName);
		}
	}

	void Form_MIDI_Log::On_Form_Closing(Object^ sender, FormClosingEventArgs^ e)
	{
		// Hide instead of close (modeless dialog pattern)
		if (e->CloseReason == CloseReason::UserClosing)
		{
			e->Cancel = true;
			this->Hide();
		}
		else
		{
			// Actually closing - stop timer
			if (_Update_Timer != nullptr)
			{
				_Update_Timer->Stop();
				_Update_Timer = nullptr;
			}
		}
	}

	String^ Form_MIDI_Log::Format_Timestamp(double timestamp_ms)
	{
		int Total_Seconds = (int)(timestamp_ms / 1000.0);
		int Minutes = Total_Seconds / 60;
		int Seconds = Total_Seconds % 60;
		int Milliseconds = (int)(timestamp_ms - (Total_Seconds * 1000));

		return String::Format("{0:D2}:{1:D2}.{2:D3}", Minutes, Seconds, Milliseconds);
	}

	String^ Form_MIDI_Log::Format_MIDI_Command(unsigned char command, unsigned char data1, unsigned char data2)
	{
		unsigned char Command_Type = command & 0xF0;
		unsigned char Channel = command & 0x0F;

		switch (Command_Type)
		{
		case 0x80:
		{
			String^ Octave_Name = Get_Octave_Name(data1);
			String^ Color_Name = Get_Color_Name(data1);
			return String::Format("Note Off: {0}, Vel: {1} [{2}, {3}]", data1, data2, Octave_Name, Color_Name);
		}
		case 0x90:
		{
			String^ Octave_Name = Get_Octave_Name(data1);
			String^ Color_Name = Get_Color_Name(data1);
			if (data2 == 0)
				return String::Format("Note Off: {0}, Vel: 0 [{1}, {2}]", data1, Octave_Name, Color_Name);
			else
				return String::Format("Note On: {0}, Vel: {1} [{2}, {3}]", data1, data2, Octave_Name, Color_Name);
		}
		case 0xA0:
			return String::Format("Aftertouch: {0}, Pressure: {1}", data1, data2);
		case 0xB0:
			return String::Format("Control Change: CC{0} = {1}", data1, data2);
		case 0xC0:
			return String::Format("Program Change: {0}", data1);
		case 0xD0:
			return String::Format("Channel Pressure: {0}", data1);
		case 0xE0:
			return String::Format("Pitch Bend: {0}", (data2 << 7) | data1);
		default:
			return String::Format("Unknown: 0x{0:X2}, {1}, {2}", command, data1, data2);
		}
	}

	String^ Form_MIDI_Log::Get_Octave_Name(int midi_note)
	{
		int Octave_Number = -2 + midi_note / 12; // -2 is the lowest MIDI Octave
		
		// Find matching octave entry
		Settings^ Current_Settings = Settings::Get_Instance();
		for each (Settings::Octave_Entry^ Entry in Current_Settings->Octave_Entries)
		{
			if (Entry->Octave_Number == Octave_Number)
			{
				return Entry->Name;
			}
		}

		// Fallback to generic octave number
		return String::Format("Oct {0}", Octave_Number);
	}

	String^ Form_MIDI_Log::Get_Color_Name(int midi_note)
	{
		int Note_In_Octave = midi_note % 12;

		Settings^ Current_Settings = Settings::Get_Instance();

		if (Note_In_Octave == Current_Settings->MIDI_Note_Red || (Current_Settings->MIDI_Export_Anti_Flicker && Note_In_Octave == Current_Settings->MIDI_Note_Red + 1)) {
			return "Red";
		}
		else if (Note_In_Octave == Current_Settings->MIDI_Note_Green || (Current_Settings->MIDI_Export_Anti_Flicker && Note_In_Octave == Current_Settings->MIDI_Note_Green + 1)) {
			return "Green";
		}
		else if (Note_In_Octave == Current_Settings->MIDI_Note_Blue || (Current_Settings->MIDI_Export_Anti_Flicker && Note_In_Octave == Current_Settings->MIDI_Note_Blue + 1)) {
			return "Blue";
		}
		else {
			return "?";
		}
	}
}
#pragma once

#include "Control_DataGrid.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	public ref struct MIDI_Log_Entry
	{
		double Timestamp_Ms;
		int Track_Number;
		int MIDI_Channel;
		String^ Command_Type;
		String^ Data;
		String^ Description;

		MIDI_Log_Entry()
		{
			Timestamp_Ms = 0.0;
			Track_Number = 0;
			MIDI_Channel = 0;
			Command_Type = "";
			Data = "";
			Description = "";
		}
	};

	delegate void Add_MIDI_Event_Delegate(double timestamp_ms, int track, int channel, unsigned char command, unsigned char data1, unsigned char data2);
	delegate void Add_Log_Entry_Delegate(MIDI_Log_Entry^ entry);

	public ref class Form_MIDI_Log : public Form
	{
	private:
		Control_DataGrid^ _Grid_Log;
		Button^ _Button_Clear;
		Button^ _Button_Export;
		List<MIDI_Log_Entry^>^ _Log_Entries;

		static const int MAX_LOG_ENTRIES = 10000;

	public:
		Form_MIDI_Log();

		void Add_Log_Entry(MIDI_Log_Entry^ entry);
		void Add_MIDI_Event(double timestamp_ms, int track, int channel, unsigned char command, unsigned char data1, unsigned char data2);
		void Clear_Log();
		void Export_Log_To_File(String^ file_path);

	private:
		void Initialize_Components();
		void Setup_Grid_Columns();
		void Attach_Event_Handlers();

		// Event handlers
		void On_Clear_Click(Object^ sender, EventArgs^ e);
		void On_Export_Click(Object^ sender, EventArgs^ e);
		void On_Form_Closing(Object^ sender, FormClosingEventArgs^ e);

		// Helper methods
		String^ Format_Timestamp(double timestamp_ms);
		String^ Format_MIDI_Command(unsigned char command, unsigned char data1, unsigned char data2);
	};
}
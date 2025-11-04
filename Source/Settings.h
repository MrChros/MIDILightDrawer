#pragma once

#include "Hotkey_Manager.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	public ref class Settings
	{
	public:
		ref class Octave_Entry
		{
		public:
			String^ Name;
			int Octave_Number;

			Octave_Entry(String^ name, int octave);
		};

	private:
		static Settings^ _Instance;
		static String^ _Settings_File_Path;
		static Settings();

		// Octave Settings Members
		List<Octave_Entry^>^ _Octave_Entries;

		// MIDI Settings Members
		int _MIDI_Note_Red;
		int _MIDI_Note_Green;
		int _MIDI_Note_Blue;

		bool _MIDI_Export_Anti_Flicker;

		List<String^>^ _ColorPresets;

		// Playback Device Settings Members
		String^ _Selected_MIDI_Output_Device;
		int _Global_MIDI_Output_Channel;
		String^ _Selected_Audio_Output_Device;
		int _Audio_Buffer_Size;

		Settings();
		void Load_Defaults();
		void Load_From_File();
		String^ SerializeToString();
		void DeserializeFromString(String^ data);

	public:
		static Settings^ Get_Instance();
		static void Initialize(String^ settingsFilePath);
		void Save_To_File();

		// Hotkey methods
		String^ Get_Hotkey_Binding(String^ action);

		void Set_Hotkey_Binding(String^ action, String^ key);

		// MIDI settings properties
		property int MIDI_Note_Red
		{
			int get();
			void set(int value);
		}

		property int MIDI_Note_Green
		{
			int get();
			void set(int value);
		}

		property int MIDI_Note_Blue
		{
			int get();
			void set(int value);
		}

		property bool MIDI_Export_Anti_Flicker
		{
			bool get();
			void set(bool value);
		}

		property List<Octave_Entry^>^ Octave_Entries
		{
			List<Octave_Entry^>^ get();
			void set(List<Octave_Entry^>^ value);
		}

		property List<String^>^ ColorPresetsString
		{
			List<String^>^ get();
			void set(List<String^>^ value);
		}

		property List<Color>^ ColorPresetsColor
		{
			List<Color>^ get();
		}

		// Playback Device Settings Properties
		property String^ Selected_MIDI_Output_Device
		{
			String^ get();
			void set(String^ value);
		}

		property int Global_MIDI_Output_Channel
		{
			int get();
			void set(int value);
		}

		property String^ Selected_Audio_Output_Device
		{
			String^ get();
			void set(String^ value);
		}

		property int Audio_Buffer_Size
		{
			int get();
			void set(int value);
		}
	};
}
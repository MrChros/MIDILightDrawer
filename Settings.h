#pragma once

#include "Hotkey_Manager.h"

using namespace System;
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

			Octave_Entry(String^ name, int octave) {
				Name = name;
				Octave_Number = octave;
			}
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

		List<String^>^ _ColorPresets;

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
		String^ Get_Hotkey_Binding(String^ action) {
			return Hotkey_Manager::Instance->Get_Binding(action);
		}

		void Set_Hotkey_Binding(String^ action, String^ key) {
			auto bindings = Hotkey_Manager::Instance->Get_All_Bindings();
			bindings[action] = key;
			Hotkey_Manager::Instance->Set_Bindings(bindings);
			Save_To_File();
		}

		// MIDI settings properties
		property int MIDI_Note_Red
		{
			int get() { return _MIDI_Note_Red; }
			void set(int value) {
				_MIDI_Note_Red = value;
				Save_To_File();
			}
		}

		property int MIDI_Note_Green
		{
			int get() { return _MIDI_Note_Green; }
			void set(int value) {
				_MIDI_Note_Green = value;
				Save_To_File();
			}
		}

		property int MIDI_Note_Blue
		{
			int get() { return _MIDI_Note_Blue; }
			void set(int value) {
				_MIDI_Note_Blue = value;
				Save_To_File();
			}
		}

		property List<Octave_Entry^>^ Octave_Entries
		{
			List<Octave_Entry^> ^ get() { return _Octave_Entries; }
			void set(List<Octave_Entry^> ^ value) {
				_Octave_Entries = value;
				Save_To_File();
			}
		}

		property List<String^>^ ColorPresets
		{
			List<String^>^ get() { return _ColorPresets; }
			void set(List<String^>^ value) {
				_ColorPresets = value;
				Save_To_File();
			}
		}
	};
}
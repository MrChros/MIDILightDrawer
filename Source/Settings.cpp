#include "Settings.h"

using namespace System::IO;
using namespace System::Text;

namespace MIDILightDrawer {
	static Settings::Settings() {
		_Instance = nullptr;
		_Settings_File_Path = nullptr;
	}

	Settings::Settings() {
		_Octave_Entries = gcnew List<Octave_Entry^>();
		Load_Defaults();
	}

	void Settings::Load_Defaults() {
		// Add default note assignments
		_MIDI_Note_Red = 0;     // C
		_MIDI_Note_Green = 4;   // E
		_MIDI_Note_Blue = 7;    // G

		_ColorPresets = gcnew List<String^>();

		for (int i = 0; i < 10; i++) {
			_ColorPresets->Add("255,255,255"); // Default white color
		}
	}

	void Settings::Initialize(String^ settingsFilePath) {
		_Settings_File_Path = settingsFilePath;
		Get_Instance();
	}

	void Settings::Save_To_File() {
		try {
			String^ jsonString = SerializeToString();
			File::WriteAllText(_Settings_File_Path, jsonString);
		}
		catch (Exception^ ex) {
			Console::WriteLine("Error saving settings: " + ex->Message);
		}
	}

	Settings^ Settings::Get_Instance() {
		if (_Instance == nullptr) {
			_Instance = gcnew Settings();

			if (File::Exists(_Settings_File_Path)) {
				_Instance->Load_From_File();
			}
			else {
				_Instance->Save_To_File();
			}
		}
		return _Instance;
	}

	String^ Settings::SerializeToString() {
		StringBuilder^ sb = gcnew StringBuilder();

		// Start JSON object
		sb->AppendLine("{");

		// Serialize hotkey bindings
		sb->AppendLine("  \"HotkeyBindings\": {");
		auto bindings = Hotkey_Manager::Instance->Get_All_Bindings();
		for each(KeyValuePair<String^, String^> pair in bindings) {
			sb->AppendLine(String::Format("    \"{0}\": \"{1}\",",
				pair.Key->Replace("\"", "\\\""),
				pair.Value->Replace("\"", "\\\"")));
		}
		// Remove the last comma and close the hotkey bindings object
		sb->Length = sb->Length - 3;
		sb->AppendLine("");
		sb->AppendLine("  },");

		// Add MIDI settings
		sb->AppendLine(String::Format("  \"MidiNoteRed\": {0},", _MIDI_Note_Red));
		sb->AppendLine(String::Format("  \"MidiNoteGreen\": {0},", _MIDI_Note_Green));
		sb->AppendLine(String::Format("  \"MidiNoteBlue\": {0}", _MIDI_Note_Blue));

		// Add octave entries
		sb->AppendLine("  \"OctaveEntries\": [");
		for (int i = 0; i < _Octave_Entries->Count; i++) {
			Octave_Entry^ entry = _Octave_Entries[i];
			sb->AppendLine(String::Format("    {{\"Name\": \"{0}\", \"Octave\": {1}}}{2}",
				entry->Name->Replace("\"", "\\\""),
				entry->Octave_Number,
				i < _Octave_Entries->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ]");

		sb->AppendLine("  \"ColorPresets\": [");
		for (int i = 0; i < _ColorPresets->Count; i++) {
			sb->AppendLine(String::Format("    \"{0}\"{1}",
				_ColorPresets[i]->Replace("\"", "\\\""),
				i < _ColorPresets->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ]");

		// Close JSON object
		sb->AppendLine("}");
		return sb->ToString();

		return sb->ToString();
	}

	void Settings::DeserializeFromString(String^ data) {
		try {
			// Split into lines for simple parsing
			array<String^>^ lines = data->Split(gcnew array<String^> { "\r\n", "\n" }, StringSplitOptions::None);

			// Simple state machine for parsing
			bool inHotkeyBindings = false;
			Dictionary<String^, String^>^ hotkeyBindings = gcnew Dictionary<String^, String^>();

			bool inOctaveEntries = false;
			_Octave_Entries->Clear();

			bool inColorPresets = false;
			_ColorPresets->Clear();

			for each(String ^ line in lines) {
				String^ currentLine = line->Trim();

				// Skip empty lines and brackets
				if (currentLine->Length == 0 || currentLine == "{" || currentLine == "}")
					continue;

				// Check for octave entries section
				if (currentLine == "\"OctaveEntries\": [") {
					inOctaveEntries = true;
					continue;
				}

				if (currentLine == "]" || currentLine == "],") {
					inOctaveEntries = false;
					continue;
				}

				if (inOctaveEntries && currentLine->Contains("\"Name\"")) {
					String^ name = currentLine->Split('"')[3];
					String^ octaveStr = currentLine->Split(':')[2]->Trim(L'}', L' ', L',');
					int octave = Int32::Parse(octaveStr);
					_Octave_Entries->Add(gcnew Octave_Entry(name, octave));
				}

				// Remove trailing commas and any remaining whitespace
				if (currentLine->EndsWith(","))
					currentLine = currentLine->Substring(0, currentLine->Length - 1)->Trim();

				// Check for hotkey bindings section
				if (currentLine == "\"HotkeyBindings\":{" || currentLine == "\"HotkeyBindings\": {") {
					inHotkeyBindings = true;
					continue;
				}

				// End of hotkey bindings section
				if (currentLine == "}" || currentLine == "},") {
					inHotkeyBindings = false;
					continue;
				}

				// Parse hotkey bindings
				if (inHotkeyBindings) {
					array<String^>^ parts = currentLine->Split(gcnew array<wchar_t> { ':' });
					if (parts->Length == 2) {
						String^ key = parts[0]->Trim()->Trim(L'"', L' ');
						String^ value = parts[1]->Trim()->Trim(L'"', L' ', L',');
						if (!String::IsNullOrEmpty(key)) {
							hotkeyBindings->Add(key, value);
						}
					}
					continue;
				}

				// Parse MIDI note settings
				if (currentLine->StartsWith("\"MidiNoteRed\"")) {
					array<String^>^ parts = currentLine->Split(gcnew array<wchar_t> { ':' });
					if (parts->Length == 2) {
						_MIDI_Note_Red = Int32::Parse(parts[1]->Trim(L',', L' '));
					}
				}
				else if (currentLine->StartsWith("\"MidiNoteGreen\"")) {
					array<String^>^ parts = currentLine->Split(gcnew array<wchar_t> { ':' });
					if (parts->Length == 2) {
						_MIDI_Note_Green = Int32::Parse(parts[1]->Trim(L',', L' '));
					}
				}
				else if (currentLine->StartsWith("\"MidiNoteBlue\"")) {
					array<String^>^ parts = currentLine->Split(gcnew array<wchar_t> { ':' });
					if (parts->Length == 2) {
						_MIDI_Note_Blue = Int32::Parse(parts[1]->Trim(L',', L' '));
					}
				}

				// Parse Color Preset settings
				if (currentLine == "\"ColorPresets\": [") {
					inColorPresets = true;
					continue;
				}

				if (currentLine == "]" || currentLine == "],") {
					inColorPresets = false;
					continue;
				}

				if (inColorPresets && currentLine->StartsWith("\"")) {
					String^ colorStr = currentLine->Trim(L'"', L' ', L',');
					_ColorPresets->Add(colorStr);
				}
			}

			// Set the parsed hotkey bindings to the Hotkey_Manager
			Hotkey_Manager::Instance->Set_Bindings(hotkeyBindings);
		}
		catch (Exception^ ex) {
			Console::WriteLine("Error parsing settings: " + ex->Message);
			Load_Defaults();
		}
	}

	void Settings::Load_From_File() {
		try {
			String^ fileContent = File::ReadAllText(_Settings_File_Path);
			DeserializeFromString(fileContent);
		}
		catch (Exception^ ex) {
			Console::WriteLine("Error loading settings: " + ex->Message);
			Load_Defaults();
			Save_To_File();
		}
	}
}
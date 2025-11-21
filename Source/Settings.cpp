#include "Settings.h"

using namespace System::IO;
using namespace System::Text;

namespace MIDILightDrawer
{
	//////////////////
	// Octave_Entry //
	//////////////////
	Settings::Octave_Entry::Octave_Entry(String^ name, int octave)
	{
		Name = name;
		Octave_Number = octave;
	}


	//////////////////////
	// Settings Private //
	//////////////////////
	static Settings::Settings()
	{
		_Instance = nullptr;
		_Settings_File_Path = nullptr;
	}

	Settings::Settings()
	{
		_Octave_Entries = gcnew List<Octave_Entry^>();
		Load_Defaults();
	}

	void Settings::Load_Defaults()
	{
		// Add default note assignments
		_MIDI_Note_Red = 0;	// C
		_MIDI_Note_Green = 2;	// D
		_MIDI_Note_Blue = 4;	// E

		_MIDI_Export_Anti_Flicker = false;

		_ColorPresets = gcnew List<String^>();

		for (int i = 0; i < 10; i++) {
			_ColorPresets->Add("255,255,255"); // Default white color
		}

		_ColorPresets[0] = "255,0,0";		// Red
		_ColorPresets[1] = "0,127,0";		// Green
		_ColorPresets[2] = "0,0,255";		// Blue
		_ColorPresets[3] = "255,255,0";		// Yellow
		_ColorPresets[4] = "127,0,127";		// Purple
		_ColorPresets[5] = "255,0,255";		// Fuchsia
		_ColorPresets[6] = "0,255,255";		// Aqua
		_ColorPresets[7] = "127,127,127";	// Gray
		_ColorPresets[8] = "0,0,0";			// Black

		// Playback device defaults
		_Selected_MIDI_Output_Device = "";  // Empty string means use first available device
		_Global_MIDI_Output_Channel = 1;     // Default to MIDI channel 1
		_Selected_Audio_Output_Device = ""; // Empty string means use system default
		_Audio_Buffer_Size = 1024;           // Default buffer size

		// Recent files defaults
		_Recent_GP_Files = gcnew List<String^>();
		_Recent_Light_Files = gcnew List<String^>();
		_Recent_Audio_Files = gcnew List<String^>();
	}

	void Settings::Load_From_File()
	{
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

	String^ Settings::SerializeToString()
	{
		StringBuilder^ sb = gcnew StringBuilder();

		// Start JSON object
		sb->AppendLine("{");

		// Serialize hotkey bindings
		sb->AppendLine("  \"HotkeyBindings\": {");
		auto bindings = Hotkey_Manager::Instance->Get_All_Bindings();
		bool First_Binding = true;
		for each (KeyValuePair<String^, String^> pair in bindings) {
			if (!First_Binding) {
				sb->AppendLine(",");
			}
			sb->Append(String::Format("    \"{0}\": \"{1}\"",
				pair.Key->Replace("\"", "\\\""),
				pair.Value->Replace("\"", "\\\"")));
			First_Binding = false;
		}
		sb->AppendLine("");
		sb->AppendLine("  },");

		// Add MIDI settings
		sb->AppendLine(String::Format("  \"MidiNoteRed\": {0},", _MIDI_Note_Red));
		sb->AppendLine(String::Format("  \"MidiNoteGreen\": {0},", _MIDI_Note_Green));
		sb->AppendLine(String::Format("  \"MidiNoteBlue\": {0},", _MIDI_Note_Blue));
		sb->AppendLine(String::Format("  \"MidiExportAntiFlicker\": {0},", _MIDI_Export_Anti_Flicker ? "true" : "false"));

		// Add playback device settings
		sb->AppendLine(String::Format("  \"SelectedMidiOutputDevice\": \"{0}\",", _Selected_MIDI_Output_Device->Replace("\"", "\\\"")));
		sb->AppendLine(String::Format("  \"GlobalMidiOutputChannel\": {0},", _Global_MIDI_Output_Channel));
		sb->AppendLine(String::Format("  \"SelectedAudioOutputDevice\": \"{0}\",", _Selected_Audio_Output_Device->Replace("\"", "\\\"")));
		sb->AppendLine(String::Format("  \"AudioBufferSize\": {0},", _Audio_Buffer_Size));

		// Add octave entries
		sb->AppendLine("  \"OctaveEntries\": [");
		for (int i = 0; i < _Octave_Entries->Count; i++) {
			Octave_Entry^ entry = _Octave_Entries[i];
			sb->AppendLine(String::Format("    {{\"Name\": \"{0}\", \"Octave\": {1}}}{2}",
				entry->Name->Replace("\"", "\\\""),
				entry->Octave_Number,
				i < _Octave_Entries->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ],");

		// Add color presets
		sb->AppendLine("  \"ColorPresets\": [");
		for (int i = 0; i < _ColorPresets->Count; i++) {
			sb->AppendLine(String::Format("    \"{0}\"{1}",
				_ColorPresets[i]->Replace("\"", "\\\""),
				i < _ColorPresets->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ],");

		// Add recent GP files
		sb->AppendLine("  \"RecentGPFiles\": [");
		for (int i = 0; i < _Recent_GP_Files->Count; i++) {
			sb->AppendLine(String::Format("    \"{0}\"{1}",
				_Recent_GP_Files[i]->Replace("\\", "\\\\")->Replace("\"", "\\\""),
				i < _Recent_GP_Files->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ],");

		// Add recent Light files
		sb->AppendLine("  \"RecentLightFiles\": [");
		for (int i = 0; i < _Recent_Light_Files->Count; i++) {
			sb->AppendLine(String::Format("    \"{0}\"{1}",
				_Recent_Light_Files[i]->Replace("\\", "\\\\")->Replace("\"", "\\\""),
				i < _Recent_Light_Files->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ],");

		// Add recent Audio files
		sb->AppendLine("  \"RecentAudioFiles\": [");
		for (int i = 0; i < _Recent_Audio_Files->Count; i++) {
			sb->AppendLine(String::Format("    \"{0}\"{1}",
				_Recent_Audio_Files[i]->Replace("\\", "\\\\")->Replace("\"", "\\\""),
				i < _Recent_Audio_Files->Count - 1 ? "," : ""));
		}
		sb->AppendLine("  ]");

		// Close JSON object
		sb->AppendLine("}");
		return sb->ToString();
	}

	void Settings::DeserializeFromString(String^ data)
	{
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

			bool inRecentGPFiles = false;
			bool inRecentLightFiles = false;
			bool inRecentAudioFiles = false;
			_Recent_GP_Files->Clear();
			_Recent_Light_Files->Clear();
			_Recent_Audio_Files->Clear();

			for each (String ^ line in lines) {
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
					inColorPresets = false;
					inRecentGPFiles = false;
					inRecentLightFiles = false;
					inRecentAudioFiles = false;
					continue;
				}

				if (inOctaveEntries && currentLine->Contains("\"Name\"")) {
					String^ name = currentLine->Split('"')[3];
					String^ octaveStr = currentLine->Split(':')[2]->Trim(L'}', L' ', L',');
					int octave = Int32::Parse(octaveStr);
					_Octave_Entries->Add(gcnew Octave_Entry(name, octave));
					continue;
				}

				// Remove trailing commas and any remaining whitespace
				if (currentLine->EndsWith(","))
					currentLine = currentLine->Substring(0, currentLine->Length - 1)->Trim();

				// Check for hotkey bindings section
				if (currentLine == "\"HotkeyBindings\":{" || currentLine == "\"HotkeyBindings\": {") {
					inHotkeyBindings = true;
					continue;
				}

				if (currentLine == "}") {
					if (inHotkeyBindings) {
						inHotkeyBindings = false;
					}
					continue;
				}

				// Parse hotkey bindings
				if (inHotkeyBindings && currentLine->Contains(":")) {
					array<String^>^ parts = currentLine->Split(':');
					if (parts->Length >= 2) {
						String^ key = parts[0]->Trim()->Trim('"');
						String^ value = parts[1]->Trim()->Trim('"');
						hotkeyBindings[key] = value;
					}
				}

				// Parse MIDI settings
				if (currentLine->StartsWith("\"MidiNoteRed\":")) {
					String^ valueStr = currentLine->Split(':')[1]->Trim();
					_MIDI_Note_Red = Int32::Parse(valueStr);
				}
				else if (currentLine->StartsWith("\"MidiNoteGreen\":")) {
					String^ valueStr = currentLine->Split(':')[1]->Trim();
					_MIDI_Note_Green = Int32::Parse(valueStr);
				}
				else if (currentLine->StartsWith("\"MidiNoteBlue\":")) {
					String^ valueStr = currentLine->Split(':')[1]->Trim();
					_MIDI_Note_Blue = Int32::Parse(valueStr);
				}
				else if (currentLine->StartsWith("\"MidiExportAntiFlicker\":")) {
					String^ valueStr = currentLine->Split(':')[1]->Trim();
					_MIDI_Export_Anti_Flicker = (valueStr == "true");
				}
				// Parse playback device settings
				else if (currentLine->StartsWith("\"SelectedMidiOutputDevice\":")) {
					String^ valueStr = currentLine->Split(gcnew array<Char> {':'}, 2)[1]->Trim()->Trim('"');
					_Selected_MIDI_Output_Device = valueStr;
				}
				else if (currentLine->StartsWith("\"GlobalMidiOutputChannel\":")) {
					String^ valueStr = currentLine->Split(':')[1]->Trim();
					_Global_MIDI_Output_Channel = Int32::Parse(valueStr);
				}
				else if (currentLine->StartsWith("\"SelectedAudioOutputDevice\":")) {
					String^ valueStr = currentLine->Split(gcnew array<Char> {':'}, 2)[1]->Trim()->Trim('"');
					_Selected_Audio_Output_Device = valueStr;
				}
				else if (currentLine->StartsWith("\"AudioBufferSize\":")) {
					String^ valueStr = currentLine->Split(':')[1]->Trim();
					_Audio_Buffer_Size = Int32::Parse(valueStr);
				}

				// Check for Color Preset settings
				if (currentLine == "\"ColorPresets\": [") {
					inColorPresets = true;
					continue;
				}

				if (inColorPresets && currentLine->StartsWith("\"")) {
					String^ colorStr = currentLine->Trim(L'"', L' ', L',');
					_ColorPresets->Add(colorStr);
				}

				// Check for Recent GP Files
				if (currentLine == "\"RecentGPFiles\": [") {
					inRecentGPFiles = true;
					continue;
				}

				if (inRecentGPFiles && currentLine->StartsWith("\"")) {
					String^ filePath = currentLine->Trim(L'"', L' ', L',');
					filePath = filePath->Replace("\\\\", "\\");
					_Recent_GP_Files->Add(filePath);
				}

				// Check for Recent Light Files
				if (currentLine == "\"RecentLightFiles\": [") {
					inRecentLightFiles = true;
					continue;
				}

				if (inRecentLightFiles && currentLine->StartsWith("\"")) {
					String^ filePath = currentLine->Trim(L'"', L' ', L',');
					filePath = filePath->Replace("\\\\", "\\");
					_Recent_Light_Files->Add(filePath);
				}

				// Check for Recent Audio Files
				if (currentLine == "\"RecentAudioFiles\": [") {
					inRecentAudioFiles = true;
					continue;
				}

				if (inRecentAudioFiles && currentLine->StartsWith("\"")) {
					String^ filePath = currentLine->Trim(L'"', L' ', L',');
					filePath = filePath->Replace("\\\\", "\\");
					_Recent_Audio_Files->Add(filePath);
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


	/////////////////////
	// Settings Public //
	/////////////////////
	Settings^ Settings::Get_Instance()
	{
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

	void Settings::Initialize(String^ settingsFilePath)
	{
		_Settings_File_Path = settingsFilePath;
		Get_Instance();
	}

	void Settings::Save_To_File()
	{
		try {
			String^ JsonString = SerializeToString();
			File::WriteAllText(_Settings_File_Path, JsonString);
		}
		catch (Exception^ ex) {
			Console::WriteLine("Error saving settings: " + ex->Message);
		}
	}

	String^ Settings::Get_Hotkey_Binding(String^ action)
	{
		return Hotkey_Manager::Instance->Get_Binding(action);
	}

	void Settings::Set_Hotkey_Binding(String^ action, String^ key)
	{
		auto bindings = Hotkey_Manager::Instance->Get_All_Bindings();
		bindings[action] = key;
		Hotkey_Manager::Instance->Set_Bindings(bindings);

		Save_To_File();
	}


	/////////////////////////
	// Settings Properties //
	/////////////////////////
	int Settings::MIDI_Note_Red::get()
	{
		return _MIDI_Note_Red;
	}

	void Settings::MIDI_Note_Red::set(int value)
	{
		_MIDI_Note_Red = value;
		Save_To_File();
	}

	int Settings::MIDI_Note_Green::get()
	{
		return _MIDI_Note_Green;
	}

	void Settings::MIDI_Note_Green::set(int value)
	{
		_MIDI_Note_Green = value;
		Save_To_File();
	}

	int Settings::MIDI_Note_Blue::get()
	{
		return _MIDI_Note_Blue;
	}

	void Settings::MIDI_Note_Blue::set(int value)
	{
		_MIDI_Note_Blue = value;
		Save_To_File();
	}

	bool Settings::MIDI_Export_Anti_Flicker::get()
	{
		return _MIDI_Export_Anti_Flicker;
	}

	void Settings::MIDI_Export_Anti_Flicker::set(bool value)
	{
		_MIDI_Export_Anti_Flicker = value;
		Save_To_File();
	}

	List<Settings::Octave_Entry^>^ Settings::Octave_Entries::get()
	{
		return _Octave_Entries;
	}

	void Settings::Octave_Entries::set(List<Settings::Octave_Entry^>^ value)
	{
		_Octave_Entries = value;
		Save_To_File();
	}

	List<String^>^ Settings::ColorPresetsString::get()
	{
		return _ColorPresets;
	}

	void Settings::ColorPresetsString::set(List<String^>^ value)
	{
		_ColorPresets = value;
		Save_To_File();
	}

	List<Color>^ Settings::ColorPresetsColor::get()
	{
		List<Color>^ Colors = gcnew List<Color>();

		for each (String ^ ColorStr in _ColorPresets)
		{
			array<String^>^ Parts = ColorStr->Split(',');

			if (Parts->Length == 3)
			{
				int R = Int32::Parse(Parts[0]);
				int G = Int32::Parse(Parts[1]);
				int B = Int32::Parse(Parts[2]);

				Colors->Add(Color::FromArgb(R, G, B));
			}
		}

		return Colors;
	}

	// Playback Device Settings Properties
	String^ Settings::Selected_MIDI_Output_Device::get()
	{
		return _Selected_MIDI_Output_Device;
	}

	void Settings::Selected_MIDI_Output_Device::set(String^ value)
	{
		_Selected_MIDI_Output_Device = value;
		Save_To_File();
	}

	int Settings::Global_MIDI_Output_Channel::get()
	{
		return _Global_MIDI_Output_Channel;
	}

	void Settings::Global_MIDI_Output_Channel::set(int value)
	{
		if (value < 1 || value > 16)
			throw gcnew ArgumentOutOfRangeException("value", "MIDI channel must be between 1 and 16");

		_Global_MIDI_Output_Channel = value;
		Save_To_File();
	}

	String^ Settings::Selected_Audio_Output_Device::get()
	{
		return _Selected_Audio_Output_Device;
	}

	void Settings::Selected_Audio_Output_Device::set(String^ value)
	{
		_Selected_Audio_Output_Device = value;
		Save_To_File();
	}

	int Settings::Audio_Buffer_Size::get()
	{
		return _Audio_Buffer_Size;
	}

	void Settings::Audio_Buffer_Size::set(int value)
	{
		// Validate that buffer size is a power of 2 and within reasonable range
		if (value < 64 || value > 8192 || (value & (value - 1)) != 0) {
			throw gcnew ArgumentOutOfRangeException("value", "Audio buffer size must be a power of 2 between 64 and 8192");
		}

		_Audio_Buffer_Size = value;

		Save_To_File();
	}

	// Recent Files Properties
	List<String^>^ Settings::Recent_GP_Files::get()
	{
		return _Recent_GP_Files;
	}

	List<String^>^ Settings::Recent_Light_Files::get()
	{
		return _Recent_Light_Files;
	}

	List<String^>^ Settings::Recent_Audio_Files::get()
	{
		return _Recent_Audio_Files;
	}

	// Recent Files Methods
	void Settings::Add_Recent_GP_File(String^ filePath)
	{
		// Remove if already exists (to move to top)
		_Recent_GP_Files->Remove(filePath);

		// Insert at beginning
		_Recent_GP_Files->Insert(0, filePath);

		// Trim to max size
		while (_Recent_GP_Files->Count > MAX_RECENT_FILES) {
			_Recent_GP_Files->RemoveAt(_Recent_GP_Files->Count - 1);
		}

		Save_To_File();
	}

	void Settings::Add_Recent_Light_File(String^ filePath)
	{
		// Remove if already exists (to move to top)
		_Recent_Light_Files->Remove(filePath);

		// Insert at beginning
		_Recent_Light_Files->Insert(0, filePath);

		// Trim to max size
		while (_Recent_Light_Files->Count > MAX_RECENT_FILES) {
			_Recent_Light_Files->RemoveAt(_Recent_Light_Files->Count - 1);
		}

		Save_To_File();
	}

	void Settings::Add_Recent_Audio_File(String^ filePath)
	{
		// Remove if already exists (to move to top)
		_Recent_Audio_Files->Remove(filePath);

		// Insert at beginning
		_Recent_Audio_Files->Insert(0, filePath);

		// Trim to max size
		while (_Recent_Audio_Files->Count > MAX_RECENT_FILES) {
			_Recent_Audio_Files->RemoveAt(_Recent_Audio_Files->Count - 1);
		}

		Save_To_File();
	}
}
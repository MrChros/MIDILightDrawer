#include "pch.h"
#include "Hotkey_Manager.h"

namespace MIDILightDrawer {
	static Hotkey_Manager::Hotkey_Manager() {
		_Instance = nullptr;
	}

	Hotkey_Definition::Hotkey_Definition(String^ action, String^ default_key, String^ category) {
		Action = action;
		Default_Key = default_key;
		Category = category;
	}

	Hotkey_Manager::Hotkey_Manager() {
		_Hotkey_Definitions = gcnew List<Hotkey_Definition^>();
		_Current_Bindings = gcnew Dictionary<String^, String^>();
		Initialize_Default_Definitions();
	}

	void Hotkey_Manager::Initialize_Default_Definitions() {
		// Tools
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Select Tool", "S", "Tools"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Draw Tool", "D", "Tools"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Erase Tool", "E", "Tools"));
		//_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Fade Tool", "F", "Tools"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Color Tool", "C", "Tools"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Duration Tool", "X", "Tools"));
		//_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Bucket Tool", "B", "Tools"));

		// Colors
		for (int i = 1; i <= 10; i++) {
			_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Select Color " + i, (i % 10).ToString(), "Colors"));
		}

		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Draw/Length Quantization Up", "R", "Editing"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Draw/Length Quantization Down", "", "Editing"));

		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Zoom In", "", "View"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Zoom Out", "", "View"));
		_Hotkey_Definitions->Add(gcnew Hotkey_Definition("Reset Zoom", "", "View"));


		// Future Actions
		//array<String^>^ categories = { "Editing", "Navigation", "Settings" };
		//int categoryIndex = 0;
		//for (int i = 1; i <= 10; i++) {
		//	_Hotkey_Definitions->Add(gcnew Hotkey_Definition(
		//		"FutureAction" + i,
		//		"F" + i,
		//		categories[categoryIndex]));
		//	if (i % 4 == 0 && categoryIndex < 2) categoryIndex++;
		//}
	}

	Hotkey_Manager^ Hotkey_Manager::Instance::get() {
		if (_Instance == nullptr) {
			_Instance = gcnew Hotkey_Manager();
		}
		return _Instance;
	}

	List<Hotkey_Definition^>^ Hotkey_Manager::Definitions::get() {
		return _Hotkey_Definitions;
	}

	String^ Hotkey_Manager::Get_Binding(String^ action) {
		if (_Current_Bindings != nullptr && _Current_Bindings->ContainsKey(action)) {
			return _Current_Bindings[action];
		}
		for each (Hotkey_Definition ^ def in Definitions) {
			if (def->Action == action) return def->Default_Key;
		}
		return "";
	}

	void Hotkey_Manager::Set_Bindings(Dictionary<String^, String^>^ bindings) {
		_Current_Bindings = bindings;
	}

	Dictionary<String^, String^>^ Hotkey_Manager::Get_All_Bindings() {
		Dictionary<String^, String^>^ bindings = gcnew Dictionary<String^, String^>();
		for each (Hotkey_Definition ^ def in Definitions) {
			bindings[def->Action] = Get_Binding(def->Action);
		}
		return bindings;
	}
}
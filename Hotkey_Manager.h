#pragma once

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer {
	public ref class Hotkey_Definition {
	public:
		String^ Action;
		String^ Default_Key;
		String^ Category;

		Hotkey_Definition(String^ action, String^ default_key, String^ category);
	};

	public ref class Hotkey_Manager {
	private:
		static Hotkey_Manager^ _Instance;
		List<Hotkey_Definition^>^ _Hotkey_Definitions;
		Dictionary<String^, String^>^ _Current_Bindings;

		Hotkey_Manager();
		void Initialize_Default_Definitions();
		static Hotkey_Manager();

	public:
		static property Hotkey_Manager^ Instance {
			Hotkey_Manager^ get();
		}

		property List<Hotkey_Definition^>^ Definitions {
			List<Hotkey_Definition^>^ get();
		}

		String^ Get_Binding(String^ action);
		void Set_Bindings(Dictionary<String^, String^>^ bindings);
		Dictionary<String^, String^>^ Get_All_Bindings();
	};
}
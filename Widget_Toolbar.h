#pragma once

using namespace System::Collections::Generic;

#include "Widget_Timeline_Common.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer {

	using namespace System;
	using namespace System::Windows::Forms;
	using namespace System::Drawing;

	public ref class Widget_Toolbar : public UserControl {
	public:
		Widget_Toolbar();

		// Event that fires when tool selection changes
		event EventHandler<TimelineToolType>^ OnToolChanged;

		// Current selected tool
		property TimelineToolType CurrentTool {
			TimelineToolType get() { return _Current_Tool; }
			void set(TimelineToolType tool);
		}

	private:
		// Constants for layout
		literal int BUTTON_SIZE = 48;
		literal int BUTTON_PADDING = 4;
		literal int BUTTON_SPACING = 2;

		// Member variables
		System::Resources::ResourceManager^		_Resources;
		TimelineToolType						_Current_Tool;
		array<Button^>^							_Tool_Buttons;
		Dictionary<TimelineToolType, String^>^	_Tool_Icons;

		// Initialize controls and layout
		void InitializeComponent();
		void SetupToolIcons();
		void CreateToolButtons();

		// Event handlers
		void OnToolButtonClick(Object^ sender, EventArgs^ e);
		void UpdateButtonStates();

		// Helper methods
		void StyleButton(Button^ button, String^ iconName, String^ toolTip);
		Drawing::Image^ GetIconForTool(String^ iconName);
	};
}
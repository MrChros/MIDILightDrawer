#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;
using namespace System::Drawing;

namespace MIDILightDrawer {

	public ref class Widget_Toolbar : public System::Windows::Forms::UserControl
	{
	public:
		// Enum for tool types
		enum class ToolType
		{
			Selection,
			Draw,
			Erase,
			Fade,
			Duration,
			Change_Color,
			Bucket_Fill
		};

	private:
		List<Button^>^ _Toolbar_Buttons;
		Button^ _Selected_Button;
		ToolType _Current_Tool;
		ToolTip^ _ToolTip;
		Dictionary<ToolType, Image^>^ _Tool_Icons;

		void Initialize_Component();
		void Initialize_Buttons();
		Button^ Create_Tool_Button(ToolType toolType, String^ name);
		void Set_Tool_Icon(ToolType tool, Image^ icon);
		String^ Get_Tooltip_Text(ToolType toolType);
		void Button_Click(Object^ sender, EventArgs^ e);

	public:
		Widget_Toolbar();

		// Event that fires when a tool is selected
		event EventHandler<ToolType>^ ToolChanged;

		// Get the currently selected tool
		ToolType Get_Current_Tool();

		// Programmatically select a button by index
		void Select_Button(int index);
	};

	public delegate void ToolChangedEventHandler(Widget_Toolbar::ToolType newTool);
}
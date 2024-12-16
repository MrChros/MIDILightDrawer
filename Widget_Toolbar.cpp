#include "Widget_Toolbar.h"

namespace MIDILightDrawer {

	Widget_Toolbar::Widget_Toolbar()
	{
		Initialize_Component();
		_Toolbar_Buttons = gcnew List<Button^>();
		_Tool_Icons = gcnew Dictionary<ToolType, Image^>();

		_ToolTip = gcnew ToolTip();
		_ToolTip->InitialDelay = 500;    // Half second delay before showing
		_ToolTip->ReshowDelay = 100;     // Short delay when moving between controls
		_ToolTip->ShowAlways = true;     // Show tooltip even when form is inactive
		_ToolTip->UseAnimation = true;
		_ToolTip->UseFading = true;

		Initialize_Buttons();
	}

	void Widget_Toolbar::Initialize_Component()
	{
		this->AutoSize = true;
		this->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		this->Dock = System::Windows::Forms::DockStyle::Top;
	}

	void Widget_Toolbar::Initialize_Buttons()
	{
		System::Resources::ResourceManager^ Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
		
		Create_Tool_Button(ToolType::Selection, "btnSelection");
		Set_Tool_Icon(ToolType::Selection, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Select"))));

		Create_Tool_Button(ToolType::Draw, "btnDraw");
		Set_Tool_Icon(ToolType::Draw, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Pen"))));

		Create_Tool_Button(ToolType::Erase, "btnErase");
		Set_Tool_Icon(ToolType::Erase, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Eraser"))));

		Create_Tool_Button(ToolType::Fade, "btnFade");
		Set_Tool_Icon(ToolType::Fade, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Fade"))));
		
		Create_Tool_Button(ToolType::Duration, "btnChangeLength");
		Set_Tool_Icon(ToolType::Duration, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Length"))));

		Create_Tool_Button(ToolType::Change_Color, "btnChangeColor");
		Set_Tool_Icon(ToolType::Change_Color, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Color"))));

		Create_Tool_Button(ToolType::Bucket_Fill, "btnBuccketFill");
		Set_Tool_Icon(ToolType::Bucket_Fill, (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Cursor_Bucket"))));

		// Select Selection tool by default
		Select_Button(0);
	}

	Button^ Widget_Toolbar::Create_Tool_Button(ToolType toolType, String^ name)
	{
		Button^ button = gcnew Button();
		button->Name = name;
		button->Width = 40;  // Reduced width since we're using icons
		button->Height = 40;  // Made square for icons
		button->FlatStyle = FlatStyle::Standard;

		// Set the image if available
		if (_Tool_Icons->ContainsKey(toolType)) {
			button->Image = _Tool_Icons[toolType];
			button->ImageAlign = System::Drawing::ContentAlignment::MiddleCenter;
		}

		button->Tag = toolType;

		// Set tooltip
		String^ Tooltip_Text = Get_Tooltip_Text(toolType);
		_ToolTip->SetToolTip(button, Tooltip_Text);

		// Calculate position for the new button
		if (_Toolbar_Buttons->Count > 0)
		{
			Button^ lastButton = _Toolbar_Buttons[_Toolbar_Buttons->Count - 1];
			button->Left = lastButton->Right + 5;
		}
		else
		{
			button->Left = 5;
		}
		button->Top = 5;

		// Add click event handler
		button->Click += gcnew EventHandler(this, &Widget_Toolbar::Button_Click);

		// Add to controls and internal list
		this->Controls->Add(button);
		_Toolbar_Buttons->Add(button);

		return button;
	}

	void Widget_Toolbar::Set_Tool_Icon(ToolType tool, Image^ icon)
	{
		_Tool_Icons[tool] = icon;

		// Update existing button if it exists
		for each (Button ^ btn in _Toolbar_Buttons)
		{
			if (safe_cast<ToolType>(btn->Tag) == tool)
			{
				btn->Image = icon;
				btn->ImageAlign = System::Drawing::ContentAlignment::MiddleCenter;
				break;
			}
		}
	}

	String^ Widget_Toolbar::Get_Tooltip_Text(ToolType toolType)
	{
		switch (toolType)
		{
		case ToolType::Selection:		return "Selection Tool";
		case ToolType::Draw:			return "Draw Tool";
		case ToolType::Erase:			return "Erase Tool";
		case ToolType::Fade:			return "Fade Tool";
		case ToolType::Duration:	return "Change Length Tool";
		case ToolType::Change_Color:	return "Color Change Tool";
		case ToolType::Bucket_Fill:		return "Bucket Fill Tool";
		default:
			return "";
		}
	}

	void Widget_Toolbar::Button_Click(Object^ sender, EventArgs^ e)
	{
		Button^ Clicked_Button = safe_cast<Button^>(sender);

		// If the clicked button is already selected, do nothing
		if (_Selected_Button == Clicked_Button)
			return;

		// Unselect all buttons
		for each (Button ^ btn in _Toolbar_Buttons)
		{
			btn->BackColor = System::Drawing::SystemColors::Control;
			btn->FlatStyle = FlatStyle::Standard;
		}

		// Select clicked button
		Clicked_Button->BackColor = System::Drawing::SystemColors::ActiveCaption;
		Clicked_Button->FlatStyle = FlatStyle::Flat;
		_Selected_Button = Clicked_Button;

		// Update current tool
		_Current_Tool = safe_cast<ToolType>(Clicked_Button->Tag);

		// Raise the ToolChanged event
		ToolChanged(this, _Current_Tool);
	}

	Widget_Toolbar::ToolType Widget_Toolbar::Get_Current_Tool()
	{
		return _Current_Tool;
	}

	void Widget_Toolbar::Select_Button(int index)
	{
		if (index >= 0 && index < _Toolbar_Buttons->Count)
		{
			Button_Click(_Toolbar_Buttons[index], EventArgs::Empty);
		}
	}
}
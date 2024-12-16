#include "Control_ColorPreset.h"

namespace MIDILightDrawer {
	Control_ColorPreset::Control_ColorPreset(void)
	{
		Initialize_Component();
		Load_PresetColors();
	}

	void Control_ColorPreset::SelectedIndex::set(int index) {
		// Check if index is within valid range
		if (index >= 0 && index < _Color_Preset_Buttons->Length) {
			// Set the selected color to the color of the button at the given index
			this->SelectedColor = _Color_Preset_Buttons[index]->BackColor;
			// Trigger the color changed event
			OnSelectedColorChanged();
		}
	}

	void Control_ColorPreset::Initialize_Component(void)
	{
		TableLayoutPanel^ Panel = gcnew TableLayoutPanel();
		Panel->ColumnCount = 10;  // 10 columns for all buttons
		Panel->RowCount = 2;      // 1 row for color buttons, 1 row for set buttons
		Panel->AutoSize = true;
		Panel->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		Panel->Dock = DockStyle::Fill;

		// Configure columns with fixed widths
		for (int i = 0; i < 10; i++) {
			Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 45));  // 40px button + 5px margin
		}

		// Configure rows with fixed heights
		Panel->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));  // Color buttons row
		Panel->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));  // Set buttons row

		// Initialize button arrays
		_Color_Preset_Buttons = gcnew array<Button^>(10);
		_Set_Buttons = gcnew array<Button^>(10);

		// Create all buttons
		for (int i = 0; i < 10; i++) {
			// Color preset button
			Button^ Color_Button = gcnew Button();
			Color_Button->Size = System::Drawing::Size(40, 40);
			Color_Button->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			Color_Button->FlatAppearance->BorderSize = 1;
			Color_Button->Margin = System::Windows::Forms::Padding(2);
			Color_Button->Anchor = static_cast<System::Windows::Forms::AnchorStyles>
				(System::Windows::Forms::AnchorStyles::Top |
					System::Windows::Forms::AnchorStyles::Left);
			Color_Button->BackColor = Color::White;
			Color_Button->Click += gcnew System::EventHandler(this, &Control_ColorPreset::Color_Preset_Button_Click);
			_Color_Preset_Buttons[i] = Color_Button;
			Panel->Controls->Add(Color_Button, i, 0);

			// Set button
			Button^ Set_Button = gcnew Button();
			Set_Button->Text = "Set";
			Set_Button->Size = System::Drawing::Size(40, 25);
			Set_Button->Margin = System::Windows::Forms::Padding(2);
			Set_Button->Font = gcnew System::Drawing::Font(Set_Button->Font->FontFamily, 8.0f);
			Set_Button->Anchor = static_cast<System::Windows::Forms::AnchorStyles>
				(System::Windows::Forms::AnchorStyles::Top |
					System::Windows::Forms::AnchorStyles::Left);
			Set_Button->Tag = i;
			Set_Button->Click += gcnew System::EventHandler(this, &Control_ColorPreset::Set_Button_Click);
			_Set_Buttons[i] = Set_Button;
			Panel->Controls->Add(Set_Button, i, 1);
		}

		this->Controls->Add(Panel);
	}

	void Control_ColorPreset::Save_PresetColors()
	{
		Settings^ settings = Settings::Get_Instance();
		System::Collections::Generic::List<String^>^ colorList = gcnew System::Collections::Generic::List<String^>();

		for (int i = 0; i < _Color_Preset_Buttons->Length; i++) {
			Color color = _Color_Preset_Buttons[i]->BackColor;
			String^ colorString = String::Format("{0},{1},{2}", color.R, color.G, color.B);
			colorList->Add(colorString);
		}

		settings->ColorPresets = colorList;
		//OnPresetColorsChanged();
	}

	void Control_ColorPreset::Load_PresetColors()
	{
		Settings^ settings = Settings::Get_Instance();
		auto colorList = settings->ColorPresets;

		if (colorList != nullptr && colorList->Count == 10) {
			for (int i = 0; i < _Color_Preset_Buttons->Length; i++) {
				array<String^>^ rgb = colorList[i]->Split(',');
				if (rgb->Length == 3) {
					Color color = Color::FromArgb(
						Int32::Parse(rgb[0]),
						Int32::Parse(rgb[1]),
						Int32::Parse(rgb[2])
					);
					_Color_Preset_Buttons[i]->BackColor = color;
				}
			}
		}
	}

	void Control_ColorPreset::Color_Preset_Button_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ Color_Button = safe_cast<Button^>(sender);
		this->SelectedColor = Color_Button->BackColor;
		OnSelectedColorChanged();
	}

	void Control_ColorPreset::Set_Button_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ Set_Button = safe_cast<Button^>(sender);
		int index = safe_cast<int>(Set_Button->Tag);
		_Color_Preset_Buttons[index]->BackColor = this->_Selected_Color;
		Save_PresetColors();
	}
}
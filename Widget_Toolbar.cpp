#include "Widget_Toolbar.h"

namespace MIDILightDrawer {

	Widget_Toolbar::Widget_Toolbar()
	{
		_Resources		= gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
		_Current_Tool	= TimelineToolType::Pointer;

		InitializeComponent();
	}

	void Widget_Toolbar::InitializeComponent() {
		// Basic control setup
		this->AutoSize = true;
		this->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		this->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
		this->Padding = System::Windows::Forms::Padding(4);

		// Initialize collections
<<<<<<< HEAD
		_Tool_Buttons = gcnew List<Button^>;
=======
		_Tool_Buttons = gcnew array<Button^>(6); // One for each tool type
>>>>>>> 005f683fa6889f25d21c7c95edf25278c7baf8d3
		_Tool_Icons = gcnew Dictionary<TimelineToolType, String^>();

		// Setup icons and create buttons
		SetupToolIcons();
		CreateToolButtons();

		// Initial state
		UpdateButtonStates();
	}

	void Widget_Toolbar::CurrentTool::set(TimelineToolType tool)
	{
		if (_Current_Tool != tool)
		{
			_Current_Tool = tool;
			UpdateButtonStates();
			OnToolChanged(this, _Current_Tool);
		}
	}

	void Widget_Toolbar::SetupToolIcons() {
		// Map tools to their corresponding icon names
		// Note: These are placeholder names - replace with actual icon resources
		_Tool_Icons->Add(TimelineToolType::Pointer	, "Pointer_White"	);
		_Tool_Icons->Add(TimelineToolType::Draw		, "Draw_White"		);
<<<<<<< HEAD
		//_Tool_Icons->Add(TimelineToolType::Split	, "Split_White"		);
		_Tool_Icons->Add(TimelineToolType::Erase	, "Erase_White"		);
		_Tool_Icons->Add(TimelineToolType::Duration	, "Duration_White"	);
		_Tool_Icons->Add(TimelineToolType::Color	, "Color_White"		);
		_Tool_Icons->Add(TimelineToolType::Fade		, "Fade_White"		);
		_Tool_Icons->Add(TimelineToolType::Strobe	, "Strobe_White"	);
=======
		_Tool_Icons->Add(TimelineToolType::Split	, "Split_White"		);
		_Tool_Icons->Add(TimelineToolType::Erase	, "Erase_White"		);
		_Tool_Icons->Add(TimelineToolType::Duration	, "Duration_White"	);
		_Tool_Icons->Add(TimelineToolType::Color	, "Color_White"		);
>>>>>>> 005f683fa6889f25d21c7c95edf25278c7baf8d3
	}

	void Widget_Toolbar::CreateToolButtons()
	{
		FlowLayoutPanel^ toolPanel = gcnew FlowLayoutPanel();
		toolPanel->AutoSize = true;
		toolPanel->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		toolPanel->FlowDirection = FlowDirection::LeftToRight;
		toolPanel->WrapContents = false;
		toolPanel->Padding = System::Windows::Forms::Padding(1, 0, 0, 0);
		toolPanel->Margin = System::Windows::Forms::Padding(0, 0, 0, 0);
		toolPanel->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;

		// Create a button for each tool
		array<TimelineToolType>^ tools = {
			TimelineToolType::Pointer,
			TimelineToolType::Draw,
			TimelineToolType::Split,
			TimelineToolType::Erase,
			TimelineToolType::Duration,
<<<<<<< HEAD
			TimelineToolType::Color,
			TimelineToolType::Fade
		};

		//for (int i = 0; i < tools->Length; i++)
		for each (KeyValuePair<TimelineToolType, String^> toolEntry in _Tool_Icons)
		{
=======
			TimelineToolType::Color
		};

		for (int i = 0; i < tools->Length; i++) {
>>>>>>> 005f683fa6889f25d21c7c95edf25278c7baf8d3
			Button^ btn = gcnew Button();
			btn->Size		= Drawing::Size(BUTTON_SIZE, BUTTON_SIZE);
			btn->Margin		= System::Windows::Forms::Padding(BUTTON_SPACING);
			btn->FlatStyle	= FlatStyle::Flat;
<<<<<<< HEAD
			btn->Tag		= toolEntry.Key;
=======
			btn->Tag		= tools[i];
>>>>>>> 005f683fa6889f25d21c7c95edf25278c7baf8d3
			btn->Text		= "";
			btn->UseVisualStyleBackColor = false;
			btn->Click += gcnew EventHandler(this, &Widget_Toolbar::OnToolButtonClick);

			// Style the button using Theme Manager
			Theme_Manager::Get_Instance()->ApplyThemeToButton(btn);

			// Set icon and tooltip
<<<<<<< HEAD
			StyleButton(btn, toolEntry.Value, toolEntry.Key.ToString());

			_Tool_Buttons->Add(btn);
=======
			StyleButton(btn, _Tool_Icons[tools[i]], tools[i].ToString());

			_Tool_Buttons[i] = btn;
>>>>>>> 005f683fa6889f25d21c7c95edf25278c7baf8d3
			toolPanel->Controls->Add(btn);
		}

		this->Controls->Add(toolPanel);
	}

	void Widget_Toolbar::StyleButton(Button^ button, String^ iconName, String^ toolTip)
	{
		// Set up the button's appearance
		button->ImageAlign			= ContentAlignment::MiddleCenter;
		button->TextAlign			= ContentAlignment::MiddleCenter;
		button->TextImageRelation	= TextImageRelation::ImageBeforeText;
		button->Padding				= System::Windows::Forms::Padding(0);
		button->Image				= GetIconForTool(iconName);

		// Create and assign tooltip
		ToolTip^ tip = gcnew ToolTip();
		tip->SetToolTip(button, toolTip);
	}

	void Widget_Toolbar::OnToolButtonClick(Object^ sender, EventArgs^ e)
	{
		Button^ clickedButton = safe_cast<Button^>(sender);
		TimelineToolType newTool = safe_cast<TimelineToolType>(clickedButton->Tag);

		if (_Current_Tool != newTool) {
			_Current_Tool = newTool;
			UpdateButtonStates();

			// Raise the tool changed event
			OnToolChanged(this, _Current_Tool);
		}
	}

	void Widget_Toolbar::UpdateButtonStates()
	{
		for each (Button ^ btn in _Tool_Buttons) {
			TimelineToolType buttonTool = safe_cast<TimelineToolType>(btn->Tag);
			if (buttonTool == _Current_Tool) {
				btn->BackColor = Theme_Manager::Get_Instance()->AccentPrimary;
			}
			else {
				btn->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
			}
		}
	}

	Drawing::Image^ Widget_Toolbar::GetIconForTool(String^ iconName)
	{
		try
		{
			Drawing::Image^ originalImage = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(iconName)));

			// Calculate the target size (slightly smaller than button for padding)
			int targetSize = (int)(BUTTON_SIZE * 0.7);  // Increased padding for better appearance

			// Create a new bitmap at the target size
			Bitmap^ scaledImage = gcnew Bitmap(targetSize, targetSize);

			// Use high quality scaling
			Graphics^ g = Graphics::FromImage(scaledImage);
			g->InterpolationMode	= Drawing2D::InterpolationMode::HighQualityBicubic;
			g->SmoothingMode		= Drawing2D::SmoothingMode::HighQuality;
			g->PixelOffsetMode		= Drawing2D::PixelOffsetMode::HighQuality;
			g->CompositingQuality	= Drawing2D::CompositingQuality::HighQuality;

			// Clear the background (make it transparent)
			g->Clear(Color::Transparent);

			// Draw the scaled image centered in the bitmap
			Rectangle destRect	= Rectangle(0, 0, targetSize, targetSize);
			Rectangle srcRect	= Rectangle(0, 0, originalImage->Width, originalImage->Height);
			g->DrawImage(originalImage, destRect, srcRect, GraphicsUnit::Pixel);

			delete g;
			delete originalImage;

			return scaledImage;
		}
<<<<<<< HEAD
		catch (...)
=======
		catch (Exception^ ex)
>>>>>>> 005f683fa6889f25d21c7c95edf25278c7baf8d3
		{
			// Create a simple placeholder if image loading fails
			int targetSize = BUTTON_SIZE - 12;
			Bitmap^ placeholder = gcnew Bitmap(targetSize, targetSize);
			Graphics^ g = Graphics::FromImage(placeholder);
			g->Clear(Color::Transparent);

			// Draw a simple shape as placeholder
			Pen^ pen = gcnew Pen(Color::White, 2);
			g->DrawRectangle(pen, 2, 2, targetSize - 4, targetSize - 4);

			delete pen;
			delete g;
			return placeholder;
		}
	}
}
#include "Version.h"
#include "Form_Main.h"
#include "Theme_Manager.h"

using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Runtime::InteropServices;

namespace MIDILightDrawer
{

	Form_Main::Form_Main(void)
	{
		// Basic form setup with modern styling
		this->Text			= "MIDI Light Drawer";
		this->Size			= System::Drawing::Size(1200, 800);
		this->MinimumSize	= System::Drawing::Size(1200, 800);
		this->Padding		= System::Windows::Forms::Padding(1); // Border padding

		// Register form closing event
		this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &Form_Main::Form_Main_FormClosing);

		_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
		Settings::Initialize("settings.json");

		// Create main container panel
		Panel^ mainContainer = gcnew Panel();
		mainContainer->Dock = DockStyle::Fill;
		mainContainer->Padding = System::Windows::Forms::Padding(10);

		// Create and configure main layout
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->Dock = DockStyle::Fill;
		Table_Layout_Main->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;
		Table_Layout_Main->BackColor = Theme_Manager::Get_Instance()->Background;

		// Configure table layout
		Table_Layout_Main->RowCount = 3;
		Table_Layout_Main->ColumnCount = 1;

		// Configure row styles with better proportions
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 260)); // Tools section
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));  // Timeline
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 60));  // Bottom controls

		// Configure column styles
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));

		// Create tools container with styling
		Panel^ toolsContainer = gcnew Panel();
		toolsContainer->Dock = DockStyle::Fill;
		toolsContainer->Padding = System::Windows::Forms::Padding(0, 0, 0, 10);
		toolsContainer->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
		toolsContainer->BorderStyle = BorderStyle::FixedSingle;


		////////////////////////
		// Tools and Controls //
		////////////////////////
		this->_Tools_And_Control = gcnew Widget_Tools_And_Control();
		this->_Tools_And_Control->Dock = DockStyle::Fill;
		this->_Tools_And_Control->Margin = System::Windows::Forms::Padding(10);
		toolsContainer->Controls->Add(this->_Tools_And_Control);

		Table_Layout_Main->Controls->Add(toolsContainer, 0, 0);
		Table_Layout_Main->SetColumnSpan(toolsContainer, Table_Layout_Main->ColumnCount);

		// Initialize toolbar and connect events
		this->_Toolbar = this->_Tools_And_Control->Get_Widget_Toolbar();
		this->_Toolbar->OnToolChanged += gcnew System::EventHandler<TimelineToolType>(this, &Form_Main::Toolbar_OnToolChanged);

		// Initialize other tool options
		InitializeToolOptions();


		//////////////////////
		// Timeline Section //
		//////////////////////
		Panel^ timelineContainer = gcnew Panel();
		timelineContainer->Dock = DockStyle::Fill;
		timelineContainer->Padding = System::Windows::Forms::Padding(0);
		timelineContainer->BackColor = Theme_Manager::Get_Instance()->BackgroundLight;
		timelineContainer->BorderStyle = BorderStyle::FixedSingle;

		this->_Timeline = gcnew Widget_Timeline();
		this->_Timeline->Dock = System::Windows::Forms::DockStyle::Fill;
		this->_Timeline->Name = L"timeline";
		this->_Timeline->Theme = Theme_Manager::Get_Instance()->GetTimelineTheme();
		timelineContainer->Controls->Add(this->_Timeline);

		Table_Layout_Main->Controls->Add(timelineContainer, 0, 1);
		Table_Layout_Main->SetColumnSpan(timelineContainer, Table_Layout_Main->ColumnCount);


		///////////////////////////
		// Bottom Controls Panel //
		///////////////////////////
		Panel^ bottomControlsPanel = gcnew Panel();
		bottomControlsPanel->Dock = DockStyle::Fill;
		bottomControlsPanel->Padding = System::Windows::Forms::Padding(0, 10, 0, 0);
		bottomControlsPanel->BackColor = Theme_Manager::Get_Instance()->BackgroundAlt;

		// Configure and add bottom controls
		InitializeBottomControls(bottomControlsPanel);
		Table_Layout_Main->Controls->Add(bottomControlsPanel, 0, 2);
		Table_Layout_Main->SetColumnSpan(bottomControlsPanel, Table_Layout_Main->ColumnCount);

		mainContainer->Controls->Add(Table_Layout_Main);
		this->Controls->Add(mainContainer);

		// Initialize menu
		InitializeMainMenu();

		// Apply theme
		//Theme_Manager::Get_Instance()->ApplyTheme(this);
		this->BackColor = Theme_Manager::Get_Instance()->BackgroundLight;
		Theme_Manager::Get_Instance()->ApplyThemeToMenuStrip(this->_Menu_Strip);

		// Initialize hotkeys and settings
		Initialize_Hotkeys();
		OnMidiSettingsAccepted();

	#ifdef _DEBUG

	#endif

	}

	Form_Main::~Form_Main()
	{

	}

	void Form_Main::InitializeBottomControls(Panel^ container)
	{
		TableLayoutPanel^ bottomLayout = gcnew TableLayoutPanel();
		bottomLayout->Dock = DockStyle::Fill;
		bottomLayout->ColumnCount = 4;
		bottomLayout->RowCount = 1;

		// Configure column styles
		bottomLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		bottomLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 170));
		bottomLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 270));
		bottomLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 170));

		bottomLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.f));

		// Create the Tablature Info Widget
		this->_Tab_Info = gcnew Widget_Tab_Info();
		this->_Tab_Info->Dock = DockStyle::Fill;


		// Track Preset Widget
		array<String^>^ Lines_First_Track_Height	= gcnew array<String^>	{ "Compact", "Normal", "Large", "Extended"	};
		array<String^>^ Lines_Second_Track_Height	= gcnew array<String^>	{ "", "", "", "" };
		array<int>^		Values_Track_Height			= gcnew array<int>		{ _Timeline->MINIMUM_TRACK_HEIGHT, _Timeline->DEFAULT_TRACK_HEIGHT, (int)(_Timeline->DEFAULT_TRACK_HEIGHT * 1.5), (int)(_Timeline->DEFAULT_TRACK_HEIGHT * 2.5)};

		this->_DropDown_Track_Height = gcnew Control_DropDown();
		this->_DropDown_Track_Height->Dock = DockStyle::Fill;
		this->_DropDown_Track_Height->Set_Tile_Layout(161, 30, 1);
		this->_DropDown_Track_Height->Title_Text = "Set Track Height";
		this->_DropDown_Track_Height->Set_Title_Color(Theme_Manager::Get_Instance()->ForegroundText);
		this->_DropDown_Track_Height->Set_Open_Direction(true);
		this->_DropDown_Track_Height->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Track_Height->Set_Items(Lines_First_Track_Height, Lines_Second_Track_Height, Values_Track_Height);
		this->_DropDown_Track_Height->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_Main::DropDown_Track_Height_OnItem_Selected);

		// Style the marker dropdown
		this->_DropDown_Marker = gcnew Control_DropDown();
		this->_DropDown_Marker->Dock = DockStyle::Fill;
		this->_DropDown_Marker->Set_Tile_Layout(261, 30, 1);
		this->_DropDown_Marker->Title_Text = "Jump to Marker";
		this->_DropDown_Marker->Set_Title_Color(Theme_Manager::Get_Instance()->ForegroundText);
		this->_DropDown_Marker->Set_Open_Direction(true);
		this->_DropDown_Marker->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Marker->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_Main::DropDown_View_Marker_OnItem_Selected);

		// Configure zoom control with container for right alignment
		Panel^ zoomPanel = gcnew Panel();
		zoomPanel->Dock			= DockStyle::Fill;

		array<double>^ Zoom_Values = { 0.1, 0.25, 0.5, 0.75, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		_TrackBar_Zoom = gcnew Control_TrackBar_Zoom();
		_TrackBar_Zoom->Dock = DockStyle::Fill;
		_TrackBar_Zoom->Slider_Color = Theme_Manager::Get_Instance()->AccentPrimary;
		_TrackBar_Zoom->Set_Values(Zoom_Values);
		_TrackBar_Zoom->Value_Changed += gcnew EventHandler<Track_Bar_Value_Changed_Event_Args^>(this, &Form_Main::TrackBar_Zoom_OnValue_Changed);
		_TrackBar_Zoom->Value = 1;

		// Add zoom control to its container
		zoomPanel->Controls->Add(_TrackBar_Zoom);

		// Add controls to layout with margins
		bottomLayout->Controls->Add(this->_Tab_Info				, 0, 0);
		bottomLayout->Controls->Add(this->_DropDown_Track_Height, 1, 0);
		bottomLayout->Controls->Add(this->_DropDown_Marker		, 2, 0);
		bottomLayout->Controls->Add(zoomPanel					, 3, 0);

		container->Controls->Add(bottomLayout);
	}

	void Form_Main::InitializeToolOptions()
	{
		// Pointer Options
		this->_Pointer_Options = this->_Tools_And_Control->Get_Widget_Pointer_Options();
		this->_Pointer_Options->SnappingChanged += gcnew QuantizationChangedHandler(this, &Form_Main::Pointer_Options_OnSnappingChanged);
		
		// Draw Options
		this->_Draw_Options = this->_Tools_And_Control->Get_Widget_Draw_Options();
		this->_Draw_Options->SnappingChanged	+= gcnew QuantizationChangedHandler	(this, &Form_Main::Draw_Options_OnSnappingChanged);
		this->_Draw_Options->LengthChanged		+= gcnew QuantizationChangedHandler	(this, &Form_Main::Draw_Options_OnLengthChanged);
		this->_Draw_Options->ConsiderTabChanged += gcnew ConsiderTabChangedHandler	(this, &Form_Main::Draw_Options_OnConsiderTabChanged);
		this->_Draw_Options->ColorChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Draw_Options_OnColorChanged);

		// Length Options
		this->_Length_Options = this->_Tools_And_Control->Get_Widget_Length_Options();
		this->_Length_Options->QuantizationChanged += gcnew QuantizationChangedHandler(this, &Form_Main::Length_Options_OnLengthChanged);

		// Color Options
		this->_Color_Options = this->_Tools_And_Control->Get_Widget_Color_Options();
		this->_Color_Options->ColorChanged += gcnew ColorChangedHandler(this, &Form_Main::Color_Options_OnColorChanged);

		// Fade Options
		this->_Fade_Options = this->_Tools_And_Control->Get_Widget_Fade_Options();
		this->_Fade_Options->QuantizationChanged	+= gcnew QuantizationChangedHandler	(this, &Form_Main::Fade_Options_OnLengthChanged);
		this->_Fade_Options->ColorStartChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Fade_Options_OnColorStartChanged);
		this->_Fade_Options->ColorEndChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Fade_Options_OnColorEndChanged);
		this->_Fade_Options->ColorCenterChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Fade_Options_OnColorCenterChanged);
		this->_Fade_Options->FadeModeChanged		+= gcnew FadeModeChangedHandler		(this, &Form_Main::Fade_Options_OnFadeModeChanged);

		// Strobe Options
		this->_Strobe_Options = this->_Tools_And_Control->Get_Widget_Strobe_Options();
		this->_Strobe_Options->QuantizationChanged += gcnew QuantizationChangedHandler(this, &Form_Main::Strobe_Options_OnLengthChanged);
		this->_Strobe_Options->ColorChanged += gcnew ColorChangedHandler(this, &Form_Main::Strobe_Options_OnColorChanged);

		// Bucket Options
		this->_Bucket_Options = this->_Tools_And_Control->Get_Widget_Bucket_Options();
		this->_Bucket_Options->ColorChanged += gcnew ColorChangedHandler(this, &Form_Main::Bucket_Options_OnColorChanged);
	}

	void Form_Main::InitializeMainMenu()
	{
		this->_Menu_Strip = gcnew MenuStrip();
		this->_Menu_Strip->Padding = System::Windows::Forms::Padding(6, 2, 0, 2);

		// File Menu
		ToolStripMenuItem^ Menu_File = gcnew ToolStripMenuItem("File");
		Menu_File->Padding = System::Windows::Forms::Padding(4, 0, 4, 0);

		// File -> Open Guitar Pro
		ToolStripMenuItem^ Menu_File_Open_GP = gcnew ToolStripMenuItem("Open Guitar Pro 5 File");
		Menu_File_Open_GP->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"GP5")));
		Menu_File_Open_GP->ShortcutKeys = Keys::Control | Keys::O;
		Menu_File_Open_GP->Click += gcnew System::EventHandler(this, &Form_Main::Menu_File_Open_GP_Click);

		// File -> Open Light
		ToolStripMenuItem^ Menu_File_Open_Light = gcnew ToolStripMenuItem("Open Light Information File");
		Menu_File_Open_Light->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Light_Open")));
		Menu_File_Open_Light->ShortcutKeys = Keys::Control | Keys::Shift | Keys::O;
		Menu_File_Open_Light->Click += gcnew System::EventHandler(this, &Form_Main::Menu_File_Open_Light_Click);

		// File -> Save Light
		ToolStripMenuItem^ Menu_File_Save_Light = gcnew ToolStripMenuItem("Save Light Information");
		Menu_File_Save_Light->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Save")));
		Menu_File_Save_Light->ShortcutKeys = Keys::Control | Keys::S;
		Menu_File_Save_Light->Click += gcnew System::EventHandler(this, &Form_Main::Menu_File_Save_Light_Click);

		// File -> Export MIDI
		ToolStripMenuItem^ Menu_File_Export_MIDI = gcnew ToolStripMenuItem("Export to MIDI File");
		Menu_File_Export_MIDI->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Midi")));
		Menu_File_Export_MIDI->ShortcutKeys = Keys::Control | Keys::E;
		Menu_File_Export_MIDI->Click += gcnew System::EventHandler(this, &Form_Main::Menu_File_Export_MIDI_Click);

		// File -> Exit
		ToolStripMenuItem^ Menu_File_Exit = gcnew ToolStripMenuItem("Exit");
		Menu_File_Exit->ShortcutKeys = Keys::Alt | Keys::F4;
		Menu_File_Exit->Click += gcnew System::EventHandler(this, &Form_Main::Menu_File_Exit_Click);

		// Build File menu
		Menu_File->DropDownItems->Add(Menu_File_Open_GP);
		Menu_File->DropDownItems->Add(Menu_File_Open_Light);
		Menu_File->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_File->DropDownItems->Add(Menu_File_Save_Light);
		Menu_File->DropDownItems->Add(Menu_File_Export_MIDI);
		Menu_File->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_File->DropDownItems->Add(Menu_File_Exit);

		// Settings Menu
		ToolStripMenuItem^ Menu_Settings = gcnew ToolStripMenuItem("Settings");
		Menu_Settings->Padding = System::Windows::Forms::Padding(4, 0, 4, 0);

		// Settings -> Hotkeys
		ToolStripMenuItem^ Menu_Settings_Hotkeys = gcnew ToolStripMenuItem("Keyboard Shortcuts");
		Menu_Settings_Hotkeys->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Hotkey")));
		Menu_Settings_Hotkeys->ShortcutKeys = Keys::Control | Keys::K;
		Menu_Settings_Hotkeys->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Settings_Hotkeys_Click);

		// Settings -> MIDI
		ToolStripMenuItem^ Menu_Settings_Midi = gcnew ToolStripMenuItem("MIDI Settings");
		Menu_Settings_Midi->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Midi_Settings")));
		Menu_Settings_Midi->ShortcutKeys = Keys::Control | Keys::M;
		Menu_Settings_Midi->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Settings_Midi_Click);

		// Build Settings menu
		Menu_Settings->DropDownItems->Add(Menu_Settings_Hotkeys);
		Menu_Settings->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_Settings->DropDownItems->Add(Menu_Settings_Midi);

		ToolStripLabel^ Label_Version = gcnew ToolStripLabel();
		Label_Version->Text = "v" + VERSION_BUILD_STRING;
		#ifdef _DEBUG
			Label_Version->Text = "Debug Version";
		#endif
		Label_Version->Alignment = ToolStripItemAlignment::Right;
		Label_Version->Padding = System::Windows::Forms::Padding(0, 0, 10, 0); // Add some right padding
		

		// Add menus to strip
		this->_Menu_Strip->Items->Add(Menu_File);
		this->_Menu_Strip->Items->Add(Menu_Settings);
		this->_Menu_Strip->Items->Add(Label_Version);

		#ifdef _DEBUG
			InitializeDebugButtons();
		#endif

		// Add menu strip to form
		this->Controls->Add(this->_Menu_Strip);
	}

	void Form_Main::InitializeDebugButtons()
	{
	#ifdef _DEBUG
			// Test 1 button
			ToolStripMenuItem^ Menu_Debug_Test1 = gcnew ToolStripMenuItem("Test 1");
			Menu_Debug_Test1->Click += gcnew System::EventHandler(this, &Form_Main::Button_1_Click);

			// Test 2 button
			ToolStripMenuItem^ Menu_Debug_Test2 = gcnew ToolStripMenuItem("Test 2");
			Menu_Debug_Test2->Click += gcnew System::EventHandler(this, &Form_Main::Button_2_Click);

			// Add test buttons directly to menu strip
			this->_Menu_Strip->Items->Add(Menu_Debug_Test1);
			this->_Menu_Strip->Items->Add(Menu_Debug_Test2);
	#endif
	}

	void Form_Main::Menu_File_Open_GP_Click(Object^ sender, System::EventArgs^ e)
	{
		OpenFileDialog^ Open_Dialog_File = gcnew OpenFileDialog();
		Open_Dialog_File->InitialDirectory = ".";
		Open_Dialog_File->Filter = "Guitar Pro 5 Files (*.gp5)|*.gp5|All Files (*.*)|*.*";
		Open_Dialog_File->RestoreDirectory = true;

		if (Open_Dialog_File->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			System::String^ GuiterPro5_Filename = Open_Dialog_File->FileName;

			if (this->_GP_Tab != NULL) {
				delete(this->_GP_Tab);
				this->_GP_Tab = NULL;
			}

			this->_GP_Tab = new gp_parser::Parser(ConvertToStdString(GuiterPro5_Filename));

			this->_Tab_Info->Update_Info(GuiterPro5_Filename, gcnew String(this->_GP_Tab->getTabFile().title.data()), (unsigned int)this->_GP_Tab->getTabFile().measureHeaders.size(), this->_GP_Tab->getTabFile().trackCount);

			this->_Timeline->Clear();
			OnMidiSettingsAccepted();


			if (Settings::Get_Instance()->Octave_Entries->Count == 0) {
				MessageBox::Show(this, "The Guitar Pro File has been successfully opened.\nTo see the Tablature, add Light Tracks in the MIDI Settings.", "Open Guitar Pro File", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
		}
	}

	void Form_Main::Menu_File_Open_Light_Click(System::Object^ sender, System::EventArgs^ e)
	{
		OpenFileDialog^ Open_Dialog_File = gcnew OpenFileDialog();
		Open_Dialog_File->InitialDirectory = ".";
		Open_Dialog_File->Filter = "Light Information Files (*.light)|*.light|All Files (*.*)|*.*";
		Open_Dialog_File->RestoreDirectory = true;

		if (Open_Dialog_File->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			System::String^ Filename = Open_Dialog_File->FileName;

			String^ Error_Message = this->_Timeline->LoadBarEventsFromFile(Filename);

			if (Error_Message->Length > 0) {
				MessageBox::Show(this, "Error:\n" + Error_Message,  "Failed to load light information file", MessageBoxButtons::OK,	MessageBoxIcon::Error);
			}
		}
	}

	void Form_Main::Menu_File_Save_Light_Click(System::Object^ sender, System::EventArgs^ e)
	{
		String^ Default_Filename = this->_Tab_Info->Get_Song_Name() + ".light";

		SaveFileDialog^ Save_Dialog_File = gcnew SaveFileDialog();
		Save_Dialog_File->InitialDirectory = ".";
		Save_Dialog_File->Filter = "Light Information Files (*.light)|*.light|All Files (*.*)|*.*";
		Save_Dialog_File->RestoreDirectory = true;
		Save_Dialog_File->FileName = Default_Filename;

		if (Save_Dialog_File->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			System::String^ Filename = Save_Dialog_File->FileName;

			String^ Error_Message = this->_Timeline->SaveBarEventsToFile(Filename);

			if (Error_Message->Length > 0) {
				MessageBox::Show(this, "Error:\n" + Error_Message, "Failed to save light information to file", MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
			else {
				MessageBox::Show(this, "Light Configuration has been successfully saved.", "Save Successful", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
		}
	}

	void Form_Main::Menu_File_Export_MIDI_Click(System::Object^ sender, System::EventArgs^ e)
	{
		const int OCTAVE_OFFSET		= 2;
		const int NOTES_PER_OCTAVE	= 12;

		String^ Default_Filename = this->_Tab_Info->Get_Song_Name() + ".mid";

		SaveFileDialog^ Save_Dialog_File = gcnew SaveFileDialog();
		Save_Dialog_File->InitialDirectory = ".";
		Save_Dialog_File->Filter = "MIDI Files (*.mid)|*.mid|All Files (*.*)|*.*";
		Save_Dialog_File->RestoreDirectory = true;
		Save_Dialog_File->FileName = Default_Filename;

		if (Save_Dialog_File->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			try {
				Settings^ Settings = Settings::Get_Instance();
				List<Settings::Octave_Entry^>^ Octave_Entries = Settings->Octave_Entries;

				MIDI_Writer Writer(960);  // Use 960 ticks per quarter note

				if (this->_GP_Tab == NULL) {
					throw gcnew Exception("No Guitar Pro 5 file opened");
				}

				// First, add all measures from the Guitar Pro file
				bool has_measures = false;
				for (auto i = 0; i < this->_GP_Tab->getTabFile().measureHeaders.size(); i++)
				{
					has_measures = true;
					gp_parser::MeasureHeader* MH = &(this->_GP_Tab->getTabFile().measureHeaders.at(i));
					Writer.Add_Measure(MH->timeSignature.numerator,
						MH->timeSignature.denominator.value,
						MH->tempo.value);
				}

				// Then add all the notes
				int Count_RGB_Bars = 0;

				for each(Track ^ T in this->_Timeline->Tracks)
				{
					String^ Track_Name	= T->Name;
					int Octave			= T->Octave;

					int Octave_Note_Offset = (Octave + OCTAVE_OFFSET) * NOTES_PER_OCTAVE;

					for each(BarEvent ^ B in T->Events)
					{
						int Tick_Start	= B->StartTick;
						int Tick_Length = B->Duration;
						Color Bar_Color = B->Color;

						uint8_t Value_Red	= (Bar_Color.R >> 1);
						uint8_t Value_Green = (Bar_Color.G >> 1);
						uint8_t Value_Blue	= (Bar_Color.B >> 1);

						if (Value_Red > 0) {
							Writer.Add_Note(Tick_Start, Tick_Length, 0, Octave_Note_Offset + Settings->MIDI_Note_Red, Value_Red);
						}

						if (Value_Green > 0) {
							Writer.Add_Note(Tick_Start, Tick_Length, 0, Octave_Note_Offset + Settings->MIDI_Note_Green, Value_Green);
						}

						if (Value_Blue > 0) {
							Writer.Add_Note(Tick_Start, Tick_Length, 0, Octave_Note_Offset + Settings->MIDI_Note_Blue, Value_Blue);
						}
					}
				}
				
				if (!Writer.Save_To_File(ConvertToStdString(Save_Dialog_File->FileName))) {
					throw gcnew Exception("Failed to write MIDI file");
				}

				// Show success message
				MessageBox::Show(this, "MIDI file has been successfully exported.", "Export Successful", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
			catch (Exception^ ex) {
				MessageBox::Show(this, "Error:\n" + ex->Message, "Failed to export MIDI file", MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
		}
	}

	void Form_Main::Menu_File_Exit_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->Close();
	}

	void Form_Main::Menu_Settings_Hotkeys_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Form_Settings_Hotkeys^ hotkeyForm = gcnew Form_Settings_Hotkeys();
		hotkeyForm->Owner = this;
		hotkeyForm->ShowDialog();
	}

	void Form_Main::Menu_Settings_Midi_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Form_Settings_MIDI^ midiForm = gcnew Form_Settings_MIDI();
		midiForm->On_Settings_Accepted += gcnew On_Settings_Accepted_Handler(this, &Form_Main::OnMidiSettingsAccepted);
		midiForm->ShowDialog();
	}

	void Form_Main::Form_Main_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e)
	{
		bool Events_Preset = false;
		for each(Track ^ T in this->_Timeline->Tracks)
		{
			if (T->Events->Count > 0) {
				Events_Preset = true;
				break;
			}
		}

		if (Events_Preset)
		{
			System::Windows::Forms::DialogResult Result = MessageBox::Show(this, "There are unsaved changes in the light configuration.\nDo you want to save them before closing?", "Unsaved Changes", MessageBoxButtons::YesNoCancel, MessageBoxIcon::Question);
			if (Result == System::Windows::Forms::DialogResult::Yes)
			{
				Menu_File_Save_Light_Click(sender, nullptr);
			}
			else if (Result == System::Windows::Forms::DialogResult::Cancel)
			{
				e->Cancel = true;
			}
		}
	}

	void Form_Main::Toolbar_OnToolChanged(System::Object^ sender, TimelineToolType e)
	{
		DrawTool^ Draw_Tool			= this->_Timeline->GetDrawTool();
		DurationTool^ Duration_Tool = this->_Timeline->GetDurationTool();
		ColorTool^ Color_Tool		= this->_Timeline->GetColorTool();
		FadeTool^ Fade_Tool			= this->_Timeline->GetFadeTool();
		StrobeTool^ Strobe_Tool		= this->_Timeline->GetStrobeTool();

		switch (e)
		{
		case TimelineToolType::Pointer:
			this->_Timeline->SetCurrentTool(TimelineToolType::Pointer);
			this->_Timeline->SetToolSnapping((SnappingType)_Pointer_Options->PointerSnapping);
			break;

		case TimelineToolType::Draw:
			this->_Timeline->SetCurrentTool(TimelineToolType::Draw);
			this->_Timeline->SetToolSnapping((SnappingType)_Draw_Options->DrawSnapping);

			Draw_Tool->DrawTickLength	= _Draw_Options->DrawLength;
			Draw_Tool->UseAutoLength	= _Draw_Options->LengthByTablature;
			Draw_Tool->DrawColor		= _Draw_Options->SelectedColor;

			break;

		case TimelineToolType::Erase:
			this->_Timeline->SetCurrentTool(TimelineToolType::Erase);
			break;

		case TimelineToolType::Duration:
			this->_Timeline->SetCurrentTool(TimelineToolType::Duration);

			Duration_Tool->ChangeTickLength = this->_Length_Options->Value;
			break;

		case TimelineToolType::Color:
			this->_Timeline->SetCurrentTool(TimelineToolType::Color);

			Color_Tool->CurrentColor = this->_Color_Options->SelectedColor;
			break;

		case TimelineToolType::Fade:
			this->_Timeline->SetCurrentTool(TimelineToolType::Fade);

			Fade_Tool->TickLength	= _Fade_Options->TickLength;
			Fade_Tool->ColorStart	= _Fade_Options->StartColor;
			Fade_Tool->ColorEnd		= _Fade_Options->EndColor;
			Fade_Tool->ColorCenter	= _Fade_Options->CenterColor;
			Fade_Tool->Type			= (FadeType)_Fade_Options->FadeMode;
			break;

		case TimelineToolType::Strobe:
			this->_Timeline->SetCurrentTool(TimelineToolType::Strobe);

			Strobe_Tool->TickLength		= _Strobe_Options->TickLength;
			Strobe_Tool->StrobeColor	= _Strobe_Options->SelectedColor;
			break;
		}
	}

	void Form_Main::OnMidiSettingsAccepted()
	{
		Settings^ Settings = Settings::Get_Instance();

		if (Settings->Octave_Entries->Count != this->_Timeline->Tracks->Count)
		{
			this->_DropDown_Marker->Clear();
			this->_Timeline->Clear();

			List<String^>^ First_Marker_List = gcnew List<String^>();
			List<String^>^ Second_Marker_List = gcnew List<String^>();
			List<int>^ Values_List = gcnew List<int>();

			for (int i = 0;i < Settings->Octave_Entries->Count;i++)
			{
				this->_Timeline->AddTrack(Settings->Octave_Entries[i]->Name, Settings->Octave_Entries[i]->Octave_Number);
			}

			if (this->_GP_Tab != NULL)
			{
				////////////////////////////////////////////
				// Add Basic Structure of Measure Headers //
				////////////////////////////////////////////
				for (auto i = 0;i < this->_GP_Tab->getTabFile().measureHeaders.size();i++)
				{
					gp_parser::MeasureHeader* MH = &(this->_GP_Tab->getTabFile().measureHeaders.at(i));


					String^ Marker_Text = gcnew String(MH->marker.title.data());

					this->_Timeline->AddMeasure(MH->timeSignature.numerator,
												MH->timeSignature.denominator.value,
												MH->tempo.value,
												Marker_Text);

					if (Marker_Text->Length > 0) {
						String^ markerTitle = Marker_Text;
						First_Marker_List->Add(markerTitle);
						Second_Marker_List->Add(""); 
						Values_List->Add(MH->number);
					}
				}

				array<String^>^ Lines_First_Marker = First_Marker_List->ToArray();
				array<String^>^ Lines_Second_Marker = Second_Marker_List->ToArray();
				array<int>^ Values_Marker = Values_List->ToArray();

				this->_DropDown_Marker->Set_Items(Lines_First_Marker, Lines_Second_Marker, Values_Marker);



				///////////////////////////
				// Add Tablature Content //
				///////////////////////////
				for (auto t = 0; t < this->_GP_Tab->getTabFile().tracks.size();t++)
				{
					String^ Track_Name = gcnew String(_GP_Tab->getTabFile().tracks.at(t).name.data());
					this->_Tab_Info->Add_Track_Title(Track_Name);
					
					Track^ Track_Target = nullptr;

					for each(Track ^ T in this->_Timeline->Tracks) {
						if (T->Name == Track_Name) {
							Track_Target = T;
							break;
						}
					}
					if (Track_Target == nullptr) {
						continue;
					}

					gp_parser::Track* Track_Source = &(this->_GP_Tab->getTabFile().tracks.at(t));

					for (auto m = 0; m < Track_Source->measures.size();m++)
					{
						Track_Target->IsDrumTrack = Track_Source->isDrumsTrack;
						
						gp_parser::Measure* M = &(Track_Source->measures.at(m));

						gp_parser::Voice* V = &M->voices[0];
						for (auto b = 0; b <V->beats.size();b++)
						{
							gp_parser::Beat* B = &(V->beats.at(b));
							int Duration	= (int)B->durationInTicks;
							bool IsDotted	= B->duration.dotted;
							bool Empty		= (B->status == gp_parser::BEAT_EMTPY) || (B->status == gp_parser::BEAT_REST);

							if (Empty == true) {
								continue;
							}

							Beat^ New_Beat =  this->_Timeline->AddBeat(Track_Target, (int)m, B->start, Duration, IsDotted);

							for (auto n = 0; n < B->notes.size();n++)
							{
								this->_Timeline->AddNote(New_Beat, B->notes.at(n).string, B->notes.at(n).value, B->notes.at(n).tiedNote);
							}
						}
					}
				}

			}
		}
		else
		{
			for (int i = 0;i < Settings->Octave_Entries->Count;i++)
			{
				this->_Timeline->Tracks[i]->Name = Settings->Octave_Entries[i]->Name;
			}
		}
	}

	void Form_Main::DropDown_Track_Height_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		this->_Timeline->SetAllTracksHeight(e->Value);
	}

	void Form_Main::DropDown_View_Marker_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		this->_Timeline->ScrollToMeasure(e->Value);
	}

	void Form_Main::TrackBar_Zoom_OnValue_Changed(System::Object^ sender, Track_Bar_Value_Changed_Event_Args^ e)
	{
		this->_Timeline->SetZoom(e->Value);
	}

	void Form_Main::Initialize_Hotkeys()
	{
		_Active_Hotkeys = gcnew Dictionary<String^, System::Windows::Forms::Keys>();
		Update_Hotkeys();
		Register_Hotkey_Handlers();
	}

	void Form_Main::Update_Hotkeys()
	{
		_Active_Hotkeys->Clear();
		auto bindings = Hotkey_Manager::Instance->Get_All_Bindings();

		for each (KeyValuePair<String^, String^> binding in bindings)
		{
			array<String^>^ parts = binding.Value->Split('+');
			Keys key_code = Keys::None;

			String^ main_key = parts[parts->Length - 1]->Trim();
			if (main_key == "") {
				continue;
			}
			key_code = safe_cast<Keys>(Enum::Parse(Keys::typeid, main_key));

			if (binding.Value->Contains("Ctrl"))
				key_code = key_code | Keys::Control;
			if (binding.Value->Contains("Shift"))
				key_code = key_code | Keys::Shift;
			if (binding.Value->Contains("Alt"))
				key_code = key_code | Keys::Alt;

			_Active_Hotkeys->Add(binding.Key, key_code);
		}
	}

	bool Form_Main::Process_Hotkey(System::Windows::Forms::Keys key_code)
	{
		// Get current key with modifiers
		Keys currentKey = key_code;
		if (ModifierKeys.HasFlag(Keys::Control)) {
			currentKey = currentKey | Keys::Control;
		}
		if (ModifierKeys.HasFlag(Keys::Shift)) {
			currentKey = currentKey | Keys::Shift;
		}
		if (ModifierKeys.HasFlag(Keys::Alt)) {
			currentKey = currentKey | Keys::Alt;
		}

		for each (KeyValuePair<String^, System::Windows::Forms::Keys> hotkey in _Active_Hotkeys)
		{
			if (currentKey == hotkey.Value)
			{
					 if (hotkey.Key == "Select Tool")	{ this->_Toolbar->CurrentTool = TimelineToolType::Pointer;	return true; }
				else if (hotkey.Key == "Draw Tool"	)	{ this->_Toolbar->CurrentTool = TimelineToolType::Draw;		return true; }
				else if (hotkey.Key == "Erase Tool"	)	{ this->_Toolbar->CurrentTool = TimelineToolType::Erase;	return true; }
				else if (hotkey.Key == "Color Tool"	)	{ this->_Toolbar->CurrentTool = TimelineToolType::Color;	return true; }
				else if (hotkey.Key == "Duration Tool") { this->_Toolbar->CurrentTool = TimelineToolType::Duration; return true; }
				else if (hotkey.Key == "Fade Tool")		{ this->_Toolbar->CurrentTool = TimelineToolType::Fade;		return true; }
				else if (hotkey.Key == "Strobe Tool")	{ this->_Toolbar->CurrentTool = TimelineToolType::Strobe;	return true; }
				//else if (hotkey.Key == "Bucket Tool") { this->_Toolbar->CurrentTool = TimelineToolType::Pointer; return true; }

				else if (hotkey.Key->StartsWith("Select Color "))
				{
					String^ color_num = hotkey.Key->Substring(13);
					int color_index = Int32::Parse(color_num);
					this->_Tools_And_Control->Select_Color_From_Preset(color_index-1);
					return true;
				}

				else if (hotkey.Key == "Length Up"						) { this->_Tools_And_Control->Length_Up();				return true; }
				else if (hotkey.Key == "Length Down"					) { this->_Tools_And_Control->Length_Down();			return true; }
				else if (hotkey.Key == "Draw Tool - Toggle Length Tab"	) { this->_Draw_Options->Toggle_LengthByTablature();	return true; }

				else if (hotkey.Key == "Snapping Next"					) { this->_Tools_And_Control->Snapping_Up();			return true; }
				else if (hotkey.Key == "Snapping Previous"				) { this->_Tools_And_Control->Snapping_Down();			return true; }
				else if (hotkey.Key == "Snap To None"					) { this->_Tools_And_Control->Snap_To((int)SnappingType::Snap_None);		return true; }
				else if (hotkey.Key == "Snap To Grid"					) { this->_Tools_And_Control->Snap_To((int)SnappingType::Snap_Grid);		return true; }
				else if (hotkey.Key == "Snap To Bars"					) { this->_Tools_And_Control->Snap_To((int)SnappingType::Snap_Bars);		return true; }
				else if (hotkey.Key == "Snap To Tablature"				) { this->_Tools_And_Control->Snap_To((int)SnappingType::Snap_Tablature);	return true; }

				else if (hotkey.Key == "Zoom In"						) { this->_TrackBar_Zoom->Move_To_Next_Value();			return true; }
				else if (hotkey.Key == "Zoom Out"						) { this->_TrackBar_Zoom->Move_To_Previous_Value();		return true; }
				else if (hotkey.Key == "Zoom Reset"						) { this->_TrackBar_Zoom->Value = 1;					return true; }

				else if (hotkey.Key == "Track Height Increase"			) { this->_DropDown_Track_Height->Select_Next();		return true; }
				else if (hotkey.Key == "Track Height Decrease"			) { this->_DropDown_Track_Height->Select_Previous();	return true; }
				else if (hotkey.Key == "Track Height Reset"				) { this->_DropDown_Track_Height->Selected_Index = 1;	return true; }
			}
		}
		return false;
	}

	void Form_Main::Register_Hotkey_Handlers()
	{
		this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &Form_Main::Form_KeyDown);
		this->KeyPreview = true;
	}

	void Form_Main::Form_KeyDown(Object^ sender, System::Windows::Forms::KeyEventArgs^ e)
	{
		if (Process_Hotkey(e->KeyCode))
		{
			e->Handled = true;
		}
	}

	void Form_Main::Pointer_Options_OnSnappingChanged(int value)
	{
		this->_Timeline->SetToolSnapping((SnappingType)value);
	}

	void Form_Main::Draw_Options_OnSnappingChanged(int value)
	{
		this->_Timeline->SetToolSnapping((SnappingType)value);
	}

	void Form_Main::Draw_Options_OnLengthChanged(int value)
	{
		DrawTool^ Draw_Tool = this->_Timeline->GetDrawTool();
		Draw_Tool->DrawTickLength = value;
	}

	void Form_Main::Draw_Options_OnConsiderTabChanged(bool value)
	{
		DrawTool^ Draw_Tool = this->_Timeline->GetDrawTool();
		Draw_Tool->UseAutoLength = value;
	}

	void Form_Main::Draw_Options_OnColorChanged(System::Drawing::Color color)
	{
		DrawTool^ Draw_Tool = this->_Timeline->GetDrawTool();
		Draw_Tool->DrawColor = color;
	}

	void Form_Main::Length_Options_OnLengthChanged(int value)
	{
		DurationTool^ Duration_Tool = this->_Timeline->GetDurationTool();
		Duration_Tool->ChangeTickLength = value;
	}

	void Form_Main::Color_Options_OnColorChanged(System::Drawing::Color color)
	{
		ColorTool^ Color_Tool = this->_Timeline->GetColorTool();
		Color_Tool->CurrentColor = color;
	}

	void Form_Main::Fade_Options_OnLengthChanged(int value)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->TickLength = value;
	}

	void Form_Main::Fade_Options_OnColorStartChanged(System::Drawing::Color color)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->ColorStart = color;
	}

	void Form_Main::Fade_Options_OnColorEndChanged(System::Drawing::Color color)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->ColorEnd = color;
	}

	void Form_Main::Fade_Options_OnColorCenterChanged(System::Drawing::Color color)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->ColorCenter = _Fade_Options->CenterColor;
	}

	void Form_Main::Fade_Options_OnFadeModeChanged(Fade_Mode mode)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->Type = (FadeType)_Fade_Options->FadeMode;
	}

	void Form_Main::Strobe_Options_OnLengthChanged(int value)
	{
		StrobeTool^ Strobe_Tool = this->_Timeline->GetStrobeTool();
		Strobe_Tool->TickLength = value;
	}

	void Form_Main::Strobe_Options_OnColorChanged(System::Drawing::Color color)
	{
		StrobeTool^ Strobe_Tool = this->_Timeline->GetStrobeTool();
		Strobe_Tool->StrobeColor = color;
	}

	void Form_Main::Bucket_Options_OnColorChanged(System::Drawing::Color color)
	{

	}

	std::string Form_Main::ConvertToStdString(System::String^ input_string)
	{
		if (input_string == nullptr)
			return "";

		pin_ptr<const wchar_t> wch = PtrToStringChars(input_string);
		int len = ((input_string->Length + 1) * 2);
		char* ch = new char[len];

		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, ch, len, wch, _TRUNCATE);
		std::string standardString(ch);
		delete[] ch;

		return standardString;
	}

	void Form_Main::Button_1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Console::WriteLine("Button 1 Clicked");

		this->_Timeline->LogPerformanceMetrics();
	}

	void Form_Main::Button_2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Console::WriteLine("Button 2 Clicked");
	}
}
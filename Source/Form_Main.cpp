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
		this->StartPosition = FormStartPosition::CenterScreen;
		this->_GP_Tab		= NULL;

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

		// Get transport controls reference
		this->_Transport_Controls = this->_Tools_And_Control->Get_Widget_Transport_Controls();

		
		//////////////////////
		// Timeline Section //
		//////////////////////
		Panel^ TimelineContainer = gcnew Panel();
		TimelineContainer->Dock = DockStyle::Fill;
		TimelineContainer->Padding = System::Windows::Forms::Padding(0);
		TimelineContainer->BackColor = Theme_Manager::Get_Instance()->BackgroundLight;
		TimelineContainer->BorderStyle = BorderStyle::FixedSingle;

		this->_Timeline = gcnew Widget_Timeline(this->_Tools_And_Control);
		this->_Timeline->Dock = System::Windows::Forms::DockStyle::Fill;
		this->_Timeline->Name = L"timeline";
		this->_Timeline->Theme = Theme_Manager::Get_Instance()->GetTimelineTheme();
		this->_Timeline->CommandManager()->CommandStateChanged += gcnew TimelineCommandManager::CommandStateChangedHandler(this, &Form_Main::UpdateUndoRedoState);
		TimelineContainer->Controls->Add(this->_Timeline);

		Table_Layout_Main->Controls->Add(TimelineContainer, 0, 1);
		Table_Layout_Main->SetColumnSpan(TimelineContainer, Table_Layout_Main->ColumnCount);

		// Initialize other tool options
		InitializeToolOptions();


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


		///////////////////
		// MIDI Exporter //
		///////////////////
		_MIDI_Exporter = gcnew MIDI_Exporter(this->_Timeline);


		// Initialize menu
		InitializeMainMenu();

		// Apply theme
		//Theme_Manager::Get_Instance()->ApplyTheme(this);
		this->BackColor = Theme_Manager::Get_Instance()->BackgroundLight;
		Theme_Manager::Get_Instance()->ApplyThemeToMenuStrip(this->_Menu_Strip);

		// Initialize hotkeys and settings
		Initialize_Hotkeys();
		SettingsMIDI_On_Settings_Accepted();

		// Initialize playback system
		Initialize_Playback_System();
	}

	Form_Main::~Form_Main()
	{
		if (_Playback_Update_Timer != nullptr)
		{
			_Playback_Update_Timer->Stop();
			delete _Playback_Update_Timer;
		}

		Shutdown_Playback_System();
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
		this->_Pointer_Options->QuantizationChanged += gcnew QuantizationChangedHandler(this, &Form_Main::Pointer_Options_OnQuantizationChanged);
		
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
		this->_Fade_Options->FadeModeChanged		+= gcnew FadeModeChangedHandler		(this, &Form_Main::Fade_Options_OnFadeModeChanged);
		this->_Fade_Options->ColorStartChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Fade_Options_OnColorStartChanged);
		this->_Fade_Options->ColorEndChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Fade_Options_OnColorEndChanged);
		this->_Fade_Options->ColorCenterChanged		+= gcnew ColorChangedHandler		(this, &Form_Main::Fade_Options_OnColorCenterChanged);
		this->_Fade_Options->EasingsChanged			+= gcnew EasingsChangedHandler		(this, &Form_Main::Fade_Options_OnEasingsChanged);

		// Strobe Options
		this->_Strobe_Options = this->_Tools_And_Control->Get_Widget_Strobe_Options();
		this->_Strobe_Options->QuantizationChanged += gcnew QuantizationChangedHandler(this, &Form_Main::Strobe_Options_OnLengthChanged);
		this->_Strobe_Options->ColorChanged += gcnew ColorChangedHandler(this, &Form_Main::Strobe_Options_OnColorChanged);

		// Bucket Options
		this->_Bucket_Options = this->_Tools_And_Control->Get_Widget_Bucket_Options();
		this->_Bucket_Options->ColorChanged += gcnew ColorChangedHandler(this, &Form_Main::Bucket_Options_OnColorChanged);


		PointerTool^ PointerTool = this->_Timeline->GetPointerTool();
		if (PointerTool != nullptr) {
			PointerTool->SelectionChanged += gcnew System::EventHandler(this, &Form_Main::UpdateEditMenuState);
		}

		EraseTool^ EraseTool = this->_Timeline->GetEraseTool();
		if (EraseTool != nullptr) {
			EraseTool->SelectionChanged += gcnew EventHandler(this, &Form_Main::UpdateEditMenuState);
		}
	}

	void Form_Main::InitializeMainMenu()
	{
		this->_Menu_Strip = gcnew MenuStrip();
		this->_Menu_Strip->Padding = System::Windows::Forms::Padding(6, 2, 0, 2);

		///////////////
		// File Menu //
		///////////////
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

		ToolStripMenuItem^ Menu_File_Open_Light_Special = gcnew ToolStripMenuItem("Open Light Information File Special...");
		Menu_File_Open_Light_Special->Click += gcnew System::EventHandler(this, &Form_Main::Menu_File_Open_Light_Special_Click);

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
		Menu_File->DropDownItems->Add(Menu_File_Open_Light_Special);
		Menu_File->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_File->DropDownItems->Add(Menu_File_Save_Light);
		Menu_File->DropDownItems->Add(Menu_File_Export_MIDI);
		Menu_File->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_File->DropDownItems->Add(Menu_File_Exit);


		///////////////
		// Edit Menu //
		///////////////
		ToolStripMenuItem^ Menu_Edit = gcnew ToolStripMenuItem("Edit");
		Menu_Edit->Padding = System::Windows::Forms::Padding(4, 0, 4, 0);

		// Edit -> Undo
		_Menu_Edit_Undo = gcnew ToolStripMenuItem("Undo");
		_Menu_Edit_Undo->ShortcutKeys = Keys::Control | Keys::Z;
		_Menu_Edit_Undo->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Edit_Undo_Click);
		_Menu_Edit_Undo->Enabled = false;

		// Edit -> UndoSteps
		_Menu_Edit_UndoSteps = gcnew ToolStripMenuItem("Undo Steps");
		_Menu_Edit_UndoSteps->Enabled = false;

		// Edit -> Redo
		_Menu_Edit_Redo = gcnew ToolStripMenuItem("Redo");
		_Menu_Edit_Redo->ShortcutKeys = Keys::Control | Keys::Y;
		_Menu_Edit_Redo->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Edit_Redo_Click);
		_Menu_Edit_Redo->Enabled = false;

		// Edit -> Copy
		_Menu_Edit_Copy = gcnew ToolStripMenuItem("Copy");
		_Menu_Edit_Copy->ShortcutKeys = Keys::Control | Keys::C;
		_Menu_Edit_Copy->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Edit_Copy_Click);
		_Menu_Edit_Copy->Enabled = false;

		// Edit -> Paste
		_Menu_Edit_Paste = gcnew ToolStripMenuItem("Paste");
		_Menu_Edit_Paste->ShortcutKeys = Keys::Control | Keys::V;
		_Menu_Edit_Paste->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Edit_Paste_Click);
		_Menu_Edit_Paste->Enabled = false;

		// Edit -> Delete
		_Menu_Edit_Delete = gcnew ToolStripMenuItem("Delete");
		_Menu_Edit_Delete->ShortcutKeys = Keys::Delete;
		_Menu_Edit_Delete->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Edit_Delete_Click);
		_Menu_Edit_Delete->Enabled = false;

		// Edit -> Batch Action
		_Menu_Edit_BatchAction = gcnew ToolStripMenuItem("Batch Action...");
		_Menu_Edit_BatchAction-> Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Batch")));
		_Menu_Edit_BatchAction->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Edit_BatchAction_Click);

		// Build Edit menu
		Menu_Edit->DropDownItems->Add(_Menu_Edit_Undo);
		Menu_Edit->DropDownItems->Add(_Menu_Edit_UndoSteps);
		Menu_Edit->DropDownItems->Add(_Menu_Edit_Redo);
		Menu_Edit->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_Edit->DropDownItems->Add(_Menu_Edit_Copy);
		Menu_Edit->DropDownItems->Add(_Menu_Edit_Paste);
		Menu_Edit->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_Edit->DropDownItems->Add(_Menu_Edit_Delete);
		Menu_Edit->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_Edit->DropDownItems->Add(_Menu_Edit_BatchAction);

		_Menu_Edit_UndoSteps_Items = gcnew List<ToolStripMenuItem^>();

		for (int i = 0; i < 10; i++)
		{
			ToolStripMenuItem^ HistoryItem = gcnew ToolStripMenuItem();
			HistoryItem->Visible = false;
			HistoryItem->Click += gcnew EventHandler(this, &Form_Main::Menu_Edit_UndoHistory_Click);
			_Menu_Edit_UndoSteps->DropDownItems->Add(HistoryItem);
			_Menu_Edit_UndoSteps_Items->Add(HistoryItem);
		}

		// Register for tool change events to update menu state
		_Timeline->ToolChanged += gcnew System::EventHandler<TimelineToolType>(this, &Form_Main::UpdateEditMenuState);


		///////////////////
		// Settings Menu //
		///////////////////
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

		// Settings -> Device
		ToolStripMenuItem^ Menu_Settings_Device = gcnew ToolStripMenuItem("Device Configuration");
		//Menu_Settings_Device->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Midi_Settings")));
		//Menu_Settings_Device->ShortcutKeys = Keys::Control | Keys::M;
		Menu_Settings_Device->Click += gcnew System::EventHandler(this, &Form_Main::Menu_Settings_Device_Click);

		// Build Settings menu
		Menu_Settings->DropDownItems->Add(Menu_Settings_Hotkeys);
		Menu_Settings->DropDownItems->Add(gcnew ToolStripSeparator());
		Menu_Settings->DropDownItems->Add(Menu_Settings_Midi);
		Menu_Settings->DropDownItems->Add(Menu_Settings_Device);

		ToolStripLabel^ Label_Version = gcnew ToolStripLabel();
		Label_Version->Text = "v" + VERSION_BUILD_STRING;
		#ifdef _DEBUG
			Label_Version->Text = "Debug Version";
		#endif
		Label_Version->Alignment = ToolStripItemAlignment::Right;
		Label_Version->Padding = System::Windows::Forms::Padding(0, 0, 10, 0); // Add some right padding
		

		////////////////////////
		// Add menus to strip //
		////////////////////////
		this->_Menu_Strip->Items->Add(Menu_File);
		this->_Menu_Strip->Items->Add(Menu_Edit);
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

	void Form_Main::Initialize_Playback_System()
	{
		try
		{
			// Create Playback_Manager
			this->_Playback_Manager = gcnew Playback_Manager();
			this->_Timeline->SetPlaybackManager(this->_Playback_Manager);

			Device_Manager^ Devices = gcnew Device_Manager();
			Devices->Enumerate_All_Devices();

			// Get device settings from Settings
			Settings^ Config = Settings::Get_Instance();
			String^ MIDI_Device_Name = Config->Selected_MIDI_Output_Device;
			int MIDI_Device_ID = Devices->Find_MIDI_Device_By_Name(MIDI_Device_Name);
			int MIDI_Channel = Config->Global_MIDI_Output_Channel;
			String^ Audio_Device_Name = Config->Selected_Audio_Output_Device;
			String^ Audio_Device_ID = Devices->Find_Audio_Device_By_Name(Audio_Device_Name);
			int Buffer_Size = Config->Audio_Buffer_Size;

			// Initialize MIDI engine if device is configured
			if (!String::IsNullOrEmpty(MIDI_Device_Name))
			{
				// Note: Device ID would need to be resolved from device name
				// For now, using default device (ID 0)
				if (!_Playback_Manager->Initialize_MIDI(MIDI_Device_ID))
				{
					Console::WriteLine("Warning: Failed to initialize MIDI playback");
				}
			}

			// Initialize Audio engine if device is configured
			if (!String::IsNullOrEmpty(Audio_Device_Name))
			{
				if (!_Playback_Manager->Initialize_Audio(Audio_Device_ID, Buffer_Size))
				{
					Console::WriteLine("Warning: Failed to initialize audio playback");
				}
			}

			// Connect transport controls to playback manager
			if (_Transport_Controls != nullptr)
			{
				_Transport_Controls->Set_Playback_Manager(_Playback_Manager);
			}

			// Create and start update timer
			this->_Playback_Update_Timer = gcnew System::Windows::Forms::Timer();
			this->_Playback_Update_Timer->Interval = 16; // ~60 FPS
			this->_Playback_Update_Timer->Tick += gcnew EventHandler(this, &Form_Main::Playback_Update_Timer_Tick);
			this->_Playback_Update_Timer->Start();
		}
		catch (Exception^ Ex)
		{
			Console::WriteLine("Error initializing playback system: " + Ex->Message);
		}
	}

	void Form_Main::Shutdown_Playback_System()
	{
		if (_Playback_Manager != nullptr)
		{
			_Playback_Manager->Stop();
			_Playback_Manager->Cleanup();
			delete _Playback_Manager;
			_Playback_Manager = nullptr;
		}
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

			this->_GP_Tab = new gp_parser::Parser(_MIDI_Exporter->ConvertToStdString(GuiterPro5_Filename));

			this->_Tab_Info->Update_Info(GuiterPro5_Filename, gcnew String(this->_GP_Tab->getTabFile().title.data()), (unsigned int)this->_GP_Tab->getTabFile().measureHeaders.size(), this->_GP_Tab->getTabFile().trackCount);

			this->_Timeline->Clear();
			this->_TrackBar_Zoom->Value = 1.0f;
			SettingsMIDI_On_Settings_Accepted();


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

	void Form_Main::Menu_File_Open_Light_Special_Click(System::Object^ sender, System::EventArgs^ e)
	{
		OpenFileDialog^ Open_Dialog_File = gcnew OpenFileDialog();
		Open_Dialog_File->InitialDirectory = ".";
		Open_Dialog_File->Filter = "Light Information Files (*.light)|*.light|All Files (*.*)|*.*";
		Open_Dialog_File->RestoreDirectory = true;

		if (Open_Dialog_File->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			System::String^ Filename = Open_Dialog_File->FileName;

			// Get the tracks from the light file
			List<LightTrackInfo^>^ AvailableTracks = Widget_Timeline::GetTracksFromLightFile(Filename);

			if (AvailableTracks->Count == 0) {
				MessageBox::Show(this, "No valid tracks found in the selected file.", "Import Light Information", MessageBoxButtons::OK, MessageBoxIcon::Information);
				return;
			}

			// Show the import dialog
			Form_Light_Import^ ImportForm = gcnew Form_Light_Import(AvailableTracks);
			ImportForm->Owner = this;

			if (ImportForm->ShowDialog() == System::Windows::Forms::DialogResult::OK)
			{
				List<LightTrackInfo^>^ SelectedTracks = ImportForm->SelectedTracks;

				if (SelectedTracks->Count == 0) {
					MessageBox::Show(this, "No tracks were selected for import.", "Import Light Information", MessageBoxButtons::OK, MessageBoxIcon::Information);
					return;
				}

				// Create dictionary mapping track names to existing tracks
				Dictionary<String^, Track^>^ TrackMap = gcnew Dictionary<String^, Track^>();
				for each (Track ^ T in this->_Timeline->Tracks) {
					TrackMap[T->Name] = T;
				}

				// Clear events from all tracks that will be imported
				for each (LightTrackInfo ^ TrackInfo in SelectedTracks) {
					if (TrackMap->ContainsKey(TrackInfo->Name)) {
						TrackMap[TrackInfo->Name]->Events->Clear();
					}
				}

				// Import the selected tracks and their events
				for each (LightTrackInfo ^ TrackInfo in SelectedTracks) {
					// Find or create track
					Track^ TargetTrack = nullptr;

					if (TrackMap->ContainsKey(TrackInfo->Name)) {
						TargetTrack = TrackMap[TrackInfo->Name];
					}
					else {
						// Create a new track if it doesn't exist
						// Use a default octave of 4 (can be changed later in MIDI settings)
						this->_Timeline->AddTrack(TrackInfo->Name, 4);
						TargetTrack = this->_Timeline->Tracks[this->_Timeline->Tracks->Count - 1];
						TrackMap[TrackInfo->Name] = TargetTrack; // Add to map
					}

					// Import events
					for each (BarEvent^ Event in TrackInfo->Events) {
						// We need to make a copy with the correct track reference
						switch (Event->Type) {
						case BarEventType::Solid:
							TargetTrack->AddBar(Event->StartTick, Event->Duration, Event->Color);
							break;

						case BarEventType::Fade:
							if (Event->FadeInfo != nullptr) {
								BarEventFadeInfo^ FadeInfo;

								if (Event->FadeInfo->Type == FadeType::Two_Colors) {
									FadeInfo = gcnew BarEventFadeInfo(
										Event->FadeInfo->QuantizationTicks,
										Event->FadeInfo->ColorStart,
										Event->FadeInfo->ColorEnd,
										Event->FadeInfo->EaseIn,
										Event->FadeInfo->EaseOut);
								}
								else {
									FadeInfo = gcnew BarEventFadeInfo(
										Event->FadeInfo->QuantizationTicks,
										Event->FadeInfo->ColorStart,
										Event->FadeInfo->ColorCenter,
										Event->FadeInfo->ColorEnd,
										Event->FadeInfo->EaseIn,
										Event->FadeInfo->EaseOut);
								}

								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, Event->StartTick, Event->Duration, FadeInfo));
							}
							break;

						case BarEventType::Strobe:
							if (Event->StrobeInfo != nullptr) {
								BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(
									Event->StrobeInfo->QuantizationTicks,
									Event->StrobeInfo->ColorStrobe);

								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, Event->StartTick, Event->Duration, StrobeInfo));
							}
							break;
						}
					}
				}

				// Sort events in each track
				for each (Track ^ T in this->_Timeline->Tracks) {
					T->Events->Sort(Track::barComparer);
				}

				this->_Timeline->Invalidate();
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

			String^ ErrorMessage = this->_Timeline->SaveBarEventsToFile(Filename);

			if (ErrorMessage == String::Empty) {
				//MessageBox::Show(this, "Light Configuration has been successfully saved.", "Save Successful", MessageBoxButtons::OK, MessageBoxIcon::Infor
			}
			else {
				MessageBox::Show(this, "Error:\n" + ErrorMessage, "Failed to save light information to file", MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
		}
	}

	void Form_Main::Menu_File_Export_MIDI_Click(System::Object^ sender, System::EventArgs^ e)
	{
		String^ Default_Filename = this->_Tab_Info->Get_Song_Name() + ".mid";

		SaveFileDialog^ Save_Dialog_File = gcnew SaveFileDialog();
		Save_Dialog_File->InitialDirectory = ".";
		Save_Dialog_File->Filter = "MIDI Files (*.mid)|*.mid|All Files (*.*)|*.*";
		Save_Dialog_File->RestoreDirectory = true;
		Save_Dialog_File->FileName = Default_Filename;

		if (Save_Dialog_File->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			String^ ErrorMessage = _MIDI_Exporter->Export(Save_Dialog_File->FileName, this->_GP_Tab);

			if(ErrorMessage == String::Empty) {
				//MessageBox::Show(this, "MIDI file has been successfully exported.", "Export Successful", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
			else {
				MessageBox::Show(this, "Error:\n" + ErrorMessage, "Failed to export MIDI file", MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
		}
	}

	void Form_Main::Menu_File_Exit_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->Close();
	}

	void Form_Main::Menu_Edit_Undo_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->_Timeline != nullptr) {
			this->_Timeline->Undo();
			UpdateUndoRedoState();
		}
	}

	void Form_Main::Menu_Edit_UndoHistory_Click(Object^ sender, EventArgs^ e)
	{
		ToolStripMenuItem^ Item = safe_cast<ToolStripMenuItem^>(sender);

		int TargetIndex = safe_cast<int>(Item->Tag);
		int CurrentIndex = _Timeline->CommandManager()->GetCurrentIndex();

		for (int i = 0; i <= TargetIndex && i <= CurrentIndex; i++) {
			_Timeline->Undo();
		}
	}

	void Form_Main::Menu_Edit_Redo_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->_Timeline != nullptr) {
			this->_Timeline->Redo();
			UpdateUndoRedoState();
		}
	}

	void Form_Main::Menu_Edit_Copy_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (_Timeline != nullptr && _Timeline->CurrentTool == TimelineToolType::Pointer)
		{
			PointerTool^ PointerTool = _Timeline->GetPointerTool();
			if (PointerTool != nullptr) {
				PointerTool->HandleCopy();
			}
		}
	}

	void Form_Main::Menu_Edit_Paste_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (_Timeline != nullptr && _Timeline->CurrentTool == TimelineToolType::Pointer)
		{
			PointerTool^ PointerTool = _Timeline->GetPointerTool();
			if (PointerTool != nullptr) {
				PointerTool->StartPaste();
			}
		}
	}

	void Form_Main::Menu_Edit_Delete_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (_Timeline == nullptr) {
			return;
		}
		
		if (_Timeline->CurrentTool == TimelineToolType::Pointer)
		{
			PointerTool^ PointerTool = _Timeline->GetPointerTool();
			if (PointerTool != nullptr && PointerTool->SelectedBars->Count > 0)
			{
				System::Windows::Forms::KeyEventArgs^ keyArgs = gcnew System::Windows::Forms::KeyEventArgs(Keys::Delete);
				PointerTool->OnKeyDown(keyArgs);
			}
		}
		else if (_Timeline->CurrentTool == TimelineToolType::Erase)
		{
			EraseTool^ EraseTool = _Timeline->GetEraseTool();
			if (EraseTool != nullptr && EraseTool->SelectedBars->Count > 0)
			{
				System::Windows::Forms::KeyEventArgs^ keyArgs = gcnew System::Windows::Forms::KeyEventArgs(Keys::Delete);
				EraseTool->OnKeyDown(keyArgs);
			}
		}
	}

	void Form_Main::Menu_Edit_BatchAction_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->_Toolbar->CurrentTool = TimelineToolType::Pointer;
		
		Form_BatchAction^ BatchForm = gcnew Form_BatchAction(this->_Timeline);
		BatchForm->Owner = this;
		BatchForm->ShowDialog();
	}

	void Form_Main::Menu_Settings_Hotkeys_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Form_Settings_Hotkeys^ Hotkey_Form = gcnew Form_Settings_Hotkeys();
		Hotkey_Form->Owner = this;
		Hotkey_Form->ShowDialog();
	}

	void Form_Main::Menu_Settings_Midi_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Form_Settings_MIDI^ Midi_Form = gcnew Form_Settings_MIDI();
		Midi_Form->On_Settings_Accepted += gcnew On_Settings_Accepted_Handler(this, &Form_Main::SettingsMIDI_On_Settings_Accepted);
		Midi_Form->ShowDialog();
	}

	void Form_Main::Menu_Settings_Device_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Form_Device_Configuration^ Device_Form = gcnew Form_Device_Configuration();
		Device_Form->On_Device_Settings_Changed += gcnew On_Device_Settings_Changed_Handler(this, &Form_Main::SettingsDevice_On_Settings_Changed);
		Device_Form->ShowDialog();
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

	void Form_Main::UpdateUndoRedoState()
	{
		if (this->_Timeline != nullptr) {
			_Menu_Edit_Undo->Enabled		= this->_Timeline->CommandManager()->CanUndo;
			_Menu_Edit_UndoSteps->Enabled	= this->_Timeline->CommandManager()->CanUndo;
			_Menu_Edit_Redo->Enabled		= this->_Timeline->CommandManager()->CanRedo;
		}

		UpdateEditMenuState();
		UpdateUndoHistoryMenu();
	}

	void Form_Main::UpdateUndoHistoryMenu()
	{
		if (_Timeline == nullptr) {
			return;
		}

		auto Commands		= _Timeline->CommandManager()->GetCommands();
		int CurrentIndex	= _Timeline->CommandManager()->GetCurrentIndex();

		for (int i = 0; i < _Menu_Edit_UndoSteps_Items->Count; i++)
		{
			if (i <= CurrentIndex && i < Commands->Count) {
				_Menu_Edit_UndoSteps_Items[i]->Text = Commands[i]->GetDescription();
				_Menu_Edit_UndoSteps_Items[i]->Tag = i;
				_Menu_Edit_UndoSteps_Items[i]->Visible = true;
			}
			else {
				_Menu_Edit_UndoSteps_Items[i]->Visible = false;
			}
		}
	}

	void Form_Main::UpdateEditMenuState(System::Object^ sender, MIDILightDrawer::TimelineToolType e)
	{
		UpdateEditMenuState();
	}

	void Form_Main::UpdateEditMenuState(System::Object^ sender, System::EventArgs^ e)
	{
		UpdateEditMenuState();
	}

	void Form_Main::UpdateEditMenuState()
	{
		if (_Timeline == nullptr) {
			_Menu_Edit_Copy->Enabled = false;
			_Menu_Edit_Paste->Enabled = false;
			_Menu_Edit_Delete->Enabled = false;
			return;
		}

		bool IsPointerToolActive	= _Timeline->CurrentTool == TimelineToolType::Pointer;
		bool IsEraseToolActive		= _Timeline->CurrentTool == TimelineToolType::Erase;

		PointerTool^ PointerTool	= _Timeline->GetPointerTool();
		EraseTool^ EraseTool		= _Timeline->GetEraseTool();

		// Handle Copy and Paste (only for pointer tool)
		if (PointerTool != nullptr && IsPointerToolActive) {
			bool hasSelection = PointerTool->SelectedBars != nullptr && PointerTool->SelectedBars->Count > 0;
			_Menu_Edit_Copy->Enabled = hasSelection;

			bool hasClipboardContent = TimelineClipboardManager::Content != nullptr && TimelineClipboardManager::Content->Count > 0;
			_Menu_Edit_Paste->Enabled = hasClipboardContent;
		}
		else {
			_Menu_Edit_Copy->Enabled = false;
			_Menu_Edit_Paste->Enabled = false;
		}

		// Handle Delete (for both pointer and erase tools)
		bool HasSelectedBarsForDelete = false;

		if (PointerTool != nullptr && IsPointerToolActive) {
			HasSelectedBarsForDelete = PointerTool->SelectedBars != nullptr && PointerTool->SelectedBars->Count > 0;
		}
		else if (EraseTool != nullptr && IsEraseToolActive) {
			HasSelectedBarsForDelete = EraseTool->SelectedBars != nullptr && EraseTool->SelectedBars->Count > 0;
		}

		_Menu_Edit_Delete->Enabled = HasSelectedBarsForDelete;
	}

	void Form_Main::Toolbar_OnToolChanged(System::Object^ sender, TimelineToolType e)
	{
		PointerTool^ Pointer_Tool	= this->_Timeline->GetPointerTool();
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
			
			Pointer_Tool->ChangeTickLength = _Pointer_Options->ResizeQuantization;
			break;

		case TimelineToolType::Draw:
			this->_Timeline->SetCurrentTool(TimelineToolType::Draw);
			this->_Timeline->SetToolSnapping((SnappingType)_Draw_Options->DrawSnapping);

			Draw_Tool->DrawTickLength	= _Draw_Options->DrawLength;
			Draw_Tool->UseAutoLength	= _Draw_Options->LengthByTablature;
			Draw_Tool->DrawColor		= _Draw_Options->SelectedColor;
			break;

		case TimelineToolType::Split:
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
			Fade_Tool->Type			= _Fade_Options->FadeMode;
			Fade_Tool->ColorStart	= _Fade_Options->StartColor;
			Fade_Tool->ColorEnd		= _Fade_Options->EndColor;
			Fade_Tool->ColorCenter	= _Fade_Options->CenterColor;
			Fade_Tool->EaseIn		= _Fade_Options->EaseIn;
			Fade_Tool->EaseOut		= _Fade_Options->EaseOut;
			break;

		case TimelineToolType::Strobe:
			this->_Timeline->SetCurrentTool(TimelineToolType::Strobe);

			Strobe_Tool->TickLength		= _Strobe_Options->TickLength;
			Strobe_Tool->ColorStrobe	= _Strobe_Options->SelectedColor;
			break;
		}
	}

	void Form_Main::SettingsMIDI_On_Settings_Accepted()
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

		UpdateUndoRedoState();
	}

	void Form_Main::SettingsDevice_On_Settings_Changed()
	{
		Initialize_Playback_System();
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
		if (this->_Tools_And_Control->ColorPickerIsTyping()) {
			return false;
		}
		
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
				//else if (hotkey.Key == "Duration Tool") { this->_Toolbar->CurrentTool = TimelineToolType::Duration; return true; }
				else if (hotkey.Key == "Fade Tool")		{ this->_Toolbar->CurrentTool = TimelineToolType::Fade;		return true; }
				else if (hotkey.Key == "Strobe Tool")	{ this->_Toolbar->CurrentTool = TimelineToolType::Strobe;	return true; }
				//else if (hotkey.Key == "Bucket Tool") { this->_Toolbar->CurrentTool = TimelineToolType::Pointer; return true; }

				else if (hotkey.Key->StartsWith("Select Color "))
				{
					String^ color_num = hotkey.Key->Substring(13);
					int color_index = Int32::Parse(color_num);
					this->_Tools_And_Control->SelectColorFromPreset(color_index-1);
					return true;
				}

				else if (hotkey.Key == "Length Up"						) { this->_Tools_And_Control->LengthUp();				return true; }
				else if (hotkey.Key == "Length Down"					) { this->_Tools_And_Control->LengthDown();			return true; }
				else if (hotkey.Key == "Draw Tool - Toggle Length Tab"	) { this->_Draw_Options->Toggle_LengthByTablature();	return true; }

				else if (hotkey.Key == "Snapping Next"					) { this->_Tools_And_Control->SnappingUp();			return true; }
				else if (hotkey.Key == "Snapping Previous"				) { this->_Tools_And_Control->SnappingDown();			return true; }
				else if (hotkey.Key == "Snap To None"					) { this->_Tools_And_Control->SnapTo((int)SnappingType::Snap_None);		return true; }
				else if (hotkey.Key == "Snap To Grid"					) { this->_Tools_And_Control->SnapTo((int)SnappingType::Snap_Grid);		return true; }
				else if (hotkey.Key == "Snap To Bars"					) { this->_Tools_And_Control->SnapTo((int)SnappingType::Snap_Events);		return true; }
				else if (hotkey.Key == "Snap To Tablature"				) { this->_Tools_And_Control->SnapTo((int)SnappingType::Snap_Tablature);	return true; }

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

		if (e->Control && e->KeyCode == Keys::Z)
		{
			Menu_Edit_Undo_Click(sender, e);
			e->Handled = true;
		}
		else if (e->Control && e->KeyCode == Keys::Y)
		{
			Menu_Edit_Redo_Click(sender, e);
			e->Handled = true;
		}
		else if (e->Control && e->KeyCode == Keys::C)
		{
			Menu_Edit_Copy_Click(sender, e);
			e->Handled = true;
		}
		else if (e->Control && e->KeyCode == Keys::V)
		{
			Menu_Edit_Paste_Click(sender, e);
			e->Handled = true;
		}
		else if (e->KeyCode == Keys::Delete)
		{
			Menu_Edit_Delete_Click(sender, e);
			e->Handled = true;
		}
		else if (e->Control && e->KeyCode == Keys::O)
		{
			// Open GP Tab
			e->Handled = true;
		}
		else if (e->Control && e->Shift && e->KeyCode == Keys::O)
		{
			// Open Light file
			e->Handled = true;
		}
		else if (e->Control && e->KeyCode == Keys::S)
		{
			// Save Light file
			e->Handled = true;
		}
		else if (e->Control && e->KeyCode == Keys::E)
		{
			// MIDI Export
			e->Handled = true;
		}
		else if (e->Alt && e->KeyCode == Keys::F4)
		{
			// Exit
			e->Handled = true;
		}
		else if (e->KeyCode == Keys::Escape)
		{
			
		}
		else
		{
			Process_Hotkey(e->KeyCode);
			e->Handled = true;
		}
	}

	void Form_Main::Playback_Update_Timer_Tick(Object^ sender, EventArgs^ e)
	{
		if (_Playback_Manager == nullptr || _Transport_Controls == nullptr) {
			return;
		}

		_Transport_Controls->Update_State(_Playback_Manager->Is_Playing());

		if (_Transport_Controls->Is_Playing) {
			this->_Timeline->AutoScrollForPlayback();
		}
		else if (_Transport_Controls->Is_Rewinding) {
			this->_Timeline->AutoScrollForPlayback();
		}
		else if (_Transport_Controls->Is_Fast_Forwarding) {
			this->_Timeline->AutoScrollForPlayback();
		}
		else if (_Transport_Controls->Moved_To_Start) {
			this->_Timeline->AutoScrollForPlayback();
		}
		else if (_Transport_Controls->Moved_To_End) {
			this->_Timeline->AutoScrollForPlayback();
		}
	}

	void Form_Main::Pointer_Options_OnSnappingChanged(int value)
	{
		this->_Timeline->SetToolSnapping((SnappingType)value);
	}

	void Form_Main::Pointer_Options_OnQuantizationChanged(int value)
	{
		PointerTool^ Pointer_Tool = this->_Timeline->GetPointerTool();
		Pointer_Tool->ChangeTickLength = value;
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

	void Form_Main::Fade_Options_OnFadeModeChanged(FadeType mode)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->Type = _Fade_Options->FadeMode;
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

	void Form_Main::Fade_Options_OnEasingsChanged(FadeEasing easeIn, FadeEasing easeOut)
	{
		FadeTool^ Fade_Tool = this->_Timeline->GetFadeTool();
		Fade_Tool->EaseIn = easeIn;
		Fade_Tool->EaseOut = easeOut;
	}

	void Form_Main::Strobe_Options_OnLengthChanged(int value)
	{
		StrobeTool^ Strobe_Tool = this->_Timeline->GetStrobeTool();
		Strobe_Tool->TickLength = value;
	}

	void Form_Main::Strobe_Options_OnColorChanged(System::Drawing::Color color)
	{
		StrobeTool^ Strobe_Tool = this->_Timeline->GetStrobeTool();
		Strobe_Tool->ColorStrobe = color;
	}

	void Form_Main::Bucket_Options_OnColorChanged(System::Drawing::Color color)
	{

	}

	void Form_Main::Button_1_Click(System::Object^ sender, System::EventArgs^ e)
	{
#ifdef _DEBUG
		Console::WriteLine("Button 1 Clicked");

		this->_Timeline->LogPerformanceMetrics();
#endif
	}

	void Form_Main::Button_2_Click(System::Object^ sender, System::EventArgs^ e)
	{
#ifdef _DEBUG
		Console::WriteLine("Button 2 Clicked");

#endif
	}
}






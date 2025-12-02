#include "Form_Auto_Generate.h"
#include "Theme_Manager.h"
#include "Dialog_ColorPicker.h"

namespace MIDILightDrawer
{
	//////////////////////////////////
	// Audio_Analysis_Data
	//////////////////////////////////
	Audio_Analysis_Data::Audio_Analysis_Data()
	{
		Energy_Points = gcnew List<Audio_Energy_Point>();
		Segments = gcnew List<Audio_Segment>();
		Global_Average_Energy = 0.0f;
		Global_Peak_Energy = 0.0f;
		Total_Duration_ms = 0.0;
	}

	float Audio_Analysis_Data::GetEnergyAtTime(double time_ms)
	{
		if (Energy_Points->Count == 0) return 0.0f;

		// Binary search for closest point
		int Low = 0;
		int High = Energy_Points->Count - 1;

		while (Low < High)
		{
			int Mid = (Low + High) / 2;
			if (Energy_Points[Mid].Time_ms < time_ms) {
				Low = Mid + 1;
			}
			else {
				High = Mid;
			}
		}

		// Interpolate if possible
		if (Low > 0 && Low < Energy_Points->Count)
		{
			Audio_Energy_Point Prev = Energy_Points[Low - 1];
			Audio_Energy_Point Next = Energy_Points[Low];

			double Range = Next.Time_ms - Prev.Time_ms;
			if (Range > 0)
			{
				float Ratio = (float)((time_ms - Prev.Time_ms) / Range);
				return Prev.Energy + (Next.Energy - Prev.Energy) * Ratio;
			}
		}

		return Low < Energy_Points->Count ? Energy_Points[Low].Energy : 0.0f;
	}

	float Audio_Analysis_Data::GetNormalizedEnergyAtTime(double time_ms)
	{
		if (Global_Peak_Energy <= 0.0f) return 0.0f;
		return GetEnergyAtTime(time_ms) / Global_Peak_Energy;
	}

	bool Audio_Analysis_Data::IsTransientAtTime(double time_ms, double tolerance_ms)
	{
		for each (Audio_Energy_Point Point in Energy_Points)
		{
			if (Point.Is_Transient &&
				Math::Abs(Point.Time_ms - time_ms) <= tolerance_ms)
			{
				return true;
			}
		}
		return false;
	}

	bool Audio_Analysis_Data::IsOnsetAtTime(double time_ms, double tolerance_ms)
	{
		for each (Onset_Info Onset in Onsets)
		{
			if (Math::Abs(Onset.Time_ms - time_ms) <= tolerance_ms)
				return true;
		}
		return false;
	}

	Spectral_Energy Audio_Analysis_Data::GetSpectrumAtTime(double time_ms)
	{
		if (Energy_Points->Count == 0)
			return Spectral_Energy();

		// Binary search for closest point
		int Low = 0;
		int High = Energy_Points->Count - 1;
		int Closest_Index = 0;
		double Min_Diff = Double::MaxValue;

		// First, narrow down using binary search
		while (Low <= High)
		{
			int Mid = (Low + High) / 2;
			double Diff = Math::Abs(Energy_Points[Mid].Time_ms - time_ms);

			if (Diff < Min_Diff)
			{
				Min_Diff = Diff;
				Closest_Index = Mid;
			}

			if (Energy_Points[Mid].Time_ms < time_ms)
				Low = Mid + 1;
			else if (Energy_Points[Mid].Time_ms > time_ms)
				High = Mid - 1;
			else
				break;	// Exact match
		}

		return Energy_Points[Closest_Index].Spectrum;
	}

	Frequency_Band Audio_Analysis_Data::GetDominantBandAtTime(double time_ms)
	{
		Spectral_Energy Spectrum = GetSpectrumAtTime(time_ms);

		float Max_Energy = Spectrum.Sub_Bass;
		Frequency_Band Dominant = Frequency_Band::Sub_Bass;

		if (Spectrum.Bass > Max_Energy) { Max_Energy = Spectrum.Bass; Dominant = Frequency_Band::Bass; }
		if (Spectrum.Low_Mid > Max_Energy) { Max_Energy = Spectrum.Low_Mid; Dominant = Frequency_Band::Low_Mid; }
		if (Spectrum.Mid > Max_Energy) { Max_Energy = Spectrum.Mid; Dominant = Frequency_Band::Mid; }
		if (Spectrum.High_Mid > Max_Energy) { Max_Energy = Spectrum.High_Mid; Dominant = Frequency_Band::High_Mid; }
		if (Spectrum.High > Max_Energy) { Max_Energy = Spectrum.High; Dominant = Frequency_Band::High; }
		if (Spectrum.Brilliance > Max_Energy) { Dominant = Frequency_Band::Brilliance; }

		return Dominant;
	}

	List<Onset_Info>^ Audio_Analysis_Data::GetOnsetsInRange(double start_ms, double end_ms)
	{
		List<Onset_Info>^ Result = gcnew List<Onset_Info>();

		for each (Onset_Info Onset in Onsets)
		{
			if (Onset.Time_ms >= start_ms && Onset.Time_ms < end_ms)
			{
				Result->Add(Onset);
			}
		}

		return Result;
	}

	//////////////////////////////////
	// Tablature_Analysis_Data
	//////////////////////////////////
	Tablature_Analysis_Data::Tablature_Analysis_Data()
	{
		Events = gcnew List<Tab_Event_Info>();
		Measures = gcnew List<Measure_Analysis>();
		Total_Notes = 0;
		Total_Beats = 0;
		Average_Note_Density = 0.0f;
	}

	List<Tab_Event_Info>^ Tablature_Analysis_Data::GetEventsInRange(int start_tick, int end_tick)
	{
		List<Tab_Event_Info>^ Result = gcnew List<Tab_Event_Info>();

		for each (Tab_Event_Info Event in Events)
		{
			if (Event.Start_Tick >= start_tick && Event.Start_Tick < end_tick)
			{
				Result->Add(Event);
			}
		}

		return Result;
	}

	Measure_Analysis Tablature_Analysis_Data::GetMeasureAt(int tick)
	{
		for each (Measure_Analysis Measure in Measures)
		{
			if (tick >= Measure.Start_Tick &&
				tick < Measure.Start_Tick + Measure.Length_Ticks)
			{
				return Measure;
			}
		}

		// Return empty measure if not found
		Measure_Analysis Empty;
		Empty.Measure_Index = -1;
		return Empty;
	}

	//////////////////////////////////
	// Auto_Generate_Settings
	//////////////////////////////////
	Auto_Generate_Settings::Auto_Generate_Settings()
	{
		// Default values
		Mode = Generation_Mode::Follow_Tablature;
		Distribution = Event_Distribution::Per_Beat;
		Custom_Interval_Ticks = 480;	// Quarter note
		Clear_Existing_Events = true;

		Selected_Track_Indices = gcnew List<int>();
		Apply_To_All_Tracks = true;

		Start_Measure = 1;
		End_Measure = 1;
		Use_Full_Range = true;

		Color_Selection_Mode = Color_Mode::Single_Color;
		Primary_Color = Color::Red;
		Secondary_Color = Color::Green;
		Tertiary_Color = Color::Blue;
		Color_Palette = gcnew List<Color>();
		Color_Palette->Add(Color::Red);
		Color_Palette->Add(Color::Green);
		Color_Palette->Add(Color::Blue);
		Color_Palette->Add(Color::Yellow);
		Color_Palette->Add(Color::Cyan);
		Color_Palette->Add(Color::Magenta);

		Type_Selection_Mode = Event_Type_Mode::Solid_Only;
		Energy_Threshold_Strobe = 0.8f;
		Energy_Threshold_Fade = 0.5f;
		Strobe_Quantization_Ticks = 240;	// 16th note
		Fade_Quantization_Ticks = 240;

		Default_Ease_In = FadeEasing::Linear;
		Default_Ease_Out = FadeEasing::Linear;

		Beat_Detection_Sensitivity = 0.5f;
		Transient_Detection_Sensitivity = 0.5f;
		Use_Audio_Energy = true;
		Use_Audio_Beats = true;

		Use_Note_Velocity = true;
		Use_Note_Effects = true;
		Detect_Accents = true;
		Accent_Threshold = 0.7f;

		Auto_Duration = true;
		Fixed_Duration_Ticks = 480;
		Duration_Scale_Factor = 1.0f;
		Minimum_Duration_Ticks = 120;
		Maximum_Duration_Ticks = 3840;

		Fill_Gaps = false;
		Gap_Fill_Mode = 0;
		Minimum_Gap_Ticks = 120;
	}

	//////////////////////////////////
	// Form_Auto_Generate Constructor/Destructor
	//////////////////////////////////
	Form_Auto_Generate::Form_Auto_Generate(Widget_Timeline^ timeline, Playback_Audio_Engine^ audio_engine)
	{
		_Timeline = timeline;
		_Audio_Engine = audio_engine;

		if (_Audio_Engine != nullptr && _Audio_Engine->Is_Audio_Loaded)
		{
			_Waveform_Data = _Audio_Engine->Waveform_Data;
			_Audio_Duration_ms = _Audio_Engine->Duration_ms;
		}
		else
		{
			_Waveform_Data = nullptr;
			_Audio_Duration_ms = 0.0;
		}


		_Settings = gcnew Auto_Generate_Settings();
		_Audio_Analysis = gcnew Audio_Analysis_Data();
		_Tab_Analysis = gcnew Tablature_Analysis_Data();

		_Is_Generating = false;
		_Preview_Events = gcnew List<BarEvent^>();

		_Background_Worker = gcnew BackgroundWorker();
		_Background_Worker->WorkerReportsProgress = true;
		_Background_Worker->WorkerSupportsCancellation = true;
		_Background_Worker->DoWork += gcnew DoWorkEventHandler(this, &Form_Auto_Generate::OnBackgroundWorkerDoWork);
		_Background_Worker->ProgressChanged += gcnew ProgressChangedEventHandler(this, &Form_Auto_Generate::OnBackgroundWorkerProgressChanged);
		_Background_Worker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &Form_Auto_Generate::OnBackgroundWorkerCompleted);

		InitializeComponent();
		PopulateDropDowns();
		PopulateTrackList();
		ApplyTheme();
		UpdateUIState();
	}

	Form_Auto_Generate::~Form_Auto_Generate()
	{
		if (_Background_Worker != nullptr)
		{
			if (_Background_Worker->IsBusy) {
				_Background_Worker->CancelAsync();
			}
			delete _Background_Worker;
		}
	}

	//////////////////////////////////
	// UI Initialization
	//////////////////////////////////
	void Form_Auto_Generate::InitializeComponent()
	{
		this->Text = "Auto Generate Light Events";
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->ShowInTaskbar = false;
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
		this->Size = System::Drawing::Size(800, 800);

		// Main layout
		_Main_Layout = gcnew TableLayoutPanel();
		_Main_Layout->Dock = DockStyle::Fill;
		_Main_Layout->ColumnCount = 1;
		_Main_Layout->RowCount = 2;
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 100));
		_Main_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		_Main_Layout->Padding = System::Windows::Forms::Padding(10);

		// Scrollable panel for settings
		_Scrollable_Panel = gcnew Control_ScrollablePanel();
		_Scrollable_Panel->Dock = DockStyle::Fill;
		_Scrollable_Panel->AutoScroll = true;

		// Create a flow layout panel for the settings sections
		FlowLayoutPanel^ Settings_Flow = gcnew FlowLayoutPanel();
		//Settings_Flow->Dock = DockStyle::Fill;
		Settings_Flow->FlowDirection = FlowDirection::TopDown;
		Settings_Flow->WrapContents = false;
		Settings_Flow->AutoSize = true;
		Settings_Flow->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		//Settings_Flow->Width = 500;
		Settings_Flow->AutoScroll = false;

		// Initialize all sections
		InitializeComponentModeSection();
		InitializeComponentTrackSection();
		InitializeComponentRangeSection();
		InitializeComponentColorSection();
		InitializeComponentEventTypeSection();
		InitializeComponentFadeSection();
		InitializeComponentDurationSection();
		InitializeComponentAudioSection();
		InitializeComponentOptionsSection();

		// Add group boxes to flow panel
		Settings_Flow->Controls->Add(_GroupBox_Mode);
		Settings_Flow->Controls->Add(_GroupBox_Tracks);
		Settings_Flow->Controls->Add(_GroupBox_Range);
		Settings_Flow->Controls->Add(_GroupBox_Colors);
		Settings_Flow->Controls->Add(_GroupBox_Event_Type);
		Settings_Flow->Controls->Add(_GroupBox_Fade_Settings);
		Settings_Flow->Controls->Add(_GroupBox_Duration);
		Settings_Flow->Controls->Add(_GroupBox_Audio);
		Settings_Flow->Controls->Add(_GroupBox_Options);

		_Scrollable_Panel->ContentPanel->Controls->Add(Settings_Flow);
		_Main_Layout->Controls->Add(_Scrollable_Panel, 0, 0);

		// Progress and buttons panel
		InitializeComponentProgressAndButtons();
		_Main_Layout->Controls->Add(_Panel_Progress, 0, 1);

		this->Controls->Add(_Main_Layout);
	}

	void Form_Auto_Generate::InitializeComponentModeSection()
	{
		_GroupBox_Mode = gcnew Control_GroupBox();
		_GroupBox_Mode->Text = "Generation Mode";
		_GroupBox_Mode->Size = System::Drawing::Size(715, 150);
		_GroupBox_Mode->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 3;
		Layout->RowCount = 3;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 150));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 350));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));

		_Label_Mode = gcnew Label();
		_Label_Mode->Text = "Mode:";
		_Label_Mode->AutoSize = true;
		_Label_Mode->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Mode, 0, 0);

		_ComboBox_Mode = gcnew Control_ComboBox();
		_ComboBox_Mode->Dock = DockStyle::Fill;
		_ComboBox_Mode->Drop_Down_Style = ComboBoxStyle::DropDownList;
		_ComboBox_Mode->Selection_Changed += gcnew MIDILightDrawer::Control_ComboBox_Selection_Changed_Event_Handler(this, &Form_Auto_Generate::OnModeSelection_Changed);
		Layout->Controls->Add(_ComboBox_Mode, 1, 0);

		_Label_Distribution = gcnew Label();
		_Label_Distribution->Text = "Distribution:";
		_Label_Distribution->AutoSize = true;
		_Label_Distribution->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Distribution, 0, 1);

		_ComboBox_Distribution = gcnew Control_ComboBox();
		_ComboBox_Distribution->Dock = DockStyle::Fill;
		_ComboBox_Distribution->Drop_Down_Style = ComboBoxStyle::DropDownList;
		_ComboBox_Distribution->Selection_Changed += gcnew Control_ComboBox_Selection_Changed_Event_Handler(this, &Form_Auto_Generate::OnDistributionSelection_Changed);
		Layout->Controls->Add(_ComboBox_Distribution, 1, 1);

		_Label_Custom_Interval = gcnew Label();
		_Label_Custom_Interval->Text = "Interval (Ticks):";
		_Label_Custom_Interval->AutoSize = true;
		_Label_Custom_Interval->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Custom_Interval, 0, 2);

		_TextBox_Custom_Interval = gcnew TextBox();
		_TextBox_Custom_Interval->Text = "480";
		_TextBox_Custom_Interval->Width = 100;
		_TextBox_Custom_Interval->Enabled = false;
		_TextBox_Custom_Interval->Margin = System::Windows::Forms::Padding(3, 7, 0, 0);
		Layout->Controls->Add(_TextBox_Custom_Interval, 1, 2);

		_GroupBox_Mode->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentTrackSection()
	{
		_GroupBox_Tracks = gcnew Control_GroupBox();
		_GroupBox_Tracks->Text = "Track Selection";
		_GroupBox_Tracks->Size = System::Drawing::Size(715, 200);
		_GroupBox_Tracks->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 1;
		Layout->RowCount = 2;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));

		_CheckBox_All_Tracks = gcnew CheckBox();
		_CheckBox_All_Tracks->Text = "Apply to all tracks";
		_CheckBox_All_Tracks->Checked = true;
		_CheckBox_All_Tracks->AutoSize = true;
		_CheckBox_All_Tracks->CheckedChanged += gcnew EventHandler(this, &Form_Auto_Generate::OnAllTracksChanged);
		Layout->Controls->Add(_CheckBox_All_Tracks, 0, 0);

		_CheckedList_Tracks = gcnew Control_CheckedListBox();
		_CheckedList_Tracks->Dock = DockStyle::Fill;
		_CheckedList_Tracks->Enabled = false;
		_CheckedList_Tracks->BorderStyle = BorderStyle::FixedSingle;
		Layout->Controls->Add(_CheckedList_Tracks, 0, 1);

		_GroupBox_Tracks->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentRangeSection()
	{
		_GroupBox_Range = gcnew Control_GroupBox();
		_GroupBox_Range->Text = "Range Selection";
		_GroupBox_Range->Size = System::Drawing::Size(715, 100);
		_GroupBox_Range->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 6;
		Layout->RowCount = 2;
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 65));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent,  100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 60));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 30));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 60));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 20));

		_CheckBox_Full_Range = gcnew CheckBox();
		_CheckBox_Full_Range->Text = "Use full song range";
		_CheckBox_Full_Range->Checked = true;
		_CheckBox_Full_Range->AutoSize = true;
		_CheckBox_Full_Range->CheckedChanged += gcnew EventHandler(this, &Form_Auto_Generate::OnFullRangeChanged);
		Layout->Controls->Add(_CheckBox_Full_Range, 0, 0);
		Layout->SetColumnSpan(_CheckBox_Full_Range, 4);

		_Label_Range = gcnew Label();
		_Label_Range->Text = "Range:";
		_Label_Range->AutoSize = true;
		_Label_Range->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Range, 0, 1);

		_TrackBar_Measure = gcnew Control_Trackbar_Range();
		_TrackBar_Measure->Mode = TrackbarRangeMode::Range;
		_TrackBar_Measure->Size = System::Drawing::Size(350, 30);
		_TrackBar_Measure->Minimum = 0;
		_TrackBar_Measure->Maximum = 1; // Will be updated based on timeline

		// Initialize with measure 1 or measures 0 if no timeline
		int MaxMeasure = (_Timeline->Measures->Count > 0) ? _Timeline->Measures->Count : 1;
		_TrackBar_Measure->Maximum = MaxMeasure;
		_TrackBar_Measure->Value = 1;
		_TrackBar_Measure->MinValue = 0;
		_TrackBar_Measure->MaxValue = MaxMeasure;
		_TrackBar_Measure->TrackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
		_TrackBar_Measure->RangeColor = Theme_Manager::Get_Instance()->AccentPrimary;
		_TrackBar_Measure->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_Auto_Generate::OnMeasureValueChanged);

		Layout->Controls->Add(_TrackBar_Measure, 1, 1);

		_TextBox_Start_Measure = gcnew TextBox();
		_TextBox_Start_Measure->Text = _TrackBar_Measure->Minimum.ToString();
		_TextBox_Start_Measure->Width = 60;
		_TextBox_Start_Measure->TextAlign = HorizontalAlignment::Center;
		_TextBox_Start_Measure->Margin = System::Windows::Forms::Padding(0, 6, 0, 0);
		Layout->Controls->Add(_TextBox_Start_Measure, 2, 1);

		_Label_Range_To = gcnew Label();
		_Label_Range_To->Dock = DockStyle::Fill;
		_Label_Range_To->Text = "to";
		_Label_Range_To->TextAlign = ContentAlignment::MiddleCenter;
		_Label_Range_To->Padding = System::Windows::Forms::Padding(0, 0, 0, 1);
		Layout->Controls->Add(_Label_Range_To, 3, 1);

		_TextBox_End_Measure = gcnew TextBox();
		_TextBox_End_Measure->Text = _TrackBar_Measure->Maximum.ToString();
		_TextBox_End_Measure->Width = 60;
		_TextBox_End_Measure->TextAlign = HorizontalAlignment::Center;
		_TextBox_End_Measure->Margin = System::Windows::Forms::Padding(0, 6, 0, 0);
		Layout->Controls->Add(_TextBox_End_Measure, 4, 1);

		_GroupBox_Range->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentColorSection()
	{
		_GroupBox_Colors = gcnew Control_GroupBox();
		_GroupBox_Colors->Text = "Color Settings";
		_GroupBox_Colors->Size = System::Drawing::Size(715, 172);
		_GroupBox_Colors->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 5;
		Layout->RowCount = 5;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 35));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 60));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 200));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		
		_Label_Color_Mode = gcnew Label();
		_Label_Color_Mode->Text = "Color Mode:";
		_Label_Color_Mode->AutoSize = true;
		_Label_Color_Mode->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Color_Mode, 0, 0);

		_ComboBox_Color_Mode = gcnew Control_ComboBox();
		_ComboBox_Color_Mode->Dock = DockStyle::Fill;
		_ComboBox_Color_Mode->Drop_Down_Style = ComboBoxStyle::DropDownList;
		_ComboBox_Color_Mode->Selection_Changed += gcnew Control_ComboBox_Selection_Changed_Event_Handler(this, &Form_Auto_Generate::OnColorModeSelection_Changed);
		Layout->Controls->Add(_ComboBox_Color_Mode, 1, 0);
		Layout->SetColumnSpan(_ComboBox_Color_Mode, 3);

		// Primary color
		Label^ Label_Primary = gcnew Label();
		Label_Primary->Dock = DockStyle::Fill;
		Label_Primary->Text = "Primary:";
		Label_Primary->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(Label_Primary, 0, 1);

		_PictureBox_Primary_Color = gcnew PictureBox();
		_PictureBox_Primary_Color->Size = System::Drawing::Size(30, 25);
		_PictureBox_Primary_Color->BackColor = _Settings->Primary_Color;
		_PictureBox_Primary_Color->BorderStyle = BorderStyle::FixedSingle;
		Layout->Controls->Add(_PictureBox_Primary_Color, 1, 1);

		_Button_Primary_Color = gcnew Button();
		_Button_Primary_Color->Dock = DockStyle::Fill;
		_Button_Primary_Color->Text = "...";
		_Button_Primary_Color->Tag = "Primary";
		_Button_Primary_Color->Click += gcnew EventHandler(this, &Form_Auto_Generate::OnPickColorClick);
		Layout->Controls->Add(_Button_Primary_Color, 2, 1);

		// Secondary color
		Label^ Label_Secondary = gcnew Label();
		Label_Secondary->Dock = DockStyle::Fill;
		Label_Secondary->Text = "Secondary:";
		Label_Secondary->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(Label_Secondary, 0, 2);

		_PictureBox_Secondary_Color = gcnew PictureBox();
		_PictureBox_Secondary_Color->Size = System::Drawing::Size(30, 25);
		_PictureBox_Secondary_Color->BackColor = _Settings->Secondary_Color;
		_PictureBox_Secondary_Color->BorderStyle = BorderStyle::FixedSingle;
		Layout->Controls->Add(_PictureBox_Secondary_Color, 1, 2);

		_Button_Secondary_Color = gcnew Button();
		_Button_Secondary_Color->Dock = DockStyle::Fill;
		_Button_Secondary_Color->Text = "...";
		_Button_Secondary_Color->Tag = "Secondary";
		_Button_Secondary_Color->Click += gcnew EventHandler(this, &Form_Auto_Generate::OnPickColorClick);
		Layout->Controls->Add(_Button_Secondary_Color, 2, 2);

		// Tertiary color
		Label^ Label_Tertiary = gcnew Label();
		Label_Tertiary->Dock = DockStyle::Fill;
		Label_Tertiary->Text = "Tertiary:";
		Label_Tertiary->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(Label_Tertiary, 0, 3);

		_PictureBox_Tertiary_Color = gcnew PictureBox();
		_PictureBox_Tertiary_Color->Size = System::Drawing::Size(30, 25);
		_PictureBox_Tertiary_Color->BackColor = _Settings->Tertiary_Color;
		_PictureBox_Tertiary_Color->BorderStyle = BorderStyle::FixedSingle;
		Layout->Controls->Add(_PictureBox_Tertiary_Color, 1, 3);

		_Button_Tertiary_Color = gcnew Button();
		_Button_Tertiary_Color->Dock = DockStyle::Fill;
		_Button_Tertiary_Color->Text = "...";
		_Button_Tertiary_Color->Tag = "Tertiary";
		_Button_Tertiary_Color->Click += gcnew EventHandler(this, &Form_Auto_Generate::OnPickColorClick);
		Layout->Controls->Add(_Button_Tertiary_Color, 2, 3);

		_GroupBox_Colors->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentEventTypeSection()
	{
		_GroupBox_Event_Type = gcnew Control_GroupBox();
		_GroupBox_Event_Type->Text = "Event Type Settings";
		_GroupBox_Event_Type->Size = System::Drawing::Size(715, 250);
		_GroupBox_Event_Type->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 3;
		Layout->RowCount = 6;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 50));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 50));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 150));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));

		_Label_Event_Type_Mode = gcnew Label();
		_Label_Event_Type_Mode->Text = "Type Selection:";
		_Label_Event_Type_Mode->AutoSize = true;
		_Label_Event_Type_Mode->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Event_Type_Mode, 0, 0);

		_ComboBox_Event_Type_Mode = gcnew Control_ComboBox();
		_ComboBox_Event_Type_Mode->Dock = DockStyle::Fill;
		_ComboBox_Event_Type_Mode->Drop_Down_Style = ComboBoxStyle::DropDownList;
		_ComboBox_Event_Type_Mode->Selection_Changed += gcnew Control_ComboBox_Selection_Changed_Event_Handler(this, &Form_Auto_Generate::OnEventTypeModeSelection_Changed);
		Layout->Controls->Add(_ComboBox_Event_Type_Mode, 1, 0);

		_Label_Energy_Threshold_Strobe = gcnew Label();
		_Label_Energy_Threshold_Strobe->Text = "Strobe Threshold:";
		_Label_Energy_Threshold_Strobe->AutoSize = true;
		_Label_Energy_Threshold_Strobe->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Energy_Threshold_Strobe, 0, 1);

		_TrackBar_Energy_Strobe = gcnew Control_Trackbar_Range();
		_TrackBar_Energy_Strobe->Dock = DockStyle::Fill;
		_TrackBar_Energy_Strobe->Minimum = 0;
		_TrackBar_Energy_Strobe->Maximum = 100;
		_TrackBar_Energy_Strobe->Value = 80;
		_TrackBar_Energy_Strobe->Mode = TrackbarRangeMode::Specific;
		_TrackBar_Energy_Strobe->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_Auto_Generate::OnEnergy_StrobeValueChanged);
		Layout->Controls->Add(_TrackBar_Energy_Strobe, 1, 1);

		_Label_Energy_Strobe_Value = gcnew Label();
		_Label_Energy_Strobe_Value->Dock = DockStyle::Fill;
		_Label_Energy_Strobe_Value->Text = _TrackBar_Energy_Strobe->Value.ToString();
		_Label_Energy_Strobe_Value->TextAlign = ContentAlignment::MiddleCenter;
		Layout->Controls->Add(_Label_Energy_Strobe_Value, 2, 1);

		_Label_Strobe_Quantization = gcnew Label();
		_Label_Strobe_Quantization->Text = "Strobe Quant.:";
		_Label_Strobe_Quantization->AutoSize = true;
		_Label_Strobe_Quantization->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Strobe_Quantization, 0, 2);

		_DropDown_Strobe_Quantization = gcnew Control_DropDown();
		_DropDown_Strobe_Quantization->Dock = DockStyle::Fill;
		_DropDown_Strobe_Quantization->Set_Tile_Layout(55, 55, 7);
		_DropDown_Strobe_Quantization->Title_Text = "Strobe Quantization";
		_DropDown_Strobe_Quantization->Set_Title_Color(Color::DarkGray);
		_DropDown_Strobe_Quantization->Set_Open_Direction(false);
		_DropDown_Strobe_Quantization->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		Layout->Controls->Add(_DropDown_Strobe_Quantization, 1, 2);

		_Label_Energy_Threshold_Fade = gcnew Label();
		_Label_Energy_Threshold_Fade->Text = "Fade Threshold:";
		_Label_Energy_Threshold_Fade->AutoSize = true;
		_Label_Energy_Threshold_Fade->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Energy_Threshold_Fade, 0, 3);

		_TrackBar_Energy_Fade = gcnew Control_Trackbar_Range();
		_TrackBar_Energy_Fade->Dock = DockStyle::Fill;
		_TrackBar_Energy_Fade->Minimum = 0;
		_TrackBar_Energy_Fade->Maximum = 100;
		_TrackBar_Energy_Fade->Value = 50;
		_TrackBar_Energy_Fade->Mode = TrackbarRangeMode::Specific;
		_TrackBar_Energy_Fade->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_Auto_Generate::OnEnergy_FadeValueChanged);
		Layout->Controls->Add(_TrackBar_Energy_Fade, 1, 3);

		_Label_Energy_Fade_Value = gcnew Label();
		_Label_Energy_Fade_Value->Dock = DockStyle::Fill;
		_Label_Energy_Fade_Value->Text = _TrackBar_Energy_Fade->Value.ToString();
		_Label_Energy_Fade_Value->TextAlign = ContentAlignment::MiddleCenter;
		Layout->Controls->Add(_Label_Energy_Fade_Value, 2, 3);

		_Label_Fade_Quantization = gcnew Label();
		_Label_Fade_Quantization->Text = "Fade Quant.:";
		_Label_Fade_Quantization->AutoSize = true;
		_Label_Fade_Quantization->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Fade_Quantization, 0, 4);

		_DropDown_Fade_Quantization = gcnew Control_DropDown();
		_DropDown_Fade_Quantization->Dock = DockStyle::Fill;
		_DropDown_Fade_Quantization->Set_Tile_Layout(55, 55, 7);
		_DropDown_Fade_Quantization->Title_Text = "Fade Quantization";
		_DropDown_Fade_Quantization->Set_Title_Color(Color::DarkGray);
		_DropDown_Fade_Quantization->Set_Open_Direction(false);
		_DropDown_Fade_Quantization->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		Layout->Controls->Add(_DropDown_Fade_Quantization, 1, 4);

		_GroupBox_Event_Type->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentFadeSection()
	{
		_GroupBox_Fade_Settings = gcnew Control_GroupBox();
		_GroupBox_Fade_Settings->Text = "Fade Settings";
		_GroupBox_Fade_Settings->Size = System::Drawing::Size(715, 90);
		_GroupBox_Fade_Settings->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 4;
		Layout->RowCount = 1;
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 80));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 80));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50));

		_Label_Ease_In = gcnew Label();
		_Label_Ease_In->Text = "Ease In:";
		_Label_Ease_In->AutoSize = true;
		_Label_Ease_In->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Ease_In, 0, 0);

		_DropDown_Ease_In = gcnew Control_DropDown();
		_DropDown_Ease_In->Dock = DockStyle::Fill;
		_DropDown_Ease_In->Set_Tile_Layout(55, 55, 8);
		_DropDown_Ease_In->Title_Text = "Ease In";
		_DropDown_Ease_In->Set_Title_Color(Color::DarkGray);
		_DropDown_Ease_In->Set_Open_Direction(false);
		_DropDown_Ease_In->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		Layout->Controls->Add(_DropDown_Ease_In, 1, 0);

		_Label_Ease_Out = gcnew Label();
		_Label_Ease_Out->Text = "Ease Out:";
		_Label_Ease_Out->AutoSize = true;
		_Label_Ease_Out->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Ease_Out, 2, 0);

		_DropDown_Ease_Out = gcnew Control_DropDown();
		_DropDown_Ease_Out->Dock = DockStyle::Fill;
		_DropDown_Ease_Out->Set_Tile_Layout(55, 55, 8);
		_DropDown_Ease_Out->Title_Text = "Ease Out";
		_DropDown_Ease_Out->Set_Title_Color(Color::DarkGray);
		_DropDown_Ease_Out->Set_Open_Direction(false);
		_DropDown_Ease_Out->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Right);
		Layout->Controls->Add(_DropDown_Ease_Out, 3, 0);

		_GroupBox_Fade_Settings->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentDurationSection()
	{
		_GroupBox_Duration = gcnew Control_GroupBox();
		_GroupBox_Duration->Text = "Duration Settings";
		_GroupBox_Duration->Size = System::Drawing::Size(715, 140);
		_GroupBox_Duration->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 5;
		Layout->RowCount = 5;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 120));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 120));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 200));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));

		_CheckBox_Auto_Duration = gcnew CheckBox();
		_CheckBox_Auto_Duration->Text = "Auto-calculate duration";
		_CheckBox_Auto_Duration->Checked = true;
		_CheckBox_Auto_Duration->AutoSize = true;
		_CheckBox_Auto_Duration->CheckedChanged += gcnew EventHandler(this, &Form_Auto_Generate::OnAutoDurationChanged);
		Layout->Controls->Add(_CheckBox_Auto_Duration, 0, 0);
		Layout->SetColumnSpan(_CheckBox_Auto_Duration, 4);

		_Label_Fixed_Duration = gcnew Label();
		_Label_Fixed_Duration->Text = "Fixed (ticks):";
		_Label_Fixed_Duration->AutoSize = true;
		_Label_Fixed_Duration->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Fixed_Duration, 0, 1);

		_TextBox_Fixed_Duration = gcnew TextBox();
		_TextBox_Fixed_Duration->Text = "480";
		_TextBox_Fixed_Duration->Width = 80;
		_TextBox_Fixed_Duration->Enabled = false;
		Layout->Controls->Add(_TextBox_Fixed_Duration, 1, 1);

		_Label_Duration_Scale = gcnew Label();
		_Label_Duration_Scale->Text = "Scale Factor:";
		_Label_Duration_Scale->AutoSize = true;
		_Label_Duration_Scale->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Duration_Scale, 2, 1);

		_TrackBar_Duration_Scale = gcnew Control_Trackbar_Range();
		_TrackBar_Duration_Scale->Dock = DockStyle::Fill;
		_TrackBar_Duration_Scale->Minimum = 25;
		_TrackBar_Duration_Scale->Maximum = 200;
		_TrackBar_Duration_Scale->Value = 100;
		_TrackBar_Duration_Scale->Mode = TrackbarRangeMode::Specific;
		_TrackBar_Duration_Scale->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_Auto_Generate::OnDuration_ScaleValueChanged);
		Layout->Controls->Add(_TrackBar_Duration_Scale, 3, 1);

		_Label_Duration_Scale_Value = gcnew Label();
		_Label_Duration_Scale_Value->Dock = DockStyle::Fill;
		_Label_Duration_Scale_Value->Text = _TrackBar_Duration_Scale->Value.ToString();
		_Label_Duration_Scale_Value->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(_Label_Duration_Scale_Value, 4, 1);

		_Label_Min_Duration = gcnew Label();
		_Label_Min_Duration->Text = "Min (ticks):";
		_Label_Min_Duration->AutoSize = true;
		_Label_Min_Duration->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Min_Duration, 0, 2);

		_TextBox_Min_Duration = gcnew TextBox();
		_TextBox_Min_Duration->Text = "120";
		_TextBox_Min_Duration->Width = 80;
		Layout->Controls->Add(_TextBox_Min_Duration, 1, 2);

		_Label_Max_Duration = gcnew Label();
		_Label_Max_Duration->Text = "Max (ticks):";
		_Label_Max_Duration->AutoSize = true;
		_Label_Max_Duration->Anchor = AnchorStyles::Left;
		Layout->Controls->Add(_Label_Max_Duration, 2, 2);

		_TextBox_Max_Duration = gcnew TextBox();
		_TextBox_Max_Duration->Text = "3840";
		_TextBox_Max_Duration->Width = 80;
		Layout->Controls->Add(_TextBox_Max_Duration, 3, 2);

		_GroupBox_Duration->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentAudioSection()
	{
		_GroupBox_Audio = gcnew Control_GroupBox();
		_GroupBox_Audio->Text = "Audio Analysis Settings";
		_GroupBox_Audio->Size = System::Drawing::Size(715, 200);
		_GroupBox_Audio->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		bool Has_Audio = (_Waveform_Data != nullptr && _Audio_Duration_ms > 0);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 3;
		Layout->RowCount = 5;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));

		_CheckBox_Use_Audio_Energy = gcnew CheckBox();
		_CheckBox_Use_Audio_Energy->Text = "Use audio energy levels";
		_CheckBox_Use_Audio_Energy->Checked = Has_Audio;
		_CheckBox_Use_Audio_Energy->Enabled = Has_Audio;
		_CheckBox_Use_Audio_Energy->AutoSize = true;
		Layout->Controls->Add(_CheckBox_Use_Audio_Energy, 0, 0);
		Layout->SetColumnSpan(_CheckBox_Use_Audio_Energy, 2);

		_CheckBox_Use_Audio_Beats = gcnew CheckBox();
		_CheckBox_Use_Audio_Beats->Text = "Use detected beats";
		_CheckBox_Use_Audio_Beats->Checked = Has_Audio;
		_CheckBox_Use_Audio_Beats->Enabled = Has_Audio;
		_CheckBox_Use_Audio_Beats->AutoSize = true;
		Layout->Controls->Add(_CheckBox_Use_Audio_Beats, 0, 1);
		Layout->SetColumnSpan(_CheckBox_Use_Audio_Beats, 2);

		_Label_Beat_Sensitivity = gcnew Label();
		_Label_Beat_Sensitivity->Text = "Beat Sensitivity:";
		_Label_Beat_Sensitivity->AutoSize = true;
		_Label_Beat_Sensitivity->Anchor = AnchorStyles::Left;
		_Label_Beat_Sensitivity->Enabled = Has_Audio;
		Layout->Controls->Add(_Label_Beat_Sensitivity, 0, 2);

		_TrackBar_Beat_Sensitivity = gcnew Control_Trackbar_Range();
		_TrackBar_Beat_Sensitivity->Dock = DockStyle::Fill;
		_TrackBar_Beat_Sensitivity->Minimum = 0;
		_TrackBar_Beat_Sensitivity->Maximum = 100;
		_TrackBar_Beat_Sensitivity->Value = 50;
		_TrackBar_Beat_Sensitivity->Mode = TrackbarRangeMode::Specific;
		_TrackBar_Beat_Sensitivity->Enabled = Has_Audio;
		_TrackBar_Beat_Sensitivity->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_Auto_Generate::OnBeat_SensitivityValueChanged);
		Layout->Controls->Add(_TrackBar_Beat_Sensitivity, 1, 2);

		_Label_Beat_Sensitivity_Value = gcnew Label();
		_Label_Beat_Sensitivity_Value->Dock = DockStyle::Fill;
		_Label_Beat_Sensitivity_Value->Text = _TrackBar_Beat_Sensitivity->Value.ToString();
		_Label_Beat_Sensitivity_Value->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(_Label_Beat_Sensitivity_Value, 2, 2);

		_Label_Transient_Sensitivity = gcnew Label();
		_Label_Transient_Sensitivity->Text = "Transient Sensitivity:";
		_Label_Transient_Sensitivity->AutoSize = true;
		_Label_Transient_Sensitivity->Anchor = AnchorStyles::Left;
		_Label_Transient_Sensitivity->Enabled = Has_Audio;
		Layout->Controls->Add(_Label_Transient_Sensitivity, 0, 3);

		_TrackBar_Transient_Sensitivity = gcnew Control_Trackbar_Range();
		_TrackBar_Transient_Sensitivity->Dock = DockStyle::Fill;
		_TrackBar_Transient_Sensitivity->Minimum = 0;
		_TrackBar_Transient_Sensitivity->Maximum = 100;
		_TrackBar_Transient_Sensitivity->Value = 50;
		_TrackBar_Transient_Sensitivity->Mode = TrackbarRangeMode::Specific;
		_TrackBar_Transient_Sensitivity->Enabled = Has_Audio;
		_TrackBar_Transient_Sensitivity->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_Auto_Generate::OnTransient_SensitivityValueChanged);
		Layout->Controls->Add(_TrackBar_Transient_Sensitivity, 1, 3);

		_Label_Transient_Sensitivity_Value = gcnew Label();
		_Label_Transient_Sensitivity_Value->Dock = DockStyle::Fill;
		_Label_Transient_Sensitivity_Value->Text = _TrackBar_Transient_Sensitivity->Value.ToString();
		_Label_Transient_Sensitivity_Value->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(_Label_Transient_Sensitivity_Value, 2, 3);

		_GroupBox_Audio->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentOptionsSection()
	{
		_GroupBox_Options = gcnew Control_GroupBox();
		_GroupBox_Options->Text = "Options";
		_GroupBox_Options->Size = System::Drawing::Size(715, 110);
		_GroupBox_Options->Padding = System::Windows::Forms::Padding(10, 20, 10, 10);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 2;
		Layout->RowCount = 3;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 35));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 200));
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));

		_CheckBox_Clear_Existing = gcnew CheckBox();
		_CheckBox_Clear_Existing->Text = "Clear existing events in range";
		_CheckBox_Clear_Existing->Checked = true;
		Layout->Controls->Add(_CheckBox_Clear_Existing, 0, 0);
		Layout->SetColumnSpan(_CheckBox_Clear_Existing, 2);

		_CheckBox_Fill_Gaps = gcnew CheckBox();
		_CheckBox_Fill_Gaps->Dock = DockStyle::Fill;
		_CheckBox_Fill_Gaps->Text = "Fill gaps between events";
		_CheckBox_Fill_Gaps->Checked = false;
		_CheckBox_Fill_Gaps->Margin = System::Windows::Forms::Padding(3, 1, 0, 0);
		Layout->Controls->Add(_CheckBox_Fill_Gaps, 0, 1);

		_ComboBox_Gap_Mode = gcnew Control_ComboBox();
		_ComboBox_Gap_Mode->Dock = DockStyle::Fill;
		_ComboBox_Gap_Mode->Drop_Down_Style = ComboBoxStyle::DropDownList;
		_ComboBox_Gap_Mode->Enabled = false;
		Layout->Controls->Add(_ComboBox_Gap_Mode, 1, 1);

		_CheckBox_Fill_Gaps->CheckedChanged += gcnew EventHandler(this, &Form_Auto_Generate::OnAutoDurationChanged);

		_GroupBox_Options->Controls->Add(Layout);
	}

	void Form_Auto_Generate::InitializeComponentProgressAndButtons()
	{
		_Panel_Progress = gcnew Panel();
		_Panel_Progress->Dock = DockStyle::Fill;
		_Panel_Progress->Padding = System::Windows::Forms::Padding(0);

		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 1;
		Layout->RowCount = 3;
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 25));
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 40));

		_Label_Status = gcnew Label();
		_Label_Status->Text = "Ready";
		_Label_Status->Dock = DockStyle::Fill;
		_Label_Status->TextAlign = ContentAlignment::MiddleLeft;
		Layout->Controls->Add(_Label_Status, 0, 0);

		_ProgressBar = gcnew Control_ProgressBar();
		_ProgressBar->Dock = DockStyle::Fill;
		_ProgressBar->Value = 0;
		Layout->Controls->Add(_ProgressBar, 0, 1);

		_Panel_Buttons = gcnew Panel();
		_Panel_Buttons->Dock = DockStyle::Fill;

		FlowLayoutPanel^ Button_Flow = gcnew FlowLayoutPanel();
		Button_Flow->Dock = DockStyle::Fill;
		Button_Flow->FlowDirection = FlowDirection::RightToLeft;

		_Button_Cancel = gcnew Button();
		_Button_Cancel->Text = "Cancel";
		_Button_Cancel->Size = System::Drawing::Size(100, 30);
		_Button_Cancel->Click += gcnew EventHandler(this, &Form_Auto_Generate::OnCancelClick);
		Button_Flow->Controls->Add(_Button_Cancel);

		_Button_Generate = gcnew Button();
		_Button_Generate->Text = "Generate";
		_Button_Generate->Size = System::Drawing::Size(100, 30);
		_Button_Generate->Click += gcnew EventHandler(this, &Form_Auto_Generate::OnGenerateClick);
		Button_Flow->Controls->Add(_Button_Generate);

		_Button_Preview = gcnew Button();
		_Button_Preview->Text = "Preview";
		_Button_Preview->Size = System::Drawing::Size(100, 30);
		_Button_Preview->Click += gcnew EventHandler(this, &Form_Auto_Generate::OnPreviewClick);
		Button_Flow->Controls->Add(_Button_Preview);

		_Panel_Buttons->Controls->Add(Button_Flow);
		Layout->Controls->Add(_Panel_Buttons, 0, 2);

		_Panel_Progress->Controls->Add(Layout);
	}

	void Form_Auto_Generate::ApplyTheme()
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();
		Theme->ApplyTheme(this);

		// Apply theme to Buttons
		array<Button^>^ Buttons = {
			_Button_Primary_Color, _Button_Secondary_Color, _Button_Tertiary_Color,
			_Button_Preview, _Button_Generate, _Button_Cancel
		};

		for each(Button ^ B in Buttons) {
			Theme->ApplyThemeToButton(B, Theme->BackgroundAlt);
		}


		// Apply theme to Text Boxes
		array<TextBox^>^ TextBoxes = {
			_TextBox_Custom_Interval, _TextBox_Start_Measure, _TextBox_End_Measure,
			_TextBox_Fixed_Duration, _TextBox_Min_Duration, _TextBox_Max_Duration
		};

		for each(TextBox ^ T in TextBoxes) {
			T->BackColor = Theme->BackgroundLight;
			T->ForeColor = Theme->ForegroundText;
			T->BorderStyle = BorderStyle::FixedSingle;
		}

		// Apply theme to Labels
		array<Label^>^ Labels = {
			_Label_Mode, _Label_Distribution, _Label_Custom_Interval,
			_Label_Range, _Label_Range_To,
			_Label_Color_Mode,
			_Label_Event_Type_Mode, _Label_Energy_Threshold_Strobe, _Label_Energy_Strobe_Value, _Label_Energy_Threshold_Fade, _Label_Energy_Fade_Value, _Label_Strobe_Quantization, _Label_Fade_Quantization,
			_Label_Duration_Scale_Value
		};

		for each(Label ^ L in Labels) {
			L->ForeColor = Theme->ForegroundText;
			L->BackColor = Color::Transparent;
		}
	}

	void Form_Auto_Generate::PopulateDropDowns()
	{
		// Mode ComboBox
		array<String^>^ Entries_Mode = gcnew array<String^>	{ "Follow Tablature", "Follow Audio Energy", "Follow Audio Beats", "Combined (Tab + Audio)", "Pattern Based" };
		_ComboBox_Mode->Items->AddRange(Entries_Mode);
		_ComboBox_Mode->Selected_Index = 0;

		// Distribution ComboBox
		array<String^>^ Entries_Distribution = gcnew array<String^>	{ "Per Note", "Per Beat", "Per Measure", "Custom Interval" };
		_ComboBox_Distribution->Items->AddRange(Entries_Distribution);
		_ComboBox_Distribution->Selected_Index = 1;

		// Color Mode ComboBox
		array<String^>^ Entries_Color_Mode = gcnew array<String^>	{ "Single Color", "Gradient by Time", "Gradient by Energy", "Alternate Colors", "Random from Palette", "Map to Velocity" };
		_ComboBox_Color_Mode->Items->AddRange(Entries_Color_Mode);
		_ComboBox_Color_Mode->Selected_Index = 0;

		// Event Type Mode ComboBox
		array<String^>^ Entries_Type_Mode = gcnew array<String^>	{ "Solid Only", "Fade on Dynamics", "Strobe on Fast", "Mixed (Auto)" };
		_ComboBox_Event_Type_Mode->Items->AddRange(Entries_Type_Mode);
		_ComboBox_Event_Type_Mode->Selected_Index = 0;

		// Quantization dropdowns
		array<String^>^ Lines_1st_Quantization = TimeSignatures::TimeSignatureExtendedStringMain->ToArray();
		array<String^>^ Lines_2nd_Quantization = TimeSignatures::TimeSignatureExtendedStringSub->ToArray();
		array<int>^ Values_Quantization = TimeSignatures::TimeSignatureExtendedValues->ToArray();

		_DropDown_Strobe_Quantization->Set_Items(Lines_1st_Quantization, Lines_2nd_Quantization, Values_Quantization);
		_DropDown_Fade_Quantization->Set_Items(Lines_1st_Quantization, Lines_2nd_Quantization, Values_Quantization);

		_DropDown_Strobe_Quantization->Selected_Index = 2;	// 1/4
		_DropDown_Fade_Quantization->Selected_Index = 2;

		// Easing dropdowns
		array<String^>^ Lines_First_Easings = gcnew array<String^>	{ "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ", "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ", "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ" };
		array<String^>^ Lines_Second_Easings = gcnew array<String^>	{ "", "In", "In", "In", "In", "In", "In", "In", "", "Out", "Out", "Out", "Out", "Out", "Out", "Out", "", "InOut", "InOut", "InOut", "InOut", "InOut", "InOut", "InOut" };
		array<int>^ Values_Easings = gcnew array<int> {
			(int)FadeEasing::Linear,
			(int)FadeEasing::In_Sine,
			(int)FadeEasing::In_Quad,
			(int)FadeEasing::In_Cubic,
			(int)FadeEasing::In_Quart,
			(int)FadeEasing::In_Quint,
			(int)FadeEasing::In_Expo,
			(int)FadeEasing::In_Circ,
			(int)FadeEasing::Linear,
			(int)FadeEasing::Out_Sine,
			(int)FadeEasing::Out_Quad,
			(int)FadeEasing::Out_Cubic,
			(int)FadeEasing::Out_Quart,
			(int)FadeEasing::Out_Quint,
			(int)FadeEasing::Out_Expo,
			(int)FadeEasing::Out_Circ,
			(int)FadeEasing::Linear,
			(int)FadeEasing::InOut_Sine,
			(int)FadeEasing::InOut_Quad,
			(int)FadeEasing::InOut_Cubic,
			(int)FadeEasing::InOut_Quart,
			(int)FadeEasing::InOut_Quint,
			(int)FadeEasing::InOut_Expo,
			(int)FadeEasing::InOut_Circ
		};

		System::Resources::ResourceManager^ EasingImages = gcnew System::Resources::ResourceManager("MIDILightDrawer.Easing", System::Reflection::Assembly::GetExecutingAssembly());

		array<Image^>^ Images_Easings = gcnew array<Image^> {
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"00_LINEAR")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"01_IN_SINE")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"04_IN_QUAD")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"07_IN_CUBIC")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"10_IN_QUART")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"13_IN_QUINT")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"16_IN_EXPO")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"19_IN_CIRC")),

			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"00_LINEAR")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"02_OUT_SINE")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"05_OUT_QUAD")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"08_OUT_CUBIC")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"11_OUT_QUART")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"14_OUT_QUINT")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"17_OUT_EXPO")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"20_OUT_CIRC")),

			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"00_LINEAR")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"03_INOUT_SINE")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"06_INOUT_QUAD")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"09_INOUT_CUBIC")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"12_INOUT_QUART")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"15_INOUT_QUINT")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"18_INOUT_EXPO")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"21_INOUT_CIRC"))
		};
		_DropDown_Ease_In->Set_Items(Lines_First_Easings, Lines_Second_Easings, Values_Easings, Images_Easings);
		_DropDown_Ease_Out->Set_Items(Lines_First_Easings, Lines_Second_Easings, Values_Easings, Images_Easings);

		_DropDown_Ease_In->Selected_Index	= 0; // Linear
		_DropDown_Ease_Out->Selected_Index	= 0; // Linear

		// Gap Mode ComboBox
		array<String^>^ Entries_Gap_Mode = gcnew array<String^>	{ "Extend Previous", "Insert Fade", "Insert Dark" };
		_ComboBox_Gap_Mode->Items->AddRange(Entries_Gap_Mode);
		_ComboBox_Gap_Mode->Selected_Index = 0;
	}

	void Form_Auto_Generate::PopulateTrackList()
	{
		_CheckedList_Tracks->Items->Clear();

		if (_Timeline != nullptr && _Timeline->Tracks != nullptr)
		{
			for each (Track ^ T in _Timeline->Tracks)
			{
				CheckedListItem^ Item = gcnew CheckedListItem(T->Name, true);
				Item->Subtitle =  "Octave " + T->Octave.ToString();
				_CheckedList_Tracks->AddItem(Item);
			}

			if (_Timeline->Measures != nullptr && _Timeline->Measures->Count > 0)
			{
				_TextBox_End_Measure->Text = _Timeline->Measures->Count.ToString();
			}
		}
	}

	//////////////////////////////////
	// Event Handlers
	//////////////////////////////////
	void Form_Auto_Generate::OnModeSelection_Changed(System::Object^ sender,Control_ComboBox_Selection_Changed_Event_Args^ e)
	{
		UpdateUIState();
	}

	void Form_Auto_Generate::OnDistributionSelection_Changed(System::Object^ sender, MIDILightDrawer::Control_ComboBox_Selection_Changed_Event_Args^ e)
	{
		bool Is_Custom = (e->Selected_Index == 3);	// Custom Interval

		_TextBox_Custom_Interval->Enabled = Is_Custom;
	}

	void Form_Auto_Generate::OnColorModeSelection_Changed(System::Object^ sender, Control_ComboBox_Selection_Changed_Event_Args^ e)
	{
		UpdateUIState();
	}

	void MIDILightDrawer::Form_Auto_Generate::OnEventTypeModeSelection_Changed(System::Object^ sender, MIDILightDrawer::Control_ComboBox_Selection_Changed_Event_Args^ e)
	{
		UpdateUIState();
	}

	void Form_Auto_Generate::OnEnergy_StrobeValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
	{
		_Label_Energy_Strobe_Value->Text = e->Value.ToString();
	}

	void Form_Auto_Generate::OnEnergy_FadeValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
	{
		_Label_Energy_Fade_Value->Text = e->Value.ToString();
	}

	void Form_Auto_Generate::OnAllTracksChanged(Object^ sender, EventArgs^ e)
	{
		_CheckedList_Tracks->Enabled = !_CheckBox_All_Tracks->Checked;
	}

	void Form_Auto_Generate::OnFullRangeChanged(Object^ sender, EventArgs^ e)
	{
		if(_CheckBox_Full_Range->Checked)
		{
			_TrackBar_Measure->MinValue = _TrackBar_Measure->Minimum;
			_TrackBar_Measure->MaxValue = _TrackBar_Measure->Maximum;

			_TextBox_Start_Measure->Text = _TrackBar_Measure->MinValue.ToString();
			_TextBox_End_Measure->Text = _TrackBar_Measure->MaxValue.ToString();
		}
	}

	void Form_Auto_Generate::OnMeasureValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
	{
		_TextBox_Start_Measure->Text = e->MinValue.ToString();
		_TextBox_End_Measure->Text = e->MaxValue.ToString();

		_CheckBox_Full_Range->Checked = (e->MinValue == _TrackBar_Measure->Minimum && e->MaxValue == _TrackBar_Measure->Maximum);
	}

	void Form_Auto_Generate::OnAutoDurationChanged(Object^ sender, EventArgs^ e)
	{
		bool Auto = _CheckBox_Auto_Duration->Checked;
		_TextBox_Fixed_Duration->Enabled = !Auto;

		_ComboBox_Gap_Mode->Enabled = _CheckBox_Fill_Gaps->Checked;
	}

	void Form_Auto_Generate::OnDuration_ScaleValueChanged(System::Object^ sender,TrackbarRangeValueChangedEventArgs^ e)
	{
		_Label_Duration_Scale_Value->Text = e->Value.ToString();
	}

	void Form_Auto_Generate::OnBeat_SensitivityValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
	{
		_Label_Beat_Sensitivity_Value->Text = e->Value.ToString();
	}

	void Form_Auto_Generate::OnTransient_SensitivityValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
	{
		_Label_Transient_Sensitivity_Value->Text = e->Value.ToString();
	}

	void Form_Auto_Generate::OnPickColorClick(Object^ sender, EventArgs^ e)
	{
		Button^ Btn = safe_cast<Button^>(sender);
		String^ Tag = safe_cast<String^>(Btn->Tag);

		Color Current_Color;
		if (Tag == "Primary") {
			Current_Color = _Settings->Primary_Color;
		}
		else if (Tag == "Secondary") {
			Current_Color = _Settings->Secondary_Color;
		}
		else {
			Current_Color = _Settings->Tertiary_Color;
		}

		Dialog_ColorPicker^ Picker = gcnew Dialog_ColorPicker(Current_Color);

		if (Picker->ShowDialog(this) == System::Windows::Forms::DialogResult::OK)
		{
			Color New_Color = Picker->SelectedColor;

			if (Tag == "Primary")
			{
				_Settings->Primary_Color = New_Color;
				_PictureBox_Primary_Color->BackColor = New_Color;
			}
			else if (Tag == "Secondary")
			{
				_Settings->Secondary_Color = New_Color;
				_PictureBox_Secondary_Color->BackColor = New_Color;
			}
			else
			{
				_Settings->Tertiary_Color = New_Color;
				_PictureBox_Tertiary_Color->BackColor = New_Color;
			}
		}
	}

	void Form_Auto_Generate::OnPreviewClick(Object^ sender, EventArgs^ e)
	{
		// Generate preview without applying
		CollectSettingsFromUI();

		_Label_Status->Text = "Generating preview...";
		_ProgressBar->Value = 0;

		// Analyze data first
		AnalyzeTablature();

		if (_Settings->Use_Audio_Energy || _Settings->Use_Audio_Beats) {
			AnalyzeAudio();
		}

		_Preview_Events = GenerateEvents();

		_Label_Status->Text = String::Format("Preview: {0} events would be generated", _Preview_Events->Count);
		_ProgressBar->Value = 100;
	}

	void Form_Auto_Generate::OnGenerateClick(Object^ sender, EventArgs^ e)
	{
		if (_Is_Generating) return;

		CollectSettingsFromUI();

		_Is_Generating = true;
		_Button_Generate->Enabled = false;
		_Button_Preview->Enabled = false;
		_Label_Status->Text = "Generating events...";
		_ProgressBar->Value = 0;

		_Background_Worker->RunWorkerAsync();
	}

	void Form_Auto_Generate::OnCancelClick(Object^ sender, EventArgs^ e)
	{
		if (_Is_Generating)
		{
			_Background_Worker->CancelAsync();
		}
		else
		{
			this->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->Close();
		}
	}

	void Form_Auto_Generate::OnBackgroundWorkerDoWork(Object^ sender, DoWorkEventArgs^ e)
	{
		BackgroundWorker^ Worker = safe_cast<BackgroundWorker^>(sender);

		// Analyze tablature
		Worker->ReportProgress(10, "Analyzing tablature...");
		AnalyzeTablature();

		if (Worker->CancellationPending)
		{
			e->Cancel = true;
			return;
		}

		// Analyze audio if needed
		if (_Settings->Use_Audio_Energy || _Settings->Use_Audio_Beats)
		{
			Worker->ReportProgress(30, "Analyzing audio...");
			AnalyzeAudio();
		}

		if (Worker->CancellationPending)
		{
			e->Cancel = true;
			return;
		}

		// Generate events
		Worker->ReportProgress(50, "Generating events...");
		List<BarEvent^>^ Generated_Events = GenerateEvents();

		if (Worker->CancellationPending)
		{
			e->Cancel = true;
			return;
		}

		e->Result = Generated_Events;
		Worker->ReportProgress(100, "Complete!");
	}

	void Form_Auto_Generate::OnBackgroundWorkerProgressChanged(Object^ sender, ProgressChangedEventArgs^ e)
	{
		_ProgressBar->Value = e->ProgressPercentage;

		if (e->UserState != nullptr) {
			_Label_Status->Text = safe_cast<String^>(e->UserState);
		}
	}

	void Form_Auto_Generate::OnBackgroundWorkerCompleted(Object^ sender, RunWorkerCompletedEventArgs^ e)
	{
		_Is_Generating = false;
		_Button_Generate->Enabled = true;
		_Button_Preview->Enabled = true;

		if (e->Cancelled)
		{
			_Label_Status->Text = "Generation cancelled";
			_ProgressBar->Value = 0;
		}
		else if (e->Error != nullptr)
		{
			_Label_Status->Text = "Error: " + e->Error->Message;
			MessageBox::Show(this, "Error during generation:\n" + e->Error->Message, "Error", MessageBoxButtons::OK, MessageBoxIcon::Error);
		}
		else
		{
			List<BarEvent^>^ Generated_Events = safe_cast<List<BarEvent^>^>(e->Result);

			// Get range
			int Start_Tick = 0;
			int End_Tick = _Timeline->TotalTicks;

			if (!_Settings->Use_Full_Range)
			{
				Start_Tick = MeasureToTick(_Settings->Start_Measure - 1);
				End_Tick = MeasureToTick(_Settings->End_Measure);
			}

			// Clear existing events if requested
			if (_Settings->Clear_Existing_Events)
			{
				for each (Track ^ T in _Timeline->Tracks)
				{
					if (_Settings->Apply_To_All_Tracks || _Settings->Selected_Track_Indices->Contains(T->Index))
					{
						ClearEventsInRange(T, Start_Tick, End_Tick);
					}
				}
			}

			// Add generated events
			for each (BarEvent ^ Event in Generated_Events)
			{
				Event->ContainingTrack->AddBar(Event);
			}

			_Label_Status->Text = String::Format("Generated {0} events", Generated_Events->Count);
			_Timeline->Invalidate();

			this->DialogResult = System::Windows::Forms::DialogResult::OK;
		}
	}

	//////////////////////////////////
	// Settings Collection
	//////////////////////////////////
	void Form_Auto_Generate::CollectSettingsFromUI()
	{
		// General settings
		_Settings->Mode = (Generation_Mode)_ComboBox_Mode->Selected_Index;
		_Settings->Distribution = (Event_Distribution)_ComboBox_Distribution->Selected_Index;

		int Custom_Interval;
		if (Int32::TryParse(_TextBox_Custom_Interval->Text, Custom_Interval)) {
			_Settings->Custom_Interval_Ticks = Custom_Interval;
		}

		_Settings->Clear_Existing_Events = _CheckBox_Clear_Existing->Checked;

		// Track selection
		_Settings->Apply_To_All_Tracks = _CheckBox_All_Tracks->Checked;
		_Settings->Selected_Track_Indices->Clear();

		if (!_Settings->Apply_To_All_Tracks)
		{
			for (int i = 0; i < _CheckedList_Tracks->Items->Count; i++)
			{
				if (_CheckedList_Tracks->GetItemChecked(i)) {
					_Settings->Selected_Track_Indices->Add(i);
				}
			}
		}

		int Int_For_Parsing;
		// Range selection
		_Settings->Use_Full_Range = _CheckBox_Full_Range->Checked;

		Int32::TryParse(_TextBox_Start_Measure->Text, Int_For_Parsing);
		_Settings->Start_Measure = Int_For_Parsing;

		Int32::TryParse(_TextBox_End_Measure->Text, Int_For_Parsing);
		_Settings->End_Measure = Int_For_Parsing;

		// Color settings
		_Settings->Color_Selection_Mode = (Color_Mode)_ComboBox_Color_Mode->Selected_Index;
		// _Settings->Primary_Color		<-- Already defined
		// _Settings->Secondary_Color	<-- Already defined
		// _Settings->Tertiary_Color	<-- Already defined
		// _Settings->Color_Palette		<-- Already defined

		// Event type settings
		_Settings->Type_Selection_Mode = (Event_Type_Mode)_ComboBox_Event_Type_Mode->Selected_Index;
		_Settings->Energy_Threshold_Strobe = (float)_TrackBar_Energy_Strobe->Value / 100.0f;
		_Settings->Energy_Threshold_Fade = (float)_TrackBar_Energy_Fade->Value / 100.0f;
		_Settings->Strobe_Quantization_Ticks = _DropDown_Strobe_Quantization->Selected_Value;
		_Settings->Fade_Quantization_Ticks = _DropDown_Fade_Quantization->Selected_Value;

		// Fade settings
		_Settings->Default_Ease_In = static_cast<FadeEasing>(this->_DropDown_Ease_In->Selected_Value);
		_Settings->Default_Ease_Out = static_cast<FadeEasing>(this->_DropDown_Ease_Out->Selected_Value);
		
		// Audio analysis settings
		_Settings->Beat_Detection_Sensitivity = (float)_TrackBar_Beat_Sensitivity->Value / 100.0f;
		_Settings->Transient_Detection_Sensitivity = (float)_TrackBar_Transient_Sensitivity->Value / 100.0f;
		_Settings->Use_Audio_Energy = _CheckBox_Use_Audio_Energy->Checked;
		_Settings->Use_Audio_Beats = _CheckBox_Use_Audio_Beats->Checked;
		_Settings->Use_Spectral_Analysis = _CheckBox_Use_Audio_Energy->Checked || _CheckBox_Use_Audio_Beats->Checked;
		_Settings->FFT_Window_Size = 1024;	// Could add UI control for this
		_Settings->Onset_Detection_Threshold = (float)_TrackBar_Transient_Sensitivity->Value / 100.0f;
		_Settings->Prefer_Bass_Events = false;
		_Settings->Prefer_Percussion_Events = false;

		// Tablature settings


		// Event Duration Settings
		_Settings->Auto_Duration = _CheckBox_Auto_Duration->Checked;

		Int32::TryParse(_TextBox_Fixed_Duration->Text, Int_For_Parsing);
		_Settings->Fixed_Duration_Ticks = Int_For_Parsing;

		_Settings->Duration_Scale_Factor = (float)_TrackBar_Duration_Scale->Value / 100.0f;

		Int32::TryParse(_TextBox_Min_Duration->Text, Int_For_Parsing);
		_Settings->Minimum_Duration_Ticks = Int_For_Parsing;

		Int32::TryParse(_TextBox_Max_Duration->Text, Int_For_Parsing);
		_Settings->Maximum_Duration_Ticks = Int_For_Parsing;

		// Gap Handling
		_Settings->Fill_Gaps			= _CheckBox_Fill_Gaps->Checked;
		_Settings->Gap_Fill_Mode		= _ComboBox_Gap_Mode->Selected_Index;
		_Settings->Minimum_Gap_Ticks	= 960;	// 1/4
	}

	void Form_Auto_Generate::UpdateUIState()
	{
		// Update visibility based on mode
		bool Is_Audio_Mode = (_ComboBox_Mode->Selected_Index == 1 || _ComboBox_Mode->Selected_Index == 2);
		bool Is_Combined = (_ComboBox_Mode->Selected_Index == 3);

		_CheckBox_Use_Audio_Energy->Enabled = Is_Audio_Mode || Is_Combined;
		_CheckBox_Use_Audio_Beats->Enabled = Is_Audio_Mode || Is_Combined;
		_TrackBar_Beat_Sensitivity->Enabled = Is_Audio_Mode || Is_Combined;
		_TrackBar_Transient_Sensitivity->Enabled = Is_Audio_Mode || Is_Combined;

		// Update event type visibility
		bool Show_Thresholds = (_ComboBox_Event_Type_Mode->Selected_Index >= 1);

		_Label_Energy_Threshold_Strobe->Visible = Show_Thresholds;
		_TrackBar_Energy_Strobe->Visible		= Show_Thresholds;
		_Label_Energy_Strobe_Value->Visible		= Show_Thresholds;
		_Label_Strobe_Quantization->Visible		= Show_Thresholds;
		_DropDown_Strobe_Quantization->Visible	= Show_Thresholds;
		_Label_Energy_Threshold_Fade->Visible	= Show_Thresholds;
		_TrackBar_Energy_Fade->Visible			= Show_Thresholds;
		_Label_Energy_Fade_Value->Visible		= Show_Thresholds;
		_Label_Fade_Quantization->Visible		= Show_Thresholds;
		_DropDown_Fade_Quantization->Visible	= Show_Thresholds;

		_GroupBox_Fade_Settings->Visible		= Show_Thresholds;

		// Update color controls based on mode
		bool Show_Secondary = (_ComboBox_Color_Mode->Selected_Index >= 1 && _ComboBox_Color_Mode->Selected_Index <= 4);
		bool Show_Tertiary = (_ComboBox_Color_Mode->Selected_Index == 3);

		_PictureBox_Secondary_Color->Visible = Show_Secondary;
		_Button_Secondary_Color->Visible = Show_Secondary;
		_PictureBox_Tertiary_Color->Visible = Show_Tertiary;
		_Button_Tertiary_Color->Visible = Show_Tertiary;
	}

	int Form_Auto_Generate::MeasureToTick(int measure_index)
	{
		if (_Timeline == nullptr || _Timeline->Measures == nullptr) return 0;
		if (measure_index < 0) return 0;
		if (measure_index >= _Timeline->Measures->Count) return _Timeline->TotalTicks;

		return _Timeline->Measures[measure_index]->StartTick;
	}

	int Form_Auto_Generate::TickToMeasure(int tick)
	{
		if (_Timeline == nullptr || _Timeline->Measures == nullptr) return 0;

		for (int i = 0; i < _Timeline->Measures->Count; i++)
		{
			if (tick < _Timeline->Measures[i]->StartTick + _Timeline->Measures[i]->Length) {
				return i;
			}
		}

		return _Timeline->Measures->Count - 1;
	}

	void Form_Auto_Generate::ClearEventsInRange(Track^ track, int start_tick, int end_tick)
	{
		List<BarEvent^>^ Events_To_Remove = gcnew List<BarEvent^>();

		for each (BarEvent ^ Event in track->Events)
		{
			// Remove if event overlaps with range
			if (Event->StartTick < end_tick && Event->EndTick > start_tick) {
				Events_To_Remove->Add(Event);
			}
		}

		for each (BarEvent ^ Event in Events_To_Remove)
		{
			track->RemoveBar(Event);
		}
	}
}






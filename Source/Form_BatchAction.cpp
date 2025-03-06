#include "Form_BatchAction.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer {

    Form_BatchAction::Form_BatchAction(Widget_Timeline^ timeline)
    {
        _Timeline = timeline;

        // Initialize colors
        _SelectedColor = Color::White;
        _MinColor = Color::Black;
        _MaxColor = Color::White;
        _ActionColor = Color::Red;

        InitializeComponent();
        ApplyTheme();


        // Populate Track dropdown
        List<String^>^ TrackTitles = gcnew List<String^>();
        List<String^>^ TrackSubtitles = gcnew List<String^>();
        List<int>^ TrackValues = gcnew List<int>();
        
        for each (Track ^ track in _Timeline->Tracks) {
            TrackTitles->Add(track->Name);
            TrackSubtitles->Add("");
            TrackValues->Add(TrackValues->Count);
        }

        _DropDown_Track->Set_Items(TrackTitles->ToArray(), TrackSubtitles->ToArray(), TrackValues->ToArray());
        _DropDown_Track->Selected_Index = 0;


        // Populate Event Type dropdown
        array<String^>^ TypeTitles = gcnew array<String^> { "Solid", "Fade", "Strobe" };
        array<String^>^ TypeSubtitles = gcnew array<String^> { "", "", "" };  // Empty subtitles
        array<int>^ TypeValues = gcnew array<int> { 0, 1, 2,};

        _DropDown_EventType->Set_Items(TypeTitles, TypeSubtitles, TypeValues);
        _DropDown_EventType->Selected_Index = 0;

        // Set initial enabled states
        OnActionTypeChanged(nullptr, nullptr);

        // Set initial state of trackbars and controls
        UpdateMeasureControls();
        UpdateDurationControls();
        UpdateColorRControls();
        UpdateColorGControls();
        UpdateColorBControls();
        UpdateColorPreview();
    }

    Form_BatchAction::~Form_BatchAction()
    {
        // Clean up resources if needed
    }

    void Form_BatchAction::InitializeComponent()
    {
        // Form settings
        this->Text = "Batch Action";
        this->Size = System::Drawing::Size(800, 680);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
        this->MaximizeBox = false;
        this->MinimizeBox = false;
        this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
        this->ShowInTaskbar = false;
        this->Font = gcnew System::Drawing::Font("Segoe UI", 9.0f);

        // Action Group Box
        _GroupBox_Action = gcnew Control_GroupBox();
        _GroupBox_Action->Text = "Action";
        _GroupBox_Action->Location = System::Drawing::Point(12, 12);
        _GroupBox_Action->Size = System::Drawing::Size(760, 70);

        _Label_Action = gcnew System::Windows::Forms::Label();
        _Label_Action->Text = "Select Action:";
        _Label_Action->Location = System::Drawing::Point(20, 40);
        _Label_Action->AutoSize = true;

        _DropDown_Action = gcnew Control_DropDown();
        _DropDown_Action->Location = System::Drawing::Point(140, 36);
        _DropDown_Action->Size = System::Drawing::Size(250, 24);

        array<String^>^ ActionTitles    = gcnew array<String^>  { "Select Events", "Delete Events", "Change Color", "Move Events" };
        array<String^>^ ActionSubtitles = gcnew array<String^>  { "", "", "", "" };  // Empty subtitles
        array<int>^     ActionValues    = gcnew array<int>      { 0, 1, 2, 3 };

        _DropDown_Action->Set_Items(ActionTitles, ActionSubtitles, ActionValues);
        _DropDown_Action->Selected_Index = 0;
        _DropDown_Action->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnActionTypeChanged);
        _DropDown_Action->Set_Tile_Layout(250-3, 28, 1);

        _GroupBox_Action->Controls->Add(_Label_Action);
        _GroupBox_Action->Controls->Add(_DropDown_Action);

        // Filters Group Box
        _GroupBox_Filters = gcnew Control_GroupBox();
        _GroupBox_Filters->Text = "Filters";
        _GroupBox_Filters->Location = System::Drawing::Point(12, 90);
        _GroupBox_Filters->Width = 760;
        _GroupBox_Filters->AutoSize = true;
        _GroupBox_Filters->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
        _GroupBox_Filters->Padding = System::Windows::Forms::Padding(5);

        // Create TableLayoutPanel for filters
        TableLayoutPanel^ TableLayout_Filters = gcnew TableLayoutPanel();
        TableLayout_Filters->BackColor = Color::Transparent;
        TableLayout_Filters->ColumnCount = 1;
        TableLayout_Filters->RowCount = 5; // One row for each filter panel
        TableLayout_Filters->GrowStyle = TableLayoutPanelGrowStyle::AddRows;
        TableLayout_Filters->AutoSize = true;
        TableLayout_Filters->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
        TableLayout_Filters->Dock = DockStyle::Fill;
        TableLayout_Filters->Margin = System::Windows::Forms::Padding(0);

        // Set row styles to auto-size
        for (int i = 0; i < TableLayout_Filters->RowCount; i++) {
            TableLayout_Filters->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));
        }

        // Set column styles (single column fills width)
        TableLayout_Filters->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));

        // Set padding for spacing
        TableLayout_Filters->Padding = System::Windows::Forms::Padding(0, 15, 0, 5);
        TableLayout_Filters->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

        // Create expandable panels
        int PanelWidth = 740;

        // Track Filter Panel
        _TrackFilterPanel = gcnew Control_ExpandablePanel("Track Filter", Point(0, 0), PanelWidth);
        _TrackFilterPanel->ExpandToggled = gcnew EventHandler(this, &Form_BatchAction::OnPanelExpandToggled);
        _TrackFilterPanel->Dock = DockStyle::Fill;

        // Add label for Track
        _Label_Track = gcnew System::Windows::Forms::Label();
        _Label_Track->Text = "Select Track:";
        _Label_Track->Location = System::Drawing::Point(20 - 6, 20);
        _Label_Track->AutoSize = true;

        _DropDown_Track = gcnew Control_DropDown();
        _DropDown_Track->Location = System::Drawing::Point(140 - 6, 15);
        _DropDown_Track->Size = System::Drawing::Size(250, 24);
        _DropDown_Track->Set_Tile_Layout(250-3, 28, 1);

        // Set content panel's height and add controls
        _TrackFilterPanel->ContentPanel->Height = 50;
        _TrackFilterPanel->ContentPanel->Controls->Add(_Label_Track);
        _TrackFilterPanel->ContentPanel->Controls->Add(_DropDown_Track);

        // Event Type Filter Panel
        _TypeFilterPanel = gcnew Control_ExpandablePanel("Event Type Filter", Point(0, 0), PanelWidth);
        _TypeFilterPanel->ExpandToggled = gcnew EventHandler(this, &Form_BatchAction::OnPanelExpandToggled);
        _TypeFilterPanel->Dock = DockStyle::Fill;

        // Add label for Event Type
        _Label_EventType = gcnew System::Windows::Forms::Label();
        _Label_EventType->Text = "Select Type:";
        _Label_EventType->Location = System::Drawing::Point(20 - 6, 20);
        _Label_EventType->AutoSize = true;

        _DropDown_EventType = gcnew Control_DropDown();
        _DropDown_EventType->Location = System::Drawing::Point(140 - 6, 15);
        _DropDown_EventType->Size = System::Drawing::Size(250, 24);
        _DropDown_EventType->Set_Tile_Layout(250-3, 28, 1);

        _TypeFilterPanel->ContentPanel->Height = 50;
        _TypeFilterPanel->ContentPanel->Controls->Add(_Label_EventType);
        _TypeFilterPanel->ContentPanel->Controls->Add(_DropDown_EventType);

        // Duration Filter Panel
        _DurationFilterPanel = gcnew Control_ExpandablePanel("Duration Filter", Point(0, 0), PanelWidth);
        _DurationFilterPanel->ExpandToggled = gcnew EventHandler(this, &Form_BatchAction::OnPanelExpandToggled);
        _DurationFilterPanel->Dock = DockStyle::Fill;

        _Button_DurationMode = CreateModeButton(Point(20, 15));
        _Button_DurationMode->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnDurationModeClick);

        _TrackBar_Duration = gcnew Control_Trackbar_Range();
        _TrackBar_Duration->Location = System::Drawing::Point(100, 12);
        _TrackBar_Duration->Size = System::Drawing::Size(350, 30);
        _TrackBar_Duration->Minimum = 1;
        _TrackBar_Duration->Maximum = 3840; // 4 quarter notes (960*4)
        _TrackBar_Duration->Value = 960;    // 1 quarter note
        _TrackBar_Duration->MinValue = _TrackBar_Duration->Minimum;
        _TrackBar_Duration->MaxValue = _TrackBar_Duration->Maximum;
        _TrackBar_Duration->TrackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
        _TrackBar_Duration->RangeColor = Theme_Manager::Get_Instance()->AccentPrimary;
        _TrackBar_Duration->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_BatchAction::OnDurationValueChanged);

        _TextBox_DurationValue = CreateValueTextBox(Point(460, 15), true);
        _TextBox_DurationValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnDurationValueKeyDown);

        _TextBox_DurationMinValue = CreateValueTextBox(Point(460, 15), false);
        _TextBox_DurationMinValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnDurationMinValueKeyDown);

        _Label_DurationTo = CreateToLabel(Point(520, 18));

        _TextBox_DurationMaxValue = CreateValueTextBox(Point(540, 15), false);
        _TextBox_DurationMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnDurationMaxValueKeyDown);

        _DurationFilterPanel->ContentPanel->Height = 60;
        _DurationFilterPanel->ContentPanel->Controls->Add(_Button_DurationMode);
        _DurationFilterPanel->ContentPanel->Controls->Add(_TrackBar_Duration);
        _DurationFilterPanel->ContentPanel->Controls->Add(_TextBox_DurationValue);
        _DurationFilterPanel->ContentPanel->Controls->Add(_TextBox_DurationMinValue);
        _DurationFilterPanel->ContentPanel->Controls->Add(_Label_DurationTo);
        _DurationFilterPanel->ContentPanel->Controls->Add(_TextBox_DurationMaxValue);

        // Color Filter Panel
        _ColorFilterPanel = gcnew Control_ExpandablePanel("Color Filter", Point(0, 0), PanelWidth);
        _ColorFilterPanel->ExpandToggled = gcnew EventHandler(this, &Form_BatchAction::OnPanelExpandToggled);
        _ColorFilterPanel->Dock = DockStyle::Fill;

        // Panel for grid layout of RGB controls
        Panel^ ColorGridPanel = gcnew Panel();
        ColorGridPanel->Location = Point(0, 0);
        ColorGridPanel->Size = System::Drawing::Size(PanelWidth, 160);

        // Red row
        int RowY = 10;

        // R Mode button
        _Button_ColorRMode = CreateModeButton(Point(20, RowY));
        _Button_ColorRMode->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnColorRModeClick);
        ColorGridPanel->Controls->Add(_Button_ColorRMode);

        // R Trackbar
        _TrackBar_ColorR = gcnew Control_Trackbar_Range();
        _TrackBar_ColorR->Location = System::Drawing::Point(100, RowY - 3);
        _TrackBar_ColorR->Size = System::Drawing::Size(350, 30);
        _TrackBar_ColorR->Minimum = 0;
        _TrackBar_ColorR->Maximum = 255;
        _TrackBar_ColorR->Value = 255; // White default
        _TrackBar_ColorR->MinValue = _TrackBar_ColorR->Minimum;
        _TrackBar_ColorR->MaxValue = _TrackBar_ColorR->Maximum;
        _TrackBar_ColorR->TrackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
        _TrackBar_ColorR->RangeColor = Color::FromArgb(255, 255, 100, 100); // Reddish
        _TrackBar_ColorR->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_BatchAction::OnColorRValueChanged);
        ColorGridPanel->Controls->Add(_TrackBar_ColorR);

        // R Value textboxes
        _TextBox_ColorRValue = CreateValueTextBox(Point(460, RowY), true);
        _TextBox_ColorRValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorRValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorRValue);

        _TextBox_ColorRMinValue = CreateValueTextBox(Point(460, RowY), false);
        _TextBox_ColorRMinValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorRMinValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorRMinValue);

        _Label_ColorRTo = CreateToLabel(Point(520, RowY + 3));
        ColorGridPanel->Controls->Add(_Label_ColorRTo);

        _TextBox_ColorRMaxValue = CreateValueTextBox(Point(540, RowY), false);
        _TextBox_ColorRMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorRMaxValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorRMaxValue);

        // Green row
        RowY += 40;

        // G Mode button
        _Button_ColorGMode = CreateModeButton(Point(20, RowY));
        _Button_ColorGMode->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnColorGModeClick);
        ColorGridPanel->Controls->Add(_Button_ColorGMode);

        // G Trackbar
        _TrackBar_ColorG = gcnew Control_Trackbar_Range();
        _TrackBar_ColorG->Location = System::Drawing::Point(100, RowY - 3);
        _TrackBar_ColorG->Size = System::Drawing::Size(350, 30);
        _TrackBar_ColorG->Minimum = 0;
        _TrackBar_ColorG->Maximum = 255;
        _TrackBar_ColorG->Value = 255; // White default
        _TrackBar_ColorG->MinValue = _TrackBar_ColorG->Minimum;
        _TrackBar_ColorG->MaxValue = _TrackBar_ColorG->Maximum;
        _TrackBar_ColorG->TrackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
        _TrackBar_ColorG->RangeColor = Color::FromArgb(255, 100, 255, 100); // Greenish
        _TrackBar_ColorG->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_BatchAction::OnColorGValueChanged);
        ColorGridPanel->Controls->Add(_TrackBar_ColorG);

        // G Value textboxes
        _TextBox_ColorGValue = CreateValueTextBox(Point(460, RowY), true);
        _TextBox_ColorGValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorGValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorGValue);

        _TextBox_ColorGMinValue = CreateValueTextBox(Point(460, RowY), false);
        _TextBox_ColorGMinValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorGMinValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorGMinValue);

        _Label_ColorGTo = CreateToLabel(Point(520, RowY + 3));
        ColorGridPanel->Controls->Add(_Label_ColorGTo);

        _TextBox_ColorGMaxValue = CreateValueTextBox(Point(540, RowY), false);
        _TextBox_ColorGMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorGMaxValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorGMaxValue);

        // Blue row
        RowY += 40;

        // B Mode button
        _Button_ColorBMode = CreateModeButton(Point(20, RowY));
        _Button_ColorBMode->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnColorBModeClick);
        ColorGridPanel->Controls->Add(_Button_ColorBMode);

        // B Trackbar
        _TrackBar_ColorB = gcnew Control_Trackbar_Range();
        _TrackBar_ColorB->Location = System::Drawing::Point(100, RowY - 3);
        _TrackBar_ColorB->Size = System::Drawing::Size(350, 30);
        _TrackBar_ColorB->Minimum = 0;
        _TrackBar_ColorB->Maximum = 255;
        _TrackBar_ColorB->Value = 255; // White default
        _TrackBar_ColorB->MinValue = _TrackBar_ColorB->Minimum;
        _TrackBar_ColorB->MaxValue = _TrackBar_ColorB->Maximum;
        _TrackBar_ColorB->TrackColor = Theme_Manager::Get_Instance()->BackgroundAlt;
        _TrackBar_ColorB->RangeColor = Color::FromArgb(255, 100, 100, 255); // Bluish
        _TrackBar_ColorB->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_BatchAction::OnColorBValueChanged);
        ColorGridPanel->Controls->Add(_TrackBar_ColorB);

        // B Value textboxes
        _TextBox_ColorBValue = CreateValueTextBox(Point(460, RowY), true);
        _TextBox_ColorBValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorBValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorBValue);

        _TextBox_ColorBMinValue = CreateValueTextBox(Point(460, RowY), false);
        _TextBox_ColorBMinValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorBMinValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorBMinValue);

        _Label_ColorBTo = CreateToLabel(Point(520, RowY + 3));
        ColorGridPanel->Controls->Add(_Label_ColorBTo);

        _TextBox_ColorBMaxValue = CreateValueTextBox(Point(540, RowY), false);
        _TextBox_ColorBMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorBMaxValueKeyDown);
        ColorGridPanel->Controls->Add(_TextBox_ColorBMaxValue);

        // Color preview panel
        Panel^ PreviewLabelPanel = gcnew Panel();
        PreviewLabelPanel->Location = Point(670, 10);
        PreviewLabelPanel->Size = System::Drawing::Size(60, 20);

        Label^ PreviewLabel = gcnew Label();
        PreviewLabel->Text = "Preview";
        PreviewLabel->Location = Point(0, 0);
        PreviewLabel->Size = System::Drawing::Size(60, 20);
        PreviewLabel->TextAlign = ContentAlignment::MiddleCenter;
        PreviewLabelPanel->Controls->Add(PreviewLabel);

        _Panel_Color = gcnew System::Windows::Forms::Panel();
        _Panel_Color->Location = System::Drawing::Point(670, 35);
        _Panel_Color->Size = System::Drawing::Size(60, 60);
        _Panel_Color->BackColor = _SelectedColor;
        _Panel_Color->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;

        ColorGridPanel->Controls->Add(PreviewLabelPanel);
        ColorGridPanel->Controls->Add(_Panel_Color);

        _ColorFilterPanel->ContentPanel->Height = 140; // Reduced height
        _ColorFilterPanel->ContentPanel->Controls->Add(ColorGridPanel);

        // Measure Filter Panel
        _MeasureFilterPanel = gcnew Control_ExpandablePanel("Measure Filter", Point(0, 0), PanelWidth);
        _MeasureFilterPanel->ExpandToggled = gcnew EventHandler(this, &Form_BatchAction::OnPanelExpandToggled);
        _MeasureFilterPanel->Dock = DockStyle::Fill;

        _Button_MeasureMode = CreateModeButton(Point(20, 15));
        _Button_MeasureMode->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnMeasureModeClick);

        _TrackBar_Measure = gcnew Control_Trackbar_Range();
        _TrackBar_Measure->Mode = TrackbarRangeMode::Range;
        _TrackBar_Measure->Location = System::Drawing::Point(100, 12);
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
        _TrackBar_Measure->ValueChanged += gcnew TrackbarRangeValueChangedEventHandler(this, &Form_BatchAction::OnMeasureValueChanged);

        _TextBox_MeasureValue = CreateValueTextBox(Point(460, 15), true);
        _TextBox_MeasureValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnMeasureValueKeyDown);

        _TextBox_MeasureMinValue = CreateValueTextBox(Point(460, 15), false);
        _TextBox_MeasureMinValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnMeasureMinValueKeyDown);

        _Label_MeasureTo = CreateToLabel(Point(520, 18));

        _TextBox_MeasureMaxValue = CreateValueTextBox(Point(540, 15), false);
        _TextBox_MeasureMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnMeasureMaxValueKeyDown);

        _MeasureFilterPanel->ContentPanel->Height = 60;
        //_MeasureFilterPanel->ContentPanel->Controls->Add(_Button_MeasureMode);
        _MeasureFilterPanel->ContentPanel->Controls->Add(_TrackBar_Measure);
        _MeasureFilterPanel->ContentPanel->Controls->Add(_TextBox_MeasureValue);
        _MeasureFilterPanel->ContentPanel->Controls->Add(_TextBox_MeasureMinValue);
        _MeasureFilterPanel->ContentPanel->Controls->Add(_Label_MeasureTo);
        _MeasureFilterPanel->ContentPanel->Controls->Add(_TextBox_MeasureMaxValue);


        
        // Adjust the positions of the duration and color filter panels
        TableLayout_Filters->Controls->Add(_TrackFilterPanel, 0, 0);
        TableLayout_Filters->Controls->Add(_TypeFilterPanel, 0, 1);
        TableLayout_Filters->Controls->Add(_MeasureFilterPanel, 0, 2);
        TableLayout_Filters->Controls->Add(_DurationFilterPanel, 0, 3);
        TableLayout_Filters->Controls->Add(_ColorFilterPanel, 0, 4);

        // Add table layout to filters group box
        _GroupBox_Filters->Controls->Add(TableLayout_Filters);

        // Color Action Panel (for "Change Color" action)
        _Panel_ColorAction = gcnew System::Windows::Forms::Panel();
        _Panel_ColorAction->Location = System::Drawing::Point(12, 580);
        _Panel_ColorAction->Size = System::Drawing::Size(760, 60);
        _Panel_ColorAction->Visible = false;
        _Panel_ColorAction->BorderStyle = BorderStyle::FixedSingle;

        Label^ LabelNewColor = gcnew Label();
        LabelNewColor->Text = "New Color:";
        LabelNewColor->Location = System::Drawing::Point(20, 20);
        LabelNewColor->AutoSize = true;

        _Button_ActionColor = gcnew System::Windows::Forms::Button();
        _Button_ActionColor->Text = "Select Color";
        _Button_ActionColor->Location = System::Drawing::Point(140, 16);
        _Button_ActionColor->Size = System::Drawing::Size(100, 28);
        _Button_ActionColor->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnPickActionColorClick);

        _Panel_ActionColor = gcnew System::Windows::Forms::Panel();
        _Panel_ActionColor->Location = System::Drawing::Point(250, 16);
        _Panel_ActionColor->Size = System::Drawing::Size(40, 28);
        _Panel_ActionColor->BackColor = _ActionColor;
        _Panel_ActionColor->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;

        _Panel_ColorAction->Controls->Add(LabelNewColor);
        _Panel_ColorAction->Controls->Add(_Button_ActionColor);
        _Panel_ColorAction->Controls->Add(_Panel_ActionColor);

        // Move Action Panel (for "Move Events" action)
        _Panel_MoveAction = gcnew System::Windows::Forms::Panel();
        _Panel_MoveAction->Location = System::Drawing::Point(12, 580);
        _Panel_MoveAction->Size = System::Drawing::Size(760, 60);
        _Panel_MoveAction->Visible = false;
        _Panel_MoveAction->BorderStyle = BorderStyle::FixedSingle;

        Label^ LabelMoveAmount = gcnew Label();
        LabelMoveAmount->Text = "Move by (ticks):";
        LabelMoveAmount->Location = System::Drawing::Point(20, 20);
        LabelMoveAmount->AutoSize = true;

        _NumericUpDown_MoveAmount = gcnew System::Windows::Forms::NumericUpDown();
        _NumericUpDown_MoveAmount->Location = System::Drawing::Point(140, 18);
        _NumericUpDown_MoveAmount->Size = System::Drawing::Size(100, 23);
        _NumericUpDown_MoveAmount->Minimum = 1;
        _NumericUpDown_MoveAmount->Maximum = 10000;
        _NumericUpDown_MoveAmount->Value = 960; // Default to 1 quarter note (960 ticks)

        _Radio_MoveForward = gcnew System::Windows::Forms::RadioButton();
        _Radio_MoveForward->Text = "Forward";
        _Radio_MoveForward->Location = System::Drawing::Point(260, 18);
        _Radio_MoveForward->AutoSize = true;
        _Radio_MoveForward->Checked = true;

        _Radio_MoveBackward = gcnew System::Windows::Forms::RadioButton();
        _Radio_MoveBackward->Text = "Backward";
        _Radio_MoveBackward->Location = System::Drawing::Point(350, 18);
        _Radio_MoveBackward->AutoSize = true;

        _Panel_MoveAction->Controls->Add(LabelMoveAmount);
        _Panel_MoveAction->Controls->Add(_NumericUpDown_MoveAmount);
        _Panel_MoveAction->Controls->Add(_Radio_MoveForward);
        _Panel_MoveAction->Controls->Add(_Radio_MoveBackward);

        // Apply and Cancel buttons
        _Button_Apply = gcnew System::Windows::Forms::Button();
        _Button_Apply->Text = "Apply";
        _Button_Apply->Location = System::Drawing::Point(580, 580);
        _Button_Apply->Size = System::Drawing::Size(90, 30);
        _Button_Apply->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnApplyClick);

        _Button_Cancel = gcnew System::Windows::Forms::Button();
        _Button_Cancel->Text = "Cancel";
        _Button_Cancel->Location = System::Drawing::Point(680, 580);
        _Button_Cancel->Size = System::Drawing::Size(90, 30);
        _Button_Cancel->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnCancelClick);

        // Add all components to the form
        this->Controls->Add(_GroupBox_Action);
        this->Controls->Add(_GroupBox_Filters);
        this->Controls->Add(_Panel_ColorAction);
        this->Controls->Add(_Panel_MoveAction);
        this->Controls->Add(_Button_Apply);
        this->Controls->Add(_Button_Cancel);
    }

    void Form_BatchAction::ApplyTheme()
    {
        Theme_Manager^ ThemeManager = Theme_Manager::Get_Instance();

        // Apply theme to form
        this->BackColor = ThemeManager->Background;
        this->ForeColor = ThemeManager->ForegroundText;

        // Apply theme to expandable panels
        _TrackFilterPanel->ApplyTheme();
        _TypeFilterPanel->ApplyTheme();
        _DurationFilterPanel->ApplyTheme();
        _ColorFilterPanel->ApplyTheme();

        // Apply theme to the action dropdown
        _DropDown_Action->BackColor = ThemeManager->BackgroundLight;
        _DropDown_Action->ForeColor = ThemeManager->ForegroundText;
        //_ComboBox_Action->FlatStyle = FlatStyle::Flat;

        // Apply theme to radio buttons for move action
        array<RadioButton^>^ RadioButtons = {
            _Radio_MoveForward, _Radio_MoveBackward
        };

        for each (RadioButton ^ rb in RadioButtons) {
            rb->BackColor = Color::Transparent;
            rb->ForeColor = ThemeManager->ForegroundText;
        }

        // Apply theme to color preview panels
        array<Panel^>^ ColorPanels = {
            _Panel_Color, _Panel_ActionColor
        };

        for each (Panel ^ panel in ColorPanels) {
            panel->BorderStyle = BorderStyle::FixedSingle;
        }

        // Apply theme to buttons using the Theme_Manager method
        array<Button^>^ Buttons = {
            _Button_MeasureMode, _Button_DurationMode, _Button_ColorRMode, _Button_ColorGMode, _Button_ColorBMode, _Button_ActionColor, _Button_Apply, _Button_Cancel
        };

        for each (Button ^ btn in Buttons) {
            ThemeManager->ApplyThemeToButton(btn, ThemeManager->BackgroundAlt);
        }

        // Apply theme to textboxes
        array<TextBox^>^ TextBoxes = {
            _TextBox_DurationValue, _TextBox_DurationMinValue, _TextBox_DurationMaxValue,
            _TextBox_MeasureValue, _TextBox_MeasureMinValue, _TextBox_MeasureMaxValue,
            _TextBox_ColorRValue, _TextBox_ColorRMinValue, _TextBox_ColorRMaxValue,
            _TextBox_ColorGValue, _TextBox_ColorGMinValue, _TextBox_ColorGMaxValue,
            _TextBox_ColorBValue, _TextBox_ColorBMinValue, _TextBox_ColorBMaxValue
        };

        for each (TextBox ^ txt in TextBoxes) {
            txt->BackColor = ThemeManager->BackgroundLight;
            txt->ForeColor = ThemeManager->ForegroundText;
            txt->BorderStyle = BorderStyle::FixedSingle;
        }

        // Apply theme to labels for RGB controls
        array<Label^>^ Labels = {
            _Label_ColorRTo, _Label_ColorGTo, _Label_ColorBTo,
            _Label_DurationTo, _Label_MeasureTo, _Label_Track, _Label_EventType
        };

        for each (Label^ lbl in Labels) {
            lbl->ForeColor = ThemeManager->ForegroundText;
            lbl->BackColor = Color::Transparent;
        }

        // Action panels
        _Panel_ColorAction->BackColor = ThemeManager->Background;
        _Panel_MoveAction->BackColor = ThemeManager->Background;
    }

    Button^ Form_BatchAction::CreateModeButton(Point location)
    {
        Button^ New_Button = gcnew Button();
        New_Button->Text = "Specific";
        New_Button->Location = location;
        New_Button->Size = System::Drawing::Size(70, 23);

        return New_Button;
    }

    Label^ Form_BatchAction::CreateToLabel(Point location)
    {
        Label^ New_Label = gcnew Label();
        New_Label->Text = "to";
        New_Label->Location = location;
        New_Label->AutoSize = true;
        New_Label->Visible = false;

        return New_Label;
    }

    TextBox^ Form_BatchAction::CreateValueTextBox(Point location, bool visible)
    {
        TextBox^ New_TextBox = gcnew TextBox();

        New_TextBox = gcnew TextBox();
        New_TextBox->Location = location;
        New_TextBox->Size = System::Drawing::Size(50, 20);
        New_TextBox->TextAlign = HorizontalAlignment::Center;
        New_TextBox->Visible = visible;
        New_TextBox->Enabled = visible;

        return New_TextBox;
    }

    void Form_BatchAction::OnActionTypeChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
    {
        // Hide all action-specific panels first
        _Panel_ColorAction->Visible = false;
        _Panel_MoveAction->Visible = false;

        // Show the appropriate panel based on selected action
        if (e != nullptr && e->Value == 2) { // Change Color
            _Panel_ColorAction->Visible = true;
        }
        else if (e != nullptr && e->Value == 3) { // Move Events
            _Panel_MoveAction->Visible = true;
        }
    }

    void Form_BatchAction::OnPanelExpandToggled(System::Object^ sender, System::EventArgs^ e)
    {
        // Determine which panel was toggled
        Control_ExpandablePanel^ panel = safe_cast<Control_ExpandablePanel^>(sender);

        UpdatePanelPositions();
    }

    void Form_BatchAction::OnMeasureValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
        if (_TrackBar_Measure->Mode == TrackbarRangeMode::Specific)
        {
            _TextBox_MeasureValue->Text = e->Value.ToString();
        }
        else
        {
            _TextBox_MeasureMinValue->Text = e->MinValue.ToString();
            _TextBox_MeasureMaxValue->Text = e->MaxValue.ToString();
        }
    }

    void Form_BatchAction::OnMeasureModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode
        if (_TrackBar_Measure->Mode == TrackbarRangeMode::Specific)
        {
            _TrackBar_Measure->Mode = TrackbarRangeMode::Range;
            _Button_MeasureMode->Text = "Range";
        }
        else
        {
            _TrackBar_Measure->Mode = TrackbarRangeMode::Specific;
            _Button_MeasureMode->Text = "Specific";
        }

        UpdateMeasureControls();
    }

    void Form_BatchAction::OnMeasureValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            int value;
            if (Int32::TryParse(_TextBox_MeasureValue->Text, value))
            {
                _TrackBar_Measure->Value = value;
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnMeasureMinValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            int value;
            if (Int32::TryParse(_TextBox_MeasureMinValue->Text, value))
            {
                _TrackBar_Measure->MinValue = value;
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnMeasureMaxValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            int value;
            if (Int32::TryParse(_TextBox_MeasureMaxValue->Text, value))
            {
                _TrackBar_Measure->MaxValue = value;
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnDurationValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
        _TextBox_DurationValue->Text = e->Value.ToString();
        UpdateDurationControls();
    }

    void Form_BatchAction::OnDurationModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode
        if (_TrackBar_Duration->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_Duration->Mode = TrackbarRangeMode::Range;
            _Button_DurationMode->Text = "Range";
        }
        else {
            _TrackBar_Duration->Mode = TrackbarRangeMode::Specific;
            _Button_DurationMode->Text = "Specific";
        }

        UpdateDurationControls();
    }

    void Form_BatchAction::OnDurationValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_DurationValue->Text, value)) {
                _TrackBar_Duration->Value = value;
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnDurationMinValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_DurationMinValue->Text, value)) {
                _TrackBar_Duration->MinValue = value;
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnDurationMaxValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_DurationMaxValue->Text, value)) {
                _TrackBar_Duration->MaxValue = value;
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    // Color RGB value changed event handlers
    void Form_BatchAction::OnColorRValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
        if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
            _TextBox_ColorRValue->Text = e->Value.ToString();
        }
        else {
            _TextBox_ColorRMinValue->Text = e->MinValue.ToString();
            _TextBox_ColorRMaxValue->Text = e->MaxValue.ToString();
        }
        UpdateColorPreview();
    }

    void Form_BatchAction::OnColorGValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
        if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
            _TextBox_ColorGValue->Text = e->Value.ToString();
        }
        else {
            _TextBox_ColorGMinValue->Text = e->MinValue.ToString();
            _TextBox_ColorGMaxValue->Text = e->MaxValue.ToString();
        }
        UpdateColorPreview();
    }

    void Form_BatchAction::OnColorBValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
        if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
            _TextBox_ColorBValue->Text = e->Value.ToString();
        }
        else {
            _TextBox_ColorBMinValue->Text = e->MinValue.ToString();
            _TextBox_ColorBMaxValue->Text = e->MaxValue.ToString();
        }
        UpdateColorPreview();
    }

    void Form_BatchAction::OnColorRModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode for R component
        if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_ColorR->Mode = TrackbarRangeMode::Range;
            _Button_ColorRMode->Text = "Range";
        }
        else {
            _TrackBar_ColorR->Mode = TrackbarRangeMode::Specific;
            _Button_ColorRMode->Text = "Specific";
        }

        UpdateColorRControls();
        UpdateColorPreview();
    }

    void Form_BatchAction::OnColorGModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode for G component
        if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_ColorG->Mode = TrackbarRangeMode::Range;
            _Button_ColorGMode->Text = "Range";
        }
        else {
            _TrackBar_ColorG->Mode = TrackbarRangeMode::Specific;
            _Button_ColorGMode->Text = "Specific";
        }

        UpdateColorGControls();
        UpdateColorPreview();
    }

    void Form_BatchAction::OnColorBModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode for B component
        if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_ColorB->Mode = TrackbarRangeMode::Range;
            _Button_ColorBMode->Text = "Range";
        }
        else {
            _TrackBar_ColorB->Mode = TrackbarRangeMode::Specific;
            _Button_ColorBMode->Text = "Specific";
        }

        UpdateColorBControls();
        UpdateColorPreview();
    }

    void Form_BatchAction::OnColorRValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorRValue->Text, value)) {
                _TrackBar_ColorR->Value = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorRMinValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorRMinValue->Text, value)) {
                _TrackBar_ColorR->MinValue = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorRMaxValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorRMaxValue->Text, value)) {
                _TrackBar_ColorR->MaxValue = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorGValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorGValue->Text, value)) {
                _TrackBar_ColorG->Value = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorGMinValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorGMinValue->Text, value)) {
                _TrackBar_ColorG->MinValue = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorGMaxValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorGMaxValue->Text, value)) {
                _TrackBar_ColorG->MaxValue = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorBValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorBValue->Text, value)) {
                _TrackBar_ColorB->Value = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorBMinValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorBMinValue->Text, value)) {
                _TrackBar_ColorB->MinValue = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnColorBMaxValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorBMaxValue->Text, value)) {
                _TrackBar_ColorB->MaxValue = Math::Min(255, Math::Max(0, value));
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    // Helper methods for updating UI
    void Form_BatchAction::UpdateMeasureControls()
    {
        if (_TrackBar_Measure->Mode == TrackbarRangeMode::Specific)
        {
            // Single value mode
            _TextBox_MeasureMinValue->Visible = false;
            _TextBox_MeasureMaxValue->Visible = false;
            _TextBox_MeasureMinValue->Enabled = false;
            _TextBox_MeasureMaxValue->Enabled = false;
            _Label_MeasureTo->Visible = false;

            _TextBox_MeasureValue->Text = _TrackBar_Measure->Value.ToString();
            _TextBox_MeasureValue->Visible = true;
            _TextBox_MeasureValue->Enabled = true;
        }
        else
        {
            // Range mode
            _TextBox_MeasureValue->Visible = false;
            _TextBox_MeasureValue->Enabled = false;

            _TextBox_MeasureMinValue->Text = _TrackBar_Measure->MinValue.ToString();
            _TextBox_MeasureMaxValue->Text = _TrackBar_Measure->MaxValue.ToString();
            _TextBox_MeasureMinValue->Enabled = true;
            _TextBox_MeasureMaxValue->Enabled = true;
            _TextBox_MeasureMinValue->Visible = true;
            _TextBox_MeasureMaxValue->Visible = true;
            _Label_MeasureTo->Visible = true;
        }
    }

    void Form_BatchAction::UpdateDurationControls()
    {
        if (_TrackBar_Duration->Mode == TrackbarRangeMode::Specific) {
            // Single value mode
            _TextBox_DurationMinValue->Visible = false;
            _TextBox_DurationMaxValue->Visible = false;
            _TextBox_DurationMinValue->Enabled = false;
            _TextBox_DurationMaxValue->Enabled = false;
            _Label_DurationTo->Visible = false;
            
            _TextBox_DurationValue->Text = _TrackBar_Duration->Value.ToString();
            _TextBox_DurationValue->Visible = true;
        }
        else {
            // Range mode
            _TextBox_DurationValue->Visible = false;

            _TextBox_DurationMinValue->Text = _TrackBar_Duration->MinValue.ToString();
            _TextBox_DurationMaxValue->Text = _TrackBar_Duration->MaxValue.ToString();
            _TextBox_DurationMinValue->Enabled = true;
            _TextBox_DurationMaxValue->Enabled = true;
            _TextBox_DurationMinValue->Visible = true;
            _TextBox_DurationMaxValue->Visible = true;
            _TextBox_DurationMaxValue->Visible = true;
            _Label_DurationTo->Visible = true;
            
        }
    }

    void Form_BatchAction::UpdateColorRControls()
    {
        if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
            // Single value mode
            _TextBox_ColorRMinValue->Visible = false;
            _TextBox_ColorRMaxValue->Visible = false;
            _TextBox_ColorRMinValue->Enabled = false;
            _TextBox_ColorRMaxValue->Enabled = false;
            _Label_ColorRTo->Visible = false;

            _TextBox_ColorRValue->Text = _TrackBar_ColorR->Value.ToString();
            _TextBox_ColorRValue->Visible = true;
        }
        else {
            // Range mode
            _TextBox_ColorRValue->Visible = false;

            _TextBox_ColorRMinValue->Text = _TrackBar_ColorR->MinValue.ToString();
            _TextBox_ColorRMaxValue->Text = _TrackBar_ColorR->MaxValue.ToString();
            _TextBox_ColorRMinValue->Visible = true;
            _TextBox_ColorRMaxValue->Visible = true;
            _TextBox_ColorRMinValue->Enabled = true;
            _TextBox_ColorRMaxValue->Enabled = true;
            _Label_ColorRTo->Visible = true;
        }
    }

    void Form_BatchAction::UpdateColorGControls()
    {
        if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
            // Single value mode
            _TextBox_ColorGMinValue->Visible = false;
            _TextBox_ColorGMaxValue->Visible = false;
            _TextBox_ColorGMinValue->Enabled = false;
            _TextBox_ColorGMaxValue->Enabled = false;
            _Label_ColorGTo->Visible = false;

            _TextBox_ColorGValue->Text = _TrackBar_ColorG->Value.ToString();
            _TextBox_ColorGValue->Visible = true;
        }
        else {
            // Range mode
            _TextBox_ColorGValue->Visible = false;

            _TextBox_ColorGMinValue->Text = _TrackBar_ColorG->MinValue.ToString();
            _TextBox_ColorGMaxValue->Text = _TrackBar_ColorG->MaxValue.ToString();
            _TextBox_ColorGMinValue->Visible = true;
            _TextBox_ColorGMaxValue->Visible = true;
            _TextBox_ColorGMinValue->Enabled = true;
            _TextBox_ColorGMaxValue->Enabled = true;
            _Label_ColorGTo->Visible = true;
        }
    }

    void Form_BatchAction::UpdateColorBControls()
    {
        if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
            // Single value mode
            _TextBox_ColorBMinValue->Visible = false;
            _TextBox_ColorBMaxValue->Visible = false;
            _TextBox_ColorBMinValue->Enabled = false;
            _TextBox_ColorBMaxValue->Enabled = false;
            _Label_ColorBTo->Visible = false;

            _TextBox_ColorBValue->Text = _TrackBar_ColorB->Value.ToString();
            _TextBox_ColorBValue->Visible = true;
        }
        else {
            // Range mode
            _TextBox_ColorBValue->Visible = false;

            _TextBox_ColorBMinValue->Text = _TrackBar_ColorB->MinValue.ToString();
            _TextBox_ColorBMaxValue->Text = _TrackBar_ColorB->MaxValue.ToString();
            _TextBox_ColorBMinValue->Visible = true;
            _TextBox_ColorBMaxValue->Visible = true;
            _TextBox_ColorBMinValue->Enabled = true;
            _TextBox_ColorBMaxValue->Enabled = true;
            _Label_ColorBTo->Visible = true;
        }
    }

    void Form_BatchAction::UpdateColorPreview()
    {
        // Get all color components (either specific values or midpoints of ranges)
        int r, g, b;

        if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
            r = _TrackBar_ColorR->Value;
        }
        else {
            r = (_TrackBar_ColorR->MinValue + _TrackBar_ColorR->MaxValue) / 2; // Use midpoint for preview
        }

        if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
            g = _TrackBar_ColorG->Value;
        }
        else {
            g = (_TrackBar_ColorG->MinValue + _TrackBar_ColorG->MaxValue) / 2; // Use midpoint for preview
        }

        if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
            b = _TrackBar_ColorB->Value;
        }
        else {
            b = (_TrackBar_ColorB->MinValue + _TrackBar_ColorB->MaxValue) / 2; // Use midpoint for preview
        }

        _SelectedColor = Color::FromArgb(r, g, b);
        _Panel_Color->BackColor = _SelectedColor;
    }

    void Form_BatchAction::UpdatePanelPositions()
    {
        // Fixed positions for the panels
        int baseY = 40;
        int spacing = 10;

        // Calculate positions
        int trackPanelHeight = _TrackFilterPanel->IsExpanded ?
            (_TrackFilterPanel->Height) : (_TrackFilterPanel->Height - _TrackFilterPanel->ContentPanel->Height);

        int typeY = baseY + trackPanelHeight + spacing;
        _TypeFilterPanel->Location = Point(10, typeY);

        int typePanelHeight = _TypeFilterPanel->IsExpanded ?
            (_TypeFilterPanel->Height) : (_TypeFilterPanel->Height - _TypeFilterPanel->ContentPanel->Height);

        int durationY = typeY + typePanelHeight + spacing;
        _DurationFilterPanel->Location = Point(10, durationY);

        int durationPanelHeight = _DurationFilterPanel->IsExpanded ?
            (_DurationFilterPanel->Height) : (_DurationFilterPanel->Height - _DurationFilterPanel->ContentPanel->Height);

        int colorY = durationY + durationPanelHeight + spacing;
        _ColorFilterPanel->Location = Point(10, colorY);
    }

    void Form_BatchAction::OnPickActionColorClick(System::Object^ sender, System::EventArgs^ e)
    {
        ColorDialog^ colorDialog = gcnew ColorDialog();
        colorDialog->Color = _ActionColor;

        if (colorDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
            _ActionColor = colorDialog->Color;
            _Panel_ActionColor->BackColor = _ActionColor;
        }
    }

    void Form_BatchAction::OnApplyClick(System::Object^ sender, System::EventArgs^ e)
    {
        // Execute the batch action
        ExecuteBatchAction();
        this->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->Close();
    }

    void Form_BatchAction::OnCancelClick(System::Object^ sender, System::EventArgs^ e)
    {
        this->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->Close();
    }

    bool Form_BatchAction::MatchesFilter(BarEvent^ event)
    {
        // Filter by track
        if (_TrackFilterPanel->IsExpanded) {
            int trackIndex = _DropDown_Track->Selected_Value;
            if (trackIndex >= 0 && trackIndex < _Timeline->Tracks->Count) {
                if (event->ContainingTrack != _Timeline->Tracks[trackIndex]) {
                    return false;
                }
            }
        }

        // Filter by event type
        if (_TypeFilterPanel->IsExpanded) {
            int typeIndex = _DropDown_EventType->Selected_Value;
            BarEventType selectedType;

            switch (typeIndex) {
            case 0: selectedType = BarEventType::Solid; break;
            case 1: selectedType = BarEventType::Fade; break;
            case 2: selectedType = BarEventType::Strobe; break;
            default: selectedType = BarEventType::Solid; break;
            }

            if (event->Type != selectedType) {
                return false;
            }
        }

        // Filter by duration
        if (_DurationFilterPanel->IsExpanded) {
            if (_TrackBar_Duration->Mode == TrackbarRangeMode::Specific) {
                int exactDuration = _TrackBar_Duration->Value;
                if (event->Duration != exactDuration) {
                    return false;
                }
            }
            else {
                int minDuration = _TrackBar_Duration->MinValue;
                int maxDuration = _TrackBar_Duration->MaxValue;
                if (event->Duration < minDuration || event->Duration > maxDuration) {
                    return false;
                }
            }
        }

        // Filter by color (only applicable for solid bars and fade bars)
        if (_ColorFilterPanel->IsExpanded && (event->Type == BarEventType::Solid || event->Type == BarEventType::Fade)) {
            // Check R component
            if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
                int exactR = _TrackBar_ColorR->Value;
                const int tolerance = 5;
                if (Math::Abs(event->Color.R - exactR) > tolerance) {
                    return false;
                }
            }
            else {
                int minR = _TrackBar_ColorR->MinValue;
                int maxR = _TrackBar_ColorR->MaxValue;
                if (event->Color.R < minR || event->Color.R > maxR) {
                    return false;
                }
            }

            // Check G component
            if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
                int exactG = _TrackBar_ColorG->Value;
                const int tolerance = 5;
                if (Math::Abs(event->Color.G - exactG) > tolerance) {
                    return false;
                }
            }
            else {
                int minG = _TrackBar_ColorG->MinValue;
                int maxG = _TrackBar_ColorG->MaxValue;
                if (event->Color.G < minG || event->Color.G > maxG) {
                    return false;
                }
            }

            // Check B component
            if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
                int exactB = _TrackBar_ColorB->Value;
                const int tolerance = 5;
                if (Math::Abs(event->Color.B - exactB) > tolerance) {
                    return false;
                }
            }
            else {
                int minB = _TrackBar_ColorB->MinValue;
                int maxB = _TrackBar_ColorB->MaxValue;
                if (event->Color.B < minB || event->Color.B > maxB) {
                    return false;
                }
            }
        }

        // All filters passed
        return true;
    }

    void Form_BatchAction::ExecuteBatchAction()
    {
        // Determine which action to perform
        switch (_DropDown_Action->Selected_Value)
        {
            case 0: SelectEventsAction();   break;  // Select Events
            case 1: DeleteEventsAction();   break;  // Delete Events
            case 2: ChangeColorAction();    break;  // Change Color
            case 3: MoveEventsAction();     break;  // Move Events
        }
    }

    void Form_BatchAction::SelectEventsAction()
    {
        List<BarEvent^>^ matchingEvents = gcnew List<BarEvent^>();

        // Find all matching events
        for each (Track ^ track in _Timeline->Tracks) {
            for each (BarEvent ^ event in track->Events) {
                if (MatchesFilter(event)) {
                    matchingEvents->Add(event);
                }
            }
        }

        // Select the matching events in the timeline
        if (_Timeline->CurrentTool == TimelineToolType::Pointer) {
            PointerTool^ pointerTool = _Timeline->GetPointerTool();
            if (pointerTool != nullptr) {
                //pointerTool->ClearSelection();

                for each (BarEvent ^ event in matchingEvents) {
                    //pointerTool->SelectBar(event);
                }
            }
        }
    }

    void Form_BatchAction::DeleteEventsAction()
    {
        List<BarEvent^>^ eventsToDelete = gcnew List<BarEvent^>();

        // Find all matching events
        for each (Track ^ track in _Timeline->Tracks) {
            for each (BarEvent ^ event in track->Events) {
                if (MatchesFilter(event)) {
                    eventsToDelete->Add(event);
                }
            }
        }

        // Delete the matching events
        if (eventsToDelete->Count > 0) {
            CompoundCommand^ compoundCmd = gcnew CompoundCommand("Batch Delete Events");

            for each (BarEvent ^ event in eventsToDelete) {
                DeleteBarCommand^ cmd = gcnew DeleteBarCommand(_Timeline, event->ContainingTrack, event);
                compoundCmd->AddCommand(cmd);
            }

            _Timeline->CommandManager()->ExecuteCommand(compoundCmd);
        }
    }

    void Form_BatchAction::ChangeColorAction()
    {
        List<BarEvent^>^ eventsToChange = gcnew List<BarEvent^>();

        // Find all matching events
        for each (Track ^ track in _Timeline->Tracks) {
            for each (BarEvent ^ event in track->Events) {
                if (MatchesFilter(event)) {
                    eventsToChange->Add(event);
                }
            }
        }

        // Change color of the matching events
        if (eventsToChange->Count > 0) {
            CompoundCommand^ compoundCmd = gcnew CompoundCommand("Batch Change Color");

            for each (BarEvent ^ event in eventsToChange) {
                if (event->Type == BarEventType::Solid) {
                    // For solid bars, change the color directly
                    ChangeBarColorCommand^ cmd = gcnew ChangeBarColorCommand(_Timeline, event, event->Color, _ActionColor);
                    compoundCmd->AddCommand(cmd);
                }
                else if (event->Type == BarEventType::Fade && event->FadeInfo != nullptr) {
                    // For fade bars, change start and end colors
                    ChangeFadeBarColorCommand^ cmdStart = gcnew ChangeFadeBarColorCommand(
                        _Timeline, event, ChangeFadeBarColorCommand::ColorType::Start,
                        event->FadeInfo->ColorStart, _ActionColor);

                    ChangeFadeBarColorCommand^ cmdEnd = gcnew ChangeFadeBarColorCommand(
                        _Timeline, event, ChangeFadeBarColorCommand::ColorType::End,
                        event->FadeInfo->ColorEnd, _ActionColor);

                    compoundCmd->AddCommand(cmdStart);
                    compoundCmd->AddCommand(cmdEnd);
                }
                else if (event->Type == BarEventType::Strobe && event->StrobeInfo != nullptr) {
                    // For strobe bars, change the strobe color
                    //ChangeStrobeBarColorCommand^ cmd = gcnew ChangeStrobeBarColorCommand(_Timeline, event, event->StrobeInfo->ColorStrobe, _ActionColor);
                    //compoundCmd->AddCommand(cmd);
                }
            }

            _Timeline->CommandManager()->ExecuteCommand(compoundCmd);
        }
    }

    void Form_BatchAction::MoveEventsAction()
    {
        List<BarEvent^>^ eventsToMove = gcnew List<BarEvent^>();

        // Find all matching events
        for each (Track ^ track in _Timeline->Tracks) {
            for each (BarEvent ^ event in track->Events) {
                if (MatchesFilter(event)) {
                    eventsToMove->Add(event);
                }
            }
        }

        // Move the matching events
        if (eventsToMove->Count > 0) {
            int moveAmount = (int)_NumericUpDown_MoveAmount->Value;
            if (_Radio_MoveBackward->Checked) {
                moveAmount = -moveAmount;
            }

            CompoundCommand^ compoundCmd = gcnew CompoundCommand("Batch Move Events");

            for each (BarEvent ^ event in eventsToMove) {
                int newStartTick = event->StartTick + moveAmount;
                // Ensure we don't move before tick 0
                if (newStartTick < 0) {
                    newStartTick = 0;
                }

                //MoveBarCommand^ cmd = gcnew MoveBarCommand(_Timeline, event, event->StartTick, newStartTick);
                //compoundCmd->AddCommand(cmd);
            }

            _Timeline->CommandManager()->ExecuteCommand(compoundCmd);
        }
    }
}
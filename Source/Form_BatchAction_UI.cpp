#include "Form_BatchAction.h"

#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	void Form_BatchAction::InitializeComponent()
	{
		// Form settings
		this->Text = "Batch Action";
		this->Size = System::Drawing::Size(825, 690);
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
		this->ShowInTaskbar = false;
		this->Font = gcnew System::Drawing::Font("Segoe UI", 9.0f);

		_MainLayout = gcnew TableLayoutPanel();
		_MainLayout->Dock = DockStyle::Fill;
		_MainLayout->ColumnCount = 1;
		_MainLayout->RowCount = 2; // Scrollable Content and ButtonPanel
		_MainLayout->Padding = System::Windows::Forms::Padding(0, 0, 0, 0);
		_MainLayout->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		// Set row styles - first row takes all available space, second row is fixed height
		_MainLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f)); // Scrollable content
		_MainLayout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 50.0f)); // Fixed button row height

		// Add column style
		_MainLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));

		_Scrollable_Panel = gcnew Control_ScrollablePanel();
		_Scrollable_Panel->Dock = DockStyle::Fill;
		_Scrollable_Panel->Padding = System::Windows::Forms::Padding(12, 12, 12, 0);


		TableLayoutPanel^ ContentLayout = gcnew TableLayoutPanel();
		ContentLayout->Dock = DockStyle::Top;
		ContentLayout->Width = 785; // Fixed width to match the form
		ContentLayout->AutoSize = true;
		ContentLayout->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		ContentLayout->ColumnCount = 1;
		ContentLayout->RowCount = 3; // Action, Filters, Action-specific panel
		ContentLayout->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		// Configure row styles for content
		ContentLayout->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize)); // Action group
		ContentLayout->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize)); // Filters group
		ContentLayout->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize)); // Action-specific panel

		// Add column style for content
		ContentLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));

		InitializeComponentAction();
		InitializeComponentFilter();
		InitializeComponentActionMain();
		
		ContentLayout->Controls->Add(_GroupBox_Action, 0, 0);
		ContentLayout->Controls->Add(_GroupBox_Filters, 0, 1);
		ContentLayout->Controls->Add(_Panel_Action_Container, 0, 2);

		_Scrollable_Panel->ContentPanel->Controls->Add(ContentLayout);
		_Scrollable_Panel->ContentPanel->Padding = System::Windows::Forms::Padding(0, 0, 12, 0);
		
		InitializeComponentFormButtons();
		
		// Add all components to the TableLayoutPanel
		_MainLayout->Controls->Add(_Scrollable_Panel, 0, 0);
		_MainLayout->Controls->Add(_Panel_Form_Buttons, 0, 1);

		this->Controls->Add(_MainLayout);
	}

	void Form_BatchAction::InitializeComponentAction()
	{
		// Action Group Box
		_GroupBox_Action = gcnew Control_GroupBox();
		_GroupBox_Action->Text = "Action";
		_GroupBox_Action->Location = System::Drawing::Point(12, 12);
		_GroupBox_Action->Size = System::Drawing::Size(785, 70);

		_Label_Action = gcnew System::Windows::Forms::Label();
		_Label_Action->Text = "Select Action:";
		_Label_Action->Location = System::Drawing::Point(20, 40);
		_Label_Action->AutoSize = true;

		_DropDown_Action = gcnew Control_DropDown();
		_DropDown_Action->Location = System::Drawing::Point(140, 36);
		_DropDown_Action->Size = System::Drawing::Size(250, 24);

		array<String^>^ ActionTitles = gcnew array<String^>  { "Select Events", "Delete Events", "Change Color", "Replace Events" };
		array<String^>^ ActionSubtitles = gcnew array<String^>  { "", "", "", "" };  // Empty subtitles
		array<int>^ ActionValues = gcnew array<int>      { CI_ACTION_SELECTION, CI_ACTION_DELETE, CI_ACTION_COLOR, CI_ACTION_REPLACE };

		_DropDown_Action->Set_Items(ActionTitles, ActionSubtitles, ActionValues);
		_DropDown_Action->Selected_Index = 0;
		_DropDown_Action->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnActionTypeChanged);
		_DropDown_Action->Set_Tile_Layout(250 - 3, 28, 1);

		_GroupBox_Action->Controls->Add(_Label_Action);
		_GroupBox_Action->Controls->Add(_DropDown_Action);
	}

	void Form_BatchAction::InitializeComponentFilter()
	{
		// Filters Group Box
		_GroupBox_Filters = gcnew Control_GroupBox();
		_GroupBox_Filters->Text = "Filters";
		_GroupBox_Filters->Location = System::Drawing::Point(12, 90);
		_GroupBox_Filters->Width = 785;
		_GroupBox_Filters->AutoSize = true;
		_GroupBox_Filters->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		_GroupBox_Filters->Padding = System::Windows::Forms::Padding(5);

		// Create TableLayoutPanel for filters
		TableLayoutPanel^ TableLayout_Filters = gcnew TableLayoutPanel();
		TableLayout_Filters->BackColor = Color::Transparent;
		TableLayout_Filters->ColumnCount = 1;
		TableLayout_Filters->RowCount = 6; // One row for each filter panel
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
		const int PanelWidth = 765;

		InitializeComponentFilterSelection(PanelWidth);
		InitializeComponentFilterTrack(PanelWidth);
		InitializeComponentFilterType(PanelWidth);
		InitializeComponentFilterMeasure(PanelWidth);
		InitializeComponentFilterDuration(PanelWidth);
		InitializeComponentFilterColor(PanelWidth);


		// Adjust the positions of the duration and color filter panels
		TableLayout_Filters->Controls->Add(_SelectionFilterPanel, 0, 0);
		TableLayout_Filters->Controls->Add(_TrackFilterPanel, 0, 1);
		TableLayout_Filters->Controls->Add(_TypeFilterPanel, 0, 2);
		TableLayout_Filters->Controls->Add(_MeasureFilterPanel, 0, 3);
		TableLayout_Filters->Controls->Add(_DurationFilterPanel, 0, 4);
		TableLayout_Filters->Controls->Add(_ColorFilterPanel, 0, 5);

		// Add table layout to filters group box
		_GroupBox_Filters->Controls->Add(TableLayout_Filters);
	}

	void Form_BatchAction::InitializeComponentFilterSelection(int width)
	{
		// Create Selection Filter Panel
		_SelectionFilterPanel = gcnew Control_ExpandablePanel("Event Selection", Point(0, 0), width);
		_SelectionFilterPanel->IsEnabled = false;
		_SelectionFilterPanel->EnabledChanged += gcnew EventHandler(this, &Form_BatchAction::OnFilterToggleChanged);

		// Create Selection Filter controls
		_Label_SelectionFilter = gcnew Label();
		_Label_SelectionFilter->Text = "Filter Scope:";
		_Label_SelectionFilter->Location = System::Drawing::Point(20 - 6, 20);
		_Label_SelectionFilter->AutoSize = true;

		_DropDown_SelectionFilter = gcnew Control_DropDown();
		_DropDown_SelectionFilter->Location = System::Drawing::Point(140 - 6, 15);
		_DropDown_SelectionFilter->Size = System::Drawing::Size(250, 24);
		_DropDown_SelectionFilter->Set_Tile_Layout(250 - 3, 28, 1);
		_DropDown_SelectionFilter->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnFilterDropDownItemChanged);

		// Add controls to panel
		_SelectionFilterPanel->ContentPanel->Controls->Add(_Label_SelectionFilter);
		_SelectionFilterPanel->ContentPanel->Controls->Add(_DropDown_SelectionFilter);
		_SelectionFilterPanel->ContentPanel->Height = 50;

		// Populate Selection Filter dropdown
		array<String^>^ SelectionTitles = gcnew array<String^> { "All Events", "Current Selection" };
		array<String^>^ SelectionSubtitles = gcnew array<String^> { "", "" };
		array<int>^ SelectionValues = gcnew array<int> { 0, 1 };

		_DropDown_SelectionFilter->Set_Items(SelectionTitles, SelectionSubtitles, SelectionValues);
		_DropDown_SelectionFilter->Selected_Index = 0;
	}

	void Form_BatchAction::InitializeComponentFilterTrack(int width)
	{
		// Track Filter Panel
		_TrackFilterPanel = gcnew Control_ExpandablePanel("Track Filter", Point(0, 0), width);
		_TrackFilterPanel->Dock = DockStyle::Fill;
		_TrackFilterPanel->EnabledChanged += gcnew EventHandler(this, &Form_BatchAction::OnFilterToggleChanged);

		// Add label for Track
		_Label_Track = gcnew System::Windows::Forms::Label();
		_Label_Track->Text = "Select Track:";
		_Label_Track->Location = System::Drawing::Point(20 - 6, 20);
		_Label_Track->AutoSize = true;

		_DropDown_Track = gcnew Control_DropDown();
		_DropDown_Track->Location = System::Drawing::Point(140 - 6, 15);
		_DropDown_Track->Size = System::Drawing::Size(250, 24);
		_DropDown_Track->Set_Tile_Layout(250 - 3, 28, 1);
		_DropDown_Track->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnFilterDropDownItemChanged);

		// Set content panel's height and add controls
		_TrackFilterPanel->ContentPanel->Height = 50;
		_TrackFilterPanel->ContentPanel->Controls->Add(_Label_Track);
		_TrackFilterPanel->ContentPanel->Controls->Add(_DropDown_Track);
	}

	void Form_BatchAction::InitializeComponentFilterType(int width)
	{
		// Event Type Filter Panel
		_TypeFilterPanel = gcnew Control_ExpandablePanel("Event Type Filter", Point(0, 0), width);
		_TypeFilterPanel->Dock = DockStyle::Fill;
		_TypeFilterPanel->EnabledChanged += gcnew EventHandler(this, &Form_BatchAction::OnFilterToggleChanged);

		// Add label for Event Type
		_Label_EventType = gcnew System::Windows::Forms::Label();
		_Label_EventType->Text = "Select Type:";
		_Label_EventType->Location = System::Drawing::Point(20 - 6, 20);
		_Label_EventType->AutoSize = true;

		// Populate Event Type dropdown
		array<String^>^ TypeTitles = gcnew array<String^>{ "Solid", "Fade", "Strobe" };
		array<String^>^ TypeSubtitles = gcnew array<String^>{ "", "", "" };  // Empty subtitles
		array<int>^ TypeValues = gcnew array<int>{ CI_TYPE_SOLID, CI_TYPE_FADE, CI_TYPE_STROBE, };

		_DropDown_EventType = gcnew Control_DropDown();
		_DropDown_EventType->Location = System::Drawing::Point(140 - 6, 15);
		_DropDown_EventType->Size = System::Drawing::Size(250, 24);
		_DropDown_EventType->Set_Tile_Layout(250 - 3, 28, 1);
		_DropDown_EventType->Set_Items(TypeTitles, TypeSubtitles, TypeValues);
		_DropDown_EventType->Selected_Index = 0;
		_DropDown_EventType->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnFilterDropDownItemChanged);

		_TypeFilterPanel->ContentPanel->Height = 50;
		_TypeFilterPanel->ContentPanel->Controls->Add(_Label_EventType);
		_TypeFilterPanel->ContentPanel->Controls->Add(_DropDown_EventType);
	}

	void Form_BatchAction::InitializeComponentFilterMeasure(int width)
	{
		// Measure Filter Panel
		_MeasureFilterPanel = gcnew Control_ExpandablePanel("Measure Filter", Point(0, 0), width);
		_MeasureFilterPanel->Dock = DockStyle::Fill;
		_MeasureFilterPanel->EnabledChanged += gcnew EventHandler(this, &Form_BatchAction::OnFilterToggleChanged);

		_Label_MeasureRange = gcnew System::Windows::Forms::Label();
		_Label_MeasureRange->Text = "Range:";
		_Label_MeasureRange->Location = System::Drawing::Point(20 - 6, 18);
		_Label_MeasureRange->AutoSize = true;

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

		_TextBox_MeasureMinValue = CreateValueTextBox(Point(460, 15), true);
		_TextBox_MeasureMinValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnMeasureMinValueKeyDown);

		_Label_MeasureTo = CreateToLabel(18);
		_Label_MeasureTo->Visible = true;

		_TextBox_MeasureMaxValue = CreateValueTextBox(Point(540, 15), true);
		_TextBox_MeasureMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnMeasureMaxValueKeyDown);

		_MeasureFilterPanel->ContentPanel->Height = 60;
		_MeasureFilterPanel->ContentPanel->Controls->Add(_Label_MeasureRange);
		_MeasureFilterPanel->ContentPanel->Controls->Add(_TrackBar_Measure);
		_MeasureFilterPanel->ContentPanel->Controls->Add(_TextBox_MeasureMinValue);
		_MeasureFilterPanel->ContentPanel->Controls->Add(_Label_MeasureTo);
		_MeasureFilterPanel->ContentPanel->Controls->Add(_TextBox_MeasureMaxValue);
	}

	void Form_BatchAction::InitializeComponentFilterDuration(int width)
	{
		// Duration Filter Panel
		_DurationFilterPanel = gcnew Control_ExpandablePanel("Duration Filter", Point(0, 0), width);
		_DurationFilterPanel->Dock = DockStyle::Fill;
		_DurationFilterPanel->EnabledChanged += gcnew EventHandler(this, &Form_BatchAction::OnFilterToggleChanged);

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

		_Label_DurationTo = CreateToLabel(18);

		_TextBox_DurationMaxValue = CreateValueTextBox(Point(540, 15), false);
		_TextBox_DurationMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnDurationMaxValueKeyDown);

		_DurationFilterPanel->ContentPanel->Height = 60;
		_DurationFilterPanel->ContentPanel->Controls->Add(_Button_DurationMode);
		_DurationFilterPanel->ContentPanel->Controls->Add(_TrackBar_Duration);
		_DurationFilterPanel->ContentPanel->Controls->Add(_TextBox_DurationValue);
		_DurationFilterPanel->ContentPanel->Controls->Add(_TextBox_DurationMinValue);
		_DurationFilterPanel->ContentPanel->Controls->Add(_Label_DurationTo);
		_DurationFilterPanel->ContentPanel->Controls->Add(_TextBox_DurationMaxValue);
	}

	void Form_BatchAction::InitializeComponentFilterColor(int width)
	{
		// Color Filter Panel
		_ColorFilterPanel = gcnew Control_ExpandablePanel("Color Filter", Point(0, 0), width);
		_ColorFilterPanel->Dock = DockStyle::Fill;
		_ColorFilterPanel->EnabledChanged += gcnew EventHandler(this, &Form_BatchAction::OnFilterToggleChanged);

		// Panel for grid layout of RGB controls
		Panel^ ColorGridPanel = gcnew Panel();
		ColorGridPanel->Location = Point(0, 0);
		ColorGridPanel->Size = System::Drawing::Size(width, 160);

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

		_Label_ColorRTo = CreateToLabel(RowY + 3);
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

		_Label_ColorGTo = CreateToLabel(RowY + 3);
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

		_Label_ColorBTo = CreateToLabel(RowY + 3);
		ColorGridPanel->Controls->Add(_Label_ColorBTo);

		_TextBox_ColorBMaxValue = CreateValueTextBox(Point(540, RowY), false);
		_TextBox_ColorBMaxValue->KeyDown += gcnew KeyEventHandler(this, &Form_BatchAction::OnColorBMaxValueKeyDown);
		ColorGridPanel->Controls->Add(_TextBox_ColorBMaxValue);


		RowY += 40;

		_Label_FadeColorPoint = gcnew Label();
		_Label_FadeColorPoint->Text = "Filter for Color of Fade Event:";
		_Label_FadeColorPoint->Location = System::Drawing::Point(20, RowY + 3);
		_Label_FadeColorPoint->AutoSize = true;
		ColorGridPanel->Controls->Add(_Label_FadeColorPoint);

		_DropDown_FadeColorFilter = gcnew Control_DropDown();
		_DropDown_FadeColorFilter->Location = System::Drawing::Point(240, RowY);
		_DropDown_FadeColorFilter->Size = System::Drawing::Size(150, 24);
		_DropDown_FadeColorFilter->Set_Tile_Layout(150 - 3, 28, 1);
		_DropDown_FadeColorFilter->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnFilterDropDownItemChanged);
		ColorGridPanel->Controls->Add(_DropDown_FadeColorFilter);

		// Populate Fade Color Point dropdown
		array<String^>^ FadePointTitles = gcnew array<String^>{ "Any", "Start", "Center", "End" };
		array<String^>^ FadePointSubtitles = gcnew array<String^>{ "", "", "", "" };
		array<int>^ FadePointValues = gcnew array<int>{ CI_FADE_COLOR_ANY, CI_FADE_COLOR_START, CI_FADE_COLOR_CENTER, CI_FADE_COLOR_END };

		_DropDown_FadeColorFilter->Set_Items(FadePointTitles, FadePointSubtitles, FadePointValues);
		_DropDown_FadeColorFilter->Selected_Index = 0;


		// Color preview panel
		// Define color preview size and positions
		const int PreviewSize = CI_COLOR_PREVIEW_SIZE + 2;
		const int PreviewX = 670;
		const int PreviewY = 35;

		// Preview label panel
		_Label_Color = gcnew Label();
		_Label_Color->Text = "Color";
		_Label_Color->Location = Point(PreviewX + PreviewSize / 2 - 46 / 2, 10);
		_Label_Color->Size = System::Drawing::Size(46, 20);
		_Label_Color->TextAlign = ContentAlignment::MiddleCenter;

		// Single color preview (visible when all in specific mode)
		_PictureBox_Color = gcnew PictureBox();
		_PictureBox_Color->Location = System::Drawing::Point(PreviewX, PreviewY);
		_PictureBox_Color->Size = System::Drawing::Size(PreviewSize, PreviewSize);
		_PictureBox_Color->SizeMode = PictureBoxSizeMode::CenterImage;
		_PictureBox_Color->Image = Control_ColorPreset::CreateColorBitmap(_SelectedColor, CI_COLOR_PREVIEW_SIZE);

		// Min color label
		_Label_MinColor = gcnew Label();
		_Label_MinColor->Text = "Min";
		_Label_MinColor->Location = Point(PreviewX - PreviewSize - 10 + PreviewSize/2 - 20, 10);
		_Label_MinColor->Size = System::Drawing::Size(40, 20);
		_Label_MinColor->TextAlign = ContentAlignment::MiddleCenter;
		_Label_MinColor->Visible = false;

		// Min color preview (visible when any in range mode)
		_PictureBox_MinColor = gcnew PictureBox();
		_PictureBox_MinColor->Location = System::Drawing::Point(PreviewX - PreviewSize - 10, PreviewY);
		_PictureBox_MinColor->Size = System::Drawing::Size(PreviewSize, PreviewSize);
		_PictureBox_MinColor->SizeMode = PictureBoxSizeMode::CenterImage;
		_PictureBox_MinColor->Image = Control_ColorPreset::CreateColorBitmap(_MinColor, CI_COLOR_PREVIEW_SIZE);
		_PictureBox_MinColor->Visible = false;

		// Max color label
		_Label_MaxColor = gcnew Label();
		_Label_MaxColor->Text = "Max";
		_Label_MaxColor->Location = Point(PreviewX + PreviewSize / 2 - 20, 10);
		_Label_MaxColor->Size = System::Drawing::Size(40, 20);
		_Label_MaxColor->TextAlign = ContentAlignment::MiddleCenter;
		_Label_MaxColor->Visible = false;

		// Max color preview (same position as single preview)
		_PictureBox_MaxColor = gcnew PictureBox();
		_PictureBox_MaxColor->Location = System::Drawing::Point(PreviewX, PreviewY);
		_PictureBox_MaxColor->Size = System::Drawing::Size(PreviewSize, PreviewSize);
		_PictureBox_MaxColor->SizeMode = PictureBoxSizeMode::CenterImage;
		_PictureBox_MaxColor->Image = Control_ColorPreset::CreateColorBitmap(_MaxColor, CI_COLOR_PREVIEW_SIZE);
		_PictureBox_MaxColor->Visible = false;

		// Add all preview components to the color grid panel
		ColorGridPanel->Controls->Add(_Label_Color);
		ColorGridPanel->Controls->Add(_Label_MinColor);
		ColorGridPanel->Controls->Add(_Label_MaxColor);
		ColorGridPanel->Controls->Add(_PictureBox_Color);
		ColorGridPanel->Controls->Add(_PictureBox_MinColor);
		ColorGridPanel->Controls->Add(_PictureBox_MaxColor);

		_ColorFilterPanel->ContentPanel->Height = 160;
		_ColorFilterPanel->ContentPanel->Controls->Add(ColorGridPanel);
	}

	void Form_BatchAction::InitializeComponentActionMain()
	{
		// For the action-specific panels, create a container to hold both
		_Panel_Action_Container = gcnew Panel();
		_Panel_Action_Container->Height = 180;
		_Panel_Action_Container->Dock = DockStyle::Fill;

		InitializeComponentActionSelection();
		InitializeComponentActionColor();
		InitializeComponentActionReplace();

		_Panel_Action_Container->Controls->Add(_Panel_SelectAction);
		_Panel_Action_Container->Controls->Add(_Panel_ColorAction);
		_Panel_Action_Container->Controls->Add(_Panel_ReplaceAction);
	}

	void Form_BatchAction::InitializeComponentActionSelection()
	{
		_Panel_SelectAction = gcnew System::Windows::Forms::Panel();
		_Panel_SelectAction->Location = Point(0, 0);
		_Panel_SelectAction->Dock = DockStyle::Top;
		_Panel_SelectAction->BorderStyle = BorderStyle::FixedSingle;

		TableLayoutPanel^ TableLayoutColorSelection = gcnew TableLayoutPanel();
		TableLayoutColorSelection->Dock = DockStyle::Fill;
		TableLayoutColorSelection->ColumnCount = 5;
		TableLayoutColorSelection->RowCount = 2;
		TableLayoutColorSelection->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayoutColorSelection->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0f));
		TableLayoutColorSelection->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0f));

		TableLayoutColorSelection->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		TableLayoutColorSelection->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 130));
		TableLayoutColorSelection->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 130));
		TableLayoutColorSelection->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 130));
		TableLayoutColorSelection->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));

		Label^ Label_SelectionMode = gcnew System::Windows::Forms::Label();
		Label_SelectionMode->Dock = DockStyle::Fill;
		Label_SelectionMode->Text = "Selection Mode:";
		Label_SelectionMode->AutoSize = true;
		Label_SelectionMode->TextAlign = ContentAlignment::MiddleLeft;
		Label_SelectionMode->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayoutColorSelection->Controls->Add(Label_SelectionMode, 0, 0);
			

		// Initialize radio buttons
		_Radio_NewSelection = gcnew Control_RadioButton();
		_Radio_NewSelection->Dock = DockStyle::Fill;
		_Radio_NewSelection->OptionText = "New";
		_Radio_NewSelection->GroupId = 1;		// Use a common group ID
		_Radio_NewSelection->Selected = true;	// Default selection
		_Radio_NewSelection->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		_Radio_NewSelection->Margin = System::Windows::Forms::Padding(5, 10, 5, 10);
		TableLayoutColorSelection->Controls->Add(_Radio_NewSelection, 1, 0);

		_Radio_AddToSelection = gcnew Control_RadioButton();
		_Radio_AddToSelection->OptionText = "Add";
		_Radio_AddToSelection->GroupId = 1; // Same group ID as above
		_Radio_AddToSelection->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		_Radio_AddToSelection->Margin = System::Windows::Forms::Padding(5, 10, 5, 10);
		TableLayoutColorSelection->Controls->Add(_Radio_AddToSelection, 2, 0);

		_Radio_RemoveFromSelection = gcnew Control_RadioButton();
		_Radio_RemoveFromSelection->OptionText = "Remove";
		_Radio_RemoveFromSelection->GroupId = 1; // Same group ID as above
		_Radio_RemoveFromSelection->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		_Radio_RemoveFromSelection->Margin = System::Windows::Forms::Padding(5, 10, 5, 10);
		TableLayoutColorSelection->Controls->Add(_Radio_RemoveFromSelection, 3, 0);

		_Label_SelectionCount = gcnew System::Windows::Forms::Label();
		_Label_SelectionCount->Dock = DockStyle::Fill;
		_Label_SelectionCount->Text = CS_SELECTED_EVENTS + "0";
		_Label_SelectionCount->AutoSize = true;
		_Label_SelectionCount->TextAlign = ContentAlignment::MiddleLeft;
		_Label_SelectionCount->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayoutColorSelection->Controls->Add(_Label_SelectionCount, 0, 1);
		TableLayoutColorSelection->SetColumnSpan(_Label_SelectionCount, TableLayoutColorSelection->ColumnCount);

		// Add controls to panel
		_Panel_SelectAction->Controls->Add(TableLayoutColorSelection);
	}

	void Form_BatchAction::InitializeComponentActionColor()
	{
		// Color Action Panel (for "Change Color" action)
		_Panel_ColorAction = gcnew System::Windows::Forms::Panel();
		_Panel_ColorAction->Location = Point(0, 0);
		//_Panel_ColorAction->Height = 150;
		_Panel_ColorAction->Dock = DockStyle::Top;
		_Panel_ColorAction->Visible = false;
		_Panel_ColorAction->BorderStyle = BorderStyle::FixedSingle;


		TableLayoutPanel^ TableLayoutColorAction = gcnew TableLayoutPanel();
		TableLayoutColorAction->Dock = DockStyle::Fill;
		TableLayoutColorAction->ColumnCount = 4;
		TableLayoutColorAction->RowCount = 2;
		TableLayoutColorAction->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayoutColorAction->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0f));
		TableLayoutColorAction->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0f));

		TableLayoutColorAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		TableLayoutColorAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 50));
		TableLayoutColorAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 160));
		TableLayoutColorAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		// Color selection label
		Label^ Label_ActionColor = gcnew System::Windows::Forms::Label();
		Label_ActionColor->Dock = DockStyle::Fill;
		Label_ActionColor->Text = "New Color:";
		Label_ActionColor->AutoSize = true;
		Label_ActionColor->TextAlign = ContentAlignment::MiddleLeft;
		Label_ActionColor->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayoutColorAction->Controls->Add(Label_ActionColor, 0, 0);
		
		// Circular Color Preview
		_PictureBox_ActionColor = gcnew PictureBox();
		_PictureBox_ActionColor->SizeMode = PictureBoxSizeMode::CenterImage;
		_PictureBox_ActionColor->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Top | AnchorStyles::Bottom);
		_PictureBox_ActionColor->Margin = System::Windows::Forms::Padding(0, 3, 2, 3);
		TableLayoutColorAction->Controls->Add(_PictureBox_ActionColor, 1, 0);

		// Color Picker Button
		_Button_ActionColor = gcnew System::Windows::Forms::Button();
		_Button_ActionColor->Dock = DockStyle::Fill;    // Fill the cell
		_Button_ActionColor->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		_Button_ActionColor->Text = "Select Specific Color";
		_Button_ActionColor->Margin = System::Windows::Forms::Padding(3, 10, 3, 10);
		_Button_ActionColor->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnPickActionColorClick);
		TableLayoutColorAction->Controls->Add(_Button_ActionColor, 2, 0);

		// Color Presets Section
		Label^ Label_FadeTarget = gcnew Label();
		Label_FadeTarget->Dock = DockStyle::Fill;
		Label_FadeTarget->Text = "Fade Event Target Color:";
		Label_FadeTarget->AutoSize = true;
		Label_FadeTarget->TextAlign = ContentAlignment::MiddleLeft;
		Label_FadeTarget->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayoutColorAction->Controls->Add(Label_FadeTarget, 0, 1);
		TableLayoutColorAction->SetColumnSpan(Label_FadeTarget, 2);

		_DropDown_FadeColorTarget = gcnew Control_DropDown();
		_DropDown_FadeColorTarget->Dock = DockStyle::Fill;
		_DropDown_FadeColorTarget->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Top | AnchorStyles::Bottom);
		_DropDown_FadeColorTarget->Size = System::Drawing::Size(150, 24);
		_DropDown_FadeColorTarget->Margin = System::Windows::Forms::Padding(5, 12, 0, 12);
		_DropDown_FadeColorTarget->Set_Tile_Layout(150 - 3, 28, 1);
		_DropDown_FadeColorTarget->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnFilterDropDownItemChanged);
		TableLayoutColorAction->Controls->Add(_DropDown_FadeColorTarget, 2, 1);
		TableLayoutColorAction->SetColumnSpan(_DropDown_FadeColorTarget, 2);

		// Populate Fade Color Point dropdown
		array<String^>^ FadePointTitles = gcnew array<String^>{ "Start", "Center", "End" };
		array<String^>^ FadePointSubtitles = gcnew array<String^>{ "", "", "" };
		array<int>^ FadePointValues = gcnew array<int>{ CI_FADE_COLOR_START, CI_FADE_COLOR_CENTER, CI_FADE_COLOR_END };

		_DropDown_FadeColorTarget->Set_Items(FadePointTitles, FadePointSubtitles, FadePointValues);
		_DropDown_FadeColorTarget->Selected_Index = 0;


		_Panel_ColorAction->Controls->Add(TableLayoutColorAction);

		UpdateActionColorPreview();
	}

	void Form_BatchAction::InitializeComponentActionReplace()
	{
		// Move Action Panel (for "Move Events" action)
		_Panel_ReplaceAction = gcnew System::Windows::Forms::Panel();
		_Panel_ReplaceAction->Location = Point(0, 0);
		_Panel_ReplaceAction->Height = 180;
		_Panel_ReplaceAction->Dock = DockStyle::Top;
		_Panel_ReplaceAction->Visible = false;
		_Panel_ReplaceAction->BorderStyle = BorderStyle::FixedSingle;

		TableLayoutPanel^ TableLayoutReplaceAction = gcnew TableLayoutPanel();
		TableLayoutReplaceAction->Dock = DockStyle::Fill;
		TableLayoutReplaceAction->ColumnCount = 4;
		TableLayoutReplaceAction->RowCount = 2; 
		TableLayoutReplaceAction->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayoutReplaceAction->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 50));
		TableLayoutReplaceAction->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));

		TableLayoutReplaceAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 250));
		TableLayoutReplaceAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 250));
		TableLayoutReplaceAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 160));
		TableLayoutReplaceAction->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		Label^ Label_ReplaceType = gcnew Label();
		Label_ReplaceType->Dock = DockStyle::Fill;
		Label_ReplaceType->Text = "Replace with Events of Type:";
		Label_ReplaceType->AutoSize = true;
		Label_ReplaceType->TextAlign = ContentAlignment::MiddleLeft;
		Label_ReplaceType->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayoutReplaceAction->Controls->Add(Label_ReplaceType, 0, 0);

		
		// Populate Event Type dropdown
		array<String^>^ TypeTitles = gcnew array<String^>{ "Solid", "Fade", "Strobe" };
		array<String^>^ TypeSubtitles = gcnew array<String^>{ "", "", "" };  // Empty subtitles
		array<int>^ TypeValues = gcnew array<int>{ CI_TYPE_SOLID, CI_TYPE_FADE, CI_TYPE_STROBE, };

		_DropDown_ReplaceType = gcnew Control_DropDown();
		_DropDown_ReplaceType->Dock = DockStyle::Fill;
		_DropDown_ReplaceType->Set_Tile_Layout(250 - 8, 28, 1);
		_DropDown_ReplaceType->Set_Items(TypeTitles, TypeSubtitles, TypeValues);
		_DropDown_ReplaceType->Selected_Index = 0;
		_DropDown_ReplaceType->Margin = System::Windows::Forms::Padding(5, 12, 0, 12);
		_DropDown_ReplaceType->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnReplaceTypeChanged);
		TableLayoutReplaceAction->Controls->Add(_DropDown_ReplaceType, 1, 0);

		_Panel_Replace_Options = gcnew Panel();
		_Panel_Replace_Options->Dock = DockStyle::Fill;
		_Panel_Replace_Options->Margin = System::Windows::Forms::Padding(0, 10, 0, 0);
		TableLayoutReplaceAction->Controls->Add(_Panel_Replace_Options, 0, 1);
		TableLayoutReplaceAction->SetColumnSpan(_Panel_Replace_Options, 4);

		InitializeComponentActionReplaceSolidPanel();
		InitializeComponentActionReplaceFadePanel();
		InitializeComponentActionReplaceStrobePanel();

		_Panel_Replace_Options->Controls->Add(_Panel_Replace_Solid);
		_Panel_Replace_Options->Controls->Add(_Panel_Replace_Fade);
		_Panel_Replace_Options->Controls->Add(_Panel_Replace_Strobe);

		_Panel_ReplaceAction->Controls->Add(TableLayoutReplaceAction);

		UpdateReplaceOptions();
	}

	void Form_BatchAction::InitializeComponentActionReplaceSolidPanel()
	{
		_Panel_Replace_Solid = gcnew Panel();
		_Panel_Replace_Solid->Dock = DockStyle::Fill;
		_Panel_Replace_Solid->Visible = false;

		TableLayoutPanel^ TableLayout = gcnew TableLayoutPanel();
		TableLayout->Dock = DockStyle::Fill;
		TableLayout->ColumnCount = 4;
		TableLayout->RowCount = 3;
		TableLayout->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));

		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 50));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 160));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		Label^ Label_ActionColor = gcnew System::Windows::Forms::Label();
		Label_ActionColor->Dock = DockStyle::Fill;
		Label_ActionColor->Text = "New Color:";
		Label_ActionColor->AutoSize = true;
		Label_ActionColor->TextAlign = ContentAlignment::MiddleLeft;
		Label_ActionColor->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_ActionColor, 0, 0);

		_PictureBox_ReplaceColorSolid = gcnew PictureBox();
		_PictureBox_ReplaceColorSolid->SizeMode = PictureBoxSizeMode::CenterImage;
		_PictureBox_ReplaceColorSolid->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Top | AnchorStyles::Bottom);
		_PictureBox_ReplaceColorSolid->Margin = System::Windows::Forms::Padding(0, 0, 2, 0);
		TableLayout->Controls->Add(_PictureBox_ReplaceColorSolid, 1, 0);

		_Button_ReplaceColorSolid = gcnew System::Windows::Forms::Button();
		_Button_ReplaceColorSolid->Dock = DockStyle::Fill;
		_Button_ReplaceColorSolid->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		_Button_ReplaceColorSolid->Text = "Select Specific Color";
		_Button_ReplaceColorSolid->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		_Button_ReplaceColorSolid->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnReplaceSolidColorPicked);
		TableLayout->Controls->Add(_Button_ReplaceColorSolid, 2, 0);

		_Panel_Replace_Solid->Controls->Add(TableLayout);
	}

	void Form_BatchAction::InitializeComponentActionReplaceFadePanel()
	{
		_Panel_Replace_Fade = gcnew Panel();
		_Panel_Replace_Fade->Dock = DockStyle::Fill;
		_Panel_Replace_Fade->Visible = false;

		TableLayoutPanel^ TableLayout = gcnew TableLayoutPanel();
		TableLayout->Dock = DockStyle::Fill;
		TableLayout->ColumnCount = 5;
		TableLayout->RowCount = 3;
		TableLayout->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));

		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 160));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute,  85));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		// Fade Type (2 or 3 colors)
		Label^ Label_FadeMode = gcnew Label();
		Label_FadeMode->Dock = DockStyle::Fill;
		Label_FadeMode->Text = "Fade Mode:";
		Label_FadeMode->AutoSize = true;
		Label_FadeMode->TextAlign = ContentAlignment::MiddleLeft;
		Label_FadeMode->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_FadeMode, 0, 0);


		Button^ Button_Replace_Fade_ToggleMode = gcnew Button();
		Button_Replace_Fade_ToggleMode->Dock = DockStyle::Fill;
		Button_Replace_Fade_ToggleMode->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		Button_Replace_Fade_ToggleMode->Text = "Toggle";
		Button_Replace_Fade_ToggleMode->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		Button_Replace_Fade_ToggleMode->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnReplaceToggleModeClick);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(Button_Replace_Fade_ToggleMode);
		TableLayout->Controls->Add(Button_Replace_Fade_ToggleMode, 1, 0);

		// Fade Quantization
		Label^ Label_FadeQuantization = gcnew Label();
		Label_FadeQuantization->Dock = DockStyle::Fill;
		Label_FadeQuantization->Text = "Quantization:";
		Label_FadeQuantization->AutoSize = true;
		Label_FadeQuantization->TextAlign = ContentAlignment::MiddleLeft;
		Label_FadeQuantization->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_FadeQuantization, 0, 1);
		

		array<String^>^ Lines_First_Quantization = TimeSignatures::TimeSignatureExtendedrStringMain->ToArray();
		array<String^>^ Lines_Second_Quantization = TimeSignatures::TimeSignatureExtendedStringSub->ToArray();
		array<int>^ Values_Quantization = TimeSignatures::TimeSignatureExtendedValues->ToArray();

		_DropDown_Replace_FadeQuantization = gcnew Control_DropDown();
		_DropDown_Replace_FadeQuantization->Dock = DockStyle::Fill;
		_DropDown_Replace_FadeQuantization->Set_Open_Direction(true);
		_DropDown_Replace_FadeQuantization->Set_Tile_Layout(55, 55, 7);
		_DropDown_Replace_FadeQuantization->Set_Items(Lines_First_Quantization, Lines_Second_Quantization, Values_Quantization);
		_DropDown_Replace_FadeQuantization->Selected_Index = 0;
		_DropDown_Replace_FadeQuantization->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		_DropDown_Replace_FadeQuantization->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnReplaceFadeQuantizationChanged);
		TableLayout->Controls->Add(_DropDown_Replace_FadeQuantization, 1, 1);

		// Easings (In and Out)
		Label^ Label_EaseIn = gcnew Label();
		Label_EaseIn->Dock = DockStyle::Fill;
		Label_EaseIn->Text = "Ease In:";
		Label_EaseIn->AutoSize = true;
		Label_EaseIn->TextAlign = ContentAlignment::MiddleLeft;
		Label_EaseIn->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
		TableLayout->Controls->Add(Label_EaseIn, 2, 0);

		Label^ Label_EaseOut = gcnew Label();
		Label_EaseOut->Dock = DockStyle::Fill;
		Label_EaseOut->Text = "Ease Out:";
		Label_EaseOut->AutoSize = true;
		Label_EaseOut->TextAlign = ContentAlignment::MiddleLeft;
		Label_EaseOut->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
		TableLayout->Controls->Add(Label_EaseOut, 2, 1);

		array<String^>^ Lines_First_Easings = gcnew array<String^>{ "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ", "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ", "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ" };
		array<String^>^ Lines_Second_Easings = gcnew array<String^>{ "", "In", "In", "In", "In", "In", "In", "In", "", "Out", "Out", "Out", "Out", "Out", "Out", "Out", "", "InOut", "InOut", "InOut", "InOut", "InOut", "InOut", "InOut" };
		array<int>^ Values_Easings = gcnew array<int>{
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

		array<Image^>^ Images_Easings = gcnew array<Image^>{
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

		
		_DropDown_Replace_EaseIn = gcnew Control_DropDown();
		_DropDown_Replace_EaseIn->Dock = DockStyle::Fill;
		_DropDown_Replace_EaseIn->Set_Tile_Layout(55, 55, 8);
		_DropDown_Replace_EaseIn->Set_Open_Direction(true);
		_DropDown_Replace_EaseIn->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Right);
		_DropDown_Replace_EaseIn->Set_Items(Lines_First_Easings, Lines_Second_Easings, Values_Easings, Images_Easings);
		_DropDown_Replace_EaseIn->Selected_Index = 0;
		_DropDown_Replace_EaseIn->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		_DropDown_Replace_EaseIn->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnReplaceEasingChanged);
		TableLayout->Controls->Add(_DropDown_Replace_EaseIn, 3, 0);

		_DropDown_Replace_EaseOut = gcnew Control_DropDown();
		_DropDown_Replace_EaseOut->Dock = DockStyle::Fill;
		_DropDown_Replace_EaseOut->Set_Open_Direction(true);
		_DropDown_Replace_EaseOut->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Right);
		_DropDown_Replace_EaseOut->Set_Tile_Layout(55, 55, 8);
		_DropDown_Replace_EaseOut->Set_Items(Lines_First_Easings, Lines_Second_Easings, Values_Easings, Images_Easings);
		_DropDown_Replace_EaseOut->Selected_Index = 0;
		_DropDown_Replace_EaseOut->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		_DropDown_Replace_EaseOut->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Form_BatchAction::OnReplaceEasingChanged);
		TableLayout->Controls->Add(_DropDown_Replace_EaseOut, 3, 1);


		// Fade Preview
		_FadePreview_Replace = gcnew Control_FadePreview(15);
		_FadePreview_Replace->Dock = DockStyle::Fill;
		_FadePreview_Replace->Margin = System::Windows::Forms::Padding(10, 10, 5, 10);
		_FadePreview_Replace->PreviewSideSelected += gcnew PreviewSideSelectedHandler(this, &Form_BatchAction::OnReplaceFadePreviewSideSelected);
		TableLayout->Controls->Add(_FadePreview_Replace, 4, 0);
		TableLayout->SetRowSpan(_FadePreview_Replace, TableLayout->RowCount - 1);



		Label^ Label_ResueColor = gcnew Label();
		Label_ResueColor->Dock = DockStyle::Fill;
		Label_ResueColor->Text = "Use Event Color for:";
		Label_ResueColor->AutoSize = true;
		Label_ResueColor->TextAlign = ContentAlignment::MiddleLeft;
		Label_ResueColor->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_ResueColor, 0, 2);


		array<String^>^ ReuseColorsTitles = gcnew array<String^>{ "Do not use", "Start", "Center", "End" };
		array<String^>^ ReuseColorsSubtitles = gcnew array<String^>{ "", "", "", "" };  // Empty subtitles
		array<int>^ ReuseColorsValues = gcnew array<int>{ CI_FADE_COLOR_ANY, CI_FADE_COLOR_START, CI_FADE_COLOR_CENTER, CI_FADE_COLOR_END };

		_DropDown_Replace_FadeReuseColor = gcnew Control_DropDown();
		_DropDown_Replace_FadeReuseColor->Dock = DockStyle::Fill;
		_DropDown_Replace_FadeReuseColor->Set_Open_Direction(true);
		_DropDown_Replace_FadeReuseColor->Set_Tile_Layout(140 - 8, 28, 1);
		_DropDown_Replace_FadeReuseColor->Set_Items(ReuseColorsTitles, ReuseColorsSubtitles, ReuseColorsValues);
		_DropDown_Replace_FadeReuseColor->Selected_Index = 0;
		_DropDown_Replace_FadeReuseColor->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		TableLayout->Controls->Add(_DropDown_Replace_FadeReuseColor, 1, 2);



		// Swap Colors button
		Button^ Button_Replace_Fade_SwapColors = gcnew Button();
		Button_Replace_Fade_SwapColors->Dock = DockStyle::Fill;
		Button_Replace_Fade_SwapColors->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		Button_Replace_Fade_SwapColors->Text = "Swap Colors";
		Button_Replace_Fade_SwapColors->Margin = System::Windows::Forms::Padding(10, 5, 5, 5);
		Button_Replace_Fade_SwapColors->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnReplaceSwapColorsClick);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(Button_Replace_Fade_SwapColors);
		TableLayout->Controls->Add(Button_Replace_Fade_SwapColors, 4, 2);

		_Panel_Replace_Fade->Controls->Add(TableLayout);
	}

	// Initialize Strobe Replace Options
	void Form_BatchAction::InitializeComponentActionReplaceStrobePanel()
	{
		_Panel_Replace_Strobe = gcnew Panel();
		_Panel_Replace_Strobe->Dock = DockStyle::Fill;
		_Panel_Replace_Strobe->Visible = false;


		TableLayoutPanel^ TableLayout = gcnew TableLayoutPanel();
		TableLayout->Dock = DockStyle::Fill;
		TableLayout->ColumnCount = 4;
		TableLayout->RowCount = 3;
		TableLayout->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 33.0f));

		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 140));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 50));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 160));
		TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));

		Label^ Label_NewColor = gcnew System::Windows::Forms::Label();
		Label_NewColor->Dock = DockStyle::Fill;
		Label_NewColor->Text = "New Color:";
		Label_NewColor->AutoSize = true;
		Label_NewColor->TextAlign = ContentAlignment::MiddleLeft;
		Label_NewColor->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_NewColor, 0, 0);

		_PictureBox_ReplaceColorStrobe = gcnew PictureBox();
		_PictureBox_ReplaceColorStrobe->SizeMode = PictureBoxSizeMode::CenterImage;
		_PictureBox_ReplaceColorStrobe->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Top | AnchorStyles::Bottom);
		_PictureBox_ReplaceColorStrobe->Margin = System::Windows::Forms::Padding(0, 0, 2, 0);
		TableLayout->Controls->Add(_PictureBox_ReplaceColorStrobe, 1, 0);

		_Button_ReplaceColorStrobe = gcnew System::Windows::Forms::Button();
		_Button_ReplaceColorStrobe->Dock = DockStyle::Fill;
		_Button_ReplaceColorStrobe->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top | AnchorStyles::Bottom);
		_Button_ReplaceColorStrobe->Text = "Select Specific Color";
		_Button_ReplaceColorStrobe->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		_Button_ReplaceColorStrobe->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnReplaceStrobeColorPicked);
		TableLayout->Controls->Add(_Button_ReplaceColorStrobe, 2, 0);

		Label^ Label_StrobeQuantization = gcnew Label();
		Label_StrobeQuantization->Dock = DockStyle::Fill;
		Label_StrobeQuantization->Text = "Quantization:";
		Label_StrobeQuantization->AutoSize = true;
		Label_StrobeQuantization->TextAlign = ContentAlignment::MiddleLeft;
		Label_StrobeQuantization->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_StrobeQuantization, 0, 1);
		TableLayout->SetColumnSpan(Label_StrobeQuantization, 2);


		array<String^>^ Lines_First_Quantization = TimeSignatures::TimeSignatureExtendedrStringMain->ToArray();
		array<String^>^ Lines_Second_Quantization = TimeSignatures::TimeSignatureExtendedStringSub->ToArray();
		array<int>^ Values_Quantization = TimeSignatures::TimeSignatureExtendedValues->ToArray();

		_DropDown_Replace_StrobeQuantization = gcnew Control_DropDown();
		_DropDown_Replace_StrobeQuantization->Dock = DockStyle::Fill;
		_DropDown_Replace_StrobeQuantization->Set_Open_Direction(true);
		_DropDown_Replace_StrobeQuantization->Set_Tile_Layout(55, 55, 7);
		_DropDown_Replace_StrobeQuantization->Set_Items(Lines_First_Quantization, Lines_Second_Quantization, Values_Quantization);
		_DropDown_Replace_StrobeQuantization->Selected_Index = 0;
		_DropDown_Replace_StrobeQuantization->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		TableLayout->Controls->Add(_DropDown_Replace_StrobeQuantization, 2, 1);


		Label^ Label_ResueColor = gcnew Label();
		Label_ResueColor->Dock = DockStyle::Fill;
		Label_ResueColor->Text = "Keep Event Color:";
		Label_ResueColor->AutoSize = true;
		Label_ResueColor->TextAlign = ContentAlignment::MiddleLeft;
		Label_ResueColor->Margin = System::Windows::Forms::Padding(20, 0, 0, 0);
		TableLayout->Controls->Add(Label_ResueColor, 0, 2);
		TableLayout->SetColumnSpan(Label_ResueColor, 2);


		array<String^>^ ReuseColorsTitles = gcnew array<String^>{ "Yes", "No" };
		array<String^>^ ReuseColorsSubtitles = gcnew array<String^>{ "", "" };  // Empty subtitles
		array<int>^ ReuseColorsValues = gcnew array<int>{ CI_YES, CI_NO };

		_DropDown_Replace_StrobeReuseColor = gcnew Control_DropDown();
		_DropDown_Replace_StrobeReuseColor->Dock = DockStyle::Fill;
		_DropDown_Replace_StrobeReuseColor->Set_Open_Direction(true);
		_DropDown_Replace_StrobeReuseColor->Set_Tile_Layout(160 - 9, 28, 1);
		_DropDown_Replace_StrobeReuseColor->Set_Items(ReuseColorsTitles, ReuseColorsSubtitles, ReuseColorsValues);
		_DropDown_Replace_StrobeReuseColor->Selected_Index = 1;
		_DropDown_Replace_StrobeReuseColor->Margin = System::Windows::Forms::Padding(3, 5, 3, 5);
		TableLayout->Controls->Add(_DropDown_Replace_StrobeReuseColor, 2, 2);


		_Panel_Replace_Strobe->Controls->Add(TableLayout);
	}

	void Form_BatchAction::InitializeComponentFormButtons()
	{
		// Button panel (for Apply and Cancel buttons)
		_Panel_Form_Buttons = gcnew Panel();
		_Panel_Form_Buttons->Dock = DockStyle::Fill;
		_Panel_Form_Buttons->BackColor = Theme_Manager::Get_Instance()->TimelineBeatLine;

		TableLayoutPanel^ TableLayoutFormButtons = gcnew TableLayoutPanel();
		TableLayoutFormButtons->Dock = DockStyle::Fill;
		TableLayoutFormButtons->ColumnCount = 5;
		TableLayoutFormButtons->RowCount = 3; // Action, Filters, Action-specific panel
		TableLayoutFormButtons->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		TableLayoutFormButtons->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0f));
		TableLayoutFormButtons->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 34));
		TableLayoutFormButtons->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0f));

		TableLayoutFormButtons->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50.0f));
		TableLayoutFormButtons->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50.0f));
		TableLayoutFormButtons->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 120));
		TableLayoutFormButtons->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 120));
		TableLayoutFormButtons->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 20));

		_Label_AffectedCount = gcnew Label();
		_Label_AffectedCount->Dock = DockStyle::Fill;
		_Label_AffectedCount->TextAlign = ContentAlignment::MiddleLeft;
		_Label_AffectedCount->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
		_Label_AffectedCount->Text = CS_AFFECTED_EVENTS + "0";
		TableLayoutFormButtons->Controls->Add(_Label_AffectedCount, 0, 1);

		_CheckBox_ApplyOnlySelected = gcnew CheckBox();
		_CheckBox_ApplyOnlySelected->Dock = DockStyle::Fill;
		_CheckBox_ApplyOnlySelected->Text = "Apply only to selected Events";
		_CheckBox_ApplyOnlySelected->TextAlign = ContentAlignment::MiddleLeft;
		_CheckBox_ApplyOnlySelected->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
		TableLayoutFormButtons->Controls->Add(_CheckBox_ApplyOnlySelected, 1, 1);

		// Apply and Cancel buttons
		_Button_Apply = gcnew System::Windows::Forms::Button();
		_Button_Apply->Dock = DockStyle::Fill;
		_Button_Apply->Text = "Apply";
		_Button_Apply->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnApplyClick);
		TableLayoutFormButtons->Controls->Add(_Button_Apply, 2, 1);

		_Button_Cancel = gcnew System::Windows::Forms::Button();
		_Button_Cancel->Dock = DockStyle::Fill;
		_Button_Cancel->Text = "Cancel";
		_Button_Cancel->Click += gcnew System::EventHandler(this, &Form_BatchAction::OnCancelClick);
		TableLayoutFormButtons->Controls->Add(_Button_Cancel, 3, 1);

		_Panel_Form_Buttons->Controls->Add(TableLayoutFormButtons);
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

		// Apply theme to buttons using the Theme_Manager method
		array<Button^>^ Buttons = {
			_Button_DurationMode, _Button_ColorRMode, _Button_ColorGMode, _Button_ColorBMode, _Button_ActionColor, _Button_ReplaceColorSolid, _Button_ReplaceColorStrobe, _Button_Apply, _Button_Cancel
		};

		for each (Button ^ B in Buttons) {
			ThemeManager->ApplyThemeToButton(B, ThemeManager->BackgroundAlt);
		}

		// Apply theme to textboxes
		array<TextBox^>^ TextBoxes = {
			_TextBox_MeasureMinValue, _TextBox_MeasureMaxValue,
			_TextBox_DurationValue, _TextBox_DurationMinValue, _TextBox_DurationMaxValue,
			_TextBox_ColorRValue, _TextBox_ColorRMinValue, _TextBox_ColorRMaxValue,
			_TextBox_ColorGValue, _TextBox_ColorGMinValue, _TextBox_ColorGMaxValue,
			_TextBox_ColorBValue, _TextBox_ColorBMinValue, _TextBox_ColorBMaxValue
		};

		for each (TextBox ^ T in TextBoxes) {
			T->BackColor = ThemeManager->BackgroundLight;
			T->ForeColor = ThemeManager->ForegroundText;
			T->BorderStyle = BorderStyle::FixedSingle;
		}

		// Apply theme to labels for RGB controls
		array<Label^>^ Labels = {
			_Label_Track, _Label_EventType,
			_Label_MeasureTo, _Label_MeasureRange,
			_Label_DurationTo,
			_Label_ColorRTo, _Label_ColorGTo, _Label_ColorBTo,
			_Label_Color, _Label_MinColor, _Label_MaxColor
		};

		for each (Label ^ L in Labels) {
			L->ForeColor = ThemeManager->ForegroundText;
			L->BackColor = Color::Transparent;
		}

		// Action panels
		_Panel_ColorAction->BackColor = ThemeManager->Background;
		_Panel_ReplaceAction->BackColor = ThemeManager->Background;
	}

	Button^ Form_BatchAction::CreateModeButton(Point location)
	{
		Button^ New_Button = gcnew Button();
		New_Button->Text = "Specific";
		New_Button->Location = location;
		New_Button->Size = System::Drawing::Size(70, 23);

		return New_Button;
	}

	Label^ Form_BatchAction::CreateToLabel(int y)
	{
		Label^ New_Label = gcnew Label();
		New_Label->Text = "to";
		New_Label->Location = Point(514, y);
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

	// Helper methods for updating UI
	void Form_BatchAction::UpdateMeasureControls()
	{
		_TextBox_MeasureMinValue->Text = _TrackBar_Measure->MinValue.ToString();
		_TextBox_MeasureMaxValue->Text = _TrackBar_Measure->MaxValue.ToString();
		_TextBox_MeasureMinValue->Enabled = true;
		_TextBox_MeasureMaxValue->Enabled = true;
		_TextBox_MeasureMinValue->Visible = true;
		_TextBox_MeasureMaxValue->Visible = true;
		_Label_MeasureTo->Visible = true;
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
		// Check if any RGB component is in range mode
		bool AnyInRangeMode = (_TrackBar_ColorR->Mode == TrackbarRangeMode::Range) ||
			(_TrackBar_ColorG->Mode == TrackbarRangeMode::Range) ||
			(_TrackBar_ColorB->Mode == TrackbarRangeMode::Range);

		if (AnyInRangeMode) {
			// Get min and max RGB values
			int MinR = (_TrackBar_ColorR->Mode == TrackbarRangeMode::Range) ? _TrackBar_ColorR->MinValue : _TrackBar_ColorR->Value;
			int MinG = (_TrackBar_ColorG->Mode == TrackbarRangeMode::Range) ? _TrackBar_ColorG->MinValue : _TrackBar_ColorG->Value;
			int MinB = (_TrackBar_ColorB->Mode == TrackbarRangeMode::Range) ? _TrackBar_ColorB->MinValue : _TrackBar_ColorB->Value;

			int MaxR = (_TrackBar_ColorR->Mode == TrackbarRangeMode::Range) ? _TrackBar_ColorR->MaxValue : _TrackBar_ColorR->Value;
			int MaxG = (_TrackBar_ColorG->Mode == TrackbarRangeMode::Range) ? _TrackBar_ColorG->MaxValue : _TrackBar_ColorG->Value;
			int MaxB = (_TrackBar_ColorB->Mode == TrackbarRangeMode::Range) ? _TrackBar_ColorB->MaxValue : _TrackBar_ColorB->Value;

			// Set the min and max color panels
			_MinColor = Color::FromArgb(MinR, MinG, MinB);
			_MaxColor = Color::FromArgb(MaxR, MaxG, MaxB);

			if (_PictureBox_MinColor->Image != nullptr) {
				delete _PictureBox_MinColor->Image;
			}
			if (_PictureBox_MaxColor->Image != nullptr) {
				delete _PictureBox_MaxColor->Image;
			}

			_PictureBox_MinColor->Image = Control_ColorPreset::CreateColorBitmap(_MinColor, CI_COLOR_PREVIEW_SIZE);
			_PictureBox_MaxColor->Image = Control_ColorPreset::CreateColorBitmap(_MaxColor, CI_COLOR_PREVIEW_SIZE);

			// Show min/max panels and hide single color panel
			_PictureBox_MinColor->Visible = true;
			_PictureBox_MaxColor->Visible = true;
			_Label_MinColor->Visible = true;
			_Label_MaxColor->Visible = true;
			_PictureBox_Color->Visible = false;
			_Label_Color->Visible = false;
		}
		else {
			// All components in specific mode, show single color panel
			int R = _TrackBar_ColorR->Value;
			int G = _TrackBar_ColorG->Value;
			int B = _TrackBar_ColorB->Value;

			_SelectedColor = Color::FromArgb(R, G, B);

			if (_PictureBox_Color->Image != nullptr) {
				delete _PictureBox_Color->Image;
			}

			_PictureBox_Color->Image = Control_ColorPreset::CreateColorBitmap(_SelectedColor, CI_COLOR_PREVIEW_SIZE);

			// Hide min/max panels and show single color panel
			_PictureBox_MinColor->Visible = false;
			_PictureBox_MaxColor->Visible = false;
			_Label_MinColor->Visible = false;
			_Label_MaxColor->Visible = false;
			_PictureBox_Color->Visible = true;
			_Label_Color->Visible = true;
		}
	}

	void Form_BatchAction::UpdateActionColorPreview()
	{
		Bitmap^ BMP = Control_ColorPreset::CreateColorBitmap(_ActionColor, 31);

		if (_PictureBox_ActionColor->Image != nullptr) {
			delete _PictureBox_ActionColor->Image;
		}

		_PictureBox_ActionColor->Image = BMP;
	}

	void Form_BatchAction::UpdateReplaceOptions()
	{
		// Hide all type-specific panels first
		_Panel_Replace_Solid->Visible	= false;
		_Panel_Replace_Fade->Visible	= false;
		_Panel_Replace_Strobe->Visible	= false;

		// Show the appropriate panel based on selected type
		switch (_DropDown_ReplaceType->Selected_Value)
		{
			case CI_TYPE_SOLID:		_Panel_Replace_Solid->Visible	= true;	break;
			case CI_TYPE_FADE:		_Panel_Replace_Fade->Visible	= true;	break;
			case CI_TYPE_STROBE:	_Panel_Replace_Strobe->Visible	= true;	break;
		}

		UpdateReplaceOptionsPreview();
	}

	void Form_BatchAction::UpdateReplaceOptionsPreview()
	{
		if(_Panel_Replace_Solid->Visible) 
		{
			Bitmap^ BMP = Control_ColorPreset::CreateColorBitmap(_ActionReplaceColor_Solid, 31);

			if (_PictureBox_ReplaceColorSolid->Image != nullptr) {
				delete _PictureBox_ReplaceColorSolid->Image;
			}

			_PictureBox_ReplaceColorSolid->Image = BMP;
		}
		else if (_Panel_Replace_Fade->Visible)
		{

		}
		else if (_Panel_Replace_Strobe->Visible)
		{
			Bitmap^ BMP = Control_ColorPreset::CreateColorBitmap(_ActionReplaceColor_Strobe, 31);

			if (_PictureBox_ReplaceColorStrobe->Image != nullptr) {
				delete _PictureBox_ReplaceColorStrobe->Image;
			}

			_PictureBox_ReplaceColorStrobe->Image = BMP;
		}
	}
}
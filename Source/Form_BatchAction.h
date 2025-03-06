#pragma once

#include "Theme_Manager.h"
#include "Widget_Timeline.h"
#include "Widget_Timeline_Classes.h"
#include "Control_GroupBox.h"
#include "Control_DropDown.h"
#include "Control_Trackbar_Range.h"
#include "Control_ExpandablePanel.h"


using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace MIDILightDrawer
{
    public ref class Form_BatchAction : public Form
    {
    public:
        Form_BatchAction(Widget_Timeline^ timeline);
        virtual ~Form_BatchAction();

    private:
        Widget_Timeline^ _Timeline;

        // UI Elements
        Control_GroupBox^ _GroupBox_Action;
        Label^ _Label_Action;
        Control_DropDown^ _DropDown_Action;

        Control_GroupBox^ _GroupBox_Filters;

        // Expandable panels
        Control_ExpandablePanel^ _TrackFilterPanel;
        Control_ExpandablePanel^ _TypeFilterPanel;
        Control_ExpandablePanel^ _MeasureFilterPanel;
        Control_ExpandablePanel^ _DurationFilterPanel;
        Control_ExpandablePanel^ _ColorFilterPanel;

        Label^ _Label_Track;
        Control_DropDown^ _DropDown_Track;

        Label^ _Label_EventType;
        Control_DropDown^ _DropDown_EventType;

        // Duration filter elements
        Control_Trackbar_Range^ _TrackBar_Duration;
        Button^ _Button_DurationMode;
        TextBox^ _TextBox_DurationValue;
        TextBox^ _TextBox_DurationMinValue;
        TextBox^ _TextBox_DurationMaxValue;
        Label^ _Label_DurationTo;

        // RGB controls for color selection
        Control_Trackbar_Range^ _TrackBar_ColorR;
        Button^ _Button_ColorRMode;
        TextBox^ _TextBox_ColorRValue;
        TextBox^ _TextBox_ColorRMinValue;
        TextBox^ _TextBox_ColorRMaxValue;
        Label^ _Label_ColorRTo;

        Control_Trackbar_Range^ _TrackBar_ColorG;
        Button^ _Button_ColorGMode;
        TextBox^ _TextBox_ColorGValue;
        TextBox^ _TextBox_ColorGMinValue;
        TextBox^ _TextBox_ColorGMaxValue;
        Label^ _Label_ColorGTo;

        Control_Trackbar_Range^ _TrackBar_ColorB;
        Button^ _Button_ColorBMode;
        TextBox^ _TextBox_ColorBValue;
        TextBox^ _TextBox_ColorBMinValue;
        TextBox^ _TextBox_ColorBMaxValue;
        Label^ _Label_ColorBTo;

        Panel^ _Panel_Color;


        // Measure filter elements
        Control_Trackbar_Range^ _TrackBar_Measure;
        Button^ _Button_MeasureMode;
        TextBox^ _TextBox_MeasureValue;
        TextBox^ _TextBox_MeasureMinValue;
        TextBox^ _TextBox_MeasureMaxValue;
        Label^ _Label_MeasureTo;


        // Action-specific panels
        Panel^ _Panel_ColorAction;
        Button^ _Button_ActionColor;
        Panel^ _Panel_ActionColor;

        Panel^ _Panel_MoveAction;
        NumericUpDown^ _NumericUpDown_MoveAmount;
        RadioButton^ _Radio_MoveForward;
        RadioButton^ _Radio_MoveBackward;

        // Action buttons
        Button^ _Button_Apply;
        Button^ _Button_Cancel;

        // Color values
        Color _SelectedColor;
        Color _MinColor;
        Color _MaxColor;
        Color _ActionColor;

        void InitializeComponent();
        void ApplyTheme();

        // Creation Methods
        Button^ CreateModeButton(Point location);
        Label^ CreateToLabel(Point location);
        TextBox^ CreateValueTextBox(Point location, bool visible);

        // Event handlers
        void OnActionTypeChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
        void OnPanelExpandToggled(System::Object^ sender, System::EventArgs^ e);

        // Measure controls event handlers
        void OnMeasureValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
        void OnMeasureModeClick(System::Object^ sender, System::EventArgs^ e);
        void OnMeasureValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnMeasureMinValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnMeasureMaxValueKeyDown(System::Object^ sender, KeyEventArgs^ e);

        // Duration controls event handlers
        void OnDurationValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
        void OnDurationModeClick(System::Object^ sender, System::EventArgs^ e);
        void OnDurationValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnDurationMinValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnDurationMaxValueKeyDown(System::Object^ sender, KeyEventArgs^ e);

        // Color RGB trackbar event handlers
        void OnColorRValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
        void OnColorGValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
        void OnColorBValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
        void OnColorRModeClick(System::Object^ sender, System::EventArgs^ e);
        void OnColorGModeClick(System::Object^ sender, System::EventArgs^ e);
        void OnColorBModeClick(System::Object^ sender, System::EventArgs^ e);
        void OnColorRValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorRMinValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorRMaxValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorGValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorGMinValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorGMaxValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorBValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorBMinValueKeyDown(System::Object^ sender, KeyEventArgs^ e);
        void OnColorBMaxValueKeyDown(System::Object^ sender, KeyEventArgs^ e);

        void OnPickActionColorClick(System::Object^ sender, System::EventArgs^ e);

        void OnApplyClick(System::Object^ sender, System::EventArgs^ e);
        void OnCancelClick(System::Object^ sender, System::EventArgs^ e);

        // Helper methods
        void UpdateMeasureControls();
        void UpdateDurationControls();
        void UpdateColorRControls();
        void UpdateColorGControls();
        void UpdateColorBControls();
        void UpdateColorPreview();
        void UpdatePanelPositions();

        bool MatchesFilter(BarEvent^ event);
        void ExecuteBatchAction();
        void SelectEventsAction();
        void DeleteEventsAction();
        void ChangeColorAction();
        void MoveEventsAction();
    };
}
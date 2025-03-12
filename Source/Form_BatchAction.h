#pragma once

#include "Widget_Timeline.h"
#include "Widget_Timeline_Classes.h"
#include "Control_GroupBox.h"
#include "Control_DropDown.h"
#include "Dialog_ColorPicker.h"
#include "Control_RadioButton.h"
#include "Control_FadePreview.h"
#include "Control_Trackbar_Range.h"
#include "Control_ExpandablePanel.h"
#include "Control_ScrollablePanel.h"


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
	private:
		static const int CI_NO = 0;
		static const int CI_YES = 1;
				
		static const int CI_ACTION_SELECTION = 0;
		static const int CI_ACTION_DELETE = 1;
		static const int CI_ACTION_COLOR = 2;
		static const int CI_ACTION_REPLACE = 3;

		static const int CI_TYPE_SOLID = 0;
		static const int CI_TYPE_FADE = 1;
		static const int CI_TYPE_STROBE = 2;
				
		static const int CI_FADE_COLOR_ANY = 0;
		static const int CI_FADE_COLOR_START = 1;
		static const int CI_FADE_COLOR_CENTER = 2;
		static const int CI_FADE_COLOR_END = 3;

		static const int CI_COLOR_PREVIEW_SIZE = 31;

		static initonly System::String^ CS_SPECFIC = "Specific";
		static initonly System::String^ CS_RANGE = "Range";
		static initonly System::String^ CS_AFFECTED_EVENTS = "Affected Events: ";
		static initonly System::String^ CS_SELECTED_EVENTS = "Events currently selected: ";

    public:
        Form_BatchAction(Widget_Timeline^ timeline);
        virtual ~Form_BatchAction();

    private:
        Widget_Timeline^ _Timeline;

        // UI Elements
		TableLayoutPanel^ _MainLayout;
		Control_ScrollablePanel^ _Scrollable_Panel;
        Control_GroupBox^ _GroupBox_Action;
        Label^	_Label_Action;
        Control_DropDown^ _DropDown_Action;

        Control_GroupBox^ _GroupBox_Filters;

        // Expandable panels
		Control_ExpandablePanel^ _SelectionFilterPanel;
        Control_ExpandablePanel^ _TrackFilterPanel;
        Control_ExpandablePanel^ _TypeFilterPanel;
        Control_ExpandablePanel^ _MeasureFilterPanel;
        Control_ExpandablePanel^ _DurationFilterPanel;
        Control_ExpandablePanel^ _ColorFilterPanel;

		Label^ _Label_SelectionFilter;
		Control_DropDown^ _DropDown_SelectionFilter;

        Label^ _Label_Track;
        Control_DropDown^ _DropDown_Track;

        Label^ _Label_EventType;
        Control_DropDown^ _DropDown_EventType;


		/////////////////////////////
		// Measure filter elements //
		/////////////////////////////
		Control_Trackbar_Range^ _TrackBar_Measure;
		Label^ _Label_MeasureRange;
		TextBox^ _TextBox_MeasureMinValue;
		TextBox^ _TextBox_MeasureMaxValue;
		Label^ _Label_MeasureTo;


		//////////////////////////////
        // Duration filter elements //
		//////////////////////////////
		Control_Trackbar_Range^ _TrackBar_Duration;
        Button^ _Button_DurationMode;
        TextBox^ _TextBox_DurationValue;
        TextBox^ _TextBox_DurationMinValue;
        TextBox^ _TextBox_DurationMaxValue;
        Label^ _Label_DurationTo;


		//////////////////////////////////////
        // RGB controls for color selection //
		//////////////////////////////////////
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

		Label^ _Label_FadeColorPoint;
		Control_DropDown^ _DropDown_FadeColorFilter;


		PictureBox^ _PictureBox_Color;
		PictureBox^ _PictureBox_MinColor;
		PictureBox^ _PictureBox_MaxColor;
		Label^ _Label_Color;
		Label^ _Label_MinColor;
		Label^ _Label_MaxColor;

		Button^ _Button_PickColor;
		Button^ _Button_PickMinColor;
		Button^ _Button_PickMaxColor;
		

		//////////////////////////////
        // Action-specific controls //
		//////////////////////////////
		Panel^ _Panel_Action_Container;

		Panel^ _Panel_SelectAction;
		Label^ _Label_SelectionCount;
		Control_RadioButton^ _Radio_NewSelection;
		Control_RadioButton^ _Radio_AddToSelection;
		Control_RadioButton^ _Radio_RemoveFromSelection;

		Panel^ _Panel_ColorAction;
        Button^ _Button_ActionColor;
		PictureBox^ _PictureBox_ActionColor;
		Control_DropDown^ _DropDown_FadeColorTarget;
		
        Panel^ _Panel_ReplaceAction;
		Control_DropDown^ _DropDown_ReplaceType;

		Panel^ _Panel_Replace_Options;
		
		Panel^ _Panel_Replace_Solid;
		PictureBox^ _PictureBox_ReplaceColorSolid;
		Button^ _Button_ReplaceColorSolid;

		Panel^ _Panel_Replace_Fade;

		Panel^ _Panel_Replace_Strobe;
		PictureBox^ _PictureBox_ReplaceColorStrobe;
		Button^ _Button_ReplaceColorStrobe;

		// For Fade options
		Control_FadePreview^ _FadePreview_Replace;
		Control_DropDown^ _DropDown_Replace_FadeQuantization;
		
		Control_DropDown^ _DropDown_Replace_Fade_Mode;
		Control_DropDown^ _DropDown_Replace_EaseIn;
		Control_DropDown^ _DropDown_Replace_EaseOut;
		Control_DropDown^ _DropDown_Replace_FadeReuseColor;

		// For Strobe options
		Control_DropDown^ _DropDown_Replace_StrobeQuantization;
		Control_DropDown^ _DropDown_Replace_StrobeReuseColor;


		//////////////////
        // Form Buttons //
		//////////////////
		Panel^ _Panel_Form_Buttons;
		Label^ _Label_AffectedCount;
		CheckBox^ _CheckBox_ApplyOnlySelected;
		Button^ _Button_Apply;
        Button^ _Button_Cancel;


        // Member Variables
        Color _SelectedColor;
        Color _MinColor;
        Color _MaxColor;
        Color _ActionColor;
		Color _ActionReplaceColor_Solid;
		Color _ActionReplaceColor_Strobe;
		List<BarEvent^>^ _MatchingEvents;
		int _Count_Total_Events;

        void InitializeComponent();			// In From_BatchAction_UI.cpp
        void InitializeComponentAction();	// In From_BatchAction_UI.cpp
        void InitializeComponentFilter();	// In From_BatchAction_UI.cpp
		void InitializeComponentFilterSelection(int width);	// In From_BatchAction_UI.cpp
		void InitializeComponentFilterTrack(int width);		// In From_BatchAction_UI.cpp
		void InitializeComponentFilterType(int width);		// In From_BatchAction_UI.cpp
		void InitializeComponentFilterMeasure(int width);	// In From_BatchAction_UI.cpp
		void InitializeComponentFilterDuration(int width);	// In From_BatchAction_UI.cpp
		void InitializeComponentFilterColor(int width);		// In From_BatchAction_UI.cpp
		void InitializeComponentActionMain();				// In From_BatchAction_UI.cpp
		void InitializeComponentActionSelection();			// In From_BatchAction_UI.cpp
		void InitializeComponentActionColor();				// In From_BatchAction_UI.cpp
		void InitializeComponentActionReplace();			// In From_BatchAction_UI.cpp
		void InitializeComponentActionReplaceSolidPanel();	// In From_BatchAction_UI.cpp
		void InitializeComponentActionReplaceFadePanel();	// In From_BatchAction_UI.cpp
		void InitializeComponentActionReplaceStrobePanel();	// In From_BatchAction_UI.cpp
		void InitializeComponentFormButtons();				// In From_BatchAction_UI.cpp
        void ApplyTheme();									// In From_BatchAction_UI.cpp

        // Creation Methods
        Button^ CreateModeButton(Point location);					// In From_BatchAction_UI.cpp
        Label^ CreateToLabel(int y);								// In From_BatchAction_UI.cpp
        TextBox^ CreateValueTextBox(Point location, bool visible);	// In From_BatchAction_UI.cpp

        // Event handlers
		void OnFilterToggleChanged(System::Object^ sender, EventArgs^ e);
        void OnActionTypeChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void OnFilterDropDownItemChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);

        // Measure controls event handlers
        void OnMeasureValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
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
		void OnPickColorClick(System::Object^ sender, System::EventArgs^ e);
		void OnPickMinColorClick(System::Object^ sender, System::EventArgs^ e);
		void OnPickMaxColorClick(System::Object^ sender, System::EventArgs^ e);
		void UpdateRGBTrackbarsFromColor(Color color);

        void OnPickActionColorClick(System::Object^ sender, System::EventArgs^ e);

		void OnReplaceTypeChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void OnReplaceSolidColorPicked(System::Object^ sender, System::EventArgs^ e);
		void OnReplaceToggleModeClick(System::Object^ sender, System::EventArgs^ e);
		void OnReplaceFadeQuantizationChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void OnReplaceStrobeQuantizationChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void OnReplaceEasingChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
		void OnReplaceFadePreviewSideSelected(System::Drawing::Color color);
		void OnReplaceSwapColorsClick(System::Object^ sender, System::EventArgs^ e);
		void OnReplaceStrobeColorPicked(System::Object^ sender, System::EventArgs^ e);

        void OnApplyClick(System::Object^ sender, System::EventArgs^ e);
        void OnCancelClick(System::Object^ sender, System::EventArgs^ e);
				
        // Helper methods -> All in From_BatchAction_UI.cpp
        void UpdateMeasureControls();
        void UpdateDurationControls();
        void UpdateColorRControls();
        void UpdateColorGControls();
        void UpdateColorBControls();
        void UpdateColorPreview();
		void UpdateActionColorPreview();
		void UpdateReplaceOptions();
		void UpdateReplaceOptionsPreview();


		void UpdateEventMeasureIndexes();
		void UpdateAffectedEventsCount();
		bool MatchesFilter(BarEvent^ event);
		bool MatchesFilterColor(Color color);
		void ExecuteBatchAction();
        void SelectEventsAction();
        void DeleteEventsAction();
        void ChangeColorAction();
		void ReplaceEventsAction();
    };
}
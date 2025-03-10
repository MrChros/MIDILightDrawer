#include "Form_BatchAction.h"

#include "Theme_Manager.h"
#include "Timeline_Command_Manager.h"

namespace MIDILightDrawer
{
    Form_BatchAction::Form_BatchAction(Widget_Timeline^ timeline)
    {
        _Timeline = timeline;

        // Initialize Members
        _SelectedColor				= Color::White;
        _MinColor					= Color::Black;
        _MaxColor					= Color::White;
        _ActionColor				= Color::Red;
        _ActionReplaceColor_Solid	= Color::Red;
        _ActionReplaceColor_Strobe	= Color::Red;
		_MatchingEvents = gcnew List<BarEvent^>();
		_Count_Total_Events = 0;


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


        

        // Set initial enabled states
        OnActionTypeChanged(nullptr, nullptr);

        // Set initial state of trackbars and controls
        UpdateMeasureControls();
        UpdateDurationControls();
        UpdateColorRControls();
        UpdateColorGControls();
        UpdateColorBControls();
        UpdateColorPreview();
		UpdateEventMeasureIndexes();
		UpdateAffectedEventsCount();
    }

    Form_BatchAction::~Form_BatchAction()
    {
        // Clean up resources if needed
    }

	void Form_BatchAction::OnFilterToggleChanged(System::Object^ sender, EventArgs^ e)
	{
		UpdateAffectedEventsCount();
	}

    void Form_BatchAction::OnActionTypeChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
    {
		// Hide all action-specific panels first
		_Panel_SelectAction->Visible = false;
		_Panel_ColorAction->Visible = false;
		_Panel_ReplaceAction->Visible = false;

		// Show the appropriate panel based on selected action
		if (e == nullptr || e->Value == CI_ACTION_SELECTION) { // Select Events
			_Panel_SelectAction->Visible = true;

			int Count_Selected_Event = _Timeline->ToolAccess()->SelectedBars->Count;
			_Label_SelectionCount->Text = CS_SELECTED_EVENTS + Count_Selected_Event.ToString();

			_CheckBox_ApplyOnlySelected->Visible = false;
		}
		else if (e != nullptr && e->Value == CI_ACTION_DELETE) { // Delete Events
			_CheckBox_ApplyOnlySelected->Visible = true;
		}
		else if (e != nullptr && e->Value == CI_ACTION_COLOR) { // Change Color
			_Panel_ColorAction->Visible = true;
			_CheckBox_ApplyOnlySelected->Visible = true;
		}
		else if (e != nullptr && e->Value == CI_ACTION_REPLACE) { // Replace Events
			_Panel_ReplaceAction->Visible = true;
			_CheckBox_ApplyOnlySelected->Visible = true;

			UpdateReplaceOptions();
		}
    }

	void Form_BatchAction::OnFilterDropDownItemChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		Control_DropDown^ Sender = safe_cast<Control_DropDown^>(sender);
		
		if (Sender == _DropDown_SelectionFilter) {
			_SelectionFilterPanel->IsEnabled = true;
		}
		else if (Sender == _DropDown_Track) {
			_TrackFilterPanel->IsEnabled = true;
		}
		else if (Sender == _DropDown_EventType) {
			_TypeFilterPanel->IsEnabled = true;
		}

		UpdateAffectedEventsCount();
	}

    void Form_BatchAction::OnMeasureValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
		_TextBox_MeasureMinValue->Text = e->MinValue.ToString();
		_TextBox_MeasureMaxValue->Text = e->MaxValue.ToString();

		_MeasureFilterPanel->IsEnabled = true;

		UpdateAffectedEventsCount();
    }

	void Form_BatchAction::OnMeasureMinValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter)
        {
            int value;
            if (Int32::TryParse(_TextBox_MeasureMinValue->Text, value)) {
                _TrackBar_Measure->MinValue = value;
				_MeasureFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
            if (Int32::TryParse(_TextBox_MeasureMaxValue->Text, value)) {
                _TrackBar_Measure->MaxValue = value;
				_MeasureFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }

    void Form_BatchAction::OnDurationValueChanged(Object^ sender, TrackbarRangeValueChangedEventArgs^ e)
    {
        _TextBox_DurationValue->Text = e->Value.ToString();
		_DurationFilterPanel->IsEnabled = true;

        UpdateDurationControls();
		UpdateAffectedEventsCount();
    }

    void Form_BatchAction::OnDurationModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode
        if (_TrackBar_Duration->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_Duration->Mode = TrackbarRangeMode::Range;
            _Button_DurationMode->Text = CS_RANGE;
        }
        else {
            _TrackBar_Duration->Mode = TrackbarRangeMode::Specific;
            _Button_DurationMode->Text = CS_SPECFIC;
        }

		_DurationFilterPanel->IsEnabled = true;

        UpdateDurationControls();
		UpdateAffectedEventsCount();
    }

    void Form_BatchAction::OnDurationValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_DurationValue->Text, value)) {
                _TrackBar_Duration->Value = value;
				_DurationFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_DurationFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_DurationFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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

		_ColorFilterPanel->IsEnabled = true;

        UpdateColorPreview();
		UpdateAffectedEventsCount();
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

		_ColorFilterPanel->IsEnabled = true;

        UpdateColorPreview();
		UpdateAffectedEventsCount();
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

		_ColorFilterPanel->IsEnabled = true;

        UpdateColorPreview();
		UpdateAffectedEventsCount();
    }

    void Form_BatchAction::OnColorRModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode for R component
        if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_ColorR->Mode = TrackbarRangeMode::Range;
            _Button_ColorRMode->Text = CS_RANGE;
        }
        else {
            _TrackBar_ColorR->Mode = TrackbarRangeMode::Specific;
            _Button_ColorRMode->Text = CS_SPECFIC;
        }

		_ColorFilterPanel->IsEnabled = true;

        UpdateColorRControls();
        UpdateColorPreview();
		UpdateAffectedEventsCount();
    }

    void Form_BatchAction::OnColorGModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode for G component
        if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_ColorG->Mode = TrackbarRangeMode::Range;
            _Button_ColorGMode->Text = CS_RANGE;
        }
        else {
            _TrackBar_ColorG->Mode = TrackbarRangeMode::Specific;
            _Button_ColorGMode->Text = CS_SPECFIC;
        }

		_ColorFilterPanel->IsEnabled = true;

        UpdateColorGControls();
        UpdateColorPreview();
		UpdateAffectedEventsCount();
    }

    void Form_BatchAction::OnColorBModeClick(Object^ sender, EventArgs^ e)
    {
        // Toggle between single value and range mode for B component
        if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
            _TrackBar_ColorB->Mode = TrackbarRangeMode::Range;
            _Button_ColorBMode->Text = CS_RANGE;
        }
        else {
            _TrackBar_ColorB->Mode = TrackbarRangeMode::Specific;
            _Button_ColorBMode->Text = CS_SPECFIC;
        }

		_ColorFilterPanel->IsEnabled = true;

        UpdateColorBControls();
        UpdateColorPreview();
		UpdateAffectedEventsCount();
    }

    void Form_BatchAction::OnColorRValueKeyDown(Object^ sender, KeyEventArgs^ e)
    {
        if (e->KeyCode == Keys::Enter) {
            int value;
            if (Int32::TryParse(_TextBox_ColorRValue->Text, value)) {
                _TrackBar_ColorR->Value = Math::Min(255, Math::Max(0, value));
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
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
				_ColorFilterPanel->IsEnabled = true;

				UpdateAffectedEventsCount();
            }
            e->Handled = true;
            e->SuppressKeyPress = true;
        }
    }
	
    void Form_BatchAction::OnPickActionColorClick(System::Object^ sender, System::EventArgs^ e)
    {
        Dialog_ColorPicker^ ColorPicker = gcnew Dialog_ColorPicker();
		ColorPicker->SelectedColor = _ActionColor;

        if (ColorPicker->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
            _ActionColor = ColorPicker->SelectedColor;

			UpdateActionColorPreview();
        }
    }

	void Form_BatchAction::OnReplaceTypeChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		UpdateReplaceOptions();
	}

	void Form_BatchAction::OnReplaceSolidColorPicked(System::Object^ sender, System::EventArgs^ e)
	{
		Dialog_ColorPicker^ ColorPicker = gcnew Dialog_ColorPicker();
		ColorPicker->SelectedColor = _ActionReplaceColor_Solid;

		if (ColorPicker->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			_ActionReplaceColor_Solid = ColorPicker->SelectedColor;

			UpdateReplaceOptionsPreview();
		}
	}

	void Form_BatchAction::OnReplaceToggleModeClick(System::Object^ sender, System::EventArgs^ e)
	{
		_FadePreview_Replace->Toggle_Mode();
	}

	void Form_BatchAction::OnReplaceFadeQuantizationChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		// Nothing needed here - just store the value for later
	}

	void Form_BatchAction::OnReplaceStrobeQuantizationChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		// Nothing needed here - just store the value for later
	}

	void Form_BatchAction::OnReplaceEasingChanged(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		// Update fade preview easings
		_FadePreview_Replace->EaseIn = static_cast<FadeEasing>(_DropDown_Replace_EaseIn->Selected_Value);
		_FadePreview_Replace->EaseOut = static_cast<FadeEasing>(_DropDown_Replace_EaseOut->Selected_Value);
	}

	void Form_BatchAction::OnReplaceFadePreviewSideSelected(System::Drawing::Color color)
	{
		Dialog_ColorPicker^ ColorPicker = gcnew Dialog_ColorPicker();
		ColorPicker->SelectedColor = color;

		if (ColorPicker->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			_FadePreview_Replace->Set_Color(ColorPicker->SelectedColor);

			//UpdateReplaceOptionsPreview();
		}
	}

	void Form_BatchAction::OnReplaceSwapColorsClick(System::Object^ sender, System::EventArgs^ e)
	{
		// Switch start and end colors in fade preview
		_FadePreview_Replace->Switch_Colors();
	}

	void Form_BatchAction::OnReplaceStrobeColorPicked(System::Object^ sender, System::EventArgs^ e)
	{
		Dialog_ColorPicker^ ColorPicker = gcnew Dialog_ColorPicker();
		ColorPicker->SelectedColor = _ActionReplaceColor_Strobe;

		if (ColorPicker->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
			_ActionReplaceColor_Strobe = ColorPicker->SelectedColor;

			UpdateReplaceOptionsPreview();
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

	void Form_BatchAction::UpdateEventMeasureIndexes()
	{
		_Count_Total_Events = 0;
		
		for each (Track^ Trk in _Timeline->Tracks)
		{
			_Count_Total_Events += Trk->Events->Count;
			
			for each (BarEvent^ Event in Trk->Events)
			{
				// Find the measure containing the event
				Measure^ EventMeasure = _Timeline->GetMeasureAtTick(Event->StartTick);
				if (EventMeasure == nullptr) {
					return;
				}

				// Get the measure index (1-based)
				for (int i = 0; i < _Timeline->Measures->Count; i++)
				{
					if (_Timeline->Measures[i] == EventMeasure) {
						Event->MeasureIndex = i + 1;
						break;
					}
				}
			}
		}
	}

	void Form_BatchAction::UpdateAffectedEventsCount()
	{
		_MatchingEvents->Clear();

		// Find all matching events across all tracks
		for each (Track^ Trk in _Timeline->Tracks)
		{
			for each (BarEvent^ Event in Trk->Events)
			{
				if (MatchesFilter(Event)) {
					_MatchingEvents->Add(Event);
				}
			}
		}

		// Update the label text
		_Label_AffectedCount->Text = CS_AFFECTED_EVENTS + _MatchingEvents->Count.ToString() + " of " + this->_Count_Total_Events.ToString();
	}

    bool Form_BatchAction::MatchesFilter(BarEvent^ event)
    {
		/////////////////////////////////
		// Filter by current selection //
		/////////////////////////////////
		if (_SelectionFilterPanel->IsEnabled && _DropDown_SelectionFilter->Selected_Value == 1)
		{
			// Get the current selected events from the timeline's current tool
			List<BarEvent^>^ SelectedEvents = _Timeline->ToolAccess()->SelectedBars;

			// If the event is not in the selected events, exclude it
			if (SelectedEvents == nullptr || !SelectedEvents->Contains(event))
			{
				return false;
			}
		}
		
		
		/////////////////////
		// Filter by track //
		/////////////////////
		if (_TrackFilterPanel->IsEnabled)
		{
			int TrackIndex = _DropDown_Track->Selected_Value;
			if (TrackIndex >= 0 && TrackIndex < _Timeline->Tracks->Count)
			{
				if (event->ContainingTrack != _Timeline->Tracks[TrackIndex]) {
					return false;
				}
			}
		}


		//////////////////////////
		// Filter by Event Type //
		//////////////////////////
		if (_TypeFilterPanel->IsEnabled)
		{
			int TypeIndex = _DropDown_EventType->Selected_Value;
			BarEventType SelectedType;

			switch (TypeIndex)
			{
				case CI_TYPE_SOLID:		SelectedType = BarEventType::Solid; break;
				case CI_TYPE_FADE:		SelectedType = BarEventType::Fade;	break;
				case CI_TYPE_STROBE:	SelectedType = BarEventType::Strobe; break;
				default:				SelectedType = BarEventType::Solid; break;
			}

			if (event->Type != SelectedType) {
				return false;
			}
		}


		/////////////////////////////
		// Filter by Measure Range //
		/////////////////////////////
		if (_MeasureFilterPanel->IsEnabled)
		{
			int MinMeasure = _TrackBar_Measure->MinValue;
			int MaxMeasure = _TrackBar_Measure->MaxValue;

			if (event->MeasureIndex <= MinMeasure || event->MeasureIndex > MaxMeasure)
			{
				return false;
			}
		}


		////////////////////////
		// Filter by Duration //
		////////////////////////
		if (_DurationFilterPanel->IsEnabled)
		{
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


		/////////////////////
		// Filter by Color //
		/////////////////////
		if (_ColorFilterPanel->IsEnabled)
		{
			// Get the appropriate color to check based on event type
			Color ColorToCheck;
			if (event->Type == BarEventType::Solid || event->Type == BarEventType::Strobe) {
				
				if (!MatchesFilterColor(event->Color)) {
					return false;
				}
			}
			else if (event->Type == BarEventType::Fade && event->FadeInfo != nullptr) {
				if (_DropDown_FadeColorFilter->Selected_Value == CI_FADE_COLOR_ANY) {
					bool MatchFound = false;
					if (MatchesFilterColor(event->FadeInfo->ColorStart)) {
						MatchFound = true;
					}

					if (event->FadeInfo->Type == FadeType::Three_Colors && MatchesFilterColor(event->FadeInfo->ColorCenter)) {
						MatchFound = true;
					}

					if (MatchesFilterColor(event->FadeInfo->ColorEnd)) {
						MatchFound = true;
					}

					if (!MatchFound) {
						return false;
					}
				}
				else {
					if (_DropDown_FadeColorFilter->Selected_Value == CI_FADE_COLOR_START) {
						if (!MatchesFilterColor(event->FadeInfo->ColorStart)) {
							return false;
						}
					}
					else if (_DropDown_FadeColorFilter->Selected_Value == CI_FADE_COLOR_CENTER) {
						if (event->FadeInfo->Type != FadeType::Three_Colors) {
							return false;
						}

						if (!MatchesFilterColor(event->FadeInfo->ColorCenter)) {
							return false;
						}
					}
					else if (_DropDown_FadeColorFilter->Selected_Value == CI_FADE_COLOR_START) {
						if (!MatchesFilterColor(event->FadeInfo->ColorEnd)) {
							return false;
						}
					}
				}
			}
		}

		// All filters passed
		return true;
    }

	bool Form_BatchAction::MatchesFilterColor(Color color)
	{
		// Check R component
		if (_TrackBar_ColorR->Mode == TrackbarRangeMode::Specific) {
			int ExactR = _TrackBar_ColorR->Value;
			const int Tolerance = 5;
			if (Math::Abs(color.R - ExactR) > Tolerance) {
				return false;
			}
		}
		else {
			int MinR = _TrackBar_ColorR->MinValue;
			int MaxR = _TrackBar_ColorR->MaxValue;
			if (color.R < MinR || color.R > MaxR) {
				return false;
			}
		}

		// Check G component
		if (_TrackBar_ColorG->Mode == TrackbarRangeMode::Specific) {
			int ExactG = _TrackBar_ColorG->Value;
			const int Tolerance = 5;
			if (Math::Abs(color.G - ExactG) > Tolerance) {
				return false;
			}
		}
		else {
			int MinG = _TrackBar_ColorG->MinValue;
			int MaxG = _TrackBar_ColorG->MaxValue;
			if (color.G < MinG || color.G > MaxG) {
				return false;
			}
		}

		// Check B component
		if (_TrackBar_ColorB->Mode == TrackbarRangeMode::Specific) {
			int ExactB = _TrackBar_ColorB->Value;
			const int Tolerance = 5;
			if (Math::Abs(color.B - ExactB) > Tolerance) {
				return false;
			}
		}
		else {
			int MinB = _TrackBar_ColorB->MinValue;
			int MaxB = _TrackBar_ColorB->MaxValue;
			if (color.B < MinB || color.B > MaxB) {
				return false;
			}
		}

		return true;
	}

    void Form_BatchAction::ExecuteBatchAction()
    {
		// Determine which action to perform
		switch (_DropDown_Action->Selected_Value)
		{
			case CI_ACTION_SELECTION:	SelectEventsAction();	break;  // Select Events
			case CI_ACTION_DELETE:		DeleteEventsAction();	break;  // Delete Events
			case CI_ACTION_COLOR:		ChangeColorAction();	break;  // Change Color
			case CI_ACTION_REPLACE:		ReplaceEventsAction();	break;  // Repalce Events
		}
	}

	void Form_BatchAction::SelectEventsAction()
	{
		if (_Radio_NewSelection->Selected) {
			_Timeline->ToolAccess()->SelectedBars->Clear();
		}

		for each (BarEvent^ Event in _MatchingEvents)
		{
			if (!_Timeline->ToolAccess()->SelectedBars->Contains(Event) && (_Radio_NewSelection->Selected  || _Radio_AddToSelection->Selected)) {
				_Timeline->ToolAccess()->SelectedBars->Add(Event);
			}
			else if (_Timeline->ToolAccess()->SelectedBars->Contains(Event) && _Radio_RemoveFromSelection->Selected) {
				_Timeline->ToolAccess()->SelectedBars->Remove(Event);
			}
		}
	}

    void Form_BatchAction::DeleteEventsAction()
    {
		if (this->_MatchingEvents->Count == 0) {
			return;
		}

        CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Batch Delete Events");

        for each (BarEvent^ Event in this->_MatchingEvents) {
			if (_CheckBox_ApplyOnlySelected->Checked && !_Timeline->ToolAccess()->SelectedBars->Contains(Event)) {
				continue;
			}

            DeleteBarCommand^ Cmd = gcnew DeleteBarCommand(_Timeline, Event->ContainingTrack, Event);
            CompoundCmd->AddCommand(Cmd);
        }

		if(CompoundCmd->GetCommandCount() > 0) {
			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}
    }

    void Form_BatchAction::ChangeColorAction()
    {
		if (this->_MatchingEvents->Count == 0) {
			return;
		}

        // Change color of the matching events
        CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Batch Change Color");

        for each (BarEvent^ Event in this->_MatchingEvents)
		{
			if (_CheckBox_ApplyOnlySelected->Checked && !_Timeline->ToolAccess()->SelectedBars->Contains(Event)) {
				continue;
			}
			
			if (Event->Type == BarEventType::Solid)
			{
                // For solid bars, change the color directly
                ChangeBarColorCommand^ Cmd = gcnew ChangeBarColorCommand(_Timeline, Event, Event->Color, _ActionColor);
                CompoundCmd->AddCommand(Cmd);
            }
            else if (Event->Type == BarEventType::Fade && Event->FadeInfo != nullptr)
			{
				if (_DropDown_FadeColorTarget->Selected_Value == CI_FADE_COLOR_START) {
					ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, Event, ChangeFadeBarColorCommand::ColorType::Start, Event->FadeInfo->ColorStart, _ActionColor);
					CompoundCmd->AddCommand(Cmd);
				}
				else if (Event->FadeInfo->Type == FadeType::Three_Colors && _DropDown_FadeColorTarget->Selected_Value == CI_FADE_COLOR_CENTER) {
					ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, Event, ChangeFadeBarColorCommand::ColorType::Center, Event->FadeInfo->ColorCenter, _ActionColor);
					CompoundCmd->AddCommand(Cmd);
				}
				else if (_DropDown_FadeColorTarget->Selected_Value == CI_FADE_COLOR_END) {
					ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(_Timeline, Event, ChangeFadeBarColorCommand::ColorType::End, Event->FadeInfo->ColorEnd, _ActionColor);
					CompoundCmd->AddCommand(Cmd);
				}
            }
            else if (Event->Type == BarEventType::Strobe && Event->StrobeInfo != nullptr)
			{
                // For strobe bars, change the strobe color
				ChangeBarColorCommand^ Cmd = gcnew ChangeBarColorCommand(_Timeline, Event, Event->StrobeInfo->ColorStrobe, _ActionColor);
				CompoundCmd->AddCommand(Cmd);
            }
        }

		if (CompoundCmd->GetCommandCount() > 0) {
			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}
    }

    void Form_BatchAction::ReplaceEventsAction()
    {
		if (this->_MatchingEvents->Count == 0) {
			return;
		}

		CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Batch Replace Events");

		// Determine the replacement type
		BarEventType ReplaceType = static_cast<BarEventType>(_DropDown_ReplaceType->Selected_Value);

		for each (BarEvent^ Event in this->_MatchingEvents)
		{
			if (_CheckBox_ApplyOnlySelected->Checked && !_Timeline->ToolAccess()->SelectedBars->Contains(Event)) {
				continue;
			}
			
			// Delete the existing event first
			DeleteBarCommand^ DeleteCmd = gcnew DeleteBarCommand(_Timeline, Event->ContainingTrack, Event);
			CompoundCmd->AddCommand(DeleteCmd);

			// Create a new event based on the selected type
			BarEvent^ NewEvent = nullptr;

			if (ReplaceType == BarEventType::Solid)
			{
				// Create a solid event
				NewEvent = gcnew BarEvent(
					Event->ContainingTrack,
					Event->StartTick,
					Event->Duration,
					_ActionReplaceColor_Solid
				);
			}
			else if (ReplaceType == BarEventType::Fade)
			{
				// Check fade mode (Two_Colors or Three_Colors)
				FadeType FadeMode = _FadePreview_Replace->Type;

				// Create fade info based on fade mode
				BarEventFadeInfo^ FadeInfo = nullptr;

				if (FadeMode == FadeType::Two_Colors)
				{
					Color ColorStart = _FadePreview_Replace->StartColor;
					Color ColorEnd = _FadePreview_Replace->EndColor;
					
					if (_DropDown_Replace_FadeReuseColor->Selected_Value == CI_FADE_COLOR_START) {
						if (Event->Type == BarEventType::Fade) {
							ColorStart = Event->FadeInfo->ColorStart;
						} 
						else {
							ColorStart = Event->Color;
						}
					}
					else if (_DropDown_Replace_FadeReuseColor->Selected_Value == CI_FADE_COLOR_END) {
						if (Event->Type == BarEventType::Fade) {
							ColorEnd = Event->FadeInfo->ColorEnd;
						}
						else {
							ColorEnd = Event->Color;
						}
					}

					// Two-color fade
					FadeInfo = gcnew BarEventFadeInfo(
						_DropDown_Replace_FadeQuantization->Selected_Value,
						ColorStart,
						ColorEnd,
						static_cast<FadeEasing>(_DropDown_Replace_EaseIn->Selected_Value),
						static_cast<FadeEasing>(_DropDown_Replace_EaseOut->Selected_Value)
					);
				}
				else // Three_Colors
				{
					Color ColorStart = _FadePreview_Replace->StartColor;
					Color ColorCenter = _FadePreview_Replace->CenterColor;
					Color ColorEnd = _FadePreview_Replace->EndColor;
					
					if (_DropDown_Replace_FadeReuseColor->Selected_Value == CI_FADE_COLOR_START) {
						if (Event->Type == BarEventType::Fade) {
							ColorStart = Event->FadeInfo->ColorStart;
						}
						else {
							ColorStart = Event->Color;
						}
					}
					else if (_DropDown_Replace_FadeReuseColor->Selected_Value == CI_FADE_COLOR_CENTER) {
						if (Event->Type == BarEventType::Fade && Event->FadeInfo->Type == FadeType::Three_Colors) {
							ColorCenter = Event->FadeInfo->ColorCenter;
						}
						else {
							ColorCenter = Event->Color;
						}
					}
					else if (_DropDown_Replace_FadeReuseColor->Selected_Value == CI_FADE_COLOR_END) {
						if (Event->Type == BarEventType::Fade) {
							ColorEnd = Event->FadeInfo->ColorEnd;
						}
						else {
							ColorEnd = Event->Color;
						}
					}

					// Three-color fade
					FadeInfo = gcnew BarEventFadeInfo(
						_DropDown_Replace_FadeQuantization->Selected_Value,
						ColorStart,
						ColorCenter,
						ColorEnd,
						static_cast<FadeEasing>(_DropDown_Replace_EaseIn->Selected_Value),
						static_cast<FadeEasing>(_DropDown_Replace_EaseOut->Selected_Value)
					);
				}

				// Create the fade event with the fade info
				NewEvent = gcnew BarEvent(
					Event->ContainingTrack,
					Event->StartTick,
					Event->Duration,
					FadeInfo
				);
			}
			else if (ReplaceType == BarEventType::Strobe)
			{
				Color StrobeColor = _ActionReplaceColor_Strobe;

				if (_DropDown_Replace_StrobeReuseColor->Selected_Value == CI_YES) {
					if (Event->Type == BarEventType::Fade) {
						StrobeColor = Event->FadeInfo->ColorStart;
					}
					else {
						StrobeColor = Event->Color;
					}
				}
				
				// Create strobe info
				BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(
					_DropDown_Replace_StrobeQuantization->Selected_Value,
					StrobeColor
				);

				// Create the strobe event with the strobe info
				NewEvent = gcnew BarEvent(
					Event->ContainingTrack,
					Event->StartTick,
					Event->Duration,
					StrobeInfo
				);
			}

			// Add command to create the new event
			if (NewEvent != nullptr)
			{
				AddBarCommand^ AddCmd = gcnew AddBarCommand(_Timeline, Event->ContainingTrack, NewEvent);
				CompoundCmd->AddCommand(AddCmd);
			}
		}

		if (CompoundCmd->GetCommandCount() > 0) {
			_Timeline->CommandManager()->ExecuteCommand(CompoundCmd);
		}
    }
}
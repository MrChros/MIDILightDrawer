#pragma once

#include "Widget_Timeline.h"
#include "Widget_Timeline_Classes.h"
#include "Widget_Audio_Waveform.h"
#include "Playback_Audio_Engine.h"

#include "Control_GroupBox.h"
#include "Control_DropDown.h"
#include "Control_ComboBox.h"
#include "Control_ToggleSwitch.h"
#include "Control_Trackbar_Range.h"
#include "Control_CheckedListBox.h"
#include "Control_ExpandablePanel.h"
#include "Control_ScrollablePanel.h"
#include "Control_ProgressBar.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	// Forward declarations
	ref class Auto_Generate_Settings;
	ref class Audio_Analysis_Data;
	ref class Tablature_Analysis_Data;

	//////////////////////////////////
	// Audio Analysis Data Structures
	//////////////////////////////////
	// Frequency band definitions for spectral analysis
	public enum class Frequency_Band
	{
		Sub_Bass,		// 20-60 Hz (kick drum, bass fundamentals)
		Bass,			// 60-250 Hz (bass, low toms)
		Low_Mid,		// 250-500 Hz (low vocals, guitar body)
		Mid,			// 500-2000 Hz (vocals, snare, guitars)
		High_Mid,		// 2000-4000 Hz (vocal presence, hi-hat)
		High,			// 4000-8000 Hz (cymbals, brightness)
		Brilliance		// 8000-20000 Hz (air, sparkle)
	};

	public value struct Spectral_Energy
	{
		float Sub_Bass;			// 20-60 Hz
		float Bass;				// 60-250 Hz
		float Low_Mid;			// 250-500 Hz
		float Mid;				// 500-2000 Hz
		float High_Mid;			// 2000-4000 Hz
		float High;				// 4000-8000 Hz
		float Brilliance;		// 8000-20000 Hz
		float Total;			// Sum of all bands
		float Spectral_Centroid;	// "Brightness" measure
		float Spectral_Flux;		// Change from previous frame
	};

	public value struct Audio_Energy_Point
	{
		double Time_ms;
		float Energy;
		float Peak;
		bool Is_Beat;
		bool Is_Transient;
		bool Is_Onset;			// New note/sound onset detected
		Spectral_Energy Spectrum;
	};

	public value struct Audio_Segment
	{
		double Start_ms;
		double End_ms;
		float Average_Energy;
		float Peak_Energy;
		int Beat_Count;
		int Transient_Count;
		int Onset_Count;
		Spectral_Energy Average_Spectrum;
		float Rhythmic_Complexity;	// Measure of rhythmic variation
	};

	// Onset detection result
	public value struct Onset_Info
	{
		double Time_ms;
		int Tick;
		float Strength;
		Frequency_Band Dominant_Band;
	};

	public ref class Audio_Analysis_Data
	{
	private:
		Spectral_Energy _Global_Average_Spectrum;
		Spectral_Energy _Global_Peak_Spectrum;

	public:
		Audio_Analysis_Data();

		property List<Audio_Energy_Point>^ Energy_Points;
		property List<Audio_Segment>^ Segments;
		property List<Onset_Info>^ Onsets;
		property float Global_Average_Energy;
		property float Global_Peak_Energy;
		property double Total_Duration_ms;

		// Spectral statistics
		property Spectral_Energy^ Global_Average_Spectrum {
			Spectral_Energy^ get() { return %(this->_Global_Average_Spectrum); }
		};

		property Spectral_Energy^ Global_Peak_Spectrum {
			Spectral_Energy^ get() { return % (this->_Global_Peak_Spectrum); }
			void set(Spectral_Energy^ peak)
			{
				this->_Global_Peak_Spectrum.Sub_Bass = peak->Sub_Bass;
				this->_Global_Peak_Spectrum.Bass = peak->Bass;
				this->_Global_Peak_Spectrum.Low_Mid = peak->Low_Mid;
				this->_Global_Peak_Spectrum.Mid = peak->Mid;
				this->_Global_Peak_Spectrum.High_Mid = peak->High_Mid;
				this->_Global_Peak_Spectrum.High = peak->High;
				this->_Global_Peak_Spectrum.Brilliance = peak->Brilliance;
				this->_Global_Peak_Spectrum.Total = peak->Total;
				this->_Global_Peak_Spectrum.Spectral_Centroid = peak->Spectral_Centroid;
				this->_Global_Peak_Spectrum.Spectral_Flux = peak->Spectral_Flux;
			}
		};

		float GetEnergyAtTime(double time_ms);
		float GetNormalizedEnergyAtTime(double time_ms);
		bool IsTransientAtTime(double time_ms, double tolerance_ms);
		bool IsOnsetAtTime(double time_ms, double tolerance_ms);
		Spectral_Energy GetSpectrumAtTime(double time_ms);
		Frequency_Band GetDominantBandAtTime(double time_ms);
		List<Onset_Info>^ GetOnsetsInRange(double start_ms, double end_ms);
	};

	///////////////////////////////////////
	// Tablature Analysis Data Structures
	///////////////////////////////////////
	public value struct Tab_Event_Info
	{
		int Measure_Index;
		int Beat_Index;
		int Start_Tick;
		int Duration_Ticks;
		int Note_Count;
		int Average_Velocity;
		bool Has_Accented_Notes;
		bool Has_Palm_Mute;
		bool Has_Slide;
		bool Has_Bend;
		bool Has_Hammer;
		bool Is_Rest;
		bool Is_Chord;
	};

	public value struct Measure_Analysis
	{
		int Measure_Index;
		int Start_Tick;
		int Length_Ticks;
		int Tempo;
		int Numerator;
		int Denominator;
		int Total_Notes;
		int Total_Beats;
		float Note_Density;		// Notes per beat
		float Average_Velocity;
		bool Has_Marker;
		String^ Marker_Text;
	};

	public ref class Tablature_Analysis_Data
	{
	public:
		Tablature_Analysis_Data();

		property List<Tab_Event_Info>^ Events;
		property List<Measure_Analysis>^ Measures;
		property int Total_Notes;
		property int Total_Beats;
		property float Average_Note_Density;

		List<Tab_Event_Info>^ GetEventsInRange(int start_tick, int end_tick);
		Measure_Analysis GetMeasureAt(int tick);
	};

	//////////////////////////////////
	// Event Generation Types
	//////////////////////////////////
	public enum class Generation_Mode
	{
		Follow_Tablature,		// Generate based on tablature notes
		Follow_Audio_Energy,	// Generate based on audio energy levels
		Follow_Audio_Beats,		// Generate based on detected beats
		Combined,				// Combine tablature and audio analysis
		Pattern_Based			// Repeat patterns based on measure structure
	};

	public enum class Event_Distribution
	{
		Per_Note,				// One event per note/beat
		Per_Beat,				// One event per beat
		Per_Measure,			// One event per measure
		Custom_Interval			// Custom tick interval
	};

	public enum class Color_Mode
	{
		Single_Color,			// Use single color for all events
		Gradient_By_Time,		// Color gradient across song
		Gradient_By_Energy,		// Color based on energy level
		Alternate_Colors,		// Alternate between colors
		Random_From_Palette,	// Random selection from palette
		Map_To_Velocity			// Map velocity to color intensity
	};

	public enum class Event_Type_Mode
	{
		Solid_Only,				// Only create solid events
		Fade_On_Dynamics,		// Fades for dynamic sections
		Strobe_On_Fast,			// Strobe for fast/intense sections
		Mixed_Auto,				// Automatically choose based on analysis
		Custom_Rules			// User-defined rules
	};

	/////////////////////////////
	// Generation Settings Class
	/////////////////////////////
	public ref class Auto_Generate_Settings
	{
	public:
		Auto_Generate_Settings();

		// General Settings
		property Generation_Mode Mode;
		property Event_Distribution Distribution;
		property int Custom_Interval_Ticks;
		property bool Clear_Existing_Events;

		// Track Settings
		property bool Apply_To_All_Tracks;
		property List<int>^ Selected_Track_Indices;

		// Range Settings
		property bool Use_Full_Range;
		property int Start_Measure;
		property int End_Measure;

		// Color Settings
		property Color_Mode Color_Selection_Mode;
		property Color Primary_Color;
		property Color Secondary_Color;
		property Color Tertiary_Color;
		property List<Color>^ Color_Palette;

		// Event Type Settings
		property Event_Type_Mode Type_Selection_Mode;
		property float Energy_Threshold_Strobe;		// 0.0-1.0, above this = strobe
		property float Energy_Threshold_Fade;		// 0.0-1.0, above this = fade
		property int Strobe_Quantization_Ticks;
		property int Fade_Quantization_Ticks;

		// Fade Settings
		property FadeEasing Default_Ease_In;
		property FadeEasing Default_Ease_Out;

		// Audio Analysis Settings
		property float Beat_Detection_Sensitivity;	// 0.0-1.0
		property float Transient_Detection_Sensitivity;
		property bool Use_Audio_Energy;
		property bool Use_Audio_Beats;
		property bool Use_Spectral_Analysis;		// Enable FFT-based analysis
		property int FFT_Window_Size;				// 1024, 2048, 4096
		property float Onset_Detection_Threshold;	// Sensitivity for onset detection
		property bool Prefer_Bass_Events;			// Weight bass frequencies for event placement
		property bool Prefer_Percussion_Events;		// Weight mid/high for snare/cymbal detection

		// Tablature Settings
		property bool Use_Note_Velocity;
		property bool Use_Note_Effects;
		property bool Detect_Accents;
		property float Accent_Threshold;

		// Event Duration Settings
		property bool Auto_Duration;
		property int Fixed_Duration_Ticks;
		property float Duration_Scale_Factor;		// Multiplier for auto-calculated duration
		property int Minimum_Duration_Ticks;
		property int Maximum_Duration_Ticks;

		// Gap Handling
		property bool Fill_Gaps;
		property int Gap_Fill_Mode;					// 0=extend previous, 1=insert fade, 2=insert dark
		property int Minimum_Gap_Ticks;
	};

	//////////////////////////
	// Main Form Class
	//////////////////////////
	public ref class Form_Auto_Generate : public Form
	{
	public:
		Form_Auto_Generate(Widget_Timeline^ timeline, Playback_Audio_Engine^ audio_engine);
		virtual ~Form_Auto_Generate();

	private:
		Widget_Timeline^ _Timeline;
		Playback_Audio_Engine^ _Audio_Engine;
		Waveform_Render_Data^ _Waveform_Data;
		double _Audio_Duration_ms;

		Auto_Generate_Settings^ _Settings;
		Audio_Analysis_Data^ _Audio_Analysis;
		Tablature_Analysis_Data^ _Tab_Analysis;

		// UI Elements - Main Layout
		TableLayoutPanel^ _Main_Layout;
		Control_ScrollablePanel^ _Scrollable_Panel;

		// Mode Selection
		Control_GroupBox^ _GroupBox_Mode;
		Label^ _Label_Mode;
		Control_ComboBox^ _ComboBox_Mode;
		Label^ _Label_Distribution;
		Control_ComboBox^ _ComboBox_Distribution;
		Label^ _Label_Custom_Interval;
		TextBox^ _TextBox_Custom_Interval;

		// Track Selection
		Control_GroupBox^ _GroupBox_Tracks;
		Control_CheckedListBox^ _CheckedList_Tracks;
		CheckBox^ _CheckBox_All_Tracks;

		// Range Selection
		Control_GroupBox^ _GroupBox_Range;
		CheckBox^ _CheckBox_Full_Range;
		Label^ _Label_Range;
		Control_Trackbar_Range^ _TrackBar_Measure;
		TextBox^ _TextBox_Start_Measure;
		Label^ _Label_Range_To;
		TextBox^ _TextBox_End_Measure;

		// Color Settings
		Control_GroupBox^ _GroupBox_Colors;
		Label^ _Label_Color_Mode;
		Control_ComboBox^ _ComboBox_Color_Mode;
		PictureBox^ _PictureBox_Primary_Color;
		Button^ _Button_Primary_Color;
		PictureBox^ _PictureBox_Secondary_Color;
		Button^ _Button_Secondary_Color;
		PictureBox^ _PictureBox_Tertiary_Color;
		Button^ _Button_Tertiary_Color;

		// Event Type Settings
		Control_GroupBox^ _GroupBox_Event_Type;
		Label^ _Label_Event_Type_Mode;
		Control_ComboBox^ _ComboBox_Event_Type_Mode;
		Label^ _Label_Energy_Threshold_Strobe;
		Control_Trackbar_Range^ _TrackBar_Energy_Strobe;
		Label^ _Label_Energy_Strobe_Value;
		Label^ _Label_Energy_Threshold_Fade;
		Control_Trackbar_Range^ _TrackBar_Energy_Fade;
		Label^ _Label_Energy_Fade_Value;
		Label^ _Label_Strobe_Quantization;
		Control_DropDown^ _DropDown_Strobe_Quantization;
		Label^ _Label_Fade_Quantization;
		Control_DropDown^ _DropDown_Fade_Quantization;

		// Fade Settings
		Control_GroupBox^ _GroupBox_Fade_Settings;
		Label^ _Label_Ease_In;
		Control_DropDown^ _DropDown_Ease_In;
		Label^ _Label_Ease_Out;
		Control_DropDown^ _DropDown_Ease_Out;

		// Duration Settings
		Control_GroupBox^ _GroupBox_Duration;
		CheckBox^ _CheckBox_Auto_Duration;
		Label^ _Label_Fixed_Duration;
		TextBox^ _TextBox_Fixed_Duration;
		Label^ _Label_Duration_Scale;
		Control_Trackbar_Range^ _TrackBar_Duration_Scale;
		Label^ _Label_Duration_Scale_Value;
		Label^ _Label_Min_Duration;
		TextBox^ _TextBox_Min_Duration;
		Label^ _Label_Max_Duration;
		TextBox^ _TextBox_Max_Duration;

		// Audio Analysis Settings
		Control_GroupBox^ _GroupBox_Audio;
		CheckBox^ _CheckBox_Use_Audio_Energy;
		CheckBox^ _CheckBox_Use_Audio_Beats;
		Label^ _Label_Beat_Sensitivity;
		Control_Trackbar_Range^ _TrackBar_Beat_Sensitivity;
		Label^ _Label_Beat_Sensitivity_Value;
		Label^ _Label_Transient_Sensitivity;
		Control_Trackbar_Range^ _TrackBar_Transient_Sensitivity;
		Label^ _Label_Transient_Sensitivity_Value;

		// Options
		Control_GroupBox^ _GroupBox_Options;
		CheckBox^ _CheckBox_Clear_Existing;
		CheckBox^ _CheckBox_Fill_Gaps;
		Label^ _Label_Gap_Mode;
		Control_ComboBox^ _ComboBox_Gap_Mode;

		// Progress and Buttons
		Panel^ _Panel_Progress;
		Control_ProgressBar^ _ProgressBar;
		Label^ _Label_Status;
		Panel^ _Panel_Buttons;
		Button^ _Button_Preview;
		Button^ _Button_Generate;
		Button^ _Button_Cancel;

		// Internal state
		bool _Is_Generating;
		BackgroundWorker^ _Background_Worker;
		List<BarEvent^>^ _Preview_Events;

		// UI Initialization
		void InitializeComponent();
		void InitializeComponentModeSection();
		void InitializeComponentTrackSection();
		void InitializeComponentRangeSection();
		void InitializeComponentColorSection();
		void InitializeComponentEventTypeSection();
		void InitializeComponentFadeSection();
		void InitializeComponentDurationSection();
		void InitializeComponentAudioSection();
		void InitializeComponentOptionsSection();
		void InitializeComponentProgressAndButtons();
		void ApplyTheme();
		void PopulateDropDowns();
		void PopulateTrackList();

		// Event Handlers
		void OnModeSelection_Changed(System::Object^ sender, Control_ComboBox_Selection_Changed_Event_Args^ e);
		void OnDistributionSelection_Changed(System::Object^ sender, Control_ComboBox_Selection_Changed_Event_Args^ e);
		void OnColorModeSelection_Changed(System::Object^ sender, Control_ComboBox_Selection_Changed_Event_Args^ e);
		void OnEventTypeModeSelection_Changed(System::Object^ sender, Control_ComboBox_Selection_Changed_Event_Args^ e);
		void OnEnergy_FadeValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
		void OnEnergy_StrobeValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
		void OnAllTracksChanged(Object^ sender, EventArgs^ e);
		void OnFullRangeChanged(Object^ sender, EventArgs^ e);
		void OnMeasureValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
		void OnAutoDurationChanged(Object^ sender, EventArgs^ e);
		void OnDuration_ScaleValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
		void OnBeat_SensitivityValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
		void OnTransient_SensitivityValueChanged(System::Object^ sender, TrackbarRangeValueChangedEventArgs^ e);
		void OnPickColorClick(Object^ sender, EventArgs^ e);
		void OnPreviewClick(Object^ sender, EventArgs^ e);
		void OnGenerateClick(Object^ sender, EventArgs^ e);
		void OnCancelClick(Object^ sender, EventArgs^ e);
		void OnBackgroundWorkerDoWork(Object^ sender, DoWorkEventArgs^ e);
		void OnBackgroundWorkerProgressChanged(Object^ sender, ProgressChangedEventArgs^ e);
		void OnBackgroundWorkerCompleted(Object^ sender, RunWorkerCompletedEventArgs^ e);

		// Settings collection
		void CollectSettingsFromUI();
		void UpdateUIState();

		// Analysis methods
		void AnalyzeAudio();
		void AnalyzeTablature();

		// Generation methods
		List<BarEvent^>^ GenerateEvents();
		List<BarEvent^>^ GenerateFromTablature(Track^ track, int start_tick, int end_tick);
		List<BarEvent^>^ GenerateFromAudioEnergy(Track^ track, int start_tick, int end_tick);
		List<BarEvent^>^ GenerateFromAudioBeats(Track^ track, int start_tick, int end_tick);
		List<BarEvent^>^ GenerateCombined(Track^ track, int start_tick, int end_tick);
		List<BarEvent^>^ GeneratePatternBased(Track^ track, int start_tick, int end_tick);

		// Helper methods
		Color GetColorForEvent(double time_ratio, float energy, int event_index);
		Color GetColorForSpectrum(Spectral_Energy spectrum, double time_ratio, int event_index);
		BarEventType GetEventTypeForContext(float energy, float note_density, bool has_accent);
		BarEventType GetEventTypeForSpectrum(Spectral_Energy spectrum, float note_density);
		void ApplyGapFilling(List<BarEvent^>^ events, int start_tick, int end_tick);
		void ClearEventsInRange(Track^ track, int start_tick, int end_tick);

		// Overlap prevention
		void PreventOverlaps(List<BarEvent^>^ events);
		bool EventsOverlap(BarEvent^ a, BarEvent^ b);
		void ResolveOverlap(BarEvent^ existing, BarEvent^ new_event);

		// FFT and spectral analysis (native helpers)
		void PerformSpectralAnalysis();
		void PerformSpectralAnalysisWithRawSamples();
		array<float>^ GetMonoSamplesForAnalysis(double start_ms, double end_ms);
		void ComputeFFT(array<float>^ samples, int start_index, int window_size, array<float>^% magnitude_spectrum);
		Spectral_Energy CalculateBandEnergies(array<float>^ magnitude_spectrum, int sample_rate);
		float CalculateSpectralFlux(Spectral_Energy current, Spectral_Energy previous);
		float CalculateSpectralCentroid(array<float>^ magnitude_spectrum, int sample_rate);
		Frequency_Band GetDominantBand(Spectral_Energy spectrum);

		// Utility
		int MeasureToTick(int measure_index);
		int TickToMeasure(int tick);
	};
}
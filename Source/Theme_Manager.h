#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;

#include "Custom_Color_Table.h"

namespace MIDILightDrawer
{
	ref class Control_RadioButton;
		
	// Theme colors struct
	public value struct ThemeColors {
		Color Background;
		Color HeaderBackground;
		Color Text;
		Color MeasureLine;
		Color BeatLine;
		Color SubdivisionLine;
		Color TempoMarker;
		Color KeySignature;
		Color SelectionHighlight;
		Color TrackBackground;
		Color TrackBorder;
	};
	
	public ref class Theme_Manager {
	private:
		static Theme_Manager^ _Instance;

		Theme_Manager();

	public:
		// Primary Colors
		property Color Background			{ Color get() { return Color::FromArgb(255, 45, 45, 48		); } }	// Lighter gray for main background 
		property Color BackgroundAlt		{ Color get() { return Color::FromArgb(255, 37, 37, 40		); } }	// Slightly darker for contrast
		property Color BackgroundLight		{ Color get() { return Color::FromArgb(255, 50, 50, 53		); } }	// Lighter gray for certain elements
		property Color TimelineBackground	{ Color get() { return Color::FromArgb(255, 18, 18, 18		); } }	// Keep timeline background dark
		property Color ForegroundText		{ Color get() { return Color::FromArgb(255, 220, 220, 220	); } }	// Primary text color

		// Accent Colors
		property Color AccentPrimary	{ Color get() { return Color::FromArgb(255, 65, 105, 225); } }			// Primary accent (selection, focus)
		property Color AccentSecondary	{ Color get() { return Color::FromArgb(255, 120, 190, 255); } }			// Secondary accent
		property Color AccentWarning	{ Color get() { return Color::FromArgb(255, 255, 165, 0); } }			// Warning/attention color

		// Border and Line Colors
		property Color BorderPrimary	{ Color get() { return Color::FromArgb(255, 45, 45, 45); } }			// Primary border color
		property Color BorderStrong		{ Color get() { return Color::FromArgb(255, 90, 90, 90); } }			// Stronger border/line color

		// Timeline-specific Colors
		property Color TimelineMeasureLine		{ Color get() { return BorderStrong; } }						// Measure line color
		property Color TimelineBeatLine			{ Color get() { return Color::FromArgb(255, 60, 60, 60); } }	// Beat line color
		property Color TimelineSubdivisionLine	{ Color get() { return Color::FromArgb(255, 40, 40, 40); } }	// Subdivision line color

		// Grid Lines (various strengths)
		property Color GridStrong	{ Color get() { return BorderStrong; } }									// Strong grid lines
		property Color GridMedium	{ Color get() { return TimelineBeatLine; } }								// Medium grid lines
		property Color GridWeak		{ Color get() { return TimelineSubdivisionLine; } }							// Weak grid lines

		static Theme_Manager^ Get_Instance();

		// Timeline theme colors - now uses the centralized colors
		ThemeColors GetTimelineTheme();

		// Apply theme to a form
		void ApplyTheme(Form^ form);

		// Apply theme to menu strip
		void ApplyThemeToMenuStrip(MenuStrip^ menuStrip);

		// Apply theme to buttons
		void ApplyThemeToButton(Button^ button);
		void ApplyThemeToButton(Button^ button, Color backgroundColor);

		void ApplyThemeToRadioButton(Control_RadioButton^ radioButton);
		void ApplyThemeToRadioButton(Control_RadioButton^ radioButton, Color backgroundColor);

		// Apply theme to DataGridView
		void ApplyThemeToDataGridView(DataGridView^ grid);

		// Apply theme to Widget Timeline Contect Menu
		void ApplyThemeToContextMenu(ContextMenuStrip^ contextMenu);

	private:
		Dictionary<Button^, String^>^ _Button_Texts;

		void ApplyThemeToControls(Control::ControlCollection^ controls);
		void ApplyThemeToMenuItem(ToolStripItem^ item);

		void OnButtonPaint(Object^ sender, PaintEventArgs^ e);
		void OnButtonMouseEnter(Object^ sender, EventArgs^ e);
		void OnButtonMouseLeave(Object^ sender, EventArgs^ e);

		void OnRadioButtonPaint(Object^ sender, PaintEventArgs^ e);

		void OnDataGridViewScroll(Object^ sender, ScrollEventArgs^ e);
		void OnDataGridViewPaint(Object^ sender, PaintEventArgs^ e);

		void OnContextMenuOpening(Object^ sender, CancelEventArgs^ e);

		ProfessionalColorTable^ GetColorTable();
	};

}
#include "Theme_Manager.h"

#include "Widget_Timeline.h"
#include "Control_RadioButton.h"

namespace MIDILightDrawer
{
	Theme_Manager::Theme_Manager()
	{
		_Instance = nullptr;
		_Button_Texts = gcnew Dictionary<Button^, String^>();
	}

	Theme_Manager^ Theme_Manager::Get_Instance() {
		if (_Instance == nullptr) {
			_Instance = gcnew Theme_Manager();
		}
		return _Instance;
	}

	ThemeColors Theme_Manager::GetTimelineTheme()
	{
		ThemeColors theme;
		theme.Background			= Background;
		theme.HeaderBackground		= BackgroundAlt;
		theme.Text					= ForegroundText;
		theme.MeasureLine			= TimelineMeasureLine;
		theme.BeatLine				= TimelineBeatLine;
		theme.SubdivisionLine		= TimelineSubdivisionLine;
		theme.TempoMarker			= AccentWarning;
		theme.KeySignature			= AccentSecondary;
		theme.SelectionHighlight	= AccentPrimary;
		theme.TrackBackground		= BackgroundLight;
		theme.TrackBorder			= BorderPrimary;

		return theme;
	}

	void Theme_Manager::ApplyTheme(Form^ form)
	{
		form->BackColor = Background;
		form->ForeColor = ForegroundText;
		//ApplyThemeToControls(form->Controls);
	}

	// Apply theme to menu strip
	void Theme_Manager::ApplyThemeToMenuStrip(MenuStrip^ menuStrip)
	{
		menuStrip->BackColor = BackgroundAlt;
		menuStrip->ForeColor = ForegroundText;

		for each (ToolStripItem ^ item in menuStrip->Items) {
			ApplyThemeToMenuItem(item);
		}

		menuStrip->Renderer = gcnew ToolStripProfessionalRenderer(GetColorTable());
	}

	// Apply theme to buttons
	void Theme_Manager::ApplyThemeToButton(Button^ button)
	{
		ApplyThemeToButton(button, BackgroundAlt);
	}

	void Theme_Manager::ApplyThemeToButton(Button^ button, Color backgroundColor)
	{
		// Store the original text
		if (!_Button_Texts->ContainsKey(button)) {
			_Button_Texts->Add(button, button->Text);
		}
		else {
			_Button_Texts[button] = button->Text;
		}
		button->Text = "";
		
		// Basic button properties
		button->BackColor			= backgroundColor;  // Use provided background color
		button->ForeColor			= ForegroundText;
		button->FlatStyle			= FlatStyle::Flat;
		button->Font				= gcnew Drawing::Font("Segoe UI", 9, FontStyle::Regular);
		button->Padding				= Padding(6, 2, 6, 2);
		button->TextImageRelation	= TextImageRelation::Overlay;
		button->ImageAlign			= ContentAlignment::MiddleCenter;
		button->TextAlign			= ContentAlignment::MiddleCenter;
		button->UseVisualStyleBackColor = false;

		// Custom border and appearance
		button->FlatAppearance->BorderSize = 1;
		button->FlatAppearance->BorderColor = Color::FromArgb(70, 70, 70);

		// Calculate hover and pressed colors based on the background color
		button->FlatAppearance->MouseOverBackColor = Color::FromArgb(
			Math::Min(255, backgroundColor.R + 20),
			Math::Min(255, backgroundColor.G + 20),
			Math::Min(255, backgroundColor.B + 20)
		);

		button->FlatAppearance->MouseDownBackColor = Color::FromArgb(
			Math::Max(0, backgroundColor.R - 10),
			Math::Max(0, backgroundColor.G - 10),
			Math::Max(0, backgroundColor.B - 10)
		);

		// Custom paint handler for modern appearance
		button->Paint += gcnew PaintEventHandler(this, &Theme_Manager::OnButtonPaint);
		button->MouseEnter += gcnew EventHandler(this, &Theme_Manager::OnButtonMouseEnter);
		button->MouseLeave += gcnew EventHandler(this, &Theme_Manager::OnButtonMouseLeave);
	}

	void Theme_Manager::ApplyThemeToRadioButton(Control_RadioButton^ radioButton)
	{
		ApplyThemeToRadioButton(radioButton, BackgroundAlt);
	}

	void Theme_Manager::ApplyThemeToRadioButton(Control_RadioButton^ radioButton, Color backgroundColor)
	{
		// Basic styling
		radioButton->BackColor = backgroundColor;
		radioButton->ForeColor = ForegroundText;
		radioButton->FlatStyle = FlatStyle::Flat;
		radioButton->Font = gcnew Drawing::Font("Segoe UI", 9, FontStyle::Regular);

		// Border styling
		radioButton->FlatAppearance->BorderSize = 1;
		radioButton->FlatAppearance->BorderColor = Color::FromArgb(70, 70, 70);

		// Hover and pressed states
		radioButton->FlatAppearance->MouseOverBackColor = Color::FromArgb(
			Math::Min(255, backgroundColor.R + 20),
			Math::Min(255, backgroundColor.G + 20),
			Math::Min(255, backgroundColor.B + 20)
		);

		radioButton->FlatAppearance->MouseDownBackColor = Color::FromArgb(
			Math::Max(0, backgroundColor.R - 10),
			Math::Max(0, backgroundColor.G - 10),
			Math::Max(0, backgroundColor.B - 10)
		);

		// Add paint handler for consistent appearance with other buttons
		radioButton->Paint += gcnew PaintEventHandler(this, &Theme_Manager::OnRadioButtonPaint);
		radioButton->MouseEnter += gcnew EventHandler(this, &Theme_Manager::OnButtonMouseEnter);
		radioButton->MouseLeave += gcnew EventHandler(this, &Theme_Manager::OnButtonMouseLeave);
	}

	void Theme_Manager::ApplyThemeToDataGridView(DataGridView^ grid)
	{
		// Add scroll event handlers
		grid->Scroll += gcnew ScrollEventHandler(this, &Theme_Manager::OnDataGridViewScroll);
		grid->Paint += gcnew PaintEventHandler(this, &Theme_Manager::OnDataGridViewPaint);
		
		// Basic colors
		grid->BackgroundColor = Background;
		grid->GridColor = BorderPrimary;
		grid->BorderStyle = BorderStyle::None;

		// Default cell style
		DataGridViewCellStyle^ defaultStyle = gcnew DataGridViewCellStyle();
		defaultStyle->BackColor = Background;
		defaultStyle->ForeColor = ForegroundText;
		defaultStyle->SelectionBackColor = AccentPrimary;
		defaultStyle->SelectionForeColor = ForegroundText;
		defaultStyle->Font = gcnew Drawing::Font("Segoe UI", 9);
		grid->DefaultCellStyle = defaultStyle;

		// Column header style
		DataGridViewCellStyle^ headerStyle = gcnew DataGridViewCellStyle();
		headerStyle->BackColor = BackgroundAlt;
		headerStyle->ForeColor = ForegroundText;
		headerStyle->SelectionBackColor = BackgroundAlt;
		headerStyle->SelectionForeColor = ForegroundText;
		headerStyle->Font = gcnew Drawing::Font("Segoe UI Semibold", 9);
		headerStyle->Padding = Padding(10, 5, 10, 5);
		grid->ColumnHeadersDefaultCellStyle = headerStyle;
		grid->ColumnHeadersBorderStyle = DataGridViewHeaderBorderStyle::Single;
		grid->EnableHeadersVisualStyles = false;

		// Row header style (if visible)
		grid->RowHeadersDefaultCellStyle = defaultStyle;

		// ComboBox column style
		for each (DataGridViewColumn ^ column in grid->Columns)
		{
			if (DataGridViewComboBoxColumn^ comboColumn = dynamic_cast<DataGridViewComboBoxColumn^>(column))
			{
				DataGridViewComboBoxCell^ cell = dynamic_cast<DataGridViewComboBoxCell^>(comboColumn->CellTemplate);
				if (cell != nullptr)
				{
					// Style for the combo box cells
					cell->FlatStyle = FlatStyle::Flat;

					DataGridViewCellStyle^ comboStyle = gcnew DataGridViewCellStyle();
					comboStyle->BackColor = BackgroundLight;
					comboStyle->ForeColor = ForegroundText;
					comboStyle->SelectionBackColor = AccentPrimary;
					comboStyle->SelectionForeColor = ForegroundText;

					comboColumn->DefaultCellStyle = comboStyle;
				}
			}
		}

		// Alternate row color
		grid->AlternatingRowsDefaultCellStyle->BackColor = BackgroundAlt;
		grid->AlternatingRowsDefaultCellStyle->SelectionBackColor = AccentPrimary;
		grid->AlternatingRowsDefaultCellStyle->SelectionForeColor = ForegroundText;
	}

	void Theme_Manager::ApplyThemeToContextMenu(ContextMenuStrip^ contextMenu)
	{
		// Set basic properties
		contextMenu->BackColor = BackgroundAlt;
		contextMenu->ForeColor = ForegroundText;
		contextMenu->RenderMode = ToolStripRenderMode::Professional;

		// Use the custom renderer
		contextMenu->Renderer = gcnew ToolStripProfessionalRenderer(GetColorTable());

		// Apply theme to all items
		for each (ToolStripItem ^ item in contextMenu->Items) {
			ApplyThemeToMenuItem(item);
		}

		// Handle opening event to ensure proper styling of dynamically added items
		contextMenu->Opening += gcnew CancelEventHandler(this, &Theme_Manager::OnContextMenuOpening);
	}

	void Theme_Manager::ApplyThemeToControls(Control::ControlCollection^ controls)
	{
		for each (Control ^ control in controls) {
			// Base properties for all controls
			control->BackColor = Background;
			control->ForeColor = ForegroundText;

			if (Button^ button = dynamic_cast<Button^>(control)) {
				ApplyThemeToButton(button);
			}
			else if (TextBox^ textBox = dynamic_cast<TextBox^>(control)) {
				textBox->BackColor = BackgroundLight;
				textBox->BorderStyle = BorderStyle::FixedSingle;
			}
			else if (ComboBox^ comboBox = dynamic_cast<ComboBox^>(control)) {
				comboBox->BackColor = BackgroundLight;
				comboBox->FlatStyle = FlatStyle::Flat;
			}

			// Handle child controls
			if (control->Controls->Count > 0) {
				ApplyThemeToControls(control->Controls);
			}
		}
	}

	void Theme_Manager::ApplyThemeToMenuItem(ToolStripItem^ item)
	{
		item->BackColor = BackgroundAlt;
		item->ForeColor = ForegroundText;

		if (ToolStripMenuItem^ menuItem = dynamic_cast<ToolStripMenuItem^>(item))
		{
			for each (ToolStripItem ^ subItem in menuItem->DropDownItems) {
				ApplyThemeToMenuItem(subItem);
			}
		}
	}

	void Theme_Manager::OnButtonPaint(Object^ sender, PaintEventArgs^ e)
	{
		Button^ B = safe_cast<Button^>(sender);
		Graphics^ G = e->Graphics;
		Rectangle Rect = B->ClientRectangle;

		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		G->InterpolationMode = Drawing2D::InterpolationMode::HighQualityBicubic;

		bool IsHotOrFocused = B->Focused || B->ClientRectangle.Contains(B->PointToClient(Control::MousePosition));

		// Create gradient based on button's actual background color
		Drawing2D::LinearGradientBrush^ GradientBrush;
		if (IsHotOrFocused)
		{
			GradientBrush = gcnew Drawing2D::LinearGradientBrush(Rect, Color::FromArgb(255, Math::Min(255, B->BackColor.R + 15), Math::Min(255, B->BackColor.G + 15), Math::Min(255, B->BackColor.B + 15)), B->BackColor, Drawing2D::LinearGradientMode::Vertical);
		}
		else
		{
			GradientBrush = gcnew Drawing2D::LinearGradientBrush(Rect, Color::FromArgb(255, Math::Min(255, B->BackColor.R + 5), Math::Min(255, B->BackColor.G + 5), Math::Min(255, B->BackColor.B + 5)), B->BackColor, Drawing2D::LinearGradientMode::Vertical);
		}

		// Draw button background
		//SolidBrush^ backgroundBrush = gcnew SolidBrush(button->BackColor);
		//g->FillRectangle(backgroundBrush, rect);
		//g->FillRectangle(gradientBrush, rect);

		int CenterX = Rect.Width / 2;
		int CenterY = Rect.Height / 2;

		// Not sure why that here is needed
		/*
		if (button->Image != nullptr)
		{
			int imageX = centerX - (button->Image->Width / 2);
			int imageY = centerY - (button->Image->Height / 2);
			g->DrawImage(button->Image, imageX, imageY);
		}
		*/

		if(System::String::IsNullOrEmpty(B->Text) == false)
		{ 
			if (!_Button_Texts->ContainsKey(B)) {
				_Button_Texts->Add(B, B->Text);
			}
			else {
				_Button_Texts[B] = B->Text;
			}
			B->Text = "";
		}

		String^ Button_Text;
		if (_Button_Texts->TryGetValue(B, Button_Text))
		{
			if (Button_Text != nullptr && Button_Text->Length > 0)
			{
				// Measure text to center it
				SizeF TextSize = G->MeasureString(Button_Text, B->Font);
				float TextX = CenterX - (TextSize.Width / 2);
				float TextY = CenterY - (TextSize.Height / 2);

				if (B->Enabled)
				{
					// Draw shadow
					G->DrawString(Button_Text, B->Font, gcnew SolidBrush(Color::FromArgb(50, 0, 0, 0)), TextX + 1, TextY + 1);

					// Draw text
					G->DrawString(Button_Text, B->Font, gcnew SolidBrush(B->ForeColor), TextX, TextY);
				}
				else
				{
					G->DrawString(Button_Text, B->Font, gcnew SolidBrush(Color::FromArgb(120, B->ForeColor)), TextX, TextY);
				}
			}
		}

		Pen^ ShadowPen = gcnew Pen(Color::FromArgb(20, 0, 0, 0));
		G->DrawLine(ShadowPen, 1, 1, Rect.Width - 2, 1);

		// Draw border
		if (B->Focused)
		{
			Pen^ FocusPen = gcnew Pen(Color::FromArgb(180, AccentPrimary));
			G->DrawRectangle(FocusPen, 0, 0, Rect.Width - 1, Rect.Height - 1);
			delete FocusPen;
		}
		else
		{
			Pen^ BorderPen = gcnew Pen(B->FlatAppearance->BorderColor);
			G->DrawRectangle(BorderPen, 0, 0, Rect.Width - 1, Rect.Height - 1);
			delete BorderPen;
		}

		delete GradientBrush;
		delete ShadowPen;
	}

	void Theme_Manager::OnButtonMouseEnter(Object^ sender, EventArgs^ e)
	{
		Button^ button = safe_cast<Button^>(sender);
		button->Invalidate(); // Force repaint for hover effect
	}

	void Theme_Manager::OnButtonMouseLeave(Object^ sender, EventArgs^ e)
	{
		Button^ button = safe_cast<Button^>(sender);
		button->Invalidate(); // Force repaint to remove hover effect
	}

	void Theme_Manager::OnRadioButtonPaint(Object^ sender, PaintEventArgs^ e)
	{
		Control_RadioButton^ RB = safe_cast<Control_RadioButton^>(sender);
		Graphics^ G = e->Graphics;
		Rectangle Rect = RB->ClientRectangle;

		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		G->InterpolationMode = Drawing2D::InterpolationMode::HighQualityBicubic;

		// Create gradient background
		bool IsHotOrFocused = RB->Focused || RB->ClientRectangle.Contains(RB->PointToClient(Control::MousePosition));

		Drawing2D::LinearGradientBrush^ GradientBrush;
		if (IsHotOrFocused)
		{
			GradientBrush = gcnew Drawing2D::LinearGradientBrush(Rect, Color::FromArgb(255, Math::Min(255, RB->BackColor.R + 15), Math::Min(255, RB->BackColor.G + 15), Math::Min(255, RB->BackColor.B + 15)), RB->BackColor, Drawing2D::LinearGradientMode::Vertical);
		}
		else
		{
			GradientBrush = gcnew Drawing2D::LinearGradientBrush(Rect, Color::FromArgb(255, Math::Min(255, RB->BackColor.R + 5), Math::Min(255, RB->BackColor.G + 5), Math::Min(255, RB->BackColor.B + 5)), RB->BackColor, Drawing2D::LinearGradientMode::Vertical);
		}

		// Draw button background
		G->FillRectangle(GradientBrush, Rect);

		// Draw text centered
		String^ Text = RB->OptionText;
		if (!String::IsNullOrEmpty(Text))
		{
			int CenterX = Rect.Width / 2;
			int CenterY = Rect.Height / 2;

			SizeF TextSize = G->MeasureString(Text, RB->Font);
			float TextX = CenterX - (TextSize.Width / 2);
			float TextY = CenterY - (TextSize.Height / 2);

			// Draw text shadow and text
			if (RB->Enabled)
			{
				G->DrawString(Text, RB->Font, gcnew SolidBrush(Color::FromArgb(50, 0, 0, 0)), TextX + 1, TextY + 1);
				G->DrawString(Text, RB->Font, gcnew SolidBrush(RB->ForeColor), TextX, TextY);
			}
			else
			{
				G->DrawString(Text, RB->Font, gcnew SolidBrush(Color::FromArgb(120, RB->ForeColor)), TextX, TextY);
			}
		}

		// Draw tick icon if selected
		if (RB->Selected && RB->GetTickIcon() != nullptr)
		{
			Image^ TickIcon = RB->GetTickIcon();
			int Margin = 10; // Distance from left edge
			int IconY = (Rect.Height - TickIcon->Height) / 2;
			
			G->DrawImage(TickIcon, Margin, IconY);
		}

		// Draw top highlight
		Pen^ ShadowPen = gcnew Pen(Color::FromArgb(20, 0, 0, 0));
		G->DrawLine(ShadowPen, 1, 1, Rect.Width - 2, 1);

		// Draw border
		if (RB->Focused)
		{
			Pen^ FocusPen = gcnew Pen(Color::FromArgb(180, AccentPrimary));
			G->DrawRectangle(FocusPen, 0, 0, Rect.Width - 1, Rect.Height - 1);
			delete FocusPen;
		}
		else
		{
			Pen^ BorderPen = gcnew Pen(RB->FlatAppearance->BorderColor);
			G->DrawRectangle(BorderPen, 0, 0, Rect.Width - 1, Rect.Height - 1);
			delete BorderPen;
		}

		delete GradientBrush;
		delete ShadowPen;
	}

	void Theme_Manager::OnDataGridViewScroll(Object^ sender, ScrollEventArgs^ e)
	{
		DataGridView^ grid = safe_cast<DataGridView^>(sender);
		// Force redraw of the border by invalidating the control
		grid->Invalidate();
	}

	void Theme_Manager::OnDataGridViewPaint(Object^ sender, PaintEventArgs^ e)
	{
		DataGridView^ grid = safe_cast<DataGridView^>(sender);
		Graphics^ g = e->Graphics;

		// Draw border
		Rectangle bounds = grid->ClientRectangle;
		Pen^ borderPen = gcnew Pen(BorderStrong, 1);
		g->DrawRectangle(borderPen, 0, 0, bounds.Width - 1, bounds.Height - 1);

		// Optional: Draw accent line under header
		if (grid->ColumnHeadersVisible)
		{
			Pen^ accentPen = gcnew Pen(AccentPrimary, 1);
			g->DrawLine(accentPen, 0, grid->ColumnHeadersHeight, bounds.Width, grid->ColumnHeadersHeight);
			delete accentPen;
		}

		delete borderPen;
	}

	void Theme_Manager::OnContextMenuOpening(Object^ sender, CancelEventArgs^ e)
	{
		ContextMenuStrip^ menu = safe_cast<ContextMenuStrip^>(sender);
		for each (ToolStripItem ^ item in menu->Items) {
			ApplyThemeToMenuItem(item);
		}
	}

	ProfessionalColorTable^ Theme_Manager::GetColorTable()
	{
		Custom_Color_Table^ colorTable = gcnew Custom_Color_Table();

		// Now we can set the colors using the setter methods
		colorTable->SetMenuItemSelected(AccentPrimary);
		colorTable->SetMenuItemSelectedGradientBegin(AccentPrimary);
		colorTable->SetMenuItemSelectedGradientEnd(AccentPrimary);
		colorTable->SetMenuItemPressedGradientBegin(Background);
		colorTable->SetMenuItemPressedGradientEnd(Background);
		colorTable->SetMenuStripGradientBegin(BackgroundAlt);
		colorTable->SetMenuStripGradientEnd(BackgroundAlt);
		colorTable->SetMenuBorder(BorderPrimary);
		colorTable->SetMenuItemBorder(BorderPrimary);
		colorTable->SetToolStripDropDownBackground(BackgroundAlt);
		colorTable->SetImageMarginGradientBegin(BackgroundAlt);
		colorTable->SetImageMarginGradientEnd(BackgroundAlt);
		colorTable->SetImageMarginGradientMiddle(BackgroundAlt);

		return colorTable;
	}
}
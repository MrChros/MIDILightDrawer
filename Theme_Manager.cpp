#include "Theme_Manager.h"

#include "Widget_Timeline.h"

namespace MIDILightDrawer
{
	Theme_Manager::Theme_Manager()
	{
		_Instance = nullptr;
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
		ApplyThemeToControls(form->Controls);
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
		// Basic button properties
		button->BackColor = backgroundColor;  // Use provided background color
		button->ForeColor = ForegroundText;
		button->FlatStyle = FlatStyle::Flat;
		button->Font = gcnew Drawing::Font("Segoe UI", 9, FontStyle::Regular);
		button->Padding = Padding(6, 2, 6, 2);
		button->TextImageRelation = TextImageRelation::ImageBeforeText;
		button->ImageAlign = ContentAlignment::MiddleLeft;
		button->TextAlign = ContentAlignment::MiddleCenter;

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
		Button^ button = safe_cast<Button^>(sender);
		Graphics^ g = e->Graphics;
		Rectangle rect = button->ClientRectangle;

		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
		g->InterpolationMode = Drawing2D::InterpolationMode::HighQualityBicubic;

		// Create gradient based on button's actual background color
		Drawing2D::LinearGradientBrush^ gradientBrush;
		if (button->Focused || button->ClientRectangle.Contains(button->PointToClient(Control::MousePosition)))
		{
			gradientBrush = gcnew Drawing2D::LinearGradientBrush(
				rect,
				Color::FromArgb(255, Math::Min(255, button->BackColor.R + 15),
					Math::Min(255, button->BackColor.G + 15),
					Math::Min(255, button->BackColor.B + 15)),
				button->BackColor,
				Drawing2D::LinearGradientMode::Vertical
			);
		}
		else
		{
			gradientBrush = gcnew Drawing2D::LinearGradientBrush(
				rect,
				Color::FromArgb(255, Math::Min(255, button->BackColor.R + 5),
					Math::Min(255, button->BackColor.G + 5),
					Math::Min(255, button->BackColor.B + 5)),
				button->BackColor,
				Drawing2D::LinearGradientMode::Vertical
			);
		}

		// Draw button background
		g->FillRectangle(gradientBrush, rect);

		// Draw subtle inner shadow
		Pen^ shadowPen = gcnew Pen(Color::FromArgb(20, 0, 0, 0));
		g->DrawLine(shadowPen, 1, 1, rect.Width - 2, 1);

		// Draw border
		if (button->Focused)
		{
			Pen^ focusPen = gcnew Pen(Color::FromArgb(180, AccentPrimary));
			g->DrawRectangle(focusPen, 0, 0, rect.Width - 1, rect.Height - 1);
			delete focusPen;
		}
		else
		{
			Pen^ borderPen = gcnew Pen(button->FlatAppearance->BorderColor);
			g->DrawRectangle(borderPen, 0, 0, rect.Width - 1, rect.Height - 1);
			delete borderPen;
		}

		// Calculate text bounds
		Rectangle textBounds = rect;
		textBounds.Inflate(-4, -4);

		// Draw icon if present
		if (button->Image != nullptr)
		{
			int imageX = textBounds.Left + 4; // Add some padding
			int imageY = (rect.Height - button->Image->Height) / 2;
			g->DrawImage(button->Image, imageX, imageY);

			// Adjust text bounds to account for image
			int imageOffset = button->Image->Width + 8; // Image width plus padding
			textBounds.X += imageOffset;
			textBounds.Width -= imageOffset;
		}

		// Draw text with shadow effect
		if (button->Enabled)
		{
			g->DrawString(button->Text, button->Font,
				gcnew SolidBrush(Color::FromArgb(50, 0, 0, 0)),
				RectangleF(textBounds.X + 1, textBounds.Y + 1, textBounds.Width, textBounds.Height));

			g->DrawString(button->Text, button->Font,
				gcnew SolidBrush(button->ForeColor),
				textBounds);
		}
		else
		{
			g->DrawString(button->Text, button->Font,
				gcnew SolidBrush(Color::FromArgb(120, button->ForeColor)),
				textBounds);
		}

		delete gradientBrush;
		delete shadowPen;
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
#include "Control_RadioButton.h"

namespace MIDILightDrawer
{
	Control_RadioButton::Control_RadioButton()
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::ResizeRedraw, true);
		
		_IsSelected = false;
		_GroupId = 0;
		_OptionText = "";

		this->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
		this->FlatAppearance->BorderSize = 1;
		this->Text = "";  // Clear text - it'll be drawn by the Theme Manager

		_TickIcon = CreateTickIcon();

		this->Click += gcnew EventHandler(this, &Control_RadioButton::OnClick);

		ApplyTheme();
		UpdateAppearance();
	}

	System::Drawing::Image^ Control_RadioButton::GetTickIcon()
	{
		return this->_TickIcon;
	}

	void Control_RadioButton::ApplyTheme()
	{
		Theme_Manager^ ThemeManager = Theme_Manager::Get_Instance();
		ThemeManager->ApplyThemeToRadioButton(this, ThemeManager->BackgroundAlt);
	}

	void Control_RadioButton::UpdateAppearance()
	{
		this->Invalidate();
	}

	void Control_RadioButton::OnClick(Object^ sender, EventArgs^ e)
	{
		// Find all other radio buttons in the same group and deselect them
		Control^ parent = this->Parent;
		if (parent != nullptr)
		{
			for each (Control ^ control in parent->Controls)
			{
				Control_RadioButton^ radioButton = dynamic_cast<Control_RadioButton^>(control);
				if (radioButton != nullptr && radioButton != this && radioButton->GroupId == this->GroupId)
				{
					radioButton->Selected = false;
				}
			}
		}

		// Select this button
		this->Selected = true;
	}

	System::Drawing::Image^ Control_RadioButton::CreateTickIcon()
	{
		Bitmap^ IconBitmap = gcnew Bitmap(16, 16);

		Graphics^ G = Graphics::FromImage(IconBitmap);

		G->Clear(Color::Transparent);

		Color CheckColor = Color::FromArgb(255, 40, 190, 40); // Bright green color
		Pen^ CheckPen = gcnew Pen(CheckColor, 2.0f);

		// Draw the checkmark shape
		array<Point>^ Points = gcnew array<Point> {
			Point(3, 8),     // Start point
				Point(6, 12),    // Bottom point
				Point(13, 3)     // End point of tick
		};

		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		G->DrawLines(CheckPen, Points);

		delete CheckPen;
		delete G;

		return IconBitmap;
	}

	int Control_RadioButton::GroupId::get()
	{
		return _GroupId;
	}

	void Control_RadioButton::GroupId::set(int value)
	{
		_GroupId = value;
	}

	bool Control_RadioButton::Selected::get()
	{
		return _IsSelected;
	}

	void Control_RadioButton::Selected::set(bool value)
	{
		if (_IsSelected != value)
		{
			_IsSelected = value;
			UpdateAppearance();
			SelectedChanged(this, EventArgs::Empty);
		}
	}

	String^ Control_RadioButton::OptionText::get()
	{
		return _OptionText;
	}

	void Control_RadioButton::OptionText::set(String^ value)
	{
		_OptionText = value;
		UpdateAppearance();
	}
}
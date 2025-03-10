#include "Dialog_ColorPicker.h"

namespace MIDILightDrawer
{
	Dialog_ColorPicker::Dialog_ColorPicker(void)
	{
		InitializeComponent();
	}

	Dialog_ColorPicker::Dialog_ColorPicker(Color initialColor)
	{
		InitializeComponent();
		SelectedColor = initialColor;
	}

	Dialog_ColorPicker::~Dialog_ColorPicker()
	{
		if (_Components)
		{
			delete _Components;
		}
	}

	void Dialog_ColorPicker::InitializeComponent(void)
	{
		this->_Components = gcnew System::ComponentModel::Container();

		// Apply theme
		Theme_Manager^ ThemeManager = Theme_Manager::Get_Instance();

		// Initialize layout
		_MainLayout = gcnew TableLayoutPanel();
		_MainLayout->RowCount = 4;
		_MainLayout->ColumnCount = 3;
		_MainLayout->Dock = DockStyle::Fill;
		_MainLayout->Padding = System::Windows::Forms::Padding(10);
		_MainLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 60));
		_MainLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 25));
		_MainLayout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 15));
		_MainLayout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 15));
		_MainLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		_MainLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100));
		_MainLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));
		_MainLayout->BackColor = ThemeManager->Background;

		// Initialize color picker
		_ColorPicker = gcnew Control_ColorPicker();
		_ColorPicker->Dock = DockStyle::Fill;
		_ColorPicker->ColorChanged += gcnew EventHandler(this, &Dialog_ColorPicker::OnColorPickerChanged);

		// Initialize preset control
		_ColorPreset = gcnew Control_ColorPreset(true);
		_ColorPreset->Dock = DockStyle::Fill;
		_ColorPreset->SelectedColorChanged += gcnew EventHandler(this, &Dialog_ColorPicker::OnPresetColorChanged);

		// Preview panel
		_PreviewPanel = gcnew Panel();
		_PreviewPanel->Size = Drawing::Size(80, 30);
		_PreviewPanel->BorderStyle = BorderStyle::FixedSingle;
		_PreviewPanel->BackColor = Color::White;
		_PreviewPanel->Dock = DockStyle::Fill;
		_PreviewPanel->Margin = System::Windows::Forms::Padding(20, 10, 20, 10);

		_PreviewLabel = gcnew Label();
		_PreviewLabel->Text = "Preview";
		_PreviewLabel->AutoSize = true;
		_PreviewLabel->ForeColor = ThemeManager->ForegroundText;
		_PreviewLabel->Dock = DockStyle::Fill;
		_PreviewLabel->TextAlign = ContentAlignment::MiddleCenter;

		// OK Button
		_ButtonOK = gcnew Button();
		_ButtonOK->Text = "OK";
		_ButtonOK->DialogResult = System::Windows::Forms::DialogResult::OK;
		_ButtonOK->Dock = DockStyle::Fill;
		_ButtonOK->Margin = System::Windows::Forms::Padding(10, 35, 10, 10);
		ThemeManager->ApplyThemeToButton(_ButtonOK);

		// Cancel Button
		_ButtonCancel = gcnew Button();
		_ButtonCancel->Text = "Cancel";
		_ButtonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
		_ButtonCancel->Dock = DockStyle::Fill;
		_ButtonCancel->Margin = System::Windows::Forms::Padding(10, 35, 10, 10);
		ThemeManager->ApplyThemeToButton(_ButtonCancel);

		_MainLayout->Controls->Add(_ColorPicker, 0, 0);
		_MainLayout->SetColumnSpan(_ColorPicker, 3);
		_MainLayout->Controls->Add(_ColorPreset, 0, 1);
		_MainLayout->SetColumnSpan(_ColorPreset, 3);
		_MainLayout->Controls->Add(_PreviewLabel, 1, 2);
		_MainLayout->Controls->Add(_ButtonOK, 0, 3);
		_MainLayout->Controls->Add(_PreviewPanel, 1, 3);
		_MainLayout->Controls->Add(_ButtonCancel, 2, 3);
		
		this->Controls->Add(_MainLayout);

		// Form properties
		this->AcceptButton = _ButtonOK;
		this->CancelButton = _ButtonCancel;
		this->FormBorderStyle = Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->StartPosition = FormStartPosition::CenterParent;
		this->Text = "Color Picker";
		this->Size = Drawing::Size(490, 600);
		this->BackColor = ThemeManager->Background;
		this->ForeColor = ThemeManager->ForegroundText;

		// Apply theme to form
		ThemeManager->ApplyTheme(this);
	}

	// Event handlers
	void Dialog_ColorPicker::OnColorPickerChanged(Object^ sender, EventArgs^ e)
	{
		_PreviewPanel->BackColor = _ColorPicker->SelectedColor;
		_ColorPreset->SelectedColor = _ColorPicker->SelectedColor;
	}

	void Dialog_ColorPicker::OnPresetColorChanged(Object^ sender, EventArgs^ e)
	{
		_ColorPicker->SelectedColor = _ColorPreset->SelectedColor;
		_PreviewPanel->BackColor = _ColorPreset->SelectedColor;
	}

	Color Dialog_ColorPicker::SelectedColor::get() 
	{
		return _ColorPicker->SelectedColor; 
	}

	void Dialog_ColorPicker::SelectedColor::set(Color value)
	{
		_ColorPicker->SelectedColor = value;
		_PreviewPanel->BackColor = value;
	}

	Color Dialog_ColorPicker::ShowDialog(IWin32Window^ owner, Color initialColor)
	{
		Dialog_ColorPicker^ dialog = gcnew Dialog_ColorPicker(initialColor);
		if (dialog->ShowDialog(owner) == System::Windows::Forms::DialogResult::OK)
		{
			return dialog->SelectedColor;
		}
		return initialColor;
	}
}
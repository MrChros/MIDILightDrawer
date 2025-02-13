#include "Form_Loading.h"

namespace MIDILightDrawer
{
	FormLoading::FormLoading()
	{
		InitializeComponent();
		ApplyTheme();

		this->ActiveControl = nullptr;

		AddStatus("Initializing...");
	}

	void FormLoading::AddStatus(System::String^ status)
	{
		if (this->InvokeRequired)
		{
			this->BeginInvoke(gcnew Action<System::String^>(this, &FormLoading::AddStatus), status);
			return;
		}

		// Add bullet point and the new status
		_StatusListBox->Items->Add("• " + status);

		// Auto-scroll to the last item
		_StatusListBox->TopIndex = _StatusListBox->Items->Count - 1;
		_StatusListBox->Refresh();
		Application::DoEvents();
	}

	void FormLoading::OnLoadingStageChanged(LoadingStage stage)
	{
		switch (stage)
		{
		case LoadingStage::Images:			AddStatus("Loading Images...");				break;
		case LoadingStage::TabText:			AddStatus("Loading Tablature Texts...");	break;
		case LoadingStage::DrumSymbols:		AddStatus("Loading Drum Symbols...");		break;
		case LoadingStage::DurationSymbols:	AddStatus("Loading Duration Symbols...");	break;

		case LoadingStage::Complete:
			AddStatus("Loading complete.");

			Application::DoEvents();
			System::Threading::Thread::Sleep(500);
			this->Close();
			break;
		}
	}

	FormLoading::~FormLoading()
	{
		if (components)
		{
			delete components;
		}
	}

	void FormLoading::InitializeComponent(void)
	{
		this->_StatusListBox = gcnew System::Windows::Forms::ListBox();
		this->SuspendLayout();

		// Status ListBox
		this->_StatusListBox->FormattingEnabled = true;
		this->_StatusListBox->Location = System::Drawing::Point(20, 20);
		this->_StatusListBox->Name = L"_StatusListBox";
		this->_StatusListBox->Size = System::Drawing::Size(360, 140);
		this->_StatusListBox->TabIndex = 0;
		this->_StatusListBox->TabStop = false;
		this->_StatusListBox->SelectionMode = System::Windows::Forms::SelectionMode::None;
		this->_StatusListBox->IntegralHeight = false;
		this->_StatusListBox->Font = gcnew System::Drawing::Font("Segoe UI", 9);

		// Loading Dialog
		this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
		this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
		this->ClientSize = System::Drawing::Size(300, 180);
		this->ControlBox = false;
		this->Controls->Add(this->_StatusListBox);
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->Name = L"FromLoading";
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
		this->Text = L"Loading";
		this->TopMost = true;
		this->ResumeLayout(false);
		this->PerformLayout();
	}

	void FormLoading::ApplyTheme()
	{
		// Get theme manager instance
		Theme_Manager^ ThemeManager = Theme_Manager::Get_Instance();

		// Apply colors
		this->BackColor = ThemeManager->BackgroundAlt;
		this->ForeColor = ThemeManager->ForegroundText;

		// Apply to listbox
		_StatusListBox->BackColor = ThemeManager->BackgroundAlt;
		_StatusListBox->ForeColor = ThemeManager->ForegroundText;
		_StatusListBox->BorderStyle = System::Windows::Forms::BorderStyle::None;

		// Custom border
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
		this->Paint += gcnew PaintEventHandler(this, &FormLoading::OnFormPaint);
	}

	void FormLoading::OnFormPaint(Object^ sender, PaintEventArgs^ e)
	{
		// Draw border
		Rectangle FormRectangle = this->ClientRectangle;
		Theme_Manager^ ThemeManager = Theme_Manager::Get_Instance();
		Pen^ BorderPen = gcnew Pen(ThemeManager->BorderStrong);

		e->Graphics->DrawRectangle(BorderPen, 0, 0, FormRectangle.Width - 1, FormRectangle.Height - 1);

		delete BorderPen;
	}
}
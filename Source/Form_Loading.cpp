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

		if(_StatusListBox->Items->Count > 0) {
			_StatusListBox->Items->Add(" • " + status);
		}
		else {
			_StatusListBox->Items->Add(status);
		}

		// Auto-scroll to the last item
		_StatusListBox->TopIndex = _StatusListBox->Items->Count - 1;
		_StatusListBox->Refresh();
		Application::DoEvents();
	}

	void FormLoading::OnLoadingStageChanged(LoadingStage stage)
	{
		for (int i = 0; i < _ProgressBars->Length; i++)
		{
			if (_ProgressBars[i]->Value < 100)
				_ProgressBars[i]->Value = 0;
		}
		
		switch (stage)
		{
		case LoadingStage::Images:			AddStatus("Loading Images");			_CurrentProgressBar = _ImagesProgressBar;			break;
		case LoadingStage::TabText:			AddStatus("Loading Tablature Texts");	_CurrentProgressBar = _TabTextProgressBar;			break;
		case LoadingStage::DrumSymbols:		AddStatus("Loading Drum Symbols");		_CurrentProgressBar = _DrumSymbolsProgressBar;		break;
		case LoadingStage::DurationSymbols:	AddStatus("Loading Duration Symbols");	_CurrentProgressBar = _DurationSymbolsProgressBar;	break;

		case LoadingStage::Complete:
			AddStatus("Done");

			Application::DoEvents();
			System::Threading::Thread::Sleep(500);
			this->Close();
			break;
		}

		_CurrentProgressBar->Visible = true;
	}

	void FormLoading::UpdateProgress(float progress)
	{
		if (this->InvokeRequired)
		{
			this->BeginInvoke(gcnew Action<float>(this, &FormLoading::UpdateProgress), progress);
			return;
		}

		if (_CurrentProgressBar != nullptr)
		{
			int value = (int)(progress * 100);
			_CurrentProgressBar->Value = value;

			Application::DoEvents();
		}
	}

	FormLoading::~FormLoading()
	{
		if (_Components)
		{
			delete _Components;
		}
	}

	void FormLoading::InitializeComponent(void)
	{
		this->_StatusListBox = gcnew System::Windows::Forms::ListBox();
		this->_ImagesProgressBar = gcnew Control_ProgressBar();
		this->_TabTextProgressBar = gcnew Control_ProgressBar();
		this->_DrumSymbolsProgressBar = gcnew Control_ProgressBar();
		this->_DurationSymbolsProgressBar = gcnew Control_ProgressBar();
		this->SuspendLayout();

		// Status ListBox
		this->_StatusListBox->FormattingEnabled = true;
		this->_StatusListBox->Location = System::Drawing::Point(20, 20);
		this->_StatusListBox->Name = L"_StatusListBox";
		this->_StatusListBox->Size = System::Drawing::Size(160, 140);
		this->_StatusListBox->TabIndex = 0;
		this->_StatusListBox->TabStop = false;
		this->_StatusListBox->SelectionMode = System::Windows::Forms::SelectionMode::None;
		this->_StatusListBox->IntegralHeight = false;
		this->_StatusListBox->Font = gcnew System::Drawing::Font("Segoe UI", 9);

		// Progress Bars
		InitializeProgressBar(_ImagesProgressBar, 41);
		InitializeProgressBar(_TabTextProgressBar, 56);
		InitializeProgressBar(_DrumSymbolsProgressBar, 71);
		InitializeProgressBar(_DurationSymbolsProgressBar, 86);

		// Store progress bars in array for easy access
		_ProgressBars = gcnew array<Control_ProgressBar^>
		{
			_ImagesProgressBar,
			_TabTextProgressBar,
			_DrumSymbolsProgressBar,
			_DurationSymbolsProgressBar
		};

		// Loading Dialog
		this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
		this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
		this->ClientSize = System::Drawing::Size(360, 180);
		this->ControlBox = false;
		this->Controls->Add(this->_StatusListBox);
		this->Controls->Add(this->_ImagesProgressBar);
		this->Controls->Add(this->_TabTextProgressBar);
		this->Controls->Add(this->_DrumSymbolsProgressBar);
		this->Controls->Add(this->_DurationSymbolsProgressBar);
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

	void FormLoading::InitializeProgressBar(Control_ProgressBar^ progressBar, int yPosition)
	{
		progressBar->Location = System::Drawing::Point(180, yPosition);
		progressBar->Name = L"progressBar";
		progressBar->Size = System::Drawing::Size(150, 5);
		progressBar->Style = System::Windows::Forms::ProgressBarStyle::Continuous;
		progressBar->Value = 0;
		progressBar->Visible = false;
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

		// Apply to progress bars
		for each (System::Windows::Forms::ProgressBar^ bar in _ProgressBars)
		{
			bar->BackColor = ThemeManager->BackgroundAlt;
		}

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
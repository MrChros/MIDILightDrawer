#include "Form_Loading.h"

namespace MIDILightDrawer
{
	FormLoading::FormLoading()
	{
		InitializeComponent();
		ApplyTheme();

		this->ActiveControl = nullptr;

		_CurrentIndex = -1;

		AddStatus("Initializing...");
	}

	void FormLoading::AddStatus(System::String^ status)
	{
		if (this->InvokeRequired)
		{
			this->BeginInvoke(gcnew Action<System::String^>(this, &FormLoading::AddStatus), status);
			return;
		}

		_CurrentIndex++;

		if (_CurrentIndex > 0) {
			_Labels[_CurrentIndex]->Text = " • " + status;
		}
		else {
			_Labels[_CurrentIndex]->Text =status;
		}

		Application::DoEvents();
	}

	void FormLoading::OnLoadingStageChanged(LoadingStage stage)
	{
		for (int i = 0; i < _ProgressBars->Length; i++)
		{
			if (_ProgressBars[i] != nullptr && _ProgressBars[i]->Value < 100)
				_ProgressBars[i]->Value = 0;
		}
		
		switch (stage)
		{
		case LoadingStage::Images:			AddStatus("Loading Images");			_CurrentProgressBar = _ProgressBars[_CurrentIndex];	break;
		case LoadingStage::TabText:			AddStatus("Loading Tablature Texts");	_CurrentProgressBar = _ProgressBars[_CurrentIndex];	break;
		case LoadingStage::DrumSymbols:		AddStatus("Loading Drum Symbols");		_CurrentProgressBar = _ProgressBars[_CurrentIndex];	break;
		case LoadingStage::DurationSymbols:	AddStatus("Loading Duration Symbols");	_CurrentProgressBar = _ProgressBars[_CurrentIndex];	break;

		case LoadingStage::Complete:
			AddStatus("Done");

			Application::DoEvents();
			System::Threading::Thread::Sleep(500);
			this->Close();
			break;
		}

		if (_CurrentProgressBar != nullptr) {
			_CurrentProgressBar->Visible = true;
		}
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
		_TableLayout_Form = gcnew TableLayoutPanel();
		_TableLayout_Form->Dock = DockStyle::Fill;
		_TableLayout_Form->ColumnCount = 2;
		_TableLayout_Form->RowCount = NUM_ENTRIES;
		_TableLayout_Form->CellBorderStyle = TableLayoutPanelCellBorderStyle::None;

		for (int i = 0; i < _TableLayout_Form->RowCount; i++) {
			_TableLayout_Form->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f  / _TableLayout_Form->RowCount));
		}

		_TableLayout_Form->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50.0f));
		_TableLayout_Form->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50.0f));
		

		this->SuspendLayout();

		_ProgressBars = gcnew array<Control_ProgressBar^>
		{
			nullptr,
			gcnew Control_ProgressBar(),
			gcnew Control_ProgressBar(),
			gcnew Control_ProgressBar(),
			gcnew Control_ProgressBar(),
			nullptr
		};

		_Labels = gcnew array<Label^>
		{
			gcnew Label(),
			gcnew Label(),
			gcnew Label(),
			gcnew Label(),
			gcnew Label(),
			gcnew Label()
		};

		for (int i = 0; i < NUM_ENTRIES; i++)
		{
			InitializeProgressBar(_ProgressBars[i], i);
			InitializeLabel(_Labels[i], i);
		}

		// Loading Dialog
		this->ClientSize = System::Drawing::Size(360, 180);
		this->ControlBox = false;
		this->Controls->Add(this->_TableLayout_Form);
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->Name = L"FromLoading";
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
		this->Text = L"Loading";
		this->TopMost = true;
		this->Padding = System::Windows::Forms::Padding(20, 20, 20, 40);
		this->ResumeLayout(false);
		this->PerformLayout();
	}

	void FormLoading::InitializeProgressBar(Control_ProgressBar^ progressBar, int row)
	{
		if (progressBar == nullptr) {
			return;
		}

		progressBar->Dock = DockStyle::Fill;
		progressBar->Name = L"progressBar";
		progressBar->Height = 5;
		progressBar->Style = System::Windows::Forms::ProgressBarStyle::Continuous;
		progressBar->Value = 0;
		progressBar->Visible = false;
		progressBar->Margin = System::Windows::Forms::Padding(2, 8, 2, 8);
		_TableLayout_Form->Controls->Add(progressBar, 1, row);
	}

	void  FormLoading::InitializeLabel(Label^ label, int row)
	{
		label->Dock = DockStyle::Fill;
		label->TextAlign = ContentAlignment::MiddleLeft;
		_TableLayout_Form->Controls->Add(label, 0, row);
	}

	void FormLoading::ApplyTheme()
	{
		Theme_Manager^ ThemeManager = Theme_Manager::Get_Instance();

		this->BackColor = ThemeManager->BackgroundAlt;
		this->ForeColor = ThemeManager->ForegroundText;

		for each (System::Windows::Forms::ProgressBar^ bar in _ProgressBars)
		{
			if (bar == nullptr) {
				continue;
			}

			bar->BackColor = ThemeManager->BackgroundAlt;
		}

		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
		this->Paint += gcnew PaintEventHandler(this, &FormLoading::OnFormPaint);
	}

	void FormLoading::OnFormPaint(Object^ sender, PaintEventArgs^ e)
	{
		Rectangle FormRectangle = this->ClientRectangle;
		Pen^ BorderPen = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong);

		e->Graphics->DrawRectangle(BorderPen, 0, 0, FormRectangle.Width - 1, FormRectangle.Height - 1);

		delete BorderPen;
	}
}
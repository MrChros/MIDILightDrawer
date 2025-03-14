#include "Form_Light_Import.h"

namespace MIDILightDrawer
{
	Form_Light_Import::Form_Light_Import(List<LightTrackInfo^>^ availableTracks)
	{
		InitializeComponent();

		// Apply theme
		this->BackColor = Theme_Manager::Get_Instance()->Background;
		ApplyTheme();

		_AvailableTracks = availableTracks;
		_SelectedTracks = gcnew List<LightTrackInfo^>();

		PopulateTrackList();
	}

	Form_Light_Import::~Form_Light_Import()
	{
		if (components)
		{
			delete components;
		}
	}

	void Form_Light_Import::InitializeComponent(void)
	{
		this->components = gcnew System::ComponentModel::Container();
		this->_GroupBoxTracks = gcnew Control_GroupBox();
		this->_CheckListTracks = gcnew Control_CheckedListBox();
		this->_ButtonOk = gcnew System::Windows::Forms::Button();
		this->_ButtonCancel = gcnew System::Windows::Forms::Button();
		this->_ButtonSelectAll = gcnew System::Windows::Forms::Button();
		this->_ButtonSelectNone = gcnew System::Windows::Forms::Button();
		this->_GroupBoxTracks->SuspendLayout();
		this->SuspendLayout();

		// GroupBoxTracks
		this->_GroupBoxTracks->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
			| System::Windows::Forms::AnchorStyles::Left)
			| System::Windows::Forms::AnchorStyles::Right));
		this->_GroupBoxTracks->Controls->Add(this->_CheckListTracks);
		this->_GroupBoxTracks->Location = System::Drawing::Point(12, 12);
		this->_GroupBoxTracks->Name = L"_GroupBoxTracks";
		this->_GroupBoxTracks->Size = System::Drawing::Size(460, 308);
		this->_GroupBoxTracks->TabIndex = 0;
		this->_GroupBoxTracks->TabStop = false;
		this->_GroupBoxTracks->Text = L"Select Tracks to Import";

		// CheckListTracks
		this->_CheckListTracks->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
			| System::Windows::Forms::AnchorStyles::Left)
			| System::Windows::Forms::AnchorStyles::Right));
		//this->_CheckListTracks->FormattingEnabled = true;
		this->_CheckListTracks->Location = System::Drawing::Point(6, 40); // Adjusted for Control_GroupBox
		this->_CheckListTracks->Name = L"_CheckListTracks";
		this->_CheckListTracks->Size = System::Drawing::Size(448, 259); // Adjusted size
		this->_CheckListTracks->TabIndex = 0;

		// ButtonOk
		this->_ButtonOk->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
		this->_ButtonOk->DialogResult = System::Windows::Forms::DialogResult::OK;
		this->_ButtonOk->Location = System::Drawing::Point(397, 326);
		this->_ButtonOk->Name = L"_ButtonOk";
		this->_ButtonOk->Size = System::Drawing::Size(75, 23);
		this->_ButtonOk->TabIndex = 1;
		this->_ButtonOk->Text = L"OK";
		this->_ButtonOk->UseVisualStyleBackColor = true;
		this->_ButtonOk->Click += gcnew System::EventHandler(this, &Form_Light_Import::ButtonOk_Click);

		// ButtonCancel
		this->_ButtonCancel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
		this->_ButtonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
		this->_ButtonCancel->Location = System::Drawing::Point(316, 326);
		this->_ButtonCancel->Name = L"_ButtonCancel";
		this->_ButtonCancel->Size = System::Drawing::Size(75, 23);
		this->_ButtonCancel->TabIndex = 2;
		this->_ButtonCancel->Text = L"Cancel";
		this->_ButtonCancel->UseVisualStyleBackColor = true;

		// ButtonSelectAll
		this->_ButtonSelectAll->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
		this->_ButtonSelectAll->Location = System::Drawing::Point(12, 326);
		this->_ButtonSelectAll->Name = L"_ButtonSelectAll";
		this->_ButtonSelectAll->Size = System::Drawing::Size(75, 23);
		this->_ButtonSelectAll->TabIndex = 3;
		this->_ButtonSelectAll->Text = L"Select All";
		this->_ButtonSelectAll->UseVisualStyleBackColor = true;
		this->_ButtonSelectAll->Click += gcnew System::EventHandler(this, &Form_Light_Import::ButtonSelectAll_Click);

		// ButtonSelectNone
		this->_ButtonSelectNone->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
		this->_ButtonSelectNone->Location = System::Drawing::Point(93, 326);
		this->_ButtonSelectNone->Name = L"_ButtonSelectNone";
		this->_ButtonSelectNone->Size = System::Drawing::Size(75, 23);
		this->_ButtonSelectNone->TabIndex = 4;
		this->_ButtonSelectNone->Text = L"Select None";
		this->_ButtonSelectNone->UseVisualStyleBackColor = true;
		this->_ButtonSelectNone->Click += gcnew System::EventHandler(this, &Form_Light_Import::ButtonSelectNone_Click);

		// Form_Light_Import
		this->AcceptButton = this->_ButtonOk;
		this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
		this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
		this->CancelButton = this->_ButtonCancel;
		this->ClientSize = System::Drawing::Size(484, 361);
		this->Controls->Add(this->_ButtonSelectNone);
		this->Controls->Add(this->_ButtonSelectAll);
		this->Controls->Add(this->_ButtonCancel);
		this->Controls->Add(this->_ButtonOk);
		this->Controls->Add(this->_GroupBoxTracks);
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->MinimumSize = System::Drawing::Size(400, 300);
		this->Name = L"Form_Light_Import";
		this->ShowIcon = false;
		this->ShowInTaskbar = false;
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
		this->Text = L"Import Light Tracks";
		this->_GroupBoxTracks->ResumeLayout(false);
		this->ResumeLayout(false);
	}

	void Form_Light_Import::ApplyTheme()
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		// Apply theme to form
		this->BackColor = Theme->Background;
		this->ForeColor = Theme->ForegroundText;

		// Apply theme to all buttons
		Theme->ApplyThemeToButton(_ButtonOk);
		Theme->ApplyThemeToButton(_ButtonCancel);
		Theme->ApplyThemeToButton(_ButtonSelectAll);
		Theme->ApplyThemeToButton(_ButtonSelectNone);
	}

	void Form_Light_Import::PopulateTrackList()
	{
		_CheckListTracks->ClearItems();

		// Add each track to the checklist
		for (int i = 0; i < _AvailableTracks->Count; i++)
		{
			LightTrackInfo^ Track = _AvailableTracks[i];
			CheckedListItem^ Item = gcnew CheckedListItem(Track->Name, true, Track);
			Item->Subtitle = Track->EventCount + " events";
			_CheckListTracks->AddItem(Item);
		}
	}

	void Form_Light_Import::ButtonOk_Click(System::Object^ sender, System::EventArgs^ e)
	{
		_SelectedTracks->Clear();

		// Add selected tracks to the result list
		for (int i = 0; i < _CheckListTracks->Items->Count; i++)
		{
			if (_CheckListTracks->GetItemChecked(i))
			{
				_SelectedTracks->Add(_AvailableTracks[i]);
			}
		}

		this->DialogResult = System::Windows::Forms::DialogResult::OK;
		this->Close();
	}

	void Form_Light_Import::ButtonSelectAll_Click(System::Object^ sender, System::EventArgs^ e)
	{
		for (int i = 0; i < _CheckListTracks->Items->Count; i++)
		{
			_CheckListTracks->SetItemChecked(i, true);
		}
	}

	void Form_Light_Import::ButtonSelectNone_Click(System::Object^ sender, System::EventArgs^ e)
	{
		for (int i = 0; i < _CheckListTracks->Items->Count; i++)
		{
			_CheckListTracks->SetItemChecked(i, false);
		}
	}

	List<LightTrackInfo^>^ Form_Light_Import::SelectedTracks::get()
	{ 
		return _SelectedTracks;
	}
}
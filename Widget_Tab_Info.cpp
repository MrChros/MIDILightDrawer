#include "pch.h"
#include "Widget_Tab_Info.h"


MIDILightDrawer::Widget_Tab_Info::Widget_Tab_Info(void)
{
	TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
	Table_Layout_Main->Dock = DockStyle::Fill;

	Table_Layout_Main->RowCount = 4;
	Table_Layout_Main->ColumnCount = 2;

	Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 90));
	Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 50.0f));
	
	for(auto i=0;i< Table_Layout_Main->RowCount;i++) {
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));
	}
	
	Table_Layout_Main->Controls->Add(Create_Text_Label("Song Name:")	, 0, 0);
	Table_Layout_Main->Controls->Add(Create_Text_Label("Count Meaures:"), 0, 1);
	Table_Layout_Main->Controls->Add(Create_Text_Label("Count Tracks:")	, 0, 2);


	this->_Label_Song_Name = Create_Text_Label("");
	Table_Layout_Main->Controls->Add(this->_Label_Song_Name, 1, 0);

	this->_Label_Measures_Count = Create_Text_Label("");
	Table_Layout_Main->Controls->Add(this->_Label_Measures_Count, 1, 1);

	this->_Label_Tracks_Count = Create_Text_Label("");
	Table_Layout_Main->Controls->Add(this->_Label_Tracks_Count, 1, 2);

	this->Controls->Add(Table_Layout_Main);
}

MIDILightDrawer::Widget_Tab_Info::~Widget_Tab_Info(void)
{

}

void MIDILightDrawer::Widget_Tab_Info::Update_Info(String^ song_name, UInt32 count_measures, UInt32 count_tracks)
{
	this->_Label_Song_Name->Text		= song_name;
	if (song_name->Length == 0) {
		this->_Label_Song_Name->Text = "<n/a>";
		this->_Label_Song_Name->Font = gcnew System::Drawing::Font(this->_Label_Song_Name->Font->FontFamily, this->_Label_Song_Name->Font->Size, FontStyle::Italic);
	}
	else {
		this->_Label_Song_Name->Font = gcnew System::Drawing::Font(this->_Label_Song_Name->Font->FontFamily, this->_Label_Song_Name->Font->Size, FontStyle::Regular);
	}

	this->_Label_Measures_Count->Text	= count_measures.ToString();
	this->_Label_Tracks_Count->Text		= count_tracks.ToString();
}

Label^ MIDILightDrawer::Widget_Tab_Info::Create_Text_Label(String^ text)
{
	Label^ Return_Label = gcnew Label();
	Return_Label->Dock = DockStyle::Fill;
	Return_Label->Text = text;

	return Return_Label;
}
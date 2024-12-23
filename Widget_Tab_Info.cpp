#include "pch.h"
#include "Widget_Tab_Info.h"

namespace MIDILightDrawer
{
	Widget_Tab_Info::Widget_Tab_Info(void)
	{
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->Dock = DockStyle::Fill;

		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 2;

		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 400));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
	
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 50.0));

		this->_Label_Song_Name		= Create_Text_Label("");
		this->_Label_File_Path		= Create_Text_Label("");
		this->_Label_Measures_Count = Create_Text_Label("");
		this->_Label_Tracks_Count	= Create_Text_Label("");
	
		Table_Layout_Main->Controls->Add(this->_Label_Song_Name		, 0, 0);
		Table_Layout_Main->Controls->Add(this->_Label_File_Path		, 0, 1);
		Table_Layout_Main->Controls->Add(this->_Label_Measures_Count, 1, 0);
		Table_Layout_Main->Controls->Add(this->_Label_Tracks_Count	, 1, 1);

		this->Controls->Add(Table_Layout_Main);

		this->BackColor = Color::Transparent;
		this->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;

		this->_Song_Name = "untitled";
	}

	Widget_Tab_Info::~Widget_Tab_Info(void)
	{

	}

	void Widget_Tab_Info::Update_Info(String^ file_path, String^ song_name, UInt32 count_measures, UInt32 count_tracks)
	{
		if (song_name->Length == 0)
		{
			String^ filename = System::IO::Path::GetFileName(file_path);

			// Remove extension if present
			if (filename->Contains(".")) {
				song_name = filename->Substring(0, filename->LastIndexOf("."));
			}
			else {
				song_name = filename;
			}
		}

		this->_Song_Name = song_name;

		this->_Label_Song_Name->Text		= "Song Title: " + song_name;
		this->_Label_File_Path->Text		= file_path;
		this->_Label_Measures_Count->Text	= "Measures: "	+ count_measures.ToString();
		this->_Label_Tracks_Count->Text		= "Tracks: "	+ count_tracks.ToString();
	}

	String^ Widget_Tab_Info::Get_Song_Name(void)
	{
		return this->_Song_Name;
	}

	Label^ Widget_Tab_Info::Create_Text_Label(String^ text)
	{
		Label^ Return_Label = gcnew Label();
		Return_Label->Dock = DockStyle::Fill;
		Return_Label->Text = text;

		return Return_Label;
	}
}
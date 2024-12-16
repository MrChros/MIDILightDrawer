#include "pch.h"
#include "Widget_Tools_And_Control.h"

namespace MIDILightDrawer
{

	Widget_Tools_And_Control::Widget_Tools_And_Control(void) : System::Windows::Forms::UserControl()
	{
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->Dock = DockStyle::Fill;

		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 2;

		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 60));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 300.0f));


		this->_Toolbar = gcnew Widget_Toolbar();
		this->_Toolbar->Dock = DockStyle::Fill;
		this->_Toolbar->ToolChanged += gcnew System::EventHandler<MIDILightDrawer::Widget_Toolbar::ToolType>(this, &MIDILightDrawer::Widget_Tools_And_Control::OnToolbar_ModeChanged);
		Table_Layout_Main->Controls->Add(this->_Toolbar, 0, 0);


		this->_Color_Picker = gcnew Control_ColorPicker();
		this->_Color_Picker->Dock = DockStyle::Fill;
		Table_Layout_Main->Controls->Add(this->_Color_Picker, 1, 0);
		Table_Layout_Main->SetRowSpan(this->_Color_Picker, 2);


		// Create options container
		this->_Options_Container = gcnew TableLayoutPanel();
		this->_Options_Container->Dock = DockStyle::Fill;
		this->_Options_Container->RowCount = 1;
		this->_Options_Container->ColumnCount = 1;
		this->_Options_Container->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
		this->_Options_Container->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->Controls->Add(this->_Options_Container, 0, 1);

		// Initialize option panels
		this->_Draw_Options = gcnew Widget_Draw_Options(this->_Color_Picker);
		this->_Draw_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Draw_Options, 0, 0);

		// Initialize Fade options (you'll need to create this class)
		this->_Fade_Options = gcnew Widget_Fade_Options; // Replace with your actual Fade options control
		this->_Fade_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Fade_Options, 0, 0);

		// Initialize option panels
		this->_Length_Options = gcnew Widget_Length_Options();
		this->_Length_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Length_Options, 0, 0);

		this->_Color_Options = gcnew Widget_Color_Options(this->_Color_Picker);
		this->_Color_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Color_Options, 0, 0);

		this->_Bucket_Options = gcnew Widget_Bucket_Options(this->_Color_Picker);
		this->_Bucket_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Bucket_Options, 0, 0);

		// Set initial visibility based on default tool
		UpdateOptionsVisibility(this->_Toolbar->Get_Current_Tool());


		this->Controls->Add(Table_Layout_Main);
	}

	void Widget_Tools_And_Control::Select_Color_From_Preset(int color_index)
	{
		Widget_Toolbar::ToolType Current_Tool = _Toolbar->Get_Current_Tool();

		switch (Current_Tool)
		{
		case MIDILightDrawer::Widget_Toolbar::ToolType::Draw:			_Draw_Options->PresetColor = color_index;	break;
		case MIDILightDrawer::Widget_Toolbar::ToolType::Change_Color:	_Color_Options->PresetColor = color_index;	break;
		case MIDILightDrawer::Widget_Toolbar::ToolType::Bucket_Fill:	_Bucket_Options->PresetColor = color_index;	break;

		case MIDILightDrawer::Widget_Toolbar::ToolType::Fade:
			break;

		case MIDILightDrawer::Widget_Toolbar::ToolType::Selection:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Erase:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Duration:
		default:
			break;
		}
	}

	void Widget_Tools_And_Control::Quantization_Up(void)
	{
		Widget_Toolbar::ToolType Current_Tool = _Toolbar->Get_Current_Tool();

		switch (Current_Tool)
		{
		case MIDILightDrawer::Widget_Toolbar::ToolType::Draw:			_Draw_Options->Select_Next_Draw_Value();		break;
		case MIDILightDrawer::Widget_Toolbar::ToolType::Duration:	_Length_Options->Select_Next_Length_Value();	break;
			
		case MIDILightDrawer::Widget_Toolbar::ToolType::Fade:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Selection:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Erase:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Change_Color:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Bucket_Fill:
		default:
			break;
		}
	}

	void Widget_Tools_And_Control::Quantization_Down(void)
	{
		Widget_Toolbar::ToolType Current_Tool = _Toolbar->Get_Current_Tool();

		switch (Current_Tool)
		{
		case MIDILightDrawer::Widget_Toolbar::ToolType::Draw:			_Draw_Options->Select_Previous_Draw_Value();		break;
		case MIDILightDrawer::Widget_Toolbar::ToolType::Duration:	_Length_Options->Select_Previous_Length_Value();	break;

		case MIDILightDrawer::Widget_Toolbar::ToolType::Fade:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Selection:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Erase:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Change_Color:
		case MIDILightDrawer::Widget_Toolbar::ToolType::Bucket_Fill:
		default:
			break;
		}
	}

	Widget_Toolbar^ Widget_Tools_And_Control::Get_Widget_Toolbar(void)
	{
		return this->_Toolbar;
	}

	Widget_Draw_Options^ Widget_Tools_And_Control::Get_Widget_Draw_Options(void)
	{
		return this->_Draw_Options;
	}

	Widget_Fade_Options^ Widget_Tools_And_Control::Get_Widget_Fade_Options(void)
	{
		return this->_Fade_Options;
	}

	Widget_Length_Options^ Widget_Tools_And_Control::Get_Widget_Length_Options(void)
	{
		return this->_Length_Options;
	}

	Widget_Color_Options^ Widget_Tools_And_Control::Get_Widget_Color_Options(void)
	{
		return this->_Color_Options;
	}

	Widget_Bucket_Options^ Widget_Tools_And_Control::Get_Widget_Bucket_Options(void)
	{
		return this->_Bucket_Options;
	}

	Widget_Tools_And_Control::~Widget_Tools_And_Control()
	{

	}

	void Widget_Tools_And_Control::UpdateOptionsVisibility(Widget_Toolbar::ToolType tool)
	{
		// Hide all option panels first
		this->_Draw_Options->Visible = false;
		this->_Fade_Options->Visible = false;
		this->_Length_Options->Visible = false;
		this->_Color_Options->Visible = false;
		this->_Bucket_Options->Visible = false;

		// Show the appropriate panel based on the selected tool
		switch (tool)
		{
		case Widget_Toolbar::ToolType::Draw:
			this->_Draw_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;
		case Widget_Toolbar::ToolType::Fade:
			this->_Fade_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;
		case Widget_Toolbar::ToolType::Selection:
		case Widget_Toolbar::ToolType::Erase:
			// No options to show for these tools
			this->_Color_Picker->Enabled = false;
			break;

		case Widget_Toolbar::ToolType::Duration: 
			this->_Length_Options->Visible = true;
			this->_Color_Picker->Enabled = false;
			break;

		case Widget_Toolbar::ToolType::Change_Color:
			this->_Color_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;

		case Widget_Toolbar::ToolType::Bucket_Fill:
			this->_Bucket_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;
		}
	}

	void Widget_Tools_And_Control::OnToolbar_ModeChanged(System::Object^ sender, MIDILightDrawer::Widget_Toolbar::ToolType e)
	{
		UpdateOptionsVisibility(e);
	}
}
#include "Widget_Tools_And_Control.h"

namespace MIDILightDrawer
{

	Widget_Tools_And_Control::Widget_Tools_And_Control(void) : System::Windows::Forms::UserControl()
	{
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->Dock = DockStyle::Fill;

		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 4;

		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 60));
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 350));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 450));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 400));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));


		// Create and add toolbar
		this->_Toolbar = gcnew Widget_Toolbar();
		this->_Toolbar->Dock = DockStyle::Fill;
		this->_Toolbar->OnToolChanged += gcnew System::EventHandler<TimelineToolType>(this, &Widget_Tools_And_Control::Toolbar_OnToolChanged);
		Table_Layout_Main->Controls->Add(this->_Toolbar, 0, 0);


		// Create and add Tab Info Widget
		this->_Tab_Info = gcnew Widget_Tab_Info();
		this->_Tab_Info->Dock = DockStyle::Fill;
		this->_Tab_Info->Margin = System::Windows::Forms::Padding(0, 10, 0, 0);
		Table_Layout_Main->Controls->Add(this->_Tab_Info, 1, 0);


		// Create and add color picker
		this->_Color_Picker = gcnew Control_ColorPicker();
		this->_Color_Picker->Dock = DockStyle::Fill;
		Table_Layout_Main->Controls->Add(this->_Color_Picker, 2, 0);
		Table_Layout_Main->SetRowSpan(this->_Color_Picker, 2);


		// Create options container
		this->_Options_Container = gcnew TableLayoutPanel();
		this->_Options_Container->Dock = DockStyle::Fill;
		this->_Options_Container->RowCount = 1;
		this->_Options_Container->ColumnCount = 1;
		this->_Options_Container->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
		this->_Options_Container->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
		Table_Layout_Main->Controls->Add(this->_Options_Container, 0, 1);
		Table_Layout_Main->SetColumnSpan(this->_Options_Container, 2);


		this->_Pointer_Options = gcnew Widget_Pointer_Options();
		this->_Pointer_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Pointer_Options, 0, 0);

		this->_Draw_Options = gcnew Widget_Draw_Options(this->_Color_Picker);
		this->_Draw_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Draw_Options, 0, 0);
		
		this->_Length_Options = gcnew Widget_Length_Options();
		this->_Length_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Length_Options, 0, 0);

		this->_Color_Options = gcnew Widget_Color_Options(this->_Color_Picker);
		this->_Color_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Color_Options, 0, 0);

		this->_Fade_Options = gcnew Widget_Fade_Options(this->_Color_Picker);
		this->_Fade_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Fade_Options, 0, 0);

		this->_Strobe_Options = gcnew Widget_Strobe_Options(this->_Color_Picker);
		this->_Strobe_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Strobe_Options, 0, 0);

		this->_Bucket_Options = gcnew Widget_Bucket_Options(this->_Color_Picker);
		this->_Bucket_Options->Dock = DockStyle::Fill;
		this->_Options_Container->Controls->Add(this->_Bucket_Options, 0, 0);

		// Set initial visibility based on default tool
		UpdateOptionsVisibility(this->_Toolbar->CurrentTool);

		this->Controls->Add(Table_Layout_Main);
	}

	void Widget_Tools_And_Control::SelectColorFromPreset(int color_index)
	{
		switch (_Toolbar->CurrentTool)
		{
		case TimelineToolType::Draw:		_Draw_Options->PresetColor		= color_index;	break;
		case TimelineToolType::Color:		_Color_Options->PresetColor		= color_index;	break;
		case TimelineToolType::Strobe:		_Strobe_Options->PresetColor	= color_index;	break;
		//case TimelineToolType::Bucket_Fill:	_Bucket_Options->PresetColor = color_index;	break;

		default:
			break;
		}
	}

	void Widget_Tools_And_Control::SetColorDirect(Color color)
	{
		_Color_Picker->SelectedColor = color;
	}

	void Widget_Tools_And_Control::SnappingUp(void)
	{
		_Pointer_Options->Select_Pointer_Snapping_Next();
		_Draw_Options->Select_Draw_Snapping_Next();
	}

	void Widget_Tools_And_Control::SnappingDown(void)
	{
		_Pointer_Options->Select_Pointer_Snapping_Previous();
		_Draw_Options->Select_Draw_Snapping_Previous();
	}

	void Widget_Tools_And_Control::SnapTo(int index)
	{
		_Pointer_Options->PointerSnapping = index;
		_Draw_Options->DrawSnapping = index;
	}

	void Widget_Tools_And_Control::LengthUp(void)
	{
		_Pointer_Options->Select_Next_ResizeQuantization_Value();
		_Draw_Options->Select_Draw_Length_Next();
		_Length_Options->Select_Next_Length_Value();
		_Fade_Options->Select_Next_Fade_Value();
		_Strobe_Options->Select_Next_Strobe_Value();
	}

	void Widget_Tools_And_Control::LengthDown(void)
	{
		_Pointer_Options->Select_Previous_ResizeQuantization_Value();
		_Draw_Options->Select_Draw_Length_Previous();
		_Length_Options->Select_Previous_Length_Value();
		_Fade_Options->Select_Previous_Fade_Value();
		_Strobe_Options->Select_Previous_Strobe_Value();

	}

	bool Widget_Tools_And_Control::ColorPickerIsTyping()
	{
		return _Color_Picker->IsTyping;
	}

	Widget_Toolbar^ Widget_Tools_And_Control::Get_Widget_Toolbar(void)
	{
		return this->_Toolbar;
	}

	Widget_Tab_Info^ Widget_Tools_And_Control::Get_Widget_Tab_Info(void)
	{
		return this->_Tab_Info;
	}

	Widget_Pointer_Options^	Widget_Tools_And_Control::Get_Widget_Pointer_Options(void)
	{
		return this->_Pointer_Options;
	}

	Widget_Draw_Options^ Widget_Tools_And_Control::Get_Widget_Draw_Options(void)
	{
		return this->_Draw_Options;
	}

	Widget_Length_Options^ Widget_Tools_And_Control::Get_Widget_Length_Options(void)
	{
		return this->_Length_Options;
	}

	Widget_Color_Options^ Widget_Tools_And_Control::Get_Widget_Color_Options(void)
	{
		return this->_Color_Options;
	}

	Widget_Fade_Options^ Widget_Tools_And_Control::Get_Widget_Fade_Options(void)
	{
		return this->_Fade_Options;
	}

	Widget_Strobe_Options^ Widget_Tools_And_Control::Get_Widget_Strobe_Options(void)
	{
		return this->_Strobe_Options;
	}

	Widget_Bucket_Options^ Widget_Tools_And_Control::Get_Widget_Bucket_Options(void)
	{
		return this->_Bucket_Options;
	}

	Color Widget_Tools_And_Control::GetColorPickerSelectedColor(void)
	{
		return this->_Color_Picker->SelectedColor;
	}

	Widget_Tools_And_Control::~Widget_Tools_And_Control()
	{

	}

	void Widget_Tools_And_Control::UpdateOptionsVisibility(TimelineToolType tool)
	{
		// Hide all option panels first
		this->_Pointer_Options->Visible	= false;
		this->_Draw_Options->Visible	= false;
		this->_Length_Options->Visible	= false;
		this->_Color_Options->Visible	= false;
		this->_Fade_Options->Visible	= false;
		this->_Strobe_Options->Visible	= false;
		this->_Bucket_Options->Visible	= false;

		// Show the appropriate panel based on the selected tool
		switch (tool)
		{
		case TimelineToolType::Draw:
			this->_Draw_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;

		case TimelineToolType::Split:
			break;

		case TimelineToolType::Pointer:
			this->_Pointer_Options->Visible = true;
			this->_Color_Picker->Enabled = false;
			break;

		case TimelineToolType::Erase:
			// No options to show for these tools
			this->_Color_Picker->Enabled = false;
			break;

		case TimelineToolType::Duration:
			this->_Length_Options->Visible = true;
			this->_Color_Picker->Enabled = false;
			break;

		case TimelineToolType::Color:
			this->_Color_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;

		case TimelineToolType::Fade:
			this->_Fade_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;

		case TimelineToolType::Strobe:
			this->_Strobe_Options->Visible = true;
			this->_Color_Picker->Enabled = true;
			break;

		//case TimelineToolType::Bucket_Fill:
		//	this->_Bucket_Options->Visible = true;
		//	this->_Color_Picker->Enabled = true;
			//break;
		}
	}

	void Widget_Tools_And_Control::Toolbar_OnToolChanged(System::Object^ sender, TimelineToolType e)
	{
		UpdateOptionsVisibility(e);
	}
}
#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

#include "Theme_Manager.h"

namespace MIDILightDrawer {

	public ref class Control_DropDown_Item_Selected_Event_Args : public EventArgs {
	private:
		int _Selected_Index;
		int _Value;

	public:
		Control_DropDown_Item_Selected_Event_Args(int selected_index, int value) {
			_Selected_Index = selected_index;
			_Value = value;
		}

		property int Selected_Index {
			int get() { return _Selected_Index; }
		}

		property int Value {
			int get() { return _Value; }
		}
	};

	public delegate void Control_DropDown_Item_Selected_Event_Handler(Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
	
	public enum class Panel_Horizontal_Alignment {
		Left,
		Right
	};

	private ref class Double_Buffered_Panel : public Panel {
	public:
		Double_Buffered_Panel() {
			this->DoubleBuffered = true;
		}
	};
	
	public ref class Control_DropDown : public Control {
	public:
		// Constructor
		Control_DropDown();

	private:
		// Private members
		array<String^>^ _First_Lines;
		array<String^>^ _Second_Lines;
		array<int>^		_Values;

		array<Color>^	_First_Line_Colors;
		array<Color>^	_Second_Line_Colors;

		Double_Buffered_Panel^		_Drop_Down_Panel;
		Panel_Horizontal_Alignment	_Horizontal_Alignment;
		EventHandler^				_Mouse_Leave_Handler;
		Timer^						_Close_Timer;

		Rectangle	_Arrow_Bounds;
		String^		_Title_Text;

		Color _Title_Color;
		Color _Background_Color;
		Color _Border_Color;
		Color _Hover_Color;
		Color _Selected_Color;
		Color _Dropdown_Background;
		Color _Dropdown_Border;

		bool _Is_Dropped;
		bool _Open_Above;
			
		int _Selected_Index;
		int _Highlight_Index;
		int _Tile_Width;
		int _Tile_Height;
		int _Columns;
		int _Close_Delay_Ms;
	
		// Private methods
		void Initialize_Component();
		void Show_Drop_Down();
		void Close_Drop_Down(Object^ sender, EventArgs^ e);
		void Paint_Drop_Down(Object^ sender, PaintEventArgs^ e);
		void Draw_Item(Graphics^ g, int index, Rectangle bounds, bool is_highlighted, bool is_main_control);

		void Handle_Mouse_Move(Object^ sender, MouseEventArgs^ e);
		void Handle_Mouse_Click(Object^ sender, MouseEventArgs^ e);
		void Handle_Mouse_Leave(Object^ sender, EventArgs^ e);
		void Handle_Close_Timer_Tick(Object^ sender, EventArgs^ e);

		bool Is_Mouse_Over_Controls();
		void Update_Panel_Position();

		int Get_Row_Count();
		int Find_Index_By_Value(int value);

		Rectangle Get_Control_Screen_Bounds();
		Rectangle Get_Panel_Screen_Bounds();
		Rectangle Get_Tile_Bounds(int index);

		void Apply_Theme();

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseClick(MouseEventArgs^ e) override;
		virtual void OnSizeChanged(EventArgs^ e) override;

	public:
		void Set_Title_Color(Color color);
		void Set_Items(array<String^>^ first_lines, array<String^>^ second_lines, array<int>^ values);
		void Set_First_Line_Color(int index, Color color);
		void Set_Second_Line_Color(int index, Color color);
		void Set_Tile_Layout(int tile_width, int tile_height, int columns);
		void Set_Open_Direction(bool open_above);
		void Set_Horizontal_Alignment(Panel_Horizontal_Alignment alignment);
		bool Select_Next();
		bool Select_Previous();
		bool Select_By_Value(int value);
		void Clear();

		event Control_DropDown_Item_Selected_Event_Handler^ Item_Selected;

		property int Selected_Index {
			int get() { return _Selected_Index; }
			void set(int value);
		}

		property int Selected_Value {
			int get() {
				if(_Selected_Index >= 0 && _Selected_Index < _Values->Length) {
					return _Values[_Selected_Index];
				}

				return -1;
			}
		}

		property String^ Title_Text {
			String^ get() { return _Title_Text; }
			void set(String^ value) {
				_Title_Text = value;
				this->Invalidate();
			}
		}
	};

}
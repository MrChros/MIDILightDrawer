#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;

#include "Theme_Manager.h"

namespace MIDILightDrawer {

	// Event args for selection changed event
	public ref class Control_ComboBox_Selection_Changed_Event_Args : public EventArgs {
	private:
		int _Selected_Index;
		Object^ _Selected_Item;

	public:
		Control_ComboBox_Selection_Changed_Event_Args(int selected_index, Object^ selected_item) {
			_Selected_Index = selected_index;
			_Selected_Item = selected_item;
		}

		property int Selected_Index {
			int get() { return _Selected_Index; }
		}

		property Object^ Selected_Item {
			Object^ get() { return _Selected_Item; }
		}
	};

	public delegate void Control_ComboBox_Selection_Changed_Event_Handler(Object^ sender, Control_ComboBox_Selection_Changed_Event_Args^ e);
	public delegate void Control_ComboBox_Drop_Down_Event_Handler(Object^ sender, EventArgs^ e);

	// Double-buffered panel for smooth dropdown rendering
	private ref class ComboBox_Drop_Down_Panel : public Panel {
	public:
		ComboBox_Drop_Down_Panel() {
			this->DoubleBuffered = true;
			this->SetStyle(ControlStyles::AllPaintingInWmPaint, true);
			this->SetStyle(ControlStyles::OptimizedDoubleBuffer, true);
		}
	};

	public ref class Control_ComboBox : public Control {
	public:
		// Constructor
		Control_ComboBox();
		~Control_ComboBox();
		!Control_ComboBox();

	private:
		// Private members - Items
		List<Object^>^ _Items;
		int _Selected_Index;
		int _Highlight_Index;
		int _Max_Drop_Down_Items;
		int _Item_Height;
		int _Scroll_Offset;

		// Private members - State
		bool _Is_Dropped;
		bool _Is_Hovered;
		bool _Is_Focused;
		bool _Is_Mouse_Over_Arrow;
		bool _Enabled;

		// Private members - Style
		ComboBoxStyle _Drop_Down_Style;

		// Private members - Colors
		Color _Background_Color;
		Color _Background_Disabled_Color;
		Color _Border_Color;
		Color _Border_Focused_Color;
		Color _Arrow_Color;
		Color _Text_Color;
		Color _Text_Disabled_Color;
		Color _Dropdown_Background_Color;
		Color _Dropdown_Border_Color;
		Color _Hover_Color;
		Color _Selected_Color;

		// Private members - UI Elements
		ComboBox_Drop_Down_Panel^ _Drop_Down_Panel;
		VScrollBar^ _Scroll_Bar;
		Timer^ _Close_Timer;
		EventHandler^ _Mouse_Leave_Handler;
		Rectangle _Arrow_Bounds;
		TextBox^ _Edit_Box;

		// Private methods - Initialization
		void Initialize_Component();
		void Apply_Theme();

		// Private methods - Dropdown management
		void Show_Drop_Down();
		void Close_Drop_Down();
		void Update_Drop_Down_Position();
		void Update_Drop_Down_Size();
		void Update_Scroll_Bar();

		// Private methods - Drawing
		void Paint_Drop_Down(Object^ sender, PaintEventArgs^ e);
		void Draw_Item(Graphics^ g, int index, Rectangle bounds, bool is_highlighted, bool is_selected);
		void Draw_Arrow(Graphics^ g, Rectangle bounds, Color color);

		// Private methods - Event handlers
		void Handle_Drop_Down_Mouse_Move(Object^ sender, MouseEventArgs^ e);
		void Handle_Drop_Down_Mouse_Click(Object^ sender, MouseEventArgs^ e);
		void Handle_Drop_Down_Mouse_Leave(Object^ sender, EventArgs^ e);
		void Handle_Drop_Down_Mouse_Wheel(Object^ sender, MouseEventArgs^ e);
		void Handle_Close_Timer_Tick(Object^ sender, EventArgs^ e);
		void Handle_Scroll_Bar_Scroll(Object^ sender, ScrollEventArgs^ e);
		void Handle_Edit_Box_Text_Changed(Object^ sender, EventArgs^ e);
		void Handle_Edit_Box_Key_Down(Object^ sender, KeyEventArgs^ e);

		// Private methods - Utility
		bool Is_Mouse_Over_Controls();
		int Get_Item_At_Point(Point point);
		int Get_Visible_Item_Count();
		Rectangle Get_Item_Bounds(int visible_index);
		int Find_String_Start(String^ text);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseClick(MouseEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseEnter(EventArgs^ e) override;
		virtual void OnMouseLeave(EventArgs^ e) override;
		virtual void OnMouseWheel(MouseEventArgs^ e) override;
		virtual void OnGotFocus(EventArgs^ e) override;
		virtual void OnLostFocus(EventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;
		virtual void OnSizeChanged(EventArgs^ e) override;
		virtual void OnEnabledChanged(EventArgs^ e) override;

	public:
		// Events
		event Control_ComboBox_Selection_Changed_Event_Handler^ Selection_Changed;
		event Control_ComboBox_Drop_Down_Event_Handler^ Drop_Down_Opened;
		event Control_ComboBox_Drop_Down_Event_Handler^ Drop_Down_Closed;

		// Public methods - Item management
		void Add_Item(Object^ item);
		void Add_Items(array<Object^>^ items);
		void Insert_Item(int index, Object^ item);
		void Remove_Item(Object^ item);
		void Remove_At(int index);
		void Clear_Items();
		int Find_String(String^ text);
		int Find_String_Exact(String^ text);

		// Public methods - Selection
		void Select_Next();
		void Select_Previous();
		void Begin_Update();
		void End_Update();

		// Properties - Items
		property List<Object^>^ Items {
			List<Object^>^ get() { return _Items; }
		}

		property int Item_Count {
			int get() { return _Items->Count; }
		}

		property int Selected_Index {
			int get() { return _Selected_Index; }
			void set(int value);
		}

		property Object^ Selected_Item {
			Object^ get();
			void set(Object^ value);
		}

		property String^ Selected_Text {
			String^ get();
		}

		// Properties - Appearance
		property int Max_Drop_Down_Items {
			int get() { return _Max_Drop_Down_Items; }
			void set(int value);
		}

		property int Item_Height {
			int get() { return _Item_Height; }
			void set(int value);
		}

		property ComboBoxStyle Drop_Down_Style {
			ComboBoxStyle get() { return _Drop_Down_Style; }
			void set(ComboBoxStyle value);
		}

		property bool Is_Dropped_Down {
			bool get() { return _Is_Dropped; }
		}

		// Properties - Colors (for manual override if needed)
		property Color Background_Color {
			Color get() { return _Background_Color; }
			void set(Color value);
		}

		property Color Border_Color {
			Color get() { return _Border_Color; }
			void set(Color value);
		}

		property Color Text_Color {
			Color get() { return _Text_Color; }
			void set(Color value);
		}
	};

}
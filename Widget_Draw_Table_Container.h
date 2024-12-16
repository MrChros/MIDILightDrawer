#pragma once


#include "Control_ScrollBar.h"
#include "Widget_Draw_Table.h"


namespace MIDILightDrawer {

	public ref class Widget_Draw_Table_Container : public System::Windows::Forms::Panel
	{
	public:
		Widget_Draw_Table_Container();

	private:
		TableLayoutPanel^	_Table_Layout;
		Widget_Draw_Table^	_Draw_Table;
		Control_ScrollBar^	_ScrollBar;

		Point	_Last_Scroll_Position;
		bool	_User_Has_Scrolled;
		int		_Last_Centered_Measure;

		void	OnScrollPositionChanged(Object^ sender, int newPosition);
		void	OnDrawTableSizeChanged(Object^ sender, EventArgs^ e);
		int		FindCenteredMeasureColumn();
		int		GetMeasureNumberAtCenter();
		void	CenterOnMeasure(int measureNumber);

	protected:
		virtual void OnSizeChanged(System::EventArgs^ e) override;

	public:
		void SetDrawTable(Widget_Draw_Table^ table);
		void SetScale(double newScale);
		void UpdateLastCenteredMeasure();
		void UpdateVirtualScrollRange();
	};
}
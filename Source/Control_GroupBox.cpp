#include "Control_GroupBox.h"

#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	Control_GroupBox::Control_GroupBox()
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);
	}

	void Control_GroupBox::OnPaint(PaintEventArgs^ e)
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Calculate text size and positions
		Drawing::Font^ TitleFont = gcnew Drawing::Font("Segoe UI Semibold", 9.5f);
		SizeF TextSize = G->MeasureString(this->Text, TitleFont);

		// Define header rect
		Rectangle HeaderRect = Rectangle(0, 0, this->Width, 28);

		// Draw header background with gradient
		Drawing2D::LinearGradientBrush^ HeaderBrush = gcnew Drawing2D::LinearGradientBrush(HeaderRect, Theme->BackgroundAlt, Theme->Background, Drawing2D::LinearGradientMode::Vertical);

		G->FillRectangle(HeaderBrush, HeaderRect);

		// Draw title
		G->DrawString(this->Text, TitleFont, gcnew SolidBrush(Theme->ForegroundText), Point(12, 6));

		// Draw border
		Pen^ BorderPen = gcnew Pen(Theme->BorderStrong);
		G->DrawRectangle(BorderPen, 0, 0, this->Width - 1, this->Height - 1);

		// Draw header bottom line with accent
		Pen^ AccentPen = gcnew Pen(Theme->AccentPrimary, 1);
		G->DrawLine(AccentPen, 0, HeaderRect.Bottom, this->Width, HeaderRect.Bottom);

		// Clean up
		delete HeaderBrush;
		delete BorderPen;
		delete AccentPen;
		delete TitleFont;
	}
}
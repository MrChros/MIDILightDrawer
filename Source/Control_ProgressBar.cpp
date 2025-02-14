#include "Control_ProgressBar.h"

namespace MIDILightDrawer
{
	Control_ProgressBar::Control_ProgressBar() : ProgressBar()
	{
        this->SetStyle( ControlStyles::UserPaint                |
                        ControlStyles::AllPaintingInWmPaint     |
                        ControlStyles::OptimizedDoubleBuffer    |
                        ControlStyles::ResizeRedraw, true);

        this->_ProgressColor = Color::FromArgb(0, 122, 204);  // Default blue
        this->_BorderColor = Color::FromArgb(60, 60, 60);
        this->_IsCompleted = false;
	}

    void Control_ProgressBar::OnPaint(PaintEventArgs^ e)
    {
        Rectangle Rect = ClientRectangle;
        Graphics^ g = e->Graphics;

        // Enable anti-aliasing for smoother drawing
        g->SmoothingMode = System::Drawing::Drawing2D::SmoothingMode::AntiAlias;
        
        // Draw background
        g->FillRectangle(gcnew SolidBrush(this->BackColor), Rect);

        // Calculate progress width
        int Width = (int)(Rect.Width * (Value / (double)Maximum));
        if (Width > 0)
        {
            // Draw progress
            Rectangle ProgressRect = Rectangle(Rect.X, Rect.Y, Width, Rect.Height);
            g->FillRectangle(gcnew SolidBrush(_ProgressColor), ProgressRect);
        }

        // Draw border (inset by 0.5 pixels to keep it sharp)
        g->DrawRectangle(gcnew Pen(_BorderColor, 1.0f), Rect.X, Rect.Y, Rect.Width - 1, Rect.Height - 1);
    }

    Color Control_ProgressBar::ProgressColor::get() 
    { 
        return this->_ProgressColor; 
    }

    void Control_ProgressBar::ProgressColor::set(Color value)
    {
        this->_ProgressColor = value;
        this->Invalidate();
    }

    Color Control_ProgressBar::BorderColor::get()
    {
        return this->_BorderColor;
    }

    void Control_ProgressBar::BorderColor::set(Color value)
    {
        this->_BorderColor = value;
        this->Invalidate();
    }

    bool Control_ProgressBar::IsCompleted::get()
    {
        return _IsCompleted;
    }

    void Control_ProgressBar::IsCompleted::set(bool value)
    {
        this->_IsCompleted = value;

        if (this->_IsCompleted) {
            this->_ProgressColor = Color::FromArgb(75, 219, 106); // Green
        }

        this->Invalidate();
    }
}
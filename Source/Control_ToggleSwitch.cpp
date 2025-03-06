#include "Control_ToggleSwitch.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
    Control_ToggleSwitch::Control_ToggleSwitch()
    {
        // Enable double buffering
        this->DoubleBuffered = true;
        this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
            ControlStyles::AllPaintingInWmPaint |
            ControlStyles::UserPaint |
            ControlStyles::ResizeRedraw, true);
        
        this->Text = "";
        this->Size = System::Drawing::Size(40, 20);
        this->Appearance = System::Windows::Forms::Appearance::Button;
        this->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
        this->FlatAppearance->BorderSize = 0;
        this->CheckAlign = ContentAlignment::MiddleCenter;
        this->TextAlign = ContentAlignment::MiddleCenter;
        this->Cursor = Cursors::Hand;
        this->Checked = false;

        // UI Properties
        _TrackWidth = 36;
        _TrackHeight = 14;
        _ThumbDiameter = 18;

        // Apply theme
        ApplyTheme();
    }

    Control_ToggleSwitch::~Control_ToggleSwitch()
    {
        // Clean up resources if needed
    }

    void Control_ToggleSwitch::ApplyTheme()
    {
        Theme_Manager^ theme = Theme_Manager::Get_Instance();
        this->BackColor = Color::Transparent;
        this->ForeColor = theme->ForegroundText;
        this->Invalidate(); // Force redraw with new theme colors
    }

    void Control_ToggleSwitch::OnPaint(PaintEventArgs^ e)
    {
        Control^ parent = this->Parent;
        if (parent != nullptr) {
            e->Graphics->Clear(parent->BackColor);
        }
        else {
            e->Graphics->Clear(this->BackColor);
        }

        // Get the graphics object
        Graphics^ G = e->Graphics;
        G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

        Rectangle Rect = this->ClientRectangle;

        // Calculate sizes based on control dimensions
        int minDimension = Math::Min(Rect.Width, Rect.Height);

        // Calculate track dimensions (proportional to control size)
        _TrackWidth = Math::Max(static_cast<int>(Rect.Width * 0.9), minDimension);
        _TrackHeight = Math::Max(static_cast<int>(minDimension * 0.3), 6);

        // Calculate thumb diameter (proportional to track height)
        _ThumbDiameter = this->Height - 2;

        // Calculate positions
        int TrackX = (Rect.Width - _TrackWidth) / 2;
        int TrackY = (Rect.Height - _TrackHeight) / 2;
        int ThumbX;

        // Calculate thumb position with proper boundary constraints
        if (this->Checked) {
            ThumbX = TrackX + _TrackWidth - _ThumbDiameter;
        }
        else {
            ThumbX = TrackX;
        }

        int ThumbY = (Rect.Height - _ThumbDiameter) / 2;

        // Get theme colors
        Theme_Manager^ Theme = Theme_Manager::Get_Instance();
        Color TrackColor = this->Checked ? Color::FromArgb(128, Theme->AccentPrimary) : Theme->BackgroundAlt;
        Color ThumbColor;

        // Apply different thumb color based on mouse state
        if (_IsMouseOver) {
            ThumbColor = this->Checked ? Color::FromArgb(220, Theme->AccentPrimary) : Theme->BackgroundLight;
        }
        else {
            ThumbColor = this->Checked ? Theme->AccentPrimary : Theme->BackgroundLight;
        }

        Color BorderColor = Theme->BorderStrong;

        // Draw the track (rounded rectangle)
        Drawing2D::GraphicsPath^ TrackPath = gcnew Drawing2D::GraphicsPath();
        int cornerRadius = _TrackHeight;
        TrackPath->AddArc(TrackX, TrackY, cornerRadius, cornerRadius, 90, 180);
        TrackPath->AddArc(TrackX + _TrackWidth - cornerRadius, TrackY, cornerRadius, cornerRadius, 270, 180);
        TrackPath->CloseFigure();

        G->FillPath(gcnew SolidBrush(TrackColor), TrackPath);
        G->DrawPath(gcnew Pen(BorderColor, 1), TrackPath);

        // Draw the thumb (circle)
        G->FillEllipse(gcnew SolidBrush(ThumbColor), ThumbX, ThumbY, _ThumbDiameter, _ThumbDiameter);
        G->DrawEllipse(gcnew Pen(BorderColor, 1), ThumbX, ThumbY, _ThumbDiameter, _ThumbDiameter);

        delete TrackPath;
    }

    void Control_ToggleSwitch::OnMouseEnter(EventArgs^ e)
    {
        _IsMouseOver = true;
        this->Invalidate();
        CheckBox::OnMouseEnter(e);
    }

    void Control_ToggleSwitch::OnMouseLeave(EventArgs^ e)
    {
        _IsMouseOver = false;
        this->Invalidate();
        CheckBox::OnMouseLeave(e);
    }
}
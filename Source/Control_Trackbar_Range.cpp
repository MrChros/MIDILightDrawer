#include "Control_Trackbar_Range.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
    Control_Trackbar_Range::Control_Trackbar_Range()
    {
        // Enable double buffering
        this->DoubleBuffered = true;
        this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
            ControlStyles::AllPaintingInWmPaint |
            ControlStyles::UserPaint |
            ControlStyles::ResizeRedraw, true);

        // Initialize default values
        _Mode = TrackbarRangeMode::Specific;
        _Minimum = 0;
        _Maximum = 100;
        _Value = 50;
        _MinValue = 30;
        _MaxValue = 70;
        _Step = 1;

        // Initialize appearance
        Theme_Manager^ theme = Theme_Manager::Get_Instance();
        _TrackColor = theme->BackgroundAlt;
        _RangeColor = theme->AccentPrimary;
        _ThumbColor = theme->BackgroundLight;
        _ThumbBorderColor = theme->BorderStrong;
        _ThumbSize = 14;
        _TrackHeight = 6;

        // Initialize interaction state
        _IsDraggingMinThumb = false;
        _IsDraggingMaxThumb = false;
        _IsDraggingSingleThumb = false;

        // Set control size
        this->Height = 30;
        this->MinimumSize = Drawing::Size(100, 30);
    }

    void Control_Trackbar_Range::OnPaint(PaintEventArgs^ e)
    {
        Graphics^ g = e->Graphics;
        g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

        // Draw the track and range
        DrawTrack(g);
        DrawRange(g);

        // Draw thumbs based on mode
        if (_Mode == TrackbarRangeMode::Specific)
        {
            // Draw single thumb
            int thumbX = PositionFromValue(_Value);
            DrawThumb(g, thumbX, _IsDraggingSingleThumb);
        }
        else // Range mode
        {
            // Draw min thumb
            int minThumbX = PositionFromValue(_MinValue);
            DrawThumb(g, minThumbX, _IsDraggingMinThumb);

            // Draw max thumb
            int maxThumbX = PositionFromValue(_MaxValue);
            DrawThumb(g, maxThumbX, _IsDraggingMaxThumb);
        }
    }

    void Control_Trackbar_Range::OnMouseDown(MouseEventArgs^ e)
    {
        if (_Mode == TrackbarRangeMode::Specific)
        {
            if (IsOverSingleThumb(Point(e->X, e->Y)))
            {
                _IsDraggingSingleThumb = true;
                this->Capture = true;
            }
            else
            {
                // Click on track - move thumb to that position
                _Value = ValueFromPosition(e->X);
                _Value = Math::Max(_Minimum, Math::Min(_Maximum, _Value));
                RaiseValueChangedEvent();
                Invalidate();
            }
        }
        else // Range mode
        {
            if (IsOverMinThumb(Point(e->X, e->Y)))
            {
                _IsDraggingMinThumb = true;
                this->Capture = true;
            }
            else if (IsOverMaxThumb(Point(e->X, e->Y)))
            {
                _IsDraggingMaxThumb = true;
                this->Capture = true;
            }
            else
            {
                // Click on track - determine which thumb to move based on proximity
                int value = ValueFromPosition(e->X);
                if (Math::Abs(value - _MinValue) < Math::Abs(value - _MaxValue))
                {
                    _MinValue = value;
                    _MinValue = Math::Max(_Minimum, Math::Min(_MaxValue - _Step, _MinValue));
                }
                else
                {
                    _MaxValue = value;
                    _MaxValue = Math::Max(_MinValue + _Step, Math::Min(_Maximum, _MaxValue));
                }
                RaiseValueChangedEvent();
                Invalidate();
            }
        }

        Control::OnMouseDown(e);
    }

    void Control_Trackbar_Range::OnMouseMove(MouseEventArgs^ e)
    {
        if (_Mode == TrackbarRangeMode::Specific)
        {
            if (_IsDraggingSingleThumb)
            {
                _Value = ValueFromPosition(e->X);
                _Value = Math::Max(_Minimum, Math::Min(_Maximum, _Value));
                RaiseValueChangedEvent();
                Invalidate();
            }

            // Update cursor
            if (IsOverSingleThumb(Point(e->X, e->Y)))
            {
                this->Cursor = Cursors::Hand;
            }
            else
            {
                this->Cursor = Cursors::Default;
            }
        }
        else // Range mode
        {
            if (_IsDraggingMinThumb)
            {
                _MinValue = ValueFromPosition(e->X);
                _MinValue = Math::Max(_Minimum, Math::Min(_MaxValue - _Step, _MinValue));
                RaiseValueChangedEvent();
                Invalidate();
            }
            else if (_IsDraggingMaxThumb)
            {
                _MaxValue = ValueFromPosition(e->X);
                _MaxValue = Math::Max(_MinValue + _Step, Math::Min(_Maximum, _MaxValue));
                RaiseValueChangedEvent();
                Invalidate();
            }

            // Update cursor
            if (IsOverMinThumb(Point(e->X, e->Y)) || IsOverMaxThumb(Point(e->X, e->Y)))
            {
                this->Cursor = Cursors::Hand;
            }
            else
            {
                this->Cursor = Cursors::Default;
            }
        }

        Control::OnMouseMove(e);
    }

    void Control_Trackbar_Range::OnMouseUp(MouseEventArgs^ e)
    {
        _IsDraggingSingleThumb = false;
        _IsDraggingMinThumb = false;
        _IsDraggingMaxThumb = false;
        this->Capture = false;
        Invalidate();

        Control::OnMouseUp(e);
    }

    void Control_Trackbar_Range::OnMouseWheel(MouseEventArgs^ e)
    {
        if (_Mode == TrackbarRangeMode::Specific)
        {
            int delta = e->Delta > 0 ? _Step : -_Step;
            _Value += delta;
            _Value = Math::Max(_Minimum, Math::Min(_Maximum, _Value));
            RaiseValueChangedEvent();
            Invalidate();
        }
        else // Range mode
        {
            // Determine which thumb to adjust based on mouse position
            int x = e->X;
            int minThumbX = PositionFromValue(_MinValue);
            int maxThumbX = PositionFromValue(_MaxValue);

            int delta = e->Delta > 0 ? _Step : -_Step;

            if (Math::Abs(x - minThumbX) < Math::Abs(x - maxThumbX))
            {
                // Adjust min value
                _MinValue += delta;
                _MinValue = Math::Max(_Minimum, Math::Min(_MaxValue - _Step, _MinValue));
            }
            else
            {
                // Adjust max value
                _MaxValue += delta;
                _MaxValue = Math::Max(_MinValue + _Step, Math::Min(_Maximum, _MaxValue));
            }

            RaiseValueChangedEvent();
            Invalidate();
        }

        Control::OnMouseWheel(e);
    }

    int Control_Trackbar_Range::ValueFromPosition(int x)
    {
        int rightPadding = _ThumbSize;
        int trackWidth = this->Width - _ThumbSize - rightPadding;
        int position = Math::Max(0, Math::Min(trackWidth, x - _ThumbSize / 2));
        return _Minimum + (int)((double)position / trackWidth * (_Maximum - _Minimum));
    }

    int Control_Trackbar_Range::PositionFromValue(int value)
    {
        int rightPadding = _ThumbSize;
        int trackWidth = this->Width - _ThumbSize - rightPadding;
        double valueRange = _Maximum - _Minimum;
        int position = (int)((value - _Minimum) / valueRange * trackWidth);
        return position + _ThumbSize / 2;
    }

    bool Control_Trackbar_Range::IsOverMinThumb(Point p)
    {
        int thumbX = PositionFromValue(_MinValue);
        Rectangle thumbRect(thumbX - _ThumbSize / 2, (this->Height - _ThumbSize) / 2, _ThumbSize, _ThumbSize);
        return thumbRect.Contains(p);
    }

    bool Control_Trackbar_Range::IsOverMaxThumb(Point p)
    {
        int thumbX = PositionFromValue(_MaxValue);
        Rectangle thumbRect(thumbX - _ThumbSize / 2, (this->Height - _ThumbSize) / 2, _ThumbSize, _ThumbSize);
        return thumbRect.Contains(p);
    }

    bool Control_Trackbar_Range::IsOverSingleThumb(Point p)
    {
        int thumbX = PositionFromValue(_Value);
        Rectangle thumbRect(thumbX - _ThumbSize / 2, (this->Height - _ThumbSize) / 2, _ThumbSize, _ThumbSize);
        return thumbRect.Contains(p);
    }

    void Control_Trackbar_Range::DrawTrack(Graphics^ g)
    {
        int trackY = (this->Height - _TrackHeight) / 2;

        // Make track shorter by adding padding on the right side
        int rightPadding = _ThumbSize;
        Rectangle trackRect(_ThumbSize / 2, trackY, this->Width - _ThumbSize - rightPadding, _TrackHeight);

        // Draw track with rounded corners
        Drawing2D::GraphicsPath^ path = gcnew Drawing2D::GraphicsPath();
        int radius = _TrackHeight / 2;
        path->AddArc(trackRect.X, trackRect.Y, radius * 2, radius * 2, 180, 90);
        path->AddArc(trackRect.Right - radius * 2, trackRect.Y, radius * 2, radius * 2, 270, 90);
        path->AddArc(trackRect.Right - radius * 2, trackRect.Bottom - radius * 2, radius * 2, radius * 2, 0, 90);
        path->AddArc(trackRect.X, trackRect.Bottom - radius * 2, radius * 2, radius * 2, 90, 90);
        path->CloseFigure();

        g->FillPath(gcnew SolidBrush(_TrackColor), path);

        delete path;
    }

    void Control_Trackbar_Range::DrawRange(Graphics^ g)
    {
        int trackY = (this->Height - _TrackHeight) / 2;
        int rightPadding = _ThumbSize;

        if (_Mode == TrackbarRangeMode::Specific)
        {
            // In single value mode, we draw the range from min to the value
            int startX = _ThumbSize / 2;
            int endX = PositionFromValue(_Value);

            Rectangle rangeRect(startX, trackY, endX - startX, _TrackHeight);

            if (rangeRect.Width > 0)
            {
                Drawing2D::GraphicsPath^ path = gcnew Drawing2D::GraphicsPath();
                int radius = _TrackHeight / 2;
                path->AddArc(rangeRect.X, rangeRect.Y, radius * 2, radius * 2, 180, 90);
                path->AddLine(rangeRect.X + radius, rangeRect.Y, rangeRect.Right, rangeRect.Y);
                path->AddLine(rangeRect.Right, rangeRect.Y, rangeRect.Right, rangeRect.Bottom);
                path->AddLine(rangeRect.Right, rangeRect.Bottom, rangeRect.X + radius, rangeRect.Bottom);
                path->AddArc(rangeRect.X, rangeRect.Bottom - radius * 2, radius * 2, radius * 2, 90, 90);
                path->CloseFigure();

                g->FillPath(gcnew SolidBrush(_RangeColor), path);

                delete path;
            }
        }
        else // Range mode
        {
            int startX = PositionFromValue(_MinValue);
            int endX = PositionFromValue(_MaxValue);

            Rectangle rangeRect(startX, trackY, endX - startX, _TrackHeight);

            if (rangeRect.Width > 0)
            {
                g->FillRectangle(gcnew SolidBrush(_RangeColor), rangeRect);
            }
        }
    }

    void Control_Trackbar_Range::DrawThumb(Graphics^ g, int x, bool isActive)
    {
        int thumbY = (this->Height - _ThumbSize) / 2;
        Rectangle thumbRect(x - _ThumbSize / 2, thumbY, _ThumbSize, _ThumbSize);

        // Fill thumb
        Color thumbColor = isActive ? Color::FromArgb(
            Math::Min(255, _ThumbColor.R + 20),
            Math::Min(255, _ThumbColor.G + 20),
            Math::Min(255, _ThumbColor.B + 20)) : _ThumbColor;

        g->FillEllipse(gcnew SolidBrush(thumbColor), thumbRect);

        // Draw border
        g->DrawEllipse(gcnew Pen(_ThumbBorderColor), thumbRect);
    }

    void Control_Trackbar_Range::RaiseValueChangedEvent()
    {
        if (_Mode == TrackbarRangeMode::Specific)
        {
            ValueChanged(this, gcnew TrackbarRangeValueChangedEventArgs(_Value, _Minimum, _Maximum));
        }
        else
        {
            ValueChanged(this, gcnew TrackbarRangeValueChangedEventArgs(0, _MinValue, _MaxValue));
        }
    }

    void Control_Trackbar_Range::Mode::set(TrackbarRangeMode value)
    {
        if (_Mode != value)
        {
            _Mode = value;
            Invalidate();
        }
    }

    void Control_Trackbar_Range::Minimum::set(int value)
    {
        if (value < _Maximum)
        {
            _Minimum = value;
            _Value = Math::Max(_Minimum, _Value);
            _MinValue = Math::Max(_Minimum, _MinValue);
            Invalidate();
        }
    }

    void Control_Trackbar_Range::Maximum::set(int value)
    {
        if (value > _Minimum)
        {
            _Maximum = value;
            _Value = Math::Min(_Maximum, _Value);
            _MaxValue = Math::Min(_Maximum, _MaxValue);
            Invalidate();
        }
    }

    void Control_Trackbar_Range::Value::set(int value)
    {
        if (_Value != value)
        {
            _Value = Math::Max(_Minimum, Math::Min(_Maximum, value));
            RaiseValueChangedEvent();
            Invalidate();
        }
    }

    void Control_Trackbar_Range::MinValue::set(int value)
    {
        if (_MinValue != value)
        {
            _MinValue = Math::Max(_Minimum, Math::Min(_MaxValue - _Step, value));
            RaiseValueChangedEvent();
            Invalidate();
        }
    }

    void Control_Trackbar_Range::MaxValue::set(int value)
    {
        if (_MaxValue != value)
        {
            _MaxValue = Math::Max(_MinValue + _Step, Math::Min(_Maximum, value));
            RaiseValueChangedEvent();
            Invalidate();
        }
    }

    void Control_Trackbar_Range::Step::set(int value)
    {
        if (value > 0)
        {
            _Step = value;
        }
    }

    void Control_Trackbar_Range::TrackColor::set(Color value)
    {
        if (_TrackColor != value)
        {
            _TrackColor = value;
            Invalidate();
        }
    }

    void Control_Trackbar_Range::RangeColor::set(Color value)
    {
        if (_RangeColor != value)
        {
            _RangeColor = value;
            Invalidate();
        }
    }

    void Control_Trackbar_Range::ThumbColor::set(Color value)
    {
        if (_ThumbColor != value)
        {
            _ThumbColor = value;
            Invalidate();
        }
    }
}
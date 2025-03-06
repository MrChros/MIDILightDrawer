#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::ComponentModel;

namespace MIDILightDrawer
{
    // Event arguments for value changed events
    public ref class TrackbarRangeValueChangedEventArgs : public EventArgs
    {
    public:
        TrackbarRangeValueChangedEventArgs(int value, int minValue, int maxValue)
        {
            Value = value;
            MinValue = minValue;
            MaxValue = maxValue;
        }

        property int Value;
        property int MinValue;
        property int MaxValue;
    };

    // Delegate for value changed events
    public delegate void TrackbarRangeValueChangedEventHandler(Object^ sender, TrackbarRangeValueChangedEventArgs^ e);

    // Enum to define the mode of the trackbar
    public enum class TrackbarRangeMode
    {
        Specific,
        Range
    };

    public ref class Control_Trackbar_Range : public Control
    {
    public:
        Control_Trackbar_Range();

        // Events
        event TrackbarRangeValueChangedEventHandler^ ValueChanged;

    protected:
        virtual void OnPaint(PaintEventArgs^ e) override;
        virtual void OnMouseDown(MouseEventArgs^ e) override;
        virtual void OnMouseMove(MouseEventArgs^ e) override;
        virtual void OnMouseUp(MouseEventArgs^ e) override;
        virtual void OnMouseWheel(MouseEventArgs^ e) override;

    private:
        // Mode settings
        TrackbarRangeMode _Mode;

        // Value properties
        int _Minimum;
        int _Maximum;
        int _Value;       // Used in SingleValue mode
        int _MinValue;    // Used in Range mode
        int _MaxValue;    // Used in Range mode
        int _Step;        // Step size for value changes

        // Appearance
        Color _TrackColor;
        Color _RangeColor;
        Color _ThumbColor;
        Color _ThumbBorderColor;
        int _ThumbSize;
        int _TrackHeight;

        // Interaction state
        bool _IsDraggingMinThumb;
        bool _IsDraggingMaxThumb;
        bool _IsDraggingSingleThumb;

        // Helper methods
        int ValueFromPosition(int x);
        int PositionFromValue(int value);
        bool IsOverMinThumb(Point p);
        bool IsOverMaxThumb(Point p);
        bool IsOverSingleThumb(Point p);
        void DrawTrack(Graphics^ g);
        void DrawRange(Graphics^ g);
        void DrawThumb(Graphics^ g, int x, bool isActive);
        void RaiseValueChangedEvent();

    public:
        // Properties
        property TrackbarRangeMode Mode {
            TrackbarRangeMode get() { return _Mode; }
            void set(TrackbarRangeMode value);
        }

        property int Minimum {
            int get() { return _Minimum; }
            void set(int value);
        }

        property int Maximum {
            int get() { return _Maximum; }
            void set(int value);
        }

        property int Value {
            int get() { return _Value; }
            void set(int value);
        }

        property int MinValue {
            int get() { return _MinValue; }
            void set(int value);
        }

        property int MaxValue {
            int get() { return _MaxValue; }
            void set(int value);
        }

        property int Step {
            int get() { return _Step; }
            void set(int value);
        }

        property Color TrackColor {
            Color get() { return _TrackColor; }
            void set(Color value);
        }

        property Color RangeColor {
            Color get() { return _RangeColor; }
            void set(Color value);
        }

        property Color ThumbColor {
            Color get() { return _ThumbColor; }
            void set(Color value);
        }
    };
}
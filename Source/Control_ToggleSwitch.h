#pragma once

#include "Theme_Manager.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
    public ref class Control_ToggleSwitch : public CheckBox
    {
    public:
        Control_ToggleSwitch();
        virtual ~Control_ToggleSwitch();

        void ApplyTheme();
    
    protected:
        virtual void OnPaint(PaintEventArgs^ e) override;
        virtual void OnMouseEnter(EventArgs^ e) override;
        virtual void OnMouseLeave(EventArgs^ e) override;

    private:
        int _TrackWidth;
        int _TrackHeight;
        int _ThumbDiameter;
        bool _IsMouseOver;
    };
}
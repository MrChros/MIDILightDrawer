#pragma once

#include "Theme_Manager.h"
#include "Control_ToggleSwitch.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
    public ref class Control_ExpandablePanel : public Panel
    {
    public:
        static const int HEADER_HEIGHT = 24;

    public:
        Control_ExpandablePanel(String^ title, Point location, int width);
        virtual ~Control_ExpandablePanel();

        // Methods
        void Toggle();
        void Expand();
        void Collapse();
        void ApplyTheme();

    protected:
        virtual void OnPaint(PaintEventArgs^ e) override;
        virtual void OnEnabledChanged(EventArgs^ e) override;

    private:
        // UI Elements
        Panel^ _HeaderPanel;
        Panel^ _ContentPanel;
        Label^ _HeaderLabel;
        PictureBox^ _ExpandIcon;
        Control_ToggleSwitch^ _ToggleSwitch;

        // State
        bool _IsExpanded;
        bool _IsEnabled;
        EventHandler^ _ExpandToggled;
        EventHandler^ _EnabledChanged;
        Bitmap^ _ExpandedIcon;
        Bitmap^ _CollapsedIcon;

        // Methods
        void InitializeComponent();
        void CreateExpandCollapseIcons();
        void OnHeaderClick(Object^ sender, EventArgs^ e);
        void OnToggleSwitchChanged(Object^ sender, EventArgs^ e);

    public:
        // Properties
        property bool IsExpanded {
            bool get() { return _IsExpanded; }
        }

        property bool IsEnabled {
            bool get() { return _IsEnabled; }
            void set(bool value) {
                _IsEnabled = value;
                if (_ToggleSwitch != nullptr) {
                    _ToggleSwitch->Checked = value;
                }
                OnEnabledChanged(EventArgs::Empty);
            }
        }

        property Panel^ ContentPanel {
            Panel^ get() { return _ContentPanel; }
        }

        property String^ Title {
            String^ get() { return _HeaderLabel->Text; }
            void set(String^ value) { _HeaderLabel->Text = value; }
        }

        property EventHandler^ ExpandToggled {
            EventHandler^ get() { return _ExpandToggled; }
            void set(EventHandler^ value) { _ExpandToggled = value; }
        }

        property EventHandler^ EnabledChanged {
            EventHandler^ get() { return _EnabledChanged; }
            void set(EventHandler^ value) { _EnabledChanged = value; }
        }
    };
}
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
			bool get();
        }

        property bool IsEnabled {
			bool get();
			void set(bool value);
        }

        property Panel^ ContentPanel {
			Panel^ get();
        }

        property String^ Title {
			String^ get();
			void set(String^ value);
        }

        property EventHandler^ ExpandToggled {
			EventHandler^ get();
			void set(EventHandler^ value);
        }

        property EventHandler^ EnabledChanged {
			EventHandler^ get();
			void set(EventHandler^ value);
        }
    };
}
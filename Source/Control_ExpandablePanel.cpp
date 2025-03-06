#include "Control_ExpandablePanel.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
    Control_ExpandablePanel::Control_ExpandablePanel(String^ title, Point location, int width)
    {
        this->DoubleBuffered = true;
        this->SetStyle(ControlStyles::OptimizedDoubleBuffer | ControlStyles::ResizeRedraw, true);
        this->Location = location;
        this->Width = width;
        this->Height = HEADER_HEIGHT;
        this->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top);

        _IsExpanded = false;
        _IsEnabled = false; // Default to disabled
        _ExpandToggled = nullptr;
        _EnabledChanged = nullptr;

        InitializeComponent();
        CreateExpandCollapseIcons();

        _HeaderLabel->Text = title;
        _ExpandIcon->Image = _CollapsedIcon;

        this->ApplyTheme();
    }

    Control_ExpandablePanel::~Control_ExpandablePanel()
    {
        // Clean up resources
        if (_ExpandedIcon != nullptr) {
            delete _ExpandedIcon;
        }

        if (_CollapsedIcon != nullptr) {
            delete _CollapsedIcon;
        }
    }

    void Control_ExpandablePanel::InitializeComponent()
    { 
        // Create header panel
        _HeaderPanel = gcnew Panel();
        _HeaderPanel->Location = Point(0, 0);
        _HeaderPanel->Size = System::Drawing::Size(this->Width, HEADER_HEIGHT);
        _HeaderPanel->Cursor = Cursors::Hand;
        _HeaderPanel->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top);
        _HeaderPanel->Click += gcnew EventHandler(this, &Control_ExpandablePanel::OnHeaderClick);

        // Create header label
        _HeaderLabel = gcnew Label();
        _HeaderLabel->Location = Point(36, 4);
        _HeaderLabel->AutoSize = true;
        _HeaderLabel->Font = gcnew System::Drawing::Font("Segoe UI Semibold", 9.0f);
        _HeaderLabel->Cursor = Cursors::Hand;
        _HeaderLabel->Click += gcnew EventHandler(this, &Control_ExpandablePanel::OnHeaderClick);

        // Create expand/collapse icon
        _ExpandIcon = gcnew PictureBox();
        _ExpandIcon->Size = System::Drawing::Size(20, 20);
        _ExpandIcon->Location = Point(6, (HEADER_HEIGHT - _ExpandIcon->Height) / 2);
        _ExpandIcon->Cursor = Cursors::Hand;
        _ExpandIcon->Click += gcnew EventHandler(this, &Control_ExpandablePanel::OnHeaderClick);
        _ExpandIcon->SizeMode = PictureBoxSizeMode::StretchImage;

        // Create toggle switch
        _ToggleSwitch = gcnew Control_ToggleSwitch();
        _ToggleSwitch->Size = System::Drawing::Size(32, 16);
        _ToggleSwitch->Location = Point(this->Width - 50, (HEADER_HEIGHT - _ToggleSwitch->Height) / 2);
        _ToggleSwitch->Checked = _IsEnabled;
        _ToggleSwitch->Anchor = static_cast<AnchorStyles>(AnchorStyles::Right | AnchorStyles::Top);
        _ToggleSwitch->CheckedChanged += gcnew EventHandler(this, &Control_ExpandablePanel::OnToggleSwitchChanged);

        // Create content panel
        _ContentPanel = gcnew Panel();
        _ContentPanel->Location = Point(0, HEADER_HEIGHT);
        _ContentPanel->Size = System::Drawing::Size(this->Width, 0); // Start with zero height
        _ContentPanel->Visible = false;
        _ContentPanel->Anchor = static_cast<AnchorStyles>(AnchorStyles::Left | AnchorStyles::Right | AnchorStyles::Top);

        // Add controls to their parent containers
        _HeaderPanel->Controls->Add(_HeaderLabel);
        _HeaderPanel->Controls->Add(_ExpandIcon);
        _HeaderPanel->Controls->Add(_ToggleSwitch);
        this->Controls->Add(_HeaderPanel);
        this->Controls->Add(_ContentPanel);
    }


    void Control_ExpandablePanel::OnPaint(PaintEventArgs^ e)
    {
        Panel::OnPaint(e);

        // Draw optional custom painting here
        Graphics^ g = e->Graphics;
        Pen^ borderPen = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong, 1);

        // Draw a border around the panel
        g->DrawRectangle(borderPen, 0, 0, this->Width - 1, this->Height - 1);

        // Draw a separator line below the header
        g->DrawLine(borderPen, 0, _HeaderPanel->Height, this->Width, _HeaderPanel->Height);

        delete borderPen;
    }

    void Control_ExpandablePanel::OnEnabledChanged(EventArgs^ e)
    {
        // Invoke the event handler if it exists
        if (_EnabledChanged != nullptr) {
            _EnabledChanged(this, e);
        }
    }

    void Control_ExpandablePanel::CreateExpandCollapseIcons()
    {
        // Create expanded icon (arrow pointing down)
        _ExpandedIcon = gcnew Bitmap(20, 20);
        Graphics^ G1 = Graphics::FromImage(_ExpandedIcon);
        G1->Clear(Color::Transparent);
        G1->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

        array<Point>^ TrianglePoints1 = {
            Point(4, 6),
            Point(16, 6),
            Point(10, 14)
        };

        G1->FillPolygon(Brushes::Black, TrianglePoints1);
        delete G1;

        // Create collapsed icon (arrow pointing right)
        _CollapsedIcon = gcnew Bitmap(20, 20);
        Graphics^ G2 = Graphics::FromImage(_CollapsedIcon);
        G2->Clear(Color::Transparent);
        G2->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

        array<Point>^ TrianglePoints2 = {
            Point(6, 4),
            Point(6, 16),
            Point(14, 10)
        };

        G2->FillPolygon(Brushes::Black, TrianglePoints2);
        delete G2;
    }

    void Control_ExpandablePanel::Toggle()
    {
        if (_IsExpanded) {
            Collapse();
        }
        else {
            Expand();
        }

        // Fire the event if there's a handler
        if (_ExpandToggled != nullptr) {
            _ExpandToggled(this, EventArgs::Empty);
        }
    }

    void Control_ExpandablePanel::Expand()
    {
        if (_IsExpanded) {
            return;
        }

        _IsExpanded = true;
        _ContentPanel->Visible = true;
        this->Height = _HeaderPanel->Height + _ContentPanel->Height;
        _ExpandIcon->Image = _ExpandedIcon;
    }

    void Control_ExpandablePanel::Collapse()
    {
        if (!_IsExpanded) {
            return;
        }

        _IsExpanded = false;
        _ContentPanel->Visible = false;
        this->Height = _HeaderPanel->Height;
        _ExpandIcon->Image = _CollapsedIcon;
    }

    void Control_ExpandablePanel::ApplyTheme()
    {
        Theme_Manager^ theme = Theme_Manager::Get_Instance();

        // Apply theme to this panel
        this->BackColor = theme->Background;

        // Apply theme to header panel
        _HeaderPanel->BackColor = theme->BackgroundAlt;

        // Apply theme to header label
        _HeaderLabel->ForeColor = theme->ForegroundText;
        _HeaderLabel->BackColor = Color::Transparent;

        // Update icon colors
        Color iconColor = theme->ForegroundText;

        for (int x = 0; x < _ExpandedIcon->Width; x++) {
            for (int y = 0; y < _ExpandedIcon->Height; y++) {
                Color pixelColor = _ExpandedIcon->GetPixel(x, y);
                if (pixelColor.A > 0) { // If not fully transparent
                    _ExpandedIcon->SetPixel(x, y, Color::FromArgb((int)pixelColor.A, iconColor));
                }
            }
        }

        for (int x = 0; x < _CollapsedIcon->Width; x++) {
            for (int y = 0; y < _CollapsedIcon->Height; y++) {
                Color pixelColor = _CollapsedIcon->GetPixel(x, y);
                if (pixelColor.A > 0) { // If not fully transparent
                    _CollapsedIcon->SetPixel(x, y, Color::FromArgb((int)pixelColor.A, iconColor));
                }
            }
        }

        // Refresh the current icon
        _ExpandIcon->Refresh();

        // Apply theme to content panel
        _ContentPanel->BackColor = theme->Background;
    }

    void Control_ExpandablePanel::OnHeaderClick(Object^ sender, EventArgs^ e)
    {
        Toggle();
    }

    void Control_ExpandablePanel::OnToggleSwitchChanged(Object^ sender, EventArgs^ e)
    {
        _IsEnabled = _ToggleSwitch->Checked;
        OnEnabledChanged(EventArgs::Empty);
    }
}
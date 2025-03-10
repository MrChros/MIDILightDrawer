#include "Control_ScrollablePanel.h"

namespace MIDILightDrawer
{
	Control_ScrollablePanel::Control_ScrollablePanel()
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::DoubleBuffer |
			ControlStyles::ResizeRedraw |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::SupportsTransparentBackColor |
			ControlStyles::OptimizedDoubleBuffer, true);

		// Create vertical scrollbar
		_VScrollBar = gcnew VScrollBar();
		_VScrollBar->Dock = DockStyle::Right;
		_VScrollBar->Width = SystemInformation::VerticalScrollBarWidth;
		_VScrollBar->Enabled = false;

		// Create content panel
		_ContentPanel = gcnew Panel();
		_ContentPanel->Dock = DockStyle::Fill;
		_ContentPanel->AutoScroll = true;
		//_ContentPanel->DoubleBuffered = true;
		_ContentPanel->GetType()->GetProperty("DoubleBuffered",
			System::Reflection::BindingFlags::Instance |
			System::Reflection::BindingFlags::NonPublic)
			->SetValue(_ContentPanel, true, nullptr);

		_ContentPanel->Resize += gcnew System::EventHandler(this, &Control_ScrollablePanel::OnContentResize);

		// Add controls to the panel
		this->Controls->Add(_ContentPanel);
		this->Controls->Add(_VScrollBar);

		_ScrollNeeded = false;
		_UpdatePending = false;
	}

	void Control_ScrollablePanel::OnResize(EventArgs^ e)
	{
		Panel::OnResize(e);

		UpdateScrollbarVisibility();
	}

	void Control_ScrollablePanel::UpdateScrollbarVisibility()
	{
		try
		{
			SuspendLayout();

			// Check if vertical scrollbar is needed
			bool needsScroll = _ContentPanel->VerticalScroll->Visible;

			if (needsScroll != _ScrollNeeded)
			{
				_ScrollNeeded = needsScroll;

				// When scrolling IS needed, hide our custom scrollbar
				// When scrolling is NOT needed, show our custom scrollbar
				_VScrollBar->Visible = !needsScroll;
			}
		}
		finally
		{
			ResumeLayout(false); // Pass false to avoid immediate repainting
		}
	}

	void Control_ScrollablePanel::OnContentResize(System::Object^ sender, System::EventArgs^ e)
	{
		SuspendLayout();
		UpdateScrollbarVisibility();
		ResumeLayout();
	}

	Panel^ Control_ScrollablePanel::ContentPanel::get() 
	{
		return _ContentPanel; 
	}
}



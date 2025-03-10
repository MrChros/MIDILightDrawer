#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	public ref class Control_ScrollablePanel : public Panel
	{
	private:
		VScrollBar^ _VScrollBar;
		Panel^ _ContentPanel;

		bool _ScrollNeeded;
		bool _UpdatePending;

	public:
		Control_ScrollablePanel();

	protected:
		virtual void OnResize(EventArgs^ e) override;

	private:
		void UpdateScrollbarVisibility();
		void OnContentResize(System::Object^ sender, System::EventArgs^ e);

	public:
		property Panel^ ContentPanel {
			Panel^ get();
		}
	};
}
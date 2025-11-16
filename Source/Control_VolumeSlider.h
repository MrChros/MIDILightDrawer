#pragma once

#include "Theme_Manager.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Control_VolumeSlider : public UserControl
	{
	private:
		int _Value;
		int _Minimum;
		int _Maximum;
		bool _Is_Dragging;
		bool _Is_Hovering;
		int _Previous_Volume_Before_Mute;
		Rectangle _Track_Rect;
		Rectangle _Thumb_Rect;
		Rectangle _Icon_Rect;

		EventHandler^ _Value_Changed;

		// Visual properties
		literal int THUMB_SIZE = 16;
		literal int TRACK_HEIGHT = 6;
		literal int ICON_SIZE = 20;
		literal int PADDING = 10;
		literal int WHEEL_STEP_SIZE = 5;

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnMouseWheel(MouseEventArgs^ e) override;
		virtual void OnMouseEnter(EventArgs^ e) override;
		virtual void OnMouseLeave(EventArgs^ e) override;
		void On_Value_Changed(EventArgs^ e);

		void Draw_Volume_Icon(Graphics^ g, Rectangle bounds, float volume_percent);

	public:
		Control_VolumeSlider();

		property int Value
		{
			int get() { return _Value; }
			void set(int value)
			{
				if (value < _Minimum) value = _Minimum;
				if (value > _Maximum) value = _Maximum;

				if (_Value != value) {
					// Save non-zero values as the restore point for unmute
					if (value > 0) {
						_Previous_Volume_Before_Mute = value;
					}

					_Value = value;
					Invalidate();
					On_Value_Changed(EventArgs::Empty);
				}
			}
		}

		property int Minimum
		{
			int get() { return _Minimum; }
			void set(int value) { _Minimum = value; Invalidate(); }
		}

		property int Maximum
		{
			int get() { return _Maximum; }
			void set(int value) { _Maximum = value; Invalidate(); }
		}

		event EventHandler^ ValueChanged {
			void add(EventHandler^ handler) { _Value_Changed += handler; }
			void remove(EventHandler^ handler) { _Value_Changed -= handler; }
		}
	};
}

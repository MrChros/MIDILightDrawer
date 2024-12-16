#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Track_Bar_Value_Changed_Event_Args : public EventArgs {
	private:
		double _Value;

	public:
		Track_Bar_Value_Changed_Event_Args(double value) : _Value(value) {}

		property double Value {
			double get() { return _Value; }
		}
	};
	
	public ref class Control_TrackBar_Zoom : public Control {
	private:
		array<double>^	_Values;
		Rectangle		_Slider_Rect;
		Rectangle		_Left_Circle_Rect;
		Rectangle		_Right_Circle_Rect;
		Color			_Slider_Color;
		bool			_Is_Dragging;
		int				_Drag_Offset;
		int				_Current_Step;
		

		EventHandler<Track_Bar_Value_Changed_Event_Args^>^ _Value_Changed;

		// Visual properties
		literal int SLIDER_WIDTH		= 10;
		literal int SLIDER_HEIGHT		= 20;
		literal int TRACK_HEIGHT		= 2;
		literal int LEFT_CIRCLE_RADIUS	= 5;
		literal int RIGHT_CIRCLE_RADIUS	= 8;

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		void On_Value_Changed(Track_Bar_Value_Changed_Event_Args^ e);

	public:
		Control_TrackBar_Zoom();

		property double Value
		{
			double get();
			void set(double value);
		}

		property Color Slider_Color
		{
			Color get() { return _Slider_Color; }
			void set(Color value)
			{
				_Slider_Color = value;
				Invalidate(); 
			}
		}

		event EventHandler<Track_Bar_Value_Changed_Event_Args^>^ Value_Changed {
			void add(EventHandler<Track_Bar_Value_Changed_Event_Args^>^ handler) { _Value_Changed += handler; }
			void remove(EventHandler<Track_Bar_Value_Changed_Event_Args^>^ handler) { _Value_Changed -= handler; }
		}

		void Set_Values(array<double>^ values_array);

		bool Move_To_Next_Value();
		bool Move_To_Previous_Value();
	};
}
#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Drawing::Drawing2D;

namespace MIDILightDrawer
{
	public enum class SliderType {
		Saturation,
		Value
	};

	public ref class Control_ColorSlider : public Control {
	public:
		Control_ColorSlider(SliderType type);

		void ApplyTheme(Color backgroundColor);

		property int Value {
			int get();
			void set(int value);
		}

		property float Hue {
			float get();
			void set(float value);
		}

		property float Saturation {
			float get();
			void set(float value);
		}

		property float Brightness {
			float get();
			void set(float value);
		}

		event EventHandler^ ValueChanged;

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;

	private:
		SliderType _Type;
		int _Value;
		float _Hue;
		float _Saturation;
		float _Brightness;
		bool _IsDragging;

		void UpdateValueFromMouse(int x);
		static Color ColorFromHSV(float hue, float saturation, float value);
	};
}
#pragma once

namespace MIDILightDrawer
{
	public enum class Easing
	{
		Linear,
		In_Sine,
		In_Quad,
		In_Cubic,
		In_Quart,
		In_Quint,
		In_Expo,
		In_Circ,
		Out_Sine,
		Out_Quad,
		Out_Cubic,
		Out_Quart,
		Out_Quint,
		Out_Expo,
		Out_Circ,
		InOut_Sine,
		InOut_Quad,
		InOut_Cubic,
		InOut_Quart,
		InOut_Quint,
		InOut_Expo,
		InOut_Circ
	};
	
	public ref class Easings abstract sealed
	{
		public:
			static float Linear(float x);
			static float Ease_In_Sine(float x);
			static float Ease_Out_Sine(float x);
			static float Ease_InOut_Sine(float x);
			static float Ease_In_Quad(float x);
			static float Ease_Out_Quad(float x);
			static float Ease_InOut_Quad(float x);
			static float Ease_In_Cubic(float x);
			static float Ease_Out_Cubic(float x);
			static float Ease_InOut_Cubic(float x);
			static float Ease_In_Quart(float x);
			static float Ease_Out_Quart(float x);
			static float Ease_InOut_Quart(float x);
			static float Ease_In_Quint(float x);
			static float Ease_Out_Quint(float x);
			static float Ease_InOut_Quint(float x);
			static float Ease_In_Expo(float x);
			static float Ease_Out_Expo(float x);
			static float Ease_InOut_Expo(float x);
			static float Ease_In_Circ(float x);
			static float Ease_Out_Circ(float x);
			static float Ease_InOut_Circ(float x);

			static float ApplyEasing(float ratio, float value1, float value2, Easing easing);
			static float ApplyEasing(float ratio, Easing easing);
	};
}

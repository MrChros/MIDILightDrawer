#include "Easings.h"

namespace MIDILightDrawer
{

	float Easings::Linear(float x)
	{
		if(x <= 1.0f) {
			return x;
		}

		return 1.0f;
	}

	float Easings::Ease_In_Sine(float x)
	{
		if(x <= 1.0f) {
			return (float)( 1 - System::Math::Cos((x * System::Math::PI) / 2.0f));
		}

		return 1.0f;
	}

	float Easings::Ease_Out_Sine(float x)
	{
		if(x <= 1.0f) {
			return (float)(System::Math::Sin((x * System::Math::PI) / 2.0f));
		}

		return 1.0f;
	}

	float Easings::Ease_InOut_Sine(float x)
	{
		if(x <= 1.0f) {
			return (float)(-(System::Math::Cos(System::Math::PI * x) - 1.0) / 2.0);
		}

		return 1.0f;
	}

	float Easings::Ease_In_Quad(float x)
	{
		if(x <= 1.0f) {
			return x * x;
		}
	
		return 1.0f;
	}

	float Easings::Ease_Out_Quad(float x)
	{
		if(x <= 1.0f) {
			return 1 - (1 - x) * (1 - x);
		}
	
		return 1.0f;
	}

	float Easings::Ease_InOut_Quad(float x)
	{
		if(x <= 1.0f) {
			return (float)(x < 0.5 ? 2 * x * x : 1 - System::Math::Pow(-2 * x + 2, 2) / 2);
		}
	
		return 1.0f;
	}

	float Easings::Ease_In_Cubic(float x)
	{
		if(x <= 1.0f) {
			return x * x * x;
		}
	
		return 1.0f;
	}

	float Easings::Ease_Out_Cubic(float x)
	{
		if(x <= 1.0f) {
			return (float)(1 - System::Math::Pow(1 - x, 3));
		}
	
		return 1.0f;
	}

	float Easings::Ease_InOut_Cubic(float x)
	{
		if(x <= 1.0f) {
			return (float)(x < 0.5 ? 4 * x * x * x : 1 - System::Math::Pow(-2 * x + 2, 3) / 2);
		}
	
		return 1.0f;
	}


	float Easings::Ease_In_Quart(float x)
	{
		if(x <= 1.0f) {
			return x * x * x * x;
		}
	
		return 1.0f;
	}

	float Easings::Ease_Out_Quart(float x)
	{
		if(x <= 1.0f) {
			return (float)(1 - System::Math::Pow(1 - x, 4));
		}
	
		return 1.0f;
	}

	float Easings::Ease_InOut_Quart(float x)
	{
		if(x <= 1.0f) {
			return (float)(x < 0.5 ? 8 * x * x * x * x : 1 - System::Math::Pow(-2 * x + 2, 4) / 2);
		}
	
		return 1.0f;
	}


	float Easings::Ease_In_Quint(float x)
	{
		if(x <= 1.0f) {
			return x * x * x * x * x;
		}
	
		return 1.0f;
	}

	float Easings::Ease_Out_Quint(float x)
	{
		if(x <= 1.0f) {
			return (float)(1 - System::Math::Pow(1 - x, 5));
		}
	
		return 1.0f;
	}

	float Easings::Ease_InOut_Quint(float x)
	{
		if(x <= 1.0f) {
			return (float)(x < 0.5 ? 16 * x * x * x * x * x : 1 - System::Math::Pow(-2 * x + 2, 5) / 2);
		}
	
		return 1.0f;
	}


	float Easings::Ease_In_Expo(float x)
	{
		if(x <= 1.0f) {
			return (float)(x == 0 ? 0 : System::Math::Pow(2, 10 * x - 10));
		}
	
		return 1.0f;
	}

	float Easings::Ease_Out_Expo(float x)
	{
		if(x <= 1.0f) {
			return (float)(x == 1 ? 1 : 1 - System::Math::Pow(2, -10 * x));
		}
	
		return 1.0f;
	}

	float Easings::Ease_InOut_Expo(float x)
	{
		if(x <= 1.0f) {
			return (float)(x == 0
				? 0
				: x == 1
				? 1
				: x < 0.5 ? System::Math::Pow(2, 20 * x - 10) / 2
				: (2 - System::Math::Pow(2, -20 * x + 10)) / 2);
		}
	
		return 1.0f;
	}


	float Easings::Ease_In_Circ(float x)
	{
		if(x <= 1.0f) {
			return (float)(1 - System::Math::Sqrt(1 - System::Math::Pow(x, 2)));
		}
	
		return 1.0f;
	}

	float Easings::Ease_Out_Circ(float x)
	{
		if(x <= 1.0f) {
			return (float)(System::Math::Sqrt(1 - System::Math::Pow(x - 1, 2)));
		}
	
		return 1.0f;
	}

	float Easings::Ease_InOut_Circ(float x)
	{
		if(x <= 1.0f) {
			return	(float)(x < 0.5
					? (1 - System::Math::Sqrt(1 - System::Math::Pow(2 * x, 2))) / 2
					: (System::Math::Sqrt(1 - System::Math::Pow(-2 * x + 2, 2)) + 1) / 2);
		}
	
		return 1.0f;
	}

	float Easings::ApplyEasing(float ratio, float value1, float value2, Easing easing)
	{
		return value1 + (value2 - value1) * ApplyEasing(ratio, easing);
	}

	float Easings::ApplyEasing(float ratio, Easing easing)
	{
		switch (easing)
		{
			case Easing::Linear:		return Easings::Linear(ratio);
			case Easing::In_Sine:		return Easings::Ease_In_Sine(ratio);
			case Easing::In_Quad:		return Easings::Ease_In_Quad(ratio);
			case Easing::In_Cubic:		return Easings::Ease_In_Cubic(ratio);
			case Easing::In_Quart:		return Easings::Ease_In_Quart(ratio);
			case Easing::In_Quint:		return Easings::Ease_In_Quint(ratio);
			case Easing::In_Expo:		return Easings::Ease_In_Expo(ratio);
			case Easing::In_Circ:		return Easings::Ease_In_Circ(ratio);
			case Easing::Out_Sine:		return Easings::Ease_Out_Sine(ratio);
			case Easing::Out_Quad:		return Easings::Ease_Out_Quad(ratio);
			case Easing::Out_Cubic:		return Easings::Ease_Out_Cubic(ratio);
			case Easing::Out_Quart:		return Easings::Ease_Out_Quart(ratio);
			case Easing::Out_Quint:		return Easings::Ease_Out_Quint(ratio);
			case Easing::Out_Expo:		return Easings::Ease_Out_Expo(ratio);
			case Easing::Out_Circ:		return Easings::Ease_Out_Circ(ratio);
			case Easing::InOut_Sine:	return Easings::Ease_InOut_Sine(ratio);
			case Easing::InOut_Quad:	return Easings::Ease_InOut_Quad(ratio);
			case Easing::InOut_Cubic:	return Easings::Ease_InOut_Cubic(ratio);
			case Easing::InOut_Quart:	return Easings::Ease_InOut_Quart(ratio);
			case Easing::InOut_Quint:	return Easings::Ease_InOut_Quint(ratio);
			case Easing::InOut_Expo:	return Easings::Ease_InOut_Expo(ratio);
			case Easing::InOut_Circ:	return Easings::Ease_InOut_Circ(ratio);
		}

		return Linear(ratio);
	}
}
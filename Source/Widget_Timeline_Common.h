#pragma once

#include "Easings.h"

namespace MIDILightDrawer
{
	// Tool types enumeration
	public enum class TimelineToolType {
		Pointer,	// Select and move
		Draw,		// Create new bars
		Split,		// Cut bars
		Erase,		// Delete bars
		Duration,	// Change Duration of bars,
		Color,		// Change Corlor of bars
		Fade,		// Create Color Fades
		Strobe		// Create Strobe Sequences
	};

	public enum class DrawToolMode {
		Draw,
		Erase,
		Move,
		Resize
	};

	public enum class TimelineCursor {
		Default,    // Standard arrow cursor
		Cross,      // Crosshair cursor for drawing/selection
		Hand,       // Hand cursor for dragging
		SizeWE,     // Horizontal resize cursor
		No,         // Not allowed cursor
		HSplit      // Horizontal split cursor
	};

	public ref class TimelineCursorHelper abstract sealed {
	public:
		static System::Windows::Forms::Cursor^ GetCursor(TimelineCursor cursorType) {
			switch (cursorType) {
			case TimelineCursor::Default:
				return System::Windows::Forms::Cursors::Default;
			case TimelineCursor::Cross:
				return System::Windows::Forms::Cursors::Cross;
			case TimelineCursor::Hand:
				return System::Windows::Forms::Cursors::Hand;
			case TimelineCursor::SizeWE:
				return System::Windows::Forms::Cursors::SizeWE;
			case TimelineCursor::No:
				return System::Windows::Forms::Cursors::No;
			case TimelineCursor::HSplit:
				return System::Windows::Forms::Cursors::HSplit;
			default:
				return System::Windows::Forms::Cursors::Default;
			}
		}
	};

	public enum class BarPreviewType
	{
		Creation,    // New bar being created
		Movement,    // Existing bar being moved
		Duration     // Duration change preview
	};

	public enum class SnappingType
	{
		Snap_None,
		Snap_Grid,
		Snap_Bars,
		Snap_Tablature
	};

	public enum class FadeType
	{
		Two_Colors,
		Three_Colors
	};

	public enum class BarEventType
	{
		Solid,
		Fade,
		Strobe
	};

	public ref class TimeSignatures abstract sealed
	{
	public:
		static initonly System::Collections::Generic::List<System::String^>^	TimeSignatureRegularStringMain;
		static initonly System::Collections::Generic::List<System::String^>^	TimeSignatureRegularStringSub;
		static initonly System::Collections::Generic::List<System::String^>^	TimeSignatureRegularStringComplete;
		static initonly System::Collections::Generic::List<int>^				TimeSignatureRegularValues;

		static initonly System::Collections::Generic::List<System::String^>^	TimeSignatureExtendedrStringMain;
		static initonly System::Collections::Generic::List<System::String^>^	TimeSignatureExtendedStringSub;
		static initonly System::Collections::Generic::List<System::String^>^	TimeSignatureExtendedStringComplete;
		static initonly System::Collections::Generic::List<int>^				TimeSignatureExtendedValues;

		static initonly System::Collections::Generic::Dictionary<int, System::String^>^ TimeSignatureLookup;

		static TimeSignatures()
		{
			TimeSignatureRegularStringMain = gcnew System::Collections::Generic::List<System::String^>;
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/1"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/2"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/4"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/32"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/4"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/32"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/32"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/32"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/8"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));
			TimeSignatureRegularStringMain->Add(gcnew System::String("1/16"));

			TimeSignatureRegularStringSub = gcnew System::Collections::Generic::List<System::String^>;
			TimeSignatureRegularStringSub->Add(gcnew System::String(""));
			TimeSignatureRegularStringSub->Add(gcnew System::String(""));
			TimeSignatureRegularStringSub->Add(gcnew System::String(""));
			TimeSignatureRegularStringSub->Add(gcnew System::String(""));
			TimeSignatureRegularStringSub->Add(gcnew System::String(""));
			TimeSignatureRegularStringSub->Add(gcnew System::String(""));
			TimeSignatureRegularStringSub->Add(gcnew System::String("T"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("T"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("T"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("T"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("5:4"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("5:4"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("5:4"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("7:8"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("7:8"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("7:8"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("20% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("40% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("60% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("80% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("100% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("20% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("40% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("60% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("80% SW"));
			TimeSignatureRegularStringSub->Add(gcnew System::String("100% SW"));

			TimeSignatureRegularStringComplete = gcnew System::Collections::Generic::List<System::String^>;
			for (int i = 0;i < TimeSignatureRegularStringMain->Count;i++) {
				TimeSignatureRegularStringComplete->Add(gcnew System::String(TimeSignatureRegularStringMain[i] + " " + TimeSignatureRegularStringSub[i]));
			}

			TimeSignatureRegularValues = gcnew System::Collections::Generic::List<int>;
			TimeSignatureRegularValues->Add(3840);
			TimeSignatureRegularValues->Add(1920);
			TimeSignatureRegularValues->Add(960);
			TimeSignatureRegularValues->Add(480);
			TimeSignatureRegularValues->Add(240);
			TimeSignatureRegularValues->Add(120);
			TimeSignatureRegularValues->Add(640);
			TimeSignatureRegularValues->Add(320);
			TimeSignatureRegularValues->Add(160);
			TimeSignatureRegularValues->Add(80);
			TimeSignatureRegularValues->Add(384);
			TimeSignatureRegularValues->Add(192);
			TimeSignatureRegularValues->Add(96);
			TimeSignatureRegularValues->Add(619);
			TimeSignatureRegularValues->Add(278);
			TimeSignatureRegularValues->Add(137);
			TimeSignatureRegularValues->Add(542);
			TimeSignatureRegularValues->Add(610);
			TimeSignatureRegularValues->Add(672);
			TimeSignatureRegularValues->Add(734);
			TimeSignatureRegularValues->Add(802);
			TimeSignatureRegularValues->Add(257);
			TimeSignatureRegularValues->Add(271);
			TimeSignatureRegularValues->Add(288);
			TimeSignatureRegularValues->Add(305);
			TimeSignatureRegularValues->Add(319);

			TimeSignatureExtendedrStringMain = gcnew System::Collections::Generic::List<System::String^>;
			TimeSignatureExtendedrStringMain->AddRange(TimeSignatureRegularStringMain);
			TimeSignatureExtendedrStringMain->Add("1/64");
			TimeSignatureExtendedrStringMain->Add("1/128");

			TimeSignatureExtendedStringSub = gcnew System::Collections::Generic::List<System::String^>;
			TimeSignatureExtendedStringSub->AddRange(TimeSignatureRegularStringSub);
			TimeSignatureExtendedStringSub->Add("");
			TimeSignatureExtendedStringSub->Add("");

			TimeSignatureExtendedStringComplete = gcnew System::Collections::Generic::List<System::String^>;
			for (int i = 0;i < TimeSignatureExtendedrStringMain->Count;i++) {
				TimeSignatureExtendedStringComplete->Add(gcnew System::String(TimeSignatureExtendedrStringMain[i] + " " + TimeSignatureExtendedStringSub[i]));
			}

			TimeSignatureExtendedValues = gcnew System::Collections::Generic::List<int>;
			TimeSignatureExtendedValues->AddRange(TimeSignatureRegularValues);
			TimeSignatureExtendedValues->Add(60);
			TimeSignatureExtendedValues->Add(30);


			TimeSignatureLookup = gcnew System::Collections::Generic::Dictionary<int, System::String^>;

			for (int i = 0;i < TimeSignatureExtendedrStringMain->Count;i++) {
				TimeSignatureLookup->Add(TimeSignatureExtendedValues[i], TimeSignatureExtendedStringComplete[i]);
			}
		}
	};

	typedef Easing FadeEasing;

	public ref class ContextMenuStrings abstract sealed
	{
	public:
		// Common menu items
		static initonly System::String^ Copy = "Copy";
		static initonly System::String^ Paste = "Paste";
		static initonly System::String^ Delete = "Delete";

		// Color-related menu items
		static initonly System::String^ ChangeColor = "Change Color";
		static initonly System::String^ ChangeColorStart = "Change Start Color";
		static initonly System::String^ ChangeColorCenter = "Change Center Color";
		static initonly System::String^ ChangeColorEnd = "Change End Color";

		// Easing related menu items
		static initonly System::String^ ChangeEasingIn = "Change Easing In";
		static initonly System::String^ ChangeEasingOut = "Change Easing Out";

		// Fade & Strobe specific menu items
		static initonly System::String^ ChangeQuantization = "Change Quantization";

		// Fade-specific menu items
		static initonly System::String^ FadeSwitchTwo = "Switch to Two Colors";
		static initonly System::String^ FadeSwitchThree = "Switch to Three Colors";

		// Strobe-specific menu items
		// All already covered

		// Separator
		static initonly System::String^ Separator = "-";

		static initonly System::Collections::Generic::Dictionary<System::String^, FadeEasing>^ FadeEasings;
		
		static initonly System::Collections::Generic::Dictionary<System::String^, int>^ QuantizationValues;

		static ContextMenuStrings()
		{
			FadeEasings = gcnew System::Collections::Generic::Dictionary<System::String^, FadeEasing>;
			FadeEasings->Add("Linear", Easing::Linear);
			FadeEasings->Add("In Sine", Easing::In_Sine);
			FadeEasings->Add("In Quad", Easing::In_Quad);
			FadeEasings->Add("In Cubic", Easing::In_Cubic);
			FadeEasings->Add("In Quart", Easing::In_Quart);
			FadeEasings->Add("In Quint", Easing::In_Quint);
			FadeEasings->Add("In Expo", Easing::In_Expo);
			FadeEasings->Add("In Circ", Easing::In_Circ);
			FadeEasings->Add("Out Sine", Easing::Out_Sine);
			FadeEasings->Add("Out Quad", Easing::Out_Quad);
			FadeEasings->Add("Out Cubic", Easing::Out_Cubic);
			FadeEasings->Add("Out Quart", Easing::Out_Quart);
			FadeEasings->Add("Out Quint", Easing::Out_Quint);
			FadeEasings->Add("Out Expo", Easing::Out_Expo);
			FadeEasings->Add("Out Circ", Easing::Out_Circ);
			FadeEasings->Add("InOut Sine", Easing::InOut_Sine);
			FadeEasings->Add("InOut Quad", Easing::InOut_Quad);
			FadeEasings->Add("InOut Cubic", Easing::InOut_Cubic);
			FadeEasings->Add("InOut Quart", Easing::InOut_Quart);
			FadeEasings->Add("InOut Quint", Easing::InOut_Quint);
			FadeEasings->Add("InOut Expo", Easing::InOut_Expo);
			FadeEasings->Add("InOut Circ", Easing::InOut_Circ);


			QuantizationValues = gcnew System::Collections::Generic::Dictionary<System::String^, int>;
			QuantizationValues->Add(L"1/1", 3840);
			QuantizationValues->Add(L"1/2", 1920);
			QuantizationValues->Add(L"1/4", 960);
			QuantizationValues->Add(L"1/8", 480);
			QuantizationValues->Add(L"1/16", 240);
			QuantizationValues->Add(L"1/32", 120);
			QuantizationValues->Add(L"1/64", 60);
			QuantizationValues->Add(L"1/128", 30);

			QuantizationValues->Add(L"1/4 (T)", 640);
			QuantizationValues->Add(L"1/8 (T)", 320);
			QuantizationValues->Add(L"1/16 (T)", 160);
			QuantizationValues->Add(L"1/32 (T)", 80);

			QuantizationValues->Add(L"1/8 (5:4)", 384);
			QuantizationValues->Add(L"1/16 (5:4)", 192);
			QuantizationValues->Add(L"1/32 (5:4)", 96);

			QuantizationValues->Add(L"1/8 (7:8)", 619);
			QuantizationValues->Add(L"1/16 (7:8)", 278);
			QuantizationValues->Add(L"1/32 (7:8)", 137);

			QuantizationValues->Add(L"1/8 (20% SW)", 542);
			QuantizationValues->Add(L"1/8 (40% SW)", 610);
			QuantizationValues->Add(L"1/8 (60% SW)", 672);
			QuantizationValues->Add(L"1/8 (80% SW)", 734);
			QuantizationValues->Add(L"1/8 (100% SW)", 802);

			QuantizationValues->Add(L"1/16 (20% SW)", 257);
			QuantizationValues->Add(L"1/16 (40% SW)", 271);
			QuantizationValues->Add(L"1/16 (60% SW)", 288);
			QuantizationValues->Add(L"1/16 (80% SW)", 305);
			QuantizationValues->Add(L"1/16 (100% SW)", 319);
		}
	};

	public enum class LoadingStage
	{
		Images,
		TabText,
		DrumSymbols,
		DurationSymbols,
		Complete
	};
}
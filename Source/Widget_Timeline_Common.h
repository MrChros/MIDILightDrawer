#pragma once

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

		// Fade & Strobe specific menu items
		static initonly System::String^ ChangeQuantization = "Change Quantization";

		// Fade-specific menu items
		static initonly System::String^ FadeSwitchTwo = "Switch to Two Colors";
		static initonly System::String^ FadeSwitchThree = "Switch to Three Colors";

		// Strobe-specific menu items
		// All already covered

		// Separator
		static initonly System::String^ Separator = "-";


		static initonly System::Collections::Generic::Dictionary<System::String^, int>^ QuantizationValues;
		static ContextMenuStrings()
		{
			QuantizationValues = gcnew System::Collections::Generic::Dictionary<System::String^, int>;
			QuantizationValues->Add(L"1/1", 3840);
			QuantizationValues->Add(L"1/2", 1920);
			QuantizationValues->Add(L"1/4", 960);
			QuantizationValues->Add(L"1/8", 480);
			QuantizationValues->Add(L"1/16", 240);
			QuantizationValues->Add(L"1/32", 120);

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
}
#pragma once

namespace MIDILightDrawer {
	// Tool types enumeration
	public enum class TimelineToolType {
		Pointer,	// Select and move
		Draw,		// Create new bars
		Split,		// Cut bars
		Erase,		// Delete bars
		Duration,	// Change Duration of bars,
		Color		// Change Corlor of bars
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
}
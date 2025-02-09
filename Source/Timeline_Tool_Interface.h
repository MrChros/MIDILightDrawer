#pragma once

using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
    // Forward declarations
    ref class Track;
    ref class BarEvent;
    enum class DrawToolMode;
    enum class TimelineToolType;

    value struct TrackButtonId;

    // Interface for tool state access
    public interface class ITimelineToolAccess
    {
        // TimelineTool abstract
		property List<BarEvent^>^ SelectedBars { List<BarEvent^>^ get(); }
		property List<BarEvent^>^ PreviewBars { List<BarEvent^>^ get(); }
		property bool IsSelecting { bool get(); }
		property System::Drawing::Rectangle SelectionRect { System::Drawing::Rectangle get(); }
		property System::Drawing::Point CurrentMousePosition { System::Drawing::Point get(); }
		
		// Pointer Tool
        property bool IsDragging { bool get(); }
        property bool IsMultiTrackSelection { bool get(); }
        property bool IsPasting { bool get(); }
        property List<BarEvent^>^ PastePreviewBars { List<BarEvent^>^ get(); }

        // Draw Tool
		property System::Drawing::Color DrawColor { System::Drawing::Color get(); }
		property int DrawTickLength { int get(); }
		property Track^ TargetTrack { Track^ get(); }
		property DrawToolMode CurrentMode { DrawToolMode  get(); }
		property bool IsResizing { bool get(); }

        // Erase Tool
		property BarEvent^ HoveredBar{ BarEvent ^ get(); }

        // Duration Tool
		// All properties already covered by above definitions

        // Color Tool
        property System::Drawing::Color CurrentColor { System::Drawing::Color get(); }
        property float BarXHoverRatio { float get(); }

        // Fade Tool
		// All properties already covered by above definitions

        // Strobe Tool
        // All properties already covered by above definitions
  
		void OnCommandStateChanged();
    };

    // Interface for timeline access
    public interface class ITimelineAccess
    {
        TimelineToolType CurrentToolType();
        ITimelineToolAccess^ ToolAccess();
        TrackButtonId HoverButton();
    };
}
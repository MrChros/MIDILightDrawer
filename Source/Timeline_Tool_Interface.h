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
        // Pointer Tool
        property List<BarEvent^>^ SelectedBars { List<BarEvent^>^ get(); }
        property System::Drawing::Rectangle SelectionRect { System::Drawing::Rectangle get(); }
        property bool IsDragging { bool get(); }
        property Track^ DragSourceTrack { Track^ get(); }
        property Track^ DragTargetTrack { Track^ get(); }
        property bool IsMultiTrackSelection { bool get(); }
        property System::Drawing::Point CurrentMousePosition { System::Drawing::Point get(); }
        property bool IsPasting { bool get(); }
        property List<BarEvent^>^ PastePreviewBars { List<BarEvent^>^ get(); }

        // Draw Tool
		property System::Drawing::Color DrawColor { System::Drawing::Color get(); }
		property int DrawTickLength { int get(); }
		property BarEvent^ PreviewBar { BarEvent^ get(); }
		property Track^ TargetTrack { Track^ get(); }
		property Track^ SourceTrack { Track^  get(); }
		property DrawToolMode CurrentMode { DrawToolMode  get(); }
		property bool IsMoving  { bool get(); }
		property bool IsResizing { bool get(); }
		property BarEvent^ SelectedBar { BarEvent^ get(); }

        // Erase Tool
        property System::Drawing::Rectangle ErasePreviewRect { System::Drawing::Rectangle get(); }

        // Duration Tool
		// All properties already covered by above definitions

        // Color Tool
        property System::Drawing::Rectangle PreviewRect{ System::Drawing::Rectangle get(); }
        property System::Drawing::Color CurrentColor { System::Drawing::Color get(); }
        property float BarXHoverRatio { float get(); }

        // Fade Tool
        property List<BarEvent^>^ PreviewBars { List<BarEvent^> ^ get(); }

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
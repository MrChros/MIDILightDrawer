#pragma once

using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
    // Forward declarations
    ref class Track;
    ref class BarEvent;
    enum class TimelineToolType;

    // Interface for tool state access
    public interface class ITimelineToolAccess
    {
        property List<BarEvent^>^ SelectedBars { List<BarEvent^>^ get(); }
        property System::Drawing::Rectangle SelectionRect { System::Drawing::Rectangle get(); }
        property bool IsDragging { bool get(); }
        property Track^ DragSourceTrack { Track^ get(); }
        property Track^ DragTargetTrack { Track^ get(); }
        property System::Drawing::Point CurrentMousePosition { System::Drawing::Point get(); }
        property bool IsPasting { bool get(); }
        property List<BarEvent^>^ PastePreviewBars { List<BarEvent^>^ get(); }
    };

    // Interface for timeline access
    public interface class ITimelineAccess
    {
        TimelineToolType GetCurrentToolType();
        ITimelineToolAccess^ GetToolAccess();
    };
}
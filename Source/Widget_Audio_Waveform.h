#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Track;
	ref class Widget_Timeline;
	ref class Waveform_Render_Data;
	
	// Defines the interaction mode when clicking/dragging on the waveform mini-map
	public enum class WaveformInteractionMode
	{
		None,           // No interaction
		SeekCursor,     // Click to seek playback position
		DragViewport,   // Dragging the viewport rectangle
		ResizeLeft,     // Resizing viewport from left edge
		ResizeRight     // Resizing viewport from right edge
	};

	// Defines what elements are displayed in the mini-map
	// [Flags]
	public enum class MiniMapDisplayFlags
	{
		None		= 0,
		Waveform	= 1 << 0,   // Show audio waveform
		Events		= 1 << 1,   // Show bar events as colored blocks
		Markers		= 1 << 2,   // Show measure markers
		Viewport	= 1 << 3,   // Show viewport indicator
		Cursor		= 1 << 4,   // Show playback cursor
		LoopRegion	= 1 << 5,   // Show loop in/out region
		All = Waveform | Events | Markers | Viewport | Cursor | LoopRegion
	};

	public ref class Widget_Audio_Waveform : UserControl
	{
	public:
		static const double NO_AUDIO = -1.0;
		static const double NO_MIDI = -1.0;
		static const int MINI_MAP_TRACK_HEIGHT		= 10;	// Height per track in mini-map
		static const int MINI_MAP_TRACK_SPACING		= 6;
		static const float MINI_MAP_EVENT_OPACITY	= 0.7f;	// Opacity of event blocks (0.0 - 1.0)
		static const int VIEWPORT_HANDLE_WIDTH		= 6;	// Width of resize handles

		event EventHandler<double>^ OnCursorPositionChanged;
		event EventHandler<double>^ OnViewportScrollChanged;		// Request to scroll view to start at this ms (no zoom change)
		event EventHandler<array<double>^>^ OnViewportRangeChanged; // Request to set view to [start_ms, end_ms]

	private:
		Widget_Timeline^ _Timeline;
		Waveform_Render_Data^ _Waveform_Data;

		double _Audio_Duration_ms;
		double _MIDI_Duration_ms;
		double _Maximum_Duration_ms;
		double _Cursor_Position_ms;

		// View range from timeline
		double _View_Range_Start_ms;
		double _View_Range_End_ms;

		List<double>^ _Marker_Timestamps;

		// Event visualization cache
		bool _Events_Cache_Valid;
		Bitmap^ _Events_Cache_Bitmap;
		int _Last_Cache_Width;
		int _Last_Cache_Height;

		// Interaction state
		WaveformInteractionMode _Interaction_Mode;
		double _Drag_Start_Position_ms;
		double _Drag_Start_View_Start_ms;
		double _Drag_Start_View_End_ms;
		int _Drag_Start_X;
		bool _Is_Mouse_Over_Viewport;
		bool _Is_Mouse_Over_Left_Handle;
		bool _Is_Mouse_Over_Right_Handle;

		// Display settings
		MiniMapDisplayFlags _Display_Flags;

		// Colors (cached from theme)
		Color _Color_Viewport_Fill;
		Color _Color_Viewport_Border;
		Color _Color_Viewport_Handle;
		Color _Color_Cursor;
		Color _Color_Loop_Region;
		Color _Color_Marker;

	public:
		Widget_Audio_Waveform(List<double>^ marker_timestamp_list);

		void Set_Widget_Timeline(Widget_Timeline^ timeline);
		void Set_Waveform_Data(Waveform_Render_Data^ waveform_data);
		void Set_Audio_Duration_ms(double audio_duration_ms);
		void Set_MIDI_Duration_ms(double midi_duration_ms);
		void Set_Cursor_Position_ms(double cursor_position_ms);
		void Set_View_Range_ms(double start_ms, double end_ms);

		// New mini-map methods
		void Invalidate_Event_Cache();

		// Display configuration
		void Set_Display_Flags(MiniMapDisplayFlags flags);

		// Navigation helpers
		void Zoom_To_Range(double start_ms, double end_ms);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnMouseLeave(EventArgs^ e) override;

	private:
		int ms_To_Pixel_Maximum(double ms);
		double Pixel_To_ms_Maximum(int pixel);

		int ms_To_Pixel_MIDI(double ms);
		double Pixel_To_ms_MIDI(int pixel);

		void Update_Max_Duration();

		// Rendering helpers
		void Render_Waveform(Graphics^ g);
		void Render_Events_Cached(Graphics^ g);
		void Rebuild_Event_Cache();
		void Render_Markers(Graphics^ g);
		void Render_Viewport(Graphics^ g);
		void Render_Cursor(Graphics^ g);
		void Render_Audio_Overflow_Region(Graphics^ g);

		// Interaction helpers
		void Update_Hover_State(Point mouse_pos);
		void Update_Cursor_Appearance();
		bool Is_Over_Viewport(Point mouse_pos);
		bool Is_Over_Left_Handle(Point mouse_pos);
		bool Is_Over_Right_Handle(Point mouse_pos);
		bool Is_Within_MIDI_Region(int pixel_x);

		// Theme helpers
		void Update_Colors_From_Theme();
	};
}


#include "Widget_Audio_Waveform.h"

#include "Theme_Manager.h"
#include "Widget_Timeline.h"
#include "Playback_Audio_Engine.h"
#include "Widget_Timeline_Classes.h"

namespace MIDILightDrawer
{
	Widget_Audio_Waveform::Widget_Audio_Waveform(List<double>^ marker_timestamp_list)
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);

		this->_Timeline = nullptr;
		this->_Marker_Timestamps = marker_timestamp_list;

		Set_Waveform_Data(nullptr);
		Set_Audio_Duration_ms(Widget_Audio_Waveform::NO_AUDIO);
		Set_MIDI_Duration_ms(Widget_Audio_Waveform::NO_MIDI);
		Set_Cursor_Position_ms(0.0);

		_View_Range_Start_ms = 0.0;
		_View_Range_End_ms = 0.0;

		// Initialize waveform cache
		_Waveform_Cache_Valid = false;
		_Waveform_Cache_Bitmap = nullptr;
		_Waveform_Cache_Width = 0;
		_Waveform_Cache_Height = 0;
		_Waveform_Cache_Audio_Duration = 0.0;

		// Initialize event visualization
		_Events_Render_Enabled = true;
		_Events_Cache_Valid = false;
		_Events_Cache_Bitmap = nullptr;
		_Last_Cache_Width = 0;
		_Last_Cache_Height = 0;

		// Initialize interaction state
		_Interaction_Mode = WaveformInteractionMode::None;
		_Drag_Start_Position_ms = 0.0;
		_Drag_Start_View_Start_ms = 0.0;
		_Drag_Start_View_End_ms = 0.0;
		_Drag_Start_X = 0;
		_Is_Mouse_Over_Viewport = false;
		_Is_Mouse_Over_Left_Handle = false;
		_Is_Mouse_Over_Right_Handle = false;

		// Initialize display settings

		// Initialize colors from theme
		Update_Colors_From_Theme();
	}

	void Widget_Audio_Waveform::Set_Widget_Timeline(Widget_Timeline^ timeline)
	{
		if (timeline) {
			this->_Timeline = timeline;

			Invalidate_Event_Cache();
		}
	}

	void Widget_Audio_Waveform::Set_Waveform_Data(Waveform_Render_Data^ waveform_data)
	{
		this->_Waveform_Data = waveform_data;

		Invalidate_Waveform_Cache();
	}

	void Widget_Audio_Waveform::Set_Audio_Duration_ms(double audio_duration_ms)
	{
		if (audio_duration_ms == 0) {
			return;
		}

		this->_Audio_Duration_ms = audio_duration_ms;

		Update_Max_Duration();
		Invalidate_Waveform_Cache();
		Invalidate_Event_Cache();

		this->Invalidate();
	}
	
	void Widget_Audio_Waveform::Set_MIDI_Duration_ms(double midi_duration_ms)
	{
		if (midi_duration_ms == 0) {
			return;
		}
		
		this->_MIDI_Duration_ms = midi_duration_ms;

		Update_Max_Duration();
		Invalidate_Waveform_Cache();
		Invalidate_Event_Cache();

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Set_Cursor_Position_ms(double cursor_position_ms)
	{
		this->_Cursor_Position_ms = cursor_position_ms;

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Set_View_Range_ms(double start_ms, double end_ms)
	{
		this->_View_Range_Start_ms = start_ms;
		this->_View_Range_End_ms = end_ms;

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Invalidate_Event_Cache()
	{
		_Events_Cache_Valid = false;

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Invalidate_Waveform_Cache()
	{
		_Waveform_Cache_Valid = false;

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Set_Events_Render_Enabled(bool enabled)
	{
		_Events_Render_Enabled = enabled;

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Zoom_To_Range(double start_ms, double end_ms)
	{
		array<double>^ Range = gcnew array<double>(2);
		Range[0] = start_ms;
		Range[1] = end_ms;
		OnViewportRangeChanged(this, Range);
	}

	void Widget_Audio_Waveform::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Render layers in order (back to front)
		Render_Waveform_Cached(G);

		if (_Events_Render_Enabled) {
			Render_Events_Cached(G);
		}

		Render_Audio_Overflow_Region(G);
		Render_Markers(G);
		Render_Viewport(G);
		Render_Cursor(G);
	}

	void Widget_Audio_Waveform::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		Invalidate_Waveform_Cache();
		Invalidate_Event_Cache();
		this->Invalidate();
	}

	void Widget_Audio_Waveform::OnMouseDown(MouseEventArgs^ e)
	{
		Point Mouse_Pos = Point(e->X, e->Y);

		if (e->Button == Windows::Forms::MouseButtons::Left)
		{
			// Check what we're clicking on
			if (Is_Over_Left_Handle(Mouse_Pos))
			{
				_Interaction_Mode = WaveformInteractionMode::ResizeLeft;
				_Drag_Start_X = e->X;
				_Drag_Start_View_Start_ms = _View_Range_Start_ms;
				_Drag_Start_View_End_ms = _View_Range_End_ms;
			}
			else if (Is_Over_Right_Handle(Mouse_Pos))
			{
				_Interaction_Mode = WaveformInteractionMode::ResizeRight;
				_Drag_Start_X = e->X;
				_Drag_Start_View_Start_ms = _View_Range_Start_ms;
				_Drag_Start_View_End_ms = _View_Range_End_ms;
			}
			else if (Is_Over_Viewport(Mouse_Pos))
			{
				_Interaction_Mode = WaveformInteractionMode::DragViewport;
				_Drag_Start_X = e->X;
				_Drag_Start_View_Start_ms = _View_Range_Start_ms;
				_Drag_Start_View_End_ms = _View_Range_End_ms;
			}
			else if (Is_Within_MIDI_Region(Mouse_Pos.X))
			{
				// Click outside viewport - seek to position
				_Interaction_Mode = WaveformInteractionMode::SeekCursor;
			}

			this->Capture = true;
		}
	}

	void Widget_Audio_Waveform::OnMouseMove(MouseEventArgs^ e)
	{
		Point Mouse_Pos = Point(e->X, e->Y);

		if (_Interaction_Mode == WaveformInteractionMode::DragViewport)
		{
			// Calculate delta in milliseconds
			int Delta_Pixels = e->X - _Drag_Start_X;
			double Delta_ms = (double)Delta_Pixels / (double)this->Width * _Maximum_Duration_ms;

			double New_Start = _Drag_Start_View_Start_ms + Delta_ms;
			double New_End = _Drag_Start_View_End_ms + Delta_ms;

			// Clamp to valid range
			double View_Width = New_End - New_Start;
			if (New_Start < 0)
			{
				New_Start = 0;
				New_End = View_Width;
			}
			if (New_End > _Maximum_Duration_ms)
			{
				New_End = _Maximum_Duration_ms;
				New_Start = New_End - View_Width;
			}

			OnViewportScrollChanged(this, New_Start);
		}
		else if (_Interaction_Mode == WaveformInteractionMode::ResizeLeft)
		{
			double New_Start = Pixel_To_ms_Maximum(e->X);

			// Ensure minimum viewport width (50ms minimum)
			double Min_Width = 50.0;
			if (_Drag_Start_View_End_ms - New_Start < Min_Width) {
				New_Start = _Drag_Start_View_End_ms - Min_Width;
			}
			if (New_Start < 0) {
				New_Start = 0;
			}

			array<double>^ Range = gcnew array<double>(2);
			Range[0] = New_Start;
			Range[1] = _Drag_Start_View_End_ms;
			OnViewportRangeChanged(this, Range);
		}
		else if (_Interaction_Mode == WaveformInteractionMode::ResizeRight)
		{
			double New_End = Pixel_To_ms_Maximum(e->X);

			// Ensure minimum viewport width (50ms minimum)
			double Min_Width = 50.0;
			if (New_End - _Drag_Start_View_Start_ms < Min_Width) {
				New_End = _Drag_Start_View_Start_ms + Min_Width;
			}
			if (New_End > _Maximum_Duration_ms) {
				New_End = _Maximum_Duration_ms;
			}

			array<double>^ Range = gcnew array<double>(2);
			Range[0] = _Drag_Start_View_Start_ms;
			Range[1] = New_End;
			OnViewportRangeChanged(this, Range);
		}
		else if (_Interaction_Mode == WaveformInteractionMode::SeekCursor)
		{
			// Do Nothing...
		}
		else
		{
			// No active interaction - update hover state
			Update_Hover_State(Mouse_Pos);
			Update_Cursor_Appearance();
		}
	}

	void Widget_Audio_Waveform::OnMouseUp(MouseEventArgs^ e)
	{
		Point Mouse_Pos = Point(e->X, e->Y);
		
		if ((_Interaction_Mode == WaveformInteractionMode::SeekCursor) ||
			(_Interaction_Mode == WaveformInteractionMode::DragViewport && Math::Abs(_Drag_Start_X - e->X) < 1))
		{
			double Cursor_Position_ms = Pixel_To_ms_Maximum(Mouse_Pos.X);
			OnCursorPositionChanged(this, Cursor_Position_ms);
		}
		
		_Interaction_Mode = WaveformInteractionMode::None;
		this->Capture = false;

		Update_Hover_State(Point(e->X, e->Y));
		Update_Cursor_Appearance();
	}

	void Widget_Audio_Waveform::OnMouseLeave(EventArgs^ e)
	{
		_Is_Mouse_Over_Viewport = false;
		_Is_Mouse_Over_Left_Handle = false;
		_Is_Mouse_Over_Right_Handle = false;

		this->Cursor = Cursors::Default;
	}

	int Widget_Audio_Waveform::ms_To_Pixel_Maximum(double ms)
	{
		if (_Maximum_Duration_ms <= 0) {
			return 0;
		}

		return (int)((double)this->Width * (ms / _Maximum_Duration_ms));
	}

	double Widget_Audio_Waveform::Pixel_To_ms_Maximum(int pixel)
	{
		if (this->Width <= 0) {
			return 0.0;
		}

		return _Maximum_Duration_ms * (double)pixel / (double)this->Width;
	}

	int Widget_Audio_Waveform::ms_To_Pixel_MIDI(double ms)
	{
		if (_MIDI_Duration_ms <= 0) {
			return 0;
		}

		double MIDI_Width = (double)this->Width * _MIDI_Duration_ms / _Maximum_Duration_ms;

		return (int)(MIDI_Width * (ms / _MIDI_Duration_ms));
	}

	double Widget_Audio_Waveform::Pixel_To_ms_MIDI(int pixel)
	{
		if (this->Width <= 0) {
			return 0.0;
		}

		double MIDI_Width = (double)this->Width * _MIDI_Duration_ms / _Maximum_Duration_ms;

		return _MIDI_Duration_ms * (double)pixel / MIDI_Width;
	}

	void Widget_Audio_Waveform::Update_Max_Duration()
	{
		this->_Maximum_Duration_ms = Math::Max(this->_Audio_Duration_ms, this->_MIDI_Duration_ms);
	}

	void Widget_Audio_Waveform::Render_Waveform_Cached(Graphics^ g)
	{
		if (this->_Waveform_Data == nullptr) {
			return;
		}

		// Check if we need to rebuild the cache
		if (_Waveform_Cache_Valid == false ||
			_Waveform_Cache_Bitmap == nullptr ||
			_Waveform_Cache_Width != this->Width ||
			_Waveform_Cache_Height != this->Height ||
			_Waveform_Cache_Audio_Duration != this->_Audio_Duration_ms)
		{
			Rebuild_Waveform_Cache();
		}

		// Draw the cached bitmap
		if (_Waveform_Cache_Bitmap != nullptr)
		{
			g->DrawImage(_Waveform_Cache_Bitmap, 0, 0);
		}
	}

	void Widget_Audio_Waveform::Rebuild_Waveform_Cache()
	{
		if (this->_Waveform_Data == nullptr || this->Width <= 0 || this->Height <= 0) {
			return;
		}

		// Clean up old cache
		if (_Waveform_Cache_Bitmap != nullptr) {
			delete _Waveform_Cache_Bitmap;
		}

		_Waveform_Cache_Bitmap = gcnew Bitmap(this->Width, this->Height);
		_Waveform_Cache_Width = this->Width;
		_Waveform_Cache_Height = this->Height;
		_Waveform_Cache_Audio_Duration = this->_Audio_Duration_ms;

		Graphics^ G = Graphics::FromImage(_Waveform_Cache_Bitmap);
		G->Clear(Color::Transparent);
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Render waveform to cache
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		int Segment_Count = _Waveform_Data->TotalSegments;

		float Pixel_Width_Audio = (float)((double)this->Width * (this->_Audio_Duration_ms / this->_Maximum_Duration_ms));
		float Pixels_Per_Segment = Pixel_Width_Audio / (float)Segment_Count;

		SolidBrush^ Waveform_Brush = gcnew System::Drawing::SolidBrush(Theme->BorderStrong);
		Rectangle Waveform_Rect = Rectangle(0, 0, (int)(Pixels_Per_Segment)+2, 0);

		float Min;
		float Max;
		float X = 0;

		for (int i = 0; i < Segment_Count; i++) {

			_Waveform_Data->GetSegmentMinMax(i, Min, Max);

			int Segment_Upper = (int)(this->Height / 2 * Max);
			int Segment_Lower = abs((int)(this->Height / 2 * Min));

			if (Segment_Upper == 0) {
				Segment_Upper = 1;
			}

			if (Segment_Lower == 0) {
				Segment_Lower = 1;
			}

			Waveform_Rect.X = (int)X;
			Waveform_Rect.Y = (int)((this->Height / 2) - Segment_Upper);
			Waveform_Rect.Height = Segment_Upper + Segment_Lower;

			G->FillRectangle(Waveform_Brush, Waveform_Rect);

			X += Pixels_Per_Segment;
		}

		delete Waveform_Brush;
		delete G;

		_Waveform_Cache_Valid = true;
	}

	void Widget_Audio_Waveform::Render_Events_Cached(Graphics^ g)
	{
		if (this->_Timeline == nullptr || this->_Timeline->Tracks == nullptr || this->_Timeline->Tracks->Count == 0) return;

		// Check if we need to rebuild the cache
		if (!_Events_Cache_Valid ||
			_Events_Cache_Bitmap == nullptr ||
			_Last_Cache_Width != this->Width ||
			_Last_Cache_Height != this->Height)
		{
			Rebuild_Event_Cache();
		}

		// Draw the cached bitmap
		if (_Events_Cache_Bitmap != nullptr)
		{
			g->DrawImage(_Events_Cache_Bitmap, 0, 0);
		}
	}

	void Widget_Audio_Waveform::Rebuild_Event_Cache()
	{
		if (this->_Timeline == nullptr || this->_Timeline->Tracks == nullptr || this->Width <= 0 || this->Height <= 0) {
			return;
		}

		// Clean up old cache
		if (_Events_Cache_Bitmap != nullptr) {
			delete _Events_Cache_Bitmap;
		}

		_Events_Cache_Bitmap = gcnew Bitmap(this->Width, this->Height);
		_Last_Cache_Width = this->Width;
		_Last_Cache_Height = this->Height;

		Graphics^ G = Graphics::FromImage(_Events_Cache_Bitmap);
		G->Clear(Color::Transparent);

		// Calculate layout
		int Track_Count = this->_Timeline->Tracks->Count;
		if (Track_Count == 0) {
			delete G;
			_Events_Cache_Valid = true;
			return;
		}

		int Total_Events_Height = Track_Count * MINI_MAP_TRACK_HEIGHT + (Track_Count - 1) * MINI_MAP_TRACK_SPACING;
		int Available_Height = this->Height;


		int Track_Height = Math::Min(MINI_MAP_TRACK_HEIGHT, (Available_Height - (Track_Count - 1) * MINI_MAP_TRACK_SPACING) / Track_Count);
		Track_Height = Math::Max(2, Track_Height);

		// Center vertically if we have extra space
		int Y_Offset = (Available_Height - Total_Events_Height) / 2;
		Y_Offset = Math::Max(0, Y_Offset);

		// Calculate alpha from opacity
		int Alpha = (int)(MINI_MAP_EVENT_OPACITY * 255);

		// Draw events for each track
		for (int Track_Index = 0; Track_Index < Track_Count; Track_Index++)
		{
			Track^ Current_Track = this->_Timeline->Tracks[Track_Index];
			if (Current_Track == nullptr) {
				continue;
			}

			int Track_Y = Y_Offset + (Track_Index * Track_Height) + (Track_Index * MINI_MAP_TRACK_SPACING);

			for each (BarEvent^ Event in Current_Track->Events)
			{
				if (Event == nullptr) {
					continue;
				}

				// Convert tick to ms
				// For now, we assume events store time in ticks and we need to convert
				// This may need adjustment based on your actual data model
				double Start_ms = 0;
				double End_ms = 0;

				// If we have measure data, we should convert properly
				// For simplicity, we'll use a rough conversion assuming 120 BPM
				// In practice, you'd want to use the timeline's TicksToMilliseconds
				int Start_Tick = Event->StartTick;
				int End_Tick = Event->EndTick;

				// Rough conversion: at 120 BPM, 960 ticks = 500ms (1 quarter note)
				Start_ms = this->_Timeline->TicksToMilliseconds(Start_Tick);
				End_ms = this->_Timeline->TicksToMilliseconds(End_Tick);

				int X_Start = ms_To_Pixel_MIDI(Start_ms);
				int X_End = ms_To_Pixel_MIDI(End_ms);
				int Width = Math::Max(1, X_End - X_Start);

				// Get event color with opacity
				Color Event_Color = Event->Color;
				Color Draw_Color = Color::FromArgb(Alpha, Event_Color);

				SolidBrush^ Event_Brush = gcnew SolidBrush(Draw_Color);
				G->FillRectangle(Event_Brush, X_Start, Track_Y, Width, Track_Height);
				delete Event_Brush;
			}
		}

		delete G;
		_Events_Cache_Valid = true;
	}

	void Widget_Audio_Waveform::Render_Markers(Graphics^ g)
	{
		SolidBrush^ Marker_Brush = gcnew SolidBrush(_Color_Marker);
		Rectangle Marker_Rect = Rectangle(0, 0, 1, 10);

		for each (double Timestamp_ms in this->_Marker_Timestamps)
		{
			Marker_Rect.X = ms_To_Pixel_Maximum(Timestamp_ms);
			g->FillRectangle(Marker_Brush, Marker_Rect);
		}

		delete Marker_Brush;
	}

	void Widget_Audio_Waveform::Render_Viewport(Graphics^ g)
	{
		if (this->_View_Range_End_ms <= this->_View_Range_Start_ms || this->_Maximum_Duration_ms <= 0)
			return;

		int Start_Pixel = ms_To_Pixel_Maximum(_View_Range_Start_ms);
		int End_Pixel = ms_To_Pixel_Maximum(_View_Range_End_ms);

		// Clamp to widget bounds
		Start_Pixel = Math::Max(0, Start_Pixel);
		End_Pixel = Math::Min(this->Width, End_Pixel);

		if (End_Pixel <= Start_Pixel) {
			return;
		}

		int Viewport_Width = End_Pixel - Start_Pixel;

		// Draw semi-transparent fill for visible region
		SolidBrush^ View_Range_Brush = gcnew SolidBrush(_Color_Viewport_Fill);
		Rectangle View_Range_Rect = Rectangle(Start_Pixel, 0, Viewport_Width, this->Height);
		g->FillRectangle(View_Range_Brush, View_Range_Rect);
		delete View_Range_Brush;

		// Draw border lines at start and end
		Pen^ View_Range_Pen = gcnew Pen(_Color_Viewport_Border, 1.0f);
		g->DrawLine(View_Range_Pen, Start_Pixel, 0, Start_Pixel, this->Height);
		g->DrawLine(View_Range_Pen, End_Pixel - 1, 0, End_Pixel - 1, this->Height);
		delete View_Range_Pen;
	}

	void Widget_Audio_Waveform::Render_Cursor(Graphics^ g)
	{
		if (this->_MIDI_Duration_ms <= Widget_Audio_Waveform::NO_MIDI) return;

		int Pixel_Width_MIDI = ms_To_Pixel_Maximum(this->_MIDI_Duration_ms);
		int Cursor_Pixel_Position = (int)((double)Pixel_Width_MIDI * (this->_Cursor_Position_ms / this->_MIDI_Duration_ms));

		if (Cursor_Pixel_Position >= Pixel_Width_MIDI) {
			Cursor_Pixel_Position = Pixel_Width_MIDI - 1;
		}

		SolidBrush^ Cursor_Brush = gcnew SolidBrush(_Color_Cursor);
		Rectangle Cursor_Rect = Rectangle(Cursor_Pixel_Position, 0, 2, this->Height);

		g->FillRectangle(Cursor_Brush, Cursor_Rect);

		delete Cursor_Brush;
	}

	void Widget_Audio_Waveform::Render_Audio_Overflow_Region(Graphics^ g)
	{
		if (this->_Audio_Duration_ms <= this->_MIDI_Duration_ms) return;

		HatchBrush^ Audio_Only_Brush = gcnew HatchBrush(HatchStyle::BackwardDiagonal, Color::Yellow, Color::FromArgb(0));

		int Last_Pixel_MIDI = ms_To_Pixel_Maximum(this->_MIDI_Duration_ms);
		Rectangle Audio_Only_Rect = Rectangle(Last_Pixel_MIDI, 0, this->Width - Last_Pixel_MIDI, this->Height);

		g->FillRectangle(Audio_Only_Brush, Audio_Only_Rect);

		delete Audio_Only_Brush;
	}

	void Widget_Audio_Waveform::Update_Hover_State(Point mouse_pos)
	{
		_Is_Mouse_Over_Left_Handle	= Is_Over_Left_Handle(mouse_pos);
		_Is_Mouse_Over_Right_Handle = Is_Over_Right_Handle(mouse_pos);
		_Is_Mouse_Over_Viewport		= Is_Over_Viewport(mouse_pos) && !_Is_Mouse_Over_Left_Handle && !_Is_Mouse_Over_Right_Handle;
	}

	void Widget_Audio_Waveform::Update_Cursor_Appearance()
	{
		if (_Is_Mouse_Over_Left_Handle || _Is_Mouse_Over_Right_Handle)
		{
			this->Cursor = Cursors::SizeWE;
		}
		else if (_Is_Mouse_Over_Viewport)
		{
			this->Cursor = Cursors::Hand;
		}
		else
		{
			this->Cursor = Cursors::Default;
		}
	}

	bool Widget_Audio_Waveform::Is_Over_Viewport(Point mouse_pos)
	{
		if (_View_Range_End_ms <= _View_Range_Start_ms) {
			return false;
		}

		int Start_Pixel = ms_To_Pixel_Maximum(_View_Range_Start_ms);
		int End_Pixel = ms_To_Pixel_Maximum(_View_Range_End_ms);

		return mouse_pos.X >= Start_Pixel && mouse_pos.X <= End_Pixel;
	}

	bool Widget_Audio_Waveform::Is_Over_Left_Handle(Point mouse_pos)
	{
		if (_View_Range_End_ms <= _View_Range_Start_ms) {
			return false;
		}

		int Start_Pixel = ms_To_Pixel_Maximum(_View_Range_Start_ms);
		int End_Pixel = ms_To_Pixel_Maximum(_View_Range_End_ms);

		// Only show handles if viewport is wide enough
		if ((End_Pixel - Start_Pixel) < VIEWPORT_HANDLE_WIDTH * 3) {
			return false;
		}

		return mouse_pos.X >= Start_Pixel && mouse_pos.X <= Start_Pixel + VIEWPORT_HANDLE_WIDTH;
	}

	bool Widget_Audio_Waveform::Is_Over_Right_Handle(Point mouse_pos)
	{
		if (_View_Range_End_ms <= _View_Range_Start_ms) return false;

		int Start_Pixel = ms_To_Pixel_Maximum(_View_Range_Start_ms);
		int End_Pixel = ms_To_Pixel_Maximum(_View_Range_End_ms);

		// Only show handles if viewport is wide enough
		if ((End_Pixel - Start_Pixel) < VIEWPORT_HANDLE_WIDTH * 3) {
			return false;
		}

		return mouse_pos.X >= End_Pixel - VIEWPORT_HANDLE_WIDTH && mouse_pos.X <= End_Pixel;
	}

	bool Widget_Audio_Waveform::Is_Within_MIDI_Region(int pixel_x)
	{
		int Pixel_Width_MIDI = ms_To_Pixel_Maximum(this->_MIDI_Duration_ms);

		return pixel_x >= 0 && pixel_x <= Pixel_Width_MIDI;
	}

	void Widget_Audio_Waveform::Update_Colors_From_Theme()
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		_Color_Viewport_Fill = Color::FromArgb(40, Theme->AccentPrimary);
		_Color_Viewport_Border = Color::FromArgb(150, Theme->AccentPrimary);
		_Color_Viewport_Handle = Color::FromArgb(80, Theme->AccentPrimary);
		_Color_Cursor = Color::White;
		_Color_Loop_Region = Color::FromArgb(255, 100, 200, 100); // Green for loop
		_Color_Marker = Color::White;
	}
}

#include "Widget_Audio_Waveform.h"

#include "Theme_Manager.h"
#include "Playback_Audio_Engine.h"

namespace MIDILightDrawer
{
	Widget_Audio_Waveform::Widget_Audio_Waveform(List<double>^ marker_timestamp_list)
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);

		this->_Marker_Timestamps = marker_timestamp_list;

		Set_Waveform_Data(nullptr);
		Set_Audio_Duration_ms(Widget_Audio_Waveform::NO_AUDIO);
		Set_MIDI_Duration_ms(Widget_Audio_Waveform::NO_MIDI);
		Set_Cursor_Position_ms(0.0);
	}

	void Widget_Audio_Waveform::Set_Waveform_Data(Waveform_Render_Data^ waveform_data)
	{
		this->_Waveform_Data = waveform_data;
	}

	void Widget_Audio_Waveform::Set_Audio_Duration_ms(double audio_duration_ms)
	{
		if (audio_duration_ms == 0) {
			return;
		}

		this->_Audio_Duration_ms = audio_duration_ms;

		Update_Max_Duration();

		this->Invalidate();
	}
	
	void Widget_Audio_Waveform::Set_MIDI_Duration_ms(double midi_duration_ms)
	{
		if (midi_duration_ms == 0) {
			return;
		}
		
		this->_MIDI_Duration_ms = midi_duration_ms;

		Update_Max_Duration();

		this->Invalidate();
	}

	void Widget_Audio_Waveform::Set_Cursor_Position_ms(double cursor_position_ms)
	{
		this->_Cursor_Position_ms = cursor_position_ms;
		this->Invalidate();
	}

	void Widget_Audio_Waveform::OnPaint(PaintEventArgs^ e)
	{
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Draw Waveform
		if (this->_Waveform_Data != nullptr)
		{
			int Segment_Count = _Waveform_Data->TotalSegments;

			float Pixel_Width_Audio = (float)((double)this->Width * (this->_Audio_Duration_ms / this->_Maximum_Duration_ms));
			float Pixels_Per_Segment = Pixel_Width_Audio / (float)Segment_Count;
		
			SolidBrush^ Waveform_Brush = gcnew System::Drawing::SolidBrush(Theme_Manager::Get_Instance()->BorderStrong);
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
		}

		// Draw Marker Markings
		SolidBrush^ Marker_Brush = gcnew System::Drawing::SolidBrush(Color::White);
		Rectangle Marker_Rect = Rectangle(0, 0, 1, 10);

		for each (double Timestamp_ms in this->_Marker_Timestamps)
		{
			Marker_Rect.X = (int)(this->Width * (Timestamp_ms / this->_Maximum_Duration_ms));

			G->FillRectangle(Marker_Brush, Marker_Rect);
		}

		delete Marker_Brush;

		if (this->_Audio_Duration_ms > this->_MIDI_Duration_ms) {
			HatchBrush^ Audio_Only_Brush = gcnew HatchBrush(HatchStyle::BackwardDiagonal, Color::Yellow, Color::FromArgb(0));
			
			int Last_Pixel_MIDI = (int)((double)this->Width * (this->_MIDI_Duration_ms / this->_Maximum_Duration_ms));
			
			Rectangle Audio_Only_Rect = Rectangle(Last_Pixel_MIDI, 0, this->Width - Last_Pixel_MIDI, this->Height);

			G->FillRectangle(Audio_Only_Brush, Audio_Only_Rect);

			delete Audio_Only_Brush;
		}

		// Draw Cursor Position
		if(this->_MIDI_Duration_ms > Widget_Audio_Waveform::NO_MIDI)
		{
			int Pixel_Width_MIDI = (int)((double)this->Width * (this->_MIDI_Duration_ms / this->_Maximum_Duration_ms));
			int Cursor_Pixel_Position = (int)((double)Pixel_Width_MIDI * (this->_Cursor_Position_ms / this->_MIDI_Duration_ms));

			if (Cursor_Pixel_Position >= Pixel_Width_MIDI) {
				Cursor_Pixel_Position = Pixel_Width_MIDI - 1;
			}
			
			SolidBrush^ Cursor_Brush = gcnew System::Drawing::SolidBrush(Color::White);
			Rectangle Cursor_Rect = Rectangle(Cursor_Pixel_Position, 0, 2, this->Height);

			G->FillRectangle(Cursor_Brush, Cursor_Rect);

			delete Cursor_Brush;
		}
	}

	void Widget_Audio_Waveform::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		this->Invalidate();
	}

	void Widget_Audio_Waveform::OnMouseDown(MouseEventArgs^ e)
	{
		Point Mouse_Pos = Point(e->X, e->Y);

		if (e->Button == Windows::Forms::MouseButtons::Left)
		{
			int Pixel_Width_MIDI = (int)((double)this->Width * (this->_MIDI_Duration_ms / this->_Maximum_Duration_ms));

			if (Mouse_Pos.X <= Pixel_Width_MIDI)
			{
				double Cursor_Position_ms = this->_MIDI_Duration_ms * (double)(Mouse_Pos.X) / (double)Pixel_Width_MIDI;
				
				OnCursorPositionChanged(this, Cursor_Position_ms);
			}
		}
	}

	void Widget_Audio_Waveform::Update_Max_Duration()
	{
		this->_Maximum_Duration_ms = Math::Max(this->_Audio_Duration_ms, this->_MIDI_Duration_ms);
	}
}

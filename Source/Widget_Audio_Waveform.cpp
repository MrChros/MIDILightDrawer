#include "Widget_Audio_Waveform.h"

#include "Theme_Manager.h"
#include "Playback_Audio_Engine.h"

namespace MIDILightDrawer
{
	Widget_Audio_Waveform::Widget_Audio_Waveform()
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);

		Set_Waveform_Data(nullptr);
		Set_Cursor_Position_ms(0.0, 1.0);
	}

	void Widget_Audio_Waveform::Set_Waveform_Data(Waveform_Render_Data^ waveform_data)
	{
		this->_Waveform_Data = waveform_data;
	}

	void Widget_Audio_Waveform::Set_Cursor_Position_ms(double cursor_position_ms, double audio_duration_ms)
	{
		this->_Cursor_Position_ms = cursor_position_ms;
		this->_Audio_Duration_ms = audio_duration_ms;

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
			float Pixels_Per_Segment = (float)(this->Width) / (float)Segment_Count;
		
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

		// Draw Cursor Position
		int Cursor_Pixel_Position = (int)((double)this->Width * (_Cursor_Position_ms / _Audio_Duration_ms));

		if (Cursor_Pixel_Position > this->Width) {
			Cursor_Pixel_Position = this->Width;
		}
			
		SolidBrush^ Cursor_Brush = gcnew System::Drawing::SolidBrush(Color::White);
		Rectangle Cursor_Rect = Rectangle(Cursor_Pixel_Position, 0, 2, this->Height);

		G->FillRectangle(Cursor_Brush, Cursor_Rect);

		delete Cursor_Brush;
	}

	void Widget_Audio_Waveform::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		this->Invalidate();
	}
}

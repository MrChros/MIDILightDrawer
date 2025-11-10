#include "Widget_Audio_Waveform.h"

#include "Theme_Manager.h"
#include "Playback_Audio_File_Manager.h"

namespace MIDILightDrawer
{
	Widget_Audio_Waveform::Widget_Audio_Waveform()
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);

		Set_Audio_File_Manager(nullptr);
		Set_Cursor_Position_ms(0.0);
	}

	void Widget_Audio_Waveform::Set_Audio_File_Manager(Playback_Audio_File_Manager^ audio_file_manager)
	{
		this->_Audio_File_Manager = audio_file_manager;
	}

	void Widget_Audio_Waveform::Set_Cursor_Position_ms(double cursor_position_ms)
	{
		this->_Cursor_Poisiton_ms = cursor_position_ms;

		this->Invalidate();
	}

	void Widget_Audio_Waveform::OnPaint(PaintEventArgs^ e)
	{
		if (this->_Audio_File_Manager == nullptr) {
			return;
		}
		
		Theme_Manager^ Theme = Theme_Manager::Get_Instance();

		Graphics^ G = e->Graphics;
		G->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		Waveform_Render_Data^ WaveformData = this->_Audio_File_Manager->WaveformData;
		
		int Segment_Count = WaveformData->TotalSegments;
		float Pixels_Per_Segment = (float)(this->Width) / (float)Segment_Count;

		SolidBrush^ Waveform_Brush = gcnew System::Drawing::SolidBrush(Theme_Manager::Get_Instance()->BorderStrong);
		Rectangle Waveform_Rect = Rectangle(0, 0, (int)(Pixels_Per_Segment)+2, 0);

		float Min;
		float Max;
		float X = 0;

		for (int i = 0; i < Segment_Count; i++) {
			
			WaveformData->GetSegmentMinMax(i, Min, Max);

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

		int Cursor_Pixel_Position = (int)((double)this->Width * (_Cursor_Poisiton_ms / this->_Audio_File_Manager->DurationMilliseconds));

		if (Cursor_Pixel_Position > this->Width) {
			Cursor_Pixel_Position = this->Width;
		}

		SolidBrush^ Cursor_Brush = gcnew System::Drawing::SolidBrush(Color::White);
		Rectangle Cursor_Rect = Rectangle(Cursor_Pixel_Position, 0, 2, this->Height);

		G->FillRectangle(Cursor_Brush, Cursor_Rect);

		delete Waveform_Brush;
		delete Cursor_Brush;
	}

	void Widget_Audio_Waveform::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		this->Invalidate();
	}
}

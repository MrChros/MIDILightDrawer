#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::ComponentModel;

namespace MIDILightDrawer {

	public ref class Control_ScrollBar : public Control
	{
	public:
		Control_ScrollBar();

		void SetEnabledState(bool value);
		int VirtualToPhysical(double virtualPos);
		double PhysicalToVirtual(int physicalPos);

		// Properties
		property int TotalWidth
		{
			int get() { return _TotalWidth; }
			void set(int value);
		}

		property int ViewportWidth
		{
			int get() { return _ViewportWidth; }
			void set(int value);
		}

		property int ScrollPosition
		{
			int get() { return _ScrollPosition; }
			void set(int value);
		}

		property double VirtualTotalWidth
		{
			double get() { return _Virtual_Total_Width; }
			void set(double value);
		}

		property double VirtualViewportWidth
		{
			double get() { return _Virtual_Viewport_Width; }
			void set(double value);
		}

		property double VirtualScrollPosition
		{
			double get() { return _Virtual_Scroll_Position; }
			void set(double value);
		}

		// Events
		event EventHandler<int>^ ScrollPositionChanged;

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;

	private:
		// Member variables
		int			_TotalWidth;
		int			_ViewportWidth;
		int			_ScrollPosition;
		bool		_IsDraggingThumb;
		Point		_LastMousePos;
		Rectangle	_ThumbBounds;
		double		_Virtual_Total_Width;
		double		_Virtual_Viewport_Width;
		double		_Virtual_Scroll_Position;

		// Constants
		static const int SCROLLBAR_HEIGHT = 17;
		static const int SCROLLBAR_BUTTON_WIDTH = 17;
		static const int MIN_THUMB_WIDTH = 30;

		// Helper methods
		void UpdateScrollbar();
		void DrawScrollbar(Graphics^ g);
		bool IsMouseOverThumb(Point mousePos);
		int CalculateThumbPosition(int mouseX);
		void ScrollToPosition(int newPosition);
		
	};
}
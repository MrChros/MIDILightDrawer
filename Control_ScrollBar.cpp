#include "Control_ScrollBar.h"

namespace MIDILightDrawer {

	Control_ScrollBar::Control_ScrollBar()
	{
		this->SetStyle(ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::OptimizedDoubleBuffer, true);

		_TotalWidth			= 0;
		_ViewportWidth		= 0;
		_ScrollPosition		= 0;
		_IsDraggingThumb	= false;

		this->Height = SCROLLBAR_HEIGHT;
	}

	void Control_ScrollBar::SetEnabledState(bool value)
	{
		this->Enabled = value;
		this->Invalidate(); // Redraw to show disabled state
	}

	int Control_ScrollBar::VirtualToPhysical(double virtualPos)
	{
		if (_Virtual_Total_Width <= _Virtual_Viewport_Width) return 0;

		double scrollRatio = virtualPos / (_Virtual_Total_Width - _Virtual_Viewport_Width);
		return static_cast<int>(Math::Round(scrollRatio * _TotalWidth));
	}

	double Control_ScrollBar::PhysicalToVirtual(int physicalPos)
	{
		if (_TotalWidth <= 0) return 0.0;

		double scrollRatio = static_cast<double>(physicalPos) / _TotalWidth;
		return scrollRatio * (_Virtual_Total_Width - _Virtual_Viewport_Width);
	}

	void Control_ScrollBar::TotalWidth::set(int value)
	{
		if (_TotalWidth != value)
		{
			_TotalWidth = value;
			UpdateScrollbar();
		}
	}

	void Control_ScrollBar::ViewportWidth::set(int value)
	{
		if (_ViewportWidth != value)
		{
			_ViewportWidth = value;
			UpdateScrollbar();
		}
	}

	void Control_ScrollBar::VirtualTotalWidth::set(double value)
	{
		if (_Virtual_Total_Width != value)
		{
			_Virtual_Total_Width = value;
			UpdateScrollbar();
		}
	}

	void Control_ScrollBar::VirtualViewportWidth::set(double value)
	{
		if (_Virtual_Viewport_Width != value)
		{
			_Virtual_Viewport_Width = value;
			UpdateScrollbar();
		}
	}

	void Control_ScrollBar::VirtualScrollPosition::set(double value)
	{
		double maxScroll = Math::Max(0.0, _Virtual_Total_Width - _Virtual_Viewport_Width);
		double newPosition = Math::Max(0.0, Math::Min(value, maxScroll));

		if (_Virtual_Scroll_Position != newPosition)
		{
			_Virtual_Scroll_Position = newPosition;
			_ScrollPosition = VirtualToPhysical(newPosition);
			ScrollPositionChanged(this, _ScrollPosition);
			UpdateScrollbar();
		}
	}

	void Control_ScrollBar::ScrollPosition::set(int value)
	{
		int maxScroll = Math::Max(0, _TotalWidth - _ViewportWidth);
		int newPosition = Math::Max(0, Math::Min(value, maxScroll));

		if (_ScrollPosition != newPosition)
		{
			_ScrollPosition = newPosition;
			ScrollPositionChanged(this, _ScrollPosition);
			UpdateScrollbar();
		}
	}

	void Control_ScrollBar::UpdateScrollbar()
	{
		if (_TotalWidth <= 0 || _ViewportWidth <= 0)
		{
			Console::WriteLine("Invalid dimensions: Total={0}, Viewport={1}", _TotalWidth, _ViewportWidth);
			return;
		}

		// Calculate the scroll thumb width and position
		int scrollBarTrackWidth = this->Width - (2 * SCROLLBAR_BUTTON_WIDTH);

		// Calculate viewport ratio
		double viewportRatio = static_cast<double>(_ViewportWidth) / _TotalWidth;
		viewportRatio = Math::Min(1.0, viewportRatio); // Ensure ratio doesn't exceed 1

		// Calculate thumb width - ensure minimum size
		int thumbWidth = static_cast<int>(scrollBarTrackWidth * viewportRatio);
		thumbWidth = Math::Max(MIN_THUMB_WIDTH, thumbWidth);

		// Calculate maximum scrollable area
		int maxScroll = _TotalWidth - _ViewportWidth;
		if (maxScroll <= 0)
		{
			// Content fits within viewport - full width thumb
			_ThumbBounds = Rectangle(
				SCROLLBAR_BUTTON_WIDTH,
				0,
				scrollBarTrackWidth,
				SCROLLBAR_HEIGHT
			);
		}
		else
		{
			// Calculate thumb position based on current scroll position
			double scrollRatio = static_cast<double>(_ScrollPosition) / maxScroll;
			int thumbX = SCROLLBAR_BUTTON_WIDTH +
				static_cast<int>((scrollBarTrackWidth - thumbWidth) * scrollRatio);

			_ThumbBounds = Rectangle(
				thumbX,
				0,
				thumbWidth,
				SCROLLBAR_HEIGHT
			);
		}

		// Debug output
		Console::WriteLine("UpdateScrollbar: Ratio={0}, ThumbWidth={1}, ScrollPos={2}",
			viewportRatio, thumbWidth, _ScrollPosition);

		this->Invalidate();
	}

	void Control_ScrollBar::DrawScrollbar(Graphics^ g)
	{
		// Draw background
		g->FillRectangle(SystemBrushes::Control, this->ClientRectangle);

		// Set brushes based on enabled state, but make thumb RED for debugging
		Brush^ buttonBrush = this->Enabled ? SystemBrushes::ControlLight : SystemBrushes::ControlDark;
		Brush^ trackBrush = this->Enabled ? SystemBrushes::ControlDark : SystemBrushes::ControlDarkDark;
		Brush^ thumbBrush = this->Enabled ? Brushes::Red : SystemBrushes::ControlDark; // Changed to RED
		Pen^ borderPen = this->Enabled ? SystemPens::ControlDark : SystemPens::ControlDarkDark;

		// Draw left and right buttons
		Rectangle leftButton = Rectangle(0, 0, SCROLLBAR_BUTTON_WIDTH, SCROLLBAR_HEIGHT);
		Rectangle rightButton = Rectangle(this->Width - SCROLLBAR_BUTTON_WIDTH, 0,
			SCROLLBAR_BUTTON_WIDTH, SCROLLBAR_HEIGHT);

		g->FillRectangle(buttonBrush, leftButton);
		g->FillRectangle(buttonBrush, rightButton);
		g->DrawRectangle(borderPen, leftButton);
		g->DrawRectangle(borderPen, rightButton);

		// Draw track
		Rectangle trackRect = Rectangle(SCROLLBAR_BUTTON_WIDTH, 0,
			this->Width - (2 * SCROLLBAR_BUTTON_WIDTH), SCROLLBAR_HEIGHT);
		g->FillRectangle(trackBrush, trackRect);

		// Draw thumb if enabled and table is wider than viewport
		if (this->Enabled && _TotalWidth > _ViewportWidth)
		{
			// Draw a border around thumb for better visibility
			g->FillRectangle(thumbBrush, _ThumbBounds);
			g->DrawRectangle(Pens::DarkRed, _ThumbBounds);

			// Debug output
			Console::WriteLine("Thumb Bounds: X={0}, Width={1}, Total Width={2}, Viewport Width={3}",
				_ThumbBounds.X, _ThumbBounds.Width, _TotalWidth, _ViewportWidth);
		}
	}

	void Control_ScrollBar::OnPaint(PaintEventArgs^ e)
	{
		DrawScrollbar(e->Graphics);
	}

	void Control_ScrollBar::OnMouseDown(MouseEventArgs^ e)
	{
		if (!this->Enabled) return;
		
		if (IsMouseOverThumb(e->Location))
		{
			_IsDraggingThumb = true;
			_LastMousePos = e->Location;
		}
		else
		{
			// Click in track - move thumb to this position
			int newPosition = CalculateThumbPosition(e->X);
			ScrollToPosition(newPosition);
		}
	}

	void Control_ScrollBar::OnMouseMove(MouseEventArgs^ e)
	{
		if (!this->Enabled) return;
		
		if (_IsDraggingThumb)
		{
			int delta = e->X - _LastMousePos.X;
			_LastMousePos = e->Location;

			int newPosition = CalculateThumbPosition(e->X);
			ScrollToPosition(newPosition);
		}
	}

	void Control_ScrollBar::OnMouseUp(MouseEventArgs^ e)
	{
		if (!this->Enabled) return;
		
		_IsDraggingThumb = false;
	}

	bool Control_ScrollBar::IsMouseOverThumb(Point mousePos)
	{
		return _ThumbBounds.Contains(mousePos);
	}

	int Control_ScrollBar::CalculateThumbPosition(int mouseX)
	{
		int scrollBarTrackWidth = this->Width - (2 * SCROLLBAR_BUTTON_WIDTH);
		int thumbPosition = mouseX - SCROLLBAR_BUTTON_WIDTH - (_ThumbBounds.Width / 2);
		double scrollRatio = static_cast<double>(thumbPosition) /
			(scrollBarTrackWidth - _ThumbBounds.Width);
		return static_cast<int>(scrollRatio * (_TotalWidth - _ViewportWidth));
	}

	void Control_ScrollBar::ScrollToPosition(int newPosition)
	{
		// Convert physical position to virtual
		double virtualPos = PhysicalToVirtual(newPosition);

		// Update virtual position (which will update physical position internally)
		VirtualScrollPosition = virtualPos;
	}
}
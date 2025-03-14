#pragma once

#include "Theme_Manager.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;

namespace MIDILightDrawer
{
	// Forward declarations
	ref class Control_CheckedListBox;

	// Item selected event arguments
	public ref class CheckedListItemEventArgs : public EventArgs
	{
	public:
		property int Index;
		property bool IsChecked;
		property Object^ Item;

	public:
		CheckedListItemEventArgs(int index, bool checked, Object^ item)
		{
			this->Index = index;
			this->IsChecked = checked;
			this->Item = item;
		}
	};

	// Item for the checked list
	public ref class CheckedListItem
	{
	public:
		CheckedListItem(String^ text)
		{
			Text = text;
			Checked = false;
			Tag = nullptr;
		}

		CheckedListItem(String^ text, bool checked)
		{
			Text = text;
			Checked = checked;
			Tag = nullptr;
		}

		CheckedListItem(String^ text, bool checked, Object^ tag)
		{
			Text = text;
			Checked = checked;
			Tag = tag;
		}

		property String^ Text;
		property bool Checked;
		property Object^ Tag;
		property String^ Subtitle;
	};

	// Delegate for item checked events
	public delegate void CheckedListItemEventHandler(Object^ sender, CheckedListItemEventArgs^ e);

	public ref class Control_CheckedListBox : public UserControl
	{
	public:
		Control_CheckedListBox();
		virtual ~Control_CheckedListBox();

		// Events
		event CheckedListItemEventHandler^ ItemCheckedChanged;
		event EventHandler^ SelectionChanged;

		// Public Methods
		void AddItem(String^ text);
		void AddItem(String^ text, bool checked);
		void AddItem(String^ text, bool checked, Object^ tag);
		void AddItem(CheckedListItem^ item);

		void RemoveItem(int index);
		void ClearItems();

		void SetItemChecked(int index, bool checked);
		bool GetItemChecked(int index);

		int ItemCount();
		void SelectAll();
		void SelectNone();

	protected:
		// Override rendering methods
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;
		virtual void OnMouseLeave(EventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseUp(MouseEventArgs^ e) override;
		virtual void OnKeyDown(KeyEventArgs^ e) override;

	private:
		// Item collection
		List<CheckedListItem^>^ _Items;

		// Visual properties
		int _ItemHeight;
		int _SelectedIndex;
		int _HoverIndex;
		int _ScrollOffset;
		bool _ShowItemCount;

		// UI State
		bool _IsScrolling;
		Rectangle _ScrollBarBounds;
		Rectangle _ScrollThumbBounds;
		int _ScrollStartY;
		int _ScrollStartOffset;

		// Cached resource images
		Image^ _CheckedIcon;
		Image^ _UncheckedIcon;

		// Methods
		void Initialize();
		Image^ CreateCheckIcon(bool isChecked);
		void RenderBackground(Graphics^ g);
		void RenderItems(Graphics^ g);
		void RenderScrollBar(Graphics^ g);

		int GetItemIndexAt(Point p);
		Rectangle GetItemBounds(int index);
		Rectangle GetCheckBoxBounds(int index);

		void UpdateScrollThumbBounds();
		void EnsureVisible(int index);

		int VisibleItemCount();
		int TotalItemHeight();
		void SetScrollOffset(int offset);

		void ToggleItemChecked(int index);

	public:
		// Properties
		property CheckedListItem^ SelectedItem
		{
			CheckedListItem^ get();
		}

		property int SelectedIndex
		{
			int get();
			void set(int value);
		}

		property List<CheckedListItem^>^ Items
		{
			List<CheckedListItem^>^ get();
		}

		property List<CheckedListItem^>^ CheckedItems
		{
			List<CheckedListItem^>^ get();
		}

		property bool ShowItemCount
		{
			bool get();
			void set(bool value);
		}
	};
}
#include "Timeline_Interval_Tree.h"
#include "Widget_Timeline_Classes.h"

namespace MIDILightDrawer
{
	//////////////////////////
	// BarEventIntervalTree //
	//////////////////////////

	Timeline_BarEvent_Interval_Tree::Timeline_BarEvent_Interval_Tree()
	{
		_Root = nullptr;
		_Count = 0;
		_Is_Dirty = false;
		_All_Events = gcnew List<BarEvent^>();
	}

	Timeline_BarEvent_Interval_Tree::Timeline_BarEvent_Interval_Tree(List<BarEvent^>^ events)
	{
		_Root = nullptr;
		_Count = 0;
		_Is_Dirty = false;
		_All_Events = gcnew List<BarEvent^>();

		if (events != nullptr && events->Count > 0)
		{
			Rebuild(events);
		}
	}

	void Timeline_BarEvent_Interval_Tree::Insert(BarEvent^ event)
	{
		if (event == nullptr) {
			return;
		}

		_All_Events->Add(event);
		_Count++;
		_Is_Dirty = true;

		// For single insertions, we mark dirty and let the next query trigger a rebuild
		// This is more efficient than trying to maintain balance on every insert
	}

	bool Timeline_BarEvent_Interval_Tree::Remove(BarEvent^ event)
	{
		if (event == nullptr) {
			return false;
		}

		bool Removed = _All_Events->Remove(event);

		if (Removed)
		{
			_Count--;
			_Is_Dirty = true;
		}

		return Removed;
	}

	void Timeline_BarEvent_Interval_Tree::MarkDirty()
	{
		_Is_Dirty = true;
	}

	void Timeline_BarEvent_Interval_Tree::Rebuild()
	{
		if (_All_Events->Count == 0)
		{
			_Root = nullptr;
			_Is_Dirty = false;
			return;
		}

		int Min_Tick, Max_Tick;
		FindTickRange(_All_Events, Min_Tick, Max_Tick);

		_Root = BuildTree(_All_Events, Min_Tick, Max_Tick);
		_Is_Dirty = false;
	}

	void Timeline_BarEvent_Interval_Tree::Rebuild(List<BarEvent^>^ events)
	{
		_All_Events->Clear();

		if (events != nullptr)
		{
			_All_Events->AddRange(events);
			_Count = events->Count;
		}
		else
		{
			_Count = 0;
		}

		Rebuild();
	}

	List<BarEvent^>^ Timeline_BarEvent_Interval_Tree::QueryRange(int start_tick, int end_tick)
	{
		// Auto-rebuild if dirty
		if (_Is_Dirty)
		{
			Rebuild();
		}

		List<BarEvent^>^ Results = gcnew List<BarEvent^>();

		if (_Root != nullptr)
		{
			QueryRange(_Root, start_tick, end_tick, Results);
		}

		return Results;
	}

	List<BarEvent^>^ Timeline_BarEvent_Interval_Tree::QueryPoint(int tick)
	{
		return QueryRange(tick, tick);
	}

	void Timeline_BarEvent_Interval_Tree::Clear()
	{
		_Root = nullptr;
		_All_Events->Clear();
		_Count = 0;
		_Is_Dirty = false;
	}

	List<BarEvent^>^ Timeline_BarEvent_Interval_Tree::GetAllEvents()
	{
		return gcnew List<BarEvent^>(_All_Events);
	}

	Timeline_BarEvent_Interval_Tree_Node^ Timeline_BarEvent_Interval_Tree::BuildTree(List<BarEvent^>^ events, int min_tick, int max_tick)
	{
		if (events == nullptr || events->Count == 0) {
			return nullptr;
		}

		// Calculate center point
		int Center = min_tick + (max_tick - min_tick) / 2;

		Timeline_BarEvent_Interval_Tree_Node^ Node = gcnew Timeline_BarEvent_Interval_Tree_Node(Center);

		// Lists for partitioning
		List<BarEvent^>^ Left_Events = gcnew List<BarEvent^>();
		List<BarEvent^>^ Right_Events = gcnew List<BarEvent^>();

		int Left_Max = min_tick;
		int Right_Min = max_tick;

		// Partition events
		for each (BarEvent ^ Event in events)
		{
			if (Event->EndTick < Center)
			{
				// Event is entirely to the left of center
				Left_Events->Add(Event);
				if (Event->EndTick > Left_Max) {
					Left_Max = Event->EndTick;
				}
			}
			else if (Event->StartTick > Center)
			{
				// Event is entirely to the right of center
				Right_Events->Add(Event);
				if (Event->StartTick < Right_Min) {
					Right_Min = Event->StartTick;
				}
			}
			else
			{
				// Event overlaps the center point
				Node->Overlapping->Add(Event);
			}
		}

		// Sort overlapping events by start tick for efficient querying
		Node->Overlapping->Sort(gcnew Comparison<BarEvent^>(&Timeline_BarEvent_Interval_Tree::CompareByStartTick));

		// Recursively build subtrees
		if (Left_Events->Count > 0)
		{
			Node->Left = BuildTree(Left_Events, min_tick, Left_Max);
		}

		if (Right_Events->Count > 0)
		{
			Node->Right = BuildTree(Right_Events, Right_Min, max_tick);
		}

		return Node;
	}

	void Timeline_BarEvent_Interval_Tree::QueryRange(Timeline_BarEvent_Interval_Tree_Node^ node, int start_tick, int end_tick, List<BarEvent^>^ results)
	{
		if (node == nullptr) {
			return;
		}

		// Check all overlapping intervals at this node
		// An interval [s, e] overlaps query range [start_tick, end_tick] if:
		// s <= end_tick AND e >= start_tick
		for each (BarEvent ^ Event in node->Overlapping)
		{
			if (Event->StartTick <= end_tick && Event->EndTick >= start_tick)
			{
				results->Add(Event);
			}
		}

		// If query range extends to the left of center, search left subtree
		if (start_tick < node->Center && node->Left != nullptr)
		{
			QueryRange(node->Left, start_tick, end_tick, results);
		}

		// If query range extends to the right of center, search right subtree
		if (end_tick > node->Center && node->Right != nullptr)
		{
			QueryRange(node->Right, start_tick, end_tick, results);
		}
	}

	void Timeline_BarEvent_Interval_Tree::FindTickRange(List<BarEvent^>^ events, int% out_min, int% out_max)
	{
		if (events == nullptr || events->Count == 0)
		{
			out_min = 0;
			out_max = 0;
			return;
		}

		out_min = Int32::MaxValue;
		out_max = Int32::MinValue;

		for each (BarEvent ^ Event in events)
		{
			if (Event->StartTick < out_min) {
				out_min = Event->StartTick;
			}
			if (Event->EndTick > out_max) {
				out_max = Event->EndTick;
			}
		}
	}

	int Timeline_BarEvent_Interval_Tree::CompareByStartTick(BarEvent^ a, BarEvent^ b)
	{
		return a->StartTick.CompareTo(b->StartTick);
	}

	int Timeline_BarEvent_Interval_Tree::CompareByEndTick(BarEvent^ a, BarEvent^ b)
	{
		return a->EndTick.CompareTo(b->EndTick);
	}

	int Timeline_BarEvent_Interval_Tree::Count::get()
	{
		return _Count;
	}

	bool Timeline_BarEvent_Interval_Tree::IsDirty::get()
	{
		return _Is_Dirty;
	}
}
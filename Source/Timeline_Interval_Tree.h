#pragma once

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward declaration
	ref class BarEvent;

	ref class Timeline_BarEvent_Interval_Tree_Node
	{
	public:
		int Center;										// Center point for this node
		List<BarEvent^>^ Overlapping;					// Intervals that overlap the center point
		Timeline_BarEvent_Interval_Tree_Node^ Left;		// Left subtree (intervals entirely to the left of center)
		Timeline_BarEvent_Interval_Tree_Node^ Right;	// Right subtree (intervals entirely to the right of center)

		Timeline_BarEvent_Interval_Tree_Node(int center)
		{
			Center = center;
			Overlapping = gcnew List<BarEvent^>();
			Left = nullptr;
			Right = nullptr;
		}
	};

	public ref class Timeline_BarEvent_Interval_Tree
	{
	private:
		Timeline_BarEvent_Interval_Tree_Node^	_Root;
		int										_Count;
		bool									_Is_Dirty;		// Indicates if tree needs rebuilding
		List<BarEvent^>^						_All_Events;	// Flat list for rebuilding

	public:
		Timeline_BarEvent_Interval_Tree();
		Timeline_BarEvent_Interval_Tree(List<BarEvent^>^ events);

		void Insert(BarEvent^ event);
		bool Remove(BarEvent^ event);
		void MarkDirty();
		void Rebuild();
		void Rebuild(List<BarEvent^>^ events);
		void Clear();

		List<BarEvent^>^ QueryRange(int start_tick, int end_tick);
		List<BarEvent^>^ QueryPoint(int tick);

		List<BarEvent^>^ GetAllEvents();

	private:
		Timeline_BarEvent_Interval_Tree_Node^ BuildTree(List<BarEvent^>^ events, int min_tick, int max_tick);

		void QueryRange(Timeline_BarEvent_Interval_Tree_Node^ node, int start_tick, int end_tick, List<BarEvent^>^ results);
		void FindTickRange(List<BarEvent^>^ events, int% out_min, int% out_max);
		
		static int CompareByStartTick(BarEvent^ a, BarEvent^ b);
		static int CompareByEndTick(BarEvent^ a, BarEvent^ b);

	public:
		property int Count {
			int get();
		}

		property bool IsDirty {
			bool get();
		}
	};
}
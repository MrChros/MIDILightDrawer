#include "Performance_Monitor.h"

namespace MIDILightDrawer
{
	/////////////////
	// MethodStats //
	/////////////////
	MethodStats::MethodStats() 
	{
		TotalTime = 0;
		CallCount = 0;
		MinTime = System::Int64::MaxValue;
		MaxTime = 0;
	}

	void MethodStats::AddMeasurement(System::Int64 duration_us)
	{
		TotalTime += duration_us;
		CallCount++;
		MinTime = System::Math::Min(MinTime, duration_us);
		MaxTime = System::Math::Max(MaxTime, duration_us);
	}

	/////////////////
	// ScopedTimer //
	/////////////////
	ScopedTimer::ScopedTimer(System::String^ methodNameFull, Dictionary<System::String^, MethodStats^>^ stats) : m_stats(stats)
	{
		array<System::String^>^ parts = methodNameFull->Split(':');
		m_methodName = parts[parts->Length - 1];

		m_frequency = System::Diagnostics::Stopwatch::Frequency;
		m_startTicks = System::Diagnostics::Stopwatch::GetTimestamp();
	}

	ScopedTimer::~ScopedTimer()
	{
		this->!ScopedTimer();
	}

	ScopedTimer::!ScopedTimer()
	{
		try {
			System::Int64 endTicks = System::Diagnostics::Stopwatch::GetTimestamp();
			System::Int64 elapsedTicks = endTicks - m_startTicks;

			double seconds = static_cast<double>(elapsedTicks) / m_frequency;
			System::Int64 microseconds = static_cast<System::Int64>(seconds * 1000000.0);

			MethodStats^ Stats;
			if (!m_stats->TryGetValue(m_methodName, Stats)) {
				Stats = gcnew MethodStats();
				Stats->MethodName = m_methodName;
				m_stats[m_methodName] = Stats;
			}

			Stats->AddMeasurement(microseconds);
		}
		catch (...) {
			// Silently fail in finalizer
		}
	}

	////////////////////////
	// PerformanceMonitor //
	////////////////////////
	PerformanceMonitor::PerformanceMonitor()
	{
		m_stats = gcnew Dictionary<System::String^, MethodStats^>();
	}

	void PerformanceMonitor::Reset()
	{
		m_stats->Clear();
	}

	array<MethodStats^>^ PerformanceMonitor::GetSortedStats()
	{
		List<MethodStats^>^ result = gcnew List<MethodStats^>(m_stats->Values);
		result->Sort(gcnew System::Comparison<MethodStats^>(&PerformanceMonitor::CompareStats));
		return result->ToArray();
	}

	System::String^ PerformanceMonitor::GetReport()
	{
		System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
		array<MethodStats^>^ stats = GetSortedStats();

		sb->AppendLine("Performance Report (values in milliseconds):");
		sb->AppendLine(gcnew System::String('-', 88));
		sb->AppendFormat("{0,-30} {1,10} {2,12} {3,12} {4,12} {5,12}\n", "Method", "Calls", "Total(ms)", "Avg(ms)", "Min(ms)", "Max(ms)");
		sb->AppendLine(gcnew System::String('-', 88));

		System::Int64 totalTime = 0;
		int totalCalls = 0;
		System::Int64 minAll = System::Int64::MaxValue;
		System::Int64 maxAll = 0;

		for each(MethodStats ^ stat in stats)
		{
			if (stat->CallCount == 0) {
				continue;
			}

			// Convert microseconds to milliseconds
			double totalMs = stat->TotalTime / 1000.0;
			double avgMs = totalMs / stat->CallCount;
			double minMs = stat->MinTime / 1000.0;
			double maxMs = stat->MaxTime / 1000.0;

			sb->AppendFormat("{0,-30} {1,10} {2,12:F3} {3,12:F3} {4,12:F3} {5,12:F3}\n",
				stat->MethodName,
				stat->CallCount,
				totalMs,
				avgMs,
				minMs,
				maxMs);

			totalTime += stat->TotalTime;
			totalCalls += stat->CallCount;
			minAll = System::Math::Min(minAll, stat->MinTime);
			maxAll = System::Math::Max(maxAll, stat->MaxTime);
		}

		// Add total line
		sb->AppendLine(gcnew System::String('-', 88));
		double totalTimeMs = totalTime / 1000.0;
		double avgTimeMs = totalCalls > 0 ? totalTimeMs / totalCalls : 0;
		double minAllMs = minAll == System::Int64::MaxValue ? 0 : minAll / 1000.0;
		double maxAllMs = maxAll / 1000.0;

		sb->AppendFormat("{0,-30} {1,10} {2,12:F3} {3,12:F3} {4,12:F3} {5,12:F3}\n",
			"TOTAL",
			totalCalls,
			totalTimeMs,
			avgTimeMs,
			minAllMs,
			maxAllMs);

		return sb->ToString();
	}

	ScopedTimer^ PerformanceMonitor::CreateTimer(System::String^ methodName)
	{
		return gcnew ScopedTimer(methodName, m_stats);
	}
}
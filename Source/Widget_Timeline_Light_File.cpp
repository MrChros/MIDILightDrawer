#include "Widget_Timeline.h"
#include "Form_Light_Import.h"

namespace MIDILightDrawer
{
	// Static version of SaveBarEventsToFile
	String^ Widget_Timeline::SaveBarEventsToFile(String^ filePath, List<Track^>^ tracks, List<Measure^>^ measures)
	{
		try
		{
			List<String^>^ lines = gcnew List<String^>();

			// Header with version - updated to v2.0 to indicate support for fade/strobe
			lines->Add("MIDILightDrawer_BarEvents_v2.0");

			// Save pattern information - number of measures
			lines->Add(measures->Count.ToString());

			// Save time signatures for all measures
			for each (Measure ^ measure in measures) {
				lines->Add(String::Format("{0},{1}",
					measure->Numerator,
					measure->Denominator));
			}

			// Calculate total number of bars across all tracks
			int totalBars = 0;
			for each (Track ^ track in tracks) {
				totalBars += track->Events->Count;
			}
			lines->Add(totalBars.ToString());

			// Save each bar's data with track name and type-specific information
			for each (Track ^ track in tracks) {
				for each (BarEvent ^ bar in track->Events) {
					String^ baseData = String::Format("{0},{1},{2},{3}",
						bar->StartTick,
						bar->Duration,
						tracks->IndexOf(track),
						static_cast<int>(bar->Type)  // Save the bar event type
					);

					// Add type-specific data
					switch (bar->Type) {
					case BarEventType::Solid:
						lines->Add(String::Format("{0},{1},{2},{3},{4},{5}",
							baseData,
							bar->Color.R,
							bar->Color.G,
							bar->Color.B,
							track->Name,
							"SOLID"  // Type identifier for verification
						));
						break;

					case BarEventType::Fade:
						if (bar->FadeInfo != nullptr) {
							if (bar->FadeInfo->Type == FadeType::Two_Colors) {
								lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11}",
									baseData,
									bar->FadeInfo->QuantizationTicks,
									bar->FadeInfo->ColorStart.R,
									bar->FadeInfo->ColorStart.G,
									bar->FadeInfo->ColorStart.B,
									bar->FadeInfo->ColorEnd.R,
									bar->FadeInfo->ColorEnd.G,
									bar->FadeInfo->ColorEnd.B,
									track->Name,
									static_cast<int>(bar->FadeInfo->EaseIn),  // Add easing parameters
									static_cast<int>(bar->FadeInfo->EaseOut),
									"FADE2"  // Two-color fade identifier
								));
							}
							else {  // Three-color fade
								lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14}",
									baseData,
									bar->FadeInfo->QuantizationTicks,
									bar->FadeInfo->ColorStart.R,
									bar->FadeInfo->ColorStart.G,
									bar->FadeInfo->ColorStart.B,
									bar->FadeInfo->ColorCenter.R,
									bar->FadeInfo->ColorCenter.G,
									bar->FadeInfo->ColorCenter.B,
									bar->FadeInfo->ColorEnd.R,
									bar->FadeInfo->ColorEnd.G,
									bar->FadeInfo->ColorEnd.B,
									track->Name,
									static_cast<int>(bar->FadeInfo->EaseIn),  // Add easing parameters
									static_cast<int>(bar->FadeInfo->EaseOut),
									"FADE3"  // Three-color fade identifier
								));
							}
						}
						break;

					case BarEventType::Strobe:
						if (bar->StrobeInfo != nullptr) {
							lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6}",
								baseData,
								bar->StrobeInfo->QuantizationTicks,
								bar->StrobeInfo->ColorStrobe.R,
								bar->StrobeInfo->ColorStrobe.G,
								bar->StrobeInfo->ColorStrobe.B,
								track->Name,
								"STROBE"  // Strobe identifier
							));
						}
						break;
					}
				}
			}

			System::IO::File::WriteAllLines(filePath, lines->ToArray());
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error saving bar events: {0}", ex->Message);
		}
	}

	// Static version of LoadBarEventsFromFile
	String^ Widget_Timeline::LoadBarEventsFromFile(String^ filePath, List<Track^>^ tracks, List<Measure^>^ measures, Dictionary<String^, Track^>^ trackMap)
	{
		try
		{
			array<String^>^ lines = System::IO::File::ReadAllLines(filePath);
			if (lines->Length < 2) return "Invalid file format";

			// Check version
			bool IsLegacyFormat = lines[0]->StartsWith("MIDILightDrawer_BarEvents_v1.0");
			bool IsCurrentFormat = lines[0]->StartsWith("MIDILightDrawer_BarEvents_v2.0");

			if (!IsLegacyFormat && !IsCurrentFormat) {
				return "Invalid or unsupported file format version";
			}

			// Parse number of measures
			int FileMeasureCount;
			if (!Int32::TryParse(lines[1], FileMeasureCount)) {
				return "Invalid measure count";
			}

			// Verify measure count matches
			if (FileMeasureCount != measures->Count)
				return String::Format("Pattern mismatch: File has {0} measures, but timeline has {1} measures", FileMeasureCount, measures->Count);

			// Verify time signatures for each measure
			for (int i = 0; i < measures->Count; i++)
			{
				String^ FileMeasureInfo = lines[2 + i];
				array<String^>^ Parts = FileMeasureInfo->Split(',');

				if (Parts->Length != 2) {
					return String::Format("Invalid time signature format in measure {0}", i + 1);
				}

				int FileNumerator, FileDenominator;
				if (!Int32::TryParse(Parts[0], FileNumerator) || !Int32::TryParse(Parts[1], FileDenominator)) {
					return String::Format("Invalid time signature numbers in measure {0}", i + 1);
				}

				if (FileNumerator != measures[i]->Numerator || FileDenominator != measures[i]->Denominator) {
					return String::Format("Time signature mismatch at measure {0}: File has {1}/{2}, but timeline has {3}/{4}", i + 1, FileNumerator, FileDenominator, measures[i]->Numerator, measures[i]->Denominator);
				}
			}

			// Clear existing bars from all tracks
			for each (Track ^ Trk in tracks) {
				Trk->Events->Clear();
			}

			int BarsStartLine = 2 + measures->Count;
			int BarCount;

			if (!Int32::TryParse(lines[BarsStartLine], BarCount)) {
				return "Invalid bar count";
			}

			// Load each bar
			for (int i = 0; i < BarCount; i++)
			{
				String^ BarData = lines[BarsStartLine + 1 + i];
				array<String^>^ Parts = BarData->Split(',');

				if (IsLegacyFormat) {
					// Handle v1.0 format (solid bars only)
					if (Parts->Length != 7) {
						continue;
					}

					String^ TrackName = Parts[6];
					if (!trackMap->ContainsKey(TrackName)) {
						continue;
					}

					Track^ TargetTrack = trackMap[TrackName];
					int StartTick, Length, R, G, B;

					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) || !Int32::TryParse(Parts[3], R) || !Int32::TryParse(Parts[4], G) || !Int32::TryParse(Parts[5], B)) {
						continue;
					}

					TargetTrack->AddBar(StartTick, Length, Color::FromArgb(R, G, B));
				}
				else {
					// Handle v2.0 format with multiple bar types
					if (Parts->Length < 5) {
						continue;
					}

					int StartTick, Length, TrackIndex, EventType;
					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) || !Int32::TryParse(Parts[2], TrackIndex) || !Int32::TryParse(Parts[3], EventType)) {
						continue;
					}

					String^ Identifier = Parts[Parts->Length - 1];
					String^ TrackName = Parts[Parts->Length - 2];

					if (!trackMap->ContainsKey(TrackName)) {
						continue;
					}

					Track^ TargetTrack = trackMap[TrackName];

					switch (static_cast<BarEventType>(EventType))
					{
					case BarEventType::Solid:
						if (Identifier == "SOLID" && Parts->Length >= 8) {
							int r, g, b;
							if (!Int32::TryParse(Parts[4], r) || !Int32::TryParse(Parts[5], g) || !Int32::TryParse(Parts[6], b)) {
								continue;
							}

							TargetTrack->AddBar(StartTick, Length, Color::FromArgb(r, g, b));
						}
						break;

					case BarEventType::Fade:
						if (Identifier == "FADE2" && Parts->Length >= 11) {

							bool HasEasing = Parts->Length >= 11 && IsCurrentFormat;
							if (Parts->Length >= (HasEasing ? 12 : 10))
							{
								int quantization, r1, g1, b1, r2, g2, b2;
								int easeIn = static_cast<int>(FadeEasing::Linear);
								int easeOut = static_cast<int>(FadeEasing::Linear);

								if (!Int32::TryParse(Parts[4], quantization) ||
									!Int32::TryParse(Parts[5], r1) || !Int32::TryParse(Parts[6], g1) || !Int32::TryParse(Parts[7], b1) ||
									!Int32::TryParse(Parts[8], r2) || !Int32::TryParse(Parts[9], g2) || !Int32::TryParse(Parts[10], b2)) {
									continue;
								}

								if (HasEasing) {
									Int32::TryParse(Parts[9], easeIn);
									Int32::TryParse(Parts[10], easeOut);
								}

								BarEventFadeInfo^ fadeInfo = gcnew BarEventFadeInfo(
									quantization,
									Color::FromArgb(r1, g1, b1),
									Color::FromArgb(r2, g2, b2),
									static_cast<FadeEasing>(easeIn),
									static_cast<FadeEasing>(easeOut)
								);
								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, fadeInfo));
							}
						}
						else if (Identifier == "FADE3" && Parts->Length >= 14)
						{
							bool HasEasing = Parts->Length >= 14 && IsCurrentFormat;
							if (Parts->Length >= (HasEasing ? 15 : 13)) {
								int quantization, r1, g1, b1, r2, g2, b2, r3, g3, b3;
								int easeIn = static_cast<int>(FadeEasing::Linear);
								int easeOut = static_cast<int>(FadeEasing::Linear);

								if (!Int32::TryParse(Parts[4], quantization) ||
									!Int32::TryParse(Parts[5], r1) || !Int32::TryParse(Parts[6], g1) || !Int32::TryParse(Parts[7], b1) ||
									!Int32::TryParse(Parts[8], r2) || !Int32::TryParse(Parts[9], g2) || !Int32::TryParse(Parts[10], b2) ||
									!Int32::TryParse(Parts[11], r3) || !Int32::TryParse(Parts[12], g3) || !Int32::TryParse(Parts[13], b3)) {
									continue;
								}

								if (HasEasing) {
									Int32::TryParse(Parts[12], easeIn);
									Int32::TryParse(Parts[13], easeOut);
								}

								BarEventFadeInfo^ fadeInfo = gcnew BarEventFadeInfo(
									quantization,
									Color::FromArgb(r1, g1, b1),
									Color::FromArgb(r2, g2, b2),
									Color::FromArgb(r3, g3, b3),
									static_cast<FadeEasing>(easeIn),
									static_cast<FadeEasing>(easeOut)
								);
								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, fadeInfo));
							}
						}
						break;

					case BarEventType::Strobe:
						if (Identifier == "STROBE" && Parts->Length >= 8) {
							int quantization, r, g, b;
							if (!Int32::TryParse(Parts[4], quantization) || !Int32::TryParse(Parts[5], r) || !Int32::TryParse(Parts[6], g) || !Int32::TryParse(Parts[7], b)) {
								continue;
							}

							BarEventStrobeInfo^ strobeInfo = gcnew BarEventStrobeInfo(
								quantization,
								Color::FromArgb(r, g, b)
							);
							TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, strobeInfo));
						}
						break;
					}
				}
			}

			// Sort events in each track
			for each (Track ^ Trk in tracks) {
				Trk->Events->Sort(Track::barComparer);
			}

			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error loading bar events: {0}", ex->Message);
		}
	}

	// Static method to get tracks from a light file without loading them
	List<LightTrackInfo^>^ Widget_Timeline::GetTracksFromLightFile(String^ filePath)
	{
		List<LightTrackInfo^>^ result = gcnew List<LightTrackInfo^>();
		Dictionary<String^, List<BarEvent^>^>^ trackEvents = gcnew Dictionary<String^, List<BarEvent^>^>();

		try
		{
			array<String^>^ lines = System::IO::File::ReadAllLines(filePath);
			if (lines->Length < 2) return result;

			// Check version
			bool IsLegacyFormat = lines[0]->StartsWith("MIDILightDrawer_BarEvents_v1.0");
			bool IsCurrentFormat = lines[0]->StartsWith("MIDILightDrawer_BarEvents_v2.0");

			if (!IsLegacyFormat && !IsCurrentFormat) {
				return result;
			}

			// Parse number of measures
			int FileMeasureCount;
			if (!Int32::TryParse(lines[1], FileMeasureCount)) {
				return result;
			}

			int BarsStartLine = 2 + FileMeasureCount;
			int BarCount;

			if (!Int32::TryParse(lines[BarsStartLine], BarCount)) {
				return result;
			}

			// Process each bar
			for (int i = 0; i < BarCount; i++)
			{
				String^ BarData = lines[BarsStartLine + 1 + i];
				array<String^>^ Parts = BarData->Split(',');

				String^ TrackName;
				BarEvent^ bar = nullptr;

				if (IsLegacyFormat) {
					// Handle v1.0 format (solid bars only)
					if (Parts->Length != 7) continue;

					TrackName = Parts[6];
					int StartTick, Length, R, G, B;

					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) ||
						!Int32::TryParse(Parts[3], R) || !Int32::TryParse(Parts[4], G) || !Int32::TryParse(Parts[5], B)) {
						continue;
					}

					// Create a temporary track just for the bar
					Track^ tempTrack = gcnew Track(TrackName, 4);
					bar = gcnew BarEvent(tempTrack, StartTick, Length, Color::FromArgb(R, G, B));
				}
				else {
					// Handle v2.0 format with multiple bar types
					if (Parts->Length < 5) continue;

					int StartTick, Length, TrackIndex, EventType;
					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) ||
						!Int32::TryParse(Parts[2], TrackIndex) || !Int32::TryParse(Parts[3], EventType)) {
						continue;
					}

					String^ Identifier = Parts[Parts->Length - 1];
					TrackName = Parts[Parts->Length - 2];

					// Create a temporary track just for the bar
					Track^ tempTrack = gcnew Track(TrackName, 4);

					switch (static_cast<BarEventType>(EventType))
					{
					case BarEventType::Solid:
						if (Identifier == "SOLID" && Parts->Length >= 8) {
							int r, g, b;
							if (!Int32::TryParse(Parts[4], r) || !Int32::TryParse(Parts[5], g) || !Int32::TryParse(Parts[6], b)) {
								continue;
							}

							bar = gcnew BarEvent(tempTrack, StartTick, Length, Color::FromArgb(r, g, b));
						}
						break;

					case BarEventType::Fade:
						if (Identifier == "FADE2" && Parts->Length >= 11) {
							int quantization, r1, g1, b1, r2, g2, b2;
							int easeIn = static_cast<int>(FadeEasing::Linear);
							int easeOut = static_cast<int>(FadeEasing::Linear);

							if (!Int32::TryParse(Parts[4], quantization) ||
								!Int32::TryParse(Parts[5], r1) || !Int32::TryParse(Parts[6], g1) || !Int32::TryParse(Parts[7], b1) ||
								!Int32::TryParse(Parts[8], r2) || !Int32::TryParse(Parts[9], g2) || !Int32::TryParse(Parts[10], b2)) {
								continue;
							}

							if (Parts->Length >= 12) {
								Int32::TryParse(Parts[9], easeIn);
								Int32::TryParse(Parts[10], easeOut);
							}

							BarEventFadeInfo^ fadeInfo = gcnew BarEventFadeInfo(
								quantization,
								Color::FromArgb(r1, g1, b1),
								Color::FromArgb(r2, g2, b2),
								static_cast<FadeEasing>(easeIn),
								static_cast<FadeEasing>(easeOut)
							);
							bar = gcnew BarEvent(tempTrack, StartTick, Length, fadeInfo);
						}
						else if (Identifier == "FADE3" && Parts->Length >= 14) {
							int quantization, r1, g1, b1, r2, g2, b2, r3, g3, b3;
							int easeIn = static_cast<int>(FadeEasing::Linear);
							int easeOut = static_cast<int>(FadeEasing::Linear);

							if (!Int32::TryParse(Parts[4], quantization) ||
								!Int32::TryParse(Parts[5], r1) || !Int32::TryParse(Parts[6], g1) || !Int32::TryParse(Parts[7], b1) ||
								!Int32::TryParse(Parts[8], r2) || !Int32::TryParse(Parts[9], g2) || !Int32::TryParse(Parts[10], b2) ||
								!Int32::TryParse(Parts[11], r3) || !Int32::TryParse(Parts[12], g3) || !Int32::TryParse(Parts[13], b3)) {
								continue;
							}

							if (Parts->Length >= 15) {
								Int32::TryParse(Parts[12], easeIn);
								Int32::TryParse(Parts[13], easeOut);
							}

							BarEventFadeInfo^ fadeInfo = gcnew BarEventFadeInfo(
								quantization,
								Color::FromArgb(r1, g1, b1),
								Color::FromArgb(r2, g2, b2),
								Color::FromArgb(r3, g3, b3),
								static_cast<FadeEasing>(easeIn),
								static_cast<FadeEasing>(easeOut)
							);
							bar = gcnew BarEvent(tempTrack, StartTick, Length, fadeInfo);
						}
						break;

					case BarEventType::Strobe:
						if (Identifier == "STROBE" && Parts->Length >= 8) {
							int quantization, r, g, b;
							if (!Int32::TryParse(Parts[4], quantization) || !Int32::TryParse(Parts[5], r) || !Int32::TryParse(Parts[6], g) || !Int32::TryParse(Parts[7], b)) {
								continue;
							}

							BarEventStrobeInfo^ strobeInfo = gcnew BarEventStrobeInfo(
								quantization,
								Color::FromArgb(r, g, b)
							);
							bar = gcnew BarEvent(tempTrack, StartTick, Length, strobeInfo);
						}
						break;
					}
				}

				// Add the bar to the appropriate track's event list
				if (bar != nullptr) {
					if (!trackEvents->ContainsKey(TrackName)) {
						trackEvents[TrackName] = gcnew List<BarEvent^>();
					}
					trackEvents[TrackName]->Add(bar);
				}
			}

			// Convert the dictionary to a list of LightTrackInfo objects
			for each (KeyValuePair<String^, List<BarEvent^>^> pair in trackEvents) {
				result->Add(gcnew LightTrackInfo(pair.Key, pair.Value->Count, pair.Value));
			}

			return result;
		}
		catch (Exception^)
		{
			return result;
		}
	}
}
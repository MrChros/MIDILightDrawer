#include "Widget_Timeline.h"
#include "Form_Light_Import.h"

namespace MIDILightDrawer
{
	// Static version of SaveBarEventsToFile
	String^ Widget_Timeline::SaveBarEventsToFile(String^ filePath, List<Track^>^ tracks, List<Measure^>^ measures)
	{
		try
		{
			List<String^>^ Lines = gcnew List<String^>();

			// Header with version - updated to v2.0 to indicate support for fade/strobe
			Lines->Add("MIDILightDrawer_BarEvents_v2.0");

			// Save pattern information - number of measures
			Lines->Add(measures->Count.ToString());

			// Save time signatures for all measures
			for each (Measure^ M in measures) {
				Lines->Add(String::Format("{0},{1}",
					M->Numerator,
					M->Denominator));
			}

			// Calculate total number of bars across all tracks
			int Total_Bars = 0;
			for each (Track^ Tr in tracks) {
				Total_Bars += Tr->Events->Count;
			}
			Lines->Add(Total_Bars.ToString());

			// Save each bar's data with track name and type-specific information
			for each (Track^ Tr in tracks) {
				for each (BarEvent^ Bar in Tr->Events) {
					String^ Base_Data = String::Format("{0},{1},{2},{3}",
						Bar->StartTick,
						Bar->Duration,
						tracks->IndexOf(Tr),
						static_cast<int>(Bar->Type)  // Save the bar event type
					);

					// Add type-specific data
					switch (Bar->Type) {
						case BarEventType::Solid:
							Lines->Add(String::Format("{0},{1},{2},{3},{4},{5}",
								Base_Data,
								Bar->Color.R,
								Bar->Color.G,
								Bar->Color.B,
								Tr->Name,
								"SOLID"  // Type identifier for verification
							));
							break;

						case BarEventType::Fade:
							if (Bar->FadeInfo != nullptr) {
								if (Bar->FadeInfo->Type == FadeType::Two_Colors) {
									Lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11}",
										Base_Data,
										Bar->FadeInfo->QuantizationTicks,
										Bar->FadeInfo->ColorStart.R,
										Bar->FadeInfo->ColorStart.G,
										Bar->FadeInfo->ColorStart.B,
										Bar->FadeInfo->ColorEnd.R,
										Bar->FadeInfo->ColorEnd.G,
										Bar->FadeInfo->ColorEnd.B,
										static_cast<int>(Bar->FadeInfo->EaseIn),  // Add easing parameters
										static_cast<int>(Bar->FadeInfo->EaseOut),
										Tr->Name,
										"FADE2"  // Two-color fade identifier
									));
								}
								else {  // Three-color fade
									Lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14}",
										Base_Data,
										Bar->FadeInfo->QuantizationTicks,
										Bar->FadeInfo->ColorStart.R,
										Bar->FadeInfo->ColorStart.G,
										Bar->FadeInfo->ColorStart.B,
										Bar->FadeInfo->ColorCenter.R,
										Bar->FadeInfo->ColorCenter.G,
										Bar->FadeInfo->ColorCenter.B,
										Bar->FadeInfo->ColorEnd.R,
										Bar->FadeInfo->ColorEnd.G,
										Bar->FadeInfo->ColorEnd.B,
										static_cast<int>(Bar->FadeInfo->EaseIn),  // Add easing parameters
										static_cast<int>(Bar->FadeInfo->EaseOut),
										Tr->Name,
										"FADE3"  // Three-color fade identifier
									));
								}
							}
							break;

						case BarEventType::Strobe:
							if (Bar->StrobeInfo != nullptr) {
								Lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6}",
									Base_Data,
									Bar->StrobeInfo->QuantizationTicks,
									Bar->StrobeInfo->ColorStrobe.R,
									Bar->StrobeInfo->ColorStrobe.G,
									Bar->StrobeInfo->ColorStrobe.B,
									Tr->Name,
									"STROBE"  // Strobe identifier
								));
							}
							break;
					}
				}
			}

			System::IO::File::WriteAllLines(filePath, Lines->ToArray());
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

			// Track skipped records for error reporting
			int SkippedRecords = 0;
			int LoadedRecords = 0;

			// Load each bar
			for (int i = 0; i < BarCount; i++)
			{
				String^ BarData = lines[BarsStartLine + 1 + i];
				array<String^>^ Parts = BarData->Split(',');

				if (IsLegacyFormat) {
					// Handle v1.0 format (solid bars only)
					if (Parts->Length != 7) {
						SkippedRecords++;
						continue;
					}

					String^ TrackName = Parts[6];
					if (!trackMap->ContainsKey(TrackName)) {
						SkippedRecords++;
						continue;
					}

					Track^ TargetTrack = trackMap[TrackName];
					int StartTick, Length, R, G, B;

					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) || !Int32::TryParse(Parts[3], R) || !Int32::TryParse(Parts[4], G) || !Int32::TryParse(Parts[5], B)) {
						SkippedRecords++;
						continue;
					}

					// Clamp RGB values to valid range [0-255]
					R = Math::Max(0, Math::Min(255, R));
					G = Math::Max(0, Math::Min(255, G));
					B = Math::Max(0, Math::Min(255, B));

					TargetTrack->AddBar(StartTick, Length, Color::FromArgb(R, G, B));
					LoadedRecords++;
				}
				else {
					// Handle v2.0 format with multiple bar types
					if (Parts->Length < 5) {
						SkippedRecords++;
						continue;
					}

					int StartTick, Length, TrackIndex, EventType;
					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) || !Int32::TryParse(Parts[2], TrackIndex) || !Int32::TryParse(Parts[3], EventType)) {
						SkippedRecords++;
						continue;
					}

					String^ Identifier = Parts[Parts->Length - 1];
					String^ TrackName = Parts[Parts->Length - 2];

					if (!trackMap->ContainsKey(TrackName)) {
						SkippedRecords++;
						continue;
					}

					Track^ TargetTrack = trackMap[TrackName];

					switch (static_cast<BarEventType>(EventType))
					{
					case BarEventType::Solid:
						if (Identifier == "SOLID" && Parts->Length >= 8) {
							int R, G, B;
							if (!Int32::TryParse(Parts[4], R) || !Int32::TryParse(Parts[5], G) || !Int32::TryParse(Parts[6], B)) {
								SkippedRecords++;
								continue;
							}

							// Clamp RGB values to valid range [0-255]
							R = Math::Max(0, Math::Min(255, R));
							G = Math::Max(0, Math::Min(255, G));
							B = Math::Max(0, Math::Min(255, B));

							TargetTrack->AddBar(StartTick, Length, Color::FromArgb(R, G, B));
							LoadedRecords++;
						}
						break;

					case BarEventType::Fade:
						if (Identifier == "FADE2" && Parts->Length >= 12) {

							bool HasEasing = Parts->Length >= 15 && IsCurrentFormat;
							if (Parts->Length >= (HasEasing ? 15 : 12))
							{
								int Quantization, R1, G1, B1, R2, G2, B2;
								int EaseIn = static_cast<int>(FadeEasing::Linear);
								int EaseOut = static_cast<int>(FadeEasing::Linear);

								if (!Int32::TryParse(Parts[4], Quantization) ||
									!Int32::TryParse(Parts[5], R1) || !Int32::TryParse(Parts[6], G1) || !Int32::TryParse(Parts[7], B1) ||
									!Int32::TryParse(Parts[8], R2) || !Int32::TryParse(Parts[9], G2) || !Int32::TryParse(Parts[10], B2)) {
									SkippedRecords++;
									continue;
								}

								// Clamp RGB values to valid range [0-255]
								R1 = Math::Max(0, Math::Min(255, R1));
								G1 = Math::Max(0, Math::Min(255, G1));
								B1 = Math::Max(0, Math::Min(255, B1));
								R2 = Math::Max(0, Math::Min(255, R2));
								G2 = Math::Max(0, Math::Min(255, G2));
								B2 = Math::Max(0, Math::Min(255, B2));

								if (HasEasing) {
									Int32::TryParse(Parts[11], EaseIn);
									Int32::TryParse(Parts[12], EaseOut);
								}

								BarEventFadeInfo^ FadeInfo = gcnew BarEventFadeInfo(
									Quantization,
									Color::FromArgb(R1, G1, B1),
									Color::FromArgb(R2, G2, B2),
									static_cast<FadeEasing>(EaseIn),
									static_cast<FadeEasing>(EaseOut)
								);
								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, FadeInfo));
								LoadedRecords++;
							}
						}
						else if (Identifier == "FADE3" && Parts->Length >= 15)
						{
							bool HasEasing = Parts->Length >= 18 && IsCurrentFormat;
							if (Parts->Length >= (HasEasing ? 18 : 15)) {
								int Quantization, R1, G1, B1, R2, G2, B2, R3, G3, B3;
								int EaseIn = static_cast<int>(FadeEasing::Linear);
								int EaseOut = static_cast<int>(FadeEasing::Linear);

								if (!Int32::TryParse(Parts[4], Quantization) ||
									!Int32::TryParse(Parts[5], R1) || !Int32::TryParse(Parts[6], G1) || !Int32::TryParse(Parts[7], B1) ||
									!Int32::TryParse(Parts[8], R2) || !Int32::TryParse(Parts[9], G2) || !Int32::TryParse(Parts[10], B2) ||
									!Int32::TryParse(Parts[11], R3) || !Int32::TryParse(Parts[12], G3) || !Int32::TryParse(Parts[13], B3)) {
									SkippedRecords++;
									continue;
								}

								// Clamp RGB values to valid range [0-255]
								R1 = Math::Max(0, Math::Min(255, R1));
								G1 = Math::Max(0, Math::Min(255, G1));
								B1 = Math::Max(0, Math::Min(255, B1));
								R2 = Math::Max(0, Math::Min(255, R2));
								G2 = Math::Max(0, Math::Min(255, G2));
								B2 = Math::Max(0, Math::Min(255, B2));
								R3 = Math::Max(0, Math::Min(255, R3));
								G3 = Math::Max(0, Math::Min(255, G3));
								B3 = Math::Max(0, Math::Min(255, B3));

								if (HasEasing) {
									Int32::TryParse(Parts[14], EaseIn);
									Int32::TryParse(Parts[15], EaseOut);
								}

								BarEventFadeInfo^ FadeInfo = gcnew BarEventFadeInfo(
									Quantization,
									Color::FromArgb(R1, G1, B1),
									Color::FromArgb(R2, G2, B2),
									Color::FromArgb(R3, G3, B3),
									static_cast<FadeEasing>(EaseIn),
									static_cast<FadeEasing>(EaseOut)
								);
								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, FadeInfo));
								LoadedRecords++;
							}
						}
						break;

					case BarEventType::Strobe:
						if (Identifier == "STROBE" && Parts->Length >= 8) {
							int Quantization, R, G, B;
							if (!Int32::TryParse(Parts[4], Quantization) || !Int32::TryParse(Parts[5], R) || !Int32::TryParse(Parts[6], G) || !Int32::TryParse(Parts[7], B)) {
								SkippedRecords++;
								continue;
							}

							// Clamp RGB values to valid range [0-255]
							R = Math::Max(0, Math::Min(255, R));
							G = Math::Max(0, Math::Min(255, G));
							B = Math::Max(0, Math::Min(255, B));

							BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(
								Quantization,
								Color::FromArgb(R, G, B)
							);
							TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, StrobeInfo));
							LoadedRecords++;
						}
						break;
					}
				}
			}

			// Sort events in each track
			for each (Track ^ Trk in tracks) {
				Trk->Events->Sort(Track::barComparer);
			}

			// Report any skipped records
			if (SkippedRecords > 0) {
				return String::Format("Warning: {0} of {1} bar events could not be loaded due to parse errors or missing tracks", SkippedRecords, BarCount);
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
				BarEvent^ Bar = nullptr;

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
					Track^ tempTrack = gcnew Track(TrackName, 0, 4);
					Bar = gcnew BarEvent(tempTrack, StartTick, Length, Color::FromArgb(R, G, B));
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
					Track^ tempTrack = gcnew Track(TrackName, 0, 4);

					switch (static_cast<BarEventType>(EventType))
					{
					case BarEventType::Solid:
						if (Identifier == "SOLID" && Parts->Length >= 8) {
							int r, g, b;
							if (!Int32::TryParse(Parts[4], r) || !Int32::TryParse(Parts[5], g) || !Int32::TryParse(Parts[6], b)) {
								continue;
							}

							Bar = gcnew BarEvent(tempTrack, StartTick, Length, Color::FromArgb(r, g, b));
						}
						break;

					case BarEventType::Fade:
						if (Identifier == "FADE2" && Parts->Length >= 12) {
							int Quantization, R1, G1, B1, R2, G2, B2;
							int EaseIn = static_cast<int>(FadeEasing::Linear);
							int EaseOut = static_cast<int>(FadeEasing::Linear);

							if (!Int32::TryParse(Parts[4], Quantization) ||
								!Int32::TryParse(Parts[5], R1) || !Int32::TryParse(Parts[6], G1) || !Int32::TryParse(Parts[7], B1) ||
								!Int32::TryParse(Parts[8], R2) || !Int32::TryParse(Parts[9], G2) || !Int32::TryParse(Parts[10], B2)) {
								continue;
							}

							if (Parts->Length >= 15) {
								Int32::TryParse(Parts[12], EaseIn);
								Int32::TryParse(Parts[13], EaseOut);
							}

							BarEventFadeInfo^ FadeInfo = gcnew BarEventFadeInfo(
								Quantization,
								Color::FromArgb(R1, G1, B1),
								Color::FromArgb(R2, G2, B2),
								static_cast<FadeEasing>(EaseIn),
								static_cast<FadeEasing>(EaseOut)
							);
							Bar = gcnew BarEvent(tempTrack, StartTick, Length, FadeInfo);
						}
						else if (Identifier == "FADE3" && Parts->Length >= 15) {
							int Quantization, R1, G1, B1, R2, G2, B2, R3, G3, B3;
							int EaseIn = static_cast<int>(FadeEasing::Linear);
							int EaseOut = static_cast<int>(FadeEasing::Linear);

							if (!Int32::TryParse(Parts[4], Quantization) ||
								!Int32::TryParse(Parts[5], R1) || !Int32::TryParse(Parts[6], G1) || !Int32::TryParse(Parts[7], B1) ||
								!Int32::TryParse(Parts[8], R2) || !Int32::TryParse(Parts[9], G2) || !Int32::TryParse(Parts[10], B2) ||
								!Int32::TryParse(Parts[11], R3) || !Int32::TryParse(Parts[12], G3) || !Int32::TryParse(Parts[13], B3)) {
								continue;
							}

							if (Parts->Length >= 18) {
								Int32::TryParse(Parts[15], EaseIn);
								Int32::TryParse(Parts[16], EaseOut);
							}

							BarEventFadeInfo^ FadeInfo = gcnew BarEventFadeInfo(
								Quantization,
								Color::FromArgb(R1, G1, B1),
								Color::FromArgb(R2, G2, B2),
								Color::FromArgb(R3, G3, B3),
								static_cast<FadeEasing>(EaseIn),
								static_cast<FadeEasing>(EaseOut)
							);
							Bar = gcnew BarEvent(tempTrack, StartTick, Length, FadeInfo);
						}
						break;

					case BarEventType::Strobe:
						if (Identifier == "STROBE" && Parts->Length >= 8) {
							int Quantization, R, G, B;
							if (!Int32::TryParse(Parts[4], Quantization) || !Int32::TryParse(Parts[5], R) || !Int32::TryParse(Parts[6], G) || !Int32::TryParse(Parts[7], B)) {
								continue;
							}

							BarEventStrobeInfo^ StrobeInfo = gcnew BarEventStrobeInfo(
								Quantization,
								Color::FromArgb(R, G, B)
							);
							Bar = gcnew BarEvent(tempTrack, StartTick, Length, StrobeInfo);
						}
						break;
					}
				}

				// Add the bar to the appropriate track's event list
				if (Bar != nullptr) {
					if (!trackEvents->ContainsKey(TrackName)) {
						trackEvents[TrackName] = gcnew List<BarEvent^>();
					}
					trackEvents[TrackName]->Add(Bar);
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
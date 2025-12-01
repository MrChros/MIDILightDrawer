#include "Form_Auto_Generate.h"
#include "Theme_Manager.h"
#include <cmath>

namespace MIDILightDrawer
{
	//////////////////////////////////
	// FFT Implementation (Cooley-Tukey)
	// This is a simple radix-2 FFT
	//////////////////////////////////
	void Form_Auto_Generate::ComputeFFT(array<float>^ samples, int start_index, int window_size, array<float>^% magnitude_spectrum)
	{
		// Ensure window_size is power of 2
		int N = window_size;

		// Allocate arrays for real and imaginary parts
		array<double>^ Real = gcnew array<double>(N);
		array<double>^ Imag = gcnew array<double>(N);

		// Apply Hann window and copy samples
		for (int i = 0; i < N; i++)
		{
			int Sample_Index = start_index + i;
			double Sample = (Sample_Index < samples->Length) ? samples[Sample_Index] : 0.0;

			// Hann window: 0.5 * (1 - cos(2*pi*n/(N-1)))
			double Window = 0.5 * (1.0 - Math::Cos(2.0 * Math::PI * i / (N - 1)));
			Real[i] = Sample * Window;
			Imag[i] = 0.0;
		}

		// Bit-reversal permutation
		int J = 0;
		for (int i = 0; i < N - 1; i++)
		{
			if (i < J)
			{
				double Temp_Real = Real[i];
				double Temp_Imag = Imag[i];
				Real[i] = Real[J];
				Imag[i] = Imag[J];
				Real[J] = Temp_Real;
				Imag[J] = Temp_Imag;
			}
			int K = N / 2;
			while (K <= J)
			{
				J -= K;
				K /= 2;
			}
			J += K;
		}

		// Cooley-Tukey FFT
		for (int L = 2; L <= N; L *= 2)
		{
			double Angle = -2.0 * Math::PI / L;
			double W_Real = Math::Cos(Angle);
			double W_Imag = Math::Sin(Angle);

			for (int i = 0; i < N; i += L)
			{
				double Wr = 1.0;
				double Wi = 0.0;

				for (int j = 0; j < L / 2; j++)
				{
					int Idx1 = i + j;
					int Idx2 = i + j + L / 2;

					double Tr = Wr * Real[Idx2] - Wi * Imag[Idx2];
					double Ti = Wr * Imag[Idx2] + Wi * Real[Idx2];

					Real[Idx2] = Real[Idx1] - Tr;
					Imag[Idx2] = Imag[Idx1] - Ti;
					Real[Idx1] = Real[Idx1] + Tr;
					Imag[Idx1] = Imag[Idx1] + Ti;

					double New_Wr = Wr * W_Real - Wi * W_Imag;
					Wi = Wr * W_Imag + Wi * W_Real;
					Wr = New_Wr;
				}
			}
		}

		// Calculate magnitude spectrum (only first half is meaningful)
		magnitude_spectrum = gcnew array<float>(N / 2);
		for (int i = 0; i < N / 2; i++)
		{
			magnitude_spectrum[i] = (float)Math::Sqrt(Real[i] * Real[i] + Imag[i] * Imag[i]);
		}
	}

	Spectral_Energy Form_Auto_Generate::CalculateBandEnergies(array<float>^ magnitude_spectrum, int sample_rate)
	{
		Spectral_Energy Energy;
		Energy.Sub_Bass = 0;
		Energy.Bass = 0;
		Energy.Low_Mid = 0;
		Energy.Mid = 0;
		Energy.High_Mid = 0;
		Energy.High = 0;
		Energy.Brilliance = 0;
		Energy.Total = 0;

		if (magnitude_spectrum == nullptr || magnitude_spectrum->Length == 0)
			return Energy;

		int N = magnitude_spectrum->Length * 2;	// FFT size
		float Freq_Resolution = (float)sample_rate / N;

		// Frequency bands (in Hz)
		// Sub_Bass: 20-60, Bass: 60-250, Low_Mid: 250-500
		// Mid: 500-2000, High_Mid: 2000-4000, High: 4000-8000, Brilliance: 8000-20000

		for (int i = 0; i < magnitude_spectrum->Length; i++)
		{
			float Freq = i * Freq_Resolution;
			float Mag_Squared = magnitude_spectrum[i] * magnitude_spectrum[i];

			if (Freq >= 20 && Freq < 60)
				Energy.Sub_Bass += Mag_Squared;
			else if (Freq >= 60 && Freq < 250)
				Energy.Bass += Mag_Squared;
			else if (Freq >= 250 && Freq < 500)
				Energy.Low_Mid += Mag_Squared;
			else if (Freq >= 500 && Freq < 2000)
				Energy.Mid += Mag_Squared;
			else if (Freq >= 2000 && Freq < 4000)
				Energy.High_Mid += Mag_Squared;
			else if (Freq >= 4000 && Freq < 8000)
				Energy.High += Mag_Squared;
			else if (Freq >= 8000 && Freq < 20000)
				Energy.Brilliance += Mag_Squared;

			Energy.Total += Mag_Squared;
		}

		// Take square root to get RMS-like values
		Energy.Sub_Bass = (float)Math::Sqrt(Energy.Sub_Bass);
		Energy.Bass = (float)Math::Sqrt(Energy.Bass);
		Energy.Low_Mid = (float)Math::Sqrt(Energy.Low_Mid);
		Energy.Mid = (float)Math::Sqrt(Energy.Mid);
		Energy.High_Mid = (float)Math::Sqrt(Energy.High_Mid);
		Energy.High = (float)Math::Sqrt(Energy.High);
		Energy.Brilliance = (float)Math::Sqrt(Energy.Brilliance);
		Energy.Total = (float)Math::Sqrt(Energy.Total);

		return Energy;
	}

	float Form_Auto_Generate::CalculateSpectralFlux(Spectral_Energy current, Spectral_Energy previous)
	{
		// Spectral flux: sum of positive differences (onset detection)
		float Flux = 0.0f;

		float Diff = current.Sub_Bass - previous.Sub_Bass;
		if (Diff > 0) Flux += Diff;

		Diff = current.Bass - previous.Bass;
		if (Diff > 0) Flux += Diff;

		Diff = current.Low_Mid - previous.Low_Mid;
		if (Diff > 0) Flux += Diff;

		Diff = current.Mid - previous.Mid;
		if (Diff > 0) Flux += Diff;

		Diff = current.High_Mid - previous.High_Mid;
		if (Diff > 0) Flux += Diff;

		Diff = current.High - previous.High;
		if (Diff > 0) Flux += Diff;

		Diff = current.Brilliance - previous.Brilliance;
		if (Diff > 0) Flux += Diff;

		return Flux;
	}

	float Form_Auto_Generate::CalculateSpectralCentroid(array<float>^ magnitude_spectrum, int sample_rate)
	{
		if (magnitude_spectrum == nullptr || magnitude_spectrum->Length == 0)
			return 0.0f;

		int N = magnitude_spectrum->Length * 2;
		float Freq_Resolution = (float)sample_rate / N;

		float Weighted_Sum = 0.0f;
		float Magnitude_Sum = 0.0f;

		for (int i = 0; i < magnitude_spectrum->Length; i++)
		{
			float Freq = i * Freq_Resolution;
			Weighted_Sum += Freq * magnitude_spectrum[i];
			Magnitude_Sum += magnitude_spectrum[i];
		}

		if (Magnitude_Sum > 0)
			return Weighted_Sum / Magnitude_Sum;

		return 0.0f;
	}

	Frequency_Band Form_Auto_Generate::GetDominantBand(Spectral_Energy spectrum)
	{
		float Max_Energy = spectrum.Sub_Bass;
		Frequency_Band Dominant = Frequency_Band::Sub_Bass;

		if (spectrum.Bass > Max_Energy) { Max_Energy = spectrum.Bass; Dominant = Frequency_Band::Bass; }
		if (spectrum.Low_Mid > Max_Energy) { Max_Energy = spectrum.Low_Mid; Dominant = Frequency_Band::Low_Mid; }
		if (spectrum.Mid > Max_Energy) { Max_Energy = spectrum.Mid; Dominant = Frequency_Band::Mid; }
		if (spectrum.High_Mid > Max_Energy) { Max_Energy = spectrum.High_Mid; Dominant = Frequency_Band::High_Mid; }
		if (spectrum.High > Max_Energy) { Max_Energy = spectrum.High; Dominant = Frequency_Band::High; }
		if (spectrum.Brilliance > Max_Energy) { Dominant = Frequency_Band::Brilliance; }

		return Dominant;
	}

	//////////////////////////////////
	// Spectral Analysis using FFT
	//////////////////////////////////
	void Form_Auto_Generate::PerformSpectralAnalysis()
	{
		if (_Waveform_Data == nullptr || _Audio_Duration_ms <= 0)
			return;

		// We need access to raw audio samples
		// Since we don't have direct access, we'll use the waveform segments
		// and enhance the analysis with the available data

		int Total_Segments = _Waveform_Data->TotalSegments;
		if (Total_Segments == 0) return;

		int Samples_Per_Segment = _Waveform_Data->SamplesPerSegment;
		double Ms_Per_Segment = _Audio_Duration_ms / (double)Total_Segments;

		// Clear existing analysis
		_Audio_Analysis = gcnew Audio_Analysis_Data();
		_Audio_Analysis->Total_Duration_ms = _Audio_Duration_ms;

		// Collect waveform data into an array for pseudo-spectral analysis
		// Since we only have min/max per segment, we'll create a synthetic signal
		array<float>^ Synthetic_Samples = gcnew array<float>(Total_Segments);

		for (int i = 0; i < Total_Segments; i++)
		{
			float Min_Val, Max_Val;
			_Waveform_Data->GetSegmentMinMax(i, Min_Val, Max_Val);
			// Use the amplitude envelope
			Synthetic_Samples[i] = (Max_Val - Min_Val) / 2.0f;
		}

		// First pass: basic energy analysis
		float Sum_Energy = 0.0f;
		float Max_Energy = 0.0f;
		List<float>^ Energy_Values = gcnew List<float>();

		for (int i = 0; i < Total_Segments; i++)
		{
			float Energy = Synthetic_Samples[i];
			Energy_Values->Add(Energy);
			Sum_Energy += Energy;
			if (Energy > Max_Energy) Max_Energy = Energy;
		}

		_Audio_Analysis->Global_Average_Energy = Sum_Energy / (float)Total_Segments;
		_Audio_Analysis->Global_Peak_Energy = Max_Energy;

		// Second pass: FFT-based spectral analysis on overlapping windows
		int FFT_Size = _Settings->FFT_Window_Size;
		int Hop_Size = FFT_Size / 4;	// 75% overlap

		Spectral_Energy Previous_Spectrum;
		Previous_Spectrum.Total = 0;

		// Approximate sample rate based on segment data
		int Approx_Sample_Rate = (int)(Total_Segments * 1000.0 / _Audio_Duration_ms);
		Approx_Sample_Rate = Math::Max(100, Approx_Sample_Rate);	// Minimum 100 "samples" per second

		// Spectral analysis with sliding window
		List<Spectral_Energy>^ Frame_Spectra = gcnew List<Spectral_Energy>();
		List<float>^ Spectral_Flux_Values = gcnew List<float>();

		for (int Frame_Start = 0; Frame_Start < Total_Segments - FFT_Size; Frame_Start += Hop_Size)
		{
			array<float>^ Magnitude_Spectrum;
			ComputeFFT(Synthetic_Samples, Frame_Start, FFT_Size, Magnitude_Spectrum);

			Spectral_Energy Frame_Energy = CalculateBandEnergies(Magnitude_Spectrum, Approx_Sample_Rate);
			Frame_Energy.Spectral_Centroid = CalculateSpectralCentroid(Magnitude_Spectrum, Approx_Sample_Rate);
			Frame_Energy.Spectral_Flux = CalculateSpectralFlux(Frame_Energy, Previous_Spectrum);

			Frame_Spectra->Add(Frame_Energy);
			Spectral_Flux_Values->Add(Frame_Energy.Spectral_Flux);

			Previous_Spectrum = Frame_Energy;
		}

		// Calculate average spectral flux for onset threshold
		float Avg_Flux = 0.0f;
		for each (float Flux in Spectral_Flux_Values)
		{
			Avg_Flux += Flux;
		}
		Avg_Flux /= Math::Max(1, Spectral_Flux_Values->Count);

		float Flux_Threshold = Avg_Flux * (1.0f + _Settings->Onset_Detection_Threshold * 2.0f);

		// Third pass: create energy points with spectral data and detect onsets
		float Transient_Threshold = _Audio_Analysis->Global_Average_Energy * (1.0f + _Settings->Transient_Detection_Sensitivity * 2.0f);
		float Beat_Threshold = _Audio_Analysis->Global_Average_Energy * (1.0f + _Settings->Beat_Detection_Sensitivity);

		float Previous_Energy = 0.0f;
		int Samples_Since_Last_Beat = 0;
		int Min_Samples_Between_Beats = (int)(100.0 / Ms_Per_Segment);
		int Min_Samples_Between_Onsets = (int)(50.0 / Ms_Per_Segment);
		int Samples_Since_Last_Onset = Min_Samples_Between_Onsets;

		for (int i = 0; i < Total_Segments; i++)
		{
			float Current_Energy = Energy_Values[i];
			double Time_ms = (double)i * Ms_Per_Segment;

			Audio_Energy_Point Point;
			Point.Time_ms = Time_ms;
			Point.Energy = Current_Energy;
			Point.Peak = Current_Energy;

			// Get corresponding spectrum if available
			int Spectrum_Index = (i * Frame_Spectra->Count) / Total_Segments;
			if (Spectrum_Index >= 0 && Spectrum_Index < Frame_Spectra->Count)
			{
				Point.Spectrum = Frame_Spectra[Spectrum_Index];
			}

			// Detect transients (sudden energy increase)
			float Energy_Diff = Current_Energy - Previous_Energy;
			Point.Is_Transient = (Energy_Diff > Transient_Threshold);

			// Detect beats (energy above threshold with minimum spacing)
			Point.Is_Beat = false;
			if (Current_Energy > Beat_Threshold && Samples_Since_Last_Beat >= Min_Samples_Between_Beats)
			{
				bool Is_Local_Max = true;
				int Window = 3;

				for (int j = Math::Max(0, i - Window); j <= Math::Min(Total_Segments - 1, i + Window); j++)
				{
					if (j != i && Energy_Values[j] > Current_Energy)
					{
						Is_Local_Max = false;
						break;
					}
				}

				if (Is_Local_Max)
				{
					Point.Is_Beat = true;
					Samples_Since_Last_Beat = 0;
				}
			}

			// Detect onsets using spectral flux
			Point.Is_Onset = false;
			if (Spectrum_Index >= 0 && Spectrum_Index < Spectral_Flux_Values->Count)
			{
				float Flux = Spectral_Flux_Values[Spectrum_Index];
				if (Flux > Flux_Threshold && Samples_Since_Last_Onset >= Min_Samples_Between_Onsets)
				{
					Point.Is_Onset = true;
					Samples_Since_Last_Onset = 0;

					// Create onset info
					Onset_Info Onset;
					Onset.Time_ms = Time_ms;
					Onset.Tick = _Timeline != nullptr ? _Timeline->MillisecondsToTicks(Time_ms) : 0;
					Onset.Strength = Flux / Math::Max(0.001f, Flux_Threshold);
					Onset.Dominant_Band = GetDominantBand(Point.Spectrum);

					_Audio_Analysis->Onsets->Add(Onset);
				}
			}

			_Audio_Analysis->Energy_Points->Add(Point);

			Previous_Energy = Current_Energy;
			Samples_Since_Last_Beat++;
			Samples_Since_Last_Onset++;
		}

		// Calculate global spectrum statistics
		Spectral_Energy Sum_Spectrum;
		Sum_Spectrum.Sub_Bass = 0; Sum_Spectrum.Bass = 0; Sum_Spectrum.Low_Mid = 0;
		Sum_Spectrum.Mid = 0; Sum_Spectrum.High_Mid = 0; Sum_Spectrum.High = 0;
		Sum_Spectrum.Brilliance = 0; Sum_Spectrum.Total = 0;

		Spectral_Energy Peak_Spectrum = Sum_Spectrum;

		for each (Spectral_Energy Spec in Frame_Spectra)
		{
			Sum_Spectrum.Sub_Bass += Spec.Sub_Bass;
			Sum_Spectrum.Bass += Spec.Bass;
			Sum_Spectrum.Low_Mid += Spec.Low_Mid;
			Sum_Spectrum.Mid += Spec.Mid;
			Sum_Spectrum.High_Mid += Spec.High_Mid;
			Sum_Spectrum.High += Spec.High;
			Sum_Spectrum.Brilliance += Spec.Brilliance;
			Sum_Spectrum.Total += Spec.Total;

			if (Spec.Sub_Bass > Peak_Spectrum.Sub_Bass) Peak_Spectrum.Sub_Bass = Spec.Sub_Bass;
			if (Spec.Bass > Peak_Spectrum.Bass) Peak_Spectrum.Bass = Spec.Bass;
			if (Spec.Low_Mid > Peak_Spectrum.Low_Mid) Peak_Spectrum.Low_Mid = Spec.Low_Mid;
			if (Spec.Mid > Peak_Spectrum.Mid) Peak_Spectrum.Mid = Spec.Mid;
			if (Spec.High_Mid > Peak_Spectrum.High_Mid) Peak_Spectrum.High_Mid = Spec.High_Mid;
			if (Spec.High > Peak_Spectrum.High) Peak_Spectrum.High = Spec.High;
			if (Spec.Brilliance > Peak_Spectrum.Brilliance) Peak_Spectrum.Brilliance = Spec.Brilliance;
			if (Spec.Total > Peak_Spectrum.Total) Peak_Spectrum.Total = Spec.Total;
		}

		int Frame_Count = Math::Max(1, Frame_Spectra->Count);
		_Audio_Analysis->Global_Average_Spectrum->Sub_Bass = Sum_Spectrum.Sub_Bass / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Bass = Sum_Spectrum.Bass / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Low_Mid = Sum_Spectrum.Low_Mid / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Mid = Sum_Spectrum.Mid / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->High_Mid = Sum_Spectrum.High_Mid / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->High = Sum_Spectrum.High / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Brilliance = Sum_Spectrum.Brilliance / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Total = Sum_Spectrum.Total / Frame_Count;

		_Audio_Analysis->Global_Peak_Spectrum = %Peak_Spectrum;

		// Create segments aligned with measures
		if (_Timeline != nullptr && _Timeline->Measures != nullptr)
		{
			for each (Measure ^ M in _Timeline->Measures)
			{
				Audio_Segment Seg;
				Seg.Start_ms = M->StartTime_ms;
				Seg.End_ms = M->StartTime_ms + M->Length_ms;

				float Sum = 0.0f;
				float Peak = 0.0f;
				int Count = 0;
				int Beat_Count = 0;
				int Transient_Count = 0;
				int Onset_Count = 0;

				Spectral_Energy Seg_Spectrum;
				Seg_Spectrum.Sub_Bass = 0; Seg_Spectrum.Bass = 0; Seg_Spectrum.Low_Mid = 0;
				Seg_Spectrum.Mid = 0; Seg_Spectrum.High_Mid = 0; Seg_Spectrum.High = 0;
				Seg_Spectrum.Brilliance = 0; Seg_Spectrum.Total = 0;

				for each (Audio_Energy_Point Point in _Audio_Analysis->Energy_Points)
				{
					if (Point.Time_ms >= Seg.Start_ms && Point.Time_ms < Seg.End_ms)
					{
						Sum += Point.Energy;
						if (Point.Energy > Peak) Peak = Point.Energy;
						Count++;
						if (Point.Is_Beat) Beat_Count++;
						if (Point.Is_Transient) Transient_Count++;
						if (Point.Is_Onset) Onset_Count++;

						Seg_Spectrum.Sub_Bass += Point.Spectrum.Sub_Bass;
						Seg_Spectrum.Bass += Point.Spectrum.Bass;
						Seg_Spectrum.Low_Mid += Point.Spectrum.Low_Mid;
						Seg_Spectrum.Mid += Point.Spectrum.Mid;
						Seg_Spectrum.High_Mid += Point.Spectrum.High_Mid;
						Seg_Spectrum.High += Point.Spectrum.High;
						Seg_Spectrum.Brilliance += Point.Spectrum.Brilliance;
					}
				}

				Seg.Average_Energy = Count > 0 ? Sum / (float)Count : 0.0f;
				Seg.Peak_Energy = Peak;
				Seg.Beat_Count = Beat_Count;
				Seg.Transient_Count = Transient_Count;
				Seg.Onset_Count = Onset_Count;

				if (Count > 0)
				{
					Seg.Average_Spectrum.Sub_Bass = Seg_Spectrum.Sub_Bass / Count;
					Seg.Average_Spectrum.Bass = Seg_Spectrum.Bass / Count;
					Seg.Average_Spectrum.Low_Mid = Seg_Spectrum.Low_Mid / Count;
					Seg.Average_Spectrum.Mid = Seg_Spectrum.Mid / Count;
					Seg.Average_Spectrum.High_Mid = Seg_Spectrum.High_Mid / Count;
					Seg.Average_Spectrum.High = Seg_Spectrum.High / Count;
					Seg.Average_Spectrum.Brilliance = Seg_Spectrum.Brilliance / Count;
				}

				// Calculate rhythmic complexity based on onset variation
				Seg.Rhythmic_Complexity = (float)(Onset_Count + Transient_Count) / Math::Max(1.0f, (float)Beat_Count);

				_Audio_Analysis->Segments->Add(Seg);
			}
		}
	}

	void Form_Auto_Generate::PerformSpectralAnalysisWithRawSamples()
	{
		if (_Audio_Engine == nullptr || !_Audio_Engine->Is_Audio_Loaded || _Audio_Duration_ms <= 0)
		{
			// Fall back to waveform-based analysis
			PerformSpectralAnalysis();
			return;
		}

		_Audio_Analysis = gcnew Audio_Analysis_Data();
		_Audio_Analysis->Total_Duration_ms = _Audio_Duration_ms;

		int Sample_Rate = _Audio_Engine->Sample_Rate_File;
		int FFT_Size = _Settings->FFT_Window_Size;
		int Hop_Size = FFT_Size / 4;	// 75% overlap

		// Calculate time per FFT frame
		double Ms_Per_Sample = 1000.0 / (double)Sample_Rate;
		double Ms_Per_Frame = (double)Hop_Size * Ms_Per_Sample;

		// Get all audio samples as mono
		array<float>^ All_Mono_Samples = GetMonoSamplesForAnalysis(0, _Audio_Duration_ms);

		if (All_Mono_Samples == nullptr || All_Mono_Samples->Length < FFT_Size)
		{
			// Fall back to waveform-based analysis
			PerformSpectralAnalysis();
			return;
		}

		int Total_Samples = All_Mono_Samples->Length;
		int Num_Frames = (Total_Samples - FFT_Size) / Hop_Size;

		if (Num_Frames <= 0)
		{
			PerformSpectralAnalysis();
			return;
		}

		// Spectral analysis with sliding window
		List<Spectral_Energy>^ Frame_Spectra = gcnew List<Spectral_Energy>();
		List<float>^ Spectral_Flux_Values = gcnew List<float>();
		List<double>^ Frame_Times = gcnew List<double>();

		Spectral_Energy Previous_Spectrum;
		Previous_Spectrum.Total = 0;

		float Sum_Energy = 0.0f;
		float Max_Energy = 0.0f;

		for (int Frame = 0; Frame < Num_Frames; Frame++)
		{
			int Frame_Start = Frame * Hop_Size;
			double Frame_Time_ms = (double)Frame_Start * Ms_Per_Sample;

			array<float>^ Magnitude_Spectrum;
			ComputeFFT(All_Mono_Samples, Frame_Start, FFT_Size, Magnitude_Spectrum);

			Spectral_Energy Frame_Energy = CalculateBandEnergies(Magnitude_Spectrum, Sample_Rate);
			Frame_Energy.Spectral_Centroid = CalculateSpectralCentroid(Magnitude_Spectrum, Sample_Rate);
			Frame_Energy.Spectral_Flux = CalculateSpectralFlux(Frame_Energy, Previous_Spectrum);

			Frame_Spectra->Add(Frame_Energy);
			Spectral_Flux_Values->Add(Frame_Energy.Spectral_Flux);
			Frame_Times->Add(Frame_Time_ms);

			Sum_Energy += Frame_Energy.Total;
			if (Frame_Energy.Total > Max_Energy)
				Max_Energy = Frame_Energy.Total;

			Previous_Spectrum = Frame_Energy;
		}

		_Audio_Analysis->Global_Average_Energy = Sum_Energy / (float)Num_Frames;
		_Audio_Analysis->Global_Peak_Energy = Max_Energy;

		// Calculate average spectral flux for onset threshold
		float Avg_Flux = 0.0f;
		for each (float Flux in Spectral_Flux_Values)
		{
			Avg_Flux += Flux;
		}
		Avg_Flux /= Math::Max(1, Spectral_Flux_Values->Count);

		float Flux_Threshold = Avg_Flux * (1.0f + _Settings->Onset_Detection_Threshold * 2.0f);

		// Calculate adaptive thresholds for beat and transient detection
		float Beat_Threshold = _Audio_Analysis->Global_Average_Energy * (1.0f + _Settings->Beat_Detection_Sensitivity);
		float Transient_Threshold = Avg_Flux * (1.0f + _Settings->Transient_Detection_Sensitivity * 2.0f);

		int Min_Frames_Between_Beats = (int)(100.0 / Ms_Per_Frame);		// Minimum 100ms between beats
		int Min_Frames_Between_Onsets = (int)(50.0 / Ms_Per_Frame);		// Minimum 50ms between onsets
		int Frames_Since_Last_Beat = Min_Frames_Between_Beats;
		int Frames_Since_Last_Onset = Min_Frames_Between_Onsets;

		float Previous_Frame_Energy = 0.0f;

		// Create energy points with spectral data
		for (int i = 0; i < Frame_Spectra->Count; i++)
		{
			Spectral_Energy Spec = Frame_Spectra[i];
			double Time_ms = Frame_Times[i];

			Audio_Energy_Point Point;
			Point.Time_ms = Time_ms;
			Point.Energy = Spec.Total;
			Point.Peak = Spec.Total;
			Point.Spectrum = Spec;

			// Detect transients using spectral flux
			float Flux = Spectral_Flux_Values[i];
			Point.Is_Transient = (Flux > Transient_Threshold);

			// Detect beats (energy peaks with minimum spacing)
			Point.Is_Beat = false;
			if (Spec.Total > Beat_Threshold && Frames_Since_Last_Beat >= Min_Frames_Between_Beats)
			{
				// Check if local maximum
				bool Is_Local_Max = true;
				int Window = 2;

				for (int j = Math::Max(0, i - Window); j <= Math::Min(Frame_Spectra->Count - 1, i + Window); j++)
				{
					if (j != i && Frame_Spectra[j].Total > Spec.Total)
					{
						Is_Local_Max = false;
						break;
					}
				}

				if (Is_Local_Max)
				{
					Point.Is_Beat = true;
					Frames_Since_Last_Beat = 0;
				}
			}

			// Detect onsets using spectral flux
			Point.Is_Onset = false;
			if (Flux > Flux_Threshold && Frames_Since_Last_Onset >= Min_Frames_Between_Onsets)
			{
				Point.Is_Onset = true;
				Frames_Since_Last_Onset = 0;

				Onset_Info Onset;
				Onset.Time_ms = Time_ms;
				Onset.Tick = _Timeline != nullptr ? _Timeline->MillisecondsToTicks(Time_ms) : 0;
				Onset.Strength = Flux / Math::Max(0.001f, Flux_Threshold);
				Onset.Dominant_Band = GetDominantBand(Spec);

				_Audio_Analysis->Onsets->Add(Onset);
			}

			_Audio_Analysis->Energy_Points->Add(Point);

			Previous_Frame_Energy = Spec.Total;
			Frames_Since_Last_Beat++;
			Frames_Since_Last_Onset++;
		}

		// Calculate global spectrum statistics
		Spectral_Energy Sum_Spectrum;
		Sum_Spectrum.Sub_Bass = 0; Sum_Spectrum.Bass = 0; Sum_Spectrum.Low_Mid = 0;
		Sum_Spectrum.Mid = 0; Sum_Spectrum.High_Mid = 0; Sum_Spectrum.High = 0;
		Sum_Spectrum.Brilliance = 0; Sum_Spectrum.Total = 0;

		Spectral_Energy Peak_Spectrum = Sum_Spectrum;

		for each (Spectral_Energy Spec in Frame_Spectra)
		{
			Sum_Spectrum.Sub_Bass += Spec.Sub_Bass;
			Sum_Spectrum.Bass += Spec.Bass;
			Sum_Spectrum.Low_Mid += Spec.Low_Mid;
			Sum_Spectrum.Mid += Spec.Mid;
			Sum_Spectrum.High_Mid += Spec.High_Mid;
			Sum_Spectrum.High += Spec.High;
			Sum_Spectrum.Brilliance += Spec.Brilliance;
			Sum_Spectrum.Total += Spec.Total;

			if (Spec.Sub_Bass > Peak_Spectrum.Sub_Bass) Peak_Spectrum.Sub_Bass = Spec.Sub_Bass;
			if (Spec.Bass > Peak_Spectrum.Bass) Peak_Spectrum.Bass = Spec.Bass;
			if (Spec.Low_Mid > Peak_Spectrum.Low_Mid) Peak_Spectrum.Low_Mid = Spec.Low_Mid;
			if (Spec.Mid > Peak_Spectrum.Mid) Peak_Spectrum.Mid = Spec.Mid;
			if (Spec.High_Mid > Peak_Spectrum.High_Mid) Peak_Spectrum.High_Mid = Spec.High_Mid;
			if (Spec.High > Peak_Spectrum.High) Peak_Spectrum.High = Spec.High;
			if (Spec.Brilliance > Peak_Spectrum.Brilliance) Peak_Spectrum.Brilliance = Spec.Brilliance;
			if (Spec.Total > Peak_Spectrum.Total) Peak_Spectrum.Total = Spec.Total;
		}

		int Frame_Count = Math::Max(1, Frame_Spectra->Count);
		_Audio_Analysis->Global_Average_Spectrum->Sub_Bass = Sum_Spectrum.Sub_Bass / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Bass = Sum_Spectrum.Bass / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Low_Mid = Sum_Spectrum.Low_Mid / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Mid = Sum_Spectrum.Mid / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->High_Mid = Sum_Spectrum.High_Mid / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->High = Sum_Spectrum.High / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Brilliance = Sum_Spectrum.Brilliance / Frame_Count;
		_Audio_Analysis->Global_Average_Spectrum->Total = Sum_Spectrum.Total / Frame_Count;

		_Audio_Analysis->Global_Peak_Spectrum = Peak_Spectrum;

		// Create segments aligned with measures
		if (_Timeline != nullptr && _Timeline->Measures != nullptr)
		{
			for each (Measure ^ M in _Timeline->Measures)
			{
				Audio_Segment Seg;
				Seg.Start_ms = M->StartTime_ms;
				Seg.End_ms = M->StartTime_ms + M->Length_ms;

				float Sum = 0.0f;
				float Peak = 0.0f;
				int Count = 0;
				int Beat_Count = 0;
				int Transient_Count = 0;
				int Onset_Count = 0;

				Spectral_Energy Seg_Spectrum;
				Seg_Spectrum.Sub_Bass = 0; Seg_Spectrum.Bass = 0; Seg_Spectrum.Low_Mid = 0;
				Seg_Spectrum.Mid = 0; Seg_Spectrum.High_Mid = 0; Seg_Spectrum.High = 0;
				Seg_Spectrum.Brilliance = 0; Seg_Spectrum.Total = 0;

				for each (Audio_Energy_Point Point in _Audio_Analysis->Energy_Points)
				{
					if (Point.Time_ms >= Seg.Start_ms && Point.Time_ms < Seg.End_ms)
					{
						Sum += Point.Energy;
						if (Point.Energy > Peak) Peak = Point.Energy;
						Count++;
						if (Point.Is_Beat) Beat_Count++;
						if (Point.Is_Transient) Transient_Count++;
						if (Point.Is_Onset) Onset_Count++;

						Seg_Spectrum.Sub_Bass += Point.Spectrum.Sub_Bass;
						Seg_Spectrum.Bass += Point.Spectrum.Bass;
						Seg_Spectrum.Low_Mid += Point.Spectrum.Low_Mid;
						Seg_Spectrum.Mid += Point.Spectrum.Mid;
						Seg_Spectrum.High_Mid += Point.Spectrum.High_Mid;
						Seg_Spectrum.High += Point.Spectrum.High;
						Seg_Spectrum.Brilliance += Point.Spectrum.Brilliance;
					}
				}

				Seg.Average_Energy = Count > 0 ? Sum / (float)Count : 0.0f;
				Seg.Peak_Energy = Peak;
				Seg.Beat_Count = Beat_Count;
				Seg.Transient_Count = Transient_Count;
				Seg.Onset_Count = Onset_Count;

				if (Count > 0)
				{
					Seg.Average_Spectrum.Sub_Bass = Seg_Spectrum.Sub_Bass / Count;
					Seg.Average_Spectrum.Bass = Seg_Spectrum.Bass / Count;
					Seg.Average_Spectrum.Low_Mid = Seg_Spectrum.Low_Mid / Count;
					Seg.Average_Spectrum.Mid = Seg_Spectrum.Mid / Count;
					Seg.Average_Spectrum.High_Mid = Seg_Spectrum.High_Mid / Count;
					Seg.Average_Spectrum.High = Seg_Spectrum.High / Count;
					Seg.Average_Spectrum.Brilliance = Seg_Spectrum.Brilliance / Count;
				}

				Seg.Rhythmic_Complexity = (float)(Onset_Count + Transient_Count) / Math::Max(1.0f, (float)Beat_Count);

				_Audio_Analysis->Segments->Add(Seg);
			}
		}
	}

	array<float>^ Form_Auto_Generate::GetMonoSamplesForAnalysis(double start_ms, double end_ms)
	{
		if (_Audio_Engine == nullptr || !_Audio_Engine->Is_Audio_Loaded)
			return nullptr;

		int Sample_Rate = _Audio_Engine->Sample_Rate_File;
		int Num_Channels = _Audio_Engine->Channel_Count;

		if (Sample_Rate <= 0 || Num_Channels <= 0)
			return nullptr;

		int64_t Start_Sample = (int64_t)((start_ms / 1000.0) * Sample_Rate);
		int64_t End_Sample = (int64_t)((end_ms / 1000.0) * Sample_Rate);
		int64_t Sample_Count = End_Sample - Start_Sample;

		if (Sample_Count <= 0)
			return nullptr;

		// Get interleaved samples from audio engine
		array<float>^ Interleaved_Samples = _Audio_Engine->Get_Audio_Samples_Range(start_ms, end_ms);

		if (Interleaved_Samples == nullptr)
			return nullptr;

		// Convert to mono by averaging channels
		int Mono_Sample_Count = Interleaved_Samples->Length / Num_Channels;
		array<float>^ Mono_Samples = gcnew array<float>(Mono_Sample_Count);

		for (int i = 0; i < Mono_Sample_Count; i++)
		{
			float Sum = 0.0f;
			for (int ch = 0; ch < Num_Channels; ch++)
			{
				int Index = i * Num_Channels + ch;
				if (Index < Interleaved_Samples->Length)
					Sum += Interleaved_Samples[Index];
			}
			Mono_Samples[i] = Sum / (float)Num_Channels;
		}

		return Mono_Samples;
	}

	//////////////////////////////////
	// Audio Analysis (Main Entry)
	//////////////////////////////////
	void Form_Auto_Generate::AnalyzeAudio()
	{
		if (_Settings->Use_Spectral_Analysis)
		{
			// Try to use raw audio samples first for best quality analysis
			if (_Audio_Engine != nullptr && _Audio_Engine->Is_Audio_Loaded)
			{
				PerformSpectralAnalysisWithRawSamples();
			}
			else
			{
				// Fall back to waveform-based spectral analysis
				PerformSpectralAnalysis();
			}
		}
		else
		{
			// Simple waveform-based analysis (original implementation)
			_Audio_Analysis = gcnew Audio_Analysis_Data();

			if (_Waveform_Data == nullptr || _Audio_Duration_ms <= 0)
				return;

			int Total_Segments = _Waveform_Data->TotalSegments;
			if (Total_Segments == 0) return;

			double Ms_Per_Segment = _Audio_Duration_ms / (double)Total_Segments;
			float Sum_Energy = 0.0f;
			float Max_Energy = 0.0f;

			List<float>^ Energy_Values = gcnew List<float>();

			for (int i = 0; i < Total_Segments; i++)
			{
				float Min_Val, Max_Val;
				_Waveform_Data->GetSegmentMinMax(i, Min_Val, Max_Val);
				float Energy = Math::Abs(Max_Val - Min_Val);
				Energy_Values->Add(Energy);
				Sum_Energy += Energy;
				if (Energy > Max_Energy) Max_Energy = Energy;
			}

			_Audio_Analysis->Global_Average_Energy = Sum_Energy / (float)Total_Segments;
			_Audio_Analysis->Global_Peak_Energy = Max_Energy;
			_Audio_Analysis->Total_Duration_ms = _Audio_Duration_ms;

			float Transient_Threshold = _Audio_Analysis->Global_Average_Energy * (1.0f + _Settings->Transient_Detection_Sensitivity * 2.0f);
			float Beat_Threshold = _Audio_Analysis->Global_Average_Energy * (1.0f + _Settings->Beat_Detection_Sensitivity);

			float Previous_Energy = 0.0f;
			int Samples_Since_Last_Beat = 0;
			int Min_Samples_Between_Beats = (int)(100.0 / Ms_Per_Segment);

			for (int i = 0; i < Total_Segments; i++)
			{
				float Current_Energy = Energy_Values[i];
				double Time_ms = (double)i * Ms_Per_Segment;

				Audio_Energy_Point Point;
				Point.Time_ms = Time_ms;
				Point.Energy = Current_Energy;
				Point.Peak = Current_Energy;
				Point.Is_Onset = false;

				float Energy_Diff = Current_Energy - Previous_Energy;
				Point.Is_Transient = (Energy_Diff > Transient_Threshold);

				Point.Is_Beat = false;
				if (Current_Energy > Beat_Threshold && Samples_Since_Last_Beat >= Min_Samples_Between_Beats)
				{
					bool Is_Local_Max = true;
					int Window = 3;

					for (int j = Math::Max(0, i - Window); j <= Math::Min(Total_Segments - 1, i + Window); j++)
					{
						if (j != i && Energy_Values[j] > Current_Energy)
						{
							Is_Local_Max = false;
							break;
						}
					}

					if (Is_Local_Max)
					{
						Point.Is_Beat = true;
						Samples_Since_Last_Beat = 0;
					}
				}

				_Audio_Analysis->Energy_Points->Add(Point);
				Previous_Energy = Current_Energy;
				Samples_Since_Last_Beat++;
			}
		}
	}

	//////////////////////////////////
	// Overlap Prevention
	//////////////////////////////////
	bool Form_Auto_Generate::EventsOverlap(BarEvent^ a, BarEvent^ b)
	{
		// Events overlap if they share any ticks
		// a starts before b ends AND a ends after b starts
		return (a->StartTick < b->EndTick) && (a->EndTick > b->StartTick);
	}

	void Form_Auto_Generate::ResolveOverlap(BarEvent^ existing, BarEvent^ new_event)
	{
		// Shorten the new event to not overlap with existing
		if (new_event->StartTick < existing->StartTick)
		{
			// New event starts before existing - truncate new event's end
			int Max_Duration = existing->StartTick - new_event->StartTick;
			if (Max_Duration > 0)
			{
				new_event->Duration = Max_Duration;
			}
		}
		else
		{
			// New event starts during existing - move new event to after existing
			new_event->StartTick = existing->EndTick;
		}
	}

	void Form_Auto_Generate::PreventOverlaps(List<BarEvent^>^ events)
	{
		if (events == nullptr || events->Count < 2)
			return;

		// Sort events by start tick
		events->Sort(gcnew Comparison<BarEvent^>(&Track::CompareBarEvents));

		// Process events and resolve overlaps
		List<BarEvent^>^ Events_To_Remove = gcnew List<BarEvent^>();

		for (int i = 0; i < events->Count - 1; i++)
		{
			BarEvent^ Current = events[i];
			BarEvent^ Next = events[i + 1];

			// Check if events on the same track overlap
			if (Current->ContainingTrack == Next->ContainingTrack)
			{
				if (EventsOverlap(Current, Next))
				{
					// Resolve by truncating current event to end where next begins
					if (Current->StartTick < Next->StartTick)
					{
						int New_Duration = Next->StartTick - Current->StartTick;

						if (New_Duration >= _Settings->Minimum_Duration_Ticks)
						{
							Current->Duration = New_Duration;
						}
						else
						{
							// Event would be too short, remove it
							Events_To_Remove->Add(Current);
						}
					}
					else
					{
						// Current starts at same tick or after Next - remove Current
						Events_To_Remove->Add(Current);
					}
				}
			}
		}

		// Remove events that couldn't be resolved
		for each (BarEvent ^ Event in Events_To_Remove)
		{
			events->Remove(Event);
		}

		// Second pass: ensure back-to-back events don't overlap by even 1 tick
		for (int i = 0; i < events->Count - 1; i++)
		{
			BarEvent^ Current = events[i];
			BarEvent^ Next = events[i + 1];

			if (Current->ContainingTrack == Next->ContainingTrack)
			{
				// Ensure Current ends exactly where Next starts (or before)
				if (Current->EndTick > Next->StartTick)
				{
					Current->Duration = Next->StartTick - Current->StartTick;
				}
			}
		}
	}

	//////////////////////////////////
	// Tablature Analysis
	//////////////////////////////////
	void Form_Auto_Generate::AnalyzeTablature()
	{
		_Tab_Analysis = gcnew Tablature_Analysis_Data();

		if (_Timeline == nullptr || _Timeline->Tracks == nullptr || _Timeline->Measures == nullptr)
			return;

		for each (Measure ^ M in _Timeline->Measures)
		{
			Measure_Analysis MA;
			MA.Measure_Index = M->Number - 1;
			MA.Start_Tick = M->StartTick;
			MA.Length_Ticks = M->Length;
			MA.Tempo = M->Tempo;
			MA.Numerator = M->Numerator;
			MA.Denominator = M->Denominator;
			MA.Total_Notes = 0;
			MA.Total_Beats = 0;
			MA.Has_Marker = (M->Marker_Text != nullptr && M->Marker_Text->Length > 0);
			MA.Marker_Text = M->Marker_Text;

			float Velocity_Sum = 0.0f;
			int Velocity_Count = 0;

			for each (Track ^ T in _Timeline->Tracks)
			{
				if (T->Measures == nullptr) continue;

				for each (TrackMeasure ^ TM in T->Measures)
				{
					if (TM->Number != M->Number) continue;

					for each (Beat ^ B in TM->Beats)
					{
						if (B->StartTick < M->StartTick || B->StartTick >= M->StartTick + M->Length)
							continue;

						Tab_Event_Info Event;
						Event.Measure_Index = MA.Measure_Index;
						Event.Beat_Index = MA.Total_Beats;
						Event.Start_Tick = B->StartTick;
						Event.Duration_Ticks = B->Duration;
						Event.Note_Count = B->Notes->Count;
						Event.Is_Rest = (B->Notes->Count == 0);
						Event.Is_Chord = (B->Notes->Count > 1);

						Event.Has_Accented_Notes = false;
						Event.Has_Palm_Mute = false;
						Event.Has_Slide = false;
						Event.Has_Bend = false;
						Event.Has_Hammer = false;
						Event.Average_Velocity = 100;

						MA.Total_Notes += B->Notes->Count;
						MA.Total_Beats++;

						_Tab_Analysis->Events->Add(Event);
						_Tab_Analysis->Total_Beats++;
						_Tab_Analysis->Total_Notes += B->Notes->Count;
					}
				}
			}

			int Beats_In_Measure = MA.Numerator;
			MA.Note_Density = Beats_In_Measure > 0 ? (float)MA.Total_Notes / (float)Beats_In_Measure : 0.0f;
			MA.Average_Velocity = Velocity_Count > 0 ? Velocity_Sum / (float)Velocity_Count : 100.0f;

			_Tab_Analysis->Measures->Add(MA);
		}

		if (_Tab_Analysis->Measures->Count > 0)
		{
			float Density_Sum = 0.0f;
			for each (Measure_Analysis MA in _Tab_Analysis->Measures)
			{
				Density_Sum += MA.Note_Density;
			}
			_Tab_Analysis->Average_Note_Density = Density_Sum / (float)_Tab_Analysis->Measures->Count;
		}
	}

	//////////////////////////////////
	// Event Generation - Main Entry
	//////////////////////////////////
	List<BarEvent^>^ Form_Auto_Generate::GenerateEvents()
	{
		List<BarEvent^>^ All_Events = gcnew List<BarEvent^>();

		if (_Timeline == nullptr || _Timeline->Tracks == nullptr)
			return All_Events;

		int Start_Tick = 0;
		int End_Tick = _Timeline->TotalTicks;

		if (!_Settings->Use_Full_Range)
		{
			Start_Tick = MeasureToTick(_Settings->Start_Measure - 1);
			End_Tick = MeasureToTick(_Settings->End_Measure);
		}

		for each (Track ^ T in _Timeline->Tracks)
		{
			if (!_Settings->Apply_To_All_Tracks &&
				!_Settings->Selected_Track_Indices->Contains(T->Index))
			{
				continue;
			}

			List<BarEvent^>^ Track_Events = nullptr;

			switch (_Settings->Mode)
			{
			case Generation_Mode::Follow_Tablature:
				Track_Events = GenerateFromTablature(T, Start_Tick, End_Tick);
				break;

			case Generation_Mode::Follow_Audio_Energy:
				Track_Events = GenerateFromAudioEnergy(T, Start_Tick, End_Tick);
				break;

			case Generation_Mode::Follow_Audio_Beats:
				Track_Events = GenerateFromAudioBeats(T, Start_Tick, End_Tick);
				break;

			case Generation_Mode::Combined:
				Track_Events = GenerateCombined(T, Start_Tick, End_Tick);
				break;

			case Generation_Mode::Pattern_Based:
				Track_Events = GeneratePatternBased(T, Start_Tick, End_Tick);
				break;
			}

			if (Track_Events != nullptr)
			{
				// IMPORTANT: Prevent overlapping events on the same track
				PreventOverlaps(Track_Events);

				if (_Settings->Fill_Gaps)
				{
					ApplyGapFilling(Track_Events, Start_Tick, End_Tick);
					// Prevent overlaps again after gap filling
					PreventOverlaps(Track_Events);
				}

				All_Events->AddRange(Track_Events);
			}
		}

		return All_Events;
	}

	//////////////////////////////////
	// Generation: Follow Tablature
	//////////////////////////////////
	List<BarEvent^>^ Form_Auto_Generate::GenerateFromTablature(Track^ track, int start_tick, int end_tick)
	{
		List<BarEvent^>^ Events = gcnew List<BarEvent^>();

		if (track->Measures == nullptr || track->Measures->Count == 0)
			return Events;

		int Event_Index = 0;

		switch (_Settings->Distribution)
		{
		case Event_Distribution::Per_Note:
		case Event_Distribution::Per_Beat:
		{
			List<Tab_Event_Info>^ Tab_Events = _Tab_Analysis->GetEventsInRange(start_tick, end_tick);

			for (int i = 0; i < Tab_Events->Count; i++)
			{
				Tab_Event_Info Tab_Event = Tab_Events[i];

				if (Tab_Event.Is_Rest && !_Settings->Fill_Gaps)
					continue;

				int Duration = Tab_Event.Duration_Ticks;

				if (_Settings->Auto_Duration)
				{
					if (i + 1 < Tab_Events->Count)
					{
						int Next_Start = Tab_Events[i + 1].Start_Tick;
						Duration = Next_Start - Tab_Event.Start_Tick;
					}

					Duration = (int)(Duration * _Settings->Duration_Scale_Factor);
					Duration = Math::Max(Duration, _Settings->Minimum_Duration_Ticks);
					Duration = Math::Min(Duration, _Settings->Maximum_Duration_Ticks);
				}
				else
				{
					Duration = _Settings->Fixed_Duration_Ticks;
				}

				float Energy = 0.5f;
				Spectral_Energy Spectrum;

				if (_Audio_Analysis != nullptr && _Timeline != nullptr)
				{
					double Time_ms = _Timeline->TicksToMilliseconds(Tab_Event.Start_Tick);
					Energy = _Audio_Analysis->GetNormalizedEnergyAtTime(Time_ms);
					Spectrum = _Audio_Analysis->GetSpectrumAtTime(Time_ms);
				}

				float Note_Density = _Tab_Analysis->Measures->Count > 0 ?
					_Tab_Analysis->GetMeasureAt(Tab_Event.Start_Tick).Note_Density : 1.0f;

				BarEventType Event_Type = _Settings->Use_Spectral_Analysis ?
					GetEventTypeForSpectrum(Spectrum, Note_Density) :
					GetEventTypeForContext(Energy, Note_Density, Tab_Event.Has_Accented_Notes);

				double Time_Ratio = (double)(Tab_Event.Start_Tick - start_tick) / (double)(end_tick - start_tick);
				Color Event_Color = _Settings->Use_Spectral_Analysis ?
					GetColorForSpectrum(Spectrum, Time_Ratio, Event_Index) :
					GetColorForEvent(Time_Ratio, Energy, Event_Index);

				BarEvent^ New_Event = nullptr;

				switch (Event_Type)
				{
				case BarEventType::Solid:
					New_Event = gcnew BarEvent(track, Tab_Event.Start_Tick, Duration, Event_Color);
					break;

				case BarEventType::Fade:
				{
					Color End_Color = GetColorForEvent(Time_Ratio + 0.1, Energy * 0.5f, Event_Index + 1);
					BarEventFadeInfo^ Fade_Info = gcnew BarEventFadeInfo(
						_Settings->Fade_Quantization_Ticks,
						Event_Color, End_Color,
						_Settings->Default_Ease_In,
						_Settings->Default_Ease_Out);
					New_Event = gcnew BarEvent(track, Tab_Event.Start_Tick, Duration, Fade_Info);
					break;
				}

				case BarEventType::Strobe:
				{
					BarEventStrobeInfo^ Strobe_Info = gcnew BarEventStrobeInfo(
						_Settings->Strobe_Quantization_Ticks,
						Event_Color);
					New_Event = gcnew BarEvent(track, Tab_Event.Start_Tick, Duration, Strobe_Info);
					break;
				}
				}

				if (New_Event != nullptr)
				{
					Events->Add(New_Event);
					Event_Index++;
				}
			}
			break;
		}

		case Event_Distribution::Per_Measure:
		{
			for each (Measure ^ M in _Timeline->Measures)
			{
				if (M->StartTick < start_tick || M->StartTick >= end_tick)
					continue;

				int Duration = M->Length;

				if (!_Settings->Auto_Duration)
					Duration = _Settings->Fixed_Duration_Ticks;

				Duration = (int)(Duration * _Settings->Duration_Scale_Factor);
				Duration = Math::Max(Duration, _Settings->Minimum_Duration_Ticks);
				Duration = Math::Min(Duration, _Settings->Maximum_Duration_Ticks);

				Measure_Analysis MA = _Tab_Analysis->GetMeasureAt(M->StartTick);
				float Energy = MA.Note_Density / Math::Max(1.0f, _Tab_Analysis->Average_Note_Density * 2.0f);

				BarEventType Event_Type = GetEventTypeForContext(Energy, MA.Note_Density, false);
				double Time_Ratio = (double)(M->StartTick - start_tick) / (double)(end_tick - start_tick);
				Color Event_Color = GetColorForEvent(Time_Ratio, Energy, Event_Index);

				BarEvent^ New_Event = gcnew BarEvent(track, M->StartTick, Duration, Event_Color);
				Events->Add(New_Event);
				Event_Index++;
			}
			break;
		}

		case Event_Distribution::Custom_Interval:
		{
			int Interval = _Settings->Custom_Interval_Ticks;
			int Current_Tick = start_tick;

			while (Current_Tick < end_tick)
			{
				int Duration = Interval;
				if (Current_Tick + Duration > end_tick)
					Duration = end_tick - Current_Tick;

				double Time_Ratio = (double)(Current_Tick - start_tick) / (double)(end_tick - start_tick);
				Color Event_Color = GetColorForEvent(Time_Ratio, 0.5f, Event_Index);

				BarEvent^ New_Event = gcnew BarEvent(track, Current_Tick, Duration, Event_Color);
				Events->Add(New_Event);

				Current_Tick += Interval;
				Event_Index++;
			}
			break;
		}
		}

		return Events;
	}

	//////////////////////////////////
	// Generation: Follow Audio Energy
	//////////////////////////////////
	List<BarEvent^>^ Form_Auto_Generate::GenerateFromAudioEnergy(Track^ track, int start_tick, int end_tick)
	{
		List<BarEvent^>^ Events = gcnew List<BarEvent^>();

		if (_Audio_Analysis == nullptr || _Audio_Analysis->Energy_Points->Count == 0 || _Timeline == nullptr)
			return GenerateFromTablature(track, start_tick, end_tick);

		double Start_ms = _Timeline->TicksToMilliseconds(start_tick);
		double End_ms = _Timeline->TicksToMilliseconds(end_tick);

		float Threshold = _Audio_Analysis->Global_Average_Energy * (1.0f + _Settings->Beat_Detection_Sensitivity * 0.5f);
		int Event_Index = 0;

		bool In_Event = false;
		int Event_Start_Tick = 0;
		float Event_Peak_Energy = 0.0f;
		Spectral_Energy Event_Peak_Spectrum;

		for each (Audio_Energy_Point Point in _Audio_Analysis->Energy_Points)
		{
			if (Point.Time_ms < Start_ms || Point.Time_ms >= End_ms)
				continue;

			int Current_Tick = _Timeline->MillisecondsToTicks(Point.Time_ms);

			if (Point.Energy > Threshold)
			{
				if (!In_Event)
				{
					In_Event = true;
					Event_Start_Tick = Current_Tick;
					Event_Peak_Energy = Point.Energy;
					Event_Peak_Spectrum = Point.Spectrum;
				}
				else
				{
					if (Point.Energy > Event_Peak_Energy)
					{
						Event_Peak_Energy = Point.Energy;
						Event_Peak_Spectrum = Point.Spectrum;
					}
				}
			}
			else if (In_Event)
			{
				In_Event = false;

				int Duration = Current_Tick - Event_Start_Tick;
				Duration = (int)(Duration * _Settings->Duration_Scale_Factor);
				Duration = Math::Max(Duration, _Settings->Minimum_Duration_Ticks);
				Duration = Math::Min(Duration, _Settings->Maximum_Duration_Ticks);

				float Normalized_Energy = Event_Peak_Energy / _Audio_Analysis->Global_Peak_Energy;
				double Time_Ratio = (double)(Event_Start_Tick - start_tick) / (double)(end_tick - start_tick);

				BarEventType Event_Type = _Settings->Use_Spectral_Analysis ?
					GetEventTypeForSpectrum(Event_Peak_Spectrum, 1.0f) :
					GetEventTypeForContext(Normalized_Energy, 1.0f, false);

				Color Event_Color = _Settings->Use_Spectral_Analysis ?
					GetColorForSpectrum(Event_Peak_Spectrum, Time_Ratio, Event_Index) :
					GetColorForEvent(Time_Ratio, Normalized_Energy, Event_Index);

				BarEvent^ New_Event = nullptr;

				switch (Event_Type)
				{
				case BarEventType::Solid:
					New_Event = gcnew BarEvent(track, Event_Start_Tick, Duration, Event_Color);
					break;

				case BarEventType::Fade:
				{
					Color End_Color = GetColorForEvent(Time_Ratio + 0.05, Normalized_Energy * 0.3f, Event_Index + 1);
					BarEventFadeInfo^ Fade_Info = gcnew BarEventFadeInfo(
						_Settings->Fade_Quantization_Ticks,
						Event_Color, End_Color,
						_Settings->Default_Ease_In,
						_Settings->Default_Ease_Out);
					New_Event = gcnew BarEvent(track, Event_Start_Tick, Duration, Fade_Info);
					break;
				}

				case BarEventType::Strobe:
				{
					BarEventStrobeInfo^ Strobe_Info = gcnew BarEventStrobeInfo(
						_Settings->Strobe_Quantization_Ticks,
						Event_Color);
					New_Event = gcnew BarEvent(track, Event_Start_Tick, Duration, Strobe_Info);
					break;
				}
				}

				if (New_Event != nullptr)
				{
					Events->Add(New_Event);
					Event_Index++;
				}
			}
		}

		return Events;
	}

	//////////////////////////////////
	// Generation: Follow Audio Beats
	//////////////////////////////////
	List<BarEvent^>^ Form_Auto_Generate::GenerateFromAudioBeats(Track^ track, int start_tick, int end_tick)
	{
		List<BarEvent^>^ Events = gcnew List<BarEvent^>();

		if (_Audio_Analysis == nullptr || _Timeline == nullptr)
			return GenerateFromTablature(track, start_tick, end_tick);

		double Start_ms = _Timeline->TicksToMilliseconds(start_tick);
		double End_ms = _Timeline->TicksToMilliseconds(end_tick);
		int Event_Index = 0;

		// Use onsets if spectral analysis is enabled, otherwise use beats
		if (_Settings->Use_Spectral_Analysis && _Audio_Analysis->Onsets->Count > 0)
		{
			List<Onset_Info>^ Onsets = _Audio_Analysis->GetOnsetsInRange(Start_ms, End_ms);

			for (int i = 0; i < Onsets->Count; i++)
			{
				Onset_Info Onset = Onsets[i];
				int Onset_Tick = Onset.Tick;

				int Duration = _Settings->Fixed_Duration_Ticks;

				if (_Settings->Auto_Duration && i + 1 < Onsets->Count)
				{
					int Next_Onset_Tick = Onsets[i + 1].Tick;
					Duration = Next_Onset_Tick - Onset_Tick;
				}

				Duration = (int)(Duration * _Settings->Duration_Scale_Factor);
				Duration = Math::Max(Duration, _Settings->Minimum_Duration_Ticks);
				Duration = Math::Min(Duration, _Settings->Maximum_Duration_Ticks);

				Spectral_Energy Spectrum = _Audio_Analysis->GetSpectrumAtTime(Onset.Time_ms);
				double Time_Ratio = (double)(Onset_Tick - start_tick) / (double)(end_tick - start_tick);

				Color Event_Color = GetColorForSpectrum(Spectrum, Time_Ratio, Event_Index);
				BarEventType Event_Type = GetEventTypeForSpectrum(Spectrum, 1.0f);

				BarEvent^ New_Event = gcnew BarEvent(track, Onset_Tick, Duration, Event_Color);
				Events->Add(New_Event);
				Event_Index++;
			}
		}
		else
		{
			// Use beat detection
			List<Audio_Energy_Point>^ Beat_Points = gcnew List<Audio_Energy_Point>();

			for each (Audio_Energy_Point Point in _Audio_Analysis->Energy_Points)
			{
				if (Point.Is_Beat && Point.Time_ms >= Start_ms && Point.Time_ms < End_ms)
				{
					Beat_Points->Add(Point);
				}
			}

			for (int i = 0; i < Beat_Points->Count; i++)
			{
				Audio_Energy_Point Beat = Beat_Points[i];
				int Beat_Tick = _Timeline->MillisecondsToTicks(Beat.Time_ms);

				int Duration = _Settings->Fixed_Duration_Ticks;

				if (_Settings->Auto_Duration && i + 1 < Beat_Points->Count)
				{
					int Next_Beat_Tick = _Timeline->MillisecondsToTicks(Beat_Points[i + 1].Time_ms);
					Duration = Next_Beat_Tick - Beat_Tick;
				}

				Duration = (int)(Duration * _Settings->Duration_Scale_Factor);
				Duration = Math::Max(Duration, _Settings->Minimum_Duration_Ticks);
				Duration = Math::Min(Duration, _Settings->Maximum_Duration_Ticks);

				float Normalized_Energy = Beat.Energy / _Audio_Analysis->Global_Peak_Energy;
				double Time_Ratio = (double)(Beat_Tick - start_tick) / (double)(end_tick - start_tick);

				BarEventType Event_Type = GetEventTypeForContext(Normalized_Energy, 1.0f, Beat.Is_Transient);
				Color Event_Color = GetColorForEvent(Time_Ratio, Normalized_Energy, Event_Index);

				BarEvent^ New_Event = gcnew BarEvent(track, Beat_Tick, Duration, Event_Color);
				Events->Add(New_Event);
				Event_Index++;
			}
		}

		return Events;
	}

	//////////////////////////////////
	// Generation: Combined Mode
	//////////////////////////////////
	List<BarEvent^>^ Form_Auto_Generate::GenerateCombined(Track^ track, int start_tick, int end_tick)
	{
		List<BarEvent^>^ Events = GenerateFromTablature(track, start_tick, end_tick);

		if (_Audio_Analysis == nullptr || _Audio_Analysis->Energy_Points->Count == 0)
			return Events;

		for each (BarEvent ^ Event in Events)
		{
			if (_Timeline == nullptr) continue;

			double Time_ms = _Timeline->TicksToMilliseconds(Event->StartTick);
			float Energy = _Audio_Analysis->GetNormalizedEnergyAtTime(Time_ms);
			Spectral_Energy Spectrum = _Audio_Analysis->GetSpectrumAtTime(Time_ms);

			if (_Settings->Color_Selection_Mode == Color_Mode::Gradient_By_Energy ||
				_Settings->Color_Selection_Mode == Color_Mode::Map_To_Velocity)
			{
				Color Base_Color = Event->Color;
				int R = (int)(Base_Color.R * (0.3f + 0.7f * Energy));
				int G = (int)(Base_Color.G * (0.3f + 0.7f * Energy));
				int B = (int)(Base_Color.B * (0.3f + 0.7f * Energy));

				Event->Color = Color::FromArgb(
					Math::Min(255, Math::Max(0, R)),
					Math::Min(255, Math::Max(0, G)),
					Math::Min(255, Math::Max(0, B)));
			}
		}

		return Events;
	}

	//////////////////////////////////
	// Generation: Pattern Based
	//////////////////////////////////
	List<BarEvent^>^ Form_Auto_Generate::GeneratePatternBased(Track^ track, int start_tick, int end_tick)
	{
		List<BarEvent^>^ Events = gcnew List<BarEvent^>();

		if (_Timeline == nullptr || _Timeline->Measures == nullptr)
			return Events;

		Dictionary<String^, List<int>^>^ Section_Measures = gcnew Dictionary<String^, List<int>^>();

		for each (Measure ^ M in _Timeline->Measures)
		{
			if (M->Marker_Text != nullptr && M->Marker_Text->Length > 0)
			{
				if (!Section_Measures->ContainsKey(M->Marker_Text))
				{
					Section_Measures[M->Marker_Text] = gcnew List<int>();
				}
				Section_Measures[M->Marker_Text]->Add(M->Number - 1);
			}
		}

		if (Section_Measures->Count == 0)
			return GenerateFromTablature(track, start_tick, end_tick);

		int Pattern_Color_Index = 0;
		int Event_Index = 0;

		for each (KeyValuePair<String^, List<int>^> ^ Section in Section_Measures)
		{
			Color Section_Primary = _Settings->Color_Palette[Pattern_Color_Index % _Settings->Color_Palette->Count];
			Color Section_Secondary = _Settings->Color_Palette[(Pattern_Color_Index + 1) % _Settings->Color_Palette->Count];

			for each (int Measure_Index in Section->Value)
			{
				if (Measure_Index < 0 || Measure_Index >= _Timeline->Measures->Count)
					continue;

				Measure^ M = _Timeline->Measures[Measure_Index];

				if (M->StartTick < start_tick || M->StartTick >= end_tick)
					continue;

				int Beats_Per_Measure = M->Numerator;
				int Ticks_Per_Beat = M->Length / Beats_Per_Measure;

				for (int Beat = 0; Beat < Beats_Per_Measure; Beat++)
				{
					int Beat_Tick = M->StartTick + (Beat * Ticks_Per_Beat);
					int Duration = Ticks_Per_Beat;

					Duration = (int)(Duration * _Settings->Duration_Scale_Factor);
					Duration = Math::Max(Duration, _Settings->Minimum_Duration_Ticks);
					Duration = Math::Min(Duration, _Settings->Maximum_Duration_Ticks);

					Color Beat_Color = (Beat % 2 == 0) ? Section_Primary : Section_Secondary;

					if (Beat == 0)
					{
						BarEventFadeInfo^ Fade_Info = gcnew BarEventFadeInfo(
							_Settings->Fade_Quantization_Ticks,
							Beat_Color,
							Color::FromArgb(Beat_Color.R / 3, Beat_Color.G / 3, Beat_Color.B / 3),
							_Settings->Default_Ease_In,
							_Settings->Default_Ease_Out);

						BarEvent^ New_Event = gcnew BarEvent(track, Beat_Tick, Duration, Fade_Info);
						Events->Add(New_Event);
					}
					else
					{
						BarEvent^ New_Event = gcnew BarEvent(track, Beat_Tick, Duration, Beat_Color);
						Events->Add(New_Event);
					}

					Event_Index++;
				}
			}

			Pattern_Color_Index++;
		}

		return Events;
	}

	//////////////////////////////////
	// Helper: Get Color for Event
	//////////////////////////////////
	Color Form_Auto_Generate::GetColorForEvent(double time_ratio, float energy, int event_index)
	{
		time_ratio = Math::Max(0.0, Math::Min(1.0, time_ratio));
		energy = Math::Max(0.0f, Math::Min(1.0f, energy));

		switch (_Settings->Color_Selection_Mode)
		{
		case Color_Mode::Single_Color:
			return _Settings->Primary_Color;

		case Color_Mode::Gradient_By_Time:
		{
			int R = (int)(_Settings->Primary_Color.R + (_Settings->Secondary_Color.R - _Settings->Primary_Color.R) * time_ratio);
			int G = (int)(_Settings->Primary_Color.G + (_Settings->Secondary_Color.G - _Settings->Primary_Color.G) * time_ratio);
			int B = (int)(_Settings->Primary_Color.B + (_Settings->Secondary_Color.B - _Settings->Primary_Color.B) * time_ratio);
			return Color::FromArgb(R, G, B);
		}

		case Color_Mode::Gradient_By_Energy:
		{
			int R = (int)(_Settings->Primary_Color.R + (_Settings->Secondary_Color.R - _Settings->Primary_Color.R) * energy);
			int G = (int)(_Settings->Primary_Color.G + (_Settings->Secondary_Color.G - _Settings->Primary_Color.G) * energy);
			int B = (int)(_Settings->Primary_Color.B + (_Settings->Secondary_Color.B - _Settings->Primary_Color.B) * energy);
			return Color::FromArgb(R, G, B);
		}

		case Color_Mode::Alternate_Colors:
		{
			int Color_Index = event_index % 3;
			if (Color_Index == 0) return _Settings->Primary_Color;
			if (Color_Index == 1) return _Settings->Secondary_Color;
			return _Settings->Tertiary_Color;
		}

		case Color_Mode::Random_From_Palette:
		{
			if (_Settings->Color_Palette->Count > 0)
			{
				Random^ Rnd = gcnew Random(event_index * 17);
				int Index = Rnd->Next(_Settings->Color_Palette->Count);
				return _Settings->Color_Palette[Index];
			}
			return _Settings->Primary_Color;
		}

		case Color_Mode::Map_To_Velocity:
		{
			float Brightness = 0.3f + 0.7f * energy;
			int R = (int)(_Settings->Primary_Color.R * Brightness);
			int G = (int)(_Settings->Primary_Color.G * Brightness);
			int B = (int)(_Settings->Primary_Color.B * Brightness);
			return Color::FromArgb(Math::Min(255, R), Math::Min(255, G), Math::Min(255, B));
		}
		}

		return _Settings->Primary_Color;
	}

	//////////////////////////////////
	// Helper: Get Color for Spectrum
	//////////////////////////////////
	Color Form_Auto_Generate::GetColorForSpectrum(Spectral_Energy spectrum, double time_ratio, int event_index)
	{
		// Map frequency bands to colors
		// Bass = Red, Mid = Green, High = Blue (or user preference)

		float Total = spectrum.Sub_Bass + spectrum.Bass + spectrum.Low_Mid +
			spectrum.Mid + spectrum.High_Mid + spectrum.High + spectrum.Brilliance;

		if (Total <= 0) Total = 1.0f;

		// Normalize band contributions
		float Bass_Contribution = (spectrum.Sub_Bass + spectrum.Bass) / Total;
		float Mid_Contribution = (spectrum.Low_Mid + spectrum.Mid + spectrum.High_Mid) / Total;
		float High_Contribution = (spectrum.High + spectrum.Brilliance) / Total;

		// Map to RGB
		int R = (int)(_Settings->Primary_Color.R * Bass_Contribution +
			_Settings->Secondary_Color.R * Mid_Contribution +
			_Settings->Tertiary_Color.R * High_Contribution);
		int G = (int)(_Settings->Primary_Color.G * Bass_Contribution +
			_Settings->Secondary_Color.G * Mid_Contribution +
			_Settings->Tertiary_Color.G * High_Contribution);
		int B = (int)(_Settings->Primary_Color.B * Bass_Contribution +
			_Settings->Secondary_Color.B * Mid_Contribution +
			_Settings->Tertiary_Color.B * High_Contribution);

		return Color::FromArgb(
			Math::Min(255, Math::Max(0, R)),
			Math::Min(255, Math::Max(0, G)),
			Math::Min(255, Math::Max(0, B)));
	}

	//////////////////////////////////
	// Helper: Get Event Type for Context
	//////////////////////////////////
	BarEventType Form_Auto_Generate::GetEventTypeForContext(float energy, float note_density, bool has_accent)
	{
		switch (_Settings->Type_Selection_Mode)
		{
		case Event_Type_Mode::Solid_Only:
			return BarEventType::Solid;

		case Event_Type_Mode::Fade_On_Dynamics:
			if (energy > _Settings->Energy_Threshold_Fade)
				return BarEventType::Fade;
			return BarEventType::Solid;

		case Event_Type_Mode::Strobe_On_Fast:
			if (note_density > _Tab_Analysis->Average_Note_Density * 1.5f || energy > _Settings->Energy_Threshold_Strobe)
				return BarEventType::Strobe;
			return BarEventType::Solid;

		case Event_Type_Mode::Mixed_Auto:
			if (energy > _Settings->Energy_Threshold_Strobe || has_accent)
				return BarEventType::Strobe;
			if (energy > _Settings->Energy_Threshold_Fade)
				return BarEventType::Fade;
			return BarEventType::Solid;
		}

		return BarEventType::Solid;
	}

	//////////////////////////////////
	// Helper: Get Event Type for Spectrum
	//////////////////////////////////
	BarEventType Form_Auto_Generate::GetEventTypeForSpectrum(Spectral_Energy spectrum, float note_density)
	{
		if (_Settings->Type_Selection_Mode == Event_Type_Mode::Solid_Only)
			return BarEventType::Solid;

		// Use spectral characteristics to determine event type
		float High_Energy_Ratio = (spectrum.High + spectrum.High_Mid + spectrum.Brilliance) /
			Math::Max(0.001f, spectrum.Total);
		float Bass_Energy_Ratio = (spectrum.Sub_Bass + spectrum.Bass) /
			Math::Max(0.001f, spectrum.Total);

		// High frequency content suggests fast/percussive sounds -> strobe
		if (High_Energy_Ratio > 0.5f || spectrum.Spectral_Flux > _Settings->Energy_Threshold_Strobe)
		{
			return BarEventType::Strobe;
		}

		// Strong bass with moderate overall energy -> fade
		if (Bass_Energy_Ratio > 0.4f && spectrum.Total > _Audio_Analysis->Global_Average_Spectrum->Total)
		{
			return BarEventType::Fade;
		}

		return BarEventType::Solid;
	}

	//////////////////////////////////
	// Helper: Apply Gap Filling
	//////////////////////////////////
	void Form_Auto_Generate::ApplyGapFilling(List<BarEvent^>^ events, int start_tick, int end_tick)
	{
		if (events->Count < 2) return;

		events->Sort(gcnew Comparison<BarEvent^>(&Track::CompareBarEvents));

		List<BarEvent^>^ Gap_Events = gcnew List<BarEvent^>();

		for (int i = 0; i < events->Count - 1; i++)
		{
			BarEvent^ Current = events[i];
			BarEvent^ Next = events[i + 1];

			// Only process events on the same track
			if (Current->ContainingTrack != Next->ContainingTrack)
				continue;

			int Gap_Start = Current->EndTick;
			int Gap_Size = Next->StartTick - Gap_Start;

			if (Gap_Size > _Settings->Minimum_Gap_Ticks)
			{
				switch (_Settings->Gap_Fill_Mode)
				{
				case 0:	// Extend previous
					Current->Duration = Current->Duration + Gap_Size;
					break;

				case 1:	// Insert fade
				{
					Color Start_Color = Current->Color;
					Color End_Color = Next->Color;

					BarEventFadeInfo^ Fade_Info = gcnew BarEventFadeInfo(
						_Settings->Fade_Quantization_Ticks,
						Start_Color, End_Color,
						_Settings->Default_Ease_In,
						_Settings->Default_Ease_Out);

					BarEvent^ Gap_Fill = gcnew BarEvent(Current->ContainingTrack, Gap_Start, Gap_Size, Fade_Info);
					Gap_Events->Add(Gap_Fill);
					break;
				}

				case 2:	// Insert dark (black)
				{
					BarEvent^ Gap_Fill = gcnew BarEvent(Current->ContainingTrack, Gap_Start, Gap_Size, Color::Black);
					Gap_Events->Add(Gap_Fill);
					break;
				}
				}
			}
		}

		events->AddRange(Gap_Events);
		events->Sort(gcnew Comparison<BarEvent^>(&Track::CompareBarEvents));
	}
}
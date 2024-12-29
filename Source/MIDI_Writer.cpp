#include "MIDI_Writer.h"
#include <algorithm>
#include <iomanip>

namespace MIDILightDrawer {

	MIDI_Writer::MIDI_Writer(uint16_t ticks_per_quarter_note, bool debug)
		: _ticks_per_quarter_note(ticks_per_quarter_note)
		, _current_tick(0)
		, _debug_mode(debug) {
		Initialize_Track();
	}

	void MIDI_Writer::Debug_Print_Event(const MIDI_Event& event) const {
		if (!_debug_mode) return;

		std::cout << "Event at delta time " << event._delta_time << ":\n";
		std::cout << "  Status: 0x" << std::hex << static_cast<int>(event._status) << std::dec << "\n";

		if (event._status == 0xFF) {
			std::cout << "  Meta event type: 0x" << std::hex << static_cast<int>(event._data1) << std::dec << "\n";
			std::cout << "  Length: " << static_cast<int>(event._data2) << "\n";
			std::cout << "  Data: ";
			for (uint8_t byte : event._meta_data) {
				std::cout << std::hex << std::setw(2) << std::setfill('0')
					<< static_cast<int>(byte) << " ";
			}
			std::cout << std::dec << "\n";
		}
		else {
			std::cout << "  Data1: " << static_cast<int>(event._data1) << "\n";
			std::cout << "  Data2: " << static_cast<int>(event._data2) << "\n";
		}
		std::cout << "\n";
	}

	void MIDI_Writer::Initialize_Track() {
		// Add track name
		std::vector<uint8_t> track_name = { 'M', 'a', 'i', 'n', ' ', 'T', 'r', 'a', 'c', 'k' };
		_events.emplace_back(0, 0xFF, 0x03, track_name);
	}

	void MIDI_Writer::Add_Time_Signature_Event(uint32_t tick, uint8_t numerator, uint8_t denominator) {
		uint8_t denominator_power = 0;
		uint8_t temp_denom = denominator;
		while (temp_denom > 1) {
			temp_denom >>= 1;
			denominator_power++;
		}

		std::vector<uint8_t> time_sig_data = {
			numerator,
			denominator_power,
			24,  // MIDI clocks per metronome click
			8    // Number of 32nd notes per quarter note
		};
		_events.emplace_back(tick, 0xFF, 0x58, time_sig_data);
	}

	void MIDI_Writer::Add_Tempo_Event(uint32_t tick, uint32_t tempo_bpm) {
		uint32_t tempo_usec = static_cast<uint32_t>(60000000.0 / tempo_bpm);
		std::vector<uint8_t> tempo_data = {
			static_cast<uint8_t>((tempo_usec >> 16) & 0xFF),
			static_cast<uint8_t>((tempo_usec >> 8) & 0xFF),
			static_cast<uint8_t>(tempo_usec & 0xFF)
		};
		_events.emplace_back(tick, 0xFF, 0x51, tempo_data);
	}

	void MIDI_Writer::Add_Measure(uint8_t numerator, uint8_t denominator, uint32_t tempo_bpm) {
		uint32_t ticks_per_beat = (uint32_t)(_ticks_per_quarter_note * (4.0 / denominator));
		uint32_t measure_length = ticks_per_beat * numerator;

		_measures.emplace_back(numerator, denominator, tempo_bpm, _current_tick);

		// Only add time signature and tempo if it's different from the previous measure
		bool add_time_sig = true;
		bool add_tempo = true;

		if (_measures.size() > 1) {
			const auto& prev_measure = _measures[_measures.size() - 2];
			add_time_sig = (prev_measure.numerator != numerator ||
				prev_measure.denominator != denominator);
			add_tempo = (prev_measure.tempo_bpm != tempo_bpm);
		}

		if (add_time_sig) {
			Add_Time_Signature_Event(_current_tick, numerator, denominator);
		}
		if (add_tempo) {
			Add_Tempo_Event(_current_tick, tempo_bpm);
		}

		_current_tick += measure_length;
	}

	void MIDI_Writer::Add_Note(uint32_t start_tick, uint32_t length_ticks, uint8_t channel, uint8_t note, uint8_t velocity) {
		uint8_t channel_masked = channel & 0x0F;
		_events.emplace_back(start_tick, 0x90 | channel_masked, note, velocity);
		_events.emplace_back(start_tick + length_ticks, 0x80 | channel_masked, note, 0);
	}

	void MIDI_Writer::Write_Var_Len(std::ofstream& file, uint32_t value) {
		uint32_t buffer = value & 0x7F;

		while ((value >>= 7)) {
			buffer <<= 8;
			buffer |= ((value & 0x7F) | 0x80);
		}

		while (true) {
			file.put(buffer & 0xFF);
			if (buffer & 0x80)
				buffer >>= 8;
			else
				break;
		}
	}

	void MIDI_Writer::Write_Int32(std::ofstream& file, uint32_t value) {
		file.put((value >> 24) & 0xFF);
		file.put((value >> 16) & 0xFF);
		file.put((value >> 8) & 0xFF);
		file.put(value & 0xFF);
	}

	void MIDI_Writer::Write_Int16(std::ofstream& file, uint16_t value) {
		file.put((value >> 8) & 0xFF);
		file.put(value & 0xFF);
	}

	uint32_t MIDI_Writer::Calculate_Track_Size() const {
		uint32_t size = 0;
		for (const auto& event : _events) {
			// Delta time (variable length)
			uint32_t temp = event._delta_time;
			do {
				size++;
				temp >>= 7;
			} while (temp > 0);

			// Status byte
			size++;

			if (event._status == 0xFF) {
				size += 2;  // Meta type and length bytes
				size += (uint32_t)(event._meta_data.size());
			}
			else {
				size += 2;  // Two data bytes
			}
		}

		// End of track event (00 FF 2F 00)
		size += 4;

		if (_debug_mode) {
			std::cout << "Calculated track size: " << size << " bytes\n";
		}

		return size;
	}

	bool MIDI_Writer::Save_To_File(const std::string& filename) {
		if (_measures.empty()) {
			Add_Measure(4, 4, 120);
		}

		if (_debug_mode) {
			std::cout << "Number of events before sorting: " << _events.size() << "\n";
		}

		// Sort events by time
		std::sort(_events.begin(), _events.end(),
			[](const MIDI_Event& a, const MIDI_Event& b) {
				return a._delta_time < b._delta_time;
			});

		// Convert absolute times to delta times
		uint32_t last_time = 0;
		for (auto& event : _events) {
			uint32_t current_time = event._delta_time;
			event._delta_time = current_time - last_time;
			last_time = current_time;

			if (_debug_mode) {
				Debug_Print_Event(event);
			}
		}

		std::ofstream file(filename, std::ios::binary);
		if (!file) {
			if (_debug_mode) std::cout << "Failed to open file: " << filename << "\n";
			return false;
		}

		// Write header chunk
		file.write("MThd", 4);
		Write_Int32(file, 6);  // Header size
		Write_Int16(file, 0);  // Format 0
		Write_Int16(file, 1);  // Number of tracks
		Write_Int16(file, _ticks_per_quarter_note);

		// Write track chunk
		file.write("MTrk", 4);
		uint32_t track_size = Calculate_Track_Size();
		Write_Int32(file, track_size);

		if (_debug_mode) {
			std::cout << "Writing track chunk with size: " << track_size << "\n";
		}

		// Write all events
		for (const auto& event : _events) {
			Write_Var_Len(file, event._delta_time);
			file.put(event._status);

			if (event._status == 0xFF) {
				file.put(event._data1);  // Meta event type
				file.put(event._data2);  // Length
				file.write(reinterpret_cast<const char*>(event._meta_data.data()),
					event._meta_data.size());
			}
			else {
				file.put(event._data1);
				file.put(event._data2);
			}
		}

		// Write end of track
		Write_Var_Len(file, 0);
		file.put(0xFF);
		file.put(0x2F);
		file.put(0x00);

		if (_debug_mode) {
			std::cout << "MIDI file written successfully\n";
			std::cout << "Final file position: " << file.tellp() << " bytes\n";
		}

		return true;
	}
}
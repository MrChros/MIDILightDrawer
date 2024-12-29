#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace MIDILightDrawer {

	class MIDI_Writer {
	private:
		struct Measure {
			uint8_t numerator;
			uint8_t denominator;
			uint32_t tempo_bpm;
			uint32_t start_tick;

			Measure(uint8_t num, uint8_t denom, uint32_t tempo, uint32_t tick)
				: numerator(num), denominator(denom), tempo_bpm(tempo), start_tick(tick) {
			}
		};

		struct MIDI_Event {
			uint32_t _delta_time;
			uint8_t _status;
			uint8_t _data1;
			uint8_t _data2;
			std::vector<uint8_t> _meta_data;

			MIDI_Event(uint32_t delta_time, uint8_t status, uint8_t data1, uint8_t data2)
				: _delta_time(delta_time), _status(status), _data1(data1), _data2(data2) {
			}

			MIDI_Event(uint32_t delta_time, uint8_t status, uint8_t meta_type, const std::vector<uint8_t>& meta_data)
				: _delta_time(delta_time), _status(status), _data1(meta_type), _meta_data(meta_data) {
				_data2 = static_cast<uint8_t>(meta_data.size());
			}
		};

		std::vector<MIDI_Event> _events;
		std::vector<Measure> _measures;
		uint16_t _ticks_per_quarter_note;
		uint32_t _current_tick;
		bool _debug_mode;

		void Write_Var_Len(std::ofstream& file, uint32_t value);
		void Write_Int32(std::ofstream& file, uint32_t value);
		void Write_Int16(std::ofstream& file, uint16_t value);
		uint32_t Calculate_Track_Size() const;
		void Add_Time_Signature_Event(uint32_t tick, uint8_t numerator, uint8_t denominator);
		void Add_Tempo_Event(uint32_t tick, uint32_t tempo_bpm);
		void Initialize_Track();
		void Debug_Print_Event(const MIDI_Event& event) const;

	public:
		MIDI_Writer(uint16_t ticks_per_quarter_note = 960, bool debug = false);

		void Add_Measure(uint8_t numerator, uint8_t denominator, uint32_t tempo_bpm);
		void Add_Note(uint32_t start_tick, uint32_t length_ticks, uint8_t channel, uint8_t note, uint8_t velocity);
		bool Save_To_File(const std::string& filename);
		void Set_Debug_Mode(bool debug) { _debug_mode = debug; }
	};
}
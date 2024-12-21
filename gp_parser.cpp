/* Copyright Phillip Potter, 2019 under MIT License
 * Based upon https://github.com/juliangruber/parse-gp5 (also MIT) */
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include "gp_parser.h"

namespace gp_parser {

/* This constructor takes a Guitar Pro file and reads it into the internal
 * vector for further use */
Parser::Parser(const std::string& filePath)
{
	// Open file
	if (filePath.empty()) {
		throw std::logic_error("Null file path passed to constructor");
	}

	std::ifstream file;
	file.open(filePath, std::ifstream::in | std::ifstream::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Unable to open file: " + filePath);
	}

	// Initialise vector
	fileBuffer = std::vector<char>(std::istreambuf_iterator<char>(file), {});

	// Close file
	file.close();

	// Parse version and check it is supported
	readVersion();
	if (!isSupportedVersion(version))
		throw std::logic_error("Unsupported version");

	// Parse out major and minor version numbers
	std::regex majorAndMinorExp("(\\d+)\\.(\\d+)");
	major = std::stoi(std::regex_replace(
			  version,
			  majorAndMinorExp,
			  "$1",
			  std::regex_constants::format_no_copy));
	minor = std::stoi(std::regex_replace(
			  version,
			  majorAndMinorExp,
			  "$2",
			  std::regex_constants::format_no_copy));

	// Read attributes of tab file
	title			= readStringByteSizeOfInteger();
	subtitle		= readStringByteSizeOfInteger();
	artist			= readStringByteSizeOfInteger();
	album			= readStringByteSizeOfInteger();
	lyricsAuthor	= readStringByteSizeOfInteger();
	musicAuthor		= readStringByteSizeOfInteger();
	copyright		= readStringByteSizeOfInteger();
	tab				= readStringByteSizeOfInteger();
	instructions	= readStringByteSizeOfInteger();

	auto commentLen = readInt();
	for (auto i = 0; i < commentLen; ++i)
		comments.push_back(readStringByteSizeOfInteger());

	// Read lyrics data
	lyricTrack	= readInt();
	lyric		= readLyrics();

	// RSE Master Effects
	// There are being ignored here
	if (versionIndex > 0) { // Only for Guitar Pro 5.1 or 5.2
		skip(19);
	}

	// Read page setup
	pageSetup = readPageSetup();

	// Read tempo name
	tempoName = readStringByteSizeOfInteger();

	// Read tempo value
	tempoValue = readInt();

	// Read Hide Tempp (skipped here)
	if (versionIndex > 0)
		skip(1);

	// Read key signature
	globalKeySignature = readKeySignature();
	
	// Octave
	readInt();

	// Read channels
	channels = readChannels();

	// Directions, 19 * Short Ints (skipped here)...
	skip(38);

	// MasterEffect Reverb (skipped here)...
	readInt();

	// Read measures and track count info
	measureCount	= readInt();
	trackCount		= readInt();

	// Read measure headers
	auto timeSignature = TimeSignature();
	timeSignature.numerator = 4;
	timeSignature.denominator.value = QUARTER;
	timeSignature.denominator.division.enters = 1;
	timeSignature.denominator.division.times = 1;
	
	for (auto i = 0; i < measureCount; ++i)
	{
		if (i > 0) {
			skip(1);
		}

		/*
		The first byte is the measure's flags. It lists the data given in the
        current measure.

        - *0x01*: numerator of the key signature
        - *0x02*: denominator of the key signature
        - *0x04*: beginning of repeat
        - *0x08*: end of repeat
        - *0x10*: number of alternate ending
        - *0x20*: presence of a marker
        - *0x40*: tonality of the measure
        - *0x80*: presence of a double bar
		*/

		std::uint8_t flags = readUnsignedByte();

		auto header = MeasureHeader();
		header.number = i + 1;
		header.start = 0;
		header.tempo.value = 120;
		header.repeatOpen = (flags & 0x04) != 0;

		if ((flags & 0x01) != 0) {
			timeSignature.numerator = readByte();
		}

		if ((flags & 0x02) != 0) {
			timeSignature.denominator.value = readByte();
		}

		header.timeSignature = timeSignature;

		if ((flags & 0x08) != 0) {
			header.repeatClose = (readByte() & 0xFF) - 1;
		}

		if ((flags & 0x20) != 0) {
			header.marker.measure = header.number;
			header.marker.title = readStringByteSizeOfInteger();
			header.marker.color = readColor();
		}

		if ((flags & 0x40) != 0) {
			// Global Key signature not assigned to measure header.
			// Not sure if this matters or not
			globalKeySignature = readKeySignature(); 
			skip(1);
		}

		if ((flags & 0x10) != 0) {
			header.repeatAlternative = readUnsignedByte();
		}

		if ((flags & 0x01) != 0 || (flags & 0x02) != 0) {
			header.beams[0] = readByte();
			header.beams[1] = readByte();
			header.beams[2] = readByte();
			header.beams[3] = readByte();
		}
		else if (i > 0) {
			header.beams[0] = measureHeaders.back().beams[0];
			header.beams[1] = measureHeaders.back().beams[1];
			header.beams[2] = measureHeaders.back().beams[2];
			header.beams[3] = measureHeaders.back().beams[3];
		}


		if ((flags & 0x10) == 0) { // Maybe here error??
			skip(1);
		}

		auto tripletFeel = readByte();

		if (tripletFeel == 1) {
			header.tripletFeel = "eigth";
		}
		else if (tripletFeel == 2) {
			header.tripletFeel = "sixteents";
		}
		else {
			header.tripletFeel = "none";
		}

		// Push header to vector
		measureHeaders.push_back(header);
	}

	// Read tracks
	for (auto number = 1; number <= trackCount; ++number)
	{
		auto track = Track();
		
		// Not sure if here is correct....
		if (number == 1 || versionIndex == 0) {
			skip(1);
		}
		
		auto track_flags = readUnsignedByte(); // This should be flags

		// Only the Drum Track information is interesting for me
		track.isDrumsTrack = (track_flags & 0x01) > 0 ? true : false;

		//if (number == 1 || versionIndex == 0) {
		//	skip(1);
		//}

		track.number = number;
		track.lyrics = number == lyricTrack ? lyric : Lyric();
		track.name = readStringByte(40);

		auto stringCount = readInt();
		for (auto i = 0; i < 7; ++i)
		{
			auto tuning = readInt();
			if (stringCount > i)
			{
				auto string = GuitarString();
				string.number = i + 1;
				string.value = tuning;
				track.strings.push_back(string);
			}
		}

		readInt();			// MIDI Port used
		readChannel(track); // MIDI Channel used
		
		// This line is not present in the Guitar Pro Python lib
		// readInt();			// MIDI channel used for effects

		// Fret Count according to Guitar Pro Python lib
		track.fretCount = readInt();			// ??? Number of frets used for this instrument here ???

		track.offset = readInt(); // The fret number at which a capo is placed (0 for no capo)
		track.color = readColor();

		// Flags2				: 2
		// RSE AutoAccentuation : 1
		// Channel Bank			: 1
		// RSE Humanize			: 1
		// ???					: 3 * 4 = 12
		// ???					: 12
		// RSE Instrument		: 4
		// RSE Unknown			: 4
		// RSE Sound Bank		: 4 -> Sum = 41

		// If version = 0
		//		RSE Effect No.	: 2
		//		Skip			: 1
		// Otherweise
		//		RSE Effect No.	: 4

		// If version = 1
				// RSE EQ		: 4

		// Version = 0			-> 41 + 3		= 44 Bytes
		// Version = 1			-> 41 + 4 + 4	= 49 Bytes

		skip(versionIndex > 0 ? 49 : 44);

		if (versionIndex > 0) {
			// Read RSE Instrument Effects
			readStringByteSizeOfInteger();
			readStringByteSizeOfInteger();
		}
		tracks.push_back(track);
	}
	
	skip(versionIndex == 0 ? 2 : 1);	// This is OK

	// Iterate through measures
	auto tempo = Tempo();
	tempo.value = tempoValue;
	auto start = QUARTER_TIME;
	for (auto i = 0; i < measureCount; ++i)
	{
		auto& header = measureHeaders[i];
		header.start = start;
		for (auto j = 0; j < trackCount; ++j)
		{
			Track& track = tracks[j];
			auto measure = Measure();
			measure.header = &header;
			measure.start = start;
			track.measures.push_back(measure);
			readMeasure(track.measures.back(), track, tempo, globalKeySignature);
			skip(1);
		}
		header.tempo = tempo;
		start += getLength(header);
	}
}

/* This reads an unsigned byte from the file buffer and increments the
 * position at the same time */
std::uint8_t Parser::readUnsignedByte()
{
	return static_cast<uint8_t>(fileBuffer[bufferPosition++]);
}

/* This reads a signed byte from the file buffer and increments the
 * position at the same time */
std::int8_t Parser::readByte()
{
	return static_cast<int8_t>(fileBuffer[bufferPosition++]);
}

/* This reads a signed 16-bit integer from the file buffer and increments the
 * position at the same time */
std::int16_t Parser::readShort()
{
	auto returnVal = static_cast<int16_t>(
		((fileBuffer[bufferPosition + 1] & 0xFF) << 8) |
		(fileBuffer[bufferPosition] & 0xFF)
		);
	bufferPosition += 2;

	return returnVal;
}

/* This reads a signed 32-bit integer from the file buffer in little-endian
 * mode and increments the position at the same time */
std::int32_t Parser::readInt()
{
	auto returnVal = static_cast<int32_t>(
			    ((fileBuffer[bufferPosition + 3] & 0xFF) << 24) |
			    ((fileBuffer[bufferPosition + 2] & 0xFF) << 16) |
			    ((fileBuffer[bufferPosition + 1] & 0xFF) << 8) |
			    (fileBuffer[bufferPosition] & 0xFF)
			    );
	bufferPosition += 4;

	return returnVal;
}

/* This version of the function takes no 'len' parameter and merely forwards
 * through to the full method by setting 'len' to be equal to 'size' */
std::string Parser::readString(size_t size)
{
	return readString(size, size);
}

/* This returns a string from the file buffer, in the general case by reading
 * 'size' bytes from the file buffer then converting it to a string of 'len'
 * bytes */
std::string Parser::readString(size_t size, size_t len)
{
	// Work out number of bytes to read
	auto bytesToRead = size > 0 ? size : len;

	// Read this number of bytes from the file buffer
	auto bytes = std::vector<char>(bytesToRead);
	std::copy(fileBuffer.begin() + bufferPosition,
		  fileBuffer.begin() + bufferPosition + bytesToRead,
		  bytes.begin());

	// Increment position
	bufferPosition += bytesToRead;

	// Convert to string and return
	return std::string(bytes.begin(),
			   len >= 0 && len <= bytesToRead ?
			   (bytes.begin() + len) : (bytes.begin() + size));
}

/* This returns a string from the file buffer, but using a byte before it to
 * tell it the length of the string */
std::string Parser::readStringByte(size_t size)
{
	return readString(size, readUnsignedByte());
}

/* This returns a string from the file buffer, but using an integer before it
 * to tell it the total number of bytes to read - the initial byte that is
 * read still gives the string length */
std::string Parser::readStringByteSizeOfInteger()
{
	size_t d = readInt() - 1;
	return readStringByte(d);
}

std::string Parser::readStringInteger()
{
	return readString(readInt());
}

/* This just moves the position past 'n' number of bytes in the file buffer */
void Parser::skip(std::size_t n)
{
	bufferPosition += n;
}

/* This reads the version data from the file buffer */
void Parser::readVersion()
{
	version = readStringByte(30);
}

/* This checks if the supplied version is supported by the parser */
bool Parser::isSupportedVersion(std::string& version)
{
	auto versionsCount = sizeof(VERSIONS) / sizeof(const char *);
	for (auto i = 0; i < versionsCount; ++i) {
		if (version.compare(VERSIONS[i]) == 0) {
			versionIndex = i;
			return true;
		}
	}
	return false;
}

/* This reads lyrics data */
Lyric Parser::readLyrics()
{
	auto lyric = Lyric();
	lyric.from = readInt();
	lyric.lyric = readStringInteger();

	for (auto i = 0; i < 4; ++i) {
		readInt();
		readStringInteger();
	}

	return lyric;
}

/* This reads the page setup data */
PageSetup Parser::readPageSetup()
{
	auto pageSetup = PageSetup();
	
	pageSetup.pageWidth = readInt();
	pageSetup.pageHeight = readInt();
	pageSetup.marginLeft = readInt();
	pageSetup.marginRight = readInt();
	pageSetup.marginTop = readInt();
	pageSetup.marginBottom = readInt();
	pageSetup.scoreSizeProportion = (float)readInt() / 100.0f;
	pageSetup.headerAndFooter = readShort();
	pageSetup.title = readStringByteSizeOfInteger();
	pageSetup.subtitle = readStringByteSizeOfInteger();
	pageSetup.artist = readStringByteSizeOfInteger();
	pageSetup.album = readStringByteSizeOfInteger();
	pageSetup.words = readStringByteSizeOfInteger();
	pageSetup.music = readStringByteSizeOfInteger();
	pageSetup.wordsAndMusic = readStringByteSizeOfInteger();
	pageSetup.copyright = readStringByteSizeOfInteger() + "\n" + readStringByteSizeOfInteger();
	pageSetup.pageNumber = readStringByteSizeOfInteger();

	return pageSetup;
}

/* This reads the key signature */
std::int8_t Parser::readKeySignature()
{
	auto keySignature = readByte();
	if (keySignature < 0)
		keySignature = 7 - keySignature;

	return keySignature;
}

/* This reads the channel attributes data */
std::vector<Channel> Parser::readChannels()
{
	std::vector<Channel> channels;
	for (auto i = 0; i < 64; ++i) {
		auto channel = Channel();
		channel.program = readInt();
		channel.volume = readByte();
		channel.balance = readByte();
		channel.chorus = readByte();
		channel.reverb = readByte();
		channel.phaser = readByte();
		channel.tremolo = readByte();
		
		if (i == 9) {
			channel.bank = "default percussion bank";
			channel.isPercussionChannel = true;
		} else {
			channel.bank = "default bank";
		}

		if (channel.program < 0) {
			channel.program = 0;
		}

		channels.push_back(channel);

		// Skip two blank bytes
		skip(2);
	}

	return channels;
}

/* Read a color value */
Color Parser::readColor()
{
	auto c = Color();
	c.r = readUnsignedByte();
	c.g = readUnsignedByte();
	c.b = readUnsignedByte();
	skip(1);

	return c;
}

/* Read a channel */
void Parser::readChannel(Track& track)
{
	auto gmChannel1 = readInt() - 1;
	auto gmChannel2 = readInt() - 1;
	if (gmChannel1 >= 0 && gmChannel1 < channels.size()) {
		// Allocate temporary buffer to hold chars for conversion
		auto gmChannel1Param = ChannelParam();
		auto gmChannel2Param = ChannelParam();
		gmChannel1Param.key = "gm channel 1";
		gmChannel1Param.value.resize(numOfDigits(gmChannel1));
		std::sprintf(&gmChannel1Param.value[0], "%d", gmChannel1);
		gmChannel2Param.key = "gm channel 2";
		gmChannel2Param.value.resize(numOfDigits(gmChannel1 != 9 ? gmChannel2 : gmChannel1));
		std::sprintf(&gmChannel2Param.value[0], "%d", gmChannel1 != 9 ? gmChannel2 : gmChannel1);

		// Copy channel to temporary variable
		Channel channel = channels[gmChannel1];

		// TODO: channel auxiliary, JS code below:
		/*for (let i = 0; i < channels.length; i++) {
			let channelAux = channels[i];
			for (let n = 0; n < channelAux.; i++) {

			}
		}*/

		if (channel.id == 0) {
			channel.id = (int32_t)(channels.size() + 1);
			channel.name = "TODO";
			channel.parameters.push_back(gmChannel1Param);
			channel.parameters.push_back(gmChannel2Param);
			channels.push_back(channel);
		}
		track.channelId = channel.id;
	}
}

/* Read a measure */
void Parser::readMeasure(Measure& measure, Track& track, Tempo& tempo, std::int8_t keySignature)
{
	for (auto voice = 0; voice < 2; ++voice)
	{
		auto start = measure.start;
		
		auto beats = readInt();
		for (auto k = 0; k < beats; ++k) {
			start += (int32_t)readBeat(start, measure, track, tempo, voice);
		}	
	}

	std::vector<Beat*> emptyBeats;
	for (auto i = 0; i < measure.beats.size(); ++i) {
		auto beatPtr = &measure.beats[i];
		auto empty = true;
		for (auto v = 0; v < beatPtr->voices.size(); ++v) {
			if (beatPtr->voices[v].notes.size() != 0)
				empty = false;
		}
		if (empty)
			emptyBeats.push_back(beatPtr);
	}
	for (auto beatPtr : emptyBeats) {
		for (auto i = 0; i < measure.beats.size(); ++i) {
			if (beatPtr == &measure.beats[i]) {
				measure.beats.erase(measure.beats.begin() + i);
				break;
			}
		}
	}
	measure.clef = getClef(track);
	measure.keySignature = keySignature;
}

/* Get measure length */
std::int32_t Parser::getLength(MeasureHeader& header)
{
	return static_cast<std::int32_t>(std::round(header.timeSignature.numerator *
		getTime(denominatorToDuration(header.timeSignature.denominator))));
}

/* Adds a new measure to the beat */
Beat& Parser::getBeat(Measure& measure, std::int32_t start)
{
	for (auto& beat : measure.beats) {
		if (beat.start == start)
			return beat;
	}

	auto beat = Beat();
	beat.voices.resize(2);
	beat.start = start;
	measure.beats.push_back(beat);

	return measure.beats[measure.beats.size() - 1];
}

/* Read mix change */
void Parser::readMixChange(Tempo& tempo)
{
	readByte(); // Instrument

	skip(16); // Read RSE Instrument
	auto volume = readByte();
	auto pan = readByte();
	auto chorus = readByte();
	auto reverb = readByte();
	auto phaser = readByte();
	auto tremolo = readByte();
	readStringByteSizeOfInteger(); // Tempo Name -> Is this correct
	auto tempoValue = readInt();
	if (volume >= 0)
		readByte();
	if (pan >= 0)
		readByte();
	if (chorus >= 0)
		readByte();
	if (reverb >= 0)
		readByte();
	if (phaser >= 0)
		readByte();
	if (tremolo >= 0)
		readByte();
	if (tempoValue >= 0) {
		tempo.value = tempoValue;
		skip(1); // Duration
		
		if (versionIndex > 0) {
			skip(1); // Hide Tempo
		}
	}

	readByte();	// Wah Effect
	
	skip(1);	// Not sure about this one.... Cannot find this Byte in Guitar Pro Python Library
	
	if (versionIndex > 0) {
		readStringByteSizeOfInteger(); // RSE Instrument Effect - Effect
		readStringByteSizeOfInteger(); // RSE Instrument Effect - Effect Category
	}
}

/* Read beat effects */
void Parser::readBeatEffects(Beat& beat, NoteEffect& noteEffect)
{
	auto flags1 = readUnsignedByte();
	auto flags2 = readUnsignedByte();
	
	noteEffect.vibrato	= (flags1 & 0x02) != 0;
	noteEffect.fadeIn	= (flags1 & 0x10) != 0;

	if ((flags1 & 0x20) != 0)
	{
		auto effect = readUnsignedByte();
		noteEffect.tapping = effect == 1;
		noteEffect.slapping = effect == 2;
		noteEffect.popping = effect == 3;
	}

	if ((flags2 & 0x04) != 0) {
		readTremoloBar(noteEffect);
	}

	if ((flags1 & 0x40) != 0)
	{
		auto strokeUp = readByte();
		auto strokeDown = readByte();
		// TODO
		if (strokeUp > 0) {
			beat.stroke.direction = "stroke_up";
			beat.stroke.value = "stroke_down";
		} else if (strokeDown > 0) {
			beat.stroke.direction = "stroke_down";
			beat.stroke.value = "stroke_down";
		}
	}

	if ((flags2 & 0x02) != 0) {
		readByte(); // Pick Stroke Direction
	}
}

/* Read tremolo bar */
void Parser::readTremoloBar(NoteEffect& effect)
{
	readByte();	// Type
	readInt();	// Value
	
	auto tremoloBar = TremoloBar();
	auto numPoints = readInt();
	for (auto i = 0; i < numPoints; ++i) {
		auto position = readInt();
		auto value = readInt();
		readByte();

		auto point = TremoloPoint();
		point.pointPosition = static_cast<std::int32_t>(std::round(
				position * 1.0	/*'max position length'*/ / 
				1.0				/*'bend position'*/)); // TODO
		point.pointValue = static_cast<std::int32_t>(std::round(
				value / (1.0	/*'GP_BEND_SEMITONE'*/
				* 0x2f)));		//TODO
		tremoloBar.points.push_back(point);
	}
	if (tremoloBar.points.size() > 0)
		effect.tremoloBar = tremoloBar;
}

/* Read beat text */
void Parser::readText(Beat& beat)
{
	beat.text.value = readStringByteSizeOfInteger();
}

/* Read chord */
void Parser::readChord(std::vector<GuitarString>& strings, Beat& beat)
{
	auto chord = Chord();
	chord.strings = &strings;
	skip(17);

	chord.name = readStringByte(21);
	skip(4);

	chord.frets.resize(6);
	chord.frets[0] = readInt();

	for (auto i = 0; i < 7; ++i)
	{
		auto fret = readInt();

		if (i < chord.strings->size()) {
			chord.frets[i] = fret;
		}
	}
	skip(32);

	if (chord.strings->size() > 0) {
		beat.chord = chord;
	}
}

/* Get duration */
double Parser::getTime(Duration duration)
{
	auto time = QUARTER_TIME * 4.0 / duration.value;
	if (duration.dotted)
		time += time / 2;
	else if (duration.doubleDotted)
		time += (time / 4) * 3;

	return time * duration.division.times / duration.division.enters;
}

/* Read duration */
//double Parser::readDuration(std::uint8_t flags)
Duration Parser::readDuration(std::uint8_t flags)
{
	auto duration = Duration();
	duration.value = pow(2, (readByte() + 4)) / 4;
	duration.dotted = (flags & 0x01) != 0;
	if ((flags & 0x20) != 0) {
		auto divisionType = readInt();
		switch (divisionType) {
		case 3:
			duration.division.enters = 3;
			duration.division.times = 2;
			break;
		case 5:
			duration.division.enters = 5;
			duration.division.times = 5;
			break;
		case 6:
			duration.division.enters = 6;
			duration.division.times = 4;
			break;
		case 7:
			duration.division.enters = 7;
			duration.division.times = 4;
			break;
		case 9:
			duration.division.enters = 9;
			duration.division.times = 8;
			break;
		case 10:
			duration.division.enters = 10;
			duration.division.times = 8;
			break;
		case 11:
			duration.division.enters = 11;
			duration.division.times = 8;
			break;
		case 12:
			duration.division.enters = 12;
			duration.division.times = 8;
			break;
		case 13:
			duration.division.enters = 13;
			duration.division.times = 8;
			break;
		}
	}
	if (duration.division.enters == 0) {
		duration.division.enters = 1;
		duration.division.times = 1;
	}

	//return getTime(duration);
	return duration;
}

/* Read beat */
double Parser::readBeat(std::int32_t start, Measure& measure, Track& track, Tempo& tempo, std::size_t voiceIndex)
{
	auto flags = readUnsignedByte();
	
	auto& beat = getBeat(measure, start);
	auto& voice = beat.voices[voiceIndex];

	if ((flags & 0x40) != 0) {
		auto beatType = readUnsignedByte();
		voice.empty = (beatType & 0x02) == 0;
	}

	auto duration = readDuration(flags);
	auto effect = NoteEffect();

	if ((flags & 0x02) != 0) {
		readChord(track.strings, beat);
	}

	if ((flags & 0x04) != 0) {
		readText(beat);
	}

	if ((flags & 0x08) != 0) {
		readBeatEffects(beat, effect);
	}

	if ((flags & 0x10) != 0) {
		readMixChange(tempo);
	}

	auto stringFlags = readUnsignedByte();

	for (auto i = 6; i >= 0; --i)
	{
		if ((stringFlags & (1 << i)) != 0 && (6 - i) < track.strings.size()) {
			auto string = track.strings[6 - i];
			auto note = readNote(string, track, effect);
			voice.notes.push_back(note);
		}
		voice.duration = duration;
		voice.durationInTicks = getTime(duration);
	}

	
	auto flags2 = readShort(); // Flags2 contain some additional information, which is skipped here...
	if ((flags2 & 0x0800) != 0) {
		readByte();	// Display - Break Secondary, skipped here...
	}

	return (voice.notes.size() != 0 ? voice.durationInTicks : 0);
}

/* Read note */
Note Parser::readNote(GuitarString& string, Track& track, NoteEffect& effect)
{
	auto flags = readUnsignedByte();
	//DBG_file << "Note Flags: " << (int)flags << std::endl;

	auto note = Note();
	note.string = string.number;
	note.effect = effect;
	note.effect.accentuatedNote = (flags & 0x40) != 0;
	note.effect.heavyAccentuatedNote = (flags & 0x02) != 0;
	note.effect.ghostNote = (flags & 0x04) != 0;

	if ((flags & 0x20) != 0)
	{
		auto noteType = readUnsignedByte();

		note.tiedNote = noteType == 0x02;
		note.effect.deadNote = noteType == 0x03;
	}

	if ((flags & 0x10) != 0)
	{
		note.velocity = TGVELOCITIES_MIN_VELOCITY +
		(TGVELOCITIES_VELOCITY_INCREMENT * readByte()) -
		TGVELOCITIES_VELOCITY_INCREMENT; // TODO
	}

	if ((flags & 0x20) != 0)
	{
		auto fret = readByte();
		auto value = note.tiedNote
			? getTiedNoteValue(string.number, track)
			: fret;

		note.value = value >= 0 && value < 100
			? value
			: 0;
	}

	if ((flags & 0x80) != 0)
	{
		readByte();	// Left Hand Finger
		readByte(); // Right Hand Finger
	}

	if ((flags & 0x01) != 0)
	{
		skip(8); // Duration Percent (Type: double)
	}

	readByte(); // Flags 2: Can be used to read 'Swap Accidentals', skipped here...
	
	if ((flags & 0x08) != 0) {
		readNoteEffects(note.effect);
	}

	return note;
}

/* Get tied note value */
std::int8_t Parser::getTiedNoteValue(std::int32_t string, Track& track)
{
	std::int32_t measureCount = static_cast<std::int32_t>(track.measures.size());
	
	if (measureCount > 0)
	{
		for (std::int32_t m = measureCount - 1; m >= 0; --m)
		{
			auto& measure = track.measures[m];
			for (std::int64_t b = static_cast<std::int64_t>(measure.beats.size()) - 1; b >= 0; --b)
			{
				auto& beat = measure.beats[b];
				for (std::int32_t v = 0; v < static_cast<std::int32_t>(beat.voices.size()); ++v)
				{
					auto& voice = beat.voices[v];
					if (!voice.empty)
					{
						for (std::int32_t n = 0; n < static_cast<std::int32_t>(voice.notes.size()); ++n)
						{
							auto& note = voice.notes[n];
							if (note.string == string) {
								return note.value;
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

/* Read effects for note */
void Parser::readNoteEffects(NoteEffect& noteEffect)
{
	auto flags1 = readUnsignedByte();
	auto flags2 = readUnsignedByte();

	if ((flags1 & 0x01) != 0) {
		readBend(noteEffect);
	}

	if ((flags1 & 0x10) != 0) {
		readGrace(noteEffect);
	}

	if ((flags2 & 0x04) != 0) {
		readTremoloPicking(noteEffect);
	}

	if ((flags2 & 0x08) != 0) {
		noteEffect.slide = true;
		readByte(); // This is OK
	}

	if ((flags2 & 0x10) != 0) {
		readArtificialHarmonic(noteEffect);
	}

	if ((flags2 & 0x20) != 0) {
		readTrill(noteEffect);
	}

	noteEffect.hammer	= (flags1 & 0x02) != 0;
	noteEffect.letRing	= (flags1 & 0x08) != 0;
	noteEffect.vibrato	= (flags2 & 0x40) != 0;
	noteEffect.palmMute = (flags2 & 0x02) != 0;
	noteEffect.staccato = (flags2 & 0x01) != 0;
}

/* Read bend */
void Parser::readBend(NoteEffect& effect)
{
	readByte();	// Bend Type
	readInt();	// Bend Value

	auto bend = Bend();
	auto numPoints = readInt();
	for (auto i = 0; i < numPoints; ++i) {
		auto bendPosition	= readInt();
		auto bendValue		= readInt();

		readByte(); // Vibrate (Type: bool)

		auto p = BendPoint();
		p.pointPosition = (int32_t)std::round(bendPosition * TGEFFECTBEND_MAX_POSITION_LENGTH / static_cast<double>(GP_BEND_POSITION));
		p.pointValue	= (int32_t)std::round(bendValue * TGEFFECTBEND_SEMITONE_LENGTH / static_cast<double>(GP_BEND_SEMITONE));
		bend.points.push_back(p);
	}
	if (bend.points.size() > 0)
		effect.bend = bend;
}

/* Read grace */
void Parser::readGrace(NoteEffect& effect)
{
	auto fret		= readUnsignedByte();
	auto dynamic	= readUnsignedByte();
	auto transition = readByte();
	auto duration	= readUnsignedByte();
	auto flags		= readUnsignedByte();

	auto grace = Grace();
	grace.fret = fret;

	grace.dynamic = (TGVELOCITIES_MIN_VELOCITY +
			(TGVELOCITIES_VELOCITY_INCREMENT * dynamic)) -
			TGVELOCITIES_VELOCITY_INCREMENT;

	grace.duration = duration;
	grace.dead		= (flags & 0x01) != 0;
	grace.onBeat	= (flags & 0x02) != 0;
	if (transition == 0)
		grace.transition = "none";
	else if (transition == 1)
		grace.transition = "slide";
	else if (transition == 2)
		grace.transition = "bend";
	else if (transition == 3)
		grace.transition = "hammer";
	effect.grace = grace;
}

/* Read tremolo picking */
void Parser::readTremoloPicking(NoteEffect& effect)
{
	auto value = readUnsignedByte();
	auto tp = TremoloPicking();

	if (value == 1) {
		tp.duration.value = "eigth";
		effect.tremoloPicking = tp;
	}
	else if (value == 2) {
		tp.duration.value = "sixteenth";
		effect.tremoloPicking = tp;
	}
	else if (value == 3) {
		tp.duration.value = "thirty_second";
		effect.tremoloPicking = tp;
	}
}

/* Read artificial harmonic */
void Parser::readArtificialHarmonic(NoteEffect& effect)
{
	auto type = readByte();
	auto harmonic = Harmonic();

	if (type == 1) {
		harmonic.type = "natural";
		effect.harmonic = harmonic;
	} 
	else if (type == 2) {
		skip(3);	// Semitone, Accidental and Octave
		harmonic.type = "artificial";
		effect.harmonic = harmonic;
	}
	else if (type == 3) {
		skip(1);	// Fret
		harmonic.type = "tapped";
		effect.harmonic = harmonic;
	}
	else if (type == 4) {
		harmonic.type = "pinch";
		effect.harmonic = harmonic;
	}
	else if (type == 5) {
		harmonic.type = "semi";
		effect.harmonic = harmonic;
	}
}

/* Read trill */
void Parser::readTrill(NoteEffect& effect)
{
	auto fret	= readByte();
	auto period = readByte();

	auto trill = Trill();
	trill.fret = fret;

	if (period == 1) {
		trill.duration.value = "sixteenth";
		effect.trill = trill;
	}
	else if (period == 2) {
		trill.duration.value = "thirty_second";
		effect.trill = trill;
	}
	else if (period == 3) {
		trill.duration.value = "sixty_fourth";
		effect.trill = trill;
	}
}

/* Tests if the channel corresponding to the supplied id is a
 * drum channel */
bool Parser::isPercussionChannel(std::int32_t channelId)
{
	for (auto& channel : channels) {
		if (channel.id == channelId)
			return channel.isPercussionChannel;
	}

	return false;
}

/* Get clef */
std::string Parser::getClef(Track& track)
{
	if (!isPercussionChannel(track.channelId)) {
		for (auto& string : track.strings) {
			if (string.value <= 34)
				return "CLEF_BASS";
		}
	}	

	return "CLEF_TREBLE";
}

/* This generates the same state as the XML blob, but in object
 * form that can be manipulated by the caller */
TabFile Parser::getTabFile()
{
	return TabFile(major, minor, title, subtitle, artist, album,
		       lyricsAuthor, musicAuthor, copyright, tab,
		       instructions, comments, lyric, pageSetup, tempoName, tempoValue,
		       globalKeySignature, channels, measureCount,
		       trackCount, measureHeaders, tracks);
}

/* Tells us how many digits there are in a base 10 number */
std::int32_t numOfDigits(std::int32_t num)
{
	auto digits = 0;
	for (auto order = 1; num / order != 0; order *= 10)
		++digits;

	return digits;
}

/* Converts a denominator struct to a duration struct */
Duration denominatorToDuration(Denominator& denominator)
{
	auto duration = Duration();
	duration.value = denominator.value;
	duration.division = denominator.division;

	return duration;
}

}

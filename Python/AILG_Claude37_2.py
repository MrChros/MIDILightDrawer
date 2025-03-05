import os
import math
import inspect
import colorsys
import guitarpro as GP
from PIL import Image, ImageDraw, ImageFont


QUARTER_NOTE_TICKS = 960
MIDI_LOWEST_OCTAVE = -2
AI_NAME = "Claude37_2"  # Claude Sonnet 3.7 (Challenged to generate something sophisticated)


class Light_Change:
    def __init__(self, track_name, tick_start, tick_duration, red : int, green: int, blue : int):
        self.Track_Name = track_name
        self.Tick_Start = tick_start
        self.Tick_Duration = tick_duration
        self.Color = (min(255, red), min(255, green), min(255, blue))


#---------------------------------------------------------------------------------------
# Main Function of the whole Skript
def main(): 
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    Song_List = []

    # Iterate directory
    for File in os.listdir(os.path.dirname(os.path.abspath(__file__))):
        # Check only text files
        if File.endswith('.gp5'):
            Song_List.append(os.path.basename(File).split('.')[0])

    for Song in Song_List:
        Generate_Light(Song)
        

#---------------------------------------------------------------------------------------
# Function to call
def Generate_Light(song_name : str): 
    global AI_NAME  

    # https://pyguitarpro.readthedocs.io/en/stable/
    Song : GP.Song = None
    try:
        Song = GP.parse(song_name + '.gp5')
    except FileNotFoundError as Error:
        Print_Error("Error parsing file '" + song_name + ".gp5': " + Error.args[1], 0, inspect.stack()[0][3])
        quit()
    
    Print_Message("Generating " + AI_NAME + " Light for Song '" + song_name + "'")
   
    # Find the track objects and assign the to the according entry in the track-dictionary
    Tracks = dict.fromkeys(["Guitar", "Bass", "Bass Drive", "Drums"])
    for Track_Name in Tracks:
        Tracks[Track_Name] = GuitarPro_Get_Track_From_Name(Track_Name, Song.tracks)

    All_Tracks_Found : bool = True
    for Track_Name in Tracks:
        if Tracks[Track_Name] is None:
            All_Tracks_Found = False
            Print_Error("Cound not find track '" + str(Track_Name), 1, inspect.stack()[0][3])

    if not All_Tracks_Found:
        return

    # Preprocess entire song to analyze structure for better lighting context
    song_structure = Analyze_Song_Structure(Song, Tracks)
    
    Light_Changes : list[Light_Change] = []
    Current_Measure_Tick = 0
    
    # Create timeline of all beats across all tracks for synchronization
    timeline = Create_Beat_Timeline(Song, Tracks)
    
    # Process song in segments to detect overall dynamics and patterns
    for MH in Song.measureHeaders:
        # Get the measure for each instrument from each track
        Measure_Guitar      : GP.Measure = Tracks["Guitar"].measures[MH.number-1]
        Measure_Bass        : GP.Measure = Tracks["Bass"].measures[MH.number-1]
        Measure_Bass_Drive  : GP.Measure = Tracks["Bass Drive"].measures[MH.number-1]
        Measure_Drums       : GP.Measure = Tracks["Drums"].measures[MH.number-1]

        # Get section info (verse, chorus, bridge, etc.) if available from markers
        section_type = "default"
        section_intensity = 1.0
        if MH.marker is not None:
            section_type, section_intensity = Parse_Section_Marker(MH.marker.title)

        # Process rhythm and intensity of this measure
        measure_rhythm_info = Analyze_Measure_Rhythm(Measure_Guitar, Measure_Bass, Measure_Drums)
        
        # Generate coordinated lights based on all instruments' interaction
        Generate_Coordinated_Lights(
            Light_Changes,
            Measure_Guitar, 
            Measure_Bass, 
            Measure_Bass_Drive,
            Measure_Drums,
            Current_Measure_Tick,
            section_type,
            section_intensity,
            measure_rhythm_info,
            song_structure
        )
        
        # Current_Measure_Tick += MH.length

    # Post-process the light changes to enhance coordination and smooth transitions
    Light_Changes = Post_Process_Light_Changes(Light_Changes, Song)

    FileName_wo_Ext = song_name + "_" + AI_NAME

    Draw_Color_Color_Sequence(FileName_wo_Ext, Song, Light_Changes)
    Write_Light_Configuration_File(FileName_wo_Ext, Song, Light_Changes)
    print("")    

    return 0


def Analyze_Song_Structure(song: GP.Song, tracks: dict) -> dict:
    structure = {
        "sections": [],
        "tempo_changes": [],
        "key_signature_changes": [],
        "major_transitions": []
    }
    
    current_section = "intro"
    current_section_start = 0
    
    for i, mh in enumerate(song.measureHeaders):
        # Detect section from marker
        if mh.marker is not None:
            if i > 0:  # Record previous section
                structure["sections"].append({
                    "type": current_section,
                    "start_measure": current_section_start,
                    "end_measure": i-1
                })
            
            # Parse marker for section info
            current_section, _ = Parse_Section_Marker(mh.marker.title)
            current_section_start = i
        
        # Detect tempo changes
        if i > 0:
            # Check if tempo attribute exists or how it should be accessed
            current_tempo = getattr(mh, 'tempo', None)
            prev_tempo = getattr(song.measureHeaders[i-1], 'tempo', None)
            
            # If direct tempo attribute doesn't exist, try alternative access methods
            if current_tempo is None:
                # Try common alternatives
                if hasattr(mh, 'direction') and hasattr(mh.direction, 'tempo'):
                    current_tempo = mh.direction.tempo
                elif hasattr(song, 'tempo') and hasattr(song, 'measureTempoChanges'):
                    # Some versions store tempo changes in a separate collection
                    current_tempo = next((tc.tempo for tc in song.measureTempoChanges if tc.measure == i), None)
            
            if prev_tempo is None:
                # Similar fallbacks for previous measure
                if hasattr(song.measureHeaders[i-1], 'direction') and hasattr(song.measureHeaders[i-1].direction, 'tempo'):
                    prev_tempo = song.measureHeaders[i-1].direction.tempo
                elif hasattr(song, 'tempo') and hasattr(song, 'measureTempoChanges'):
                    prev_tempo = next((tc.tempo for tc in song.measureTempoChanges if tc.measure == i-1), None)
            
            # Now compare if both tempos are available
            if current_tempo is not None and prev_tempo is not None and current_tempo != prev_tempo:
                structure["tempo_changes"].append({
                    "measure": i,
                    "tempo": current_tempo
                })
        
        # Detect key signature changes
        if i > 0:
            # Check if keySignature exists and how it should be accessed
            current_ks = getattr(mh, 'keySignature', None)
            prev_ks = getattr(song.measureHeaders[i-1], 'keySignature', None)
            
            # If keySignature objects exist, check if we can detect a change
            if current_ks is not None and prev_ks is not None:
                # Try to get key and isMinor attributes
                current_key = getattr(current_ks, 'key', None)
                current_minor = getattr(current_ks, 'isMinor', None)
                prev_key = getattr(prev_ks, 'key', None)
                prev_minor = getattr(prev_ks, 'isMinor', None)
                
                # If attributes don't exist, try alternatives
                if current_key is None and hasattr(current_ks, 'value'):
                    current_key = current_ks.value  # Some versions use value instead of key
                if prev_key is None and hasattr(prev_ks, 'value'):
                    prev_key = prev_ks.value
                    
                # Check for mode (major/minor) using alternative attributes
                if current_minor is None and hasattr(current_ks, 'mode'):
                    current_minor = (current_ks.mode == 1)  # Often 1 indicates minor
                if prev_minor is None and hasattr(prev_ks, 'mode'):
                    prev_minor = (prev_ks.mode == 1)
                
                # Now check if there's a change in key signature
                if ((current_key is not None and prev_key is not None and current_key != prev_key) or
                    (current_minor is not None and prev_minor is not None and current_minor != prev_minor)):
                    structure["key_signature_changes"].append({
                        "measure": i,
                        "key": current_key,
                        "is_minor": current_minor
                    })
    
    # Add final section
    structure["sections"].append({
        "type": current_section,
        "start_measure": current_section_start,
        "end_measure": len(song.measureHeaders) - 1
    })
    
    # Analyze harmony density across tracks for major transition points
    # This helps identify verse-chorus transitions, etc.
    for i in range(1, len(song.measureHeaders) - 1):
        current_density = Calculate_Harmonic_Density(tracks, i)
        prev_density = Calculate_Harmonic_Density(tracks, i-1)
        
        # Significant change in harmonic density indicates transition
        if current_density > prev_density * 1.5 or current_density < prev_density * 0.67:
            structure["major_transitions"].append({
                "measure": i,
                "transition_type": "buildup" if current_density > prev_density else "breakdown"
            })
    
    return structure
    
    
def Calculate_Harmonic_Density(tracks: dict, measure_index: int) -> float:
    total_notes = 0
    total_beats = 0
    
    for track_name, track in tracks.items():
        if track is None or track_name == "Bass Drive":
            continue
        
        measure = track.measures[measure_index]
        for voice in measure.voices:
            for beat in voice.beats:
                total_beats += 1
                total_notes += len(beat.notes) if beat.notes else 0
    
    return total_notes / max(1, total_beats)


def Parse_Section_Marker(marker_title: str) -> tuple:
    marker_title = marker_title.lower()
    
    # Default values
    section_type = "default"
    section_intensity = 1.0
    
    # Check for common section types
    if "intro" in marker_title:
        section_type = "intro"
        section_intensity = 0.8
    elif "verse" in marker_title:
        section_type = "verse"
        section_intensity = 0.9
    elif "pre-chorus" in marker_title or "pre chorus" in marker_title:
        section_type = "pre_chorus"
        section_intensity = 1.1
    elif "chorus" in marker_title:
        section_type = "chorus"
        section_intensity = 1.3
    elif "bridge" in marker_title:
        section_type = "bridge"
        section_intensity = 1.0
    elif "solo" in marker_title:
        section_type = "solo"
        section_intensity = 1.4
    elif "outro" in marker_title:
        section_type = "outro"
        section_intensity = 0.85
    elif "breakdown" in marker_title:
        section_type = "breakdown"
        section_intensity = 1.5
    
    # Check for intensity modifiers
    if "quiet" in marker_title or "soft" in marker_title:
        section_intensity *= 0.7
    elif "loud" in marker_title or "heavy" in marker_title:
        section_intensity *= 1.3
    elif "intense" in marker_title:
        section_intensity *= 1.5
    
    # Extract custom intensity if specified with bracket format like [intensity:1.2]
    import re
    intensity_match = re.search(r'\[intensity:([\d\.]+)\]', marker_title)
    if intensity_match:
        try:
            custom_intensity = float(intensity_match.group(1))
            section_intensity = custom_intensity
        except ValueError:
            pass
            
    return section_type, section_intensity



def Create_Beat_Timeline(song: GP.Song, tracks: dict) -> dict:
    timeline = {}
    
    current_tick = 0
    for measure_index, mh in enumerate(song.measureHeaders):
        measure_start_tick = current_tick
        
        for track_name, track in tracks.items():
            if track is None:
                continue
                
            measure = track.measures[measure_index]
            for voice in measure.voices:
                for beat in voice.beats:
                    beat_start = measure_start_tick + beat.start
                    beat_end = beat_start + beat.duration.time
                    
                    # Initialize if doesn't exist
                    if beat_start not in timeline:
                        timeline[beat_start] = {"instruments": {}}
                    
                    # Store beat information for this instrument
                    timeline[beat_start]["instruments"][track_name] = {
                        "duration": beat.duration.time,
                        "notes": beat.notes,
                        "effects": beat.effect
                    }
        
        current_tick += mh.length
    
    return timeline
    
    
    
def Analyze_Measure_Rhythm(guitar_measure: GP.Measure, bass_measure: GP.Measure, drums_measure: GP.Measure) -> dict:
    rhythm_info = {
        "syncopation_level": 0.0,  # 0.0 - 1.0
        "density": 0.0,            # Notes per beat
        "dominant_instrument": None,
        "rhythm_complexity": 0.0,  # 0.0 - 1.0
        "beat_positions": set()    # Set of all positions where beats occur
    }
    
    # Analyze each instrument's rhythm
    guitar_rhythm = Extract_Rhythm_Pattern(guitar_measure)
    bass_rhythm = Extract_Rhythm_Pattern(bass_measure)
    drums_rhythm = Extract_Rhythm_Pattern(drums_measure)
    
    # Combine all beat positions
    all_positions = guitar_rhythm["positions"] | bass_rhythm["positions"] | drums_rhythm["positions"]
    rhythm_info["beat_positions"] = all_positions
    
    # Calculate density
    total_notes = (guitar_rhythm["note_count"] + bass_rhythm["note_count"] + 
                  drums_rhythm["note_count"])
    measure_length = 4  # Assuming 4/4 time for simplicity, adjust if needed
    rhythm_info["density"] = total_notes / (measure_length * 3)  # Normalize by all 3 instruments
    
    # Determine dominant instrument
    instruments = [
        ("Guitar", guitar_rhythm["note_count"]), 
        ("Bass", bass_rhythm["note_count"]), 
        ("Drums", drums_rhythm["note_count"])
    ]
    dominant = max(instruments, key=lambda x: x[1])
    rhythm_info["dominant_instrument"] = dominant[0]
    
    # Calculate syncopation level (notes on off-beats)
    beat_divisions = 4  # Assuming 16th note resolution
    on_beat_positions = {i * (QUARTER_NOTE_TICKS / beat_divisions) 
                         for i in range(int(measure_length * beat_divisions))}
    
    off_beat_count = len(all_positions - on_beat_positions)
    rhythm_info["syncopation_level"] = off_beat_count / max(1, len(all_positions))
    
    # Calculate rhythm complexity (based on variety of note durations)
    all_durations = (guitar_rhythm["durations"] | bass_rhythm["durations"] | 
                    drums_rhythm["durations"])
    rhythm_info["rhythm_complexity"] = min(1.0, len(all_durations) / 8)  # Normalize, max 8 types
    
    return rhythm_info
    
    
    
def Extract_Rhythm_Pattern(measure: GP.Measure) -> dict:
    pattern = {
        "positions": set(),
        "durations": set(),
        "note_count": 0
    }
    
    for voice in measure.voices:
        for beat in voice.beats:
            if not beat.notes:
                continue
                
            pattern["positions"].add(beat.start)
            pattern["durations"].add(beat.duration.time)
            pattern["note_count"] += len(beat.notes)
    
    return pattern


def Generate_Coordinated_Lights(light_changes: list[Light_Change],
                               guitar_measure: GP.Measure,
                               bass_measure: GP.Measure,
                               bass_drive_measure: GP.Measure,
                               drums_measure: GP.Measure,
                               measure_start_tick: int,
                               section_type: str,
                               section_intensity: float,
                               rhythm_info: dict,
                               song_structure: dict):
    
    # Create instrument-specific light changes first
    guitar_lights = Process_Guitar_Measure(guitar_measure, measure_start_tick, section_type, section_intensity)
    bass_lights = Process_Bass_Measure(bass_measure, bass_drive_measure, measure_start_tick, section_type, section_intensity)
    drum_lights = Process_Drums_Measure(drums_measure, measure_start_tick, section_type, section_intensity)
    
    # Extend the light changes with instrument-specific lights
    light_changes.extend(guitar_lights)
    light_changes.extend(bass_lights)
    light_changes.extend(drum_lights)
    
    # Generate additional coordinated lights based on relationships between instruments
    if rhythm_info["syncopation_level"] > 0.5 and rhythm_info["density"] > 0.7:
        # For highly syncopated, dense sections, create unified accent lights
        Create_Accent_Lights(light_changes, rhythm_info, measure_start_tick, section_intensity)
    
    # For sections with significant bass and drums interaction
    if bass_measure and drums_measure:
        Create_Rhythm_Section_Lights(light_changes, bass_measure, drums_measure, measure_start_tick, section_type, section_intensity)
    
    # Special handling for solos or dominant instrument
    if section_type == "solo" or rhythm_info["density"] > 1.0:
        Enhance_Dominant_Instrument(light_changes, rhythm_info["dominant_instrument"], measure_start_tick, section_intensity)
                                    


def Process_Guitar_Measure(measure: GP.Measure, measure_start_tick: int, 
                          section_type: str, section_intensity: float) -> list[Light_Change]:
    guitar_lights = []
    
    # Get key signature to determine if we're in major or minor
    is_minor = False  # Default to major if not specified
    if hasattr(measure, 'keySignature') and hasattr(measure.keySignature, 'isMinor'):
        is_minor = measure.keySignature.isMinor
    
    # Define color palettes based on musical mood
    if is_minor:
        # Minor keys - cooler, more melancholic colors
        low_color = (180, 50, 90)    # Purple-red for low notes
        mid_color = (100, 80, 180)   # Purple for mid-range
        high_color = (70, 130, 200)  # Blue for high notes
    else:
        # Major keys - warmer, brighter colors
        low_color = (220, 60, 40)    # Red-orange for low notes
        mid_color = (220, 160, 40)   # Yellow for mid-range
        high_color = (140, 200, 60)  # Green for high notes
    
    # Adjust for section
    if section_type == "chorus":
        # Brighter, more saturated colors for chorus
        low_color = tuple(min(255, int(c * 1.2)) for c in low_color)
        mid_color = tuple(min(255, int(c * 1.2)) for c in mid_color)
        high_color = tuple(min(255, int(c * 1.2)) for c in high_color)
    elif section_type == "verse":
        # Slightly desaturated for verses
        low_color = tuple(int(c * 0.9) for c in low_color)
        mid_color = tuple(int(c * 0.9) for c in mid_color)
        high_color = tuple(int(c * 0.9) for c in high_color)
    elif section_type == "breakdown":
        # Very intense, high contrast colors for breakdowns
        low_color = (255, 30, 30)   # Intense red
        mid_color = (30, 30, 255)   # Intense blue
        high_color = (255, 255, 255) # White
    
    for voice in measure.voices:
        for beat in voice.beats:
            if not beat.notes:
                continue
            
            # Advanced musical analysis
            notes_count = len(beat.notes)
            note_values = [note.value for note in beat.notes]
            
            # Determine if it's a chord and what type
            is_chord = notes_count > 1
            chord_type = "power" if is_chord and len(set(note % 12 for note in note_values)) <= 2 else \
                        "major" if is_chord and sum(1 for n in note_values if (n+3) % 12 in [n % 12 for n in note_values]) > 0 else \
                        "minor" if is_chord and sum(1 for n in note_values if (n+4) % 12 in [n % 12 for n in note_values]) > 0 else \
                        "other"
            
            # Calculate average pitch
            avg_pitch = sum(note_values) / notes_count if notes_count > 0 else 0
            
            # Map to normalized range (0-1) for guitar
            # Guitar range typically E2 (40) to E5 (88)
            normalized_pitch = min(1.0, max(0.0, (avg_pitch - 40) / 48))
            
            # Interpolate between colors based on pitch
            if normalized_pitch < 0.5:
                # Interpolate between low and mid
                t = normalized_pitch * 2
                red = int(low_color[0] * (1 - t) + mid_color[0] * t)
                green = int(low_color[1] * (1 - t) + mid_color[1] * t)
                blue = int(low_color[2] * (1 - t) + mid_color[2] * t)
            else:
                # Interpolate between mid and high
                t = (normalized_pitch - 0.5) * 2
                red = int(mid_color[0] * (1 - t) + high_color[0] * t)
                green = int(mid_color[1] * (1 - t) + high_color[1] * t)
                blue = int(mid_color[2] * (1 - t) + high_color[2] * t)
            
            # Adjust colors based on chord type
            if chord_type == "major":
                green = min(255, int(green * 1.2))  # Brighter for major
            elif chord_type == "minor":
                blue = min(255, int(blue * 1.2))    # More blue for minor
            elif chord_type == "power":
                red = min(255, int(red * 1.3))      # More red for power chords
            
            # Calculate intensity based on musical techniques
            intensity = section_intensity
            
            # Analyze effects
            has_vibrato = any(note.effect.vibrato for note in beat.notes)
            has_bend = any(note.effect.bend for note in beat.notes)
            has_harmonic = any(note.effect.harmonic for note in beat.notes)
            has_tremolo = any(note.effect.tremoloPicking for note in beat.notes)
            has_trill = any(note.effect.trill for note in beat.notes)
            
            # Adjust intensity based on effects
            if has_vibrato:
                intensity *= 1.1
                # Oscillating light effect for vibrato
                for i in range(3):  # Create multiple lights for vibrato effect
                    vibrato_intensity = 0.8 + 0.4 * (i % 2)  # Alternating intensity
                    vibrato_tick_offset = i * (QUARTER_NOTE_TICKS // 16)
                    
                    # Shorter pulses
                    vib_duration = min(QUARTER_NOTE_TICKS // 8, beat.duration.time // 3)
                    
                    vib_red = int(red * vibrato_intensity)
                    vib_green = int(green * vibrato_intensity)
                    vib_blue = int(blue * vibrato_intensity)
                    
                    guitar_lights.append(Light_Change(
                        "Guitar", 
                        measure_start_tick + beat.start + vibrato_tick_offset, 
                        vib_duration, 
                        vib_red, vib_green, vib_blue
                    ))
            
            if has_bend:
                intensity *= 1.2
                # Color shift for bend - increase blue
                blue = min(255, int(blue * 1.3))
                red = max(0, int(red * 0.9))
            
            if has_harmonic:
                intensity *= 1.15
                # Brighter, more ethereal color for harmonics
                green = min(255, int(green * 1.2))
                blue = min(255, int(blue * 1.2))
            
            if has_tremolo:
                intensity *= 1.3
                # Multiple rapid pulses for tremolo
                tremolo_divisions = 4
                tremolo_duration = beat.duration.time // tremolo_divisions
                
                for i in range(tremolo_divisions):
                    tremolo_intensity = 0.7 + 0.6 * ((i % 2) * 0.5)  # Alternating intensity
                    
                    trem_red = int(red * tremolo_intensity)
                    trem_green = int(green * tremolo_intensity)
                    trem_blue = int(blue * tremolo_intensity)
                    
                    guitar_lights.append(Light_Change(
                        "Guitar", 
                        measure_start_tick + beat.start + (i * tremolo_duration), 
                        tremolo_duration, 
                        trem_red, trem_green, trem_blue
                    ))
            
            if has_trill:
                intensity *= 1.25
                # Rapidly alternating colors for trill
                trill_divisions = 3
                trill_duration = beat.duration.time // trill_divisions
                
                for i in range(trill_divisions):
                    if i % 2 == 0:
                        # First note color
                        trill_red, trill_green, trill_blue = red, green, blue
                    else:
                        # Second note - shift color
                        trill_red = green
                        trill_green = blue
                        trill_blue = red
                    
                    guitar_lights.append(Light_Change(
                        "Guitar", 
                        measure_start_tick + beat.start + (i * trill_duration), 
                        trill_duration, 
                        trill_red, trill_green, trill_blue
                    ))
            
            # Only add a regular light if we haven't added any special effect lights
            if not (has_vibrato or has_tremolo or has_trill):
                # Apply final intensity adjustments
                red = min(255, int(red * intensity))
                green = min(255, int(green * intensity))
                blue = min(255, int(blue * intensity))
                
                # Create basic light change
                guitar_lights.append(Light_Change(
                    "Guitar", 
                    measure_start_tick + beat.start, 
                    beat.duration.time, 
                    red, green, blue
                ))
    
    return guitar_lights



def Process_Bass_Measure(bass_measure: GP.Measure, bass_drive_measure: GP.Measure, 
                        measure_start_tick: int, section_type: str, 
                        section_intensity: float) -> list[Light_Change]:
    bass_lights = []
    
    # Bass is generally in a lower register, so we'll use warmer colors
    base_low_color = (160, 40, 20)    # Deep red for lowest notes
    base_high_color = (140, 110, 40)  # Amber for higher bass notes
    
    # Analyze the entire bass measure for groove pattern
    groove_pattern = Analyze_Bass_Groove(bass_measure)
    
    for voice in bass_measure.voices:
        for beat in voice.beats:
            if not beat.notes:
                continue
            
            # Find lowest note (bass often emphasizes the root)
            notes_count = len(beat.notes)
            lowest_note = min(note.value for note in beat.notes) if notes_count > 0 else 0
            
            # Bass typically ranges from E1 (28) to G3 (55)
            normalized_pitch = min(1.0, max(0.0, (lowest_note - 28) / 27))
            
            # Interpolate between low and high colors
            red = int(base_low_color[0] * (1 - normalized_pitch) + base_high_color[0] * normalized_pitch)
            green = int(base_low_color[1] * (1 - normalized_pitch) + base_high_color[1] * normalized_pitch)
            blue = int(base_low_color[2] * (1 - normalized_pitch) + base_high_color[2] * normalized_pitch)
            
            # Check for bass drive
            drive_intensity = Check_Bass_Drive(bass_drive_measure, beat.start)
            
            # Analyze rhythm importance of this note in the groove
            beat_importance = Get_Beat_Importance(beat.start, groove_pattern)
            
            # Increase intensity based on drive and beat importance
            intensity = section_intensity * (1.0 + (drive_intensity * 0.5)) * (1.0 + (beat_importance * 0.3))
            
            # Apply intensity
            red = min(255, int(red * intensity))
            green = min(255, int(green * intensity))
            blue = min(255, int(blue * intensity))
            
            # Modify colors based on section
            if section_type == "chorus":
                red = min(255, int(red * 1.2))
                green = min(255, int(green * 1.1))
            elif section_type == "breakdown":
                red = min(255, int(red * 1.4))
                green = int(green * 0.8)
                blue = int(blue * 0.7)
            
            duration = beat.duration.time
           
            # Create the light change
            bass_lights.append(Light_Change(
                "Bass", 
                measure_start_tick + beat.start, 
                duration, 
                red, green, blue
            ))
            
            # Add slight tail light for sustained bass notes (if duration is long enough)
            if duration > QUARTER_NOTE_TICKS / 2:
                tail_duration = min(QUARTER_NOTE_TICKS / 4, duration / 3)
                tail_start = measure_start_tick + beat.start + duration - tail_duration
                
                # Softer, fading color for tail
                tail_intensity = 0.5
                tail_red = int(red * tail_intensity)
                tail_green = int(green * tail_intensity)
                tail_blue = int(blue * tail_intensity)
                
                bass_lights.append(Light_Change(
                    "Bass", 
                    int(tail_start), 
                    int(tail_duration), 
                    tail_red, tail_green, tail_blue
                ))
    
    return bass_lights
    
def Process_Drums_Measure(measure: GP.Measure, measure_start_tick: int, 
                         section_type: str, section_intensity: float) -> list[Light_Change]:
    drum_lights = []
    
    # Define base colors for different drum elements
    kick_color = (200, 50, 50)    # Red for kick
    snare_color = (100, 150, 100) # Green-ish for snare
    hihat_color = (50, 100, 150)  # Blue-ish for hi-hat
    crash_color = (200, 200, 200) # White for crash
    tom_color = (100, 80, 120)    # Purple-ish for toms
    
    # Adjust colors based on section type
    if section_type == "chorus":
        # Brighter drums for chorus
        kick_color = tuple(min(255, int(c * 1.2)) for c in kick_color)
        snare_color = tuple(min(255, int(c * 1.2)) for c in snare_color)
        crash_color = tuple(min(255, int(c * 1.3)) for c in crash_color)
    elif section_type == "verse":
        # Slightly subdued for verses
        kick_color = tuple(int(c * 0.9) for c in kick_color)
        snare_color = tuple(int(c * 0.9) for c in snare_color)
    elif section_type == "breakdown":
        # Very intense for breakdowns
        kick_color = (255, 40, 40)  # Intense red
        snare_color = (40, 255, 40) # Intense green
    
    # Track active drum events to prevent overlapping lights
    active_drum_types = {}
    
    for voice in measure.voices:
        for beat in voice.beats:
            if not beat.notes:
                continue
            
            # Analyze what kind of drum hits we have
            has_kick = False
            has_snare = False
            has_hihat = False
            has_crash = False
            has_tom = False
            hit_velocity = 0
            
            for note in beat.notes:
                # Keep track of average velocity
                hit_velocity += note.velocity
                
                # MIDI mapping for standard drums (may need adjustment based on your GP5 files)
                if note.value == 35 or note.value == 36:  # Bass Drum
                    has_kick = True
                elif note.value == 38 or note.value == 40:  # Snare
                    has_snare = True
                elif note.value in [42, 44, 46]:  # Hi-Hat
                    has_hihat = True
                elif note.value in [49, 57]:  # Crash
                    has_crash = True
                elif note.value in [41, 43, 45, 47, 48, 50]:  # Toms
                    has_tom = True
            
            # Average velocity
            if beat.notes:
                hit_velocity = hit_velocity / len(beat.notes)
                velocity_factor = hit_velocity / 90  # Normalizing around typical velocity
            else:
                velocity_factor = 1.0
            
            # Apply section intensity and velocity
            intensity = section_intensity * velocity_factor
            
            # Generate lights for different drum elements
            start_tick = measure_start_tick + beat.start
            
            # Process kick drum
            if has_kick:
                # Kick often drives the rhythm section, so make it prominent
                kick_duration = QUARTER_NOTE_TICKS // 4  # Short, percussive
                
                # More intense kick for important beats (1 and 3 in 4/4)
                beat_in_measure = (beat.start / QUARTER_NOTE_TICKS) % 4
                if beat_in_measure < 0.25 or (beat_in_measure > 2 and beat_in_measure < 2.25):
                    kick_intensity = intensity * 1.2
                else:
                    kick_intensity = intensity
                
                # Calculate color
                r = min(255, int(kick_color[0] * kick_intensity))
                g = min(255, int(kick_color[1] * kick_intensity))
                b = min(255, int(kick_color[2] * kick_intensity))
                
                drum_lights.append(Light_Change(
                    "Drums",
                    start_tick,
                    kick_duration,
                    r, g, b
                ))
            
            # Process snare
            if has_snare:
                # Snare often on beats 2 and 4 in 4/4
                snare_duration = QUARTER_NOTE_TICKS // 3  # Medium duration
                
                # Adjust intensity based on typical snare placement
                beat_in_measure = (beat.start / QUARTER_NOTE_TICKS) % 4
                if beat_in_measure > 0.9 and beat_in_measure < 1.1 or beat_in_measure > 2.9 and beat_in_measure < 3.1:
                    snare_intensity = intensity * 1.15
                else:
                    snare_intensity = intensity
                
                # Calculate color
                r = min(255, int(snare_color[0] * snare_intensity))
                g = min(255, int(snare_color[1] * snare_intensity))
                b = min(255, int(snare_color[2] * snare_intensity))
                
                # Add slight offset to avoid perfect overlap with kick
                snare_offset = QUARTER_NOTE_TICKS // 32
                
                drum_lights.append(Light_Change(
                    "Drums",
                    start_tick + snare_offset,
                    snare_duration,
                    r, g, b
                ))
            
            # Process hi-hat
            if has_hihat:
                # Hi-hat is typically shorter
                hihat_duration = QUARTER_NOTE_TICKS // 6
                
                # Calculate color
                r = min(255, int(hihat_color[0] * intensity))
                g = min(255, int(hihat_color[1] * intensity))
                b = min(255, int(hihat_color[2] * intensity))
                
                # Add slight offset to avoid perfect overlap
                hihat_offset = QUARTER_NOTE_TICKS // 24
                
                drum_lights.append(Light_Change(
                    "Drums",
                    start_tick + hihat_offset,
                    hihat_duration,
                    r, g, b
                ))
            
            # Process crash
            if has_crash:
                # Crashes are typically accents and last longer
                crash_duration = QUARTER_NOTE_TICKS // 2
                
                # Crashes are often emphasized
                crash_intensity = intensity * 1.3
                
                # Calculate color
                r = min(255, int(crash_color[0] * crash_intensity))
                g = min(255, int(crash_color[1] * crash_intensity))
                b = min(255, int(crash_color[2] * crash_intensity))
                
                drum_lights.append(Light_Change(
                    "Drums",
                    start_tick,
                    crash_duration,
                    r, g, b
                ))
            
            # Process toms
            if has_tom:
                # Toms have medium duration
                tom_duration = QUARTER_NOTE_TICKS // 3
                
                # Calculate color
                r = min(255, int(tom_color[0] * intensity))
                g = min(255, int(tom_color[1] * intensity))
                b = min(255, int(tom_color[2] * intensity))
                
                # Add slight offset to avoid perfect overlap
                tom_offset = QUARTER_NOTE_TICKS // 16
                
                drum_lights.append(Light_Change(
                    "Drums",
                    start_tick + tom_offset,
                    tom_duration,
                    r, g, b
                ))
            
            # If multiple elements hit simultaneously, create a combined accent
            if sum([has_kick, has_snare, has_crash, has_tom]) >= 2:
                # Combine colors
                combined_r = 0
                combined_g = 0
                combined_b = 0
                element_count = 0
                
                if has_kick:
                    combined_r += kick_color[0]
                    combined_g += kick_color[1]
                    combined_b += kick_color[2]
                    element_count += 1
                
                if has_snare:
                    combined_r += snare_color[0]
                    combined_g += snare_color[1]
                    combined_b += snare_color[2]
                    element_count += 1
                
                if has_crash:
                    combined_r += crash_color[0]
                    combined_g += crash_color[1]
                    combined_b += crash_color[2]
                    element_count += 1
                
                if has_tom:
                    combined_r += tom_color[0]
                    combined_g += tom_color[1]
                    combined_b += tom_color[2]
                    element_count += 1
                
                if element_count > 0:
                    combined_r = int(combined_r / element_count)
                    combined_g = int(combined_g / element_count)
                    combined_b = int(combined_b / element_count)
                
                # Boost the intensity for combined hits
                accent_intensity = intensity * 1.25
                combined_r = min(255, int(combined_r * accent_intensity))
                combined_g = min(255, int(combined_g * accent_intensity))
                combined_b = min(255, int(combined_b * accent_intensity))
                
                # Short bright accent for combined hits
                accent_duration = QUARTER_NOTE_TICKS // 8
                
                drum_lights.append(Light_Change(
                    "Drums",
                    start_tick,
                    accent_duration,
                    combined_r, combined_g, combined_b
                ))
    
    return drum_lights
    
    
    
def Post_Process_Light_Changes(light_changes: list[Light_Change], song: GP.Song) -> list[Light_Change]:
    """
    Post-processes all light changes to enhance coordination and create smoother transitions.
    - Removes overlapping lights
    - Enhances transitions between sections
    - Balances light intensity across instruments
    """
    # Sort light changes by start time
    light_changes.sort(key=lambda lc: lc.Tick_Start)
    
    # Initialize measure boundary information
    measure_boundaries = []
    current_tick = 0
    
    # Create a list of all measure boundaries
    for mh in song.measureHeaders:
        measure_boundaries.append(current_tick)
        current_tick += mh.length
    
    # Add the end of the song
    measure_boundaries.append(current_tick)
    
    # Organize lights by instrument for easier processing
    lights_by_instrument = {
        "Guitar": [],
        "Bass": [],
        "Drums": []
    }
    
    for lc in light_changes:
        if lc.Track_Name in lights_by_instrument:
            lights_by_instrument[lc.Track_Name].append(lc)
    
    # Process each instrument's lights
    processed_lights = []
    
    # Remove overlapping lights (keep the more recent one)
    for instrument, lights in lights_by_instrument.items():
        # Sort by start time
        lights.sort(key=lambda lc: lc.Tick_Start)
        
        filtered_lights = []
        
        for i, light in enumerate(lights):
            # Check if this light overlaps with the next one
            if i < len(lights) - 1:
                next_light = lights[i + 1]
                current_end = light.Tick_Start + light.Tick_Duration
                
                if current_end > next_light.Tick_Start:
                    # Overlap detected, shorten the current light
                    new_duration = next_light.Tick_Start - light.Tick_Start
                    
                    # Only keep if duration is still meaningful
                    if new_duration > QUARTER_NOTE_TICKS / 16:
                        light.Tick_Duration = new_duration
                        filtered_lights.append(light)
                else:
                    filtered_lights.append(light)
            else:
                # Last light, no need to check for overlap
                filtered_lights.append(light)
        
        # Update the instrument lights
        lights_by_instrument[instrument] = filtered_lights
        
    # Create transitions at measure boundaries for visual coherence
    for boundary in measure_boundaries:
        for instrument, lights in lights_by_instrument.items():
            # Find lights near the boundary
            before_boundary = [l for l in lights if l.Tick_Start < boundary and 
                              l.Tick_Start + l.Tick_Duration > boundary - QUARTER_NOTE_TICKS/2]
            
            after_boundary = [l for l in lights if l.Tick_Start > boundary and 
                             l.Tick_Start < boundary + QUARTER_NOTE_TICKS/2]
            
            # If there are lights on both sides of the boundary, create a blend
            if before_boundary and after_boundary:
                # Get the last light before and first light after
                last_before = max(before_boundary, key=lambda l: l.Tick_Start)
                first_after = min(after_boundary, key=lambda l: l.Tick_Start)
                
                # Create a transition light that blends colors
                before_color = last_before.Color
                after_color = first_after.Color
                
                blend_color = (
                    (before_color[0] + after_color[0]) // 2,
                    (before_color[1] + after_color[1]) // 2,
                    (before_color[2] + after_color[2]) // 2
                )
                
                # Short transition light right at the boundary
                transition_duration = QUARTER_NOTE_TICKS // 8
                transition_start = boundary - transition_duration // 2
                
                transition_light = Light_Change(
                    instrument,
                    transition_start,
                    transition_duration,
                    blend_color[0],
                    blend_color[1],
                    blend_color[2]
                )
                
                lights_by_instrument[instrument].append(transition_light)
    
    # Balance overall intensity between instruments
    # Calculate average intensity for each instrument
    avg_intensity = {}
    
    for instrument, lights in lights_by_instrument.items():
        if not lights:
            continue
            
        total_intensity = 0
        for light in lights:
            # Approximate intensity as sum of RGB
            intensity = sum(light.Color)
            total_intensity += intensity
        
        avg_intensity[instrument] = total_intensity / len(lights)
    
    # Balance by adjusting outliers
    if avg_intensity:
        overall_avg = sum(avg_intensity.values()) / len(avg_intensity)
        
        for instrument, lights in lights_by_instrument.items():
            if not lights or instrument not in avg_intensity:
                continue
                
            # If this instrument is significantly brighter or dimmer than average
            intensity_ratio = overall_avg / max(1, avg_intensity[instrument])
            
            # Only adjust if the difference is significant
            if intensity_ratio < 0.7 or intensity_ratio > 1.3:
                adjustment = min(1.2, max(0.8, intensity_ratio))
                
                for light in lights:
                    # Adjust color intensity
                    r, g, b = light.Color
                    new_r = min(255, int(r * adjustment))
                    new_g = min(255, int(g * adjustment))
                    new_b = min(255, int(b * adjustment))
                    light.Color = (new_r, new_g, new_b)
    
    # Combine all processed lights
    for instrument, lights in lights_by_instrument.items():
        processed_lights.extend(lights)
    
    # Re-sort by start time
    processed_lights.sort(key=lambda lc: lc.Tick_Start)
    
    return processed_lights
    
    
    
def Create_Accent_Lights(light_changes: list[Light_Change], rhythm_info: dict, 
                      measure_start_tick: int, section_intensity: float):
    """
    Creates unified accent lights for highly syncopated, dense sections where multiple
    instruments interact with complex rhythm patterns.
    """
    # Only process if we have beat positions to work with
    if not rhythm_info["beat_positions"]:
        return
    
    # For highly syncopated sections, we want to emphasize the off-beat accents
    syncopation_level = rhythm_info["syncopation_level"]
    
    # Calculate measure length in ticks (assuming 4/4 time for simplicity)
    measure_length_ticks = QUARTER_NOTE_TICKS * 4
    
    # Define colors for accents based on syncopation level
    if syncopation_level > 0.7:
        # Highly syncopated - intense contrasting colors
        accent_color = (230, 70, 180)  # Bright magenta
    elif syncopation_level > 0.5:
        # Moderately syncopated - warm accent
        accent_color = (230, 160, 50)  # Orange-gold
    else:
        # Less syncopated - cooler accent
        accent_color = (70, 180, 210)  # Cyan-blue
    
    # Sort beat positions
    sorted_positions = sorted(rhythm_info["beat_positions"])
    
    # Identify important accents in the pattern
    accent_positions = []
    
    for pos in sorted_positions:
        # Check if this position is a syncopated beat
        is_syncopated = True
        
        # Check if beat falls on common divisions (quarter notes, eighth notes)
        for division in [QUARTER_NOTE_TICKS, QUARTER_NOTE_TICKS / 2]:
            if abs(pos % division) < QUARTER_NOTE_TICKS / 16 or abs(pos % division - division) < QUARTER_NOTE_TICKS / 16:
                is_syncopated = False
                break
        
        # If it's syncopated, and not too close to another accent, add it
        if is_syncopated:
            # Check if it's too close to an existing accent
            too_close = False
            for existing_pos in accent_positions:
                if abs(existing_pos - pos) < QUARTER_NOTE_TICKS / 4:
                    too_close = True
                    break
            
            if not too_close:
                accent_positions.append(pos)
    
    # Create accent lights
    for pos in accent_positions:
        # Adjust accent intensity based on position in measure
        # (accents in the middle of the measure often feel more impactful)
        relative_pos = (pos % measure_length_ticks) / measure_length_ticks
        position_factor = 0.8 + (0.4 * (1 - abs(relative_pos - 0.5) * 2))
        
        # Apply section intensity and position factor
        intensity = section_intensity * position_factor
        
        # Scale color by intensity
        r = min(255, int(accent_color[0] * intensity))
        g = min(255, int(accent_color[1] * intensity))
        b = min(255, int(accent_color[2] * intensity))
        
        # Shorter duration for accents to create punchy effect
        accent_duration = QUARTER_NOTE_TICKS / 6
        
        # Create unified accent lights for all three tracks with slightly 
        # different timings to create a cascading effect
        
        # Guitar accent (slightly early)
        light_changes.append(Light_Change(
            "Guitar",
            measure_start_tick + pos - QUARTER_NOTE_TICKS / 32,
            accent_duration,
            r, g, b
        ))
        
        # Bass accent (on the beat)
        light_changes.append(Light_Change(
            "Bass",
            measure_start_tick + pos,
            accent_duration,
            r, g, b
        ))
        
        # Drums accent (slightly late)
        light_changes.append(Light_Change(
            "Drums",
            measure_start_tick + pos + QUARTER_NOTE_TICKS / 32,
            accent_duration,
            r, g, b
        ))
        
        
        
def Create_Rhythm_Section_Lights(light_changes: list[Light_Change], 
                             bass_measure: GP.Measure, drums_measure: GP.Measure,
                             measure_start_tick: int, section_type: str, section_intensity: float):
    """
    Creates coordinated lights for rhythm section interactions between bass and drums.
    These are moments where bass and drums lock in together for a tight groove.
    """
    # Extract all bass notes and drum hits
    bass_positions = set()
    drum_positions = set()
    
    # Get bass positions
    for voice in bass_measure.voices:
        for beat in voice.beats:
            if beat.notes:
                bass_positions.add(beat.start)
    
    # Get drum positions
    for voice in drums_measure.voices:
        for beat in voice.beats:
            if beat.notes:
                drum_positions.add(beat.start)
    
    # Find synchronized points where bass and drums hit together
    sync_points = bass_positions.intersection(drum_positions)
    
    # Group sync points that are close together
    grouped_sync_points = []
    current_group = []
    
    for pos in sorted(sync_points):
        if not current_group or pos - current_group[-1] < QUARTER_NOTE_TICKS / 4:
            current_group.append(pos)
        else:
            # Average the group into a single sync point
            if current_group:
                grouped_sync_points.append(sum(current_group) / len(current_group))
            current_group = [pos]
    
    # Add last group
    if current_group:
        grouped_sync_points.append(sum(current_group) / len(current_group))
    
    # Only proceed if we have significant synchronization
    if len(grouped_sync_points) < 2:
        return
    
    # Analyze the rhythmic pattern of sync points to identify the groove
    time_intervals = []
    for i in range(len(grouped_sync_points) - 1):
        interval = grouped_sync_points[i + 1] - grouped_sync_points[i]
        time_intervals.append(interval)
    
    # Determine if the pattern suggests a particular groove type
    avg_interval = sum(time_intervals) / max(1, len(time_intervals))
    interval_variation = max(1, max(time_intervals) / min(time_intervals)) if time_intervals else 1
    
    # Set color and effects based on groove type
    if avg_interval < QUARTER_NOTE_TICKS / 2:
        # Fast, driving rhythm
        groove_type = "driving"
        base_color = (180, 60, 80)  # Reddish
        pulse_duration = QUARTER_NOTE_TICKS / 4
    elif interval_variation > 1.5:
        # Varied, funky rhythm
        groove_type = "funky"
        base_color = (80, 180, 120)  # Greenish
        pulse_duration = QUARTER_NOTE_TICKS / 3
    else:
        # Steady, solid rhythm
        groove_type = "steady"
        base_color = (80, 100, 180)  # Bluish
        pulse_duration = QUARTER_NOTE_TICKS / 2
    
    # Adjust color based on section type
    if section_type == "chorus":
        # Brighter for chorus
        base_color = tuple(min(255, int(c * 1.2)) for c in base_color)
    elif section_type == "breakdown":
        # More intense for breakdown
        base_color = (min(255, int(base_color[0] * 1.4)), 
                      min(255, int(base_color[1] * 0.9)), 
                      min(255, int(base_color[2] * 0.8)))
    
    # Create rhythm section pulse effects at each sync point
    for i, pos in enumerate(grouped_sync_points):
        # Vary intensity slightly based on position in the pattern
        cycle_position = (i % 4) / 4.0  # Assuming common 4-beat patterns
        intensity_variation = 0.85 + 0.3 * cycle_position
        
        # Apply section intensity and variation
        intensity = section_intensity * intensity_variation
        
        # Calculate color
        r = min(255, int(base_color[0] * intensity))
        g = min(255, int(base_color[1] * intensity))
        b = min(255, int(base_color[2] * intensity))
        
        # Create coordinated bass and drum lights that pulse together
        
        # Bass pulse
        light_changes.append(Light_Change(
            "Bass",
            int(measure_start_tick + pos),
            int(pulse_duration),
            r, g, b
        ))
        
        # Drum pulse (slightly shorter for percussive feel)
        light_changes.append(Light_Change(
            "Drums",
            int(measure_start_tick + pos),
            int(pulse_duration * 0.8),
            r, g, b
        ))
        
        # For driving or funky grooves, add a second pulse to create a bouncing effect
        if groove_type in ["driving", "funky"] and pulse_duration > QUARTER_NOTE_TICKS / 8:
            second_pulse_delay = pulse_duration * 1.2
            second_pulse_duration = pulse_duration * 0.7
            
            # Softer color for the second pulse
            soft_r = int(r * 0.7)
            soft_g = int(g * 0.7)
            soft_b = int(b * 0.7)
            
            # Add second pulse if there's enough space before the next sync point
            if i == len(grouped_sync_points) - 1 or (grouped_sync_points[i + 1] - pos) > second_pulse_delay + second_pulse_duration:
                # Bass second pulse
                light_changes.append(Light_Change(
                    "Bass",
                    int(measure_start_tick + pos + second_pulse_delay),
                    int(second_pulse_duration),
                    soft_r, soft_g, soft_b
                ))
                
                # Drum second pulse
                light_changes.append(Light_Change(
                    "Drums",
                    int(measure_start_tick + pos + second_pulse_delay),
                    int(second_pulse_duration * 0.8),
                    soft_r, soft_g, soft_b
                ))
                
                
                
def Enhance_Dominant_Instrument(light_changes: list[Light_Change], dominant_instrument: str,
                           measure_start_tick: int, section_intensity: float):
    """
    Enhances lighting for the dominant instrument in solos or sections where one instrument
    stands out. This function identifies existing lights for the dominant instrument and
    creates additional enhancement effects.
    """
    if not dominant_instrument:
        return
    
    # Find all existing lights for the dominant instrument in this measure
    dominant_lights = [lc for lc in light_changes 
                      if lc.Track_Name == dominant_instrument 
                      and lc.Tick_Start >= measure_start_tick
                      and lc.Tick_Start < measure_start_tick + (QUARTER_NOTE_TICKS * 4)]  # Assuming 4/4 time
    
    if not dominant_lights:
        return
    
    # Sort by start time
    dominant_lights.sort(key=lambda lc: lc.Tick_Start)
    
    # Calculate the average color of the existing lights
    total_r, total_g, total_b = 0, 0, 0
    for light in dominant_lights:
        total_r += light.Color[0]
        total_g += light.Color[1]
        total_b += light.Color[2]
    
    avg_r = total_r / len(dominant_lights)
    avg_g = total_g / len(dominant_lights)
    avg_b = total_b / len(dominant_lights)
    
    # Calculate a complementary color for enhancement effects
    # Simple complementary: invert the color
    comp_r = 255 - avg_r
    comp_g = 255 - avg_g
    comp_b = 255 - avg_b
    
    # Adjust the complementary color to be more vibrant
    # Increase the strongest component, decrease the weakest
    max_comp = max(comp_r, comp_g, comp_b)
    min_comp = min(comp_r, comp_g, comp_b)
    
    if comp_r == max_comp:
        comp_r = min(255, comp_r * 1.2)
    elif comp_r == min_comp:
        comp_r *= 0.8
        
    if comp_g == max_comp:
        comp_g = min(255, comp_g * 1.2)
    elif comp_g == min_comp:
        comp_g *= 0.8
        
    if comp_b == max_comp:
        comp_b = min(255, comp_b * 1.2)
    elif comp_b == min_comp:
        comp_b *= 0.8
    
    # Apply enhancement techniques based on the instrument type
    if dominant_instrument == "Guitar":
        # For guitar solo: add a subtle echo/tail after significant notes
        for i, light in enumerate(dominant_lights):
            # Skip very short notes
            if light.Tick_Duration < QUARTER_NOTE_TICKS / 8:
                continue
                
            # Check if this is a significant note (longer duration or higher intensity)
            is_significant = (light.Tick_Duration > QUARTER_NOTE_TICKS / 2 or
                             sum(light.Color) > (avg_r + avg_g + avg_b) * 1.2)
            
            if is_significant:
                # Calculate echo parameters
                echo_start = light.Tick_Start + light.Tick_Duration
                echo_duration = light.Tick_Duration * 0.7
                
                # Check if there's enough space before the next note
                if i < len(dominant_lights) - 1:
                    next_start = dominant_lights[i + 1].Tick_Start
                    if echo_start + echo_duration > next_start:
                        echo_duration = max(0, next_start - echo_start - QUARTER_NOTE_TICKS / 16)
                
                # Only add echo if duration is meaningful
                if echo_duration > QUARTER_NOTE_TICKS / 16:
                    # Echo uses a blend of the original color and complementary color, but dimmer
                    echo_r = int((light.Color[0] * 0.4 + comp_r * 0.2) * section_intensity)
                    echo_g = int((light.Color[1] * 0.4 + comp_g * 0.2) * section_intensity)
                    echo_b = int((light.Color[2] * 0.4 + comp_b * 0.2) * section_intensity)
                    
                    # Add the echo light
                    light_changes.append(Light_Change(
                        "Guitar",
                        int(echo_start),
                        int(echo_duration),
                        echo_r, echo_g, echo_b
                    ))
                    
        # For guitar solos, add accent lights for technique emphasis
        for i, light in enumerate(dominant_lights):
            # Check for rapidly changing notes (runs/licks)
            is_fast_run = False
            if i < len(dominant_lights) - 2:
                this_start = light.Tick_Start
                next_start = dominant_lights[i + 1].Tick_Start
                next_next_start = dominant_lights[i + 2].Tick_Start
                
                # If three notes are close together, it's likely a run
                if (next_start - this_start < QUARTER_NOTE_TICKS / 4 and
                    next_next_start - next_start < QUARTER_NOTE_TICKS / 4):
                    is_fast_run = True
            
            if is_fast_run:
                # For fast runs, add a complementary accent on the first note
                accent_r = int(comp_r * section_intensity * 1.3)
                accent_g = int(comp_g * section_intensity * 1.3)
                accent_b = int(comp_b * section_intensity * 1.3)
                
                # Short, bright accent
                light_changes.append(Light_Change(
                    "Guitar",
                    light.Tick_Start,
                    QUARTER_NOTE_TICKS / 16,
                    accent_r, accent_g, accent_b
                ))
                
    elif dominant_instrument == "Bass":
        # For bass-focused sections, enhance the groove with pulsing effects
        
        # Find the most common time interval between bass notes to identify the groove
        intervals = []
        for i in range(len(dominant_lights) - 1):
            interval = dominant_lights[i + 1].Tick_Start - dominant_lights[i].Tick_Start
            if interval > 0:
                intervals.append(interval)
        
        if not intervals:
            return
            
        # Find the most common interval (approximate)
        from collections import Counter
        rounded_intervals = [round(i / (QUARTER_NOTE_TICKS / 8)) * (QUARTER_NOTE_TICKS / 8) for i in intervals]
        most_common_interval = Counter(rounded_intervals).most_common(1)[0][0]
        
        # Add subtle pulses on off-beats if we have a regular groove
        if QUARTER_NOTE_TICKS / 4 < most_common_interval < QUARTER_NOTE_TICKS:
            # Calculate typical off-beat positions
            for light in dominant_lights:
                # Calculate the next off-beat position
                off_beat_pos = light.Tick_Start + most_common_interval / 2
                
                # Check if there's already a note at this position
                has_existing_note = any(abs(l.Tick_Start - off_beat_pos) < QUARTER_NOTE_TICKS / 16 
                                      for l in dominant_lights)
                
                if not has_existing_note:
                    # Dimmer, complementary color for the off-beat pulse
                    pulse_r = int(comp_r * section_intensity * 0.5)
                    pulse_g = int(comp_g * section_intensity * 0.5)
                    pulse_b = int(comp_b * section_intensity * 0.5)
                    
                    # Add the off-beat pulse
                    light_changes.append(Light_Change(
                        "Bass",
                        int(off_beat_pos),
                        int(QUARTER_NOTE_TICKS / 8),
                        pulse_r, pulse_g, pulse_b
                    ))
        
    elif dominant_instrument == "Drums":
        # For drum-focused sections (e.g., drum solos, drum breaks),
        # enhance with coordinated accent lights
        
        # Find the most intense drum hits
        intensity_threshold = sum([(l.Color[0] + l.Color[1] + l.Color[2]) / 3 for l in dominant_lights]) / len(dominant_lights) * 1.2
        
        intense_hits = [l for l in dominant_lights 
                       if (l.Color[0] + l.Color[1] + l.Color[2]) / 3 > intensity_threshold]
        
        # Add accent lights for these hits that affect all instruments
        for hit in intense_hits:
            # Use a complementary color for the accent
            accent_r = int(comp_r * section_intensity * 0.7)
            accent_g = int(comp_g * section_intensity * 0.7)
            accent_b = int(comp_b * section_intensity * 0.7)
            
            # Add synchronized accents across all instruments
            for instrument in ["Guitar", "Bass"]:
                light_changes.append(Light_Change(
                    instrument,
                    hit.Tick_Start,
                    min(hit.Tick_Duration, QUARTER_NOTE_TICKS / 8),
                    accent_r, accent_g, accent_b
                ))
  
def Analyze_Bass_Groove(measure: GP.Measure) -> dict:
    """
    Analyzes a bass measure to extract groove information and identify important beats.
    Returns a dictionary with detailed groove analysis.
    """
    groove_info = {
        "positions": [],  # All note positions sorted
        "durations": [],  # Duration of each note
        "intensities": [],  # Estimated intensity of each note
        "important_beats": [],  # Positions of structurally important beats
        "groove_type": "default",  # Walking, riff, pedal, etc.
        "subdivision": QUARTER_NOTE_TICKS / 4,  # Dominant subdivision (default: 16th notes)
        "syncopation_level": 0.0  # 0.0 - 1.0
    }
    
    # Extract all note positions and durations
    for voice in measure.voices:
        for beat in voice.beats:
            if not beat.notes:
                continue
                
            # Add position
            groove_info["positions"].append(beat.start)
            
            # Add duration
            groove_info["durations"].append(beat.duration.time)
            
            # Calculate approximate intensity based on note velocity and effect
            avg_velocity = sum(note.velocity for note in beat.notes) / len(beat.notes)
            
            # Check for special techniques that affect intensity
            intensity_factor = 1.0
            
            intensity = (avg_velocity / 127.0) * intensity_factor
            groove_info["intensities"].append(intensity)
    
    # Sort everything by position
    if groove_info["positions"]:
        positions = groove_info["positions"]
        durations = groove_info["durations"]
        intensities = groove_info["intensities"]
        
        # Sort all lists together based on positions
        sorted_data = sorted(zip(positions, durations, intensities), key=lambda x: x[0])
        
        # Unpack sorted data
        groove_info["positions"], groove_info["durations"], groove_info["intensities"] = zip(*sorted_data)
    
    # Determine the groove type based on pattern analysis
    if len(groove_info["positions"]) >= 4:
        # Calculate time intervals between notes
        intervals = []
        for i in range(len(groove_info["positions"]) - 1):
            interval = groove_info["positions"][i + 1] - groove_info["positions"][i]
            intervals.append(interval)
        
        # Check interval consistency
        if intervals:
            avg_interval = sum(intervals) / len(intervals)
            max_interval = max(intervals)
            min_interval = min(intervals)
            interval_variation = max_interval / min_interval if min_interval > 0 else 1
            
            # Detect common groove patterns
            if interval_variation < 1.2:
                # Very consistent intervals
                if len(groove_info["positions"]) == 4 and abs(avg_interval - QUARTER_NOTE_TICKS) < QUARTER_NOTE_TICKS / 16:
                    groove_info["groove_type"] = "four_on_floor"  # Quarter note walk
                    groove_info["subdivision"] = QUARTER_NOTE_TICKS
                elif abs(avg_interval - QUARTER_NOTE_TICKS / 2) < QUARTER_NOTE_TICKS / 16:
                    groove_info["groove_type"] = "eighth_note_walk"  # Eighth note walk
                    groove_info["subdivision"] = QUARTER_NOTE_TICKS / 2
                elif abs(avg_interval - QUARTER_NOTE_TICKS / 4) < QUARTER_NOTE_TICKS / 16:
                    groove_info["groove_type"] = "sixteenth_note_drive"  # Sixteenth note drive
                    groove_info["subdivision"] = QUARTER_NOTE_TICKS / 4
            elif interval_variation < 2.0:
                # Moderate variation - likely a riff or pattern
                groove_info["groove_type"] = "riff"
                # Estimate subdivision from the shortest common interval
                groove_info["subdivision"] = min(intervals)
            else:
                # High variation - likely a funky or syncopated line
                groove_info["groove_type"] = "syncopated"
                # Use eighth notes as default subdivision for syncopated grooves
                groove_info["subdivision"] = QUARTER_NOTE_TICKS / 2
                
        # Detect pedal tones (repeated notes)
        if len(groove_info["positions"]) > 2:
            note_values = []
            for beat in measure.voices[0].beats:
                if beat.notes:
                    note_values.append(min(note.value for note in beat.notes))
            
            if note_values:
                most_common_note = max(set(note_values), key=note_values.count)
                pedal_count = sum(1 for v in note_values if v == most_common_note)
                
                if pedal_count / len(note_values) > 0.6:  # 60% or more notes are the same
                    groove_info["groove_type"] = "pedal_tone"
    
    # Calculate syncopation level
    syncopated_count = 0
    total_notes = len(groove_info["positions"])
    
    if total_notes > 0:
        # Define common beat divisions for a measure (assuming 4/4 time)
        common_divisions = []
        for quarter in range(4):
            common_divisions.append(quarter * QUARTER_NOTE_TICKS)  # Quarter notes
            common_divisions.append(quarter * QUARTER_NOTE_TICKS + QUARTER_NOTE_TICKS / 2)  # Eighth notes
        
        # Count notes that don't fall on common divisions
        for pos in groove_info["positions"]:
            is_syncopated = True
            for div in common_divisions:
                if abs(pos - div) < QUARTER_NOTE_TICKS / 16:  # Allow small timing deviations
                    is_syncopated = False
                    break
            
            if is_syncopated:
                syncopated_count += 1
        
        groove_info["syncopation_level"] = syncopated_count / total_notes
    
    # Identify important beats in the groove
    if groove_info["positions"]:
        # Important beats are typically:
        # 1. First note in the measure
        # 2. Notes that are significantly louder than surrounding notes
        # 3. Notes that start on strong beats (1 and 3 in 4/4 time)
        # 4. Longest notes
        
        # First note is important
        groove_info["important_beats"].append(groove_info["positions"][0])
        
        # Check for notes on strong beats
        strong_beats = [0, QUARTER_NOTE_TICKS * 2]  # Beats 1 and 3
        for pos in groove_info["positions"]:
            for beat in strong_beats:
                if abs(pos - beat) < QUARTER_NOTE_TICKS / 8:
                    groove_info["important_beats"].append(pos)
        
        # Check for intensity spikes
        if len(groove_info["intensities"]) > 2:
            avg_intensity = sum(groove_info["intensities"]) / len(groove_info["intensities"])
            
            for i, intensity in enumerate(groove_info["intensities"]):
                if intensity > avg_intensity * 1.3:  # Significantly more intense
                    groove_info["important_beats"].append(groove_info["positions"][i])
        
        # Check for longer notes
        if groove_info["durations"]:
            avg_duration = sum(groove_info["durations"]) / len(groove_info["durations"])
            
            for i, duration in enumerate(groove_info["durations"]):
                if duration > avg_duration * 1.5:  # Significantly longer
                    groove_info["important_beats"].append(groove_info["positions"][i])
        
        # Remove duplicates and sort
        groove_info["important_beats"] = sorted(set(groove_info["important_beats"]))
    
    return groove_info


def Check_Bass_Drive(bass_drive_measure: GP.Measure, position: int) -> float:
    """
    Checks if there is a bass drive effect active at the given position.
    Returns the intensity of the bass drive effect (0.0 - 1.0).
    
    Bass drive typically refers to a more aggressive/overdriven bass sound,
    which is often represented in GP5 files as a separate track.
    """
    if bass_drive_measure is None:
        return 0.0
    
    drive_intensity = 0.0
    
    for voice in bass_drive_measure.voices:
        for beat in voice.beats:
            # Check if this beat overlaps with the position we're checking
            beat_start = beat.start
            beat_end = beat_start + beat.duration.time
            
            # If position falls within this beat
            if beat_start <= position < beat_end:
                if not beat.notes:
                    continue
                
                # Calculate intensity based on note velocities and position within the beat
                note_velocities = [note.velocity for note in beat.notes]
                avg_velocity = sum(note_velocities) / len(note_velocities) if note_velocities else 0
                
                # Normalize velocity to 0.0 - 1.0 range
                normalized_velocity = avg_velocity / 127.0
                
                # Calculate position within the beat (0.0 = start, 1.0 = end)
                if beat_end > beat_start:
                    relative_position = (position - beat_start) / (beat_end - beat_start)
                else:
                    relative_position = 0.0
                
                # Drive intensity is usually highest at the beginning of a note and fades
                position_factor = 1.0 - (relative_position * 0.5)
                
                # Calculate overall intensity
                intensity = normalized_velocity * position_factor
                
                # Check for additional effects that might indicate more drive
                for note in beat.notes:
                    if hasattr(note.effect, 'distortion') and note.effect.distortion:
                        intensity *= 1.3  # Increase intensity for distorted notes
                    if hasattr(note.effect, 'palmMute') and note.effect.palmMute:
                        intensity *= 1.2  # Palm muting often has a more driven sound
                
                # Return the highest intensity we find
                drive_intensity = max(drive_intensity, intensity)
    
    return drive_intensity


def Get_Beat_Importance(position: int, groove_pattern: dict) -> float:
    """
    Determines the rhythmic importance of a note at the given position within the groove pattern.
    Returns a value from 0.0 (unimportant) to 1.0 (very important).
    
    This helps emphasize notes that are structurally significant in the bassline.
    """
    # Default importance
    importance = 0.5
    
    # If no groove pattern or positions, return default
    if not groove_pattern or "positions" not in groove_pattern or not groove_pattern["positions"]:
        return importance
    
    # Check if this is an important beat as identified by groove analysis
    if "important_beats" in groove_pattern and groove_pattern["important_beats"]:
        for important_pos in groove_pattern["important_beats"]:
            # Check if our position is very close to an important beat
            if abs(position - important_pos) < QUARTER_NOTE_TICKS / 16:
                importance = 0.9  # High importance
                break
    
    # Check position relative to measure structure (assuming 4/4 time for simplicity)
    beat_in_measure = (position / QUARTER_NOTE_TICKS) % 4
    
    # Beats 1 and 3 are stronger in 4/4 time
    if beat_in_measure < 0.25:  # Beat 1
        importance = max(importance, 1.0)  # Highest importance
    elif beat_in_measure > 2.0 and beat_in_measure < 2.25:  # Beat 3
        importance = max(importance, 0.8)  # High importance
    elif beat_in_measure > 1.0 and beat_in_measure < 1.25:  # Beat 2
        importance = max(importance, 0.6)  # Medium importance
    elif beat_in_measure > 3.0 and beat_in_measure < 3.25:  # Beat 4
        importance = max(importance, 0.7)  # Medium-high importance (often leads back to beat 1)
    
    # Adjust based on groove type
    if "groove_type" in groove_pattern:
        groove_type = groove_pattern["groove_type"]
        
        if groove_type == "four_on_floor":
            # In four on the floor, all four quarter notes are relatively important
            for i in range(4):
                if abs(position - (i * QUARTER_NOTE_TICKS)) < QUARTER_NOTE_TICKS / 8:
                    importance = max(importance, 0.8)
        
        elif groove_type == "eighth_note_walk":
            # In walking bass, off-beats often connect between main beats
            is_off_beat = False
            for i in range(4):
                if abs(position - (i * QUARTER_NOTE_TICKS + QUARTER_NOTE_TICKS / 2)) < QUARTER_NOTE_TICKS / 8:
                    is_off_beat = True
                    break
            
            if is_off_beat:
                importance = max(importance, 0.4)  # Off-beats are less important in walking bass
        
        elif groove_type == "riff":
            # In riffs, the pattern of accents is often more important than the standard beat positions
            # Check if this note is at a position where notes tend to repeat across the measure
            
            # Look for repeating positions (e.g., if this position appears multiple times with similar offset)
            positions = groove_pattern["positions"]
            
            # Normalize position to the first beat
            position_in_beat = position % QUARTER_NOTE_TICKS
            
            # Count notes that fall at the same position in other beats
            similar_positions = 0
            for pos in positions:
                if abs((pos % QUARTER_NOTE_TICKS) - position_in_beat) < QUARTER_NOTE_TICKS / 16:
                    similar_positions += 1
            
            # If this position appears multiple times, it's likely part of a pattern/riff
            if similar_positions > 1:
                importance = max(importance, 0.75)
        
        elif groove_type == "syncopated" or groove_type == "pedal_tone":
            # In syncopated/funky bass, offbeat notes are often more important
            is_on_common_division = False
            
            # Check if note falls on common divisions
            for quarter in range(4):
                # Check quarter notes and eighth notes
                if (abs(position - quarter * QUARTER_NOTE_TICKS) < QUARTER_NOTE_TICKS / 16 or
                    abs(position - (quarter * QUARTER_NOTE_TICKS + QUARTER_NOTE_TICKS / 2)) < QUARTER_NOTE_TICKS / 16):
                    is_on_common_division = True
                    break
            
            # In syncopated bass, off-beat notes often carry more weight
            if not is_on_common_division:
                # Before giving importance to offbeat notes, check if this is truly a syncopated part
                if "syncopation_level" in groove_pattern and groove_pattern["syncopation_level"] > 0.3:
                    importance = max(importance, 0.85)  # High importance for syncopated notes
    
    # Check for notes that are emphasized by pattern, length, or intensity
    if "intensities" in groove_pattern and "positions" in groove_pattern:
        positions = groove_pattern["positions"]
        intensities = groove_pattern["intensities"]
        
        # Find our position in the groove pattern
        for i, pos in enumerate(positions):
            if abs(position - pos) < QUARTER_NOTE_TICKS / 16:
                # If this note has higher than average intensity
                if i < len(intensities):
                    avg_intensity = sum(intensities) / len(intensities) if intensities else 0.5
                    if intensities[i] > avg_intensity * 1.2:
                        importance = max(importance, 0.8)  # More important if it's emphasized
    
    # Check for first or last note in the pattern
    if "positions" in groove_pattern and groove_pattern["positions"]:
        first_pos = groove_pattern["positions"][0]
        last_pos = groove_pattern["positions"][-1]
        
        if abs(position - first_pos) < QUARTER_NOTE_TICKS / 16:
            importance = max(importance, 0.9)  # First note is very important
        elif abs(position - last_pos) < QUARTER_NOTE_TICKS / 16:
            importance = max(importance, 0.8)  # Last note is important (often leads to next measure)
    
    return importance    

#---------------------------------------------------------------------------------------
# Get Guitar Pro Track Object from Track Name
def GuitarPro_Get_Track_From_Name(track_name : str, guitar_pro_track_list : list[GP.Track]) -> GP.Track:
    for Track in guitar_pro_track_list:
        if Track.name == track_name:
            return Track
        
    return None

#---------------------------------------------------------------------------------------
# Get the Octave depending on the track name
def Get_Octave_From_GuitarPro_Track_Name(track_name : str) -> int:
    if "Guitar" in track_name:
        return 0 + abs(MIDI_LOWEST_OCTAVE)
    elif "Bass" in track_name:
        return 1 + abs(MIDI_LOWEST_OCTAVE)
    elif "Drum" in track_name:
        return 2 + abs(MIDI_LOWEST_OCTAVE)
    else:
        return MIDI_LOWEST_OCTAVE


#---------------------------------------------------------------------------------------
# Function to create an image of the Light Changes and visulize them
def Draw_Color_Color_Sequence(filename_wo_ext : str, song : GP.Song, light_changes : list[Light_Change]):
    global AI_NAME
    
    Initial_X_Offset_Px         = 100
    End_X_Offset_Px             = 50
    
    Instrument_Height_Px        = 50
    Instrument_Gap_Px           = 20
    Note_32nd_Width_Px          = 5
    Ticks_per_32nd              = 120
    
    Measure_Marker_Height_Px    = 45
    Measure_Text_Height_Px      = 30
    Measure_Bar_Height_Px       = 30
    Measure_Bar_Color           = "White"
    Marker_Light_Feature_Color  = "Red"

    Background_Color            = "Black"

    Measure_Offset = Measure_Marker_Height_Px + Measure_Text_Height_Px + Measure_Bar_Height_Px

    Image_Height_Px         = Measure_Offset+3*Instrument_Height_Px + 2* Instrument_Gap_Px
    Instrument_Offset_Px    = [Measure_Offset, Measure_Offset+1*(Instrument_Height_Px+Instrument_Gap_Px), Measure_Offset+2*(Instrument_Height_Px+Instrument_Gap_Px)]
    
    Count_32nd = 0
    for m in song.measureHeaders:
        Count_32nd += m.length / Ticks_per_32nd

    Image_Width_Px = Initial_X_Offset_Px + int(Count_32nd) * Note_32nd_Width_Px + End_X_Offset_Px
    Img = Image.new(mode="RGB", size=(Image_Width_Px, Image_Height_Px), color=Background_Color)
    Img_Draw = ImageDraw.Draw(Img)   

    Octaves = [0, 1, 2]
    Tracks = ["Guitar", "Bass", "Drums"]

    for Octave in Octaves:
        for LC in light_changes:
            if Get_Octave_From_GuitarPro_Track_Name(LC.Track_Name) == Octave + abs(MIDI_LOWEST_OCTAVE):
                Start_In_32nd       = int(LC.Tick_Start / Ticks_per_32nd)
                Duration_In_32nd    = int(LC.Tick_Duration / Ticks_per_32nd)

                Start_Pixel     = Start_In_32nd     * Note_32nd_Width_Px + Initial_X_Offset_Px
                Duration_Pixel  = Duration_In_32nd  * Note_32nd_Width_Px
                End_Pixel       = Start_Pixel + Duration_Pixel

                Img_Draw.rectangle([(Start_Pixel, Instrument_Offset_Px[Octave]), (End_Pixel , Instrument_Offset_Px[Octave] + Instrument_Height_Px)], fill = LC.Color) 

    Img_Font_Big    = ImageFont.truetype('consolab.ttf', 30)
    Img_Font_Small  = ImageFont.truetype('consolab.ttf', 15)

    Last_Feature_Text_Right = -1000

    for m in song.measureHeaders:
        Count_32nd = int(m.start / Ticks_per_32nd)
        Pixel_X = Count_32nd * Note_32nd_Width_Px + Initial_X_Offset_Px

        if m.marker is not None:
            Marker_Title = m.marker.title

            Marker_Title_Sections = Marker_Title.split('[')

            Marker_Info = Marker_Title_Sections[0].strip()
            Text_Width = Img_Draw.textlength(Marker_Info, Img_Font_Small)
            
            Img_Draw.text((Pixel_X-Text_Width/2, 0), str(Marker_Info), font=Img_Font_Small, fill=Measure_Bar_Color)

            if len(Marker_Title_Sections) > 1:
                Marker_Light_Feature = "[" + Marker_Title_Sections[1].strip()
                Marker_Light_Feature = Marker_Light_Feature.replace(";", " | ")

                Text_Width = Img_Draw.textlength(Marker_Light_Feature, Img_Font_Small)

                Y_Offset_Factor = 1
                if Pixel_X-Text_Width/2 < Last_Feature_Text_Right:
                    Y_Offset_Factor += 1

                Img_Draw.text((Pixel_X-Text_Width/2, 15*Y_Offset_Factor), str(Marker_Light_Feature), font=Img_Font_Small, fill=Marker_Light_Feature_Color)
                Last_Feature_Text_Right = Pixel_X + Text_Width/2

        Text_Left_Offset_Big = len(str(m.number))*8
        
        Img_Draw.text((Pixel_X-Text_Left_Offset_Big, Measure_Marker_Height_Px), str(m.number), font=Img_Font_Big, fill=Measure_Bar_Color)
        Img_Draw.line([(Pixel_X, Measure_Marker_Height_Px+Measure_Text_Height_Px), (Pixel_X , Image_Height_Px)], fill = Measure_Bar_Color) 


    for i in range(len(Octaves)):
        Img_Draw.text((5, Instrument_Offset_Px[i] + 12), Tracks[i], font=Img_Font_Big, fill=Measure_Bar_Color)

    Filename = filename_wo_ext + ".png"

    Img.save(Filename)
    Print_Message("Light Overview as PNG File '" + Filename + "' written", 1)
    # Img.show()





#---------------------------------------------------------------------------------------
# Function write the light configuration file being compatible with the GUI
def Write_Light_Configuration_File(filename_wo_ext : str, song : GP.Song, light_changes : list[Light_Change]):
    Filename = filename_wo_ext + ".light"
    
    File = open(Filename, "w+")
    File.write("MIDILightDrawer_BarEvents_v1.0\n")
    File.write(str(len(song.measureHeaders)) + "\n")
    
    for m in song.measureHeaders:
        File.write(str(m.timeSignature.numerator) + "," +str(m.timeSignature.denominator.value) + "\n")
        
    File.write(str(len(light_changes)) + "\n")
    
    for l in light_changes:
        File.write(str(l.Tick_Start - 960) + "," + str(l.Tick_Duration) + "," + str(Get_Octave_From_GuitarPro_Track_Name(l.Track_Name)) + ",")
        File.write(str(abs(l.Color[0])) + "," + str(abs(l.Color[1])) + "," + str(abs(l.Color[2])) + "," + str(l.Track_Name))
        File.write("\n")
    
    Print_Message("Light Configuration File '" + Filename + "' written", 1)
   
   
#---------------------------------------------------------------------------------------
# Get the Octave depending on the track name
def Get_Octave_From_GuitarPro_Track_Name(track_name : str) -> int:
    if "Guitar" in track_name:
        return 0 + abs(MIDI_LOWEST_OCTAVE)
    elif "Bass" in track_name:
        return 1 + abs(MIDI_LOWEST_OCTAVE)
    elif "Drum" in track_name:
        return 2 + abs(MIDI_LOWEST_OCTAVE)
    else:
        return MIDI_LOWEST_OCTAVE
     

#---------------------------------------------------------------------------------------
# Print Regular Console Output
def Print_Message(message : str, indent : int = 0, source : str = None):
    for i in range(indent * 2):
        print(" ", end='')

    print(message, end='')

    if source is not None:
        print(" (" + source + ")", end='')

    print()


#---------------------------------------------------------------------------------------
# Print Error Console Output
def Print_Error(message : str, indent : int = 0, source : str = None):
    Print_Message("ERROR: " + message, indent, source)

if __name__ == "__main__":
    main()
import os
import math
import inspect
import colorsys
import guitarpro as GP
from PIL import Image, ImageDraw, ImageFont


QUARTER_NOTE_TICKS = 960
MIDI_LOWEST_OCTAVE = -2
AI_NAME = "Claude37_1"  # Claude Sonnet 3.7


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

    Light_Changes : list[Light_Change] = []

    Current_Measure_Tick = 0
    
    for MH in Song.measureHeaders:

        # Get the measure for each instrument from each track
        Measure_Guitar      : GP.Measure = Tracks["Guitar"].measures[MH.number-1]
        Measure_Bass        : GP.Measure = Tracks["Bass"].measures[MH.number-1]
        Measure_Bass_Drive  : GP.Measure = Tracks["Bass Drive"].measures[MH.number-1]
        Measure_Drums       : GP.Measure = Tracks["Drums"].measures[MH.number-1]

        
        # Process each measure to create light changes
        Process_Guitar_Measure(Light_Changes, Measure_Guitar, Current_Measure_Tick)
        Process_Bass_Measure(Light_Changes, Measure_Bass, Measure_Bass_Drive, Current_Measure_Tick)
        Process_Drums_Measure(Light_Changes, Measure_Drums, Current_Measure_Tick)
        
        #Current_Measure_Tick += MH.length

    
    FileName_wo_Ext = song_name + "_" + AI_NAME

    Draw_Color_Color_Sequence(FileName_wo_Ext, Song, Light_Changes)
    Write_Light_Configuration_File(FileName_wo_Ext, Song, Light_Changes)
    print("")    

    return 0


# Process Guitar track to create light changes
def Process_Guitar_Measure(light_changes: list[Light_Change], measure: GP.Measure, measure_start_tick: int):
    for voice in measure.voices:
        for beat in voice.beats:
            if not beat.notes:  # Skip if no notes in the beat
                continue
                
            # Calculate pitch average and intensity for this beat
            notes_count = len(beat.notes)
            pitch_sum = sum(note.value for note in beat.notes)
            avg_pitch = pitch_sum / notes_count if notes_count > 0 else 0
            
            # Map pitch to color (higher pitch = more blue/green, lower pitch = more red)
            # Guitar note range is typically around 40 (low E) to 84 (high E)
            normalized_pitch = min(1.0, max(0.0, (avg_pitch - 40) / 44))
            
            # Get note intensity from note effect and velocity
            intensity = 1.0
            for note in beat.notes:
                if note.effect.palmMute:
                    intensity *= 0.7  # Palm mute gives a softer light
                if note.effect.vibrato:
                    intensity *= 1.2  # Vibrato gives a slightly brighter light
            
            # Base intensity on the number of notes (chords are brighter)
            intensity *= min(1.0, 0.5 + (notes_count * 0.1))
            
            # Calculate RGB values
            if normalized_pitch < 0.5:  # Lower pitches
                red = int(255 * intensity)
                green = int(255 * normalized_pitch * 2 * intensity)
                blue = int(100 * normalized_pitch * intensity)
            else:  # Higher pitches
                red = int(255 * (1 - normalized_pitch) * 2 * intensity)
                green = int(255 * intensity)
                blue = int(100 + 155 * (normalized_pitch - 0.5) * 2 * intensity)
            
            # Create light change
            start_tick = measure_start_tick + beat.start
            duration = beat.duration.time
            
            # Apply effects to duration
            if any(note.effect.hammer for note in beat.notes):
                duration = int(duration * 0.8)  # Shorter for hammer-ons
            
            light_changes.append(Light_Change(
                "Guitar", 
                start_tick, 
                duration, 
                red, green, blue
            ))

# Process Bass tracks to create light changes
def Process_Bass_Measure(light_changes: list[Light_Change], bass_measure: GP.Measure, 
                        bass_drive_measure: GP.Measure, measure_start_tick: int):
    # Process normal bass track
    for voice in bass_measure.voices:
        for beat in voice.beats:
            if not beat.notes:
                continue
                
            # Calculate intensity based on notes
            notes_count = len(beat.notes)
            lowest_note = min(note.value for note in beat.notes) if notes_count > 0 else 0
            
            # Bass is usually in lower range (around 28-52)
            normalized_pitch = min(1.0, max(0.0, (lowest_note - 28) / 24))
            
            # Base colors on pitch range (bass focuses more on red/green spectrum)
            red = int(150 + 105 * (1 - normalized_pitch))
            green = int(100 + 100 * normalized_pitch)
            blue = int(50 + 50 * normalized_pitch)
            
            # Check if there's a corresponding note in the bass drive track
            drive_intensity = Check_Bass_Drive(bass_drive_measure, beat.start)
            if drive_intensity > 0:
                # Bass drive increases red component and overall brightness
                red = min(255, int(red * 1.3))
                green = min(255, int(green * 1.1))
                blue = min(255, int(blue * 0.9))  # Reduce blue for warmer tone
            
            start_tick = measure_start_tick + beat.start
            duration = beat.duration.time
            
            light_changes.append(Light_Change(
                "Bass", 
                start_tick, 
                duration, 
                red, green, blue
            ))

# Helper function to check if bass drive is active at a given position
def Check_Bass_Drive(bass_drive_measure: GP.Measure, position: int) -> float:
    for voice in bass_drive_measure.voices:
        for beat in voice.beats:
            # Check if this beat overlaps with the position we're checking
            if beat.start <= position < (beat.start + beat.duration.time):
                if beat.notes:
                    # Return intensity based on note velocities
                    return sum(note.velocity / 127 for note in beat.notes) / len(beat.notes)
    return 0

# Process Drums track to create light changes
def Process_Drums_Measure(light_changes: list[Light_Change], measure: GP.Measure, measure_start_tick: int):
    # Dictionary to track active drum events
    active_drums = {}
    
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
            
            for note in beat.notes:
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
            
            # Determine colors based on drum combination
            red, green, blue = 0, 0, 0
            
            if has_kick:
                red += 200
                green += 50
                blue += 50
            
            if has_snare:
                red += 100
                green += 150
                blue += 100
            
            if has_hihat:
                red += 50
                green += 100
                blue += 150
            
            if has_crash:
                red += 200
                green += 200
                blue += 200  # White flash for crash
            
            if has_tom:
                red += 100
                green += 80
                blue += 120
            
            # Normalize colors to 0-255 range
            total = max(1, red + green + blue)
            intensity_factor = min(255 / max(red, green, blue), 1.0) if max(red, green, blue) > 0 else 1.0
            
            red = int(red * intensity_factor)
            green = int(green * intensity_factor)
            blue = int(blue * intensity_factor)
            
            # Create light change with shorter duration for drums (percussive)
            start_tick = measure_start_tick + beat.start
            
            # Different durations based on type of hit
            if has_crash:
                duration = QUARTER_NOTE_TICKS // 2  # Longer for crash
            else:
                duration = QUARTER_NOTE_TICKS // 4  # Shorter for most drum hits
            
            light_changes.append(Light_Change(
                "Drums", 
                start_tick, 
                duration, 
                red, green, blue
            ))



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
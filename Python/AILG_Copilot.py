import os
import math
import inspect
import colorsys
import guitarpro as GP
from PIL import Image, ImageDraw, ImageFont


QUARTER_NOTE_TICKS = 960
MIDI_LOWEST_OCTAVE = -2
AI_NAME = "Copilot"


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

    # Print_Message("--------------------------", 0)

    Light_Changes : list[Light_Change] = []

    Current_Measure_Tick = 0
    
    for MH in Song.measureHeaders:

        # Get the measure for each instrument from each track
        Measure_Guitar      : GP.Measure = Tracks["Guitar"].measures[MH.number-1]
        Measure_Bass        : GP.Measure = Tracks["Bass"].measures[MH.number-1]
        Measure_Bass_Drive  : GP.Measure = Tracks["Bass Drive"].measures[MH.number-1]
        Measure_Drums       : GP.Measure = Tracks["Drums"].measures[MH.number-1]

        
        # Add Alorithm 
        
        # Process each measure to generate Light_Change objects
        Process_Measure(Measure_Guitar, "Guitar", Light_Changes)
        Process_Measure(Measure_Bass, "Bass", Light_Changes)
        Process_Measure(Measure_Bass_Drive, "Bass Drive", Light_Changes)
        Process_Measure(Measure_Drums, "Drums", Light_Changes)

        Current_Measure_Tick += MH.length

    
    FileName_wo_Ext = song_name + "_" + AI_NAME

    Draw_Color_Color_Sequence(FileName_wo_Ext, Song, Light_Changes)
    Write_Light_Configuration_File(FileName_wo_Ext, Song, Light_Changes)
    print("")    

    return 0



def Process_Measure(measure: GP.Measure, track_name: str, light_changes: list[Light_Change]):
    for voice in measure.voices:
        for beat in voice.beats:
            for note in beat.notes:
                tick_start = beat.start  # Adjusted to use absolute `beat.start`
                tick_duration = QUARTER_NOTE_TICKS // beat.duration.value

                # Convert realValue to color, considering harmonic context and velocity
                red, green, blue = Pitch_To_Color(note.realValue, note.velocity, note)

                # Create Light_Change
                light_change = Light_Change(track_name, tick_start, tick_duration, red, green, blue)
                light_changes.append(light_change)

def Pitch_To_Color(realValue: int, velocity: int, note: GP.Note) -> tuple:
    """ Convert a musical pitch (MIDI note value) to an RGB color with gradient based on velocity and harmonic context. """
    hue = (realValue % 12) / 12.0
    saturation = 1.0

    # Adjust value based on velocity
    value = velocity / 127.0  # Normalize velocity to range 0-1

    # Harmonic context: Adjust hue slightly based on string/fret position
    harmonic_adjustment = (note.string * note.value) % 12 / 12.0
    hue = (hue + harmonic_adjustment) % 1.0

    red, green, blue = colorsys.hsv_to_rgb(hue, saturation, value)
    return int(red * 255), int(green * 255), int(blue * 255)


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
        File.write(str(l.Tick_Start - QUARTER_NOTE_TICKS) + "," + str(l.Tick_Duration) + "," + str(Get_Octave_From_GuitarPro_Track_Name(l.Track_Name)) + ",")
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
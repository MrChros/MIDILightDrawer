# MIDI Light Drawer
A simple GUI tool to draw light events based on Guitar Pro 5 tablatures. The drawn events can be exported as MIDI files and played back by common DAWs. The MIDI information is interpreted as light information (Red, Green and Blue Colors) by custom made LED strip controllers (see repository https://github.com/MrChros/MIDI_Lighter). 

The GUI is written in C++/CLI with the .NET 4.0 Framework.

This is a private project and it has no commerical use. Feel free te reuse any of the code or contact me in case of questions.

But shoutout to https://github.com/PhilPotter/gp_parser for the GP5 parser. I fixed some issues, but it is already very good!

## Organization

* The Source code is availablie under the subfolder "Source". There, a complete Visual Studio 2022 project is located. It should be possible to open the project in VS2022, compile and run the application. Make sure the according extension for C++/CLI and .NET 4.0 is installed in VS2022. Otherwise, no external libraries are required.
* The "Release" folder contains an actual release compile with all required dll-files right next to the exe-file. If you get an error starting the application, make sure you have the .NET4.0 runtime library installed on your computer. The application itself does need to be installed and can be executed right away.
* The Python folder contains some scripts to generate so-called .light-files based on Guitar Pro 5 Tabs. I asked several AIs to generate me some algorithim to translate measures, the contained beats and notes into light information. The template file can be used to feed other AIs. So far I have asked ChatGPT, Microsoft Copilot and Claude AI.
* Example Pictures of the program can be found in the Pictures folder

## Run the Python scripts

There is a batch-file stored in the same folder. The batch file simply calls one Python script after the other. Every Python script reads in all gp5-files in the same folder and outputs a light-file and a PNG picture as result overview. If this does not make any sense to you, feel free to get in contact. The light-files can be opened in the GUI application and the edited further.

#include "pch.h"
#include "Form_Main.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
int main(array < String^>^ args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(0);
    MIDILightDrawer::Form_Main form;
    Application::Run(% form);
}


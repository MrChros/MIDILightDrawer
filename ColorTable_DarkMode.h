#pragma once

//#include <Windows.h>
//#include <vcclr.h>

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class ColorTable_DarkMode : public ProfessionalColorTable
	{
	public:
		virtual property Color MenuItemSelected
		{
			Color get() override;
		}

		virtual property Color MenuItemBorder
		{
			Color get() override;
		}

		virtual property Color MenuItemPressedGradientBegin
		{
			Color get() override;
		}

		virtual property Color MenuItemPressedGradientEnd
		{
			Color get() override;
		}

		virtual property Color MenuItemSelectedGradientBegin
		{
			Color get() override;
		}

		virtual property Color MenuItemSelectedGradientEnd
		{
			Color get() override;
		}

		virtual property Color MenuStripGradientBegin
		{
			Color get() override;
		}

		virtual property Color MenuStripGradientEnd
		{
			Color get() override;
		}

		virtual property Color MenuBorder
		{
			Color get() override;
		}

		virtual property Color MenuItemSelectedGradientMiddle
		{
			Color get() override;
		}

		virtual property Color MenuItemPressedGradientMiddle
		{
			Color get() override;
		}

		virtual property Color ImageMarginGradientBegin
		{
			Color get() override;
		}

		virtual property Color ImageMarginGradientMiddle
		{
			Color get() override;
		}

		virtual property Color ImageMarginGradientEnd
		{
			Color get() override;
		}

		virtual property Color ImageMarginRevealedGradientBegin
		{
			Color get() override;
		}

		virtual property Color ImageMarginRevealedGradientMiddle
		{
			Color get() override;
		}

		virtual property Color ImageMarginRevealedGradientEnd
		{
			Color get() override;
		}
	};
}
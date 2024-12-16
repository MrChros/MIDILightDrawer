#include "ColorTable_DarkMode.h"

namespace MIDILightDrawer
{
	Color ColorTable_DarkMode::MenuItemSelected::get()
	{
		return Color::FromArgb(255, 50, 50, 50);
	}

	Color ColorTable_DarkMode::MenuItemBorder::get()
	{
		return Color::FromArgb(255, 60, 60, 60);
	}

	Color ColorTable_DarkMode::MenuItemPressedGradientBegin::get()
	{
		return Color::FromArgb(255, 40, 40, 40);
	}

	Color ColorTable_DarkMode::MenuItemPressedGradientEnd::get()
	{
		return Color::FromArgb(255, 40, 40, 40);
	}

	Color ColorTable_DarkMode::MenuItemSelectedGradientBegin::get()
	{
		return Color::FromArgb(255, 50, 50, 50);
	}

	Color ColorTable_DarkMode::MenuItemSelectedGradientEnd::get()
	{
		return Color::FromArgb(255, 50, 50, 50);
	}

	Color ColorTable_DarkMode::MenuStripGradientBegin::get()
	{
		return Color::FromArgb(255, 25, 25, 25);
	}

	Color ColorTable_DarkMode::MenuStripGradientEnd::get()
	{
		return Color::FromArgb(255, 25, 25, 25);
	}

	Color ColorTable_DarkMode::MenuBorder::get()
	{
		return Color::FromArgb(255, 60, 60, 60);
	}

	Color ColorTable_DarkMode::MenuItemSelectedGradientMiddle::get()
	{
		return Color::FromArgb(255, 50, 50, 50);
	}

	Color ColorTable_DarkMode::MenuItemPressedGradientMiddle::get()
	{
		return Color::FromArgb(255, 40, 40, 40);
	}

	Color ColorTable_DarkMode::ImageMarginGradientBegin::get()
	{
		return Color::FromArgb(255, 25, 25, 25);
	}

	Color ColorTable_DarkMode::ImageMarginGradientMiddle::get()
	{
		return Color::FromArgb(255, 25, 25, 25);
	}

	Color ColorTable_DarkMode::ImageMarginGradientEnd::get()
	{
		return Color::FromArgb(255, 25, 25, 25);
	}

	Color ColorTable_DarkMode::ImageMarginRevealedGradientBegin::get()
	{
		return Color::FromArgb(255, 40, 40, 40);
	}

	Color ColorTable_DarkMode::ImageMarginRevealedGradientMiddle::get()
	{
		return Color::FromArgb(255, 40, 40, 40);
	}

	Color ColorTable_DarkMode::ImageMarginRevealedGradientEnd::get()
	{
		return Color::FromArgb(255, 40, 40, 40);
	}
}
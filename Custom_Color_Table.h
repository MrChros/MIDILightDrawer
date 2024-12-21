#pragma once

using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace MIDILightDrawer {

	public ref class Custom_Color_Table : public ProfessionalColorTable {
	private:
		Color menuItemSelected;
		Color menuItemSelectedGradientBegin;
		Color menuItemSelectedGradientEnd;
		Color menuItemPressedGradientBegin;
		Color menuItemPressedGradientEnd;
		Color menuStripGradientBegin;
		Color menuStripGradientEnd;
		Color menuBorder;
		Color menuItemBorder;
		Color toolStripDropDownBackground;
		Color imageMarginGradientBegin;
		Color imageMarginGradientEnd;
		Color imageMarginGradientMiddle;

	public:
		Custom_Color_Table() {
			UseSystemColors = false;
		}

		property Color MenuItemSelected {
			virtual Color get() override { return menuItemSelected; }
		}
		void SetMenuItemSelected(Color value) { menuItemSelected = value; }

		property Color MenuItemSelectedGradientBegin {
			virtual Color get() override { return menuItemSelectedGradientBegin; }
		}
		void SetMenuItemSelectedGradientBegin(Color value) { menuItemSelectedGradientBegin = value; }

		property Color MenuItemSelectedGradientEnd {
			virtual Color get() override { return menuItemSelectedGradientEnd; }
		}
		void SetMenuItemSelectedGradientEnd(Color value) { menuItemSelectedGradientEnd = value; }

		property Color MenuItemPressedGradientBegin {
			virtual Color get() override { return menuItemPressedGradientBegin; }
		}
		void SetMenuItemPressedGradientBegin(Color value) { menuItemPressedGradientBegin = value; }

		property Color MenuItemPressedGradientEnd {
			virtual Color get() override { return menuItemPressedGradientEnd; }
		}
		void SetMenuItemPressedGradientEnd(Color value) { menuItemPressedGradientEnd = value; }

		property Color MenuStripGradientBegin {
			virtual Color get() override { return menuStripGradientBegin; }
		}
		void SetMenuStripGradientBegin(Color value) { menuStripGradientBegin = value; }

		property Color MenuStripGradientEnd {
			virtual Color get() override { return menuStripGradientEnd; }
		}
		void SetMenuStripGradientEnd(Color value) { menuStripGradientEnd = value; }

		property Color MenuBorder {
			virtual Color get() override { return menuBorder; }
		}
		void SetMenuBorder(Color value) { menuBorder = value; }

		property Color MenuItemBorder {
			virtual Color get() override { return menuItemBorder; }
		}
		void SetMenuItemBorder(Color value) { menuItemBorder = value; }

		property Color ToolStripDropDownBackground {
			virtual Color get() override { return toolStripDropDownBackground; }
		}
		void SetToolStripDropDownBackground(Color value) { toolStripDropDownBackground = value; }

		property Color ImageMarginGradientBegin {
			virtual Color get() override { return imageMarginGradientBegin; }
		}
		void SetImageMarginGradientBegin(Color value) { imageMarginGradientBegin = value; }

		property Color ImageMarginGradientEnd {
			virtual Color get() override { return imageMarginGradientEnd; }
		}
		void SetImageMarginGradientEnd(Color value) { imageMarginGradientEnd = value; }

		property Color ImageMarginGradientMiddle {
			virtual Color get() override { return imageMarginGradientMiddle; }
		}
		void SetImageMarginGradientMiddle(Color value) { imageMarginGradientMiddle = value; }
	};

}
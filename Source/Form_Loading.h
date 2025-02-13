#pragma once

#include "Theme_Manager.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	public ref class FormLoading : public System::Windows::Forms::Form
	{
	public:
		FormLoading();

		void AddStatus(System::String^ status);
		void OnLoadingStageChanged(LoadingStage stage);

	protected:
		~FormLoading();

	private:
		System::Windows::Forms::ListBox^ _StatusListBox;
		System::ComponentModel::Container^ components;

		void InitializeComponent(void);
		void ApplyTheme();
		void OnFormPaint(Object^ sender, PaintEventArgs^ e);
	};
}
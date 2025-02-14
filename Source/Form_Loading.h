#pragma once

#include "Theme_Manager.h"
#include "Control_ProgressBar.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	public ref class FormLoading : public System::Windows::Forms::Form
	{
	public:
		FormLoading();

		void AddStatus(System::String^ status);
		void OnLoadingStageChanged(LoadingStage stage);
		void UpdateProgress(float progress);

	protected:
		~FormLoading();

	private:
		System::Windows::Forms::ListBox^ _StatusListBox;
		Control_ProgressBar^ _ImagesProgressBar;
		Control_ProgressBar^ _TabTextProgressBar;
		Control_ProgressBar^ _DrumSymbolsProgressBar;
		Control_ProgressBar^ _DurationSymbolsProgressBar;
		array<Control_ProgressBar^>^ _ProgressBars;
		Control_ProgressBar^ _CurrentProgressBar;
		System::ComponentModel::Container^ _Components;

		void InitializeComponent(void);
		void InitializeProgressBar(Control_ProgressBar^ progressBar, int yPosition);
		void ApplyTheme();
		void OnFormPaint(Object^ sender, PaintEventArgs^ e);
	};
}
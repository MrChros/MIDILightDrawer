#pragma once

#include "Theme_Manager.h"
#include "Control_ProgressBar.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	public ref class FormLoading : public System::Windows::Forms::Form
	{
	private:
		static const int NUM_ENTRIES = 6;

	public:
		FormLoading();

		void AddStatus(System::String^ status);
		void OnLoadingStageChanged(LoadingStage stage);
		void UpdateProgress(float progress);

	protected:
		~FormLoading();

	private:
		System::ComponentModel::Container^ _Components;
		
		TableLayoutPanel^ _TableLayout_Form;
		array<Control_ProgressBar^>^ _ProgressBars;
		array<Label^>^ _Labels;

		int _CurrentIndex;

		Control_ProgressBar^ _CurrentProgressBar;

		void InitializeComponent(void);
		void InitializeProgressBar(Control_ProgressBar^ progressBar, int row);
		void InitializeLabel(Label^ label, int row);
		void ApplyTheme();
		void OnFormPaint(Object^ sender, PaintEventArgs^ e);
	};
}
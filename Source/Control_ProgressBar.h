#pragma once
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace MIDILightDrawer
{
    public ref class Control_ProgressBar : public ProgressBar
    {
        private:
            Color _ProgressColor;
            Color _BorderColor;
            bool _IsCompleted;

        public:
            Control_ProgressBar();

        protected:
            virtual void OnPaint(PaintEventArgs^ e) override;

        public:
            property Color ProgressColor
            {
                Color get();
                void set(Color color);
            }

            property Color BorderColor
            {
                Color get();
                void set(Color color);
            }

            property bool IsCompleted
            {
                bool get();
                void set(bool value);
            }
    };
}
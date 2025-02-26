#pragma once

//#pragma comment(lib, "d2d1.lib")
//#pragma comment(lib, "dwrite.lib")
//#pragma comment(lib, "windowscodecs.lib")
//#pragma comment(lib, "user32.lib")

#include <d2d1.h>
#include <dwrite.h>
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <functional>


enum class DrawCommandType {
    Line,
    Rectangle,
    FilledRectangle,
    DashedRectangle,
    Text,
    Bitmap,
    RoundedRectangle,
    FilledRoundedRectangle,
    Ellipse,
    FilledEllipse,
    Gradient,
    TieLine
};

// Base command structure
struct DrawCommand {
    DrawCommandType Type;
    float ZOrder;

    DrawCommand(DrawCommandType t, float z = 0.0f) : Type(t), ZOrder(z) {}
    virtual ~DrawCommand() {}
};

// Line command
struct LineCommand : public DrawCommand {
    D2D1_POINT_2F Start;
    D2D1_POINT_2F End;
    D2D1_COLOR_F Color;
    float StrokeWidth;

    LineCommand(const D2D1_POINT_2F& s, const D2D1_POINT_2F& e, const D2D1_COLOR_F& c, float sw) : DrawCommand(DrawCommandType::Line), Start(s), End(e), Color(c), StrokeWidth(sw) {}
};

// Rectangle command
struct RectangleCommand : public DrawCommand {
    D2D1_RECT_F Rect;
    D2D1_COLOR_F Color;
    float StrokeWidth;

    RectangleCommand(const D2D1_RECT_F& r, const D2D1_COLOR_F& c, float sw) : DrawCommand(DrawCommandType::Rectangle), Rect(r), Color(c), StrokeWidth(sw) {}
};

// Filled rectangle command
struct FilledRectangleCommand : public DrawCommand {
    D2D1_RECT_F Rect;
    D2D1_COLOR_F Color;

    FilledRectangleCommand(const D2D1_RECT_F& r, const D2D1_COLOR_F& c) : DrawCommand(DrawCommandType::FilledRectangle), Rect(r), Color(c) {}
};

// Dashed rectangle command
struct DashedRectangleCommand : public DrawCommand {
    D2D1_RECT_F Rect;
    D2D1_COLOR_F Color;
    float StrokeWidth;
    float DashLength;
    float GapLength;

    DashedRectangleCommand(const D2D1_RECT_F& r, const D2D1_COLOR_F& c, float sw, float dl, float gl) : DrawCommand(DrawCommandType::DashedRectangle), Rect(r), Color(c), StrokeWidth(sw), DashLength(dl), GapLength(gl) {}
};

// Text command
struct TextCommand : public DrawCommand {
    std::wstring Text;
    D2D1_RECT_F LayoutRect;
    D2D1_COLOR_F Color;
    IDWriteTextFormat* TextFormat;

    TextCommand(const std::wstring& t, const D2D1_RECT_F& r, const D2D1_COLOR_F& c, IDWriteTextFormat* tf) : DrawCommand(DrawCommandType::Text), Text(t), LayoutRect(r), Color(c), TextFormat(tf) {}
};

// Direct Bitmap command (with pointer to bitmap)
struct BitmapCommand : public DrawCommand {
    ID2D1Bitmap* Bitmap;
    D2D1_RECT_F DestRect;
    float Opacity;

    BitmapCommand(ID2D1Bitmap* bmp, const D2D1_RECT_F& rect, float op) : DrawCommand(DrawCommandType::Bitmap), Bitmap(bmp), DestRect(rect), Opacity(op) {}
};

// Rounded Rectangle command
struct RoundedRectangleCommand : public DrawCommand {
    D2D1_ROUNDED_RECT Rect;
    D2D1_COLOR_F Color;
    float StrokeWidth;

    RoundedRectangleCommand(const D2D1_ROUNDED_RECT& r, const D2D1_COLOR_F& c, float sw) : DrawCommand(DrawCommandType::RoundedRectangle), Rect(r), Color(c), StrokeWidth(sw) {}
};

// Filled Rounded Rectangle command
struct FilledRoundedRectangleCommand : public DrawCommand {
    D2D1_ROUNDED_RECT Rect;
    D2D1_COLOR_F Color;

    FilledRoundedRectangleCommand(const D2D1_ROUNDED_RECT& r, const D2D1_COLOR_F& c) : DrawCommand(DrawCommandType::FilledRoundedRectangle), Rect(r), Color(c) {}
};

// Ellipse command
struct EllipseCommand : public DrawCommand {
    D2D1_ELLIPSE Ellipse;
    D2D1_COLOR_F Color;
    float StrokeWidth;

    EllipseCommand(const D2D1_ELLIPSE& e, const D2D1_COLOR_F& c, float sw) : DrawCommand(DrawCommandType::Ellipse), Ellipse(e), Color(c), StrokeWidth(sw) {}
};

// Filled Ellipse command
struct FilledEllipseCommand : public DrawCommand {
    D2D1_ELLIPSE Ellipse;
    D2D1_COLOR_F Color;

    FilledEllipseCommand(const D2D1_ELLIPSE& e, const D2D1_COLOR_F& c) : DrawCommand(DrawCommandType::FilledEllipse), Ellipse(e), Color(c) {}
};

// Gradient Rectangle command
struct GradientRectangleCommand : public DrawCommand {
    D2D1_RECT_F Rect;
    std::vector<D2D1_GRADIENT_STOP> GradientStops;
    bool IsHorizontal;

    GradientRectangleCommand(const D2D1_RECT_F& r, const std::vector<D2D1_GRADIENT_STOP>& stops, bool horizontal) : DrawCommand(DrawCommandType::Gradient), Rect(r), GradientStops(stops), IsHorizontal(horizontal) {}
};

// Tie Line command (bezier curve)
struct TieLineCommand : public DrawCommand {
    D2D1_POINT_2F Point1;  // Start point
    D2D1_POINT_2F Point2;  // First control point
    D2D1_POINT_2F Point3;  // Second control point
    D2D1_POINT_2F Point4;  // End point
    D2D1_COLOR_F Color;
    float StrokeWidth;

    TieLineCommand(const D2D1_POINT_2F& p1, const D2D1_POINT_2F& p2, const D2D1_POINT_2F& p3, const D2D1_POINT_2F& p4, const D2D1_COLOR_F& c, float sw) : DrawCommand(DrawCommandType::TieLine), Point1(p1), Point2(p2), Point3(p3), Point4(p4), Color(c), StrokeWidth(sw) {}
};

// Color hash functor for unordered_map
struct ColorHash
{
    size_t operator()(const D2D1_COLOR_F& color) const
    {
        // Simple hash combining all components
        size_t H1 = std::hash<float>{}(color.r);
        size_t H2 = std::hash<float>{}(color.g);
        size_t H3 = std::hash<float>{}(color.b);
        size_t H4 = std::hash<float>{}(color.a);
        return ((H1 ^ (H2 << 1)) ^ (H3 << 2)) ^ (H4 << 3);
    }
};

// Color equality for unordered_map
struct ColorEqual
{
    bool operator()(const D2D1_COLOR_F& lhs, const D2D1_COLOR_F& rhs) const
    {
        constexpr float Epsilon = 0.0001f;
        return (std::abs(lhs.r - rhs.r) < Epsilon && std::abs(lhs.g - rhs.g) < Epsilon && std::abs(lhs.b - rhs.b) < Epsilon && std::abs(lhs.a - rhs.a) < Epsilon);
    }
};


class CommandBatch
{
private:
    std::vector<DrawCommand*> _Commands;
    bool _IsActive;

public:
    CommandBatch();
    ~CommandBatch();

    void Begin();

    void AddLine(const D2D1_POINT_2F& start, const D2D1_POINT_2F& end, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddRectangle(float left, float top, float right, float bottom, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddFilledRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float zOrder = 0.0f);
    void AddFilledRectangle(float left, float top, float right, float bottom, const D2D1_COLOR_F& color, float zOrder = 0.0f);
    void AddDashedRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float dashLength, float gapLength, float zOrder = 0.0f);
    void AddText(const std::wstring& text, const D2D1_RECT_F& layoutRect, const D2D1_COLOR_F& color, IDWriteTextFormat* textFormat, float zOrder = 0.0f);
    void AddRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddRoundedRectangle(float left, float top, float right, float bottom, float radiusX, float radiusY, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddFilledRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float zOrder = 0.0f);
    void AddFilledRoundedRectangle(float left, float top, float right, float bottom, float radiusX, float radiusY, const D2D1_COLOR_F& color, float zOrder = 0.0f);
    void AddEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddEllipse(float x, float y, float radiusX, float radiusY, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddFilledEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float zOrder = 0.0f);
    void AddFilledEllipse(float x, float y, float radiusX, float radiusY, const D2D1_COLOR_F& color, float zOrder = 0.0f);
    void AddBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& destRect, float opacity = 1.0f, float zOrder = 0.0f);
    void AddGradientRectangle(const D2D1_RECT_F& rect, const std::vector<D2D1_GRADIENT_STOP>& gradientStops, bool isHorizontal = true, float zOrder = 0.0f);
    void AddGradientRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorStart, const D2D1_COLOR_F& colorEnd, bool isHorizontal = true, float zOrder = 0.0f);
    void AddGradientRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorStart, const D2D1_COLOR_F& colorCenter, const D2D1_COLOR_F& colorEnd, bool isHorizontal = true, float zOrder = 0.0f);
    void AddTieLine(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2, const D2D1_POINT_2F& point3, const D2D1_POINT_2F& point4, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);
    void AddTieLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const D2D1_COLOR_F& color, float strokeWidth, float zOrder = 0.0f);

    void Execute(ID2D1RenderTarget* renderTarget, std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual>& brushCache, bool doSort = true);

private:
    void Clear();

    void ExecuteCommand(DrawCommand* cmd, ID2D1RenderTarget* renderTarget, std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual>& brushCache);
    ID2D1SolidColorBrush* GetBrush(const D2D1_COLOR_F& color, ID2D1RenderTarget* renderTarget, std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual>& brushCache);
};
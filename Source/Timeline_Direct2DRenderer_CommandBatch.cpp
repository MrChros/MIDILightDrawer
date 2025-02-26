#include "Timeline_Direct2DRenderer_CommandBatch.h"


CommandBatch::CommandBatch() : _IsActive(false) { }

CommandBatch::~CommandBatch() 
{
	Clear();
}

void CommandBatch::Begin()
{
    Clear();

    _IsActive = true;
}

void CommandBatch::AddLine(const D2D1_POINT_2F& start, const D2D1_POINT_2F& end, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    if (!_IsActive) {
        return;
    }
    
    _Commands.push_back(new LineCommand(start, end, color, strokeWidth));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddLine(float x1, float y1, float x2, float y2, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    AddLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), color, strokeWidth, zOrder);
}

void CommandBatch::AddRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new RectangleCommand(rect, color, strokeWidth));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddRectangle(float left, float top, float right, float bottom, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    AddRectangle(D2D1::RectF(left, top, right, bottom), color, strokeWidth, zOrder);
}

void CommandBatch::AddFilledRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new FilledRectangleCommand(rect, color));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddFilledRectangle(float left, float top, float right, float bottom, const D2D1_COLOR_F& color, float zOrder)
{
    AddFilledRectangle(D2D1::RectF(left, top, right, bottom), color, zOrder);
}

void CommandBatch::AddDashedRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float dashLength, float gapLength, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new DashedRectangleCommand(rect, color, strokeWidth, dashLength, gapLength));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddText(const std::wstring& text, const D2D1_RECT_F& layoutRect, const D2D1_COLOR_F& color, IDWriteTextFormat* textFormat, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new TextCommand(text, layoutRect, color, textFormat));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new RoundedRectangleCommand(rect, color, strokeWidth));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddRoundedRectangle(float left, float top, float right, float bottom, float radiusX, float radiusY, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    AddRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radiusX, radiusY), color, strokeWidth, zOrder);
}

void CommandBatch::AddFilledRoundedRectangle(const D2D1_ROUNDED_RECT& rect, const D2D1_COLOR_F& color, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new FilledRoundedRectangleCommand(rect, color));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddFilledRoundedRectangle(float left, float top, float right, float bottom, float radiusX, float radiusY, const D2D1_COLOR_F& color, float zOrder)
{
    AddFilledRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), radiusX, radiusY), color, zOrder);
}

void CommandBatch::AddEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new EllipseCommand(ellipse, color, strokeWidth));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddEllipse(float x, float y, float radiusX, float radiusY, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    AddEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radiusX, radiusY), color, strokeWidth, zOrder);
}

void CommandBatch::AddFilledEllipse(const D2D1_ELLIPSE& ellipse, const D2D1_COLOR_F& color, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new FilledEllipseCommand(ellipse, color));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddFilledEllipse(float x, float y, float radiusX, float radiusY, const D2D1_COLOR_F& color, float zOrder)
{
    AddFilledEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radiusX, radiusY), color, zOrder);
}

// Direct bitmap methods with actual bitmap pointers
void CommandBatch::AddBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& destRect, float opacity, float zOrder)
{
    if (!_IsActive || !bitmap) {
        return;
    }

    _Commands.push_back(new BitmapCommand(bitmap, destRect, opacity));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddGradientRectangle(const D2D1_RECT_F& rect, const std::vector<D2D1_GRADIENT_STOP>& gradientStops, bool isHorizontal, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new GradientRectangleCommand(rect, gradientStops, isHorizontal));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddGradientRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorStart, const D2D1_COLOR_F& colorEnd, bool isHorizontal, float zOrder)
{
    std::vector<D2D1_GRADIENT_STOP> stops = {
        {0.0f, colorStart},
        {1.0f, colorEnd}
    };

    AddGradientRectangle(rect, stops, isHorizontal, zOrder);
}

void CommandBatch::AddGradientRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& colorStart, const D2D1_COLOR_F& colorCenter, const D2D1_COLOR_F& colorEnd, bool isHorizontal, float zOrder)
{
    std::vector<D2D1_GRADIENT_STOP> stops = {
        {0.0f, colorStart},
        {0.5f, colorCenter},
        {1.0f, colorEnd}
    };

    AddGradientRectangle(rect, stops, isHorizontal, zOrder);
}

void CommandBatch::AddTieLine(const D2D1_POINT_2F& point1, const D2D1_POINT_2F& point2, const D2D1_POINT_2F& point3, const D2D1_POINT_2F& point4, const D2D1_COLOR_F& color, float strokeWidth, float zOrder)
{
    if (!_IsActive) {
        return;
    }

    _Commands.push_back(new TieLineCommand(point1, point2, point3, point4, color, strokeWidth));
    _Commands.back()->ZOrder = zOrder;
}

void CommandBatch::AddTieLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const D2D1_COLOR_F& color, float strokeWidth, float zOrder) 
{
    AddTieLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), D2D1::Point2F(x3, y3), D2D1::Point2F(x4, y4), color, strokeWidth, zOrder);
}

void CommandBatch::Execute(ID2D1RenderTarget* renderTarget, std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual>& brushCache, bool doSort)
{
    if (!_IsActive || !renderTarget) {
        return;
    }

    // Sort commands first by type, then by z-order for same types
    if(doSort) {
        std::sort(_Commands.begin(), _Commands.end(), [](const DrawCommand* a, const DrawCommand* b)
        {
            if (a->Type != b->Type) return a->Type < b->Type;
            return a->ZOrder < b->ZOrder;
        });
    }

    // Execute each command
    for (auto& cmd : _Commands) {
        ExecuteCommand(cmd, renderTarget, brushCache);
    }  

    _IsActive = false;
}

void CommandBatch::Clear()
{
    for (auto cmd : _Commands) {
        delete cmd;
    }
    _Commands.clear();
}

void CommandBatch::ExecuteCommand(DrawCommand* cmd, ID2D1RenderTarget* renderTarget, std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual>& brushCache)
{
    switch (cmd->Type)
    {
        case DrawCommandType::Line:
        {
            LineCommand* lineCmd = static_cast<LineCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(lineCmd->Color, renderTarget, brushCache);
            if (brush) {
                renderTarget->DrawLine(lineCmd->Start, lineCmd->End, brush, lineCmd->StrokeWidth);
            }
            break;
        }

        case DrawCommandType::Rectangle:
        {
            RectangleCommand* rectCmd = static_cast<RectangleCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(rectCmd->Color, renderTarget, brushCache);
            if (brush) {
                renderTarget->DrawRectangle(rectCmd->Rect, brush, rectCmd->StrokeWidth);
            }
            break;
        }

        case DrawCommandType::FilledRectangle:
        {
            FilledRectangleCommand* fillRectCmd = static_cast<FilledRectangleCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(fillRectCmd->Color, renderTarget, brushCache);
            if (brush) {
                renderTarget->FillRectangle(fillRectCmd->Rect, brush);
            }
            break;
        }

        case DrawCommandType::DashedRectangle:
        {
            DashedRectangleCommand* dashedRectCmd = static_cast<DashedRectangleCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(dashedRectCmd->Color, renderTarget, brushCache);
            if (brush) {
                // Create dashed stroke style
                float dashes[] = { dashedRectCmd->DashLength / dashedRectCmd->StrokeWidth,
                                   dashedRectCmd->GapLength / dashedRectCmd->StrokeWidth };

                ID2D1StrokeStyle* dashedStroke = nullptr;
                ID2D1Factory* factory = nullptr;
                renderTarget->GetFactory(&factory);

                if (factory) {
                    D2D1_STROKE_STYLE_PROPERTIES dashedProps = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f);

                    HRESULT hr = factory->CreateStrokeStyle(dashedProps, dashes, 2, &dashedStroke);

                    if (SUCCEEDED(hr) && dashedStroke) {
                        renderTarget->DrawRectangle(dashedRectCmd->Rect, brush, dashedRectCmd->StrokeWidth, dashedStroke);
                        dashedStroke->Release();
                    }

                    factory->Release();
                }
            }
            break;
        }

        case DrawCommandType::Text:
        {
            TextCommand* textCmd = static_cast<TextCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(textCmd->Color, renderTarget, brushCache);
            if (brush && textCmd->TextFormat) {
                renderTarget->DrawText(
                    textCmd->Text.c_str(),
                    static_cast<UINT32>(textCmd->Text.length()),
                    textCmd->TextFormat,
                    textCmd->LayoutRect,
                    brush
                );
            }
            break;
        }

        case DrawCommandType::Bitmap:
        {
            BitmapCommand* bitmapCmd = static_cast<BitmapCommand*>(cmd);
            if (bitmapCmd->Bitmap) {
                renderTarget->DrawBitmap(
                    bitmapCmd->Bitmap,
                    bitmapCmd->DestRect,
                    bitmapCmd->Opacity,
                    D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                );
            }
            break;
        }

        case DrawCommandType::RoundedRectangle:
        {
            RoundedRectangleCommand* roundRectCmd = static_cast<RoundedRectangleCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(roundRectCmd->Color, renderTarget, brushCache);

            if (brush) {
                renderTarget->DrawRoundedRectangle(roundRectCmd->Rect, brush, roundRectCmd->StrokeWidth);
            }
            break;
        }

        case DrawCommandType::FilledRoundedRectangle:
        {
            FilledRoundedRectangleCommand* fillRoundRectCmd = static_cast<FilledRoundedRectangleCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(fillRoundRectCmd->Color, renderTarget, brushCache);

            if (brush) {
                renderTarget->FillRoundedRectangle(fillRoundRectCmd->Rect, brush);
            }
            break;
        }

        case DrawCommandType::Ellipse:
        {
            EllipseCommand* ellipseCmd = static_cast<EllipseCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(ellipseCmd->Color, renderTarget, brushCache);

            if (brush) {
                renderTarget->DrawEllipse(ellipseCmd->Ellipse, brush, ellipseCmd->StrokeWidth);
            }
            break;
        }

        case DrawCommandType::FilledEllipse:
        {
            FilledEllipseCommand* fillEllipseCmd = static_cast<FilledEllipseCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(fillEllipseCmd->Color, renderTarget, brushCache);

            if (brush) {
                renderTarget->FillEllipse(fillEllipseCmd->Ellipse, brush);
            }
            break;
        }

        case DrawCommandType::Gradient:
        {
            GradientRectangleCommand* gradCmd = static_cast<GradientRectangleCommand*>(cmd);

            // Create gradient stop collection
            ID2D1GradientStopCollection* pGradientStops = nullptr;
            HRESULT hr = renderTarget->CreateGradientStopCollection(
                gradCmd->GradientStops.data(),
                static_cast<UINT32>(gradCmd->GradientStops.size()),
                D2D1_GAMMA_2_2,
                D2D1_EXTEND_MODE_CLAMP,
                &pGradientStops
            );

            if (SUCCEEDED(hr) && pGradientStops) {
                // Create linear gradient brush
                ID2D1LinearGradientBrush* pLinearGradientBrush = nullptr;

                D2D1_POINT_2F StartPoint, EndPoint;
                if (gradCmd->IsHorizontal) {
                    // Horizontal gradient (left to right)
                    StartPoint = D2D1::Point2F(gradCmd->Rect.left, (gradCmd->Rect.top + gradCmd->Rect.bottom) / 2);
                    EndPoint = D2D1::Point2F(gradCmd->Rect.right, (gradCmd->Rect.top + gradCmd->Rect.bottom) / 2);
                }
                else {
                    // Vertical gradient (top to bottom)
                    StartPoint = D2D1::Point2F((gradCmd->Rect.left + gradCmd->Rect.right) / 2, gradCmd->Rect.top);
                    EndPoint = D2D1::Point2F((gradCmd->Rect.left + gradCmd->Rect.right) / 2, gradCmd->Rect.bottom);
                }

                hr = renderTarget->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(StartPoint, EndPoint),
                    pGradientStops,
                    &pLinearGradientBrush
                );

                if (SUCCEEDED(hr) && pLinearGradientBrush) {
                    // Fill rectangle with gradient
                    renderTarget->FillRectangle(gradCmd->Rect, pLinearGradientBrush);
                    pLinearGradientBrush->Release();
                }

                pGradientStops->Release();
            }
            break;
        }

        case DrawCommandType::TieLine:
        {
            TieLineCommand* tieCmd = static_cast<TieLineCommand*>(cmd);
            ID2D1SolidColorBrush* brush = GetBrush(tieCmd->Color, renderTarget, brushCache);

            if (brush) {
                // Create path geometry for bezier curve
                ID2D1Factory* factory = nullptr;
                renderTarget->GetFactory(&factory);

                if (factory) {
                    ID2D1PathGeometry* pathGeometry = nullptr;
                    HRESULT hr = factory->CreatePathGeometry(&pathGeometry);

                    if (SUCCEEDED(hr) && pathGeometry) {
                        ID2D1GeometrySink* Sink = nullptr;
                        hr = pathGeometry->Open(&Sink);

                        if (SUCCEEDED(hr) && Sink) {
                            // Define the bezier curve
                            Sink->BeginFigure(tieCmd->Point1, D2D1_FIGURE_BEGIN_HOLLOW);

                            D2D1_BEZIER_SEGMENT bezier;
                            bezier.point1 = tieCmd->Point2;  // First control point
                            bezier.point2 = tieCmd->Point3;  // Second control point
                            bezier.point3 = tieCmd->Point4;  // End point

                            Sink->AddBezier(bezier);
                            Sink->EndFigure(D2D1_FIGURE_END_OPEN);

                            // Close the sink
                            Sink->Close();
                            Sink->Release();

                            // Draw the bezier curve
                            renderTarget->DrawGeometry(pathGeometry, brush, tieCmd->StrokeWidth);
                            pathGeometry->Release();
                        }
                    }

                    factory->Release();
                }
            }
            break;
        }
    }
}

ID2D1SolidColorBrush* CommandBatch::GetBrush(const D2D1_COLOR_F& color, ID2D1RenderTarget* renderTarget, std::unordered_map<D2D1_COLOR_F, ID2D1SolidColorBrush*, ColorHash, ColorEqual>& brushCache)
{
    auto it = brushCache.find(color);
    if (it != brushCache.end()) {
        return it->second;
    }

    ID2D1SolidColorBrush* brush = nullptr;
    HRESULT hr = renderTarget->CreateSolidColorBrush(color, &brush);
    if (SUCCEEDED(hr) && brush) {
        brushCache[color] = brush;
        return brush;
    }
    return nullptr;
}
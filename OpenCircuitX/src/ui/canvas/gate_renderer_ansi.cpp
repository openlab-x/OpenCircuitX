#include "gate_renderer_ansi.h"

// Cubic Bezier approximation constant for a quarter-circle arc:
// k = 4/3 * (sqrt(2) - 1)  ~= 0.5523
static constexpr double K = 0.5523;

//--
// Dispatch
//--
void ANSIGateRenderer::Draw(wxGraphicsContext* gc,
                             GateType           type,
                             const wxRect&      bounds,
                             const wxColour&    body,
                             const wxColour&    border) const
{
    switch (type)
    {
    case GateType::AND:   DrawAND(gc, bounds, body, border); break;
    case GateType::NAND:  DrawAND(gc, bounds, body, border);
                          DrawBubble(gc,
                              bounds.x + bounds.width,
                              bounds.y + bounds.height * 0.5,
                              bounds.height * 0.10,
                              body, border);
                          break;

    case GateType::OR:    DrawOR(gc, bounds, body, border); break;
    case GateType::NOR:   DrawOR(gc, bounds, body, border);
                          DrawBubble(gc,
                              bounds.x + bounds.width,
                              bounds.y + bounds.height * 0.5,
                              bounds.height * 0.10,
                              body, border);
                          break;

    case GateType::XOR:   DrawXOR(gc, bounds, body, border); break;
    case GateType::XNOR:  DrawXOR(gc, bounds, body, border);
                          DrawBubble(gc,
                              bounds.x + bounds.width,
                              bounds.y + bounds.height * 0.5,
                              bounds.height * 0.10,
                              body, border);
                          break;

    case GateType::NOT:   DrawNOT(gc, bounds, body, border); break;

    default: break;
    }
}

//--
// AND - flat left side + semicircular right side
//         semicircle centre:  (x + W - H/2,  y + H/2)
//         radius:              H/2
//--
void ANSIGateRenderer::DrawAND(wxGraphicsContext* gc,
                                const wxRect&      b,
                                const wxColour&    fill,
                                const wxColour&    stroke) const
{
    const wxDouble x = b.x,  y = b.y;
    const wxDouble W = b.width, H = b.height;
    const wxDouble r  = H * 0.5;
    const wxDouble cx = x + W - r;   // x of semicircle centre

    wxGraphicsPath p = gc->CreatePath();
    p.MoveToPoint(x, y);
    p.AddLineToPoint(cx, y);
    // Upper quarter-circle: (cx, y) → (cx+r, y+H/2)
    p.AddCurveToPoint(cx + r * K,  y,
                      cx + r,      y + H * 0.5 - r * K,
                      cx + r,      y + H * 0.5);
    // Lower quarter-circle: (cx+r, y+H/2) → (cx, y+H)
    p.AddCurveToPoint(cx + r,      y + H * 0.5 + r * K,
                      cx + r * K,  y + H,
                      cx,          y + H);
    p.AddLineToPoint(x, y + H);
    p.CloseSubpath();

    gc->SetBrush(gc->CreateBrush(wxBrush(fill)));
    gc->SetPen(gc->CreatePen(wxPen(stroke, 1.5)));
    gc->DrawPath(p);
}

//--
// OR - concave back + two arcs meeting at a right-pointing tip
//--
void ANSIGateRenderer::DrawOR(wxGraphicsContext* gc,
                               const wxRect&      b,
                               const wxColour&    fill,
                               const wxColour&    stroke) const
{
    const wxDouble x = b.x,  y = b.y;
    const wxDouble W = b.width, H = b.height;

    wxGraphicsPath p = gc->CreatePath();
    p.MoveToPoint(x, y);
    // Top arc: top-left → output tip
    p.AddCurveToPoint(x + W * 0.50, y,
                      x + W * 0.90, y + H * 0.25,
                      x + W,        y + H * 0.5);
    // Bottom arc: output tip → bottom-left
    p.AddCurveToPoint(x + W * 0.90, y + H * 0.75,
                      x + W * 0.50, y + H,
                      x,            y + H);
    // Concave back: bottom-left → top-left
    p.AddCurveToPoint(x + W * 0.25, y + H * 0.75,
                      x + W * 0.25, y + H * 0.25,
                      x,            y);
    p.CloseSubpath();

    gc->SetBrush(gc->CreateBrush(wxBrush(fill)));
    gc->SetPen(gc->CreatePen(wxPen(stroke, 1.5)));
    gc->DrawPath(p);
}

//--
// XOR - OR body + extra concave arc to the left of the input side
//--
void ANSIGateRenderer::DrawXOR(wxGraphicsContext* gc,
                                const wxRect&      b,
                                const wxColour&    fill,
                                const wxColour&    stroke) const
{
    // Draw the OR body first
    DrawOR(gc, b, fill, stroke);

    // Extra arc: same concave shape as the OR back curve, offset left
    const wxDouble x = b.x,  y = b.y;
    const wxDouble W = b.width, H = b.height;
    const wxDouble xoff = W * 0.12;   // ~8-9 px at default zoom=1
    const wxDouble xe   = x - xoff;

    wxGraphicsPath arc = gc->CreatePath();
    arc.MoveToPoint(xe, y + H * 0.1);
    arc.AddCurveToPoint(xe + W * 0.25, y + H * 0.1,
                        xe + W * 0.25, y + H * 0.9,
                        xe,            y + H * 0.9);

    gc->SetBrush(wxNullBrush);
    gc->SetPen(gc->CreatePen(wxPen(stroke, 1.5)));
    gc->StrokePath(arc);
}

//--
// NOT - right-pointing triangle + inversion bubble at the tip
//         Bubble radius = H * 0.10, triangle tip stops short to leave room.
//--
void ANSIGateRenderer::DrawNOT(wxGraphicsContext* gc,
                                const wxRect&      b,
                                const wxColour&    fill,
                                const wxColour&    stroke) const
{
    const wxDouble x = b.x,  y = b.y;
    const wxDouble W = b.width, H = b.height;
    const wxDouble br = H * 0.10;         // bubble radius
    const wxDouble tipX = x + W - br * 2; // triangle tip x (bubble fits after it)

    wxGraphicsPath tri = gc->CreatePath();
    tri.MoveToPoint(x,    y);
    tri.AddLineToPoint(tipX, y + H * 0.5);
    tri.AddLineToPoint(x,    y + H);
    tri.CloseSubpath();

    gc->SetBrush(gc->CreateBrush(wxBrush(fill)));
    gc->SetPen(gc->CreatePen(wxPen(stroke, 1.5)));
    gc->DrawPath(tri);

    // Bubble sits just past the triangle tip
    DrawBubble(gc, tipX + br, y + H * 0.5, br, fill, stroke);
}

//--
// Bubble - small filled circle (inversion marker at output)
//--
void ANSIGateRenderer::DrawBubble(wxGraphicsContext* gc,
                                   wxDouble cx, wxDouble cy, wxDouble r,
                                   const wxColour& fill,
                                   const wxColour& stroke) const
{
    gc->SetBrush(gc->CreateBrush(wxBrush(fill)));
    gc->SetPen(gc->CreatePen(wxPen(stroke, 1.5)));
    gc->DrawEllipse(cx - r, cy - r, r * 2.0, r * 2.0);
}

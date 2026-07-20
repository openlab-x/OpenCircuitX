#pragma once
#include "gate_renderer.h"

// ANSI/IEEE Std 91 shape renderer.
// AND - flat left side + D-shaped right (semicircle via 2 cubic Beziers).
// OR - concave back + two arcs meeting at output point.
// XOR - OR body + extra concave arc to the left of the input side.
// NOT - right-pointing triangle + inversion bubble at output.
// NAND / NOR / XNOR - base shape + inversion bubble at output.
class ANSIGateRenderer : public GateRenderer
{
public:
    void Draw(wxGraphicsContext* gc,
              GateType           type,
              const wxRect&      bounds,
              const wxColour&    body,
              const wxColour&    border) const override;

private:
    void DrawAND   (wxGraphicsContext* gc, const wxRect& b,
                    const wxColour& fill, const wxColour& stroke) const;
    void DrawOR    (wxGraphicsContext* gc, const wxRect& b,
                    const wxColour& fill, const wxColour& stroke) const;
    void DrawXOR   (wxGraphicsContext* gc, const wxRect& b,
                    const wxColour& fill, const wxColour& stroke) const;
    void DrawNOT   (wxGraphicsContext* gc, const wxRect& b,
                    const wxColour& fill, const wxColour& stroke) const;
    void DrawBubble(wxGraphicsContext* gc,
                    wxDouble cx, wxDouble cy, wxDouble r,
                    const wxColour& fill, const wxColour& stroke) const;
};

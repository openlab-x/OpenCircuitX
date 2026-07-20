#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include "core/circuit/gate.h"

enum class GateScheme { ANSI, IEC, BS3939 };

// True for gates that have a distinctive ANSI shape (AND/OR/NOT family).
// All other types (DFF, MUX, adders, IO ports...) always draw as rectangular blocks.
inline bool GateHasDistinctiveShape(GateType t)
{
    switch (t)
    {
    case GateType::AND:
    case GateType::OR:
    case GateType::NOT:
    case GateType::NAND:
    case GateType::NOR:
    case GateType::XOR:
    case GateType::XNOR:
        return true;
    default:
        return false;
    }
}

class GateRenderer
{
public:
    virtual ~GateRenderer() = default;

    // Draw the gate body only - no pins, stubs, labels, or selection ring.
    // gc     : wxGraphicsContext created via wxGraphicsContext::CreateFromDC.
    // type   : caller guarantees GateHasDistinctiveShape(type) == true.
    // bounds : zoomed screen rect (sb) as computed in DrawGate.
    // body   : fill colour (already incorporates selection state).
    // border : stroke colour (type-specific accent from the body colour block).
    virtual void Draw(wxGraphicsContext* gc,
                      GateType           type,
                      const wxRect&      bounds,
                      const wxColour&    body,
                      const wxColour&    border) const = 0;
};

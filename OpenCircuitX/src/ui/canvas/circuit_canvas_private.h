#pragma once
#include <algorithm>
#include <wx/colour.h>
#include <wx/gdicmn.h>

static constexpr int   GRID      = 20;
static constexpr int   PIN_R     = 5;
static constexpr int   PIN_HIT   = 10;
static constexpr int   PIN_STUB  = 16;
static constexpr float ZOOM_STEP = 0.15f;
static constexpr float ZOOM_MIN  = 0.25f;
static constexpr float ZOOM_MAX  = 4.00f;
static constexpr int   UNDO_LIMIT = 50;

static wxColour PinColour(bool state, bool driven)
{
    if (!driven) return wxColour(90, 90, 90);
    return state ? wxColour(80, 200, 80) : wxColour(200, 80, 80);
}

static int Dist2(wxPoint a, wxPoint b)
{
    int dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Distance from point P to line segment AB (squared)
static int SegDist2(wxPoint p, wxPoint a, wxPoint b)
{
    int dx = b.x - a.x, dy = b.y - a.y;
    if (dx == 0 && dy == 0)
        return Dist2(p, a);
    float t = float((p.x - a.x) * dx + (p.y - a.y) * dy) / float(dx * dx + dy * dy);
    t = std::max(0.0f, std::min(1.0f, t));
    float cx = a.x + t * dx, cy = a.y + t * dy;
    int ex = p.x - (int)cx, ey = p.y - (int)cy;
    return ex * ex + ey * ey;
}

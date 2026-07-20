#include "wxMaterialDesignArtProvider.hpp"
#include <wx/artprov.h>
#include <wx/bmpbndl.h>
#include <wx/colour.h>

wxBitmapBundle wxMaterialDesignArtProvider::CreateBitmapBundle(const wxArtID& id,
                                                       const wxArtClient& client,
                                                       const wxSize& size,
                                                       const wxColour& color)
{
    if (client == wxART_CLIENT_MATERIAL_FILLED)
        return CreateFilledMaterialArtBitmapBundleByID(id, size, color);
    return wxBitmapBundle();
}

wxBitmapBundle wxMaterialDesignArtProvider::CreateBitmapBundle(const wxArtID& id,
                                                       const wxArtClient& client,
                                                       const wxSize& size)
{
    return CreateBitmapBundle(id, client, size, wxNullColour);
}

wxBitmap wxMaterialDesignArtProvider::GetBitmap(const wxArtID& id,
                                           const wxArtClient& client,
                                           const wxSize& size,
                                           const wxColour& color)
{
    if (client == wxART_CLIENT_MATERIAL_FILLED)
        return CreateFilledMaterialArtBitmapByID(id, size, color);
    return wxNullBitmap;
}

wxBitmap wxMaterialDesignArtProvider::CreateBitmap(const wxArtID& id,
                                           const wxArtClient& client,
                                           const wxSize& size,
                                           const wxColour& color)
{
    return GetBitmap(id, client, size, color);
}

wxBitmap wxMaterialDesignArtProvider::CreateBitmap(const wxArtID& id,
                                           const wxArtClient& client,
                                           const wxSize& size)
{
    return CreateBitmap(id, client, size, wxNullColour);
}

bool wxMaterialDesignArtProvider::HasClient(const wxArtClient& client)
{
    return (client == wxART_CLIENT_MATERIAL_FILLED);
}

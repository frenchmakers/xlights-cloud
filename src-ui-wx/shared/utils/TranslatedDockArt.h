#pragma once

/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include <wx/aui/dockart.h>
#include <wx/aui/framemanager.h>
#include <wx/translation.h>

// Draws AUI pane captions through the UI translation catalogs. Captions are
// also saved in perspectives and restored from them, so translating them when
// they are drawn covers every place a pane gets its caption.
template <class Base = wxAuiDefaultDockArt>
class TranslatedCaptionDockArt : public Base {
public:
    void DrawCaption(wxDC& dc, wxWindow* window, const wxString& text, const wxRect& rect, wxAuiPaneInfo& pane) override {
        // "Effect" (old caption of the effect settings pane, still in saved
        // perspectives) and "Color" must stay English elsewhere, where the code
        // reads them back: these panes use a caption of their own instead.
        if (pane.name == "Effect" && text == "Effect") {
            Base::DrawCaption(dc, window, wxGetTranslation(wxTRANSLATE("Effect Settings")), rect, pane);
            return;
        }
        if (pane.name == "Color" && text == "Color") {
            Base::DrawCaption(dc, window, wxGetTranslation(wxTRANSLATE("Color Panel")), rect, pane);
            return;
        }
        Base::DrawCaption(dc, window, text.empty() ? text : wxGetTranslation(text), rect, pane);
    }
};

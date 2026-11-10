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
#include <wx/aui/floatpane.h>
#include <wx/aui/framemanager.h>
#include <wx/translation.h>

// AUI pane captions are saved in perspectives and restored from them, so they
// stay English and are translated where they are shown: the caption bar of a
// docked pane and the title of a floating one.
inline wxString TranslatedPaneCaption(const wxString& text) {
    // "Effect" (old caption of the effect settings pane, still in saved
    // perspectives) and "Color" must stay English elsewhere, where the code
    // reads them back: these panes use a caption of their own instead.
    if (text == "Effect") {
        return wxGetTranslation(wxTRANSLATE("Effect Settings"));
    }
    if (text == "Color") {
        return wxGetTranslation(wxTRANSLATE("Color Panel"));
    }
    return text.empty() ? text : wxGetTranslation(text);
}

template <class Base = wxAuiDefaultDockArt>
class TranslatedCaptionDockArt : public Base {
public:
    void DrawCaption(wxDC& dc, wxWindow* window, const wxString& text, const wxRect& rect, wxAuiPaneInfo& pane) override {
        Base::DrawCaption(dc, window, TranslatedPaneCaption(text), rect, pane);
    }
};

class TranslatedFloatingFrame : public wxAuiFloatingFrame {
public:
    using wxAuiFloatingFrame::wxAuiFloatingFrame;

    void SetTitle(const wxString& title) override {
        wxAuiFloatingFrame::SetTitle(TranslatedPaneCaption(title));
    }
};

// wxAuiManager whose floating panes show a translated title.
class TranslatedAuiManager : public wxAuiManager {
public:
    using wxAuiManager::wxAuiManager;

    wxAuiFloatingFrame* CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& p) override {
        return new TranslatedFloatingFrame(parent, this, p);
    }
};

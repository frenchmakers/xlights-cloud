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

#include <wx/dialog.h>
#include <wx/intl.h>

#include <string>

#include "utils/ShowSaveMerge.h"

class wxButton;
class wxListCtrl;
class wxListEvent;
class wxStaticText;
class wxTextCtrl;

// What the two sides of ShowFileMergeDialog are called. The defaults fit a
// save: "mine" is this session, "theirs" the file someone else saved.
struct ShowFileMergeLabels {
    std::string intro = _("Someone else saved changes to the same things you changed. Choose which version to keep for each one.\n"
                          "Everything else you and they changed is merged.").utf8_string();
    std::string mine = wxGETTEXT_IN_CONTEXT("merge side", "Mine").utf8_string();
    std::string theirs = wxGETTEXT_IN_CONTEXT("merge side", "Theirs").utf8_string();
    std::string mineColumn = _("Yours").utf8_string();
    std::string theirsColumn = _("Theirs (saved on disk)").utf8_string();
    // "Save" when deciding a save; "Apply" when bringing someone else's
    // changes into the open show without saving.
    std::string ok = _("Save").utf8_string();
};

// Lists what both this session and someone else changed differently in the
// shared show files and asks which version to keep for each one. Save stays
// disabled until every conflict has an answer; Cancel saves nothing.
class ShowFileMergeDialog : public wxDialog {
public:
    using Labels = ShowFileMergeLabels;

    ShowFileMergeDialog(wxWindow* parent, ShowSaveMerge::Plan& plan, const Labels& labels = Labels());

private:
    void Populate();
    void ShowDetail();
    void Choose(ShowFileMerge::Resolution resolution);
    void ChooseMapTarget();
    void UpdateButtons();
    void OnSelect(wxListEvent& event);

    ShowSaveMerge::Plan& _plan;
    Labels _labels;
    wxListCtrl* _list = nullptr;
    wxTextCtrl* _base = nullptr;
    wxTextCtrl* _mine = nullptr;
    wxTextCtrl* _theirs = nullptr;
    wxStaticText* _status = nullptr;
    wxButton* _useMine = nullptr;
    wxButton* _useTheirs = nullptr;
    wxButton* _keepBoth = nullptr;
    wxButton* _save = nullptr;
};

// The save report: what the merge brought in and what to check.
void ShowShowFileSaveReport(wxWindow* parent, const std::string& text);

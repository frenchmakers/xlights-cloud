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

#include <wx/choice.h>
#include <wx/clntdata.h>
#include <wx/translation.h>

// A wxChoice whose items are saved and restored by their English text can
// still show them translated: TranslateChoiceItems() keeps the English text
// with each item and shows its translation in `context` (mark the items with
// wxTRANSLATE_IN_CONTEXT so they reach the catalog); read and select through
// GetEnglishSelection() / SetEnglishSelection() instead of the string ones.
inline void TranslateChoiceItems(wxChoice* choice, const wxString& context)
{
    if (choice == nullptr) {
        return;
    }
    for (unsigned int i = 0; i < choice->GetCount(); i++) {
        const wxString english = choice->GetString(i);
        choice->SetClientObject(i, new wxStringClientData(english));
        const wxString& tr = wxGetTranslation(english, wxString(), context);
        if (tr != english) {
            choice->SetString(i, tr);
        }
    }
}

inline wxString GetEnglishSelection(const wxChoice* choice)
{
    const int sel = choice->GetSelection();
    if (sel == wxNOT_FOUND) {
        return wxString();
    }
    if (auto* data = dynamic_cast<wxStringClientData*>(choice->GetClientObject(sel))) {
        return data->GetData();
    }
    return choice->GetString(sel);
}

inline bool SetEnglishSelection(wxChoice* choice, const wxString& english)
{
    for (unsigned int i = 0; i < choice->GetCount(); i++) {
        auto* data = dynamic_cast<wxStringClientData*>(choice->GetClientObject(i));
        if ((data != nullptr ? data->GetData() : choice->GetString(i)) == english) {
            choice->SetSelection(i);
            return true;
        }
    }
    return false;
}

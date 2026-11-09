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

#include <wx/propgrid/props.h>
#include <wx/propgrid/advprops.h>
#include <string>

class Model;
class OutputManager;
class xlColor;

// only allows 0-9,- characters
class wxDropPatternProperty : public wxStringProperty {
public:
    wxDropPatternProperty(const wxString& label = wxPG_LABEL,
        const wxString& name = wxPG_LABEL,
        const wxString& value = wxEmptyString)
        : wxStringProperty(label, name, value) {}
    virtual bool ValidateValue(wxVariant& value, wxPGValidationInfo& validationInfo) const override;
};

// Property grid helper arrays used by model property adapters
extern wxArrayString NODE_TYPES;
extern wxArrayString RGBW_HANDLING;
extern wxArrayString PIXEL_STYLES;
extern wxArrayString CONTROLLER_DIRECTION;

extern const std::string CLICK_TO_EDIT;

// Choice labels translated for display, for a wxEnumProperty whose values
// are the indexes: the selection is still read by index, and the shared choices
// stay in English for the code that compares their text. Only the first
// translateCount labels are translated, e.g. the built-in entries of a list
// that ends with names chosen by the user.
wxArrayString TranslatedChoices(const wxPGChoices& choices, size_t translateCount = (size_t)-1);
wxArrayString TranslatedChoices(const wxArrayString& labels, size_t translateCount = (size_t)-1);

// Helper to extract color info from a property grid color property
wxString GetColorString(wxPGProperty* p, xlColor& xc);

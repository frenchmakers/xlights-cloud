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

#include <string>
#include <vector>

#include <wx/string.h>

namespace UILanguage {

struct Entry {
    std::string code;   // value stored in the config ("" = system default)
    wxString nativeName; // shown in the Preferences choice, never translated
};

// Languages offered in Preferences. The first entry is "system default".
const std::vector<Entry>& Available();

// Config value ("", "en", "fr", ...). Read once at startup.
std::string GetConfigured();
void SetConfigured(const std::string& code);

// Loads the xlights and wxstd catalogs for the configured language.
// Only translations are switched: the C/CRT locale is left alone so number
// parsing (strtod, atof) keeps using '.' when reading show files.
void Initialise();

}

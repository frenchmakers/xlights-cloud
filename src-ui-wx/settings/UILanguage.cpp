/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "UILanguage.h"

#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include <wx/translation.h>

#include <spdlog/spdlog.h>

#include "settings/XLightsConfigAdapter.h"
#include "shared/utils/wxUtilities.h"

namespace UILanguage {

static const char* CONFIG_KEY = "xLightsUILanguage";

const std::vector<Entry>& Available() {
    static const std::vector<Entry> langs = {
        { "", wxString::FromUTF8("System default") },
        { "en", wxString::FromUTF8("English") },
        { "fr", wxString::FromUTF8("Fran\xC3\xA7" "ais") },
    };
    return langs;
}

// Unset means English: a partial translation must not switch on by itself
// for every user whose system happens to be in that language.
std::string GetConfigured() {
    return GetXLightsConfig()->Read(CONFIG_KEY, std::string("en"));
}

void SetConfigured(const std::string& code) {
    auto* config = GetXLightsConfig();
    config->Write(CONFIG_KEY, code);
    config->Flush();
}

void Initialise() {
    // Catalogs live in <dir>/locale/<lang>/xlights.mo next to the executable
    // (Windows/Linux dev builds) or under the resources dir (macOS bundle, Linux install).
    wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    wxFileTranslationsLoader::AddCatalogLookupPathPrefix(exeDir + wxFILE_SEP_PATH + "locale");
    wxString resDir = wxString::FromUTF8(GetResourcesDirectory());
    if (!resDir.IsEmpty() && resDir != exeDir) {
        wxFileTranslationsLoader::AddCatalogLookupPathPrefix(resDir + wxFILE_SEP_PATH + "locale");
    }

    auto* translations = new wxTranslations();
    const std::string code = GetConfigured();
    if (!code.empty()) {
        translations->SetLanguage(wxString::FromUTF8(code));
    }
    wxTranslations::Set(translations);

    translations->AddStdCatalog();
    const bool loaded = translations->AddCatalog("xlights", wxLANGUAGE_ENGLISH_US);
    spdlog::info("UI language: '{}' (xlights catalog {})", code.empty() ? "system" : code,
                 loaded ? "loaded" : "not loaded");
}

}

/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "CloudStorageSettings.h"

#include "ai/WxServiceSettingsStore.h"
#include "settings/XLightsConfigAdapter.h"
#include "utils/string_utils.h"

#include <wx/secretstore.h>
#include <wx/translation.h>

#include <log.h>

#include <algorithm>
#include <cstdlib>

namespace {

const char* SECRET_NAME = "xLightsCloudStorage";
// Where the secret goes when the system's secure store can't be used (Linux
// without a keyring service, e.g. a headless machine).
const char* SECRET_FALLBACK_KEY = "CloudStorageSecretAccessKey";

bool SaveToSecureStore(const std::string& secret) {
#if wxUSE_SECRETSTORE
    wxSecretStore store = wxSecretStore::GetDefault();
    if (store.IsOk()) {
        return store.Save(SECRET_NAME, "token", wxSecretValue(wxString::FromUTF8(secret)));
    }
#endif
    return false;
}

} // namespace

bool CloudStorageSettings::SecureStoreAvailable(std::string* reason) {
    // On a machine without a screen a keyring service may be installed but
    // locked, and unlocking it asks in a window nobody sees: this skips it.
    if (const char* off = std::getenv("XLIGHTS_CLOUD_NO_KEYRING"); off != nullptr && *off != '\0' && std::string(off) != "0") {
        if (reason != nullptr) {
            *reason = "turned off by XLIGHTS_CLOUD_NO_KEYRING";
        }
        return false;
    }
#if wxUSE_SECRETSTORE
    // Opening the store connects to the keyring service, so do it once.
    static wxString error;
    static const bool ok = wxSecretStore::GetDefault().IsOk(&error);
    if (!ok && reason != nullptr) {
        *reason = error.utf8_string();
    }
    return ok;
#else
    if (reason != nullptr) {
        *reason = "not supported by this build";
    }
    return false;
#endif
}

CloudStorageSettings CloudStorageSettings::Load(bool loadSecret) {
    CloudStorageSettings s;
    auto* config = GetXLightsConfig();
    s.provider = config->Read("CloudStorageProvider", std::string(PROVIDER_R2));
    s.r2AccountId = config->Read("CloudStorageR2AccountId", std::string());
    s.endpoint = config->Read("CloudStorageEndpoint", std::string());
    s.region = config->Read("CloudStorageRegion", std::string("auto"));
    s.bucket = config->Read("CloudStorageBucket", std::string());
    s.prefix = config->Read("CloudStoragePrefix", std::string("xlights"));
    s.accessKeyId = config->Read("CloudStorageAccessKeyId", std::string());
    s.pathStyle = config->ReadBool("CloudStoragePathStyle", true);
    s.includeFseq = config->ReadBool("CloudStorageIncludeFseq", true);
    s.includeThumbnail = config->ReadBool("CloudStorageIncludeThumbnail", true);
    s.keepBackups = std::clamp((int)config->ReadLong("CloudStorageKeepBackups", 0), 0, MAX_KEEP_BACKUPS);
    if (loadSecret) {
        if (SecureStoreAvailable()) {
            WxServiceSettingsStore store;
            s.secretAccessKey = store.getSecret(SECRET_NAME);
        }
        if (s.secretAccessKey.empty()) {
            s.secretAccessKey = config->Read(SECRET_FALLBACK_KEY, std::string());
        }
    }
    return s;
}

void CloudStorageSettings::Save() const {
    auto* config = GetXLightsConfig();
    config->Write("CloudStorageProvider", provider);
    config->Write("CloudStorageR2AccountId", Trim(r2AccountId));
    config->Write("CloudStorageEndpoint", Trim(endpoint));
    config->Write("CloudStorageRegion", Trim(region));
    config->Write("CloudStorageBucket", Trim(bucket));
    config->Write("CloudStoragePrefix", Trim(prefix));
    config->Write("CloudStorageAccessKeyId", Trim(accessKeyId));
    config->Write("CloudStoragePathStyle", pathStyle);
    config->Write("CloudStorageIncludeFseq", includeFseq);
    config->Write("CloudStorageIncludeThumbnail", includeThumbnail);
    config->Write("CloudStorageKeepBackups", std::clamp(keepBackups, 0, MAX_KEEP_BACKUPS));
    const std::string secret = Trim(secretAccessKey);
    std::string reason;
    if (SecureStoreAvailable(&reason) && SaveToSecureStore(secret)) {
        config->DeleteEntry(SECRET_FALLBACK_KEY);
    } else {
        // Without a secure store the key would be lost and every backup would
        // fail with "secret access key is missing": keep it in the settings
        // file instead (the preferences page says so).
        spdlog::warn("Cloud storage: the system's secure storage is not available ({}), the secret key is saved in the settings file",
                     reason.empty() ? std::string("saving failed") : reason);
        config->Write(SECRET_FALLBACK_KEY, secret);
    }
    config->Flush();
}

std::string CloudStorageSettings::EffectiveEndpoint() const {
    if (IsR2()) {
        std::string id = Trim(r2AccountId);
        return id.empty() ? std::string() : S3Config::R2Endpoint(id);
    }
    return Trim(endpoint);
}

S3Config CloudStorageSettings::ToS3Config() const {
    S3Config c;
    c.endpoint = EffectiveEndpoint();
    c.region = IsR2() ? "auto" : Trim(region);
    c.bucket = Trim(bucket);
    c.accessKeyId = Trim(accessKeyId);
    c.secretAccessKey = Trim(secretAccessKey);
    c.pathStyle = IsR2() ? true : pathStyle;
    return c;
}

std::string CloudStorageSettings::Validate() const {
    if (IsR2() && Trim(r2AccountId).empty()) {
        return wxTRANSLATE("Cloudflare account ID is missing");
    }
    if (!IsR2()) {
        std::string ep = Trim(endpoint);
        if (ep.empty()) {
            return wxTRANSLATE("Endpoint URL is missing");
        }
        if (ep.rfind("https://", 0) != 0 && ep.rfind("http://", 0) != 0) {
            return wxTRANSLATE("Endpoint URL must start with https://");
        }
    }
    if (Trim(bucket).empty()) {
        return wxTRANSLATE("Bucket name is missing");
    }
    if (Trim(accessKeyId).empty()) {
        return wxTRANSLATE("Access key ID is missing");
    }
    if (Trim(secretAccessKey).empty()) {
        return wxTRANSLATE("Secret access key is missing");
    }
    return {};
}

/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "ReadyToPlay.h"

#include "CloudBackupStore.h"
#include "S3Client.h"
#include "string_utils.h"
#include "../render/FSEQFile.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

namespace ReadyToPlay {

std::string VersionsFolderKey(const std::string& prefix, const std::string& show) {
    return CloudBackupStore::ShowFolderKey(prefix, show) + "readytoplay/";
}

std::string VersionFolderKey(const std::string& prefix, const std::string& show, const std::string& stamp) {
    return VersionsFolderKey(prefix, show) + stamp + "/";
}

std::string EntryStem(size_t index, const std::string& name) {
    size_t slash = name.find_last_of("/\\");
    std::string n = slash == std::string::npos ? name : name.substr(slash + 1);
    std::string safe;
    for (unsigned char c : n) {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
        safe += ok ? (char)c : '_';
    }
    if (safe.empty() || safe.find_first_not_of('.') == std::string::npos) {
        safe = "sequence";
    }
    if (safe.size() > MAX_STEM_LENGTH - 3) {
        safe.resize(MAX_STEM_LENGTH - 3);
    }
    char num[8];
    snprintf(num, sizeof(num), "%02d-", (int)(index + 1));
    return num + safe;
}

namespace {

bool ReadRange(const std::string& source, uint32_t start, uint32_t channels,
               const std::function<void(uint32_t frame, const uint8_t* data)>& onFrame, FSEQFile*& header, std::string& error) {
    std::unique_ptr<FSEQFile> src(FSEQFile::openFSEQFile(source));
    if (!src) {
        error = "Unable to read " + source;
        return false;
    }
    std::vector<uint8_t> buf(std::max<uint64_t>((uint64_t)src->getMaxChannel(), (uint64_t)start + channels) + 1);
    src->prepareRead({ { start, channels } });
    for (uint32_t f = 0; f < src->getNumFrames(); f++) {
        std::fill(buf.begin(), buf.end(), 0);
        std::unique_ptr<FSEQFile::FrameData> fd(src->getFrame(f));
        if (fd && !fd->readFrame(buf.data(), (uint32_t)buf.size())) {
            error = source + " is corrupt at frame " + std::to_string(f);
            return false;
        }
        onFrame(f, buf.data());
    }
    header = src.release();
    return true;
}

} // namespace

bool WriteControllerFseq(const std::string& source, uint32_t startChannel, uint32_t channels, const std::string& dest,
                         std::string& error) {
    if (startChannel < 1 || channels == 0) {
        error = "No channels to write";
        return false;
    }
    const uint32_t start = startChannel - 1;

    // FNV-1a over the frames, so the header id only changes with the data.
    uint64_t id = 1469598103934665603ULL;
    auto mix = [&id](const uint8_t* p, size_t n) {
        for (size_t i = 0; i < n; i++) {
            id ^= p[i];
            id *= 1099511628211ULL;
        }
    };
    FSEQFile* first = nullptr;
    if (!ReadRange(source, start, channels, [&](uint32_t, const uint8_t* data) { mix(data + start, channels); }, first, error)) {
        return false;
    }
    std::unique_ptr<FSEQFile> header(first);
    uint32_t meta[3] = { header->getNumFrames() > UINT32_MAX ? UINT32_MAX : (uint32_t)header->getNumFrames(),
                         (uint32_t)header->getStepTime(), startChannel };
    mix((const uint8_t*)meta, sizeof(meta));
    if (id == 0) {
        id = 1;
    }

    std::unique_ptr<FSEQFile> out(FSEQFile::createFSEQFile(dest, 2, FSEQFile::CompressionType::zstd, 3));
    if (!out) {
        error = "Unable to create " + dest;
        return false;
    }
    out->initializeFromFSEQ(*header);
    out->setUniqueId(id);
    out->setChannelCount(start + channels);
    // The media path is the author's own disk; the boards get the audio from the manifest.
    out->removeVariableHeader('m', 'f');
    out->removeVariableHeader('X', 'S');
    out->removeVariableHeader('X', 'N');
    out->removeVariableHeader('X', 'R');
    static_cast<V2FSEQFile*>(out.get())->m_sparseRanges = { { start, channels } };
    out->writeHeader();

    FSEQFile* second = nullptr;
    bool ok = ReadRange(source, start, channels, [&](uint32_t f, const uint8_t* data) { out->addFrame(f, data); }, second, error);
    delete second;
    out->finalize();
    out.reset();
    if (!ok) {
        std::error_code ec;
        std::filesystem::remove(dest, ec);
    }
    return ok;
}

std::string BuildManifest(const Version& v) {
    nlohmann::json j;
    j["format"] = FORMAT;
    j["show"] = v.show;
    j["stamp"] = v.stamp;
    j["created"] = v.createdUtc;
    j["xlights"] = v.xlightsVersion;
    nlohmann::json playlist = nlohmann::json::array();
    for (const auto& e : v.playlist) {
        nlohmann::json je;
        je["name"] = e.name;
        je["frameMs"] = e.frameMs;
        je["durationMs"] = e.durationMs;
        if (!e.media.empty()) {
            je["media"] = e.media;
        }
        playlist.push_back(je);
    }
    j["playlist"] = playlist;
    nlohmann::json controllers = nlohmann::json::object();
    for (const auto& c : v.controllers) {
        nlohmann::json jc;
        if (!c.config.empty()) {
            jc["config"] = c.config;
        }
        jc["fseq"] = c.fseq;
        controllers[c.name] = jc;
    }
    j["controllers"] = controllers;
    nlohmann::json files = nlohmann::json::object();
    for (const auto& [path, info] : v.files) {
        files[path] = { { "size", info.size }, { "sha256", info.sha256 } };
    }
    j["files"] = files;
    return j.dump(2);
}

std::string BuildCurrentShow(const std::string& versionFolderKey, const std::string& show, const std::string& stamp,
                             const std::string& publishedUtc) {
    nlohmann::json j;
    j["format"] = FORMAT;
    j["version"] = versionFolderKey;
    j["show"] = show;
    j["stamp"] = stamp;
    j["published"] = publishedUtc;
    return j.dump(2);
}

CurrentShow ParseCurrentShow(const std::string& json) {
    CurrentShow c;
    nlohmann::json j = nlohmann::json::parse(json, nullptr, false);
    if (!j.is_object()) {
        return c;
    }
    auto str = [&j](const char* key) {
        auto it = j.find(key);
        return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
    };
    c.version = str("version");
    c.show = str("show");
    c.stamp = str("stamp");
    c.publishedUtc = str("published");
    c.ok = !c.version.empty() && !c.stamp.empty();
    return c;
}

std::vector<std::string> CompleteVersions(const std::vector<S3Object>& objects, const std::string& versionsFolder) {
    std::vector<std::string> stamps;
    const std::string suffix = std::string("/") + MANIFEST;
    for (const auto& o : objects) {
        if (!StartsWith(o.key, versionsFolder)) {
            continue;
        }
        std::string rest = o.key.substr(versionsFolder.size());
        if (rest.size() > suffix.size() && EndsWith(rest, suffix) && rest.find('/') == rest.size() - suffix.size()) {
            stamps.push_back(rest.substr(0, rest.size() - suffix.size()));
        }
    }
    std::sort(stamps.begin(), stamps.end(), std::greater<std::string>());
    return stamps;
}

std::vector<std::string> SelectForPruning(const std::vector<std::string>& stamps, int keep, const std::set<std::string>& protect) {
    std::vector<std::string> out;
    if (keep <= 0) {
        return out;
    }
    std::vector<std::string> sorted = stamps;
    std::sort(sorted.begin(), sorted.end(), std::greater<std::string>());
    int kept = 0;
    for (const auto& s : sorted) {
        if (protect.count(s)) {
            kept++;
        }
    }
    for (const auto& s : sorted) {
        if (protect.count(s)) {
            continue;
        }
        if (kept < keep) {
            kept++;
        } else {
            out.push_back(s);
        }
    }
    return out;
}

bool DeleteVersion(const CloudObjectOps& ops, const std::vector<S3Object>& objects, const std::string& versionFolder,
                   std::string& error) {
    const std::string manifest = versionFolder + MANIFEST;
    bool hasManifest = std::any_of(objects.begin(), objects.end(), [&](const S3Object& o) { return o.key == manifest; });
    if (hasManifest && !ops.remove(manifest, error)) {
        return false;
    }
    for (const auto& o : objects) {
        if (o.key != manifest && StartsWith(o.key, versionFolder) && !ops.remove(o.key, error)) {
            return false;
        }
    }
    return true;
}

} // namespace ReadyToPlay

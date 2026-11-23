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

// What cloud boards play: a version of a show published next to its backup,
//   <prefix>/shows/<show>/readytoplay/<stamp>/
//     media/<NN>-<name>.mp3
//     controllers/<controller>/config.json
//     controllers/<controller>/<NN>-<name>.fseq   (only that controller's channels)
//     manifest.json                               (written last)
// and currentshow.json at the bucket root, naming the version to play.
// See plans/xlights-cloud-readytoplay.md for the format the boards rely on.

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

struct CloudObjectOps;
struct S3Object;

namespace ReadyToPlay {

constexpr int FORMAT = 1;
constexpr const char* CURRENT_SHOW_KEY = "currentshow.json";
constexpr const char* MANIFEST = "manifest.json";
// What the boards accept.
constexpr size_t MAX_PLAYLIST = 32;
constexpr size_t MAX_NAME_BYTES = 64;
constexpr size_t MAX_STEM_LENGTH = 60;

// "<prefix>/shows/<show>/readytoplay/"
std::string VersionsFolderKey(const std::string& prefix, const std::string& show);
// "<prefix>/shows/<show>/readytoplay/<stamp>/"
std::string VersionFolderKey(const std::string& prefix, const std::string& show, const std::string& stamp);

// "01-Jingle_Bells": the position in the playlist and the name reduced to
// [A-Za-z0-9._-], at most MAX_STEM_LENGTH characters.
std::string EntryStem(size_t index, const std::string& name);

// Writes the channels [startChannel, startChannel + channels) (1-based) of a
// rendered sequence as a zstd FSEQ v2 file with that one sparse range, so the
// channel numbers stay those of the show. The same input always gives the same
// bytes: the header id is derived from the data and the media path is left out.
bool WriteControllerFseq(const std::string& source, uint32_t startChannel, uint32_t channels, const std::string& dest,
                         std::string& error);

struct FileInfo {
    uint64_t size = 0;
    std::string sha256;
};

struct Entry {
    std::string name;
    int frameMs = 0;
    uint64_t durationMs = 0;
    std::string media; // path in the version, empty when the sequence has no audio
};

struct Controller {
    std::string name;
    std::string config;             // path in the version, empty when there is none
    std::vector<std::string> fseq;  // one per playlist entry
};

struct Version {
    std::string show;
    std::string stamp;
    std::string createdUtc;
    std::string xlightsVersion;
    std::vector<Entry> playlist;
    std::vector<Controller> controllers;
    std::map<std::string, FileInfo> files; // every file of the version but the manifest
};

std::string BuildManifest(const Version& version);

struct CurrentShow {
    bool ok = false;
    std::string version; // the version's folder key
    std::string show;
    std::string stamp;
    std::string publishedUtc;
};

std::string BuildCurrentShow(const std::string& versionFolderKey, const std::string& show, const std::string& stamp,
                             const std::string& publishedUtc);
CurrentShow ParseCurrentShow(const std::string& json);

// Stamps of the complete versions (manifest present) in a listing of
// versionsFolder, newest first.
std::vector<std::string> CompleteVersions(const std::vector<S3Object>& objects, const std::string& versionsFolder);

// The versions to delete so that the newest keep are left. keep <= 0 keeps
// everything; protect is never selected and counts as kept when present.
std::vector<std::string> SelectForPruning(const std::vector<std::string>& stamps, int keep, const std::set<std::string>& protect);

// Deletes every object of a version, the manifest first so a partly deleted
// version is never complete. Stops at the first failure.
bool DeleteVersion(const CloudObjectOps& ops, const std::vector<S3Object>& objects, const std::string& versionFolder,
                   std::string& error);

} // namespace ReadyToPlay

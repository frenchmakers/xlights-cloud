#include <catch2/catch_test_macros.hpp>

#include "render/FSEQFile.h"
#include "support/TempDir.h"
#include "utils/CloudBackupStore.h"
#include "utils/ReadyToPlay.h"
#include "utils/ShowArchiver.h"
#include "utils/S3Client.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

uint8_t Pattern(uint32_t frame, uint32_t ch, uint8_t seed) {
    return (uint8_t)((frame * 31 + ch * 7 + seed) & 0xFF);
}

// A rendered sequence: 120 channels, 8 frames of 25 ms, with an "mf" header.
std::string WriteSource(const TempDir& dir, const std::string& name, uint8_t seed = 3) {
    std::string path = dir.File(name);
    std::unique_ptr<FSEQFile> f(FSEQFile::createFSEQFile(path, 2, FSEQFile::CompressionType::zstd, 1));
    REQUIRE(f);
    f->setChannelCount(120);
    f->setStepTime(25);
    f->setNumFrames(8);
    FSEQFile::VariableHeader mf;
    mf.code[0] = 'm';
    mf.code[1] = 'f';
    std::string media = "C:/Users/someone/Music/song.mp3";
    mf.data.assign(media.begin(), media.end());
    mf.data.push_back(0);
    f->addVariableHeader(mf);
    f->writeHeader();
    std::vector<uint8_t> frame(120);
    for (uint32_t fr = 0; fr < 8; fr++) {
        for (uint32_t c = 0; c < 120; c++) {
            frame[c] = Pattern(fr, c, seed);
        }
        f->addFrame(fr, frame.data());
    }
    f->finalize();
    return path;
}

std::string ReadBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

S3Object Obj(const std::string& key) {
    S3Object o;
    o.key = key;
    o.size = 1;
    return o;
}

} // namespace

TEST_CASE("Ready to play keys and file names", "[cloud][readytoplay]") {
    CHECK(ReadyToPlay::VersionsFolderKey("xlights", "My Show") == "xlights/shows/My Show/readytoplay/");
    CHECK(ReadyToPlay::VersionFolderKey("xlights", "My Show", "2026-11-23-101500") ==
          "xlights/shows/My Show/readytoplay/2026-11-23-101500/");
    CHECK(ReadyToPlay::EntryStem(0, "Vive le vent") == "01-Vive_le_vent");
    CHECK(ReadyToPlay::EntryStem(11, "Songs/Noël 2026") == "12-No__l_2026");
    CHECK(ReadyToPlay::EntryStem(1, "..") == "02-sequence");
    CHECK(ReadyToPlay::EntryStem(2, std::string(200, 'a')).size() == ReadyToPlay::MAX_STEM_LENGTH);
}

TEST_CASE("A controller's fseq keeps only its channels, with the show's numbers", "[cloud][readytoplay]") {
    TempDir dir;
    std::string source = WriteSource(dir, "song.fseq");
    std::string dest = dir.File("ctrl.fseq");
    std::string error;
    REQUIRE(ReadyToPlay::WriteControllerFseq(source, 31, 60, dest, error));

    std::unique_ptr<FSEQFile> f(FSEQFile::openFSEQFile(dest));
    REQUIRE(f);
    auto* v2 = dynamic_cast<V2FSEQFile*>(f.get());
    REQUIRE(v2 != nullptr);
    REQUIRE(v2->m_sparseRanges.size() == 1);
    CHECK(v2->m_sparseRanges[0].first == 30);
    CHECK(v2->m_sparseRanges[0].second == 60);
    CHECK(f->getNumFrames() == 8);
    CHECK(f->getStepTime() == 25);
    CHECK(f->getUniqueId() != 0);
    for (const auto& h : f->getVariableHeaders()) {
        CHECK_FALSE((h.code[0] == 'm' && h.code[1] == 'f'));
    }

    std::vector<uint8_t> buf(f->getMaxChannel() + 1);
    f->prepareRead({ { 30, 60 } });
    for (uint32_t fr = 0; fr < 8; fr++) {
        std::unique_ptr<FSEQFile::FrameData> fd(f->getFrame(fr));
        REQUIRE(fd);
        REQUIRE(fd->readFrame(buf.data(), (uint32_t)buf.size()));
        for (uint32_t c = 30; c < 90; c++) {
            INFO("frame " << fr << " channel " << c);
            REQUIRE(buf[c] == Pattern(fr, c, 3));
        }
    }
}

TEST_CASE("A controller's fseq is the same bytes for the same data", "[cloud][readytoplay]") {
    TempDir dir;
    std::string error;
    std::string source = WriteSource(dir, "song.fseq");
    REQUIRE(ReadyToPlay::WriteControllerFseq(source, 1, 90, dir.File("a.fseq"), error));
    // rendered again: a new header id and timestamp in the source, same channel data
    std::string again = WriteSource(dir, "song2.fseq");
    REQUIRE(ReadyToPlay::WriteControllerFseq(again, 1, 90, dir.File("b.fseq"), error));
    CHECK(ReadBytes(dir.File("a.fseq")) == ReadBytes(dir.File("b.fseq")));

    std::string changed = WriteSource(dir, "song3.fseq", 9);
    REQUIRE(ReadyToPlay::WriteControllerFseq(changed, 1, 90, dir.File("c.fseq"), error));
    std::unique_ptr<FSEQFile> a(FSEQFile::openFSEQFile(dir.File("a.fseq")));
    std::unique_ptr<FSEQFile> c(FSEQFile::openFSEQFile(dir.File("c.fseq")));
    REQUIRE(a);
    REQUIRE(c);
    CHECK(a->getUniqueId() != c->getUniqueId());
}

TEST_CASE("A controller's fseq refuses an unreadable source", "[cloud][readytoplay]") {
    TempDir dir;
    std::string error;
    CHECK_FALSE(ReadyToPlay::WriteControllerFseq(dir.File("missing.fseq"), 1, 30, dir.File("out.fseq"), error));
    CHECK_FALSE(error.empty());
    CHECK_FALSE(ReadyToPlay::WriteControllerFseq(dir.File("missing.fseq"), 0, 30, dir.File("out.fseq"), error));
}

TEST_CASE("Ready to play manifest lists the playlist, controllers and files", "[cloud][readytoplay]") {
    ReadyToPlay::Version v;
    v.show = "Noel";
    v.stamp = "2026-11-23-101500";
    v.createdUtc = "2026-11-23T10:15:00Z";
    v.xlightsVersion = "2026.21-cloud";
    v.playlist.push_back({ "Vive le vent", 25, 15000, "media/01-Vive_le_vent.mp3" });
    v.playlist.push_back({ "Silence", 50, 1000, "" });
    v.controllers.push_back({ "frenchmakers", "controllers/frenchmakers/config.json",
                              { "controllers/frenchmakers/01-Vive_le_vent.fseq", "controllers/frenchmakers/02-Silence.fseq" } });
    v.controllers.push_back({ "fpp", "", { "controllers/fpp/01-Vive_le_vent.fseq", "controllers/fpp/02-Silence.fseq" } });
    v.files["media/01-Vive_le_vent.mp3"] = { 240000, std::string(64, 'a') };

    auto j = nlohmann::json::parse(ReadyToPlay::BuildManifest(v));
    CHECK(j["format"] == 1);
    CHECK(j["show"] == "Noel");
    CHECK(j["stamp"] == "2026-11-23-101500");
    REQUIRE(j["playlist"].size() == 2);
    CHECK(j["playlist"][0]["media"] == "media/01-Vive_le_vent.mp3");
    CHECK(j["playlist"][0]["frameMs"] == 25);
    CHECK_FALSE(j["playlist"][1].contains("media"));
    CHECK(j["controllers"]["frenchmakers"]["config"] == "controllers/frenchmakers/config.json");
    CHECK(j["controllers"]["frenchmakers"]["fseq"].size() == 2);
    CHECK_FALSE(j["controllers"]["fpp"].contains("config"));
    CHECK(j["files"]["media/01-Vive_le_vent.mp3"]["size"] == 240000);
    CHECK(j["files"]["media/01-Vive_le_vent.mp3"]["sha256"] == std::string(64, 'a'));
}

TEST_CASE("currentshow.json round trip", "[cloud][readytoplay]") {
    std::string text = ReadyToPlay::BuildCurrentShow("xlights/shows/Noel/readytoplay/2026-11-23-101500/", "Noel",
                                                     "2026-11-23-101500", "2026-11-23T10:20:00Z");
    auto c = ReadyToPlay::ParseCurrentShow(text);
    CHECK(c.ok);
    CHECK(c.version == "xlights/shows/Noel/readytoplay/2026-11-23-101500/");
    CHECK(c.show == "Noel");
    CHECK(c.stamp == "2026-11-23-101500");
    CHECK(c.publishedUtc == "2026-11-23T10:20:00Z");
    CHECK_FALSE(ReadyToPlay::ParseCurrentShow("not json").ok);
    CHECK_FALSE(ReadyToPlay::ParseCurrentShow("{\"show\":\"Noel\"}").ok);
}

TEST_CASE("Only complete ready to play versions are listed, newest first", "[cloud][readytoplay]") {
    const std::string folder = "x/shows/S/readytoplay/";
    std::vector<S3Object> objects = {
        Obj(folder + "2026-01-01-000000/manifest.json"),
        Obj(folder + "2026-01-01-000000/media/01-a.mp3"),
        Obj(folder + "2026-03-01-000000/controllers/c/config.json"), // no manifest: incomplete
        Obj(folder + "2026-02-01-000000/manifest.json"),
        Obj(folder + "2026-02-01-000000/controllers/c/manifest.json"),
        Obj("x/shows/S/source/2026-04-01-000000.manifest.json"),
    };
    auto stamps = ReadyToPlay::CompleteVersions(objects, folder);
    REQUIRE(stamps.size() == 2);
    CHECK(stamps[0] == "2026-02-01-000000");
    CHECK(stamps[1] == "2026-01-01-000000");
}

TEST_CASE("Pruning ready to play versions never removes protected ones", "[cloud][readytoplay]") {
    std::vector<std::string> stamps = { "1", "2", "3", "4", "5" };
    CHECK(ReadyToPlay::SelectForPruning(stamps, 0, {}).empty());
    CHECK(ReadyToPlay::SelectForPruning(stamps, 2, {}) == std::vector<std::string>{ "3", "2", "1" });
    // the new version and the one being played are kept, and count as kept
    CHECK(ReadyToPlay::SelectForPruning(stamps, 2, { "5", "1" }) == std::vector<std::string>{ "4", "3", "2" });
    CHECK(ReadyToPlay::SelectForPruning(stamps, 3, { "5", "1" }) == std::vector<std::string>{ "3", "2" });
    CHECK(ReadyToPlay::SelectForPruning(stamps, 2, { "9" }) == std::vector<std::string>{ "3", "2", "1" });
}

TEST_CASE("Deleting a ready to play version removes the manifest first", "[cloud][readytoplay]") {
    const std::string v = "x/shows/S/readytoplay/2026-01-01-000000/";
    std::vector<S3Object> objects = { Obj(v + "media/01-a.mp3"), Obj(v + "manifest.json"), Obj(v + "controllers/c/01-a.fseq"),
                                      Obj("x/shows/S/readytoplay/2026-02-01-000000/manifest.json") };
    std::vector<std::string> removed;
    CloudObjectOps ops;
    ops.remove = [&](const std::string& key, std::string&) {
        removed.push_back(key);
        return true;
    };
    std::string error;
    REQUIRE(ReadyToPlay::DeleteVersion(ops, objects, v, error));
    REQUIRE(removed.size() == 3);
    CHECK(removed[0] == v + "manifest.json");
    CHECK(std::set<std::string>(removed.begin(), removed.end()).count("x/shows/S/readytoplay/2026-02-01-000000/manifest.json") == 0);

    removed.clear();
    ops.remove = [&](const std::string& key, std::string& e) {
        removed.push_back(key);
        e = "denied";
        return false;
    };
    CHECK_FALSE(ReadyToPlay::DeleteVersion(ops, objects, v, error));
    CHECK(removed.size() == 1);
    CHECK(error == "denied");
}

TEST_CASE("Sequence names for the playlist come from the show folder", "[cloud][readytoplay]") {
    TempDir dir;
    std::filesystem::create_directories(dir.path / "sub");
    std::filesystem::create_directories(dir.path / "Backup");
    dir.Write("b.xsq", "x");
    dir.Write("sub/a.xsq", "x");
    dir.Write("Backup/old.xsq", "x");
    dir.Write("notes.txt", "x");
    ShowArchiveOptions options;
    options.showDir = dir.path.string();
    auto names = ShowArchiver::ListSequenceNames(options);
    CHECK(names == std::vector<std::string>{ "b", "sub/a" });
}

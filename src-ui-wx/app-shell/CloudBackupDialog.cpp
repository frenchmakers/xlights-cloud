/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "CloudBackupDialog.h"

#include "layout/ModelPreview.h"
#include "render/FSEQFile.h"
#include "utils/CloudBackupStore.h"
#include "utils/ReadyToPlay.h"
#include "utils/Sha256.h"
#include "utils/string_utils.h"
#include "utils/S3Client.h"

#include "settings/XLightsConfigAdapter.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/rearrangectrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <algorithm>
#include <future>
#include <memory>
#include <set>

namespace fs = std::filesystem;

std::string CloudFormatBytes(uint64_t b) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double v = (double)b;
    int u = 0;
    while (v >= 1024.0 && u < 4) {
        v /= 1024.0;
        ++u;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), u == 0 ? "%.0f %s" : "%.1f %s", v, units[u]);
    return buf;
}

namespace {

std::string FormatDuration(double secs) {
    long s = (long)(secs + 0.5);
    char buf[32];
    if (s >= 3600) {
        snprintf(buf, sizeof(buf), "%ldh%02ld", s / 3600, (s % 3600) / 60);
    } else if (s >= 60) {
        snprintf(buf, sizeof(buf), "%ld min %02ld s", s / 60, s % 60);
    } else {
        snprintf(buf, sizeof(buf), "%ld s", s);
    }
    return buf;
}

// Deletes the backups of folder beyond the newest keep, never the one just
// uploaded. A failure only adds a warning: the new backup is already safe.
std::string PruneOldBackups(S3Client& client, const std::string& folder, const std::string& newStamp, int keep,
                            std::vector<std::string>& warnings) {
    std::vector<S3Object> objects;
    std::vector<std::string> unused;
    std::string error;
    if (!client.ListObjects(folder, "", objects, unused, error)) {
        warnings.push_back("Older backups were not deleted: unable to list them: " + error);
        return {};
    }
    auto all = CloudBackupStore::FromListing(objects, folder);
    auto old = CloudBackupStore::SelectForPruning(all, keep, newStamp);
    if (old.empty()) {
        return {};
    }
    if (!CloudBackupStore::Delete(CloudObjectOps::For(client), folder, all, old, error)) {
        warnings.push_back("Some older backups could not be deleted: " + error);
        return {};
    }
    spdlog::info("Cloud backup: deleted {} older backup(s) of {}", old.size(), folder);
    return "Deleted " + std::to_string(old.size()) + " older backup(s), keeping the last " + std::to_string(keep) + ".";
}

fs::path FromUtf8(const std::string& s) {
    return fs::path(std::u8string(s.begin(), s.end()));
}

std::string ToUtf8(const fs::path& p) {
    auto u = p.u8string();
    return std::string(u.begin(), u.end());
}

} // namespace

std::string CaptureLayoutThumbnailPng(ModelPreview* preview, int maxWidth, std::string& warning) {
    if (preview == nullptr) {
        warning = "No thumbnail: the layout preview is not available.";
        return {};
    }
    std::unique_ptr<wxImage> img(preview->GrabImage());
    if (!img || !img->IsOk() || img->GetWidth() < 16 || img->GetHeight() < 16) {
        warning = "No thumbnail: the layout preview could not be captured. Open the Layout tab once, then back up again.";
        return {};
    }
    if (img->GetWidth() > maxWidth) {
        int h = std::max(1, img->GetHeight() * maxWidth / img->GetWidth());
        img->Rescale(maxWidth, h, wxIMAGE_QUALITY_HIGH);
    }
    wxMemoryOutputStream out;
    if (!img->SaveFile(out, wxBITMAP_TYPE_PNG)) {
        warning = "No thumbnail: the PNG could not be encoded.";
        return {};
    }
    std::string png(out.GetLength(), '\0');
    out.CopyTo(png.data(), png.size());
    return png;
}

CloudBackupDialog::CloudBackupDialog(wxWindow* parent, CloudBackupJob job) :
    wxDialog(parent, wxID_ANY, _("Backup to Cloud"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    _job(std::move(job)),
    _timer(this) {
    auto* top = new wxBoxSizer(wxVERTICAL);
    _stageText = new wxStaticText(this, wxID_ANY, _("Preparing..."));
    top->Add(_stageText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    _gauge = new wxGauge(this, wxID_ANY, 1000, wxDefaultPosition, wxSize(460, -1));
    top->Add(_gauge, 0, wxEXPAND | wxALL, 10);
    _detail = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
    top->Add(_detail, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    _log = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 90), wxTE_MULTILINE | wxTE_READONLY);
    _log->Hide();
    top->Add(_log, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    _button = new wxButton(this, wxID_CANCEL, _("Cancel"));
    top->Add(_button, 0, wxALIGN_RIGHT | wxALL, 10);
    SetSizerAndFit(top);
    SetEscapeId(wxID_NONE);

    Bind(wxEVT_TIMER, &CloudBackupDialog::OnTimer, this);
    _button->Bind(wxEVT_BUTTON, &CloudBackupDialog::OnButton, this);
    Bind(wxEVT_CLOSE_WINDOW, &CloudBackupDialog::OnCloseWindow, this);
}

CloudBackupDialog::~CloudBackupDialog() {
    _timer.Stop();
    _cancel = true;
    if (_thread.joinable()) {
        _thread.join();
    }
}

void CloudBackupDialog::Start() {
    _running = true;
    _startTime = std::chrono::steady_clock::now();
    _stageStart = _startTime;
    _timer.Start(250);
    _thread = std::thread([this]() { Run(); });
}

void CloudBackupDialog::CancelAndWait() {
    _cancel = true;
    if (_thread.joinable()) {
        _thread.join();
    }
    _running = false;
}

void CloudBackupDialog::SetCurrent(const std::string& s) {
    std::lock_guard<std::mutex> l(_currentLock);
    _current = s;
}

bool CloudBackupDialog::AskContinueWithStaleFseq(const std::vector<std::string>& names) {
    auto answer = std::make_shared<std::promise<bool>>();
    std::future<bool> result = answer->get_future();

    std::string list;
    size_t shown = 0;
    for (const auto& n : names) {
        if (shown++ == 10) {
            list += "  ... and " + std::to_string(names.size() - 10) + " more\n";
            break;
        }
        list += "  " + n + "\n";
    }
    _waitingForAnswer = true;
    CallAfter([this, answer, list, count = names.size()]() {
        wxString intro = count == 1
                             ? wxString("This sequence has no .fseq file, or its .fseq is older than the sequence:")
                             : wxString::Format("These %d sequences have no .fseq file, or their .fseq is older than the sequence:", (int)count);
        wxString msg = intro + "\n\n" + wxString::FromUTF8(list) +
                       "\nRendering alone does not write the .fseq: save the sequence after rendering, "
                       "or use Tools > Batch Render.\n\nBack up without publishing a ready to play version?";
        wxWindow* parent = IsShown() ? (wxWindow*)this : GetParent();
        int r = wxMessageBox(msg, _("Backup to Cloud"), wxYES_NO | wxICON_WARNING, parent);
        answer->set_value(r == wxYES);
    });

    bool cancelled = false;
    while (result.wait_for(std::chrono::milliseconds(200)) != std::future_status::ready) {
        if (_cancel) {
            cancelled = true;
            break;
        }
    }
    _waitingForAnswer = false;
    return !cancelled && result.get();
}

void CloudBackupDialog::Run() {
    const auto& settings = _job.settings;
    const time_t now = time(nullptr);

    _stage = SCANNING;
    ShowArchivePlan plan = ShowArchiver::Scan(_job.archive);
    if (plan.entries.empty()) {
        Finish(false, false, "Nothing to back up in " + _job.archive.showDir, plan.warnings);
        return;
    }
    if (!_job.thumbnailPng.empty()) {
        plan.extraFiles.push_back({ "thumbnail.png", _job.thumbnailPng });
    }
    if (!_job.thumbnailWarning.empty()) {
        plan.warnings.push_back(_job.thumbnailWarning);
    }
    // The sequences to publish, each with an up to date .fseq.
    std::vector<const ShowArchiveSequence*> published;
    bool publish = _job.publishReadyToPlay && !_job.playlist.empty();
    if (publish) {
        std::vector<std::string> notReady;
        for (const auto& name : _job.playlist) {
            auto it = std::find_if(plan.sequences.begin(), plan.sequences.end(), [&](const ShowArchiveSequence& s) { return s.name == name; });
            if (it == plan.sequences.end() || it->fseqPath.empty() || !it->fseqUpToDate) {
                notReady.push_back(name);
            } else {
                published.push_back(&*it);
            }
        }
        if (!notReady.empty()) {
            if (!AskContinueWithStaleFseq(notReady)) {
                Finish(false, true, "Backup cancelled.", {});
                return;
            }
            publish = false;
            plan.warnings.push_back("No ready to play version was published: some sequences have no up to date .fseq.");
        }
    }

    std::error_code ec;
    fs::path tmpDir = fs::temp_directory_path(ec);
    fs::path zipPath = tmpDir / ("xlights-cloud-backup-" + ShowArchiver::FormatUtcForKey(now) + ".zip");
    auto space = fs::space(tmpDir, ec);
    if (!ec && space.available < plan.totalBytes + 64ull * 1024 * 1024) {
        Finish(false, false,
               "Not enough free disk space in " + ToUtf8(tmpDir) + " for the archive: " +
                   CloudFormatBytes(plan.totalBytes) + " needed, " + CloudFormatBytes(space.available) + " available.",
               {});
        return;
    }

    _stage = COMPRESSING;
    _total = plan.totalBytes;
    _done = 0;
    ShowArchiveResult archive = ShowArchiver::Create(plan, zipPath, [this](uint64_t done, uint64_t total, const std::string& file) {
        _done = done;
        _total = total;
        SetCurrent(file);
        return !_cancel;
    });
    if (!archive.ok) {
        Finish(false, archive.cancelled, archive.cancelled ? "Backup cancelled." : archive.error, archive.warnings);
        return;
    }

    std::string showName = ToUtf8(FromUtf8(_job.archive.showDir).lexically_normal().filename());
    std::string folder = CloudBackupStore::SourceFolderKey(settings.prefix, showName);
    const std::string stamp = ShowArchiver::FormatUtcForKey(now);
    std::string zipKey = folder + stamp + ".zip";

    _stage = UPLOADING;
    _total = archive.archiveSize;
    _done = 0;
    SetCurrent(zipKey);
    S3Client client(settings.ToS3Config());
    std::string error;
    std::string sha;
    bool cancelled = false;
    bool ok = client.UploadFile(zipKey, zipPath, "application/zip", [this](uint64_t sent, uint64_t total) {
        _done = sent;
        _total = total;
        return !_cancel;
    }, error, &sha, &cancelled);
    fs::remove(zipPath, ec);
    if (!ok) {
        Finish(false, cancelled, cancelled ? "Backup cancelled." : "Upload failed: " + error, archive.warnings);
        return;
    }

    _stage = FINISHING;
    std::string thumbnailKey;
    if (!_job.thumbnailPng.empty()) {
        std::string key = folder + stamp + ".thumbnail.png";
        if (client.PutObject(key, _job.thumbnailPng, "image/png", error)) {
            thumbnailKey = key;
        } else {
            archive.warnings.push_back("The thumbnail could not be uploaded: " + error);
        }
    }
    ShowArchiver::ManifestInfo info;
    info.showName = showName;
    info.createdUtc = ShowArchiver::FormatUtcIso(now);
    info.archiveKey = zipKey;
    info.archiveSize = archive.archiveSize;
    info.archiveSha256 = sha;
    info.thumbnailKey = thumbnailKey;
    std::string manifest = ShowArchiver::BuildManifest(plan, info);
    std::string manifestKey = folder + stamp + ".manifest.json";
    if (!client.PutObject(manifestKey, manifest, "application/json", error) ||
        !client.PutObject(folder + "latest.manifest.json", manifest, "application/json", error)) {
        Finish(false, false, "The archive was uploaded but its manifest could not be written: " + error, archive.warnings);
        return;
    }

    spdlog::info("Cloud backup: uploaded {} ({} bytes)", zipKey, archive.archiveSize);
    std::string message = "Backup uploaded to " + settings.bucket + "/" + zipKey + " (" + CloudFormatBytes(archive.archiveSize) + ").";
    if (settings.keepBackups > 0) {
        std::string pruned = PruneOldBackups(client, folder, stamp, settings.keepBackups, archive.warnings);
        if (!pruned.empty()) {
            message += " " + pruned;
        }
    }
    if (publish) {
        _stage = PUBLISHING;
        std::string published_message;
        if (!PublishReadyToPlay(client, published, showName, stamp, now, archive.warnings, published_message, error)) {
            bool wasCancelled = _cancel;
            Finish(false, wasCancelled,
                   message + " " + (wasCancelled ? std::string("Publishing the ready to play version was cancelled.")
                                                 : "The ready to play version was not published: " + error),
                   archive.warnings);
            return;
        }
        message += " " + published_message;
    }
    Finish(true, false, message, archive.warnings);
}

bool CloudBackupDialog::PublishReadyToPlay(S3Client& client, const std::vector<const ShowArchiveSequence*>& sequences,
                                           const std::string& showName, const std::string& stamp, time_t now,
                                           std::vector<std::string>& warnings, std::string& message, std::string& error) {
    const auto& settings = _job.settings;
    const std::string folder = ReadyToPlay::VersionFolderKey(settings.prefix, showName, stamp);
    std::vector<std::string> sent;
    auto removeSent = [&]() {
        std::string ignored;
        for (const auto& k : sent) {
            client.DeleteObject(k, ignored);
        }
    };
    std::error_code ec;
    fs::path tmp = fs::temp_directory_path(ec) / ("xlights-readytoplay-" + stamp);
    fs::create_directories(tmp, ec);
    struct TmpGuard {
        fs::path p;
        ~TmpGuard() {
            std::error_code e;
            fs::remove_all(p, e);
        }
    } guard{ tmp };

    std::vector<const CloudControllerConfig::File*> controllers;
    for (const auto& c : _job.controllerConfigs) {
        for (const auto& w : c.warnings) {
            warnings.push_back(c.controller + ": " + w);
        }
        if (!c.error.empty()) {
            warnings.push_back(c.controller + " is not in the ready to play version: " + c.error);
        } else {
            controllers.push_back(&c);
        }
    }
    if (controllers.empty()) {
        warnings.push_back("The ready to play version has no controller: no active FrenchMakers controller could be published.");
    }

    _total = sequences.size() * (controllers.size() + 1) + controllers.size() + 1;
    _done = 0;
    auto upload = [&](const std::string& path, const fs::path& file, const std::string& type, ReadyToPlay::FileInfo& info) {
        std::string key = folder + path;
        SetCurrent(path);
        bool cancelled = false;
        if (!client.UploadFile(key, file, type, [this](uint64_t, uint64_t) { return !_cancel; }, error, &info.sha256, &cancelled)) {
            if (cancelled) {
                error = "cancelled";
            }
            return false;
        }
        sent.push_back(key);
        info.size = fs::file_size(file, ec);
        return true;
    };

    ReadyToPlay::Version v;
    v.show = showName;
    v.stamp = stamp;
    v.createdUtc = ShowArchiver::FormatUtcIso(now);
    v.xlightsVersion = _job.xlightsVersion;
    for (size_t i = 0; i < sequences.size(); i++) {
        const ShowArchiveSequence* s = sequences[i];
        std::string display = s->name.substr(s->name.find_last_of('/') == std::string::npos ? 0 : s->name.find_last_of('/') + 1);
        if (display.size() > ReadyToPlay::MAX_NAME_BYTES) {
            size_t cut = ReadyToPlay::MAX_NAME_BYTES;
            while (cut > 0 && ((unsigned char)display[cut] & 0xC0) == 0x80) {
                cut--;
            }
            display.resize(cut);
        }
        ReadyToPlay::Entry e;
        e.name = display;
        {
            std::unique_ptr<FSEQFile> f(FSEQFile::openFSEQFile(ToUtf8(s->fseqPath)));
            if (!f) {
                error = "unable to read " + ToUtf8(s->fseqPath);
                removeSent();
                return false;
            }
            e.frameMs = f->getStepTime();
            e.durationMs = f->getTotalTimeMS();
        }
        const std::string stem = ReadyToPlay::EntryStem(i, display);
        if (!s->mediaPath.empty()) {
            if (::Lower(ToUtf8(s->mediaPath.extension())) == ".mp3") {
                ReadyToPlay::FileInfo info;
                std::string path = "media/" + stem + ".mp3";
                if (!upload(path, s->mediaPath, "audio/mpeg", info)) {
                    removeSent();
                    return false;
                }
                v.files[path] = info;
                e.media = path;
            } else {
                warnings.push_back("The audio of " + s->name + " is not an .mp3 file: it is published without audio.");
            }
        }
        _done = _done + 1;
        v.playlist.push_back(e);
    }

    for (const auto* c : controllers) {
        std::string key = CloudControllerConfig::ObjectKey(c->controller);
        std::string dir = key.substr(0, key.size() - 5) + "/"; // "controllers/<name>/"
        ReadyToPlay::Controller rc;
        rc.name = c->controller;
        rc.config = dir + "config.json";
        SetCurrent(rc.config);
        if (!client.PutObject(folder + rc.config, c->json, "application/json", error)) {
            removeSent();
            return false;
        }
        sent.push_back(folder + rc.config);
        v.files[rc.config] = { c->json.size(), Sha256::HexOf(c->json.data(), c->json.size()) };
        _done = _done + 1;
        for (size_t i = 0; i < sequences.size(); i++) {
            if (_cancel) {
                error = "cancelled";
                removeSent();
                return false;
            }
            for (const auto& w : CloudControllerConfig::TimingWarnings(c->timing, v.playlist[i].frameMs)) {
                warnings.push_back(c->controller + ", " + sequences[i]->name + ": " + w);
            }
            std::string path = dir + ReadyToPlay::EntryStem(i, v.playlist[i].name) + ".fseq";
            fs::path file = tmp / (std::to_string(i) + ".fseq");
            SetCurrent(path);
            if (!ReadyToPlay::WriteControllerFseq(ToUtf8(sequences[i]->fseqPath), c->startChannel, c->channels, ToUtf8(file), error)) {
                removeSent();
                return false;
            }
            ReadyToPlay::FileInfo info;
            if (!upload(path, file, "application/octet-stream", info)) {
                removeSent();
                return false;
            }
            fs::remove(file, ec);
            v.files[path] = info;
            rc.fseq.push_back(path);
            _done = _done + 1;
        }
        v.controllers.push_back(rc);
    }

    SetCurrent(ReadyToPlay::MANIFEST);
    if (!client.PutObject(folder + ReadyToPlay::MANIFEST, ReadyToPlay::BuildManifest(v), "application/json", error)) {
        removeSent();
        return false;
    }
    spdlog::info("Cloud backup: published ready to play version {}", folder);
    message = "Ready to play version published (" + std::to_string(v.playlist.size()) + " sequence(s), " +
              std::to_string(v.controllers.size()) + " controller(s)).";

    std::string currentText;
    std::string readError;
    ReadyToPlay::CurrentShow current;
    if (client.ReadObject(ReadyToPlay::CURRENT_SHOW_KEY, currentText, readError)) {
        current = ReadyToPlay::ParseCurrentShow(currentText);
    }
    if (_job.putOnAir) {
        std::string onAir = ReadyToPlay::BuildCurrentShow(folder, showName, stamp, ShowArchiver::FormatUtcIso(time(nullptr)));
        if (client.PutObject(ReadyToPlay::CURRENT_SHOW_KEY, onAir, "application/json", error)) {
            message += " It is now on air.";
            current = ReadyToPlay::ParseCurrentShow(onAir);
        } else {
            warnings.push_back("The version was published but could not be put on air: " + error);
        }
    }
    _done = _total.load();

    if (settings.keepBackups > 0) {
        const std::string versions = ReadyToPlay::VersionsFolderKey(settings.prefix, showName);
        std::vector<S3Object> objects;
        std::vector<std::string> unused;
        if (!client.ListObjects(versions, "", objects, unused, readError)) {
            warnings.push_back("Older ready to play versions were not deleted: unable to list them: " + readError);
            return true;
        }
        std::set<std::string> protect{ stamp };
        if (current.ok && StartsWith(current.version, versions)) {
            protect.insert(current.stamp);
        }
        auto old = ReadyToPlay::SelectForPruning(ReadyToPlay::CompleteVersions(objects, versions), settings.keepBackups, protect);
        for (const auto& o : old) {
            if (!ReadyToPlay::DeleteVersion(CloudObjectOps::For(client), objects, versions + o + "/", readError)) {
                warnings.push_back("An older ready to play version could not be deleted: " + readError);
                break;
            }
        }
    }
    return true;
}

void CloudBackupDialog::Finish(bool ok, bool cancelled, const std::string& message, const std::vector<std::string>& warnings) {
    if (!ok && !cancelled) {
        spdlog::error("Cloud backup failed: {}", message);
    }
    CallAfter([this, ok, cancelled, message, warnings]() {
        if (_thread.joinable()) {
            _thread.join();
        }
        _running = false;
        _timer.Stop();
        if (cancelled && !IsShown()) {
            // Declined at the .fseq prompt, before anything was shown.
            Destroy();
            return;
        }
        Show();
        _gauge->SetValue(ok ? _gauge->GetRange() : 0);
        _stageText->SetLabel(ok ? _("Backup complete") : (cancelled ? _("Backup cancelled") : _("Backup failed")));
        _detail->SetLabel(wxString::FromUTF8(message));
        _detail->SetToolTip(wxString::FromUTF8(message));
        if (!ok && !cancelled) {
            _detail->SetForegroundColour(*wxRED);
        }
        if (!warnings.empty()) {
            wxString text;
            for (const auto& w : warnings) {
                text += wxString::FromUTF8(w) + "\n";
            }
            _log->SetValue(text);
            _log->Show();
        }
        _button->SetLabel(_("Close"));
        Layout();
        Fit();
        RequestUserAttention();
    });
}

void CloudBackupDialog::OnTimer(wxTimerEvent& event) {
    int stage = _stage;
    // Stay hidden while the show is scanned and any question is pending, so the
    // progress bar never runs behind an unanswered prompt. A slow scan still
    // shows the window after a moment so the user knows something is happening.
    if (!IsShown()) {
        double waited = std::chrono::duration<double>(std::chrono::steady_clock::now() - _startTime).count();
        if (_waitingForAnswer || (stage == SCANNING && waited < 1.5)) {
            return;
        }
        Show();
    }
    if (_waitingForAnswer) {
        _stageText->SetLabel(_("Waiting for your answer..."));
        _gauge->SetValue(0);
        return;
    }
    if (stage != _shownStage) {
        _shownStage = stage;
        _stageStart = std::chrono::steady_clock::now();
        switch (stage) {
        case SCANNING:
            _stageText->SetLabel(_("Scanning the show folder..."));
            break;
        case COMPRESSING:
            _stageText->SetLabel(_("Step 1/2: building the archive"));
            break;
        case UPLOADING:
            _stageText->SetLabel(_("Step 2/2: uploading"));
            break;
        case PUBLISHING:
            _stageText->SetLabel(_("Publishing the ready to play version..."));
            break;
        default:
            _stageText->SetLabel(_("Writing the manifest..."));
            break;
        }
    }
    if (_cancel) {
        _stageText->SetLabel(_("Cancelling..."));
    }

    uint64_t done = _done;
    uint64_t total = _total;
    if (stage == SCANNING || total == 0) {
        _gauge->Pulse();
        return;
    }
    _gauge->SetValue((int)std::min<uint64_t>(1000, done * 1000 / total));

    std::string text = stage == PUBLISHING ? std::to_string(done) + " / " + std::to_string(total)
                                           : CloudFormatBytes(done) + " / " + CloudFormatBytes(total);
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - _stageStart).count();
    if (stage == UPLOADING && secs > 2.0 && done > 0) {
        double rate = done / secs;
        text += "  -  " + CloudFormatBytes((uint64_t)rate) + "/s";
        if (done < total) {
            text += "  -  about " + FormatDuration((total - done) / rate) + " left";
        }
    } else {
        std::lock_guard<std::mutex> l(_currentLock);
        if (!_current.empty()) {
            text += "  -  " + _current;
        }
    }
    _detail->SetLabel(wxString::FromUTF8(text));
}

bool CloudBackupDialog::ConfirmCancel() {
    if (!_running || _cancel) {
        return true;
    }
    if (wxMessageBox(_("Cancel the cloud backup in progress?"), _("Backup to Cloud"), wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        _cancel = true;
    }
    return false;
}

void CloudBackupDialog::OnButton(wxCommandEvent& event) {
    if (_running) {
        ConfirmCancel();
        return;
    }
    Destroy();
}

void CloudBackupDialog::OnCloseWindow(wxCloseEvent& event) {
    if (_running && event.CanVeto()) {
        ConfirmCancel();
        event.Veto();
        return;
    }
    Destroy();
}

bool ChooseCloudPlaylist(wxWindow* parent, CloudBackupJob& job) {
    std::vector<std::string> names = ShowArchiver::ListSequenceNames(job.archive);
    if (names.empty()) {
        job.publishReadyToPlay = false;
        return true;
    }

    // Remembered per show folder: {"<show dir>": {"playlist": [...], "onAir": bool}}
    auto* config = GetXLightsConfig();
    nlohmann::json saved = nlohmann::json::parse(config->Read("CloudPlaylists", std::string("{}")), nullptr, false);
    if (!saved.is_object()) {
        saved = nlohmann::json::object();
    }
    std::vector<std::string> previous;
    bool onAir = false;
    if (saved.contains(job.archive.showDir) && saved[job.archive.showDir].is_object()) {
        const auto& s = saved[job.archive.showDir];
        if (s.contains("playlist") && s["playlist"].is_array()) {
            for (const auto& n : s["playlist"]) {
                if (n.is_string()) {
                    previous.push_back(n.get<std::string>());
                }
            }
        }
        onAir = s.value("onAir", false);
    }

    wxArrayString items;
    for (const auto& n : names) {
        items.Add(wxString::FromUTF8(n));
    }
    wxArrayInt order;
    std::set<int> placed;
    for (const auto& p : previous) {
        auto it = std::find(names.begin(), names.end(), p);
        if (it != names.end() && !placed.count((int)(it - names.begin()))) {
            int idx = (int)(it - names.begin());
            order.Add(idx);
            placed.insert(idx);
        }
    }
    for (int i = 0; i < (int)names.size(); i++) {
        if (!placed.count(i)) {
            order.Add(~i);
        }
    }

    while (true) {
        wxRearrangeDialog dlg(parent,
                              wxString::Format(_("Check the sequences to publish for the cloud boards and put them in playing order (%d at most).\n"
                                                 "Uncheck them all to back up without a ready to play version."),
                                               (int)ReadyToPlay::MAX_PLAYLIST),
                              _("Ready to Play"), order, items);
        auto* extra = new wxPanel(&dlg);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* onAirBox = new wxCheckBox(extra, wxID_ANY, _("Put this version on air when the backup is done"));
        onAirBox->SetValue(onAir);
        onAirBox->SetToolTip(_("The cloud boards play it at their next synchronization. Otherwise, put it on air later\n"
                               "from File > Restore from Cloud."));
        sizer->Add(onAirBox, 0, wxALL, 4);
        extra->SetSizer(sizer);
        dlg.AddExtraControls(extra);
        if (dlg.ShowModal() != wxID_OK) {
            return false;
        }
        order = dlg.GetOrder();
        onAir = onAirBox->GetValue();
        std::vector<std::string> playlist;
        for (int o : order) {
            if (o >= 0) {
                playlist.push_back(names[o]);
            }
        }
        if (playlist.size() > ReadyToPlay::MAX_PLAYLIST) {
            wxMessageBox(wxString::Format(_("The cloud boards play %d sequences at most; %d are checked."), (int)ReadyToPlay::MAX_PLAYLIST,
                                          (int)playlist.size()),
                         _("Ready to Play"), wxOK | wxICON_WARNING, parent);
            continue;
        }
        saved[job.archive.showDir] = { { "playlist", playlist }, { "onAir", onAir } };
        config->Write("CloudPlaylists", saved.dump());
        job.playlist = playlist;
        job.publishReadyToPlay = !playlist.empty();
        job.putOnAir = onAir && job.publishReadyToPlay;
        return true;
    }
}

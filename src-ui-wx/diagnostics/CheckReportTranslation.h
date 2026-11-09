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

// Translates the fixed frame of the Check Sequence HTML report (titles,
// buttons, section names, "No ... found" lines) after src-core generated it.
// Issue messages stay in English: the report groups them by parsing their
// English text.

#include <wx/translation.h>

#include <cstdlib>
#include <set>
#include <string>

namespace CheckReportTranslation {

inline const std::set<std::string>& Texts() {
    static const std::set<std::string> texts = {
        // Header, filters and navigation
        wxTRANSLATE("xLights Show Status Report"),
        "Show Folder", // translated as "Show folder": the preferences read "Show Folder" back
        wxTRANSLATE("Sequence"),
        wxTRANSLATE("No sequence was loaded when the report ran"),
        wxTRANSLATE("Report Generated"),
        wxTRANSLATE("Total Errors"),
        wxTRANSLATE("Total Warnings"),
        wxTRANSLATE("Search report..."),
        wxTRANSLATE("Errors"),
        wxTRANSLATE("Warnings"),
        wxTRANSLATE("Show/hide this section"),
        wxTRANSLATE("Expand All"),
        wxTRANSLATE("Collapse All"),
        wxTRANSLATE("Reset All"),
        wxTRANSLATE("Toggle theme"),
        wxTRANSLATE("Previous issue"),
        wxTRANSLATE("Previous issue (Shift+N)"),
        wxTRANSLATE("Next issue"),
        wxTRANSLATE("Next issue (N)"),
        // Sections
        wxTRANSLATE("Network Configuration"),
        wxTRANSLATE("Network connectivity and configuration issues"),
        wxTRANSLATE("Preference Settings"),
        wxTRANSLATE("Configuration and settings validation"),
        wxTRANSLATE("Controller Checks"),
        wxTRANSLATE("Controller configuration and connectivity"),
        wxTRANSLATE("Model Checks"),
        wxTRANSLATE("Model configuration and channel validation"),
        wxTRANSLATE("Sequence Issues"),
        wxTRANSLATE("Problems with sequence effects and media"),
        wxTRANSLATE("System Checks"),
        wxTRANSLATE("File system and performance issues"),
        // Categories
        wxTRANSLATE("Socket Issues"),
        wxTRANSLATE("Available IP Addresses"),
        wxTRANSLATE("Backup Directory Check"),
        wxTRANSLATE("Potentially Problematic Settings"),
        wxTRANSLATE("Inactive Controllers"),
        wxTRANSLATE("Multiple Outputs to Same Destination"),
        wxTRANSLATE("Controller-Specific Issues"),
        wxTRANSLATE("Other Controller Issues"),
        wxTRANSLATE("Unconfigured Models"),
        wxTRANSLATE("Model Configuration Issues"),
        wxTRANSLATE("Universe Configuration Issues"),
        wxTRANSLATE("Empty Models and Groups"),
        wxTRANSLATE("Channel Overlap Issues"),
        wxTRANSLATE("Model Channel Gap Issues"),
        wxTRANSLATE("Model Layer Allocation Issues"),
        wxTRANSLATE("Missing Face Images"),
        wxTRANSLATE("Model Groups containing duplicate nodes"),
        wxTRANSLATE("Model Group Reference Issues"),
        wxTRANSLATE("Model Group Preview Issues"),
        wxTRANSLATE("Group Name Uniqueness Issues"),
        wxTRANSLATE("Model Group Structure Errors"),
        wxTRANSLATE("Model Group DMX Issues"),
        wxTRANSLATE("Moving Head Model Groups with all heads with default fixture name"),
        wxTRANSLATE("SubModels with no nodes"),
        wxTRANSLATE("SubModels with duplicate nodes (These may not render as expected)"),
        wxTRANSLATE("SubModels with nodes not in parent model"),
        wxTRANSLATE("Model Settings Issues"),
        wxTRANSLATE("Uncommon and often undesirable Settings"),
        wxTRANSLATE("Autosave"),
        wxTRANSLATE("Models Not Found in Layout"),
        wxTRANSLATE("Hidden Model Effects"),
        wxTRANSLATE("Random Effects"),
        wxTRANSLATE("Effects With No Nodes"),
        wxTRANSLATE("Impossible Effect Overlaps"),
        wxTRANSLATE("Uncached Video Effects"),
        wxTRANSLATE("Per Model Buffer Issues"),
        wxTRANSLATE("Old Value Curves"),
        wxTRANSLATE("Canvas Mode Issues"),
        wxTRANSLATE("Transition Issues"),
        wxTRANSLATE("Effects Extending Beyond Sequence End"),
        wxTRANSLATE("Node Level Effect Issues"),
        wxTRANSLATE("Effect Settings Issues"),
        wxTRANSLATE("Corrupt Numeric Settings"),
        wxTRANSLATE("If you are planning on importing this sequence be aware the sequence relies on the following items that will not be imported."),
        wxTRANSLATE("General Notes"),
        wxTRANSLATE("File Access Performance"),
        wxTRANSLATE("File Path Issues"),
        // Group headings
        wxTRANSLATE("Socket Creation Failed"),
        wxTRANSLATE("Models not configured for controller:"),
        wxTRANSLATE("Models with configuration warnings:"),
        wxTRANSLATE("Models in your sequence do not exist in the layout. These will need to be deleted or remapped to another model next time you load this sequence."),
        wxTRANSLATE("The following effects are marked as random. This should never happen and may cause issues."),
        wxTRANSLATE("The following effects have no nodes and won't do anything:"),
        wxTRANSLATE("The following effect overlaps should not be possible:"),
        wxTRANSLATE("Model Faces Used"),
        wxTRANSLATE("Model States Used"),
        wxTRANSLATE("Viewpoints Used"),
        // "No ... found"
        wxTRANSLATE("No socket issues found"),
        wxTRANSLATE("No IP addresses found"),
        wxTRANSLATE("No backup directory issues found"),
        wxTRANSLATE("No problematic settings found"),
        wxTRANSLATE("No inactive controller issues found"),
        wxTRANSLATE("No multiple output issues found"),
        wxTRANSLATE("No issues found"),
        wxTRANSLATE("No other issues found"),
        wxTRANSLATE("No unconfigured models found"),
        wxTRANSLATE("No model configuration issues found"),
        wxTRANSLATE("No universe configuration issues found"),
        wxTRANSLATE("No empty model/group issues found"),
        wxTRANSLATE("No channel configuration issues found"),
        wxTRANSLATE("No channel overlap issues found"),
        wxTRANSLATE("No channel gap issues found"),
        wxTRANSLATE("No model layer allocation issues found"),
        wxTRANSLATE("No missing face images found"),
        wxTRANSLATE("No model group issues found"),
        wxTRANSLATE("No model group reference issues found"),
        wxTRANSLATE("No preview mismatch issues found"),
        wxTRANSLATE("No duplicate name issues found"),
        wxTRANSLATE("No structure errors found"),
        wxTRANSLATE("No DMX compatibility issues found"),
        wxTRANSLATE("No MH groups with all default fixture names found"),
        wxTRANSLATE("No submodel with no issues found"),
        wxTRANSLATE("No submodel with duplicate nodes issues found"),
        wxTRANSLATE("No submodel with nodes not in parent model issues found"),
        wxTRANSLATE("No model settings issues found"),
        wxTRANSLATE("No uncommon or undesirable settings found"),
        wxTRANSLATE("No autosave issues found"),
        wxTRANSLATE("No missing model issues found"),
        wxTRANSLATE("No hidden effect issues found"),
        wxTRANSLATE("No random effect issues found"),
        wxTRANSLATE("No effects with no nodes found"),
        wxTRANSLATE("No impossible effect overlaps found"),
        wxTRANSLATE("No uncached video effects found"),
        wxTRANSLATE("No per model buffer issues found"),
        wxTRANSLATE("No old value curve issues found"),
        wxTRANSLATE("No canvas mode issues found"),
        wxTRANSLATE("No transition issues found"),
        wxTRANSLATE("No effects extending beyond sequence end found"),
        wxTRANSLATE("No node level effect issues found"),
        wxTRANSLATE("No effect settings issues found"),
        wxTRANSLATE("No corrupt numeric setting issues found"),
        wxTRANSLATE("No general notes found"),
        wxTRANSLATE("No file access performance issues found"),
        wxTRANSLATE("No file path issues found"),
        // Shown after a count: "12 models have channel overlaps"
        wxTRANSLATE("models not configured for controller"),
        wxTRANSLATE("models have configuration warnings"),
        wxTRANSLATE("models have channel overlaps"),
        wxTRANSLATE("controller outputs have channel gaps"),
        wxTRANSLATE("models have missing face images"),
        wxTRANSLATE("model groups have reference issues"),
        wxTRANSLATE("model groups have preview mismatches"),
        wxTRANSLATE("Model groups have DMX compatibility issues"),
        wxTRANSLATE("model groups have overlapping nodes"),
    };
    return texts;
}

inline std::string Translate(const std::string& text) {
    if (text == "Show Folder") {
        return wxGetTranslation(wxTRANSLATE("Show folder")).utf8_string();
    }
    return wxGetTranslation(wxString::FromUTF8(text)).utf8_string();
}

inline std::string EscapeAttr(const std::string& text) {
    std::string out;
    for (char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c;
        }
    }
    return out;
}

// Returns the translation of a fixed text, or an empty string when the text
// is not part of the report frame.
inline std::string TranslateFixed(const std::string& text) {
    const auto& texts = Texts();
    if (texts.count(text)) {
        return Translate(text);
    }
    // "12 Errors", "3 models have channel overlaps"
    size_t i = 0;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
        ++i;
    }
    if (i > 0 && i < text.size() && text[i] == ' ' && texts.count(text.substr(i + 1))) {
        return text.substr(0, i + 1) + Translate(text.substr(i + 1));
    }
    const std::string jump = "Jump to ";
    if (text.rfind(jump, 0) == 0 && texts.count(text.substr(jump.size()))) {
        wxString fmt = wxGetTranslation(wxTRANSLATE("Jump to %s"));
        return wxString::Format(fmt, wxString::FromUTF8(Translate(text.substr(jump.size())))).utf8_string();
    }
    // Controller IP '<ip>', Output Connection '<port>':
    const std::string ipStart = "Controller IP '";
    const std::string ipMid = "', Output Connection '";
    if (text.rfind(ipStart, 0) == 0 && text.size() > 2 && text.compare(text.size() - 2, 2, "':") == 0) {
        size_t mid = text.find(ipMid);
        if (mid != std::string::npos) {
            std::string ip = text.substr(ipStart.size(), mid - ipStart.size());
            std::string port = text.substr(mid + ipMid.size(), text.size() - 2 - mid - ipMid.size());
            wxString fmt = wxGetTranslation(wxTRANSLATE("Controller IP '%s', Output Connection '%s':"));
            return wxString::Format(fmt, wxString::FromUTF8(ip), wxString::FromUTF8(port)).utf8_string();
        }
    }
    return {};
}

inline bool IsSpace(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline std::string TranslateTextNode(const std::string& node) {
    size_t b = 0;
    size_t e = node.size();
    while (b < e && IsSpace(node[b])) {
        ++b;
    }
    while (e > b && IsSpace(node[e - 1])) {
        --e;
    }
    if (b == e) {
        return node;
    }
    std::string tr = TranslateFixed(node.substr(b, e - b));
    if (tr.empty()) {
        return node;
    }
    return node.substr(0, b) + EscapeAttr(tr) + node.substr(e);
}

// Translates the title="", placeholder="" and aria-label="" values of a tag.
inline std::string TranslateTag(const std::string& tag) {
    std::string out = tag;
    for (const char* attr : { " title=\"", " placeholder=\"", " aria-label=\"" }) {
        std::string a = attr;
        size_t pos = 0;
        while ((pos = out.find(a, pos)) != std::string::npos) {
            size_t start = pos + a.size();
            size_t end = out.find('"', start);
            if (end == std::string::npos) {
                break;
            }
            std::string tr = TranslateFixed(out.substr(start, end - start));
            if (!tr.empty()) {
                tr = EscapeAttr(tr);
                out.replace(start, end - start, tr);
                end = start + tr.size();
            }
            pos = end;
        }
    }
    return out;
}

inline std::string TranslateHTML(const std::string& html, const wxString& language) {
    std::string out;
    out.reserve(html.size() + html.size() / 8);
    size_t pos = 0;
    while (pos < html.size()) {
        size_t lt = html.find('<', pos);
        if (lt == std::string::npos) {
            out += TranslateTextNode(html.substr(pos));
            break;
        }
        out += TranslateTextNode(html.substr(pos, lt - pos));
        size_t gt = html.find('>', lt);
        if (gt == std::string::npos) {
            out += html.substr(lt);
            break;
        }
        std::string tag = html.substr(lt, gt - lt + 1);
        // Leave scripts and styles untouched.
        for (const char* raw : { "script", "style" }) {
            std::string open = std::string("<") + raw;
            if (tag.compare(0, open.size(), open) == 0 && (tag.size() == open.size() + 1 || tag[open.size()] == '>' || tag[open.size()] == ' ')) {
                std::string close = std::string("</") + raw + ">";
                size_t endRaw = html.find(close, gt);
                if (endRaw != std::string::npos) {
                    gt = endRaw + close.size() - 1;
                    tag = html.substr(lt, gt - lt + 1);
                }
                break;
            }
        }
        if (tag.compare(0, 5, "<html") == 0 && !language.empty()) {
            size_t l = tag.find("lang=\"en\"");
            if (l != std::string::npos) {
                tag.replace(l, 9, "lang=\"" + language.BeforeFirst('_').utf8_string() + "\"");
            }
            out += tag;
        } else if (tag.compare(0, 2, "<!") == 0 || tag.compare(0, 7, "<script") == 0 || tag.compare(0, 6, "<style") == 0) {
            out += tag;
        } else {
            out += TranslateTag(tag);
        }
        pos = gt + 1;
    }
    return out;
}

} // namespace CheckReportTranslation

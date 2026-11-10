#include "PropertyGridBuilder.h"

#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/sizer.h>

#include <wx/translation.h>

#include <functional>
#include <string>

// Labels and help texts of the AI services. They are defined in the wx-free core,
// so they are listed here for the translation catalog and translated when the
// grid is built. Choice values are not translated: the grid saves them as shown.
[[maybe_unused]] static const char* const AI_SERVICE_TEXTS[] = {
    wxTRANSLATE_IN_CONTEXT("ai service", "Enable %s"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Prompt"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Color Palette"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Images"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Mapping"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Speech to Text"),
    wxTRANSLATE_IN_CONTEXT("ai service", "API Key"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Bearer Token"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Fake API key"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Your Anthropic API key (masked for security)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Your ChatGPT Bearer Token (masked for security)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Your Google Gemini API key (masked for security)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Model"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Image Model"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Completion Model"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Transcribe Model"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Base URL"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Host"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Port"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Https"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Size"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Steps"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Style"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Generic OpenAI Client"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Apple Intelligence Image"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Inference Device"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Hardware target for inference (AUTO lets OpenVINO pick the fastest available)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "SD Model Directory"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Path to a Stable Diffusion model exported to OpenVINO IR format.\n"
                                         "Export with: optimum-cli export openvino --model stabilityai/stable-diffusion-v1-5 <dir>"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Whisper Model Directory"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Path to a Whisper model exported to OpenVINO IR format.\n"
                                         "Export with: optimum-cli export openvino --model openai/whisper-base <dir>"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Image Width (px)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Image Height (px)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Width of generated images"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Height of generated images"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Diffusion Steps"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Denoising steps (20 is a good default; more = slower but higher quality)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Width (px)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Height (px)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Output image width in pixels (must match the model's trained resolution)"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Output image height in pixels"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Inference Steps"),
    wxTRANSLATE_IN_CONTEXT("ai service", "Number of diffusion denoising steps (more = higher quality, slower)")
};

static wxString AIText(const std::string& s) {
    static const wxString ctx("ai service");
    static const std::string enable("Enable ");
    if (s.rfind(enable, 0) == 0) {
        return wxString::Format(wxGetTranslation("Enable %s", wxString(), ctx),
                                wxGetTranslation(wxString::FromUTF8(s.substr(enable.size())), wxString(), ctx));
    }
    return wxGetTranslation(wxString::FromUTF8(s), wxString(), ctx);
}

namespace PropertyGridBuilder {

void Append(wxPropertyGrid* page, const std::vector<ServiceProperty>& props) {
    for (const auto& p : props) {
        switch (p.kind) {
            case ServiceProperty::Kind::Category: {
                page->Append(new wxPropertyCategory(AIText(p.label.empty() ? p.category : p.label), p.label.empty() ? p.category : p.label));
                break;
            }
            case ServiceProperty::Kind::Bool: {
                bool v = std::holds_alternative<bool>(p.value) ? std::get<bool>(p.value) : false;
                auto* prop = page->Append(new wxBoolProperty(AIText(p.label), p.id, v));
                prop->SetEditor("CheckBox");
                if (!p.help.empty()) prop->SetHelpString(AIText(p.help));
                break;
            }
            case ServiceProperty::Kind::Int: {
                int v = std::holds_alternative<int>(p.value) ? std::get<int>(p.value) : 0;
                auto* prop = page->Append(new wxIntProperty(AIText(p.label), p.id, v));
                if (!p.help.empty()) prop->SetHelpString(AIText(p.help));
                break;
            }
            case ServiceProperty::Kind::String: {
                const std::string& v = std::holds_alternative<std::string>(p.value)
                                       ? std::get<std::string>(p.value)
                                       : std::string();
                auto* prop = page->Append(new wxStringProperty(AIText(p.label), p.id, v));
                if (!p.help.empty()) prop->SetHelpString(AIText(p.help));
                break;
            }
            case ServiceProperty::Kind::Secret: {
                const std::string& v = std::holds_alternative<std::string>(p.value)
                                       ? std::get<std::string>(p.value)
                                       : std::string();
                auto* prop = page->Append(new wxStringProperty(AIText(p.label), p.id, v));
                prop->SetAttribute(wxPG_STRING_PASSWORD, true);
                if (!p.help.empty()) prop->SetHelpString(AIText(p.help));
                break;
            }
            case ServiceProperty::Kind::Choice: {
                wxPGChoices choices;
                for (const auto& c : p.choices) choices.Add(c);
                const std::string& v = std::holds_alternative<std::string>(p.value)
                                       ? std::get<std::string>(p.value)
                                       : std::string();
                int sel = 0;
                for (size_t i = 0; i < p.choices.size(); ++i) {
                    if (p.choices[i] == v) { sel = static_cast<int>(i); break; }
                }
                auto* prop = page->Append(new wxEnumProperty(AIText(p.label), p.id, choices, sel));
                if (!p.help.empty()) prop->SetHelpString(AIText(p.help));
                break;
            }
        }
    }
}

wxPanel* MakePanel(wxWindow* parent,
                   const std::vector<ServiceProperty>& props,
                   std::function<void(const wxPropertyGridEvent&)> onChanged) {
    auto* panel = new wxPanel(parent);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* grid = new wxPropertyGrid(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);
    grid->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);
    Append(grid, props);
    sizer->Add(grid, 1, wxEXPAND | wxALL, 0);
    panel->SetSizer(sizer);
    if (onChanged) {
        grid->Bind(wxEVT_PG_CHANGED,
                   [cb = std::move(onChanged)](wxPropertyGridEvent& e) { cb(e); });
    }
    return panel;
}

void Dispatch(const wxPropertyGridEvent& event,
              const std::function<void(const std::string&, bool)>& setBool,
              const std::function<void(const std::string&, int)>& setInt,
              const std::function<void(const std::string&, const std::string&)>& setString) {
    auto* prop = event.GetProperty();
    if (!prop) return;
    std::string id = prop->GetName().ToStdString();
    const wxVariant value = event.GetPropertyValue();
    const wxString type = value.GetType();
    if (type == "bool") {
        if (setBool) setBool(id, value.GetBool());
    } else if (type == "long") {
        // wxEnumProperty selection also arrives as a long. If the editor has a
        // string value (enum choice), prefer the choice label.
        if (prop->IsKindOf(CLASSINFO(wxEnumProperty))) {
            std::string s = prop->GetValueAsString().ToStdString();
            if (setString) setString(id, s);
        } else if (setInt) {
            setInt(id, static_cast<int>(value.GetLong()));
        }
    } else if (type == "string") {
        if (setString) setString(id, value.GetString().ToStdString());
    }
}

} // namespace PropertyGridBuilder

/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "CloudControllerConfig.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "ControllerCaps.h"
#include "ControllerUploadData.h"
#include "../models/Model.h"
#include "../outputs/Controller.h"
#include "../outputs/Output.h"
#include "../outputs/OutputManager.h"

namespace CloudControllerConfig {

bool IsCloudController(const Controller* controller) {
    return controller != nullptr && controller->GetVendor() == VENDOR;
}

std::string ObjectKey(const std::string& controllerName) {
    std::string name;
    for (unsigned char c : controllerName) {
        bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
        name += safe ? (char)c : '_';
    }
    if (name.empty() || name.find_first_not_of('.') == std::string::npos) {
        name = "controller";
    }
    return "controllers/" + name + ".json";
}

std::vector<std::string> TimingWarnings(const Timing& t, int frameMs) {
    std::vector<std::string> warnings;
    if (frameMs <= 0) {
        return warnings;
    }
    int maxWire = (frameMs * 1000 - WIRE_RESET_US) / WIRE_US_PER_PIXEL;
    if (t.longestWire > maxWire) {
        warnings.push_back("Port " + std::to_string(t.longestPort) + " drives " + std::to_string(t.longestWire) +
                           " pixels; at " + std::to_string(frameMs) + " ms per frame the board can send at most " +
                           std::to_string(maxWire) + " per port.");
    }
    int maxChannels = (int)((long long)MAX_CHANNELS_PER_SECOND * frameMs / 1000);
    if (t.totalChannels > maxChannels) {
        warnings.push_back(std::to_string(t.totalChannels) + " channels per frame; at " + std::to_string(frameMs) +
                           " ms per frame the board can read at most " + std::to_string(maxChannels) + ".");
    }
    return warnings;
}

std::string Build(Controller* controller, OutputManager* om, ModelManager* mm, std::string& error,
                  int frameMs, std::vector<std::string>* warnings, Timing* timing) {
    if (controller->GetName().size() > MAX_NAME_LENGTH) {
        error = "The controller name is longer than " + std::to_string(MAX_NAME_LENGTH) + " characters.";
        return {};
    }
    UDController cud(controller, om, mm, false);
    ControllerCaps* rules = ControllerCaps::GetControllerConfig(controller);
    std::string check;
    if (!cud.Check(rules, check)) {
        error = check.empty() ? "The ports do not fit the controller." : check;
        return {};
    }

    nlohmann::json j;
    j["controller"] = controller->GetName();
    j["vendor"] = controller->GetVendor();
    j["model"] = controller->GetModel();
    j["ip"] = controller->GetIP();

    nlohmann::json input;
    input["protocol"] = controller->GetProtocol();
    input["startChannel"] = controller->GetStartChannel();
    input["channels"] = controller->GetChannels();
    if (Output* first = controller->GetFirstOutput()) {
        if (controller->GetProtocol() == OUTPUT_E131 || controller->GetProtocol() == OUTPUT_ARTNET) {
            input["universe"] = first->GetUniverse();
            input["universes"] = (int)controller->GetOutputs().size();
        }
    }
    j["input"] = input;

    const int defaultBrightness = controller->GetDefaultBrightnessUnderFullControl();
    const float defaultGamma = controller->GetDefaultGammaUnderFullControl();

    nlohmann::json pixelPorts = nlohmann::json::array();
    int longestWire = 0;
    int longestPort = 0;
    int totalChannels = 0;
    for (int p = 1; p <= cud.GetMaxPixelPort(); p++) {
        if (!cud.HasPixelPort(p)) {
            continue;
        }
        UDControllerPort* port = cud.GetControllerPixelPort(p);
        if (port->GetModels().empty() || port->Channels() <= 0) {
            continue;
        }
        UDControllerPortModel* first = port->GetModels().front();
        nlohmann::json jp;
        jp["port"] = p;
        jp["protocol"] = port->GetProtocol();
        jp["startChannel"] = port->GetStartChannel();
        jp["channels"] = port->Channels();
        jp["pixels"] = port->Pixels();
        // Empty when no model sets it: the board keeps its own order.
        jp["colorOrder"] = first->GetColourOrder("");
        jp["brightness"] = first->GetBrightness(defaultBrightness);
        jp["gamma"] = first->GetGamma((int)defaultGamma);
        jp["groupCount"] = first->GetGroupCount(1);
        jp["startNulls"] = first->GetStartNullPixels(0);
        jp["endNulls"] = first->GetEndNullPixels(0);
        jp["zigZag"] = first->GetZigZag(0);
        jp["reverse"] = first->GetDirection("Forward") == "Reverse";
        pixelPorts.push_back(jp);

        int wire = jp["startNulls"].get<int>() + port->Pixels() * std::max(1, jp["groupCount"].get<int>()) + jp["endNulls"].get<int>();
        if (wire > longestWire) {
            longestWire = wire;
            longestPort = p;
        }
        totalChannels += port->Channels();
    }
    j["pixelPorts"] = pixelPorts;

    nlohmann::json serialPorts = nlohmann::json::array();
    for (int p = 1; p <= cud.GetMaxSerialPort(); p++) {
        if (!cud.HasSerialPort(p)) {
            continue;
        }
        UDControllerPort* port = cud.GetControllerSerialPort(p);
        if (port->GetModels().empty() || port->Channels() <= 0) {
            continue;
        }
        nlohmann::json jp;
        jp["port"] = p;
        jp["protocol"] = port->GetProtocol();
        jp["startChannel"] = port->GetStartChannel();
        jp["channels"] = port->Channels();
        jp["baudRate"] = port->GetModels().front()->GetModel()->GetControllerProtocolSpeed();
        serialPorts.push_back(jp);
    }
    j["serialPorts"] = serialPorts;

    Timing t{ longestWire, longestPort, totalChannels };
    if (timing != nullptr) {
        *timing = t;
    }
    if (warnings != nullptr) {
        for (auto& w : TimingWarnings(t, frameMs)) {
            warnings->push_back(std::move(w));
        }
    }
    return j.dump();
}

std::vector<File> BuildAll(OutputManager* om, ModelManager* mm, int frameMs) {
    std::vector<File> files;
    for (Controller* c : om->GetControllers()) {
        if (!IsCloudController(c) || !c->IsActive()) {
            continue;
        }
        File f;
        f.controller = c->GetName();
        f.key = ObjectKey(c->GetName());
        f.startChannel = (uint32_t)std::max<int32_t>(1, c->GetStartChannel());
        f.channels = (uint32_t)std::max<int32_t>(0, c->GetChannels());
        f.json = Build(c, om, mm, f.error, frameMs, &f.warnings, &f.timing);
        files.push_back(std::move(f));
    }
    return files;
}

} // namespace CloudControllerConfig

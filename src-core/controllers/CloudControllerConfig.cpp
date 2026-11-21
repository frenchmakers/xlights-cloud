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

std::string Build(Controller* controller, OutputManager* om, ModelManager* mm, const std::string& generatedUtc, std::string& error) {
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
    j["generated"] = generatedUtc;
    return j.dump();
}

std::vector<File> BuildAll(OutputManager* om, ModelManager* mm, const std::string& generatedUtc) {
    std::vector<File> files;
    for (Controller* c : om->GetControllers()) {
        if (!IsCloudController(c) || !c->IsActive()) {
            continue;
        }
        File f;
        f.controller = c->GetName();
        f.key = ObjectKey(c->GetName());
        f.json = Build(c, om, mm, generatedUtc, f.error);
        files.push_back(std::move(f));
    }
    return files;
}

} // namespace CloudControllerConfig

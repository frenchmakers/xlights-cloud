/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "CloudBoard.h"

#include "CloudControllerConfig.h"
#include "../render/UICallbacks.h"
#include "../utils/CurlManager.h"

#include <nlohmann/json.hpp>
#include <log.h>

CloudBoard::CloudBoard(const std::string& ip, const std::string& proxy) :
    BaseController(ip, proxy) {
    std::string info = GetURL("/api/system/info");
    nlohmann::json j = nlohmann::json::parse(info, nullptr, false);
    if (j.is_object()) {
        _connected = true;
        _model = j.value("Variant", std::string("STM32"));
        _version = j.value("Version", std::string());
    } else {
        spdlog::warn("xLights Cloud board at {} did not answer /api/system/info.", ip);
    }
}

std::string CloudBoard::ErrorFromResponse(int status, const std::string& body) {
    nlohmann::json j = nlohmann::json::parse(body, nullptr, false);
    if (j.is_object() && j.contains("Message") && j["Message"].is_string() && !j["Message"].get<std::string>().empty()) {
        return j["Message"].get<std::string>();
    }
    if (status == 0) {
        return "The board did not answer.";
    }
    return "The board answered HTTP " + std::to_string(status) + ".";
}

#ifndef DISCOVERYONLY
bool CloudBoard::SetOutputs(ModelManager* allmodels, OutputManager* outputManager, Controller* controller, UICallbacks* ui) {
    std::string error;
    std::vector<std::string> warnings;
    std::string json = CloudControllerConfig::Build(controller, outputManager, allmodels, error, 0, &warnings);
    if (json.empty()) {
        if (ui != nullptr) {
            ui->ShowMessage("The outputs were not sent: " + error, "Upload Outputs");
        }
        return false;
    }

    std::string const baseIP = _fppProxy.empty() ? _ip : _fppProxy;
    std::string url = "http://" + baseIP + _baseUrl + "/api/config";
    int rc = 0;
    std::string body = CurlManager::INSTANCE.doPost(url, "application/json", json, rc);
    spdlog::info("xLights Cloud board: POST {} -> {}", url, rc);
    if (rc == 200) {
        return true;
    }
    if (ui != nullptr) {
        ui->ShowMessage("The board refused the outputs: " + ErrorFromResponse(rc, body), "Upload Outputs");
    }
    return false;
}
#endif

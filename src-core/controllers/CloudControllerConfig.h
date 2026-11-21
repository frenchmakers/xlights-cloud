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

// The output configuration of a cloud controller: a board that has no upload
// API of its own and downloads its settings from the cloud bucket, next to the
// sequence it plays. The port mapping is the one "Upload Outputs" computes
// (UDController), written as a small JSON file the board can parse with a
// fixed buffer.

#include <string>
#include <vector>

class Controller;
class ModelManager;
class OutputManager;

namespace CloudControllerConfig {

constexpr const char* VENDOR = "FrenchMakers";

bool IsCloudController(const Controller* controller);

// "controllers/<name>.json", the name made safe for an object key.
std::string ObjectKey(const std::string& controllerName);

// The configuration as JSON, or an empty string and error when the ports do
// not fit the controller's capabilities.
std::string Build(Controller* controller, OutputManager* om, ModelManager* mm, const std::string& generatedUtc, std::string& error);

struct File {
    std::string controller;
    std::string key;
    std::string json;
    std::string error; // set instead of json when the configuration could not be built
};

// One file for every active cloud controller.
std::vector<File> BuildAll(OutputManager* om, ModelManager* mm, const std::string& generatedUtc);

} // namespace CloudControllerConfig

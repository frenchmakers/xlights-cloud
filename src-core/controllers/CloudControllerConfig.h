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

#include <cstdint>
#include <string>
#include <vector>

class Controller;
class ModelManager;
class OutputManager;

namespace CloudControllerConfig {

constexpr const char* VENDOR = "FrenchMakers";
constexpr const char* BOARD_MODEL = "STM32 Cloud Pixel";
constexpr const char* BOARD_VARIANT = "6 Ports";
// What the board answers FPP pings with (byte 9); unassigned in FPP's MultiSync.h.
constexpr int FPP_TYPE_ID = 0xC5;
// The board keeps the controller name in a fixed buffer.
constexpr size_t MAX_NAME_LENGTH = 32;
// All ports are clocked out together, so a frame lasts as long as the longest
// port on the wire (start nulls + pixels x group + end nulls) plus the reset.
constexpr int WIRE_US_PER_PIXEL = 30;
constexpr int WIRE_RESET_US = 350;
// What the board reads from its USB key: channels per frame x frames per second.
constexpr int MAX_CHANNELS_PER_SECOND = 500000;

bool IsCloudController(const Controller* controller);

// "controllers/<name>.json", the name made safe for an object key.
std::string ObjectKey(const std::string& controllerName);

// What limits the frame rate a board can keep up with.
struct Timing {
    int longestWire = 0;   // pixels on the wire of the longest port: start nulls + pixels x group + end nulls
    int longestPort = 0;
    int totalChannels = 0; // channels per frame
};

// Why the board cannot play frames of frameMs; empty when it can or frameMs <= 0.
std::vector<std::string> TimingWarnings(const Timing& timing, int frameMs);

// The configuration as JSON, or an empty string and error when the ports do
// not fit the controller's capabilities. frameMs > 0 is the frame time of the
// sequence the board will play; ports it cannot keep up with add warnings.
// The same layout always gives the same bytes, so a board can tell an
// unchanged configuration by its hash.
std::string Build(Controller* controller, OutputManager* om, ModelManager* mm, std::string& error,
                  int frameMs = 0, std::vector<std::string>* warnings = nullptr, Timing* timing = nullptr);

struct File {
    std::string controller;
    std::string key;
    uint32_t startChannel = 0; // 1-based, absolute
    uint32_t channels = 0;
    std::string json;
    std::string error; // set instead of json when the configuration could not be built
    std::vector<std::string> warnings;
    Timing timing;
};

// One file for every active cloud controller.
std::vector<File> BuildAll(OutputManager* om, ModelManager* mm, int frameMs = 0);

} // namespace CloudControllerConfig

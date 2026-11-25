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

// "Upload Outputs" for an xLights Cloud board (vendor FrenchMakers): posts the
// same config.json the cloud backup publishes to the board's /api/config.

#include <string>

#include "BaseController.h"

class CloudBoard : public BaseController {
public:
    CloudBoard(const std::string& ip, const std::string& proxy);
    ~CloudBoard() override {}

#ifndef DISCOVERYONLY
    bool SetOutputs(ModelManager* allmodels, OutputManager* outputManager, Controller* controller, UICallbacks* ui) override;
#endif
    [[nodiscard]] bool UsesHTTP() const override { return true; }

    // What the board said when it refused a configuration ("Message" of its
    // JSON answer), or the HTTP status when it said nothing readable.
    static std::string ErrorFromResponse(int status, const std::string& body);
};

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <pugixml.hpp>

#include "controllers/CloudControllerConfig.h"
#include "controllers/ControllerCaps.h"
#include "models/Model.h"
#include "models/ModelManager.h"
#include "outputs/ControllerEthernet.h"
#include "outputs/Output.h"
#include "outputs/OutputManager.h"
#include "utils/FileUtils.h"

namespace {

std::filesystem::path RepoResourcesDir() {
    // this file lives in tests/unit/core/
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path() / "resources";
}

// The shipped FrenchMakers board with an 18000 channel DDP input (6 ports of 1000 pixels).
struct CloudShow {
    std::string prevResources;
    OutputManager om;
    ModelManager mm{ &om };
    ControllerEthernet* ctl = nullptr;

    CloudShow() {
        prevResources = FileUtils::GetResourcesDir();
        ControllerCaps::UnloadControllers();
        FileUtils::SetResourcesDir(RepoResourcesDir().string());
        ControllerCaps::LoadControllers();

        ctl = new ControllerEthernet(&om, false);
        ctl->SetName("frenchmakers");
        ctl->SetIP("192.168.1.50");
        ctl->SetProtocol(OUTPUT_DDP);
        ctl->SetVendor(CloudControllerConfig::VENDOR);
        ctl->SetModel("STM32 Cloud Pixel");
        ctl->SetVariant("6 Ports");
        ctl->SetAutoLayout(false);
        ctl->GetOutputs().front()->SetChannels(18000);
        om.AddController(ctl);
        om.SomethingChanged();
    }
    ~CloudShow() {
        ControllerCaps::UnloadControllers();
        FileUtils::SetResourcesDir(prevResources);
    }

    void OnPort(const std::string& name, int port, int nodes, const std::string& startChannel, const std::string& extraCC = "") {
        std::string xml = "<model name=\"" + name + "\" DisplayAs=\"Single Line\" parm1=\"1\" parm2=\"" + std::to_string(nodes) +
                          "\" parm3=\"1\" StringType=\"RGB Nodes\" StartChannel=\"" + startChannel +
                          "\" Controller=\"frenchmakers\"><ControllerConnection Protocol=\"ws2811\" Port=\"" + std::to_string(port) + "\" " +
                          extraCC + "/></model>";
        pugi::xml_document doc;
        REQUIRE(doc.load_string(xml.c_str()));
        REQUIRE(mm.createAndAddModel(doc.first_child(), 100, 100) != nullptr);
    }
    void Recalc() {
        om.SomethingChanged();
        mm.RecalcStartChannels();
    }
};

} // namespace

TEST_CASE("Cloud controller object keys are safe", "[controllers][cloud]") {
    CHECK(CloudControllerConfig::ObjectKey("frenchmakers") == "controllers/frenchmakers.json");
    CHECK(CloudControllerConfig::ObjectKey("Garage tree/2") == "controllers/Garage_tree_2.json");
    CHECK(CloudControllerConfig::ObjectKey("") == "controllers/controller.json");
    CHECK(CloudControllerConfig::ObjectKey("..") == "controllers/controller.json");
}

TEST_CASE("Cloud controller config lists the ports Upload Outputs would send", "[controllers][cloud]") {
    CloudShow show;
    REQUIRE(ControllerCaps::GetControllerConfig(show.ctl) != nullptr);
    show.OnPort("Roof", 1, 100, "!frenchmakers:1", "colorOrder=\"GRB\" Brightness=\"50\" nullNodes=\"2\" Reverse=\"1\"");
    show.OnPort("Tree", 3, 400, "!frenchmakers:1201");
    show.Recalc();

    std::string error;
    std::string text = CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error);
    INFO(error);
    REQUIRE_FALSE(text.empty());
    auto j = nlohmann::json::parse(text);

    CHECK(j["controller"] == "frenchmakers");
    CHECK(j["ip"] == "192.168.1.50");
    CHECK(j["input"]["protocol"] == OUTPUT_DDP);
    CHECK(j["input"]["startChannel"] == 1);
    CHECK(j["input"]["channels"] == 18000);
    CHECK_FALSE(j.contains("generated"));
    CHECK(j["serialPorts"].empty());

    REQUIRE(j["pixelPorts"].size() == 2);
    auto roof = j["pixelPorts"][0];
    CHECK(roof["port"] == 1);
    CHECK(roof["protocol"] == "ws2811");
    CHECK(roof["startChannel"] == 1);
    CHECK(roof["channels"] == 300);
    CHECK(roof["pixels"] == 100);
    CHECK(roof["colorOrder"] == "GRB");
    CHECK(roof["brightness"] == 50);
    CHECK(roof["startNulls"] == 2);
    CHECK(roof["reverse"] == true);

    auto tree = j["pixelPorts"][1];
    CHECK(tree["port"] == 3);
    CHECK(tree["startChannel"] == 1201);
    CHECK(tree["pixels"] == 400);
    CHECK(tree["colorOrder"] == "");
    CHECK(tree["brightness"] == show.ctl->GetDefaultBrightnessUnderFullControl());
    CHECK(tree["reverse"] == false);
}

TEST_CASE("Cloud controller config refuses ports the board cannot drive", "[controllers][cloud]") {
    CloudShow show;
    show.OnPort("TooLong", 1, 1001, "!frenchmakers:1");
    show.Recalc();

    std::string error;
    CHECK(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error).empty());
    CHECK_FALSE(error.empty());
}

TEST_CASE("Only active FrenchMakers controllers get a config file", "[controllers][cloud]") {
    CloudShow show;
    show.OnPort("Roof", 1, 10, "!frenchmakers:1");
    show.Recalc();

    auto files = CloudControllerConfig::BuildAll(&show.om, &show.mm);
    REQUIRE(files.size() == 1);
    CHECK(files[0].key == "controllers/frenchmakers.json");
    CHECK(files[0].startChannel == 1);
    CHECK(files[0].channels == 18000);
    CHECK(files[0].error.empty());

    show.ctl->SetVendor("ESPixelStick");
    CHECK(CloudControllerConfig::BuildAll(&show.om, &show.mm).empty());

    show.ctl->SetVendor(CloudControllerConfig::VENDOR);
    show.ctl->SetActive("Inactive");
    CHECK(CloudControllerConfig::BuildAll(&show.om, &show.mm).empty());
}

TEST_CASE("Cloud controller config refuses ports and names the board cannot hold", "[controllers][cloud]") {
    CloudShow show;
    show.OnPort("Port7", 7, 10, "!frenchmakers:1");
    show.Recalc();
    std::string error;
    CHECK(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error).empty());
    CHECK_FALSE(error.empty());

    CloudShow named;
    named.ctl->SetName(std::string(CloudControllerConfig::MAX_NAME_LENGTH + 1, 'x'));
    error.clear();
    CHECK(CloudControllerConfig::Build(named.ctl, &named.om, &named.mm, error).empty());
    CHECK(error.find("32") != std::string::npos);
}

TEST_CASE("Cloud controller config warns when the frame rate is too high for the board", "[controllers][cloud]") {
    CloudShow show;
    show.OnPort("Long", 1, 900, "!frenchmakers:1");
    show.Recalc();

    std::string error;
    std::vector<std::string> warnings;
    // (25000 - 350) / 30 = 821 pixels per port at 40 fps
    CHECK_FALSE(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error, 25, &warnings).empty());
    REQUIRE(warnings.size() == 1);
    CHECK(warnings[0].find("Port 1") != std::string::npos);
    CHECK(warnings[0].find("821") != std::string::npos);

    warnings.clear();
    CHECK_FALSE(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error, 50, &warnings).empty());
    CHECK(warnings.empty());

    warnings.clear();
    CHECK_FALSE(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error, 0, &warnings).empty());
    CHECK(warnings.empty());
}

TEST_CASE("Cloud controller config warns when the board cannot read that many channels", "[controllers][cloud]") {
    CloudShow show;
    for (int p = 1; p <= 6; p++) {
        show.OnPort("P" + std::to_string(p), p, 800, "!frenchmakers:" + std::to_string((p - 1) * 2400 + 1));
    }
    show.Recalc();

    std::string error;
    std::vector<std::string> warnings;
    // 6 x 800 x 3 = 14400 channels, 12500 at most at 40 fps
    CHECK_FALSE(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error, 25, &warnings).empty());
    REQUIRE(warnings.size() == 1);
    CHECK(warnings[0].find("14400") != std::string::npos);
    CHECK(warnings[0].find("12500") != std::string::npos);

    warnings.clear();
    CHECK_FALSE(CloudControllerConfig::Build(show.ctl, &show.om, &show.mm, error, 50, &warnings).empty());
    CHECK(warnings.empty());
}

TEST_CASE("Cloud controller timing is kept for each sequence's frame rate", "[controllers][cloud]") {
    CloudShow show;
    show.OnPort("Long", 2, 900, "!frenchmakers:1");
    show.Recalc();

    auto files = CloudControllerConfig::BuildAll(&show.om, &show.mm);
    REQUIRE(files.size() == 1);
    CHECK(files[0].timing.longestPort == 2);
    CHECK(files[0].timing.longestWire == 900);
    CHECK(files[0].timing.totalChannels == 2700);
    CHECK(files[0].warnings.empty());

    auto at40 = CloudControllerConfig::TimingWarnings(files[0].timing, 25);
    REQUIRE(at40.size() == 1);
    CHECK(at40[0].find("Port 2") != std::string::npos);
    CHECK(CloudControllerConfig::TimingWarnings(files[0].timing, 50).empty());
    CHECK(CloudControllerConfig::TimingWarnings(files[0].timing, 0).empty());
}

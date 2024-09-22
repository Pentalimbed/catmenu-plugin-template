#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include "menu_api.h"

CatMenu::APIBase* api = nullptr;

bool NotifyTestWindow()
{
    bool window_open = true;
    ImGui::Begin("Notify Test Window", &window_open);
    if (ImGui::Button("Notify!"))
        api->InsertNotification({ImGuiToastType::Info, 5000, "Bump!"});
    ImGui::End();
    return window_open;
}

void IntegrateCatMenu()
{
    logger::info("Looking for CatMenu...");

    auto result = CatMenu::RequestCatMenuAPI();
    if (result.index() == 0) {
        api = std::get<0>(result);

        auto ver = api->GetVersion();
        logger::info("CatMenu Version {}", ver);

        ImGui::SetCurrentContext(api->GetContext());

        RE::BSString window_name{"ImGui Demo Window"};
        api->RegisterMenuDrawFunc(window_name, []() {
            bool window_open = true;
            ImGui::ShowDemoWindow(&window_open);
            return window_open;
        });

        window_name = {"Notify Test Window"};
        api->RegisterMenuDrawFunc(window_name, NotifyTestWindow);

        logger::info("CatHub integration succeeded!");

    } else
        logger::warn("CatHub integration failed! Error: {}", std::get<1>(result));
}

void InitLogging()
{
    auto path = logger::log_directory();
    if (!path)
        return;

    const auto plugin = SKSE::PluginDeclaration::GetSingleton();
    *path /= std::format("{}.log", plugin->GetName());

    std::vector<spdlog::sink_ptr> sinks{
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true),
        std::make_shared<spdlog::sinks::msvc_sink_mt>()};

    auto logger = std::make_shared<spdlog::logger>("global", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::info);

    spdlog::set_default_logger(std::move(logger));
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] [%t] [%s:%#] %v");
}

void ProcessMessage(SKSE::MessagingInterface::Message* a_msg)
{
    if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
        IntegrateCatMenu();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    InitLogging();

    const auto plugin = SKSE::PluginDeclaration::GetSingleton();
    logger::info("{} v{} is loading...", plugin->GetName(), plugin->GetVersion());

    SKSE::Init(a_skse);

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", ProcessMessage))
        return false;

    logger::info("{} loaded.", plugin->GetName());

    return true;
}

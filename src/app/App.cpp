// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <chrono>
#include <thread>

#include <app/App.hpp>
#include <common/Config.hpp>
#include <common/FunctionRef.hpp>
#include <common/Messages.hpp>
#include <common/PubSub.hpp>
#include <common/Profiler.hpp>
#include <common/PropertySet.hpp>
#include <common/System.hpp>
#include <fs/Cache.hpp>
#include <fs/FileSystem.hpp>
#include <fs/Folder.hpp>
#include <log/Log.hpp>
#include <script/Engine.hpp>
#include <task/TaskManager.hpp>

using namespace fs;

class AppImpl : public App {
public:
    bool running = true;
    inject<Log> log;
    inject<System> system{"new"};
    System::Provides globalSystem{system.get()};

    inject<TaskManager> taskman{"new"};
    TaskManager::Provides globalTaskMan{taskman.get()};

    inject<Cache> cache{"new"};
    Cache::Provides globalCache{cache.get()};

    inject<FileSystem> fs{"new"};
    FileSystem::Provides globalFS{fs.get()};

    inject<Config> config{"new"};
    Config::Provides globalConfig{config.get()};

    PubSub<msg::RequestShutdown> pub{this};

    using clock = std::chrono::high_resolution_clock;

    clock::time_point referenceTime;
    clock::time_point tockTime;
    U32 framesUntilTock = 0;

    void boot(int argc, const char* argv[]) override {
        auto lock = cache->lock();
        referenceTime = clock::now();
        log->setGlobal();
#ifdef _DEBUG
        log->setLevel(Log::Level::Verbose); // TODO: Configure level using args
#else
        log->setLevel(Log::Level::Info); // TODO: Configure level using args
#endif
        initValue();
        fs->boot();
        config->boot();
        system->boot();
        if (auto autorun = fs->find("%appdata/autorun", "dir")->get<Folder>()) {
            Vector<std::pair<S32, std::shared_ptr<File>>> files;
            autorun->forEach([&](std::shared_ptr<FSEntity> child) {
                if (child->isFile()) {
                    auto file = child->get<File>();
                    S32 priority = atoi(file->name().c_str());
                    if (priority)
                        files.emplace_back(priority, file);
                }
            });
            std::sort(files.begin(), files.end(), [](auto& left, auto& right) {
                return left.first < right.first;
            });
            for (auto& entry : files)
                entry.second->parse("", true);
        }
        pub(msg::BootComplete{});

        auto now = clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - referenceTime);
        logI("Boot time: ", delta.count());
        tockTime = now;

        PROFILER_START
    }

    void initValue() {
        Value::addBasicConverters();
        Value::addConverter([](const String& str) -> Rect {return str;});
        Value::addConverter([](const String& str) -> FunctionRef<void()> {return str;});
        Value::addConverter([](std::shared_ptr<script::EngineObjRef> eor) -> FunctionRef<void()>{
                return std::function([=]{eor->call({});});
        });

        Value::addConverter([](const String& str) -> Fit {
            if (str == "fit") return Fit::fit;
            if (str == "stretch") return Fit::stretch;
            if (str == "cover") return Fit::cover;
            if (str == "tile") return Fit::tile;
            return Fit::stretch;
        });
        Value::addConverter([](Fit fit) -> String {
            switch (fit) {
            case Fit::tile: return "tile";
            case Fit::fit: return "fit";
            case Fit::stretch: return "stretch";
            case Fit::cover: return "cover";
            }
            return "stretch";
        });

        ui::Unit::addConverters();
        Color::addConverters();
    }

    bool run() override {
        PROFILER
        pub(msg::Tick{});
        pub(msg::PostTick{});

        clock::time_point now = clock::now();

        framesUntilTock++;
        if (now - tockTime >= std::chrono::seconds(1)) {
            PROFILER_INFO("FPS: " + std::to_string(framesUntilTock));
            PROFILER_END;
            logV("FPS: ", framesUntilTock);
            pub(msg::Tock{});
            framesUntilTock = 0;
            PROFILER_START;
            tockTime = clock::now();
        }

#if !defined(EMSCRIPTEN) && !defined(__N3DS__)
        if (running) {
            auto delta = now - referenceTime;
            referenceTime = now;
            if (delta < std::chrono::milliseconds{1000 / 60}) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1000 / 60} - delta);
            }
        }
#endif

        return running;
    }

    void on(msg::RequestShutdown&) {
        running = false;
    }

    ~AppImpl() {
        pub(msg::Shutdown{});
    }
};

static App::Shared<AppImpl> app{"dotto"};

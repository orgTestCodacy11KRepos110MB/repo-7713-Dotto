// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <app/App.hpp>
#include <log/Log.hpp>

class AppImpl : public App {
public:
    bool running = true;
    inject<Log> log;

    void boot(int argc, const char* argv[]) override {
        bootLogger();
        Log::write(Log::Level::VERBOSE, "Booting complete");
    }

    bool run() override {
        Log::write(Log::Level::VERBOSE, "Running Dotto!");
        return running;
    }

    void bootLogger() {
        log->setGlobal();
        log->setLevel(Log::Level::VERBOSE); // TODO: Configure level using args
    }
};

static App::Shared<AppImpl> app{"dotto"};

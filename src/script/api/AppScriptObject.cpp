// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <sstream>
#include <optional>

#include <cmd/Command.hpp>
#include <common/Messages.hpp>
#include <common/PubSub.hpp>
#include <common/PropertySet.hpp>
#include <fs/FileSystem.hpp>
#include <gui/Node.hpp>
#include <script/Engine.hpp>
#include <script/Function.hpp>
#include <script/ScriptObject.hpp>
#include <tools/Tool.hpp>
#include <script/api/AppScriptObject.hpp>

class AppScriptObjectImpl : public AppScriptObject {
public:
    script::Value target;
    std::optional<PubSub<msg::Tick>> tick;

    void setTarget(const ::Value& target) override {
        this->target = getEngine().toValue(target);
    }

    AppScriptObjectImpl() {
        if (auto scripttarget = inject<script::ScriptTarget>{InjectSilent::Yes}) {
            setTarget(scripttarget->target);
        }

        addProperty("target", [this]{return target;});

        addFunction("command", [](const String& name){
            inject<Command> command{name};
            if (!command)
                return false;
            PropertySet properties;
            auto& args = script::Function::varArgs();
            for (std::size_t i = 1, size = args.size(); i + 1 < size; i += 2) {
                properties.set(args[i], args[i + 1].str());
            }
            command->load(properties);
            command->run();
            return true;
        });

        addFunction("addTool", [=](const String& name) {
            script::Engine::Provides engine{getEngine().shared_from_this(), "toolengine"};
            script::ScriptObject::Provides app{shared_from_this(), "toolapp"};
            inject<Tool>{"script"}->init(name);
            return true;
        });

        addFunction("addEventListener", [=](const String& name) {
            if (name == "tick") {
                if (!tick)
                    tick = this;
            }
            return true;
        });

        addFunction("parse", [=](const String& path) {
            return getEngine().toValue(FileSystem::parse(path));
        });

        addFunction("openWindow", [=](const String& name) -> script::Value {
            return ui::Node::fromXML(name) != nullptr;
        });

        makeGlobal("app");
    }

    void on(msg::Tick&) {getEngine().raiseEvent({"tick"});}
};

static script::ScriptObject::Shared<AppScriptObjectImpl> reg("AppScriptObject", {"global"});

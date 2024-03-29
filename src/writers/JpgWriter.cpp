// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef USE_SDL2

#include <common/Surface.hpp>
#include <common/Writer.hpp>
#include <doc/Document.hpp>
#include <sdl2/SDL2Image.hpp>

class JpgWriter : public SimpleImageWriter {
public:
    bool writeFile(std::shared_ptr<File> file, const Value& data) override {
        return SDLImage::instance().saveJPG(file, data, 90);
    }
};

static Writer::Shared<JpgWriter> jpg{"jpg", {
        "*.jpg",
        typeid(std::shared_ptr<Surface>).name(),
        typeid(std::shared_ptr<Document>).name()
    }
};

static Writer::Shared<JpgWriter> jpeg{"jpeg", {
        "*.jpeg",
        typeid(std::shared_ptr<Surface>).name(),
        typeid(std::shared_ptr<Document>).name()
    }
};

#endif

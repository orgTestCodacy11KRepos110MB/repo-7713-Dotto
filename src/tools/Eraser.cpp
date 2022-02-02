// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <tools/Pencil.hpp>

class Surface;

class Eraser : public Pencil {
public:
    virtual void initPaint() {
        paint = inject<Command>{"paint"};
        paint->load({
                {"selection", selection},
                {"mode", "erase"},
                {"preview", true}
            });
    }
};

static Tool::Shared<Eraser> erase{"eraser"};
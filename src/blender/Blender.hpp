// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <memory>

#include <common/inject.hpp>
#include <common/Surface.hpp>
#include <common/Profiler.hpp>

class Blender : public Injectable<Blender>, public std::enable_shared_from_this<Blender> {
public:
    static inline Color color;
    virtual Surface::PixelType blendPixel(const Color& a, const Color& b, F32 alpha) = 0;
    virtual void blend(Surface* result, Surface* low, Surface* high, F32 alpha, const Rect& area) {
        PROFILER;
        auto a = low->data(), b = high->data(), c = result->data();
        U32 stride = result->width();
        Color ca, cb;
        for (U32 y = 0; y < area.height; ++y) {
            U32 i = (y + area.y) * stride + area.x;
            for (U32 x = 0; x < area.width; ++x) {
                cb.fromU32(b[i + x]);
                if (cb.a) {
                    ca.fromU32(a[i + x]);
                    c[i + x] = blendPixel(ca, cb, alpha);
                } else {
                    c[i + x] = a[i + x];
                }
            }
        }
        result->setDirty(result->rect());
    }
};

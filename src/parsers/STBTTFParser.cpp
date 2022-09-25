// Copyright (c) 2021 LibreSprite Authors (cf. AUTHORS.md)
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#if defined(USE_STBTTF)

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype/stb_truetype.h>

#include <common/inject.hpp>
#include <common/Parser.hpp>
#include <common/Font.hpp>
#include <log/Log.hpp>

using namespace fs;

class Glyph {

public:
    Vector<U8> data;
    U32 width, height;
    S32 bearingX, bearingY;
    S32 advance;

    void blitTo(S32& offsetX, S32& offsetY, const Color& color, Surface& target, U8 threshold = 0) {
        if (threshold == 0) {
            for (U32 y = 0; y < height; ++y) {
                for (U32 x = 0; x < width; ++x) {
                    auto alpha = data[y * width + x];
                    target.setPixel(x + offsetX + bearingX,
                                    y + offsetY + bearingY,
                                    Color(color.r, color.g, color.b, alpha));
                }
            }
        } else {
            for (U32 y = 0; y < height; ++y) {
                for (U32 x = 0; x < width; ++x) {
                    if (data[y * width + x] < threshold)
                        continue;
                    target.setPixel(x + offsetX + bearingX,
                                    y + offsetY + bearingY,
                                    color);
                }
            }
        }
        offsetX += advance;
    }
};

class TTFFont : public Font {
public:

    stbtt_fontinfo font;
    bool OK = false;
    U32 currentSize = ~U32{};
    HashMap<U32, std::shared_ptr<Glyph>> glyphCache;
    Vector<U8> faceData;

    TTFFont(File& file) {
        faceData.resize(file.size());
        file.read(&faceData[0], faceData.size());

        OK = stbtt_InitFont(&font, &faceData[0], 0); // stbtt_GetFontOffsetForIndex(&faceData[0], 0)
        if (!OK) {
            logE("Failed to load face ", file.getUID());
        }
    }

    U32 getGlyphIndex(const String& text, U32& offset, U32& glyph) {
        glyph = text[offset];
        U32 extras = 0;
        if (glyph & 0b1000'0000) {
            U32 max = text.size();
            if (!(glyph & 0b0010'0000)) { // 110xxxxx 10xxxxxx
                glyph &= 0b11111;
                extras = 1;
            } else if (!(glyph & 0b0001'0000)) { // 1110xxxx 10xxxxxx 10xxxxxx
                glyph &= 0b1111;
                extras = 2;
            } else { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                glyph &= 0b111;
                extras = 3;
            }
            for (U32 i = 0; i < extras; ++i) {
                if (++offset < max) {
                    glyph <<= 6;
                    glyph |= text[offset] & 0b111111;
                }
            }
        }
        return stbtt_FindGlyphIndex(&font, glyph);
    }

    F32 scale = 1.0f;
    void setSize(U32 size) {
        if (size == currentSize)
            return;
        scale = stbtt_ScaleForMappingEmToPixels(&font, size);
        currentSize = size;
        glyphCache.clear();
    }
                                                                                                                           Glyph* loadGlyph(const String& text, U32& offset) {
        U32 utf8 = 0;
        U32 glyphIndex = getGlyphIndex(text, offset, utf8);
        if (glyphIndex == 0)
            return nullptr;

        auto it = glyphCache.find(glyphIndex);
        if (it != glyphCache.end())
            return it->second.get();

        int advance, lsb;
        stbtt_GetGlyphHMetrics(&font, glyphIndex, &advance, &lsb);

        int x, y, w, h;
        auto bitmap = stbtt_GetGlyphBitmap(&font, scale, scale, glyphIndex, &w, &h, &x, &y);

        auto glyph = std::make_shared<Glyph>();
        glyphCache[glyphIndex] = glyph;

        glyph->data.resize(w * h);
        glyph->bearingX = x;
        glyph->bearingY = y;
        glyph->advance = advance * scale + 0.5f;
        glyph->width = w;
        glyph->height = h;

        if (bitmap) {
            for (U32 y = 0; y < h; ++y) {
                for (U32 x = 0; x < w; ++x) {
                    glyph->data[y * w + x] = bitmap[y * w + x];
                }
            }
            free(bitmap);
        }

        return glyph.get();
    }

    virtual std::shared_ptr<Surface> print(U32 size, const Color& color, const String& text, const Rect& padding, Vector<S32>& advance) {
        advance.clear();
        if (text.empty())
            return nullptr;
        auto surface = std::make_shared<Surface>();
        setSize(size);

        struct Entity {
            Glyph* glyph;
            Color color;
            bool hasAdvance;
        } entity;
        entity.color = color;
        entity.hasAdvance = true;
        Vector<Entity> glyphs;

        U32 width = 0;
        U32 height = 0;
        U32 escapeDepth = 0;
        U32 escapeStart = 0;
        U32 escapeEnd = 0;
        U32 maxWidth = 0;
        for (U32 i = 0, len = text.size(); i < len; ++i) {
            if (escapeDepth) {
                if (text[i] == '[') {
                    escapeDepth++;
                } else if (text[i] == ']') {
                    escapeDepth--;
                    if (escapeDepth == 1) {
                        escapeDepth = 0;
                        escapeEnd = i - 1;
                        auto code = std::string_view{text}.substr(escapeStart, escapeEnd - escapeStart + 1);
                        if (code == "zw") {
                            entity.hasAdvance = false;
                        } else if (code == "w" ) {
                            entity.hasAdvance = true;
                        } else if (code == "r") {
                            entity.hasAdvance = true;
                            entity.color = color;
                        } else {
                            entity.color.fromString(String{code});
                        }
                    }
                }
                continue;
            }
            if (text[i] == '\x1B') {
                escapeDepth = 1;
                escapeStart = i + 2;
                continue;
            }
            if (auto glyph = loadGlyph(text, i)) {
                entity.glyph = glyph;
                glyphs.push_back(entity);
                advance.push_back(entity.hasAdvance ? glyph->advance : 0);
                maxWidth = std::max(maxWidth, width + glyph->advance);
                if (entity.hasAdvance) {
                    width += glyph->advance;
                }
                height = std::max(height, size + (glyph->height - glyph->bearingY));
            }
        }
        width = std::max(maxWidth, width);
        if (!advance.empty()) {
            advance[0] += padding.x;
        }
        surface->resize(width + padding.x + padding.width, height + padding.y + padding.height);
        S32 x = padding.x, y = size + padding.y;
        for (auto& entity : glyphs) {
            entity.glyph->blitTo(x, y, entity.color, *surface);
        }
        return surface;
    }
};

class STBTTFFontParser : public Parser {
public:
    Value parseFile(std::shared_ptr<File> file) override {
        auto font = std::make_shared<TTFFont>(*file);
        return font->OK ? std::static_pointer_cast<Font>(font) : nullptr;
    }
};

static Parser::Shared<STBTTFFontParser> font{"ttf"};

#endif

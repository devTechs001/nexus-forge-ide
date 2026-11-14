// ui/renderer/software_backend.cpp
// Cross-platform CPU 2D software renderer with optional FreeType font rendering.

#include "software_backend.hpp"
#include "../../core/platform/platform_abstraction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <unordered_map>

#if defined(NEXUS_HAS_FREETYPE) && defined(__has_include)
    #if __has_include(<ft2build.h>)
        #define NEXUS_USE_FREETYPE_RASTER 1
        #include <ft2build.h>
        #include FT_FREETYPE_H
    #endif
#endif

namespace NexusForge::UI {

namespace {

inline uint8_t colorToByte(float v) {
    float c = std::clamp(v, 0.0f, 1.0f);
    return static_cast<uint8_t>(c * 255.0f + 0.5f);
}

inline uint32_t packBGRA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8)  |
           static_cast<uint32_t>(b);
}

// Clean 8x8 bitmap font for printable ASCII 32..126 (MSB = left pixel, 8 rows top-to-bottom)
constexpr uint8_t FONT_8X8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
    {0x36,0x36,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 35 '#'
    {0x0C,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // 36 '$'
    {0x62,0x64,0x08,0x10,0x26,0x46,0x00,0x00}, // 37 '%'
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 38 '&'
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '\''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
    {0x02,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // 47 '/'
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 48 '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 '1'
    {0x3C,0x66,0x06,0x1C,0x30,0x60,0x7E,0x00}, // 50 '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 51 '3'
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, // 52 '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 53 '5'
    {0x3C,0x60,0x7C,0x66,0x66,0x66,0x3C,0x00}, // 54 '6'
    {0x7E,0x66,0x0C,0x18,0x18,0x18,0x18,0x00}, // 55 '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 56 '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, // 57 '9'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // 58 ':'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // 59 ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 '<'
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 61 '='
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 '>'
    {0x3C,0x66,0x06,0x1C,0x18,0x00,0x18,0x00}, // 63 '?'
    {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00}, // 64 '@'
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 65 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 66 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 67 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 68 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 69 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 70 'F'
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00}, // 71 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 72 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 73 'I'
    {0x1E,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00}, // 74 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 75 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 76 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 78 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 79 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 80 'P'
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // 81 'Q'
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 82 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 83 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 85 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 86 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 88 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 89 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 90 'Z'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 91 '['
    {0x40,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // 92 '\\'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 93 ']'
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00}, // 95 '_'
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 97 'a'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 'b'
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 99 'c'
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 'd'
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 'e'
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00}, // 102 'f'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // 103 'g'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 104 'h'
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 'i'
    {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x6C,0x38}, // 106 'j'
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 107 'k'
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 'l'
    {0x00,0x00,0x76,0x7F,0x6B,0x6B,0x63,0x00}, // 109 'm'
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 110 'n'
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 'o'
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 112 'p'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // 113 'q'
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 114 'r'
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 115 's'
    {0x30,0x30,0x7C,0x30,0x30,0x34,0x18,0x00}, // 116 't'
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 'u'
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 'v'
    {0x00,0x00,0x63,0x6B,0x6B,0x7F,0x36,0x00}, // 119 'w'
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 'x'
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}, // 121 'y'
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 'z'
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 '{'
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 124 '|'
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 '}'
    {0x32,0x4C,0x00,0x00,0x00,0x00,0x00,0x00}  // 126 '~'
};

// Sanitize common UTF-8 punctuation to clean ASCII for the built-in bitmap fallback
std::string sanitizeToAscii(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
            ++i;
        } else if (c == 0xC2 && i + 1 < input.size() && static_cast<unsigned char>(input[i + 1]) == 0xA9) {
            out += "(c)";
            i += 2;
        } else if (c == 0xE2 && i + 2 < input.size()) {
            unsigned char b1 = static_cast<unsigned char>(input[i + 1]);
            unsigned char b2 = static_cast<unsigned char>(input[i + 2]);
            if (b1 == 0x80 && (b2 == 0x93 || b2 == 0x94)) {
                out.push_back('-');
            } else if (b1 == 0x80 && b2 == 0xA2) {
                out.push_back('*');
            } else if (b1 == 0x80 && b2 == 0xA6) {
                out += "...";
            } else if (b1 == 0x86 && b2 == 0x92) {
                out += "->";
            }
            i += 3;
        } else {
            // Skip other multi-byte UTF-8 sequences cleanly
            if ((c & 0xE0) == 0xC0) i += 2;
            else if ((c & 0xF0) == 0xE0) i += 3;
            else if ((c & 0xF8) == 0xF0) i += 4;
            else ++i;
        }
    }
    return out;
}

inline float sampleFont8x8Bilinear(const uint8_t glyph[8], float u, float v) {
    // u, v in [0, 8)
    float gx = u - 0.5f;
    float gy = v - 0.5f;
    int x0 = static_cast<int>(std::floor(gx));
    int y0 = static_cast<int>(std::floor(gy));
    float fx = gx - static_cast<float>(x0);
    float fy = gy - static_cast<float>(y0);

    auto fetch = [&](int x, int y) -> float {
        if (x < 0 || x >= 8 || y < 0 || y >= 8) return 0.0f;
        return ((glyph[y] >> (7 - x)) & 1) ? 1.0f : 0.0f;
    };

    float c00 = fetch(x0, y0);
    float c10 = fetch(x0 + 1, y0);
    float c01 = fetch(x0, y0 + 1);
    float c11 = fetch(x0 + 1, y0 + 1);
    float top = c00 * (1.0f - fx) + c10 * fx;
    float bot = c01 * (1.0f - fx) + c11 * fx;
    return top * (1.0f - fy) + bot * fy;
}

} // anonymous namespace

struct SoftwareRenderBackend::FreeTypeState {
#ifdef NEXUS_USE_FREETYPE_RASTER
    struct CachedGlyph {
        int width = 0;
        int height = 0;
        int bearingX = 0;
        int bearingY = 0;
        float advance = 0.0f;
        std::vector<uint8_t> bitmap;
    };

    FT_Library library = nullptr;
    FT_Face sansFace = nullptr;
    FT_Face monoFace = nullptr;
    bool ready = false;
    mutable std::unordered_map<uint64_t, CachedGlyph> cache;

    bool init() {
        if (FT_Init_FreeType(&library) != 0) {
            library = nullptr;
            return false;
        }

        static const char* sansCandidates[] = {
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
            "/usr/share/fonts/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/TTF/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/cantarell/Cantarell-Regular.otf",
            "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf"
        };

        static const char* monoCandidates[] = {
            "/usr/share/fonts/TTF/JetBrainsMono-Regular.ttf",
            "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/dejavu-sans-mono-fonts/DejaVuSansMono.ttf",
            "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
            "/usr/share/fonts/TTF/LiberationMono-Regular.ttf",
            "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf"
        };

        for (const char* path : sansCandidates) {
            if (FT_New_Face(library, path, 0, &sansFace) == 0) break;
            sansFace = nullptr;
        }
        for (const char* path : monoCandidates) {
            if (FT_New_Face(library, path, 0, &monoFace) == 0) break;
            monoFace = nullptr;
        }
        if (!sansFace && monoFace) {
            sansFace = monoFace;
        }
        ready = (sansFace != nullptr);
        return ready;
    }

    ~FreeTypeState() {
        if (monoFace && monoFace != sansFace) FT_Done_Face(monoFace);
        if (sansFace) FT_Done_Face(sansFace);
        if (library) FT_Done_FreeType(library);
    }

    FT_Face selectFace(const std::string& family) const {
        if (monoFace && (family == "mono" || family == "monospace" || family == "code")) {
            return monoFace;
        }
        return sansFace ? sansFace : monoFace;
    }

    const CachedGlyph* getGlyph(const std::string& family, int pixelSize, uint32_t cp) const {
        if (!ready) return nullptr;
        FT_Face face = selectFace(family);
        if (!face) return nullptr;

        int clampedPx = std::clamp(pixelSize, 8, 96);
        uint64_t faceId = (face == monoFace && monoFace != sansFace) ? 1ULL : 0ULL;
        uint64_t key = (faceId << 48) | (static_cast<uint64_t>(clampedPx) << 32) | static_cast<uint64_t>(cp);
        auto it = cache.find(key);
        if (it != cache.end()) return &it->second;

        FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(clampedPx));
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER) != 0) {
            return nullptr;
        }

        CachedGlyph g;
        g.width = static_cast<int>(face->glyph->bitmap.width);
        g.height = static_cast<int>(face->glyph->bitmap.rows);
        g.bearingX = face->glyph->bitmap_left;
        g.bearingY = face->glyph->bitmap_top;
        g.advance = static_cast<float>(face->glyph->advance.x) / 64.0f;
        if (g.width > 0 && g.height > 0 && face->glyph->bitmap.buffer) {
            g.bitmap.resize(static_cast<size_t>(g.width) * static_cast<size_t>(g.height));
            int pitch = std::abs(face->glyph->bitmap.pitch);
            for (int r = 0; r < g.height; ++r) {
                std::memcpy(
                    g.bitmap.data() + static_cast<size_t>(r) * static_cast<size_t>(g.width),
                    face->glyph->bitmap.buffer + static_cast<size_t>(r) * static_cast<size_t>(pitch),
                    static_cast<size_t>(g.width));
            }
        }
        auto [inserted, _] = cache.emplace(key, std::move(g));
        return &inserted->second;
    }
#else
    bool ready = false;
    bool init() { return false; }
#endif
};

SoftwareRenderBackend::SoftwareRenderBackend(void* nativeWindow)
    : nativeWindow_(nativeWindow) {
    transformStack_.push_back(Mat4::identity());
}

SoftwareRenderBackend::~SoftwareRenderBackend() {
    shutdown();
}

bool SoftwareRenderBackend::initialize() {
    ft_ = std::make_unique<FreeTypeState>();
    ft_->init();
    return true;
}

void SoftwareRenderBackend::shutdown() {
    ft_.reset();
    pixels_.clear();
    width_ = 0;
    height_ = 0;
}

void SoftwareRenderBackend::ensureFramebuffer(int w, int h) {
    int cw = std::clamp(w, 1, 8192);
    int ch = std::clamp(h, 1, 8192);
    if (cw != width_ || ch != height_) {
        width_ = cw;
        height_ = ch;
        pixels_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0xFF000000U);
    }
}

void SoftwareRenderBackend::beginFrame() {
    currentTransform_ = Mat4::identity();
    transformStack_.assign(1, Mat4::identity());
    hasClip_ = false;
    blendMode_ = BlendMode::Alpha;
}

void SoftwareRenderBackend::endFrame() {
    // Nothing pending; commands rasterize directly into pixels_
}

void SoftwareRenderBackend::present() {
    if (nativeWindow_ && width_ > 0 && height_ > 0 && !pixels_.empty()) {
        Platform::PlatformAbstraction::presentFramebuffer(nativeWindow_, pixels_.data(), width_, height_);
    }
}

void SoftwareRenderBackend::setViewport(int x, int y, int width, int height) {
    (void)x;
    (void)y;
    ensureFramebuffer(width, height);
}

void SoftwareRenderBackend::clear(const Color& color) {
    if (pixels_.empty()) return;
    uint32_t packed = packBGRA(colorToByte(color.r), colorToByte(color.g), colorToByte(color.b), 255);
    std::fill(pixels_.begin(), pixels_.end(), packed);
}

Vec2 SoftwareRenderBackend::transformPoint(float x, float y) const {
    const float* m = currentTransform_.m;
    return Vec2(m[0] * x + m[4] * y + m[12],
                m[1] * x + m[5] * y + m[13]);
}

float SoftwareRenderBackend::transformScale() const {
    const float* m = currentTransform_.m;
    float sx = std::sqrt(m[0] * m[0] + m[1] * m[1]);
    float sy = std::sqrt(m[4] * m[4] + m[5] * m[5]);
    return std::max(0.01f, 0.5f * (sx + sy));
}

void SoftwareRenderBackend::blendPixel(int x, int y, uint8_t sr, uint8_t sg, uint8_t sb, float sa) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    if (hasClip_) {
        if (x < static_cast<int>(std::floor(clipRect_.x)) ||
            y < static_cast<int>(std::floor(clipRect_.y)) ||
            x >= static_cast<int>(std::ceil(clipRect_.right())) ||
            y >= static_cast<int>(std::ceil(clipRect_.bottom()))) {
            return;
        }
    }
    if (sa <= 0.001f) return;

    uint32_t& dst = pixels_[static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x)];
    if (sa >= 0.999f && blendMode_ != BlendMode::Additive) {
        dst = packBGRA(sr, sg, sb, 255);
        return;
    }

    uint32_t d = dst;
    uint8_t dr = static_cast<uint8_t>((d >> 16) & 0xFF);
    uint8_t dg = static_cast<uint8_t>((d >> 8) & 0xFF);
    uint8_t db = static_cast<uint8_t>(d & 0xFF);

    uint8_t outR, outG, outB;
    if (blendMode_ == BlendMode::Additive) {
        outR = static_cast<uint8_t>(std::min(255, static_cast<int>(dr) + static_cast<int>(sr * sa)));
        outG = static_cast<uint8_t>(std::min(255, static_cast<int>(dg) + static_cast<int>(sg * sa)));
        outB = static_cast<uint8_t>(std::min(255, static_cast<int>(db) + static_cast<int>(sb * sa)));
    } else {
        float inv = 1.0f - sa;
        outR = static_cast<uint8_t>(sr * sa + dr * inv + 0.5f);
        outG = static_cast<uint8_t>(sg * sa + dg * inv + 0.5f);
        outB = static_cast<uint8_t>(sb * sa + db * inv + 0.5f);
    }
    dst = packBGRA(outR, outG, outB, 255);
}

void SoftwareRenderBackend::fillSpan(int y, int x0, int x1, uint8_t sr, uint8_t sg, uint8_t sb, float sa) {
    if (y < 0 || y >= height_ || sa <= 0.001f) return;
    int minX = 0;
    int maxX = width_;
    if (hasClip_) {
        if (y < static_cast<int>(std::floor(clipRect_.y)) ||
            y >= static_cast<int>(std::ceil(clipRect_.bottom()))) {
            return;
        }
        minX = std::max(minX, static_cast<int>(std::floor(clipRect_.x)));
        maxX = std::min(maxX, static_cast<int>(std::ceil(clipRect_.right())));
    }
    int startX = std::max(minX, x0);
    int endX = std::min(maxX, x1);
    if (startX >= endX) return;

    uint32_t* row = pixels_.data() + static_cast<size_t>(y) * static_cast<size_t>(width_);
    if (sa >= 0.999f && blendMode_ != BlendMode::Additive) {
        uint32_t packed = packBGRA(sr, sg, sb, 255);
        std::fill(row + startX, row + endX, packed);
        return;
    }

    float inv = 1.0f - sa;
    float srf = sr * sa;
    float sgf = sg * sa;
    float sbf = sb * sa;
    for (int x = startX; x < endX; ++x) {
        uint32_t d = row[x];
        uint8_t dr = static_cast<uint8_t>((d >> 16) & 0xFF);
        uint8_t dg = static_cast<uint8_t>((d >> 8) & 0xFF);
        uint8_t db = static_cast<uint8_t>(d & 0xFF);
        uint8_t r, g, b;
        if (blendMode_ == BlendMode::Additive) {
            r = static_cast<uint8_t>(std::min(255, static_cast<int>(dr + srf)));
            g = static_cast<uint8_t>(std::min(255, static_cast<int>(dg + sgf)));
            b = static_cast<uint8_t>(std::min(255, static_cast<int>(db + sbf)));
        } else {
            r = static_cast<uint8_t>(srf + dr * inv + 0.5f);
            g = static_cast<uint8_t>(sgf + dg * inv + 0.5f);
            b = static_cast<uint8_t>(sbf + db * inv + 0.5f);
        }
        row[x] = packBGRA(r, g, b, 255);
    }
}

void SoftwareRenderBackend::draw(const DrawCommand& cmd) {
    switch (cmd.type) {
        case DrawCommandType::DrawRect:
            drawRectImpl(cmd.rect, cmd.color);
            break;
        case DrawCommandType::DrawRoundedRect:
            drawRoundedRectImpl(cmd.rect, cmd.color, cmd.cornerRadius);
            break;
        case DrawCommandType::DrawCircle:
            drawCircleImpl(cmd.rect, cmd.color, cmd.filled, cmd.thickness);
            break;
        case DrawCommandType::DrawLine:
            drawLineImpl(cmd.rect.x, cmd.rect.y,
                         cmd.rect.x + cmd.rect.width, cmd.rect.y + cmd.rect.height,
                         cmd.color, cmd.thickness);
            break;
        case DrawCommandType::DrawTriangle:
        case DrawCommandType::DrawPath:
            drawPathImpl(cmd);
            break;
        case DrawCommandType::DrawGradient:
            drawGradientImpl(cmd);
            break;
        case DrawCommandType::DrawText:
            drawTextImpl(cmd);
            break;
        case DrawCommandType::SetClipRect:
            clipRect_ = cmd.clipRect;
            hasClip_ = true;
            break;
        case DrawCommandType::ClearClipRect:
            hasClip_ = false;
            break;
        case DrawCommandType::PushTransform:
            transformStack_.push_back(currentTransform_);
            break;
        case DrawCommandType::PopTransform:
            if (!transformStack_.empty()) {
                currentTransform_ = transformStack_.back();
                transformStack_.pop_back();
            }
            break;
        case DrawCommandType::SetTransform:
            currentTransform_ = cmd.transform;
            break;
        case DrawCommandType::SetBlendMode:
            blendMode_ = cmd.blendMode;
            break;
        default:
            break;
    }
}

void SoftwareRenderBackend::drawRectImpl(const Rect& rect, const Color& color) {
    if (rect.width <= 0.0f || rect.height <= 0.0f || color.a <= 0.001f) return;
    Vec2 p0 = transformPoint(rect.x, rect.y);
    Vec2 p1 = transformPoint(rect.right(), rect.bottom());
    int x0 = static_cast<int>(std::floor(std::min(p0.x, p1.x)));
    int y0 = static_cast<int>(std::floor(std::min(p0.y, p1.y)));
    int x1 = static_cast<int>(std::ceil(std::max(p0.x, p1.x)));
    int y1 = static_cast<int>(std::ceil(std::max(p0.y, p1.y)));
    uint8_t r = colorToByte(color.r);
    uint8_t g = colorToByte(color.g);
    uint8_t b = colorToByte(color.b);
    float a = std::clamp(color.a, 0.0f, 1.0f);
    for (int y = std::max(0, y0); y < std::min(height_, y1); ++y) {
        fillSpan(y, x0, x1, r, g, b, a);
    }
}

void SoftwareRenderBackend::drawRoundedRectImpl(const Rect& rect, const Color& color, float radius) {
    if (rect.width <= 0.0f || rect.height <= 0.0f || color.a <= 0.001f) return;
    Vec2 p0 = transformPoint(rect.x, rect.y);
    Vec2 p1 = transformPoint(rect.right(), rect.bottom());
    float left = std::min(p0.x, p1.x);
    float top = std::min(p0.y, p1.y);
    float right = std::max(p0.x, p1.x);
    float bottom = std::max(p0.y, p1.y);
    float w = right - left;
    float h = bottom - top;
    float rad = std::min({radius * transformScale(), w * 0.5f, h * 0.5f});
    if (rad <= 0.5f) {
        drawRectImpl(rect, color);
        return;
    }

    uint8_t sr = colorToByte(color.r);
    uint8_t sg = colorToByte(color.g);
    uint8_t sb = colorToByte(color.b);
    float baseA = std::clamp(color.a, 0.0f, 1.0f);

    float cx = left + w * 0.5f;
    float cy = top + h * 0.5f;
    float halfW = w * 0.5f - rad;
    float halfH = h * 0.5f - rad;

    int y0 = std::max(0, static_cast<int>(std::floor(top)));
    int y1 = std::min(height_, static_cast<int>(std::ceil(bottom)));
    int x0 = std::max(0, static_cast<int>(std::floor(left)));
    int x1 = std::min(width_, static_cast<int>(std::ceil(right)));
    int innerTop = static_cast<int>(std::ceil(top + rad));
    int innerBot = static_cast<int>(std::floor(bottom - rad));

    for (int y = y0; y < y1; ++y) {
        if (y >= innerTop && y < innerBot) {
            fillSpan(y, x0, x1, sr, sg, sb, baseA);
            continue;
        }
        float py = static_cast<float>(y) + 0.5f;
        float dy = std::max(0.0f, std::abs(py - cy) - halfH);
        for (int x = x0; x < x1; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float dx = std::max(0.0f, std::abs(px - cx) - halfW);
            float dist = std::sqrt(dx * dx + dy * dy) - rad;
            float cov = std::clamp(0.5f - dist, 0.0f, 1.0f);
            if (cov > 0.001f) {
                blendPixel(x, y, sr, sg, sb, baseA * cov);
            }
        }
    }
}

void SoftwareRenderBackend::drawCircleImpl(const Rect& rect, const Color& color, bool filled, float thickness) {
    if (rect.width <= 0.0f || rect.height <= 0.0f || color.a <= 0.001f) return;
    Vec2 center = transformPoint(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f);
    float rad = 0.5f * std::min(rect.width, rect.height) * transformScale();
    if (rad <= 0.1f) return;

    uint8_t sr = colorToByte(color.r);
    uint8_t sg = colorToByte(color.g);
    uint8_t sb = colorToByte(color.b);
    float baseA = std::clamp(color.a, 0.0f, 1.0f);
    float halfThick = std::max(0.5f, thickness * 0.5f * transformScale());
    float outer = filled ? (rad + 1.5f) : (rad + halfThick + 1.5f);

    int x0 = std::max(0, static_cast<int>(std::floor(center.x - outer)));
    int x1 = std::min(width_, static_cast<int>(std::ceil(center.x + outer)));
    int y0 = std::max(0, static_cast<int>(std::floor(center.y - outer)));
    int y1 = std::min(height_, static_cast<int>(std::ceil(center.y + outer)));

    for (int y = y0; y < y1; ++y) {
        float dy = (static_cast<float>(y) + 0.5f) - center.y;
        for (int x = x0; x < x1; ++x) {
            float dx = (static_cast<float>(x) + 0.5f) - center.x;
            float d = std::sqrt(dx * dx + dy * dy);
            float cov = 0.0f;
            if (filled) {
                cov = std::clamp(rad + 0.5f - d, 0.0f, 1.0f);
            } else {
                cov = std::clamp(halfThick + 0.5f - std::abs(d - rad), 0.0f, 1.0f);
            }
            if (cov > 0.001f) {
                blendPixel(x, y, sr, sg, sb, baseA * cov);
            }
        }
    }
}

void SoftwareRenderBackend::drawLineImpl(float x1, float y1, float x2, float y2,
                                         const Color& color, float thickness) {
    if (color.a <= 0.001f) return;
    Vec2 a = transformPoint(x1, y1);
    Vec2 b = transformPoint(x2, y2);
    float halfW = std::max(0.5f, thickness * 0.5f * transformScale());
    uint8_t sr = colorToByte(color.r);
    uint8_t sg = colorToByte(color.g);
    uint8_t sb = colorToByte(color.b);
    float baseA = std::clamp(color.a, 0.0f, 1.0f);

    float minX = std::min(a.x, b.x) - halfW - 1.5f;
    float maxX = std::max(a.x, b.x) + halfW + 1.5f;
    float minY = std::min(a.y, b.y) - halfW - 1.5f;
    float maxY = std::max(a.y, b.y) + halfW + 1.5f;

    int ix0 = std::max(0, static_cast<int>(std::floor(minX)));
    int ix1 = std::min(width_, static_cast<int>(std::ceil(maxX)));
    int iy0 = std::max(0, static_cast<int>(std::floor(minY)));
    int iy1 = std::min(height_, static_cast<int>(std::ceil(maxY)));

    float vx = b.x - a.x;
    float vy = b.y - a.y;
    float len2 = vx * vx + vy * vy;

    for (int y = iy0; y < iy1; ++y) {
        float py = static_cast<float>(y) + 0.5f;
        for (int x = ix0; x < ix1; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float t = (len2 > 1e-6f) ? std::clamp(((px - a.x) * vx + (py - a.y) * vy) / len2, 0.0f, 1.0f) : 0.0f;
            float dx = px - (a.x + vx * t);
            float dy = py - (a.y + vy * t);
            float dist = std::sqrt(dx * dx + dy * dy);
            float cov = std::clamp(halfW + 0.5f - dist, 0.0f, 1.0f);
            if (cov > 0.001f) {
                blendPixel(x, y, sr, sg, sb, baseA * cov);
            }
        }
    }
}

Color SoftwareRenderBackend::evaluateGradient(const Gradient& g, float t) const {
    float ct = std::clamp(t, 0.0f, 1.0f);
    if (g.stops.empty()) return Color::White();
    if (g.stops.size() == 1 || ct <= g.stops.front().first) return g.stops.front().second;
    if (ct >= g.stops.back().first) return g.stops.back().second;

    for (size_t i = 0; i + 1 < g.stops.size(); ++i) {
        float t0 = g.stops[i].first;
        float t1 = g.stops[i + 1].first;
        if (ct >= t0 && ct <= t1) {
            float span = std::max(1e-6f, t1 - t0);
            float f = (ct - t0) / span;
            const Color& c0 = g.stops[i].second;
            const Color& c1 = g.stops[i + 1].second;
            return Color(
                c0.r + (c1.r - c0.r) * f,
                c0.g + (c1.g - c0.g) * f,
                c0.b + (c1.b - c0.b) * f,
                c0.a + (c1.a - c0.a) * f);
        }
    }
    return g.stops.back().second;
}

void SoftwareRenderBackend::drawGradientImpl(const DrawCommand& cmd) {
    const Rect& rect = cmd.rect;
    if (rect.width <= 0.0f || rect.height <= 0.0f) return;

    Vec2 p0 = transformPoint(rect.x, rect.y);
    Vec2 p1 = transformPoint(rect.right(), rect.bottom());
    int x0 = std::max(0, static_cast<int>(std::floor(std::min(p0.x, p1.x))));
    int y0 = std::max(0, static_cast<int>(std::floor(std::min(p0.y, p1.y))));
    int x1 = std::min(width_, static_cast<int>(std::ceil(std::max(p0.x, p1.x))));
    int y1 = std::min(height_, static_cast<int>(std::ceil(std::max(p0.y, p1.y))));
    if (x0 >= x1 || y0 >= y1) return;

    Gradient g = cmd.gradient;
    if (g.stops.empty()) {
        g.stops = {{0.0f, cmd.color}, {1.0f, cmd.color2}};
    }

    if (g.type == GradientType::Radial) {
        Vec2 center = transformPoint(g.startPoint.x, g.startPoint.y);
        float rad = std::max(1.0f, g.radius * transformScale());
        for (int y = y0; y < y1; ++y) {
            float dy = (static_cast<float>(y) + 0.5f) - center.y;
            for (int x = x0; x < x1; ++x) {
                float dx = (static_cast<float>(x) + 0.5f) - center.x;
                float t = std::sqrt(dx * dx + dy * dy) / rad;
                Color c = evaluateGradient(g, t);
                if (c.a > 0.001f) {
                    blendPixel(x, y, colorToByte(c.r), colorToByte(c.g), colorToByte(c.b), c.a);
                }
            }
        }
        return;
    }

    Vec2 s = transformPoint(g.startPoint.x, g.startPoint.y);
    Vec2 e = transformPoint(g.endPoint.x, g.endPoint.y);
    float vx = e.x - s.x;
    float vy = e.y - s.y;
    float len2 = vx * vx + vy * vy;

    // Fast path for pure vertical gradient
    if (std::abs(vx) < 0.5f && std::abs(vy) >= 0.5f) {
        for (int y = y0; y < y1; ++y) {
            float t = ((static_cast<float>(y) + 0.5f) - s.y) / vy;
            Color c = evaluateGradient(g, t);
            if (c.a > 0.001f) {
                fillSpan(y, x0, x1, colorToByte(c.r), colorToByte(c.g), colorToByte(c.b), c.a);
            }
        }
        return;
    }

    // General / horizontal linear gradient
    for (int y = y0; y < y1; ++y) {
        float py = static_cast<float>(y) + 0.5f;
        for (int x = x0; x < x1; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float t = (len2 > 1e-6f) ? ((px - s.x) * vx + (py - s.y) * vy) / len2 : 0.0f;
            Color c = evaluateGradient(g, t);
            if (c.a > 0.001f) {
                blendPixel(x, y, colorToByte(c.r), colorToByte(c.g), colorToByte(c.b), c.a);
            }
        }
    }
}

void SoftwareRenderBackend::drawPathImpl(const DrawCommand& cmd) {
    if (cmd.pathPoints.size() < 2 || cmd.color.a <= 0.001f) return;

    if (!cmd.filled) {
        for (size_t i = 0; i + 1 < cmd.pathPoints.size(); ++i) {
            drawLineImpl(cmd.pathPoints[i].x, cmd.pathPoints[i].y,
                         cmd.pathPoints[i + 1].x, cmd.pathPoints[i + 1].y,
                         cmd.color, cmd.thickness);
        }
        if (cmd.closePath && cmd.pathPoints.size() >= 3) {
            drawLineImpl(cmd.pathPoints.back().x, cmd.pathPoints.back().y,
                         cmd.pathPoints.front().x, cmd.pathPoints.front().y,
                         cmd.color, cmd.thickness);
        }
        return;
    }

    // Scanline polygon fill (even-odd rule) with transformed vertices
    std::vector<Vec2> pts;
    pts.reserve(cmd.pathPoints.size());
    float minY = 1e9f, maxY = -1e9f;
    for (const auto& p : cmd.pathPoints) {
        Vec2 tp = transformPoint(p.x, p.y);
        pts.push_back(tp);
        minY = std::min(minY, tp.y);
        maxY = std::max(maxY, tp.y);
    }
    if (pts.size() < 3) return;

    uint8_t sr = colorToByte(cmd.color.r);
    uint8_t sg = colorToByte(cmd.color.g);
    uint8_t sb = colorToByte(cmd.color.b);
    float baseA = std::clamp(cmd.color.a, 0.0f, 1.0f);

    int y0 = std::max(0, static_cast<int>(std::floor(minY)));
    int y1 = std::min(height_, static_cast<int>(std::ceil(maxY)));
    std::vector<float> intersections;

    for (int y = y0; y < y1; ++y) {
        float py = static_cast<float>(y) + 0.5f;
        intersections.clear();
        for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
            float yi = pts[i].y;
            float yj = pts[j].y;
            if ((yi <= py && yj > py) || (yj <= py && yi > py)) {
                float t = (py - yi) / (yj - yi);
                intersections.push_back(pts[i].x + t * (pts[j].x - pts[i].x));
            }
        }
        std::sort(intersections.begin(), intersections.end());
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            int sx0 = static_cast<int>(std::floor(intersections[i] + 0.5f));
            int sx1 = static_cast<int>(std::ceil(intersections[i + 1] - 0.5f));
            if (sx1 > sx0) {
                fillSpan(y, sx0, sx1, sr, sg, sb, baseA);
            }
        }
    }
}

bool SoftwareRenderBackend::measureText(const std::string& text, const std::string& fontFamily,
                                        float fontSize, Vec2& outSize) const {
    float sz = std::max(6.0f, fontSize);

#ifdef NEXUS_USE_FREETYPE_RASTER
    if (ft_ && ft_->ready) {
        int px = static_cast<int>(std::lround(sz));
        float maxW = 0.0f;
        float lineW = 0.0f;
        int lines = 1;
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c == '\n') {
                maxW = std::max(maxW, lineW);
                lineW = 0.0f;
                ++lines;
                continue;
            }
            if (c == '\t') {
                lineW += sz * 2.2f;
                continue;
            }
            if (c < 32 || c >= 127) continue;
            const auto* g = ft_->getGlyph(fontFamily, px, static_cast<uint32_t>(c));
            lineW += g ? g->advance : (sz * 0.55f);
        }
        maxW = std::max(maxW, lineW);
        outSize = Vec2(maxW, lines == 1 ? sz : static_cast<float>(lines) * sz * 1.25f);
        return true;
    }
#else
    (void)fontFamily;
#endif

    std::string clean = sanitizeToAscii(text);
    float charAdv = sz * 0.60f;
    float maxW = 0.0f;
    float lineW = 0.0f;
    int lines = 1;
    for (char ch : clean) {
        if (ch == '\n') {
            maxW = std::max(maxW, lineW);
            lineW = 0.0f;
            ++lines;
        } else if (ch == '\t') {
            lineW += charAdv * 4.0f;
        } else {
            lineW += charAdv;
        }
    }
    maxW = std::max(maxW, lineW);
    outSize = Vec2(maxW, lines == 1 ? sz : static_cast<float>(lines) * sz * 1.25f);
    return true;
}

void SoftwareRenderBackend::drawTextImpl(const DrawCommand& cmd) {
    if (cmd.text.empty() || cmd.color.a <= 0.001f) return;

    Vec2 origin = transformPoint(cmd.rect.x, cmd.rect.y);
    float scale = transformScale();
    float sz = std::max(6.0f, cmd.fontSize * scale);
    float maxWidth = (cmd.rect.width > 0.0f) ? (cmd.rect.width * scale) : 0.0f;

    uint8_t sr = colorToByte(cmd.color.r);
    uint8_t sg = colorToByte(cmd.color.g);
    uint8_t sb = colorToByte(cmd.color.b);
    float baseA = std::clamp(cmd.color.a, 0.0f, 1.0f);

#ifdef NEXUS_USE_FREETYPE_RASTER
    if (ft_ && ft_->ready) {
        int px = static_cast<int>(std::lround(sz));
        float cursorX = origin.x;
        float baselineY = origin.y + sz * 0.82f;
        float lineStep = sz * 1.25f;

        for (size_t i = 0; i < cmd.text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(cmd.text[i]);
            if (c == '\n') {
                cursorX = origin.x;
                baselineY += lineStep;
                continue;
            }
            if (c == '\t') {
                cursorX += sz * 2.2f;
                continue;
            }
            if (c < 32 || c >= 127) continue;
            const auto* g = ft_->getGlyph(cmd.fontFamily, px, static_cast<uint32_t>(c));
            if (!g) {
                cursorX += sz * 0.55f;
                continue;
            }
            if (maxWidth > 0.0f && (cursorX - origin.x + g->advance) > maxWidth) {
                break;
            }
            int gx = static_cast<int>(std::lround(cursorX)) + g->bearingX;
            int gy = static_cast<int>(std::lround(baselineY)) - g->bearingY;
            for (int r = 0; r < g->height; ++r) {
                int py = gy + r;
                if (py < 0 || py >= height_) continue;
                for (int col = 0; col < g->width; ++col) {
                    uint8_t cov = g->bitmap[static_cast<size_t>(r) * static_cast<size_t>(g->width) + static_cast<size_t>(col)];
                    if (cov > 0) {
                        blendPixel(gx + col, py, sr, sg, sb, baseA * (static_cast<float>(cov) / 255.0f));
                    }
                }
            }
            cursorX += g->advance;
        }
        return;
    }
#endif

    // Clean built-in 8x8 bitmap fallback with smooth bilinear scaling for larger headings
    std::string clean = sanitizeToAscii(cmd.text);
    float charW = sz * 0.58f;
    float charAdv = sz * 0.60f;
    float charH = sz * 0.88f;
    float lineStep = sz * 1.25f;
    float cursorX = origin.x;
    float cursorY = origin.y + (sz - charH) * 0.5f;
    bool smooth = (sz >= 17.0f);

    for (char ch : clean) {
        if (ch == '\n') {
            cursorX = origin.x;
            cursorY += lineStep;
            continue;
        }
        if (ch == '\t') {
            cursorX += charAdv * 4.0f;
            continue;
        }
        if (maxWidth > 0.0f && (cursorX - origin.x + charAdv) > maxWidth) {
            break;
        }

        unsigned char uc = static_cast<unsigned char>(ch);
        if (uc >= 32 && uc <= 126) {
            const uint8_t* glyph = FONT_8X8[uc - 32];
            int px0 = static_cast<int>(std::floor(cursorX));
            int py0 = static_cast<int>(std::floor(cursorY));
            int pw = std::max(1, static_cast<int>(std::ceil(charW)));
            int ph = std::max(1, static_cast<int>(std::ceil(charH)));

            for (int dy = 0; dy < ph; ++dy) {
                int py = py0 + dy;
                if (py < 0 || py >= height_) continue;
                float v = (static_cast<float>(dy) + 0.5f) * 8.0f / static_cast<float>(ph);
                int gy = std::clamp(static_cast<int>(v), 0, 7);
                for (int dx = 0; dx < pw; ++dx) {
                    int px = px0 + dx;
                    if (px < 0 || px >= width_) continue;
                    float u = (static_cast<float>(dx) + 0.5f) * 8.0f / static_cast<float>(pw);
                    float cov = 0.0f;
                    if (smooth) {
                        cov = sampleFont8x8Bilinear(glyph, u, v);
                        // Sharpen slightly for crisp modern letter edges
                        cov = std::clamp((cov - 0.18f) * 1.45f, 0.0f, 1.0f);
                    } else {
                        int gx = std::clamp(static_cast<int>(u), 0, 7);
                        cov = ((glyph[gy] >> (7 - gx)) & 1) ? 1.0f : 0.0f;
                    }
                    if (cov > 0.02f) {
                        blendPixel(px, py, sr, sg, sb, baseA * cov);
                    }
                }
            }
        }
        cursorX += charAdv;
    }
}

bool SoftwareRenderBackend::savePPM(const std::string& path) const {
    if (width_ <= 0 || height_ <= 0 || pixels_.empty()) return false;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out << "P6\n" << width_ << " " << height_ << "\n255\n";
    std::vector<uint8_t> rgb(static_cast<size_t>(width_) * 3);
    for (int y = 0; y < height_; ++y) {
        const uint32_t* row = pixels_.data() + static_cast<size_t>(y) * static_cast<size_t>(width_);
        for (int x = 0; x < width_; ++x) {
            uint32_t p = row[x];
            rgb[static_cast<size_t>(x) * 3 + 0] = static_cast<uint8_t>((p >> 16) & 0xFF);
            rgb[static_cast<size_t>(x) * 3 + 1] = static_cast<uint8_t>((p >> 8) & 0xFF);
            rgb[static_cast<size_t>(x) * 3 + 2] = static_cast<uint8_t>(p & 0xFF);
        }
        out.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    }
    return out.good();
}

} // namespace NexusForge::UI

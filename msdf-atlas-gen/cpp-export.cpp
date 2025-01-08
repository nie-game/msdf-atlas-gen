
#include "cpp-export.h"

#include "GlyphGeometry.h"
#include <string>

namespace msdf_atlas {

  static std::string escapeCppString(const char* str) {
    char uval[7] = "\\u0000";
    std::string outStr;
    while (*str) {
      switch (*str) {
      case '\\':
        outStr += "\\\\";
        break;
      case '"':
        outStr += "\\\"";
        break;
      case '\n':
        outStr += "\\n";
        break;
      case '\r':
        outStr += "\\r";
        break;
      case '\t':
        outStr += "\\t";
        break;
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
      case 0x08: /* \\t */ /* \\n */
      case 0x0b:
      case 0x0c: /* \\r */
      case 0x0e:
      case 0x0f:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
        uval[4] = '0' + (*str >= 0x10);
        uval[5] = "0123456789abcdef"[*str & 0x0f];
        outStr += uval;
        break;
      default:
        outStr.push_back(*str);
      }
      ++str;
    }
    return outStr;
  }

  static const char* imageTypeString(ImageType type) {
    switch (type) {
    case ImageType::HARD_MASK:
      return "hardmask";
    case ImageType::SOFT_MASK:
      return "softmask";
    case ImageType::SDF:
      return "sdf";
    case ImageType::PSDF:
      return "psdf";
    case ImageType::MSDF:
      return "msdf";
    case ImageType::MTSDF:
      return "mtsdf";
    }
    return nullptr;
  }

  bool exportCPP(
      const FontGeometry* fonts, int fontCount, ImageType imageType, const CppAtlasMetrics& metrics, const char* filename, bool kerning) {
    if (metrics.yDirection == YDirection::TOP_DOWN)
      return false;
    FILE* f = fopen(filename, "w");
    if (!f)
      return false;
    fputs("#include <mapbox/eternal.hpp>\r\n#include \"atlas.hpp\"\r\n"
          "namespace nie::atlas{const unsigned char raw_data[] = {\r\n"
          "#include \"atlas.bin.h\"\r\n"
          "};std::span<const char> data() { return std::span<const char>(reinterpret_cast<const char*>(&raw_data[0]),sizeof(raw_data)); }",
        f);

    // Atlas properties
    fputs("atlas_t atlas = {", f);
    {
      // fprintf(f, ".type=\"%s\",", imageTypeString(imageType));
      if (imageType == ImageType::SDF || imageType == ImageType::PSDF || imageType == ImageType::MSDF || imageType == ImageType::MTSDF) {
        fprintf(f, ".distanceRange=%.17g,", metrics.distanceRange.upper - metrics.distanceRange.lower);
        fprintf(f, ".distanceRangeMiddle=%.17g,", .5 * (metrics.distanceRange.lower + metrics.distanceRange.upper));
      }
      fprintf(f, ".size=%.17g,", metrics.size);
      fprintf(f, ".width=%d,", metrics.width);
      fprintf(f, ".height=%d,", metrics.height);
      // fprintf(f, ".yOrigin=\"%s\"", metrics.yDirection == YDirection::TOP_DOWN ? "top" : "bottom");
    }
    fputs("};", f);

    for (int i = 0; i < fontCount; ++i) {
      const FontGeometry& font = fonts[i];

      // Font name
      /*const char* name = font.getName();
      if (name)
        fprintf(f, ".name=\"%s\",", escapeCppString(name).c_str());*/

      // Font metrics
      fprintf(f, "template<>metrics_t font<%d>::metrics={", i);
      {
        double yFactor = metrics.yDirection == YDirection::TOP_DOWN ? -1 : 1;
        const msdfgen::FontMetrics& fontMetrics = font.getMetrics();
        fprintf(f, ".lineHeight=%.17g,", fontMetrics.lineHeight);
        fprintf(f, ".ascender=%.17g,", yFactor * fontMetrics.ascenderY);
        fprintf(f, ".descender=%.17g,", yFactor * fontMetrics.descenderY);
        fprintf(f, ".underlineY=%.17g,", yFactor * fontMetrics.underlineY);
      }
      fputs("};", f);

      // Glyph mapping
      fprintf(f, "template<>const glyph_t* font<%d>::glyph(uint32_t code){static const glyph_t list[] = {", i);
      bool firstGlyph = true;
      for (const GlyphGeometry& glyph : font.getGlyphs())
        if (glyph.getCodepoint()) {
          fputs(firstGlyph ? "{" : ",{", f);
          fprintf(f, ".codepoint=%u,", glyph.getCodepoint());
          fprintf(f, ".advance=%.17g", glyph.getAdvance());
          double l, b, r, t;
          glyph.getQuadPlaneBounds(l, b, r, t);
          if (l || b || r || t) {
            switch (metrics.yDirection) {
            case YDirection::BOTTOM_UP:
              fprintf(f, ",.planeBounds={.left=%.17g,.bottom=%.17g,.right=%.17g,.top=%.17g}", l, b, r, t);
              break;
            case YDirection::TOP_DOWN:
              fprintf(f, ",.planeBounds={.left=%.17g,.top=%.17g,.right=%.17g,.bottom=%.17g}", l, -t, r, -b);
              break;
            }
          }
          glyph.getQuadAtlasBounds(l, b, r, t);
          if (l || b || r || t) {
            switch (metrics.yDirection) {
            case YDirection::BOTTOM_UP:
              fprintf(f,
                  ",.atlasBounds={.left=%.17g,.bottom=%.17g,.right=%.17g,.top=%.17g}",
                  l / (metrics.width - 1),
                  b / (metrics.height - 1),
                  r / (metrics.width - 1),
                  t / (metrics.height - 1));
              break;
            case YDirection::TOP_DOWN:
              fprintf(f,
                  ",.atlasBounds={.left=%.17g,.top=%.17g,.right=%.17g,.bottom=%.17g}",
                  l / (metrics.width - 1),
                  (metrics.height - t) / (metrics.height - 1),
                  r / (metrics.width - 1),
                  (metrics.height - b) / (metrics.height - 1));
              break;
            }
          }
          fputs("}", f);
          firstGlyph = false;
        }
      fputs("};auto gmap=mapbox::eternal::map<uint32_t,size_t>({", f);
      {
        int i = 0;
        firstGlyph = true;
        for (const GlyphGeometry& glyph : font.getGlyphs())
          if (glyph.getCodepoint()) {
            fputs(firstGlyph ? "{" : ",{", f);
            fprintf(f, "%d,%d", int(glyph.getCodepoint()), i);
            fputs("}", f);
            firstGlyph = false;
            i++;
          }
        fputs("});auto it=gmap.find(code);if(it!=gmap.end())return &list[it->second];return nullptr;}", f);
      }
    }
    for (int i = 0; i < fontCount; ++i) {
      const FontGeometry& font = fonts[i];
    }

    fputs("}\n", f);
    fclose(f);
    return true;
  }

} // namespace msdf_atlas

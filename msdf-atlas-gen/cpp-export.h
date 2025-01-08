
#pragma once

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include "types.h"
#include "FontGeometry.h"

namespace msdf_atlas {

struct CppAtlasMetrics {
    struct GridMetrics {
        int cellWidth, cellHeight;
        int columns, rows;
        const double *originX, *originY;
        int spacing;
    };
    msdfgen::Range distanceRange;
    double size;
    int width, height;
    YDirection yDirection;
    const GridMetrics *grid;
};

/// Writes the font and glyph metrics and atlas layout data into a comprehensive CPP file
bool exportCPP(const FontGeometry *fonts, int fontCount, ImageType imageType, const CppAtlasMetrics &metrics, const char *filename, bool kerning);

}

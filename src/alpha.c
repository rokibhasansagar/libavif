// Copyright 2020 Joe Drago. All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause

#include "avif/internal.h"

#include <string.h>

avifBool avifFillAlpha(avifAlphaParams * params)
{
    if (params->dstDepth > 8) {
        uint16_t maxChannel = (uint16_t)((1 << params->dstDepth) - 1);
        if (params->dstRange == AVIF_RANGE_LIMITED) {
            maxChannel = (uint16_t)avifFullToLimitedY(params->dstDepth, maxChannel);
        }
        for (uint32_t j = 0; j < params->height; ++j) {
            uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
            for (uint32_t i = 0; i < params->width; ++i) {
                *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = maxChannel;
            }
        }
    } else {
        uint8_t maxChannel = 255;
        if (params->dstRange == AVIF_RANGE_LIMITED) {
            maxChannel = (uint8_t)avifFullToLimitedY(params->dstDepth, maxChannel);
        }
        for (uint32_t j = 0; j < params->height; ++j) {
            uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
            for (uint32_t i = 0; i < params->width; ++i) {
                dstRow[i * params->dstPixelBytes] = maxChannel;
            }
        }
    }
    return AVIF_TRUE;
}

// Note: The [limited -> limited] paths are here for completeness, but in practice those
//       paths will never be used, as avifRGBImage is always full range.
avifBool avifReformatAlpha(avifAlphaParams * params)
{
    int srcMaxChannel = (1 << params->srcDepth) - 1;
    int dstMaxChannel = (1 << params->dstDepth) - 1;
    float srcMaxChannelF = (float)srcMaxChannel;
    float dstMaxChannelF = (float)dstMaxChannel;

    if (params->srcDepth == params->dstDepth) {
        // no depth rescale

        if ((params->srcRange == AVIF_RANGE_FULL) && (params->dstRange == AVIF_RANGE_FULL)) {
            // no depth rescale, no range conversion

            if (params->srcDepth > 8) {
                // no depth rescale, no range conversion, uint16_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                    }
                }
            } else {
                // no depth rescale, no range conversion, uint8_t -> uint8_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        dstRow[i * params->dstPixelBytes] = srcRow[i * params->srcPixelBytes];
                    }
                }
            }
        } else if ((params->srcRange == AVIF_RANGE_LIMITED) && (params->dstRange == AVIF_RANGE_FULL)) {
            // limited -> full

            if (params->srcDepth > 8) {
                // no depth rescale, limited -> full, uint16_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                        int dstAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                    }
                }
            } else {
                // no depth rescale, limited -> full, uint8_t -> uint8_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = srcRow[i * params->srcPixelBytes];
                        int dstAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                        dstRow[i * params->dstPixelBytes] = (uint8_t)dstAlpha;
                    }
                }
            }
        } else if ((params->srcRange == AVIF_RANGE_FULL) && (params->dstRange == AVIF_RANGE_LIMITED)) {
            // full -> limited

            if (params->srcDepth > 8) {
                // no depth rescale, full -> limited, uint16_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                        int dstAlpha = avifFullToLimitedY(params->dstDepth, srcAlpha);
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                    }
                }
            } else {
                // no depth rescale, full -> limited, uint8_t -> uint8_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = srcRow[i * params->srcPixelBytes];
                        int dstAlpha = avifFullToLimitedY(params->dstDepth, srcAlpha);
                        dstRow[i * params->dstPixelBytes] = (uint8_t)dstAlpha;
                    }
                }
            }
        } else if ((params->srcRange == AVIF_RANGE_LIMITED) && (params->dstRange == AVIF_RANGE_LIMITED)) {
            // limited -> limited

            if (params->srcDepth > 8) {
                // no depth rescale, limited -> limited, uint16_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                    }
                }
            } else {
                // no depth rescale, limited -> limited, uint8_t -> uint8_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        dstRow[i * params->dstPixelBytes] = srcRow[i * params->srcPixelBytes];
                    }
                }
            }
        }

    } else {
        // depth rescale

        if ((params->srcRange == AVIF_RANGE_FULL) && (params->dstRange == AVIF_RANGE_FULL)) {
            // depth rescale, no range conversion

            if (params->srcDepth > 8) {
                if (params->dstDepth > 8) {
                    // depth rescale, no range conversion, uint16_t -> uint16_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                        }
                    }
                } else {
                    // depth rescale, no range conversion, uint16_t -> uint8_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            dstRow[i * params->dstPixelBytes] = (uint8_t)dstAlpha;
                        }
                    }
                }
            } else {
                // depth rescale, no range conversion, uint8_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = srcRow[i * params->srcPixelBytes];
                        float alphaF = srcAlpha / srcMaxChannelF;
                        int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                        dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                    }
                }

                // If (srcDepth == 8), dstDepth must be >8 otherwise we'd be in the (params->srcDepth == params->dstDepth) block above.
                // assert(params->dstDepth > 8);
            }
        } else if ((params->srcRange == AVIF_RANGE_LIMITED) && (params->dstRange == AVIF_RANGE_FULL)) {
            // limited -> full

            if (params->srcDepth > 8) {
                if (params->dstDepth > 8) {
                    // depth rescale, limited -> full, uint16_t -> uint16_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            srcAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                        }
                    }
                } else {
                    // depth rescale, limited -> full, uint16_t -> uint8_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            srcAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            dstRow[i * params->dstPixelBytes] = (uint8_t)dstAlpha;
                        }
                    }
                }
            } else {
                // depth rescale, limited -> full, uint8_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = srcRow[i * params->srcPixelBytes];
                        srcAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                        float alphaF = srcAlpha / srcMaxChannelF;
                        int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                        dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                    }
                }

                // If (srcDepth == 8), dstDepth must be >8 otherwise we'd be in the (params->srcDepth == params->dstDepth) block above.
                // assert(params->dstDepth > 8);
            }
        } else if ((params->srcRange == AVIF_RANGE_FULL) && (params->dstRange == AVIF_RANGE_LIMITED)) {
            // full -> limited

            if (params->srcDepth > 8) {
                if (params->dstDepth > 8) {
                    // depth rescale, full -> limited, uint16_t -> uint16_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            dstAlpha = avifFullToLimitedY(params->dstDepth, dstAlpha);
                            *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                        }
                    }
                } else {
                    // depth rescale, full -> limited, uint16_t -> uint8_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            dstAlpha = avifFullToLimitedY(params->dstDepth, dstAlpha);
                            dstRow[i * params->dstPixelBytes] = (uint8_t)dstAlpha;
                        }
                    }
                }
            } else {
                // depth rescale, full -> limited, uint8_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = srcRow[i * params->srcPixelBytes];
                        float alphaF = srcAlpha / srcMaxChannelF;
                        int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                        dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                        dstAlpha = avifFullToLimitedY(params->dstDepth, dstAlpha);
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                    }
                }

                // If (srcDepth == 8), dstDepth must be >8 otherwise we'd be in the (params->srcDepth == params->dstDepth) block above.
                // assert(params->dstDepth > 8);
            }
        } else if ((params->srcRange == AVIF_RANGE_LIMITED) && (params->dstRange == AVIF_RANGE_LIMITED)) {
            // limited -> limited

            if (params->srcDepth > 8) {
                if (params->dstDepth > 8) {
                    // depth rescale, limited -> limited, uint16_t -> uint16_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            srcAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            dstAlpha = avifFullToLimitedY(params->dstDepth, dstAlpha);
                            *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                        }
                    }
                } else {
                    // depth rescale, limited -> limited, uint16_t -> uint8_t

                    for (uint32_t j = 0; j < params->height; ++j) {
                        uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                        uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                        for (uint32_t i = 0; i < params->width; ++i) {
                            int srcAlpha = *((uint16_t *)&srcRow[i * params->srcPixelBytes]);
                            srcAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                            float alphaF = srcAlpha / srcMaxChannelF;
                            int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                            dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                            dstAlpha = avifFullToLimitedY(params->dstDepth, dstAlpha);
                            dstRow[i * params->dstPixelBytes] = (uint8_t)dstAlpha;
                        }
                    }
                }
            } else {
                // depth rescale, limited -> limited, uint8_t -> uint16_t

                for (uint32_t j = 0; j < params->height; ++j) {
                    uint8_t * srcRow = &params->srcPlane[params->srcOffsetBytes + (j * params->srcRowBytes)];
                    uint8_t * dstRow = &params->dstPlane[params->dstOffsetBytes + (j * params->dstRowBytes)];
                    for (uint32_t i = 0; i < params->width; ++i) {
                        int srcAlpha = srcRow[i * params->srcPixelBytes];
                        srcAlpha = avifLimitedToFullY(params->srcDepth, srcAlpha);
                        float alphaF = srcAlpha / srcMaxChannelF;
                        int dstAlpha = (int)(0.5f + (alphaF * dstMaxChannelF));
                        dstAlpha = AVIF_CLAMP(dstAlpha, 0, dstMaxChannel);
                        dstAlpha = avifFullToLimitedY(params->dstDepth, dstAlpha);
                        *((uint16_t *)&dstRow[i * params->dstPixelBytes]) = (uint16_t)dstAlpha;
                    }
                }

                // If (srcDepth == 8), dstDepth must be >8 otherwise we'd be in the (params->srcDepth == params->dstDepth) block above.
                // assert(params->dstDepth > 8);
            }
        }
    }

    return AVIF_TRUE;
}

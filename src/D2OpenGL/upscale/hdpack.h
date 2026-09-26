#ifndef D2OPENGL_UPSCALE_HDPACK_H
#define D2OPENGL_UPSCALE_HDPACK_H

// The HD pack: replacement sprite frames, upscaled ahead of time, in libd2's HD pack format
// (d2util.h), version 1 (raw images) or 2 (each image a zlib stream, or raw where that is no smaller). A frame is found by its key: FNV-1a 64 over the bounding box of its non-zero indices
// (d2_frame_key), plus the box's size. A pack is one file or a folder of them (*.hd, searched in name
// order, the first hit wins). Only the entry tables are held in memory; an image is read from its file
// when it is found, so the packs' size does not count against the process's address space.

#include <stdint.h>

// Opens the pack file or folder at szPath (NULL: none) and checks each file; logs each path, scale and
// entry count, or why a file is not used. Files whose scale differs from the first one's are left out.
// Only the first call does anything. Returns the packs' scale, 0 when there is none.
int HDPack_Open(const char *szPath);

// The pack's scale, 0 when no pack is open.
int HDPack_Scale(void);

// A frame found in the pack: its box in the searched image (1x), and the box's image at the pack's
// scale, top row first; the image is valid until the next lookup.
struct HDPackFrame {
    int x0, y0, w, h;
    const uint8_t *pPixels;
};

// Looks up the w x h index image (pitch bytes per row, top row first): true and *pFrame on a hit.
// Counts hits and misses and logs them every 500 lookups.
bool HDPack_Find(const uint8_t *pIndices, int nWidth, int nHeight, int nPitch, HDPackFrame *pFrame);

#endif

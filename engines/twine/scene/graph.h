/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TWINE_SCENE_GRAPH_H
#define TWINE_SCENE_GRAPH_H

#include "graphics/screen.h"
#include "twine/shared.h"

namespace TwinE {

// WARNING: Rewrite this function to have better performance
inline bool drawGraph(int32 posX, int32 posY, const uint8 *pGraph, bool isSprite, Graphics::Screen &frontVideoBuffer, const Common::Rect &clip) { // AffGraph
	if (!clip.isValidRect()) {
		return false;
	}

	const int32 left = posX + pGraph[2];
	if (left >= clip.right) {
		return false;
	}
	const int32 top = posY + pGraph[3];
	if (top >= clip.bottom) {
		return false;
	}
	const int32 right = pGraph[0] + left;
	if (right < clip.left) {
		return false;
	}
	const int32 bottom = pGraph[1] + top;
	if (bottom < clip.top) {
		return false;
	}
	const int32 maxY = MIN(bottom, (int32)clip.bottom);

	pGraph += 4; // skip the header

#ifdef __3DS__
	// The original graph renderer obtains a pointer at (x, y) *before*
	// clipping x. At the edges of an 800x480 viewport x can be negative or
	// beyond the stride. Decode the RLE by runs and only form valid pointers.
	// Keep the old fast renderer for the original 4:3 mode.
	if (frontVideoBuffer.w * 3 > frontVideoBuffer.h * 4) {
		const int32 safeLeft = MAX<int32>(0, clip.left);
		const int32 safeRight = MIN<int32>(frontVideoBuffer.w, clip.right);
		const int32 safeTop = MAX<int32>(0, clip.top);
		const int32 safeBottom = MIN<int32>(frontVideoBuffer.h, clip.bottom);

		for (int32 y = top; y < maxY; ++y) {
			const int32 numRuns = *pGraph++;
			int32 x = left;
			const bool visibleRow = y >= safeTop && y < safeBottom;
			for (int32 run = 0; run < numRuns; ++run) {
				const uint8 spec = *pGraph++;
				const int32 length = bits(spec, 0, 6) + 1;
				const int32 op = bits(spec, 6, 2);
				if (op == 0) {
					x += length;
					continue;
				}

				const int32 start = MAX<int32>(x, safeLeft);
				const int32 stop = MIN<int32>(MIN<int32>(x + length, safeRight), right);
				if (op == 1) { // a literal run consumes length input bytes
					if (visibleRow && stop > start) {
						uint8 *out = (uint8 *)frontVideoBuffer.getBasePtr(start, y);
						memcpy(out, pGraph + (start - x), stop - start);
					}
					pGraph += length;
				} else { // a repeated-colour run consumes one input byte
					const uint8 colour = *pGraph++;
					if (visibleRow && stop > start) {
						uint8 *out = (uint8 *)frontVideoBuffer.getBasePtr(start, y);
						memset(out, colour, stop - start);
					}
				}
				x += length;
			}
		}
		Common::Rect dirty(left, top, right, bottom);
		dirty.clip(Common::Rect(0, 0, frontVideoBuffer.w, frontVideoBuffer.h));
		dirty.clip(clip);
		if (!dirty.isEmpty()) {
			frontVideoBuffer.addDirtyRect(dirty);
		}
		return true;
	}
#endif

	int32 x = left;

	// if (left >= textWindowLeft-2 && top >= textWindowTop-2 && right <= textWindowRight-2 && bottom <= textWindowBottom-2) // crop
	{
		for (int32 y = top; y < maxY; ++y) {
			const uint8 rleAmount = *pGraph++;
			for (int32 run = 0; run < rleAmount; ++run) {
				const uint8 rleMask = *pGraph++;
				const uint8 iterations = bits(rleMask, 0, 6) + 1;
				const uint8 opCode = bits(rleMask, 6, 2);
				if (opCode == 0) {
					x += iterations;
					continue;
				}
				if (y < clip.top || x >= clip.right || x + iterations < clip.left) {
					if (opCode == 1) {
						pGraph += iterations;
					} else {
						++pGraph;
					}
					x += iterations;
					continue;
				}
				if (opCode == 1) { // 0x40
					uint8 *out = (uint8 *)frontVideoBuffer.getBasePtr(x, y);
					for (uint8 i = 0; i < iterations; i++) {
						if (x >= clip.left && x < clip.right) {
							*out = *pGraph;
						}

						++out;
						++x;
						++pGraph;
					}
				} else {
					const uint8 pixel = *pGraph++;
					uint8 *out = (uint8 *)frontVideoBuffer.getBasePtr(x, y);
					for (uint8 i = 0; i < iterations; i++) {
						if (x >= clip.left && x < clip.right) {
							*out = pixel;
						}

						++out;
						++x;
					}
				}
			}
			x = left;
		}
	}

	Common::Rect rect(left, top, right, bottom);
	frontVideoBuffer.addDirtyRect(rect);

	return true;
}

} // namespace TwinE

#endif // TWINE_SCENE_GRAPH_H

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

#include "twine/resources/hqr.h"
#include "twine/resources/lzss.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/memstream.h"
#include "common/substream.h"
#include "common/system.h"
#include "common/textconsole.h"

namespace TwinE {

namespace HQR {

#define wrap(cmd)                           \
	if ((cmd) == 0) {                       \
		warning("Failed to execute " #cmd); \
		return 0;                           \
	}

/**
 * Decompress entry based in Yaz0r and Zink decompression code
 * @param dst destination pointer where will be the decompressed entry
 * @param compBuf compressed data pointer
 * @param compSize @p compBuf buffer size
 * @param decompsize real file size after decompression
 * @param mode compression mode used
 */
static void decompressEntry(uint8 *dst, const uint8 *compBuf, uint32 compSize, int32 decompsize, int32 mode) {
	Common::MemoryReadStream stream(compBuf, compSize);
	do {
		uint8 b = stream.readByte();
		for (int32 d = 0; d < 8; d++) {
			int32 length;
			if (!(b & (1 << d))) {
				const uint16 offset = stream.readUint16LE();
				length = (offset & 0x0F) + (mode + 1);
				const uint8 *ptr = dst - (offset >> 4) - 1;
				for (int32 i = 0; i < length; i++) {
					*(dst++) = *(ptr++);
				}
			} else {
				length = 1;
				*(dst++) = stream.readByte();
			}
			decompsize -= length;
			if (decompsize <= 0) {
				return;
			}
		}
	} while (decompsize);
}

/**
 * Get a HQR entry pointer
 * @param filename HQR file name
 * @param index entry index to extract
 * @return entry real size
 */
static int voxEntrySize(const char *filename, int32 index, int32 hiddenIndex) {
	if (!filename) {
		return 0;
	}

	Common::File file;
	if (!file.open(filename)) {
		warning("HQR: Could not open %s", filename);
		return 0;
	}

	uint32 headerSize = file.readUint32LE();
	if ((uint32)index >= headerSize / 4) {
		warning("HQR: Invalid entry index");
		return 0;
	}

	wrap(file.seek(index * 4))
	uint32 offsetToData = file.readUint32LE();

	wrap(file.seek(offsetToData))
	uint32 realSize = file.readUint32LE();
	uint32 compSize = file.readUint32LE();

	// exist hidden entries
	for (int32 i = 0; i < hiddenIndex; i++) {
		wrap(file.seek(offsetToData + compSize + 10)) // hidden entry
		offsetToData = offsetToData + compSize + 10;  // current hidden offset

		realSize = file.readUint32LE();
		compSize = file.readUint32LE();
	}

	return realSize;
}

static int32 getEntryTimed(uint8 *ptr, const char *filename, int32 index, uint32 *openMs) {
	if (openMs) {
		*openMs = 0;
	}
	if (!ptr) {
		return 0;
	}
	if (!filename) {
		return 0;
	}

	Common::File file;
	const uint32 openStart = g_system->getMillis();
	const bool opened = file.open(filename);
	if (openMs) {
		*openMs = g_system->getMillis() - openStart;
	}
	if (!opened) {
		warning("HQR: Could not open %s", filename);
		return 0;
	}

	uint32 headerSize = file.readUint32LE();

	if ((uint32)index >= headerSize / 4) {
		warning("HQR: Invalid entry index");
		return 0;
	}

	wrap(file.seek(index * 4))
	uint32 offsetToData = file.readUint32LE();

	wrap(file.seek(offsetToData))
	uint32 realSize = file.readUint32LE();
	uint32 compSize = file.readUint32LE();
	uint16 mode = file.readUint16LE();

	// uncompressed
	if (mode == 0) {
		wrap(file.read(ptr, realSize))
	}
	// compressed: modes (1 & 2)
	else if (mode == 1 || mode == 2) {
		uint8 *compDataPtr = (uint8 *)malloc(compSize);
		wrap(file.read(compDataPtr, compSize))
		decompressEntry(ptr, compDataPtr, compSize, realSize, mode);
		free(compDataPtr);
	}
	debugC(1, TwinE::kDebugResources, "Loaded entry from %s for index %i with %i bytes", filename, index, realSize);
	return realSize;
}

int32 getEntry(uint8 *ptr, const char *filename, int32 index) {
	return getEntryTimed(ptr, filename, index, nullptr);
}

static int32 entrySizeTimed(const char *filename, int32 index, uint32 *openMs) {
	if (openMs) {
		*openMs = 0;
	}
	if (!filename) {
		return 0;
	}

	Common::File file;
	const uint32 openStart = g_system->getMillis();
	const bool opened = file.open(filename);
	if (openMs) {
		*openMs = g_system->getMillis() - openStart;
	}
	if (!opened) {
		warning("HQR: Could not open %s", filename);
		return 0;
	}

	uint32 headerSize = file.readUint32LE();
	if ((uint32)index >= headerSize / 4) {
		warning("HQR: Invalid entry index");
		return 0;
	}

	wrap(file.seek(index * 4))
	uint32 offsetToData = file.readUint32LE();

	wrap(file.seek(offsetToData))
	uint32 realSize = file.readUint32LE();

	return realSize;
}

int32 entrySize(const char *filename, int32 index) {
	return entrySizeTimed(filename, index, nullptr);
}

int32 numEntries(const char *filename) {
	if (!filename) {
		return 0;
	}

	Common::File file;
	if (!file.open(filename)) {
		warning("HQR: Could not open %s", filename);
		return 0;
	}

	uint32 headerSize = file.readUint32LE();
	return ((int)headerSize / 4) - 1;
}

Common::SeekableReadStream *makeReadStream(const char *filename, int index) {
	Common::File *file = new Common::File();
	if (!file->open(filename)) {
		delete file;
		warning("HQR: Could not open %s", filename);
		return nullptr;
	}

	const uint32 headerSize = file->readUint32LE();
	if ((uint32)index >= headerSize / 4) {
		warning("HQR: Invalid entry index: %i", index);
		delete file;
		return nullptr;
	}

	if (!file->seek(index * 4)) {
		warning("HQR: Invalid index: %i", index);
		delete file;
		return nullptr;
	}
	const uint32 offsetToData = file->readUint32LE();
	if (!file->seek(offsetToData)) {
		warning("HQR: Invalid index: %i", index);
		delete file;
		return nullptr;
	}

	const uint32 realSize = file->readUint32LE();
	const uint32 compressedSize = file->readUint32LE();
	const uint16 mode = file->readUint16LE();

	const uint32 begin = offsetToData + 10;
	uint32 end = 0;
	if (mode == 0) {
		end = begin + realSize;
	} else {
		end = begin + compressedSize;
	}
	Common::SeekableReadStream *stream = new Common::SeekableSubReadStream(file, begin, end, DisposeAfterUse::YES);
	if (mode != 0) {
		stream = new LzssReadStream(stream, mode, realSize);
	}
	debugC(1, TwinE::kDebugResources, "Loaded entry from %s for index %i with %i bytes", filename, index, realSize);
	return stream;
}

int32 getAllocEntryOpenTimed(uint8 **ptr, const char *filename, int32 index,
                             uint32 *sizeMs, uint32 *readMs,
                             uint32 *sizeOpenMs, uint32 *readOpenMs) {
	if (sizeOpenMs) {
		*sizeOpenMs = 0;
	}
	if (readOpenMs) {
		*readOpenMs = 0;
	}
	if (sizeMs) {
		*sizeMs = 0;
	}
	if (readMs) {
		*readMs = 0;
	}
	if (*ptr) {
		free(*ptr);
	}
	const uint32 sizeStart = g_system->getMillis();
	const int32 size = entrySizeTimed(filename, index, sizeOpenMs);
	if (sizeMs) {
		*sizeMs = g_system->getMillis() - sizeStart;
	}
	if (size <= 0) {
		*ptr = nullptr;
		warning("HQR: failed to get entry for index %i from file: %s", index, filename);
		return 0;
	}
	*ptr = (uint8 *)malloc(size * sizeof(uint8));
	if (!*ptr) {
		warning("HQR: unable to allocate entry memory");
		return 0;
	}
	const uint32 readStart = g_system->getMillis();
	const int32 entrySize = getEntryTimed(*ptr, filename, index, readOpenMs);
	if (readMs) {
		*readMs = g_system->getMillis() - readStart;
	}
	assert(entrySize == size);
	return entrySize;
}

int32 getAllocEntry(uint8 **ptr, const char *filename, int32 index,
                    uint32 *sizeMs, uint32 *readMs) {
	return getAllocEntryOpenTimed(ptr, filename, index, sizeMs, readMs, nullptr, nullptr);
}

// LBA3DS v5: read an HQR index once for a scene, then release it after loading.
// A failed prefetch falls back to the unchanged v4 per-entry lookup.
bool loadBrickIndex(const char *filename, uint8 **indexData, uint32 *indexBytes) {
    if (!filename || !indexData || !indexBytes) {
        return false;
    }
    *indexData = nullptr;
    *indexBytes = 0;

    Common::File file;
    if (!file.open(filename)) {
        warning("HQR: Could not open index archive %s", filename);
        return false;
    }
    const int64 archiveSize = file.size();
    if (archiveSize < 4) {
        return false;
    }
    const uint32 headerSize = file.readUint32LE();
    // The first offset in an HQR is the size of its little-endian offset table.
    // Cap the diagnostic allocation; malformed/large indexes use v4 instead.
    if (headerSize < 4 || (headerSize & 3) != 0 ||
        headerSize > 64 * 1024 || (uint64)headerSize > (uint64)archiveSize) {
        warning("HQR: Invalid/oversized brick index in %s", filename);
        return false;
    }
    uint8 *data = (uint8 *)malloc(headerSize);
    if (!data) {
        return false;
    }
    if (!file.seek(0) || file.read(data, headerSize) != headerSize ||
        READ_LE_UINT32(data) != headerSize) {
        free(data);
        warning("HQR: Could not read brick index from %s", filename);
        return false;
    }
    *indexData = data;
    *indexBytes = headerSize;
    return true;
}

// The indexed and original v4 paths share the same reader/decompressor.
// Other HQR callers still use the existing public getAllocEntry() function.
static int32 getAllocEntrySingleEntryImpl(uint8 **ptr, const char *filename, int32 index,
                                         uint32 *openMs, const uint8 *indexData,
                                         uint32 indexBytes, Common::File *sharedArchive) {
	if (openMs) {
		*openMs = 0;
	}
	if (!ptr || !filename || index < 0) {
		return 0;
	}

	// Match getAllocEntry() ownership: discard any previously allocated entry.
	free(*ptr);
	*ptr = nullptr;

	// v6: the scene loader may supply one already-open archive. Otherwise
	// preserve the independently opening v5 code path unchanged.
	Common::File localFile;
	Common::File &file = sharedArchive ? *sharedArchive : localFile;
	if (!sharedArchive) {
		const uint32 openStart = g_system->getMillis();
		const bool opened = file.open(filename);
		if (openMs) {
			*openMs = g_system->getMillis() - openStart;
		}
		if (!opened) {
			warning("HQR: Could not open %s", filename);
			return 0;
		}
	}

	const int64 archiveSize = file.size();
	if (archiveSize < 10) {
		warning("HQR: Invalid archive %s", filename);
		return 0;
	}
	uint32 offsetToData = 0;
	if (indexData && indexBytes >= 4) {
		if ((uint32)index >= indexBytes / 4 ||
		    indexBytes > (uint64)archiveSize) {
			warning("HQR: Invalid cached index %i in %s", index, filename);
			return 0;
		}
		offsetToData = READ_LE_UINT32(indexData + index * 4);
	} else {
		// v4 fallback: locate this entry in the archive itself.
		const uint32 headerSize = file.readUint32LE();
		if (headerSize < 4 || (uint32)index >= headerSize / 4 ||
		    headerSize > (uint64)archiveSize) {
			warning("HQR: Invalid entry index %i in %s", index, filename);
			return 0;
		}
		if (!file.seek(index * 4)) {
			warning("HQR: Could not seek to entry %i in %s", index, filename);
			return 0;
		}
		offsetToData = file.readUint32LE();
	}
	if ((uint64)offsetToData + 10 > (uint64)archiveSize ||
	    !file.seek(offsetToData)) {
		warning("HQR: Invalid entry offset for %i in %s", index, filename);
		return 0;
	}

	const uint32 realSize = file.readUint32LE();
	const uint32 compSize = file.readUint32LE();
	const uint16 mode = file.readUint16LE();
	const uint32 storedSize = mode == 0 ? realSize : compSize;
	if (realSize == 0 || realSize > 0x7fffffffU ||
	    mode > 2 || storedSize == 0 ||
	    (uint64)offsetToData + 10 + storedSize > (uint64)archiveSize) {
		warning("HQR: Invalid entry %i in %s", index, filename);
		return 0;
	}

	uint8 *data = (uint8 *)malloc(realSize);
	if (!data) {
		warning("HQR: Unable to allocate brick %i", index);
		return 0;
	}

	if (mode == 0) {
		if (file.read(data, realSize) != realSize) {
			free(data);
			warning("HQR: Failed to read brick %i", index);
			return 0;
		}
	} else {
		uint8 *compressed = (uint8 *)malloc(compSize);
		if (!compressed) {
			free(data);
			warning("HQR: Unable to allocate compressed brick %i", index);
			return 0;
		}
		const bool readOK = file.read(compressed, compSize) == compSize;
		if (readOK) {
			decompressEntry(data, compressed, compSize, realSize, mode);
		}
		free(compressed);
		if (!readOK) {
			free(data);
			warning("HQR: Failed to read compressed brick %i", index);
			return 0;
		}
	}

	*ptr = data;
	return (int32)realSize;
}

int32 getAllocEntrySingleEntry(uint8 **ptr, const char *filename, int32 index,
                              uint32 *openMs) {
	return getAllocEntrySingleEntryImpl(ptr, filename, index, openMs, nullptr, 0, nullptr);
}

int32 getAllocEntrySingleEntryIndexed(uint8 **ptr, const char *filename, int32 index,
                                     uint32 *openMs, const uint8 *indexData,
                                     uint32 indexBytes) {
	return getAllocEntrySingleEntryImpl(ptr, filename, index, openMs,
	                                    indexData, indexBytes, nullptr);
}

// v6: read a brick through the scene's open file without taking ownership of it.
// The caller closes the file after loading all missing bricks.
int32 getAllocEntrySingleEntryIndexedShared(uint8 **ptr, Common::File &archive,
                                           const char *filename, int32 index,
                                           const uint8 *indexData, uint32 indexBytes) {
	if (!indexData || indexBytes < 4 || !archive.isOpen()) {
		return 0;
	}
	return getAllocEntrySingleEntryImpl(ptr, filename, index, nullptr,
	                                   indexData, indexBytes, &archive);
}

bool dumpEntry(const char *filename, int32 index, const char *targetFileName) {
	Common::DumpFile out;
	if (!out.open(targetFileName, true)) {
		warning("Failed to save to %s", targetFileName);
		return false;
	}

	uint8 *content = nullptr;
	const int size = getAllocEntry(&content, filename, index);
	if (size == 0) {
		warning("Could not get hqr entry in %s for index %i", filename, index);
		return false;
	}
	out.write(content, size);
	out.flush();
	out.close();
	free(content);
	return true;
}

int32 getVoxEntry(uint8 *ptr, const char *filename, int32 index, int32 hiddenIndex) {
	if (!ptr) {
		return 0;
	}
	if (!filename) {
		return 0;
	}

	Common::File file;
	if (!file.open(filename)) {
		warning("HQR: Could not open %s", filename);
		return 0;
	}

	uint32 headerSize = file.readUint32LE();

	if ((uint32)index >= headerSize / 4) {
		warning("HQR: Invalid entry index");
		return 0;
	}

	wrap(file.seek(index * 4))
	uint32 offsetToData = file.readUint32LE();

	wrap(file.seek(offsetToData))
	uint32 realSize = file.readUint32LE();
	uint32 compSize = file.readUint32LE();
	uint16 mode = file.readSint16LE();

	// exist hidden entries
	for (int32 i = 0; i < hiddenIndex; i++) {
		wrap(file.seek(offsetToData + compSize + 10)) // hidden entry
		offsetToData = offsetToData + compSize + 10;  // current hidden offset

		realSize = file.readUint32LE();
		compSize = file.readUint32LE();
		mode = file.readUint16LE();
	}

	// uncompressed
	if (mode == 0) {
		wrap(file.read(ptr, realSize))
	}
	// compressed: modes (1 & 2)
	else if (mode == 1 || mode == 2) {
		uint8 *compDataPtr = (uint8 *)malloc(compSize);
		wrap(file.read(compDataPtr, compSize))
		decompressEntry(ptr, compDataPtr, compSize, realSize, mode);
		free(compDataPtr);
	}
	debugC(1, TwinE::kDebugResources, "Loaded vox entry from %s for index %i with %i bytes", filename, index, realSize);
	return realSize;
}

bool getPaletteEntry(Graphics::Palette &palette, const char *filename, int32 index) {
	byte paletteBuffer[NUMOFCOLORS * 3];
	int32 size = HQR::getEntry(paletteBuffer, filename, index);
	if (size <= 0) {
		debugC(1, TwinE::kDebugResources, "Failed to load palette from %s for index %i", filename, index);
		return false;
	}
	palette = Graphics::Palette(paletteBuffer, size / 3);
	debugC(1, TwinE::kDebugResources, "Loaded palette from %s for index %i with %i color entries", filename, index, (int)palette.size());
	debugC(1, TwinE::kDebugPalette, "Loaded palette from %s for index %i with %i color entries", filename, index, (int)palette.size());
	return true;
}

int32 getAllocVoxEntry(uint8 **ptr, const char *filename, int32 index, int32 hiddenIndex) {
	const int32 size = voxEntrySize(filename, index, hiddenIndex);
	if (size == 0) {
		warning("HQR: vox entry with 0 size found for index: %d", index);
		return 0;
	}

	*ptr = (uint8 *)malloc(size * sizeof(uint8));
	if (!*ptr) {
		warning("HQR: unable to allocate entry memory of size %d for index: %d", size, index);
		return 0;
	}
	const int32 entrySize = getVoxEntry(*ptr, filename, index, hiddenIndex);
	assert(entrySize == size);
	return entrySize;
}

#undef wrap

} // namespace HQR

} // namespace TwinE

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

#include "twine/resources/resources.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/str.h"
#include "common/system.h"
#include "common/tokenizer.h"
#include "common/util.h"
#include "graphics/palette.h"
#include "twine/audio/sound.h"
#include "twine/parser/anim3ds.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/resources/hqr.h"
#include "twine/scene/animations.h"
#include "twine/scene/scene.h"
#include "twine/shared.h"
#include "twine/text.h"
#include "twine/twine.h"

namespace TwinE {

#ifdef __3DS__
static void appendStartupResourcesLogV3(uint32 setupMs, uint32 spritesMs, uint32 animationsMs,
                                        uint32 samplesMs, uint32 inventoryMs, uint32 movieInfoMs,
                                        uint32 textLoadMs, uint32 textParseMs, uint32 textMs, uint32 totalMs) {
	const char *fileName = "lba3ds_startup_resources_v3.csv";
	Common::SaveFileManager *manager = g_system->getSavefileManager();
	char history[16384];
	uint32 historySize = 0;
	Common::InSaveFile *previous = manager->openForLoading(fileName);
	if (previous) {
		const int64 oldSize = previous->size();
		if (oldSize > 0 && oldSize < (int64)(sizeof(history) - 256)) {
			historySize = (uint32)oldSize;
			if (previous->read(history, historySize) != historySize) {
				historySize = 0;
			}
		}
		delete previous;
	}

	Common::OutSaveFile *out = manager->openForSaving(fileName, false);
	if (!out) {
		return;
	}
	if (historySize) {
		out->write(history, historySize);
	} else {
		out->writeString("setup_ms,sprites_ms,animations_ms,samples_ms,inventory_ms,movieinfo_ms,text_load_ms,text_parse_ms,text_ms,total_ms\n");
	}
	out->writeString(Common::String::format("%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
	        setupMs, spritesMs, animationsMs, samplesMs, inventoryMs, movieInfoMs,
	        textLoadMs, textParseMs, textMs, totalMs));
	out->finalize();
	delete out;
}

// The stock HQR API opens an archive for entrySize() and opens it again to read
// each entry. On the 3DS SD filesystem that turns the large startup preloads
// into hundreds of opens. Reuse the already-tested indexed/shared reader from
// the brick loader; every failed fast-path entry transparently falls back to
// the original public loader.
namespace HQR {
bool loadBrickIndex(const char *filename, uint8 **indexData, uint32 *indexBytes);
int32 getAllocEntrySingleEntryIndexedShared(uint8 **ptr, Common::File &archive,
                                           const char *filename, int32 index,
                                           const uint8 *indexData, uint32 indexBytes);
} // namespace HQR

class StartupHQRLoader {
private:
	const char *_filename;
	Common::File _archive;
	uint8 *_indexData = nullptr;
	uint32 _indexBytes = 0;
	bool _ready = false;

public:
	explicit StartupHQRLoader(const char *filename) : _filename(filename) {
		if (HQR::loadBrickIndex(filename, &_indexData, &_indexBytes) && _archive.open(filename)) {
			_ready = true;
		} else {
			free(_indexData);
			_indexData = nullptr;
			_indexBytes = 0;
			_archive.close();
		}
	}

	~StartupHQRLoader() {
		_archive.close();
		free(_indexData);
	}

	int32 load(uint8 **ptr, int32 index) {
		if (_ready) {
			const int32 size = HQR::getAllocEntrySingleEntryIndexedShared(
			    ptr, _archive, _filename, index, _indexData, _indexBytes);
			if (size > 0) {
				return size;
			}
		}
		return HQR::getAllocEntry(ptr, _filename, index);
	}
};
#endif

Resources::~Resources() {
	for (size_t i = 0; i < ARRAYSIZE(_spriteTable); ++i) {
		free(_spriteTable[i]);
	}
	for (size_t i = 0; i < ARRAYSIZE(_samplesTable); ++i) {
		free(_samplesTable[i]);
	}
	free(_fontPtr);
	free(_sjisFontPtr);
}

void Resources::initPalettes() {
	if (!HQR::getPaletteEntry(_engine->_screens->_ptrPal, Resources::HQR_RESS_FILE, RESSHQR_MAINPAL)) {
		error("Failed to load main palette");
	}
	_engine->setPalette(_engine->_screens->_ptrPal);
}

void Resources::preloadAnim3DS() {
	const int index = HQR::numEntries(Resources::HQR_ANIM3DS_FILE) - 1;
	_anim3DSData.loadFromHQR(Resources::HQR_ANIM3DS_FILE, index, _engine->isLBA1());
}

void Resources::loadEntityData(EntityData &entityData, int32 &index) {
	if (_engine->isLBA1()) {
		TwineResource modelRes(Resources::HQR_FILE3D_FILE, index);
		if (!entityData.loadFromHQR(modelRes, _engine->isLBA1())) {
			error("Failed to load actor 3d data for index: %i", index);
		}
	} else {
		// TODO: don't allocate each time
		TwineResource modelRes(Resources::HQR_RESS_FILE, 44);
		uint8 *file3dBuf = nullptr;
		const int32 holomapImageSize = HQR::getAllocEntry(&file3dBuf, modelRes);
		if (!entityData.loadFromBuffer((uint8 *)(file3dBuf + *(((uint32 *)file3dBuf) + (index))), holomapImageSize, _engine->isLBA1())) {
			delete file3dBuf;
			error("Failed to load actor 3d data for index: %i", index);
		}
		delete file3dBuf;
	}
}

const T_ANIM_3DS *Resources::getAnim(int index) const {
	if (index < 0 || index >= (int)_anim3DSData.getAnims().size()) {
		return nullptr;
	}
	return &_anim3DSData.getAnims()[index];
}

void Resources::preloadSprites() {
	const int32 numEntries = HQR::numEntries(Resources::HQR_SPRITES_FILE);
#ifdef __3DS__
	StartupHQRLoader loader(Resources::HQR_SPRITES_FILE);
#endif
	const int32 maxSprites = _engine->isLBA1() ? 200 : NUM_SPRITES;
	if (numEntries > maxSprites) {
		error("Max allowed sprites exceeded: %i/%i", numEntries, maxSprites);
	}
	debugC(1, TwinE::kDebugResources, "preload %i sprites", numEntries);
	for (int32 i = 0; i < numEntries; i++) {
#ifdef __3DS__
		_spriteSizeTable[i] = loader.load(&_spriteTable[i], i);
#else
		_spriteSizeTable[i] = HQR::getAllocEntry(&_spriteTable[i], Resources::HQR_SPRITES_FILE, i);
#endif
		if (!_spriteData[i].loadFromBuffer(_spriteTable[i], _spriteSizeTable[i], _engine->isLBA1())) {
			warning("Failed to load sprite %i", i);
		}
	}
}

void Resources::preloadAnimations() {
	const int32 numEntries = HQR::numEntries(Resources::HQR_ANIM_FILE);
#ifdef __3DS__
	StartupHQRLoader loader(Resources::HQR_ANIM_FILE);
#endif
	const int32 maxAnims = _engine->isLBA1() ? 600 : NUM_ANIMS;
	if (numEntries > maxAnims) {
		error("Max allowed animations exceeded: %i/%i", numEntries, maxAnims);
	}
	debugC(1, TwinE::kDebugResources, "preload %i animations", numEntries);
	for (int32 i = 0; i < numEntries; i++) {
#ifdef __3DS__
		uint8 *animBuffer = nullptr;
		const int32 animSize = loader.load(&animBuffer, i);
		if (animSize <= 0 || !_animData[i].loadFromBuffer(animBuffer, animSize, _engine->isLBA1())) {
			warning("Failed to load animation %i", i);
		}
		free(animBuffer);
#else
		_animData[i].loadFromHQR(Resources::HQR_ANIM_FILE, i, _engine->isLBA1());
#endif
	}
}

static bool isLba1BlankSampleEntry(int32 index) {
	// these indices contain blank hqr entries
	const int32 blankIndices[] = {80, 81, 82, 83, 115, 118, 120, 124, 125, 139, 140, 154, 155};
	for (int j = 0; j < ARRAYSIZE(blankIndices); ++j) {
		if (index == blankIndices[j]) {
			return true;
		}
	}
	return false;
}

void Resources::preloadSamples() {
	const int32 numEntries = HQR::numEntries(Resources::HQR_SAMPLES_FILE);
#ifdef __3DS__
	StartupHQRLoader loader(Resources::HQR_SAMPLES_FILE);
#endif
	const int32 maxSamples = _engine->isLBA1() ? 243 : NUM_SAMPLES;
	if (numEntries > maxSamples) {
		error("Max allowed samples exceeded: %i/%i", numEntries, maxSamples);
	}
	debugC(1, TwinE::kDebugResources, "preload %i samples", numEntries);
	for (int32 i = 0; i < numEntries; i++) {
		if (_engine->isLBA1() && isLba1BlankSampleEntry(i)) {
			_samplesSizeTable[i] = 0;
			_samplesTable[i] = nullptr;
			continue;
		}
#ifdef __3DS__
		_samplesSizeTable[i] = loader.load(&_samplesTable[i], i);
#else
		_samplesSizeTable[i] = HQR::getAllocEntry(&_samplesTable[i], Resources::HQR_SAMPLES_FILE, i);
#endif
		if (_samplesSizeTable[i] == 0) {
			warning("Failed to load sample %i", i);
			continue;
		}
		// Fix incorrect sample files first byte
		if (*_samplesTable[i] != 'C') {
			debugC(1, TwinE::kDebugResources, "Sample %i has incorrect magic id (size: %u)", i, _samplesSizeTable[i]);
			*_samplesTable[i] = 'C';
		}
	}
}

void Resources::preloadInventoryItems() {
	if (!_engine->isLBA1()) {
		// lba2 has this data in code
		return;
	}
	int32 numEntries = HQR::numEntries(Resources::HQR_INVOBJ_FILE);
	if (_engine->isPreview()) {
		if (numEntries != 32) {
			error("Unexpected inventory items for lba1 preview version: %i/32", numEntries);
		}
		// TODO: this is obviously a hack
		numEntries = NUM_INVENTORY_ITEMS;
	} else {
		if (numEntries > NUM_INVENTORY_ITEMS) {
			error("Max allowed inventory items exceeded: %i/%i", numEntries, NUM_INVENTORY_ITEMS);
		}
	}
	debugC(1, TwinE::kDebugResources, "preload %i inventory items", numEntries);
	for (int32 i = 0; i < numEntries; i++) {
		_inventoryTable[i].loadFromHQR(Resources::HQR_INVOBJ_FILE, i, _engine->isLBA1());
	}
}

void Resources::initResources() {
#ifdef __3DS__
	const uint32 resourceTotalStart = g_system->getMillis();
#endif
	initPalettes();

	_fontBufSize = HQR::getAllocEntry(&_fontPtr, Resources::HQR_RESS_FILE, RESSHQR_LBAFONT);
	if (_fontBufSize == 0) {
		error("Failed to load font");
	}

	const int kMinSjisSize = 11072 * 24 * 3;
	Common::File f24;
	if (f24.open("FNT24.DAT") && f24.size() >= kMinSjisSize) {
		// Rest is garbage
		_sjisFontPtr = (byte *)malloc(kMinSjisSize);
		assert(_sjisFontPtr);
		f24.read(_sjisFontPtr, kMinSjisSize);
	}

	_engine->_text->setFont(INTER_LEAVE, INTER_SPACE);
	_engine->_text->setFontColor(COLOR_14);
	_engine->_text->setTextCrossColor(136, 143, 2);

	if (_engine->isLBA1()) {
		if (!_spriteShadowPtr.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_SPRITESHADOW), _engine->isLBA1())) {
			error("Failed to load shadow sprites");
		}

		if (!_spriteBoundingBox.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_SPRITEBOXDATA), _engine->isLBA1())) {
			error("Failed to load sprite bounding box data");
		}

		if (!_holomapTwinsenModelPtr.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_HOLOTWINMDL), _engine->isLBA1())) {
			error("Failed to load holomap twinsen model");
		}

		if (!_holomapPointModelPtr.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_HOLOPOINTMDL), _engine->isLBA1())) {
			if (!_engine->isPreview()) {
				error("Failed to load holomap point model");
			}
		}

		if (!_holomapArrowPtr.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_HOLOARROWMDL), _engine->isLBA1())) {
			error("Failed to load holomap arrow model");
		}

		if (!_holomapTwinsenArrowPtr.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_HOLOTWINARROWMDL), _engine->isLBA1())) {
			error("Failed to load holomap twinsen arrow model");
		}

		if (!_trajectories.loadFromHQR(TwineResource(Resources::HQR_RESS_FILE, RESSHQR_HOLOPOINTANIM), _engine->isLBA1())) {
			if (!_engine->isPreview()) {
				error("Failed to parse trajectory data");
			}
		}
		debugC(1, TwinE::kDebugResources, "preload %i trajectories", (int)_trajectories.getTrajectories().size());
	} else if (_engine->isLBA2()) {
		preloadAnim3DS();
	}

#ifdef __3DS__
	const uint32 setupMs = g_system->getMillis() - resourceTotalStart;
	const uint32 spritesStart = g_system->getMillis();
	preloadSprites();
	const uint32 spritesMs = g_system->getMillis() - spritesStart;
	const uint32 animStart = g_system->getMillis();
	preloadAnimations();
	const uint32 animationsMs = g_system->getMillis() - animStart;
	const uint32 samplesStart = g_system->getMillis();
	preloadSamples();
	const uint32 samplesMs = g_system->getMillis() - samplesStart;
	const uint32 inventoryStart = g_system->getMillis();
	preloadInventoryItems();
	const uint32 inventoryMs = g_system->getMillis() - inventoryStart;
#else
	preloadSprites();
	preloadAnimations();
	preloadSamples();
	preloadInventoryItems();
#endif

#ifdef __3DS__
	const uint32 movieInfoStart = g_system->getMillis();
#endif
	loadMovieInfo();
#ifdef __3DS__
	const uint32 movieInfoMs = g_system->getMillis() - movieInfoStart;
	const uint32 textStart = g_system->getMillis();
#endif

#ifdef __3DS__
	uint32 textLoadMs = 0;
	uint32 textParseMs = 0;
#endif
	if (!_engine->isPreview()) {
		// TODO: where is the text in the preview version?
		const int32 textEntryCount = _engine->isLBA1() ? 28 : 30;
#ifdef __3DS__
		// TEXT.HQR is particularly expensive on the 3DS SD filesystem because
		// every bank consists of two entries. Keep one indexed archive open for
		// the entire preload instead of opening it roughly 28 times at startup.
		StartupHQRLoader textLoader(Resources::HQR_TEXT_FILE);
#endif
		for (int32 i = 0; i < textEntryCount / 2; ++i) {
#ifdef __3DS__
			const int langIdx = i * 2 + textEntryCount * _engine->_cfgfile._languageId;
			uint8 *indexData = nullptr;
			uint8 *offsetData = nullptr;

			const uint32 loadStart = g_system->getMillis();
			const int32 indexSize = textLoader.load(&indexData, langIdx);
			const int32 offsetSize = textLoader.load(&offsetData, langIdx + 1);
			textLoadMs += g_system->getMillis() - loadStart;

			bool loaded = false;
			if (indexSize > 0 && offsetSize > 0) {
				const uint32 parseStart = g_system->getMillis();
				loaded = _textData.loadFromBuffers(indexData, indexSize, offsetData, offsetSize,
				                                       (TextBankId)i, _engine->isLBA1());
				textParseMs += g_system->getMillis() - parseStart;
			}
			free(indexData);
			free(offsetData);

			// Keep the original path as a safety net for unusual distributions.
			if (!loaded) {
				const uint32 fallbackStart = g_system->getMillis();
				loaded = _textData.loadFromHQR(Resources::HQR_TEXT_FILE, (TextBankId)i,
				                                   _engine->_cfgfile._languageId, _engine->isLBA1(), textEntryCount);
				textParseMs += g_system->getMillis() - fallbackStart;
			}
			if (!loaded) {
#else
			if (!_textData.loadFromHQR(Resources::HQR_TEXT_FILE, (TextBankId)i, _engine->_cfgfile._languageId, _engine->isLBA1(), textEntryCount)) {
#endif
				error("HQR ERROR: Parsing textbank %i failed for language %i (%i entries)", i, _engine->_cfgfile._languageId, textEntryCount);
			}
		}
		debugC(1, TwinE::kDebugResources, "Loaded %i text banks", textEntryCount / 2);
	}
#ifdef __3DS__
	const uint32 textMs = g_system->getMillis() - textStart;
	const uint32 totalMs = g_system->getMillis() - resourceTotalStart;
	warning("LBA3DS resources v3: setup=%u sprites=%u animations=%u samples=%u inventory=%u movie=%u text(load=%u parse=%u total=%u) total=%u ms",
	        setupMs, spritesMs, animationsMs, samplesMs, inventoryMs, movieInfoMs,
	        textLoadMs, textParseMs, textMs, totalMs);
	appendStartupResourcesLogV3(setupMs, spritesMs, animationsMs, samplesMs, inventoryMs, movieInfoMs,
	                            textLoadMs, textParseMs, textMs, totalMs);
#endif
}

bool Resources::reloadTextBanks() {
	if (_engine->isPreview()) {
		return true;
	}

	const int32 textEntryCount = _engine->isLBA1() ? 28 : 30;
#ifdef __3DS__
	StartupHQRLoader textLoader(Resources::HQR_TEXT_FILE);
#endif
	for (int32 i = 0; i < textEntryCount / 2; ++i) {
		bool loaded = false;
#ifdef __3DS__
		const int langIdx = i * 2 + textEntryCount * _engine->_cfgfile._languageId;
		uint8 *indexData = nullptr;
		uint8 *offsetData = nullptr;
		const int32 indexSize = textLoader.load(&indexData, langIdx);
		const int32 offsetSize = textLoader.load(&offsetData, langIdx + 1);
		if (indexSize > 0 && offsetSize > 0) {
			loaded = _textData.loadFromBuffers(indexData, indexSize, offsetData, offsetSize,
			                                       (TextBankId)i, _engine->isLBA1());
		}
		free(indexData);
		free(offsetData);
#endif
		if (!loaded) {
			loaded = _textData.loadFromHQR(Resources::HQR_TEXT_FILE, (TextBankId)i,
			                               _engine->_cfgfile._languageId, _engine->isLBA1(), textEntryCount);
		}
		if (!loaded) {
			warning("HQR ERROR: Reloading textbank %i failed for language %i",
			        i, _engine->_cfgfile._languageId);
			return false;
		}
	}
	return true;
}

const TextEntry *Resources::getText(TextBankId textBankId, TextId index) const {
	return _textData.getText(textBankId, index);
}

const Trajectory *Resources::giveTrajPtr(int index) const {
	return _trajectories.getTrajectory(index);
}

int Resources::findSmkMovieIndex(const char *name) const {
	Common::String smkName = name;
	smkName.toLowercase();
	if (!_movieInfo.contains(smkName)) {
		warning("Movie '%s' not found in movie info", smkName.c_str());
		return -1;
	}
	const Common::Array<int32> &info = getMovieInfo(smkName);
	return info[0];
}

void Resources::loadMovieInfo() {
	uint8 *content = nullptr;
	int32 size;
	if (_engine->isLBA1()) {
		if (_engine->isPreview()) {
			size = 0;
		} else {
			size = HQR::getAllocEntry(&content, Resources::HQR_RESS_FILE, RESSHQR_FLAINFO);
		}
	} else {
		size = HQR::getAllocEntry(&content, Resources::HQR_RESS_FILE, 48);
	}
	if (size == 0) {
		return;
	}
	const Common::String str((const char *)content, size);
	free(content);
	debugC(2, TwinE::kDebugResources, "movie info:\n%s", str.c_str());
	Common::StringTokenizer tok(str, "\r\n");
	int videoIndex = 0;
	while (!tok.empty()) {
		Common::String line = tok.nextToken();
		if (_engine->isLBA1()) {
			Common::StringTokenizer lineTok(line);
			if (lineTok.empty()) {
				continue;
			}
			const Common::String &name = lineTok.nextToken();
			Common::Array<int32> frames;
			while (!lineTok.empty()) {
				const Common::String &frame = lineTok.nextToken();
				const int32 frameIdx = atoi(frame.c_str());
				frames.push_back(frameIdx);
			}
			_movieInfo.setVal(name, frames);
		} else {
			Common::Array<int32> info(1);
			info[0] = videoIndex;
			line.toLowercase();
			if (line.hasSuffix(".smk")) {
				line = line.substr(0, line.size() - 4);
			}
			_movieInfo.setVal(line, info);
			debugC(1, TwinE::kDebugResources, "movie name %s mapped to hqr index %i", line.c_str(), videoIndex);
			++videoIndex;
		}
	}
}

const Common::Array<int32> &Resources::getMovieInfo(const Common::String &name) const {
	return _movieInfo.getVal(name);
}

} // namespace TwinE

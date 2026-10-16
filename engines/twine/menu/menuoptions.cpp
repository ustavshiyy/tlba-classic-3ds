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

#include "twine/menu/menuoptions.h"
#include "common/error.h"
#include "common/keyboard.h"
#include "common/str-array.h"
#include "common/system.h"
#include "common/util.h"
#include "savestate.h"
#include "twine/audio/music.h"
#include "twine/audio/sound.h"
#include "twine/movies.h"
#include "twine/scene/gamestate.h"
#include "twine/input.h"
#include "twine/menu/interface.h"
#include "twine/menu/menu.h"
#include "twine/renderer/screens.h"
#include "twine/resources/resources.h"
#include "twine/scene/scene.h"
#include "twine/text.h"
#include "twine/twine.h"

namespace TwinE {

static const char allowedCharIndex[] = " ABCDEFGHIJKLM.NOPQRSTUVWXYZ-abcdefghijklm?nopqrstuvwxyz!0123456789\040\b\r\0";

#ifdef __3DS__
static const char *const k3DSKeyboardRows[] = {
	"1234567890",
	"QWERTYUIOP",
	"ASDFGHJKL",
	"ZXCVBNM"
};

static int qwertyRowLength(int row) {
	if (row >= 0 && row < 4) {
		return strlen(k3DSKeyboardRows[row]);
	}
	return 4; // ABC/abc, SPACE, DEL, OK
}

static char qwertyCharAt(int col, int row, bool uppercase) {
	if (row >= 0 && row < 4 && col >= 0 && col < qwertyRowLength(row)) {
		char chr = k3DSKeyboardRows[row][col];
		if (!uppercase && chr >= 'A' && chr <= 'Z') {
			chr = (char)(chr + ('a' - 'A'));
		}
		return chr;
	}
	if (row == 4 && col == 1) {
		return ' ';
	}
	return '\0';
}
#endif

void MenuOptions::newGame() {
	_engine->setTotalPlayTime(0);
	_engine->_music->stopMusic();
	_engine->_sound->stopSamples();

	if (_engine->isLBA1()) {
		int32 tmpFlagDisplayText = _engine->_cfgfile.FlagDisplayText;
		_engine->_cfgfile.FlagDisplayText = true;

		// intro screen 1 - twinsun
		_engine->_screens->loadImage(TwineImage(Resources::HQR_RESS_FILE, 15, 16));

		_engine->_text->_flagMessageShade = false;
		_engine->_text->_renderTextTriangle = true;

		_engine->_text->initDial(TextBankId::Inventory_Intro_and_Holomap);
		_engine->_text->bigWinDial();
		_engine->_text->setFontCrossColor(COLOR_WHITE);

		bool aborted = _engine->_text->drawTextProgressive(TextId::kIntroText1);

		// intro screen 2
		if (!aborted) {
			_engine->_screens->loadImage(TwineImage(Resources::HQR_RESS_FILE, 17, 18));
			aborted |= _engine->_text->drawTextProgressive(TextId::kIntroText2);

			if (!aborted) {
				_engine->_screens->loadImage(TwineImage(Resources::HQR_RESS_FILE, 19, 20));
				aborted |= _engine->_text->drawTextProgressive(TextId::kIntroText3);
			}
		}
		_engine->_cfgfile.FlagDisplayText = tmpFlagDisplayText;

#ifdef __3DS__
		_engine->_menu->fadeLowerScreenToBlack();
		_engine->_menu->requestLowerScreenFadeIn();
#endif
		_engine->_screens->fadeToBlack(_engine->_screens->_palettePcx);
		_engine->_screens->clearScreen();

		if (!aborted) {
			_engine->_music->playMidiFile(1);
			_engine->_movie->playMovie(FLA_INTROD);
		}

		_engine->_text->normalWinDial();
	} else {
		_engine->_movie->playMovie(ACF_INTRO);
	}
	_engine->_screens->clearScreen();

	_engine->_text->_flagMessageShade = true;
	_engine->_text->_renderTextTriangle = false;

	// set main palette back
	_engine->setPalette(_engine->_screens->_ptrPal);
#ifdef __3DS__
	// A skipped intro can leave the final PCX in both TwinE buffers. Merely calling
	// g_system->updateScreen() does not upload _frontVideoBuffer, so the next palette
	// change used to expose those stale indexed pixels as the psychedelic frame.
	// Clear both buffers, leave PCX mode, and force the black gameplay-palette frame
	// onto the physical upper LCD before the first scene is allowed to render.
	_engine->_screens->_flagPalettePcx = false;
	_engine->_frontVideoBuffer.fillRect(_engine->_frontVideoBuffer.getBounds(), COLOR_BLACK);
	_engine->_frontVideoBuffer.setPalette(_engine->_screens->_ptrPal.data(), 0, 256);
	_engine->_workVideoBuffer.fillRect(_engine->_workVideoBuffer.getBounds(), COLOR_BLACK);
	_engine->_workVideoBuffer.setPalette(_engine->_screens->_ptrPal.data(), 0, 256);
	_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
	_engine->_frontVideoBuffer.forceUpdate();
#endif
}

// TODO: dotemu has credits_<lang>.txt files
void MenuOptions::showCredits() {
	const int32 tmpShadowMode = _engine->_cfgfile.ShadowMode;
	_engine->_cfgfile.ShadowMode = 0;
	_engine->_gameState->initEngineVars();
	_engine->_scene->_numCube = LBA1SceneId::Credits_List_Sequence;
	_engine->_scene->_newCube = LBA1SceneId::Credits_List_Sequence;
	_engine->_screens->clearScreen();
#ifdef __3DS__
	// Credits are intentionally top-screen only in the standalone 3DS build.
	_engine->_menu->showLowerScreenBlack();
#endif

	flagCredits = true;
	_engine->mainLoop();
	_engine->_scene->stopRunningGame();
	flagCredits = false;

	_engine->_cfgfile.ShadowMode = tmpShadowMode;

	_engine->_screens->clearScreen();

	_engine->_input->enableKeyMap(uiKeyMapId);
}

void MenuOptions::showEndSequence() {
	_engine->_movie->playMovie(FLA_THEEND);

	_engine->_screens->clearScreen();
	_engine->setPalette(_engine->_screens->_ptrPal);
}

void MenuOptions::drawSelectableCharacter(int32 x, int32 y) {
	const int32 borderTop = 200;
	const int32 borderLeft = _engine->width() / 2 - 295;
	const int32 halfButtonHeight = 25;
	const int32 halfButtonWidth = 20;
	const int32 buttonDistanceX = halfButtonWidth * 2 + 5;
	const int32 buttonDistanceY = halfButtonHeight * 2 + 5;
	const int32 centerX = x * buttonDistanceX + borderLeft;
	const int32 centerY = y * buttonDistanceY + borderTop;
	const Common::Rect rect(centerX - halfButtonWidth, centerY - halfButtonHeight, centerX + halfButtonWidth, centerY + halfButtonHeight);

	if (_engine->_input->isMouseHovering(rect)) {
		setOnScreenKeyboard(x, y);
	}

	const int idx = x + y * ONSCREENKEYBOARD_WIDTH;
	if (_onScreenKeyboardDirty[idx] == 0) {
		return;
	}

	--_onScreenKeyboardDirty[idx];

	const char buffer[]{allowedCharIndex[idx], '\0'};

	const bool selected = _onScreenKeyboardX == x && _onScreenKeyboardY == y;
	if (selected) {
		_engine->_interface->box(rect, COLOR_91);
	} else {
		_engine->blitWorkToFront(rect);
		_engine->_interface->shadeBox(rect, 4);
	}

	_engine->_menu->drawRectBorders(rect);

	_engine->_text->setFontColor(COLOR_WHITE);
	const uint8 character = (uint8)allowedCharIndex[idx];
	const int32 textX = centerX - _engine->_text->getCharWidth(character) / 2;
	const int32 textY = centerY - _engine->_text->getCharHeight(character) / 2;
	_engine->_text->drawText(textX, textY, buffer);
}

void MenuOptions::setOnScreenKeyboard(int x, int y) {
#ifdef __3DS__
	if (y < 0) {
		y = 4;
	} else if (y > 4) {
		y = 0;
	}

	const int rowLen = qwertyRowLength(y);
	if (x < 0) {
		x = rowLen - 1;
	} else if (x >= rowLen) {
		x = 0;
	}

	// Moving vertically into a shorter QWERTY row clamps the selection to the
	// last real key instead of highlighting an empty slot.
	if (x >= rowLen) {
		x = rowLen - 1;
	}
#else
	if (x < 0) {
		x = ONSCREENKEYBOARD_WIDTH - 1;
	} else if (x >= ONSCREENKEYBOARD_WIDTH) {
		x = 0;
	}

	if (y < 0) {
		y = ONSCREENKEYBOARD_HEIGHT - 1;
	} else if (y >= ONSCREENKEYBOARD_HEIGHT) {
		y = 0;
	}
#endif

	if (_onScreenKeyboardX == x && _onScreenKeyboardY == y) {
		return;
	}

	++_onScreenKeyboardDirty[_onScreenKeyboardX + _onScreenKeyboardY * ONSCREENKEYBOARD_WIDTH];
	++_onScreenKeyboardDirty[x + y * ONSCREENKEYBOARD_WIDTH];

	_onScreenKeyboardX = x;
	_onScreenKeyboardY = y;

	_onScreenKeyboardLeaveViaOkButton = true;
}

void MenuOptions::drawSelectableCharacters() {
	for (int8 x = 0; x < ONSCREENKEYBOARD_WIDTH; x++) {
		for (int8 y = 0; y < ONSCREENKEYBOARD_HEIGHT; y++) {
			drawSelectableCharacter(x, y);
		}
	}
}

void MenuOptions::drawInputText(int32 centerx, int32 top, int32 type, const char *text) {
	const int32 left = 10;
	const int right = _engine->width() - left;
	const int bottom = top + PLASMA_HEIGHT;
	const Common::Rect rect(left, top, right, bottom);
	if (type == 1) {
		_engine->_menu->processPlasmaEffect(rect, 32);
	}

	Common::Rect rectBox(rect);
	rectBox.grow(-1);
	_engine->_menu->drawRectBorders(rect);
	_engine->_interface->shadeBox(rectBox, 3);

	_engine->_text->drawText(centerx - _engine->_text->sizeFont(text) / 2, top + 6, text);
	_engine->copyBlockPhys(rect);
}

/**
 * @brief Toggle a given @c OSystem::Feature and restore the previous state on destruction
 */
class ScopedFeatureState {
private:
	OSystem::Feature _feature;
	bool _changeTo;

public:
	ScopedFeatureState(OSystem::Feature feature, bool enable) : _feature(feature), _changeTo(enable) {
		if (g_system->getFeatureState(feature) != enable) {
			g_system->setFeatureState(feature, enable);
			_changeTo = !g_system->getFeatureState(feature);
		}
	}
	~ScopedFeatureState() {
		g_system->setFeatureState(_feature, _changeTo);
	}
};

bool MenuOptions::enterText(TextId textIdx, char *textTargetBuf, size_t bufSize) {
	_engine->_text->initDial(TextBankId::Options_and_menus);
	char buffer[256];
	_engine->_text->getMenuText(textIdx, buffer, sizeof(buffer));
	_engine->_text->setFontColor(COLOR_WHITE);
	const int halfScreenWidth = (_engine->width() / 2);
#ifndef __3DS__
	_engine->_text->drawText(halfScreenWidth - (_engine->_text->sizeFont(buffer) / 2), 20, buffer);
	_engine->copyBlockPhys(0, 0, _engine->width() - 1, 99);
#else
	// The 3DS frontend already has the clean LBA title on the upper LCD. Do not
	// reload or restore another framebuffer here: that produced a one-frame blank
	// flash between NEW GAME and the name keyboard. The lower keyboard restores
	// the cached logo after borrowing TwinE's native framebuffer for rendering.
#endif

	Common::fill(&_onScreenKeyboardDirty[0], &_onScreenKeyboardDirty[ARRAYSIZE(_onScreenKeyboardDirty)], 1);
	ScopedFeatureState scopedVirtualKeyboard(OSystem::kFeatureVirtualKeyboard, true);
#ifdef __3DS__
	_onScreenKeyboardX = 0;
	_onScreenKeyboardY = 0;
	_3DSKeyboardUppercase = true;
	_engine->_input->clearLowerScreenTap();
	_engine->_menu->showLowerScreenKeyboard(textTargetBuf, _onScreenKeyboardX, _onScreenKeyboardY, _3DSKeyboardUppercase);
#endif
	for (;;) {
		FrameMarker frame(_engine);
		Common::Event event;
		while (g_system->getEventManager()->pollEvent(event)) {
			switch (event.type) {
			case Common::EVENT_CUSTOM_ENGINE_ACTION_END:
				_engine->_input->processCustomEngineEventEnd(event);
				break;
			case Common::EVENT_CUSTOM_ENGINE_ACTION_START:
				_engine->_input->processCustomEngineEventStart(event);

#ifdef __3DS__
				if (event.customType == TwinEActionType::UIEnter && _engine->_input->isLowerScreenTouchHeld()) {
					_engine->_input->toggleActionIfActive(TwinEActionType::UIEnter);
					break;
				}
#endif
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
#ifdef __3DS__
					const size_t size = strlen(textTargetBuf);
					if (_onScreenKeyboardY == 4) {
						if (_onScreenKeyboardX == 0) { // ABC/abc
							_3DSKeyboardUppercase = !_3DSKeyboardUppercase;
							continue;
						}
						if (_onScreenKeyboardX == 3) { // OK
							if (textTargetBuf[0] == '\0') {
								continue;
							}
							return true;
						}
						if (_onScreenKeyboardX == 2) { // DEL
							if (size >= 1) {
								textTargetBuf[size - 1] = '\0';
							}
							continue;
						}
					}
					if (size >= bufSize - 1) {
						return true;
					}
					const char chr = qwertyCharAt(_onScreenKeyboardX, _onScreenKeyboardY, _3DSKeyboardUppercase);
					if (chr != '\0') {
						textTargetBuf[size] = chr;
						textTargetBuf[size + 1] = '\0';
					}
					continue;
#else
					if (_onScreenKeyboardLeaveViaOkButton) {
						if (_onScreenKeyboardX == ONSCREENKEYBOARD_WIDTH - 1 && _onScreenKeyboardY == ONSCREENKEYBOARD_HEIGHT - 1) {
							if (textTargetBuf[0] == '\0') {
								continue;
							}
							return true;
						}
						const size_t size = strlen(textTargetBuf);
						if (_onScreenKeyboardX == ONSCREENKEYBOARD_WIDTH - 2 && _onScreenKeyboardY == ONSCREENKEYBOARD_HEIGHT - 1) {
							if (size >= 1) {
								textTargetBuf[size - 1] = '\0';
							}
							continue;
						}
						const char chr = allowedCharIndex[_onScreenKeyboardX + _onScreenKeyboardY * ONSCREENKEYBOARD_WIDTH];
						textTargetBuf[size] = chr;
						textTargetBuf[size + 1] = '\0';
						if (size + 1 >= bufSize - 1) {
							return true;
						}
						continue;
					}
					if (textTargetBuf[0] == '\0') {
						continue;
					}

					return true;
#endif
				}
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UIAbort)) {
					return false;
				}
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
					setOnScreenKeyboard(_onScreenKeyboardX - 1, _onScreenKeyboardY);
				} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight)) {
					setOnScreenKeyboard(_onScreenKeyboardX + 1, _onScreenKeyboardY);
				}
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UIUp)) {
					setOnScreenKeyboard(_onScreenKeyboardX, _onScreenKeyboardY - 1);
				} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIDown)) {
					setOnScreenKeyboard(_onScreenKeyboardX, _onScreenKeyboardY + 1);
				}

				break;
			case Common::EVENT_KEYDOWN: {
				const size_t size = strlen(textTargetBuf);
				if (!Common::isPrint(event.kbd.ascii)) {
					if (event.kbd.keycode == Common::KEYCODE_BACKSPACE) {
						if (size >= 1) {
							textTargetBuf[size - 1] = '\0';
							_onScreenKeyboardLeaveViaOkButton = false;
						}
					}
					continue;
				}
				if (size >= bufSize - 1) {
					return true;
				}
				if (strchr(allowedCharIndex, event.kbd.ascii)) {
					textTargetBuf[size] = event.kbd.ascii;
					textTargetBuf[size + 1] = '\0';
					_onScreenKeyboardLeaveViaOkButton = false;
				}

				break;
			}
			default:
				break;
			}
		}
#ifdef __3DS__
		_engine->_input->pollLowerScreenTouch();
		Common::Point tap;
		if (_engine->_input->consumeLowerScreenTap(tap)) {
			int col = -1;
			int row = -1;

			const int keyW = 29;
			const int keyH = 26;
			const int firstTop = 58;
			const int rowStep = 28;

			for (int r = 0; r < 4 && row == -1; ++r) {
				const int count = qwertyRowLength(r);
				const int left = (320 - count * keyW) / 2;
				const int top = firstTop + r * rowStep;
				if (tap.y >= top && tap.y <= top + keyH &&
				    tap.x >= left && tap.x < left + count * keyW) {
					col = (tap.x - left) / keyW;
					row = r;
				}
			}

			if (row == -1 && tap.y >= 192 && tap.y <= 220) {
				row = 4;
				if (tap.x >= 15 && tap.x <= 68) {
					col = 0; // ABC/abc
				} else if (tap.x >= 71 && tap.x <= 178) {
					col = 1; // SPACE
				} else if (tap.x >= 181 && tap.x <= 238) {
					col = 2; // DEL
				} else if (tap.x >= 241 && tap.x <= 305) {
					col = 3; // OK
				} else {
					row = -1;
				}
			}

			if (row >= 0 && col >= 0) {
				setOnScreenKeyboard(col, row);
				const size_t size = strlen(textTargetBuf);
				if (row == 4 && col == 0) {
					_3DSKeyboardUppercase = !_3DSKeyboardUppercase;
				} else if (row == 4 && col == 3) {
					if (textTargetBuf[0] != '\0') {
						_engine->_input->clearLowerScreenTap();
						_engine->_menu->showLowerScreenIdlePanel();
						return true;
					}
				} else if (row == 4 && col == 2) {
					if (size >= 1) {
						textTargetBuf[size - 1] = '\0';
					}
				} else if (size < bufSize - 1) {
					const char chr = qwertyCharAt(col, row, _3DSKeyboardUppercase);
					if (chr != '\0') {
						textTargetBuf[size] = chr;
						textTargetBuf[size + 1] = '\0';
					}
				}
			}

			// A 3DS touch may also have produced the UI "next page" action;
			// it is not meaningful on the name keyboard.
			_engine->_input->toggleActionIfActive(TwinEActionType::UINextPage);
		}
		_engine->_menu->showLowerScreenKeyboard(textTargetBuf, _onScreenKeyboardX, _onScreenKeyboardY, _3DSKeyboardUppercase);
#endif
		if (_engine->shouldQuit()) {
			break;
		}
#ifndef __3DS__
		drawInputText(halfScreenWidth, 100, 1, textTargetBuf);
		drawSelectableCharacters();
#endif
	}
#ifdef __3DS__
	_engine->_input->clearLowerScreenTap();
	_engine->_menu->showLowerScreenIdlePanel();
#endif
	return false;
}

bool MenuOptions::newGameMenu() {
#ifndef __3DS__
	_engine->restoreFrontBuffer();
#endif
	_saveGameName[0] = '\0';
	if (!enterText(TextId::kEnterYourName, _saveGameName, sizeof(_saveGameName))) {
		return false;
	}
	_engine->_gameState->initEngineVars();
	newGame();
	return true;
}

int MenuOptions::chooseSave(TextId textIdx, bool showEmptySlots) {
	const SaveStateList &savegames = _engine->getSaveSlots();
	if (savegames.empty() && !showEmptySlots) {
		return -1;
	}

	_engine->_text->initDial(TextBankId::Options_and_menus);

	MenuSettings saveFiles;
	saveFiles.addButton(TextId::kReturnMenu);

#ifdef __3DS__
	// Return + exactly three profile slots on the standalone 3DS frontend.
	const int maxButtons = 4;
#else
	const int maxButtons = _engine->getMetaEngine()->getMaximumSaveSlot() + 1;
#endif
	uint savesIndex = 0;
	for (int i = 1; i < maxButtons; ++i) {
		if (savesIndex < savegames.size()) {
			const SaveStateDescriptor &savegame = savegames[savesIndex];
			if (savegame.getSaveSlot() == i - 1) {
				// manually creating a savegame should not overwrite the autosave slot
				if (textIdx != TextId::kCreateSaveGame || i > 1) {
					saveFiles.addButton(savegame.getDescription().c_str(), i);
				}
				++savesIndex;
			} else if (showEmptySlots) {
				saveFiles.addButton("EMPTY", i);
			}
		} else if (showEmptySlots) {
			saveFiles.addButton("EMPTY", i);
		}
	}

	const int32 id = _engine->_menu->doGameMenu(&saveFiles);
	switch (id) {
	case kQuitEngine:
	case (int32)TextId::kReturnMenu:
		return -1;
	default:
		const int slot = saveFiles.getButtonState(id) - 1;
		debug("Selected savegame slot %d", slot);
		return slot;
	}

	return -1;
}

bool MenuOptions::continueGameMenu() {
	_engine->restoreFrontBuffer();
	const int slot = chooseSave(TextId::kContinueGame);
	if (slot >= 0) {
		debug("Load slot %i", slot);
#ifdef __3DS__
		_engine->_active3DSSaveSlot = slot;
		_engine->_active3DSSaveName.clear();
		for (const SaveStateDescriptor &savegame : _engine->getSaveSlots()) {
			if (savegame.getSaveSlot() == slot) {
				_engine->_active3DSSaveName = savegame.getDescription();
				break;
			}
		}
#endif
		Common::Error state = _engine->loadGameState(slot);
		if (state.getCode() != Common::kNoError) {
			error("Failed to load slot %i", slot);
			return false;
		}

		return true;
	}
	return false;
}

bool MenuOptions::deleteSaveMenu() {
	_engine->restoreFrontBuffer();
	const int slot = chooseSave(TextId::kDeleteSaveGame);
	if (slot >= 0) {
		_engine->wipeSaveSlot(slot);
		return true;
	}
	return false;
}

bool MenuOptions::saveGameMenu() {
	if (!_engine->_scene->isGameRunning()) {
		return false;
	}
	_engine->restoreFrontBuffer();
	const int slot = chooseSave(TextId::kCreateSaveGame, true);
	if (slot >= 0) {
#ifdef __3DS__
		// A 3DS profile keeps the name entered at the beginning of the game even
		// when the player copies it to another one of the three slots.
		Common::String profileName = _engine->_active3DSSaveName;
		if (profileName.empty()) {
			for (const SaveStateDescriptor &savegame : _engine->getSaveSlots()) {
				if (savegame.getSaveSlot() == _engine->_active3DSSaveSlot) {
					profileName = savegame.getDescription();
					break;
				}
			}
		}
		if (profileName.empty()) {
			profileName = _engine->_gameState->_sceneName;
		}
		Common::Error state = _engine->saveGameState(slot, profileName, false);
#else
		char buf[30];
		strncpy(buf, _engine->_gameState->_sceneName, sizeof(buf));
		buf[sizeof(buf) - 1] = '\0';
		_engine->restoreFrontBuffer();
		enterText(TextId::kEnterYourNewName, buf, sizeof(buf));
		// may not be empty
		if (buf[0] == '\0') {
			Common::strlcpy(buf, _engine->_gameState->_sceneName, sizeof(buf));
		}
		Common::Error state = _engine->saveGameState(slot, buf, false);
#endif
		if (state.getCode() != Common::kNoError) {
			error("Failed to save slot %i", slot);
			return false;
		}

		return true;
	}
	return false;
}

} // namespace TwinE

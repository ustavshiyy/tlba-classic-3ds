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

#include <3ds.h>
#include "twine/menu/menu.h"
#include "audio/mixer.h"
#include "backends/audiocd/audiocd.h"
#include "backends/keymapper/keymapper.h"
#include "common/config-manager.h"
#include "common/events.h"
#include "common/keyboard.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "common/system.h"
#include "common/util.h"
#include "graphics/cursorman.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/palette.h"
#include "engines/savestate.h"
#include "twine/audio/music.h"
#include "twine/audio/sound.h"
#include "twine/input.h"
#include "twine/menu/interface.h"
#include "twine/menu/menuoptions.h"
#include "twine/movies.h"
#include "twine/renderer/redraw.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "twine/scene/actor.h"
#include "twine/scene/animations.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/grid.h"
#include "twine/scene/movements.h"
#include "twine/scene/scene.h"
#include "twine/shared.h"
#include "twine/text.h"
#include "twine/twine.h"
#ifdef __3DS__
#endif

namespace TwinE {

static const uint32 kPlasmaEffectFilesize = 262176;

#define HEIGHT_STANDARD 50
#define MENU_SPACE 6

#define	SIZE_INV_OBJ_X	75
#define	SIZE_INV_OBJ_Y	65

namespace MenuButtonTypes {
enum MenuButtonTypesEnum {
	kMusicVolume = 1,
	kSoundVolume = 2,
	kCDVolume = 3,
	kSpeechVolume = 4,
	kAggressiveMode = 5,
	kPolygonDetails = 6,
	kShadowSettings = 7,
	kSceneryZoom = 8,
	kHighResolution = 9,
	kWallCollision = 10,
	kVoice = 11,
	kTextLanguage = 12
};
}

#define checkMenuQuit(callMenu)      \
	if ((callMenu) == kQuitEngine) { \
		return kQuitEngine;          \
	}

namespace _priv {

static MenuSettings createMainMenu(bool lba1) {
	MenuSettings settings;
	if (lba1) {
		settings.setButtonsBoxHeight(200);
		settings.addButton(TextId::kNewGame);
		settings.addButton(TextId::kContinueGame);
		settings.addButton(TextId::kOptions);
		settings.addButton(TextId::kQuit);
	} else {
		settings.setButtonsBoxHeight(335);
		settings.addButton(TextId::toContinueGame);
		settings.addButton(TextId::toNewGame);
		settings.addButton(TextId::toOptions);
		settings.addButton(TextId::toQuit);
	}
	return settings;
}

static MenuSettings createLba1ClassicNewGame() {
	MenuSettings settings;
	settings.addButton(TextId::kReturnMenu);
	settings.addButton(TextId::kNewGame);
	settings.addButton(TextId::kNewGamePlus);
	return settings;
}

static MenuSettings createGiveUpMenu() {
	MenuSettings settings;
#ifdef __3DS__
	// Raise the in-game pause menu slightly on the 240 px touch screen.
	settings.setButtonsBoxHeight(265);
#else
	settings.setButtonsBoxHeight(285);
#endif
	settings.addButton(TextId::kContinue);
	settings.addButton(TextId::kOptions);
	settings.addButton(TextId::kGiveUp);
	return settings;
}

static MenuSettings createGiveUpSaveMenu() {
	MenuSettings settings;
#ifdef __3DS__
	// Same vertical alignment as the compact pause menu above.
	settings.setButtonsBoxHeight(280);
#else
	settings.setButtonsBoxHeight(300);
#endif
	settings.addButton(TextId::kContinue);
	settings.addButton(TextId::kCreateSaveGame);
	settings.addButton(TextId::kOptions);
	settings.addButton(TextId::kGiveUp);
	return settings;
}

static MenuSettings createLanguageMenu(bool lba1) {
	MenuSettings settings;
	settings.addButton(TextId::kReturnMenu);
	settings.addButton("Texts: English", MenuButtonTypes::kTextLanguage);
	settings.addButton(TextId::kCustomVoicesEnglish, MenuButtonTypes::kVoice);
	return settings;
}

static MenuSettings createOptionsMenu(bool lba1) {
	MenuSettings settings;
	settings.addButton(TextId::kReturnMenu);
	settings.addButton(TextId::kVolumeSettings);
	settings.addButton(TextId::kSaveManage);
	settings.addButton(TextId::kCustomLanguageOption);
	settings.addButton(TextId::kAdvanced);
	return settings;
}

static MenuSettings createAdvancedOptionsMenu() {
	MenuSettings settings;
	settings.addButton(TextId::kReturnMenu);
	settings.addButton(TextId::kBehaviourAggressiveManual, MenuButtonTypes::kAggressiveMode);
	settings.addButton(TextId::kDetailsPolygonsHigh, MenuButtonTypes::kPolygonDetails);
	settings.addButton(TextId::kDetailsShadowHigh, MenuButtonTypes::kShadowSettings);
	settings.addButton(TextId::kSceneryZoomOn, MenuButtonTypes::kSceneryZoom);
	settings.addButton(TextId::kCustomHighResOptionOn, MenuButtonTypes::kHighResolution);
	settings.addButton(TextId::kCustomWallCollisionOff, MenuButtonTypes::kWallCollision);
	return settings;
}

static MenuSettings createSaveManageMenu() {
	MenuSettings settings;
	settings.addButton(TextId::kReturnMenu);
	settings.addButton(TextId::kCreateSaveGame);
	settings.addButton(TextId::kDeleteSaveGame);
	return settings;
}

static MenuSettings createVolumeMenu() {
	MenuSettings settings;
	settings.addButton(TextId::kReturnMenu);
	settings.addButton(TextId::kMusicVolume, MenuButtonTypes::kMusicVolume);
	settings.addButton(TextId::kSoundVolume, MenuButtonTypes::kSoundVolume);
	settings.addButton(TextId::kCDVolume, MenuButtonTypes::kCDVolume);
	settings.addButton(TextId::kSpeechVolume, MenuButtonTypes::kSpeechVolume);
	return settings;
}

} // namespace _priv

const char *MenuSettings::getButtonText(Text *text, int buttonIndex) {
	if (_buttonTexts[buttonIndex].empty()) {
		TextId textId = getButtonTextId(buttonIndex);
		char dialText[256] = "";
		if (textId == TextId::kNewGamePlus) {
			text->getMenuText(TextId::kNewGame, dialText, sizeof(dialText));
			Common::strlcat(dialText, "+", sizeof(dialText));
		} else {
			text->getMenuText(textId, dialText, sizeof(dialText));
		}
		_buttonTexts[buttonIndex] = dialText;
	}
	return _buttonTexts[buttonIndex].c_str();
}

Menu::Menu(TwinEEngine *engine) {
	_engine = engine;

	_optionsMenuState = _priv::createOptionsMenu(engine->isLBA1());
	_languageMenuState = _priv::createLanguageMenu(engine->isLBA1());
	_giveUpMenuWithSaveState = _priv::createGiveUpSaveMenu();
	_volumeMenuState = _priv::createVolumeMenu();
	_saveManageMenuState = _priv::createSaveManageMenu();
	_giveUpMenuState = _priv::createGiveUpMenu();
	_mainMenuState = _priv::createMainMenu(engine->isLBA1());
	_newGameMenuState = _priv::createLba1ClassicNewGame();
	_advOptionsMenuState = _priv::createAdvancedOptionsMenu();

	Common::fill(&_behaviourAnimState[0], &_behaviourAnimState[4], 0);
	Common::fill(&_itemAngle[0], &_itemAngle[NUM_INVENTORY_ITEMS], 0);
}

Menu::~Menu() {
	free(_plasmaEffectPtr);
}

void Menu::plasmaEffectRenderFrame() {
	for (int32 j = 1; j < PLASMA_HEIGHT - 1; j++) {
		for (int32 i = 1; i < PLASMA_WIDTH - 1; i++) {
			/* Here we calculate the average of all 8 neighbour pixel values */

			int16 c;
			c = _plasmaEffectPtr[(i - 1) + (j - 1) * PLASMA_WIDTH];  // top-left
			c += _plasmaEffectPtr[(i + 0) + (j - 1) * PLASMA_WIDTH]; // top
			c += _plasmaEffectPtr[(i + 1) + (j - 1) * PLASMA_WIDTH]; // top-right

			c += _plasmaEffectPtr[(i - 1) + (j + 0) * PLASMA_WIDTH]; // left
			c += _plasmaEffectPtr[(i + 1) + (j + 0) * PLASMA_WIDTH]; // right

			c += _plasmaEffectPtr[(i - 1) + (j + 1) * PLASMA_WIDTH]; // bottom-left
			c += _plasmaEffectPtr[(i + 0) + (j + 1) * PLASMA_WIDTH]; // bottom
			c += _plasmaEffectPtr[(i + 1) + (j + 1) * PLASMA_WIDTH]; // bottom-right

			/* And the 3 least significant bits are used as a
			 * randomizing parameter for statistically fading the flames */
			c = (c >> 3) | ((c & 0x0007) << 13);

			if (!(c & 0x6500) &&
				(j >= (PLASMA_HEIGHT - 4) || c > 0)) {
				c--; /*fade this pixel*/
			}

			/* plot the pixel using the calculated color */
			_plasmaEffectPtr[i + (PLASMA_HEIGHT + j) * PLASMA_WIDTH] = (uint8)c;
		}
	}

	// flip the double-buffer while scrolling the effect vertically:
	const uint8 *src = _plasmaEffectPtr + (PLASMA_HEIGHT + 1) * PLASMA_WIDTH;
	memcpy(_plasmaEffectPtr, src, PLASMA_HEIGHT * PLASMA_WIDTH);
}

void Menu::processPlasmaEffect(const Common::Rect &rect, int32 color) {
	if (_engine->isLBA2()) {
		// LBA2 uses palette bank 12 (192) for selected items
		color = 192;
	}
	const int32 max_value = color + 15;

	plasmaEffectRenderFrame();

	const uint8 *in = _plasmaEffectPtr + 5 * PLASMA_WIDTH;
	uint8 *out = (uint8 *)_engine->_imageBuffer.getBasePtr(0, 0);
	const int32 bufWidth = _engine->_imageBuffer.w;

	// Write plasma at native 320x25 resolution, each source row doubled vertically
	// to produce 320x50 output matching the original DoFire rendering
	for (int32 y = 0; y < PLASMA_HEIGHT / 2; y++) {
		const uint8 *colPtr = &in[y * PLASMA_WIDTH];
		uint8 *row0 = out + (y * 2) * bufWidth;
		uint8 *row1 = row0 + bufWidth;
		for (int32 x = 0; x < PLASMA_WIDTH; x++) {
			const uint8 c = MIN(*colPtr / 2 + color, max_value);
			row0[x] = c;
			row1[x] = c;
			++colPtr;
		}
	}
	const Common::Rect prect(0, 0, PLASMA_WIDTH, PLASMA_HEIGHT);
	_engine->_frontVideoBuffer.blitFrom(_engine->_imageBuffer, prect, rect);
}

void Menu::drawRectBorders(const Common::Rect &rect, int32 colorLeftTop, int32 colorRightBottom) { // DrawCadre
	_engine->_interface->drawLine(rect.left, rect.top, rect.right, rect.top, colorLeftTop);                   // top line
	_engine->_interface->drawLine(rect.left, rect.top + 1, rect.left, rect.bottom, colorLeftTop);             // left line
	_engine->_interface->drawLine(rect.right, rect.top + 1, rect.right, rect.bottom, colorRightBottom);       // right line
	_engine->_interface->drawLine(rect.left + 1, rect.bottom, rect.right - 1, rect.bottom, colorRightBottom); // bottom line
}

void Menu::drawRectBorders(int32 left, int32 top, int32 right, int32 bottom, int32 colorLeftTop, int32 colorRightBottom) { // DrawCadre
	drawRectBorders(Common::Rect(left, top, right, bottom), colorLeftTop, colorLeftTop);
}

void Menu::drawButtonGfx(const MenuSettings *menuSettings, const Common::Rect &rect, int32 buttonId, const char *dialText, bool hover) {
	if (hover) {
		if (menuSettings == &_volumeMenuState && buttonId <= MenuButtonTypes::kSpeechVolume && buttonId >= MenuButtonTypes::kMusicVolume) {
			int32 newWidth = 0;
			switch (buttonId) {
			case MenuButtonTypes::kMusicVolume: {
				const int volume = _engine->_system->getMixer()->getVolumeForSoundType(Audio::Mixer::kMusicSoundType);
				newWidth = ruleThree32(rect.left, rect.right, Audio::Mixer::kMaxMixerVolume, volume);
				break;
			}
			case MenuButtonTypes::kSoundVolume: {
				const int volume = _engine->_system->getMixer()->getVolumeForSoundType(Audio::Mixer::kSFXSoundType);
				newWidth = ruleThree32(rect.left, rect.right, Audio::Mixer::kMaxMixerVolume, volume);
				break;
			}
			case MenuButtonTypes::kCDVolume: {
				const AudioCDManager::Status status = _engine->_system->getAudioCDManager()->getStatus();
				newWidth = ruleThree32(rect.left, rect.right, Audio::Mixer::kMaxMixerVolume, status.volume);
				break;
			}
			case MenuButtonTypes::kSpeechVolume: {
				const int volume = _engine->_system->getMixer()->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType);
				newWidth = ruleThree32(rect.left, rect.right, Audio::Mixer::kMaxMixerVolume, volume);
				break;
			}
			}

			processPlasmaEffect(rect, COLOR_80);
			if (!(_engine->getRandomNumber() % 5)) {
				_plasmaEffectPtr[(_engine->getRandomNumber() % PLASMA_WIDTH * 10) + 20 * PLASMA_WIDTH] = 255;
			}
			_engine->_interface->box(Common::Rect(newWidth, rect.top, rect.right, rect.bottom), COLOR_SELECT_MENU);
		} else {
			processPlasmaEffect(rect, COLOR_64);
			if (!(_engine->getRandomNumber() % 5)) {
				_plasmaEffectPtr[_engine->getRandomNumber() % PLASMA_WIDTH * 10 + 6400] = 255;
			}
		}
	} else {
		_engine->blitWorkToFront(rect);
		_engine->_interface->shadeBox(rect, 4);
	}

	drawRectBorders(rect);

	_engine->_text->setFontColor(COLOR_WHITE);
	_engine->_text->setFont(INTER_LEAVE, INTER_SPACE);
	const int32 textSize = _engine->_text->sizeFont(dialText);
	_engine->_text->drawText((_engine->width() / 2) - (textSize / 2), rect.top + 7, dialText);
}

static const char *languageConfigCode(int languageId) {
	static const char *const kCodes[] = {"en", "fr", "de", "es", "it", "pt"};
	if (languageId < 0 || languageId >= (int)ARRAYSIZE(kCodes)) {
		return kCodes[0];
	}
	return kCodes[languageId];
}

int16 Menu::drawButtons(MenuSettings *menuSettings, bool hover) {
	int16 buttonNumber = menuSettings->getActiveButton();
	const int32 maxButton = menuSettings->getButtonCount();
	int32 topHeight = menuSettings->getButtonBoxHeight();

	if (topHeight == 0) {
		topHeight = 35;
	} else {
		topHeight = topHeight - (((maxButton - 1) * 6) + (maxButton * HEIGHT_STANDARD)) / 2;
	}

	if (maxButton <= 0) {
		return -1;
	}

	int16 mouseActiveButton = -1;

	for (int16 i = 0; i < maxButton; ++i) {
		if (menuSettings == &_advOptionsMenuState) {
			int16 id = menuSettings->getButtonState(i);
			switch (id) {
			case MenuButtonTypes::kAggressiveMode:
				if (_engine->_actor->_combatAuto) {
					menuSettings->setButtonTextId(i, TextId::kBehaviourAggressiveAuto);
				} else {
					menuSettings->setButtonTextId(i, TextId::kBehaviourAggressiveManual);
				}
				break;
			case MenuButtonTypes::kPolygonDetails:
				if (_engine->_cfgfile.PolygonDetails == 0) {
					menuSettings->setButtonTextId(i, TextId::kDetailsPolygonsLow);
				} else if (_engine->_cfgfile.PolygonDetails == 1) {
					menuSettings->setButtonTextId(i, TextId::kDetailsPolygonsMiddle);
				} else {
					menuSettings->setButtonTextId(i, TextId::kDetailsPolygonsHigh);
				}
				break;
			case MenuButtonTypes::kShadowSettings:
				if (_engine->_cfgfile.ShadowMode == 0) {
					menuSettings->setButtonTextId(i, TextId::kShadowsDisabled);
				} else if (_engine->_cfgfile.ShadowMode == 1) {
					menuSettings->setButtonTextId(i, TextId::kShadowsFigures);
				} else {
					menuSettings->setButtonTextId(i, TextId::kDetailsShadowHigh);
				}
				break;
			case MenuButtonTypes::kSceneryZoom:
				if (_engine->_cfgfile.SceZoom) {
					menuSettings->setButtonTextId(i, TextId::kSceneryZoomOn);
				} else {
					menuSettings->setButtonTextId(i, TextId::kNoSceneryZoom);
				}
				break;
			case MenuButtonTypes::kHighResolution: {
				if (ConfMan.getBool("usehighres")) {
					menuSettings->setButtonTextId(i, TextId::kCustomHighResOptionOn);
				} else {
					menuSettings->setButtonTextId(i, TextId::kCustomHighResOptionOff);
				}
				break;
			}
			case MenuButtonTypes::kWallCollision: {
				if (ConfMan.getBool("wallcollision")) {
					menuSettings->setButtonTextId(i, TextId::kCustomWallCollisionOn);
				} else {
					menuSettings->setButtonTextId(i, TextId::kCustomWallCollisionOff);
				}
				break;
			}
			default:
				break;
			}
		} else if (menuSettings == &_languageMenuState) {
			int16 id = menuSettings->getButtonState(i);
			switch (id) {
			case MenuButtonTypes::kTextLanguage: {
				const int languageId = CLIP<int>(_engine->_cfgfile._languageId, 0, (int)ARRAYSIZE(ListLanguage) - 1);
				menuSettings->setButtonText(i, Common::String::format("Texts: %s", ListLanguage[languageId].name));
				break;
			}
			case MenuButtonTypes::kVoice: {
				const int voiceLanguage = ConfMan.getInt("audio_language");
				menuSettings->setButtonTextId(i, (TextId)((int)TextId::kCustomVoicesNone - voiceLanguage));
				break;
			}
			default:
				break;
			}
		}
		const int32 menuItemId = menuSettings->getButtonState(i);
		const char *text = menuSettings->getButtonText(_engine->_text, i);
		const int32 border = 45;
		const int32 mainMenuButtonHeightHalf = HEIGHT_STANDARD / 2;
		const Common::Rect rect(border, topHeight - mainMenuButtonHeightHalf, _engine->width() - border, topHeight + mainMenuButtonHeightHalf);
		if (hover) {
			if (i == buttonNumber) {
				drawButtonGfx(menuSettings, rect, menuItemId, text, hover);
			}
		} else {
			if (i == buttonNumber) {
				drawButtonGfx(menuSettings, rect, menuItemId, text, true);
			} else {
				drawButtonGfx(menuSettings, rect, menuItemId, text, false);
			}
		}
		if (_engine->_input->isMouseHovering(rect)) {
			mouseActiveButton = i;
		}

		topHeight += HEIGHT_STANDARD + MENU_SPACE; // increase button top height
	}
	return mouseActiveButton;
}

void Menu::menuDemo() {
	// TODO: lba2 only show the credits only in the main menu and you could force it by pressing shift+c
	if (_engine->isLBA2()) {
		_engine->_music->playCdTrack(2);
		_engine->_menuOptions->showCredits();
		_engine->_screens->loadMenuImage(false);
		return;
	}
	_engine->_menuOptions->showCredits();
	if (_engine->_movie->playMovie(FLA_DRAGON3)) {
		if (!_engine->_screens->loadImageDelay(TwineImage(Resources::HQR_RESS_FILE, 15, 16), 3)) {
			if (!_engine->_screens->loadImageDelay(TwineImage(Resources::HQR_RESS_FILE, 17, 18), 3)) {
				if (!_engine->_screens->loadImageDelay(TwineImage(Resources::HQR_RESS_FILE, 19, 20), 3)) {
					if (_engine->_movie->playMovie(FLA_BATEAU)) {
						if (_engine->_cfgfile.Version == USA_VERSION) {
							_engine->_screens->loadImageDelay(_engine->_resources->relentLogo(), 3);
						} else {
							_engine->_screens->loadImageDelay(_engine->_resources->lbaLogo(), 3);
						}
						_engine->_screens->adelineLogo();
					}
				}
			}
		}
	}
	_engine->_screens->loadMenuImage(false);
}

#ifdef __3DS__
static int lowerScreenMenuButtonAt(const MenuSettings *menuSettings, const Common::Point &tap, int sourceW, int sourceH);
#endif

int32 Menu::doGameMenu(MenuSettings *menuSettings) {
	int16 currentButton = menuSettings->getActiveButton();
	bool buttonsNeedRedraw = true;
	const int32 numEntry = menuSettings->getButtonCount();
	int32 maxButton = numEntry - 1;
#ifdef __3DS__
	const bool renderOnLower = _lowerScreenClassicMenuActive && _engine->_scene->isGameRunning();
	bool touchActivate = false;
	uint32 lowerMenuLastPresent = 0;
	// Touch/D-pad own this UI on 3DS. Never leave the desktop-style mouse
	// pointer painted over the lower-screen LBA menu.
	if (renderOnLower) {
		CursorMan.showMouse(false);
	}
#endif

	_engine->_input->enableKeyMap(uiKeyMapId);

	// if we are running the game already, the buttons are just rendered on top of the scene
	if (_engine->_scene->isGameRunning()) {
		_engine->restoreFrontBuffer();
	} else {
		_engine->_screens->loadMenuImage(false);
	}
	uint32 startMillis = _engine->_system->getMillis();
	do {
#ifdef __3DS__
		touchActivate = false;
#endif
		FrameMarker frame(_engine);
		const uint32 loopMillis = _engine->_system->getMillis();
		_engine->readKeys();
#ifdef __3DS__
		if (renderOnLower) {
			Common::Point tap;
			if (_engine->_input->consumeLowerScreenTap(tap)) {
				const int tapped = lowerScreenMenuButtonAt(menuSettings, tap, _engine->width(), _engine->height());
				if (tapped >= 0) {
					if (tapped == currentButton) {
						touchActivate = true;
					} else {
						currentButton = tapped;
						menuSettings->setActiveButton(currentButton);
						buttonsNeedRedraw = true;
						startMillis = loopMillis;
					}
				}
			}
		}
#endif

		if (_engine->_input->toggleActionIfActive(TwinEActionType::UIDown)) {
			currentButton++;
			if (currentButton == numEntry) { // if current button is the last, than next button is the first
				currentButton = 0;
			}
			buttonsNeedRedraw = true;
			startMillis = loopMillis;
		} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIUp)) {
			currentButton--;
			if (currentButton < 0) { // if current button is the first, than previous button is the last
				currentButton = maxButton;
			}
			buttonsNeedRedraw = true;
			startMillis = loopMillis;
		}
#ifdef __3DS__
		if (renderOnLower) {
			menuSettings->setActiveButton(currentButton);
		}
#endif

		const int16 id = menuSettings->getActiveButtonState();
		if (menuSettings == &_advOptionsMenuState) {
			switch (id) {
			case MenuButtonTypes::kAggressiveMode:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft) || _engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					_engine->_actor->_combatAuto = !_engine->_actor->_combatAuto;
					startMillis = loopMillis;
				}
				break;
			case MenuButtonTypes::kPolygonDetails:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
					_engine->_cfgfile.PolygonDetails--;
					_engine->_cfgfile.PolygonDetails %= 3;
					startMillis = loopMillis;
				} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					_engine->_cfgfile.PolygonDetails++;
					_engine->_cfgfile.PolygonDetails %= 3;
					startMillis = loopMillis;
				}
				break;
			case MenuButtonTypes::kShadowSettings:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
					_engine->_cfgfile.ShadowMode--;
					_engine->_cfgfile.ShadowMode %= 3;
					startMillis = loopMillis;
				} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					_engine->_cfgfile.ShadowMode++;
					_engine->_cfgfile.ShadowMode %= 3;
					startMillis = loopMillis;
				}
				break;
			case MenuButtonTypes::kSceneryZoom:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft) || _engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					_engine->_cfgfile.SceZoom = !_engine->_cfgfile.SceZoom;
					startMillis = loopMillis;
				}
				break;
			case MenuButtonTypes::kHighResolution:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft) || _engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					const bool highRes = ConfMan.getBool("usehighres");
					ConfMan.setBool("usehighres", !highRes);
					startMillis = loopMillis;
				}
				break;
			case MenuButtonTypes::kWallCollision:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft) || _engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					const bool highRes = ConfMan.getBool("wallcollision");
					ConfMan.setBool("wallcollision", !highRes);
					startMillis = loopMillis;
				}
				break;
			default:
				break;
			}
		} else if (menuSettings == &_languageMenuState) {
			switch (id) {
			case MenuButtonTypes::kTextLanguage: {
				int direction = 0;
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
					direction = -1;
				} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight) ||
				           _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					direction = 1;
				}
				if (direction != 0) {
					// Some historical language table entries have no text bank (Portuguese
					// in LBA1 uses an empty id). Skip those entries while cycling so
					// Italian -> Right wraps straight back to English, just like Voices.
					int languageId = _engine->_cfgfile._languageId;
					for (int tries = 0; tries < (int)ARRAYSIZE(ListLanguage); ++tries) {
						languageId += direction;
						if (languageId < 0) {
							languageId = ARRAYSIZE(ListLanguage) - 1;
						} else if (languageId >= (int)ARRAYSIZE(ListLanguage)) {
							languageId = 0;
						}
						if (ListLanguage[languageId].id && ListLanguage[languageId].id[0] != '\0') {
							break;
						}
					}

					const int oldLanguageId = _engine->_cfgfile._languageId;
					_engine->setGameLanguage(languageId);
					if (_engine->_resources->reloadTextBanks()) {
						ConfMan.set("language", languageConfigCode(languageId));
						_engine->_text->resetLanguageState();
						_engine->_text->initDial(TextBankId::Options_and_menus);
						_optionsMenuState.clearButtonTextCache();
						_languageMenuState.clearButtonTextCache();
						_volumeMenuState.clearButtonTextCache();
						_saveManageMenuState.clearButtonTextCache();
						_advOptionsMenuState.clearButtonTextCache();
						_giveUpMenuState.clearButtonTextCache();
						_giveUpMenuWithSaveState.clearButtonTextCache();
						_mainMenuState.clearButtonTextCache();
						_newGameMenuState.clearButtonTextCache();
						ConfMan.flushToDisk();
					} else {
						// Keep a failed/missing language bank from leaving half of the
						// game's text resources switched and half in the previous language.
						_engine->setGameLanguage(oldLanguageId);
						_engine->_resources->reloadTextBanks();
					}
					buttonsNeedRedraw = true;
					startMillis = loopMillis;
				}
				break;
			}
			case MenuButtonTypes::kVoice:
				if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
					int voiceLanguage = ConfMan.getInt("audio_language");
					--voiceLanguage;
					if (voiceLanguage < 0) {
						voiceLanguage = 3;
					}
					ConfMan.setInt("audio_language", voiceLanguage);
					buttonsNeedRedraw = true;
					startMillis = loopMillis;
				} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight) || _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
					int voiceLanguage = ConfMan.getInt("audio_language");
					++voiceLanguage;
					if (voiceLanguage > 3) {
						voiceLanguage = 0;
					}
					ConfMan.setInt("audio_language", voiceLanguage);
					buttonsNeedRedraw = true;
					startMillis = loopMillis;
				}
				break;
			default:
				break;
			}
		} else if (menuSettings == &_volumeMenuState) {
			Audio::Mixer *mixer = _engine->_system->getMixer();
			switch (id) {
			case MenuButtonTypes::kMusicVolume: {
				int volume = mixer->getVolumeForSoundType(Audio::Mixer::SoundType::kMusicSoundType);
				if (_engine->_input->isActionActive(TwinEActionType::UILeft)) {
					volume -= 4;
					startMillis = loopMillis;
				} else if (_engine->_input->isActionActive(TwinEActionType::UIRight)) {
					volume += 4;
					startMillis = loopMillis;
				}
				_engine->_music->musicVolume(volume);
				ConfMan.setInt("music_volume", mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType));
				break;
			}
			case MenuButtonTypes::kSoundVolume: {
				int volume = mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType);
				if (_engine->_input->isActionActive(TwinEActionType::UILeft)) {
					volume -= 4;
					startMillis = loopMillis;
				} else if (_engine->_input->isActionActive(TwinEActionType::UIRight)) {
					volume += 4;
					startMillis = loopMillis;
				}

				mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, volume);
				ConfMan.setInt("sfx_volume", mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType));
				break;
			}
			case MenuButtonTypes::kCDVolume: {
				AudioCDManager::Status status = _engine->_system->getAudioCDManager()->getStatus();
				if (_engine->_input->isActionActive(TwinEActionType::UILeft)) {
					status.volume -= 4;
					startMillis = loopMillis;
				} else if (_engine->_input->isActionActive(TwinEActionType::UIRight)) {
					status.volume += 4;
					startMillis = loopMillis;
				}
				status.volume = CLIP(status.volume, 0, 255);
				_engine->_system->getAudioCDManager()->setVolume(status.volume);
				break;
			}
			case MenuButtonTypes::kSpeechVolume: {
				int volume = mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType);
				if (_engine->_input->isActionActive(TwinEActionType::UILeft)) {
					volume -= 4;
					startMillis = loopMillis;
				} else if (_engine->_input->isActionActive(TwinEActionType::UIRight)) {
					volume += 4;
					startMillis = loopMillis;
				}
				mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, volume);
				ConfMan.setInt("speech_volume", mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType));
				break;
			}
			default:
				break;
			}
		}

#ifdef __3DS__
		if (renderOnLower) {
			// Keep the gameplay frame untouched on the upper LCD, but use the same
			// classic ship artwork as the startup SAVE selector behind the in-game
			// menu on the touch screen.
			const bool animationDue = lowerMenuLastPresent == 0 || loopMillis - lowerMenuLastPresent >= 100;
			if (buttonsNeedRedraw || animationDue) {
				if (_lowerScreenBackup.w != _engine->_workVideoBuffer.w ||
				    _lowerScreenBackup.h != _engine->_workVideoBuffer.h ||
				    _lowerScreenBackup.format != _engine->_workVideoBuffer.format) {
					_lowerScreenBackup.free();
					_lowerScreenBackup.create(_engine->_workVideoBuffer.w, _engine->_workVideoBuffer.h,
					                          _engine->_workVideoBuffer.format);
				}
				_lowerScreenBackup.blitFrom(_engine->_workVideoBuffer);

				byte gameplayPalette[256 * 3];
				_engine->_frontVideoBuffer.getPalette(gameplayPalette, 0, 256);
				if (_lowerScreenMenuBaseReady) {
					_engine->_workVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
					_engine->_frontVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
					_engine->_workVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
					_engine->_frontVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
				} else {
					// Defensive fallback for unusual direct-launch paths where the startup
					// menu artwork was never cached.
					_engine->_workVideoBuffer.fillRect(_engine->_workVideoBuffer.getBounds(), COLOR_BLACK);
					_engine->_frontVideoBuffer.fillRect(_engine->_frontVideoBuffer.getBounds(), COLOR_BLACK);
				}

				menuSettings->setActiveButton(currentButton);
				drawButtons(menuSettings, false);
				drawButtons(menuSettings, true);
				copyFrontBufferToLowerScreen();

				_engine->_workVideoBuffer.blitFrom(_lowerScreenBackup);
				_engine->_frontVideoBuffer.blitFrom(_lowerScreenBackup);
				_engine->_workVideoBuffer.setPalette(gameplayPalette, 0, 256);
				_engine->_frontVideoBuffer.setPalette(gameplayPalette, 0, 256);
				buttonsNeedRedraw = false;
				lowerMenuLastPresent = loopMillis;
			}
		} else {
#endif
			if (buttonsNeedRedraw) {
				// draw all buttons
				const int16 mouseButtonHovered = drawButtons(menuSettings, false);
				if (mouseButtonHovered != -1) {
					currentButton = mouseButtonHovered;
				}
				menuSettings->setActiveButton(currentButton);
			}

			// draw plasma effect for the current selected button
			const int16 mouseButtonHovered = drawButtons(menuSettings, true);
			if (mouseButtonHovered != -1) {
				if (mouseButtonHovered != currentButton) {
					buttonsNeedRedraw = true;
				}
				currentButton = mouseButtonHovered;
			}
#ifdef __3DS__
		}
#endif

		if (_engine->shouldQuit()) {
			return kQuitEngine;
		}
		if (_engine->_input->toggleActionIfActive(TwinEActionType::UIAbort)) {
			for (int i = 0; i < menuSettings->getButtonCount(); ++i) {
				const TextId textId = menuSettings->getButtonTextId(i);
				if (textId == TextId::kReturnMenu || textId == TextId::kReturnGame || textId == TextId::kContinue) {
					return (int32)textId;
				}
			}
			startMillis = loopMillis;
		}
		if (menuSettings == &_mainMenuState) {
			uint32 idleTime = 60 * 3 + 53 * 1000;
			if (_engine->isDemo()) {
				idleTime = 60 * 1000;
			}
			if (loopMillis - startMillis > idleTime) {
				return kDemoMenu;
			}
		}
#ifdef __3DS__
	} while (!_engine->_input->toggleActionIfActive(TwinEActionType::UIEnter) && !touchActivate);
#else
	} while (!_engine->_input->toggleActionIfActive(TwinEActionType::UIEnter));
#endif

	return (int32)menuSettings->getActiveButtonTextId();
}

int32 Menu::advoptionsMenu() {
	_engine->restoreFrontBuffer();

	ScopedCursor scoped(_engine);
	for (;;) {
		switch (doGameMenu(&_advOptionsMenuState)) {
		case (int32)TextId::kReturnMenu: {
			return 0;
		}
		case kQuitEngine:
			return kQuitEngine;
		case (int32)TextId::kBehaviourAggressiveManual:
		case (int32)TextId::kDetailsPolygonsHigh:
		case (int32)TextId::kDetailsShadowHigh:
		case (int32)TextId::kSceneryZoomOn:
		default:
			warning("Unknown menu button handled");
			break;
		}
	}

	return 0;
}

int32 Menu::savemanageMenu() {
	_engine->restoreFrontBuffer();

	ScopedCursor scoped(_engine);
	for (;;) {
		switch (doGameMenu(&_saveManageMenuState)) {
		case (int32)TextId::kReturnMenu:
			return 0;
		case (int32)TextId::kCreateSaveGame:
			_engine->_menuOptions->saveGameMenu();
			break;
		case (int32)TextId::kDeleteSaveGame:
			_engine->_menuOptions->deleteSaveMenu();
			break;
		case kQuitEngine:
			return kQuitEngine;
		default:
			warning("Unknown menu button handled");
			break;
		}
	}

	return 0;
}

int32 Menu::volumeOptions() {
	_engine->restoreFrontBuffer();

	if (_engine->isLBA1()) {
		if (_engine->isCDROM()) {
			_engine->_music->playAllMusic(9);
		} else {
			_engine->_music->playMidiFile(9);
		}
	}

	ScopedCursor scoped(_engine);
	for (;;) {
		switch (doGameMenu(&_volumeMenuState)) {
		case (int32)TextId::kReturnMenu:
			return 0;
		case kQuitEngine:
			return kQuitEngine;
		case (int32)TextId::kMusicVolume:
		case (int32)TextId::kSoundVolume:
		case (int32)TextId::kCDVolume:
		case (int32)TextId::kSpeechVolume:
		default:
			warning("Unknown menu button handled");
			break;
		}
	}

	return 0;
}

int32 Menu::languageMenu() {
	_engine->restoreFrontBuffer();

	ScopedCursor scoped(_engine);
	for (;;) {
		switch (doGameMenu(&_languageMenuState)) {
		case (int32)TextId::kReturnMenu:
			return 0;
		case kQuitEngine:
			return kQuitEngine;
		case (int32)MenuButtonTypes::kTextLanguage:
		case (int32)MenuButtonTypes::kVoice:
		default:
			warning("Unknown menu button handled");
			break;
		}
	}

	return 0;
}

void Menu::inGameOptionsMenu() {
	_engine->_text->initDial(TextBankId::Options_and_menus);
	_optionsMenuState.setButtonTextId(0, TextId::kReturnGame);
	_engine->saveFrontBuffer();
	optionsMenu();
	_engine->_text->initSceneTextBank();
	_optionsMenuState.setButtonTextId(0, TextId::kReturnMenu);
}

int32 Menu::optionsMenu() {
	_engine->restoreFrontBuffer();

	_engine->_sound->stopSamples();
	if (_engine->isLBA1()) {
		// LBA's Theme
		if (_engine->isCDROM()) {
			_engine->_music->playCdTrack(9);
		} else {
			_engine->_music->playMidiFile(9);
		}
	}

	ScopedCursor scoped(_engine);
	for (;;) {
		switch (doGameMenu(&_optionsMenuState)) {
		case (int32)TextId::kReturnGame:
		case (int32)TextId::kReturnMenu: {
			return 0;
		}
		case (int32)TextId::kVolumeSettings: {
			checkMenuQuit(volumeOptions()) break;
		}
		case (int32)TextId::kCustomLanguageOption: {
			checkMenuQuit(languageMenu()) break;
		}
		case (int32)TextId::kSaveManage: {
			checkMenuQuit(savemanageMenu()) break;
		}
		case (int32)TextId::kAdvanced: {
			checkMenuQuit(advoptionsMenu()) break;
		}
		case kQuitEngine:
			return kQuitEngine;
		default:
			break;
		}
	}

	return 0;
}

int32 Menu::newGameClassicMenu() {
	_engine->restoreFrontBuffer();

	ScopedCursor scoped(_engine);
	for (;;) {
		switch (doGameMenu(&_newGameMenuState)) {
		case (int32)TextId::kReturnGame:
		case (int32)TextId::kReturnMenu: {
			return 0;
		}
		case (int32)TextId::kNewGamePlus:
			_engine->_gameState->_endGameItems = true;
			if (_engine->_menuOptions->newGameMenu()) {
				return 1;
			}
			break;
		case (int32)TextId::kNewGame: {
			_engine->_gameState->_endGameItems = false;
			if (_engine->_menuOptions->newGameMenu()) {
				return 1;
			}
			break;
		}
		case kQuitEngine:
			return kQuitEngine;
		default:
			break;
		}
	}

	return 0;
}

static const byte cursorArrow[] = {
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	1, 0, 1, 3, 3, 3, 3, 3, 3, 3, 3,
	1, 0, 0, 1, 3, 3, 3, 3, 3, 3, 3,
	1, 0, 0, 0, 1, 3, 3, 3, 3, 3, 3,
	1, 0, 0, 0, 0, 1, 3, 3, 3, 3, 3,
	1, 0, 0, 0, 0, 0, 1, 3, 3, 3, 3,
	1, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3,
	1, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3,
	1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 0, 0, 1, 0, 0, 1, 3, 3, 3, 3,
	1, 0, 1, 3, 1, 0, 0, 1, 3, 3, 3,
	1, 1, 3, 3, 1, 0, 0, 1, 3, 3, 3,
	1, 3, 3, 3, 3, 1, 0, 0, 1, 3, 3,
	3, 3, 3, 3, 3, 1, 0, 0, 1, 3, 3,
	3, 3, 3, 3, 3, 3, 1, 1, 1, 3, 3};

static const byte cursorPalette[] = {
	0, 0, 0,
	0xff, 0xff, 0xff};

bool Menu::init() {
	_engine->_input->enableKeyMap(uiKeyMapId);
#ifndef __3DS__
	_engine->_screens->loadMenuImage();
#endif
	// On the standalone 3DS frontend the classic ship artwork belongs only on
	// the lower LCD. Loading it here briefly exposes it on the upper LCD while
	// startup resources are still being initialized; it is cached lazily by
	// ensureLowerScreenMenuBase() when PRESS START is actually prepared.
	// load menu effect file only once
	_plasmaEffectPtr = (uint8 *)malloc(kPlasmaEffectFilesize);
	memset(_plasmaEffectPtr, 0, kPlasmaEffectFilesize);

	CursorMan.pushCursor(cursorArrow, 11, 16, 1, 1, 3);
	CursorMan.pushCursorPalette(cursorPalette, 0, 2);
	return HQR::getEntry(_plasmaEffectPtr, Resources::HQR_RESS_FILE, RESSHQR_PLASMAEFFECT) > 0;
}

EngineState Menu::run() {
	FrameMarker frame(_engine);
	_engine->_text->initDial(TextBankId::Options_and_menus);

	_engine->_sound->stopSamples();
	if (_engine->isLBA1()) {
		if (_engine->isCDROM()) {
			_engine->_music->playCdTrack(9); // LBA's Theme
		} else {
			_engine->_music->playMidiFile(9); // LBA's Theme
		}
	} else {
		_engine->_music->playMusic(6); // LBA2's Theme
	}

#ifdef __3DS__
	if (_engine->isLBA1()) {
		return run3DSFrontEnd();
	}
#endif

	ScopedCursor scoped(_engine);
	switch (doGameMenu(&_mainMenuState)) {
	case (int32)TextId::toNewGame:
	case (int32)TextId::kNewGame: {
		if (_engine->isLba1Classic()) {
			if (newGameClassicMenu()) {
				return EngineState::GameLoop;
			}
			break;
		}
		if (_engine->_menuOptions->newGameMenu()) {
			return EngineState::GameLoop;
		}
		break;
	}
	case (int32)TextId::toContinueGame:
	case (int32)TextId::kContinueGame: {
		if (_engine->_menuOptions->continueGameMenu()) {
			return EngineState::LoadedGame;
		}
		break;
	}
	case (int32)TextId::toOptions:
	case (int32)TextId::kOptions: {
		optionsMenu();
		break;
	}
	case kDemoMenu: {
		menuDemo();
		break;
	}
	case (int32)TextId::kQuit:
	case (int32)TextId::toQuit:
	case kQuitEngine:
		debug("quit the game");
		return EngineState::QuitGame;
	}
	return EngineState::Menu;
}

int32 Menu::quitMenu() {
	_engine->saveFrontBuffer();
	_engine->_sound->pauseSamples();

	MenuSettings *localMenu;
	if (_engine->_cfgfile.UseAutoSaving) {
		localMenu = &_giveUpMenuState;
	} else {
		localMenu = &_giveUpMenuWithSaveState;
	}

	ScopedCursor scoped(_engine);

	int32 menuId;
	do {
		FrameMarker frame(_engine);
		_engine->_text->initDial(TextBankId::Options_and_menus);
#ifdef __3DS__
		// Every fresh opening starts on Continue; do not remember Quit/Options
		// from the previous visit to the pause menu.
		localMenu->setActiveButton(0);
#endif
		menuId = doGameMenu(localMenu);
		switch (menuId) {
		case (int32)TextId::kContinue:
			_engine->_sound->resumeSamples();
			break;
		case (int32)TextId::kGiveUp:
#ifdef __3DS__
			// Save the live profile before giveUp() resets inventory/game-state vars.
			// Saving after giveUp() writes an empty inventory back into the selected slot.
			_engine->autoSave();
#endif
			_engine->_gameState->giveUp();
			return 1;
		case (int32)TextId::kCreateSaveGame:
			_engine->_menuOptions->saveGameMenu();
			break;
		case (int32)TextId::kOptions:
			// Let the engine open the full in-game Options tree (volume, language,
			// save manager, advanced settings) after this pause menu closes.
			_engine->_sound->resumeSamples();
			return 2;
		case kQuitEngine:
			return kQuitEngine;
		default:
			warning("Unknown menu button handled: %i", menuId);
		}
		_engine->_text->initSceneTextBank();
	} while (menuId != (int32)TextId::kGiveUp && menuId != (int32)TextId::kContinue && menuId != (int32)TextId::kCreateSaveGame);

	return 0;
}

void Menu::drawHealthBar(int32 left, int32 right, int32 top, int32 barLeftPadding, int32 barHeight) {
	_engine->_grid->drawSprite(left, top + 3, _engine->_resources->_spriteData[SPRITEHQR_LIFEPOINTS]);
	const int32 barLeft = left + barLeftPadding;
	const int32 healthBarRight = ruleThree32(barLeft, right, 50, _engine->_scene->_sceneHero->_lifePoint);
	const int32 barBottom = top + barHeight;
	_engine->_interface->box(Common::Rect(barLeft, top, healthBarRight, barBottom), COLOR_91);
	_engine->_interface->box(Common::Rect(healthBarRight, top, left + 325, barBottom), COLOR_BLACK);
	drawRectBorders(Common::Rect(barLeft, top, right, barBottom));
}

void Menu::drawCloverLeafs(int32 newBoxLeft, int32 boxRight, int32 top) {
	// Clover leaf boxes
	for (int32 i = 0; i < _engine->_gameState->_inventoryNumLeafsBox; i++) {
		const int32 leftSpritePos = ruleThree32(newBoxLeft, boxRight, 10, i);
		_engine->_grid->drawSprite(leftSpritePos, top + 58, _engine->_resources->_spriteData[SPRITEHQR_CLOVERLEAFBOX]);
	}

	// Clover leafs
	for (int32 i = 0; i < _engine->_gameState->_inventoryNumLeafs; i++) {
		const int32 leftSpritePos = ruleThree32(newBoxLeft, boxRight, 10, i);
		_engine->_grid->drawSprite(leftSpritePos + 2, top + 60, _engine->_resources->_spriteData[SPRITEHQR_CLOVERLEAF]);
	}
}

void Menu::drawMagicPointsBar(int32 left, int32 right, int32 top, int32 barLeftPadding, int32 barHeight) {
	if (_engine->_gameState->inventoryDisabled()) {
		return;
	}
	if (!_engine->_gameState->hasItem(InventoryItems::kiTunic)) {
		return;
	}
	_engine->_grid->drawSprite(left, top + 1, _engine->_resources->_spriteData[SPRITEHQR_MAGICPOINTS]);
	if (_engine->_gameState->_magicLevelIdx <= 0) {
		return;
	}
	const int32 barLeft = left + barLeftPadding;
	const int32 barBottom = top + barHeight;
	// max magic level is 4
	const int32 maxMagicPoints = 4 * 20;
	const int32 barRight = ruleThree32(barLeft, right, maxMagicPoints, _engine->_gameState->_magicPoint);
	const Common::Rect pointsRect(barLeft, top, barRight, barBottom);
	_engine->_interface->box(pointsRect, COLOR_75);

	for (int32 l = 0; l < _engine->_gameState->_magicLevelIdx; l++) {
		const int32 x1 = ruleThree32(barLeft, right, 40, _engine->_gameState->_magicLevelIdx * 10);
		_engine->_interface->drawLine(x1, top + barHeight, x1, top + 35 + 15 - 1, 0);
	}

	const int32 rectRight = ruleThree32(barLeft, right, 40, _engine->_gameState->_magicLevelIdx * 10);
	drawRectBorders(barLeft, top, rectRight, barBottom);
}

void Menu::drawSpriteAndString(int32 left, int32 top, const SpriteData &spriteData, const Common::String &str, int32 color) {
	_engine->_grid->drawSprite(left, top + 15, spriteData);
	_engine->_text->setFontColor(color);
	_engine->_text->drawText(left + 30, top + 5, str.c_str());
}

void Menu::drawCoins(int32 left, int32 top) {
	const Common::String &inventoryNumKashes = Common::String::format("%d", _engine->_gameState->_goldPieces);
	drawSpriteAndString(left, top, _engine->_resources->_spriteData[SPRITEHQR_KASHES], inventoryNumKashes);
}

void Menu::drawKeys(int32 left, int32 top) {
	const Common::String &inventoryNumKeys = Common::String::format("%d", _engine->_gameState->_nbLittleKeys);
	drawSpriteAndString(left, top, _engine->_resources->_spriteData[SPRITEHQR_KEY], inventoryNumKeys);
}

void Menu::drawInfoMenu(int16 left, int16 top, int16 width, bool copyToScreen) {
	_engine->_interface->unsetClip();
	const int16 height = 80;
	const Common::Rect rect(left, top, left + width, top + height);
	if (copyToScreen) {
		drawRectBorders(rect);
	}
	Common::Rect filledRect(rect);
	filledRect.grow(-1);
	_engine->_interface->box(filledRect, COLOR_BLACK);

	const int32 boxLeft = left + 9;
	const int32 boxRight = left + 325;
	const int32 barPadding = 25;
	const int32 boxTop = top + 10;
	const int32 barHeight = 14;
	drawHealthBar(boxLeft, boxRight, boxTop, barPadding, barHeight);
	drawMagicPointsBar(boxLeft, boxRight, boxTop + 25, barPadding, barHeight);

	const int32 posLeft = left + 340;
	drawCoins(posLeft, top);
	drawKeys(posLeft, top + 35);
	drawCloverLeafs(left + barPadding, boxRight, top);

	if (copyToScreen) {
		_engine->copyBlockPhys(left, top, left + width, top + 135);
	}
}

Common::Rect Menu::calcBehaviourRect(int32 left, int32 top, HeroBehaviourType behaviour) const {
	const int border = 10;
	const int32 padding = 11;
	const int32 width = 99;
	const int height = 119;

	const int32 boxLeft = (int32)behaviour * (width + padding) + left + border;
	const int32 boxRight = boxLeft + width;
	const int32 boxTop = top + border;
	const int32 boxBottom = boxTop + height;
	return Common::Rect(boxLeft, boxTop, boxRight, boxBottom);
}

bool Menu::isBehaviourHovered(int32 left, int32 top, HeroBehaviourType behaviour) const {
	if (!_engine->_cfgfile.Mouse) {
		return false;
	}
	const Common::Rect &boxRect = calcBehaviourRect(left, top, behaviour);
	return _engine->_input->isMouseHovering(boxRect);
}

void Menu::drawBehaviour(int32 left, int32 top, HeroBehaviourType behaviour, int32 angle, bool cantDrawBox) {
	const Common::Rect &boxRect = calcBehaviourRect(left, top, behaviour);

	const int animIdx = _engine->_actor->_heroAnimIdx[(byte)behaviour];
	const AnimData &currentAnimData = _engine->_resources->_animData[animIdx];

	uint currentAnimState = _behaviourAnimState[(byte)behaviour];

	if (_engine->_animations->setInterAnimObjet(currentAnimState, currentAnimData, *_behaviourEntity, &_behaviourAnimData[(byte)behaviour])) {
		currentAnimState++; // keyframe
		if (currentAnimState >= currentAnimData.getNbFramesAnim()) {
			currentAnimState = currentAnimData.getLoopFrame();
		}
		_behaviourAnimState[(byte)behaviour] = currentAnimState;
	}

	if (!cantDrawBox) {
		Common::Rect boxRectCopy(boxRect);
		boxRectCopy.grow(1);
		drawRectBorders(boxRectCopy);
	}

	_engine->_interface->memoClip();
	_engine->_interface->unsetClip();

	if (behaviour == _engine->_actor->_heroBehaviour) {
		const int titleOffset = 10;
		const int titleHeight = 40;
		const int32 titleBoxLeft = left + 10;
		const int32 titleBoxWidth = 430;
		const int32 titleBoxCenter = titleBoxLeft + titleBoxWidth / 2;
		const int32 titleBoxRight = titleBoxLeft + titleBoxWidth;
		const int32 titleBoxTop = boxRect.bottom + titleOffset;
		const int32 titleBoxBottom = titleBoxTop + titleHeight;

		_engine->_interface->box(boxRect, COLOR_BRIGHT_BLUE2);

		// behaviour menu title
		const Common::Rect titleRect(titleBoxLeft, titleBoxTop, titleBoxRight, titleBoxBottom);
		_engine->_interface->box(titleRect, COLOR_BLACK);
		drawRectBorders(titleRect);

		_engine->_text->setFontColor(COLOR_WHITE);

		char dialText[256];
		_engine->_text->getMenuText(_engine->_actor->getTextIdForBehaviour(), dialText, sizeof(dialText));

		_engine->_text->drawText(titleBoxCenter - _engine->_text->sizeFont(dialText) / 2, titleBoxTop + 1, dialText);
	} else {
		_engine->_interface->box(boxRect, COLOR_BLACK);
	}

	_engine->_renderer->drawObj3D(boxRect, -600, angle, *_behaviourEntity, _moveMenu);

	_engine->_interface->restoreClip();
}

void Menu::prepareAndDrawBehaviour(int32 left, int32 top, int32 angle, HeroBehaviourType behaviour) {
	const int animIdx = _engine->_actor->_heroAnimIdx[(byte)behaviour];
	_engine->_animations->setAnimObjet(_behaviourAnimState[(byte)behaviour], _engine->_resources->_animData[animIdx], *_behaviourEntity, &_behaviourAnimData[(byte)behaviour]);
	drawBehaviour(left, top, behaviour, angle, false);
}

void Menu::drawBehaviourMenu(int32 left, int32 top, int32 angle, bool copyToScreen) {
	const int32 width = 450;
	const int32 height = 190;
	const int32 right = left + width;
	const int32 bottom = top + height;

	const Common::Rect titleRect(left, top, right, bottom);
	if (copyToScreen) {
		drawRectBorders(titleRect);
	}

	Common::Rect boxRect(titleRect);
	boxRect.grow(-1);
	_engine->_interface->shadeBox(boxRect, 2);

	prepareAndDrawBehaviour(left, top, angle, HeroBehaviourType::kNormal);
	prepareAndDrawBehaviour(left, top, angle, HeroBehaviourType::kAthletic);
	prepareAndDrawBehaviour(left, top, angle, HeroBehaviourType::kAggressive);
	prepareAndDrawBehaviour(left, top, angle, HeroBehaviourType::kDiscrete);

	if (copyToScreen) {
		_engine->copyBlockPhys(titleRect);
	}

	drawInfoMenu(titleRect.left, titleRect.bottom + 10, titleRect.width(), copyToScreen);
}

void Menu::processBehaviourMenu(bool behaviourMenu) {
	if (_engine->_actor->_heroBehaviour == HeroBehaviourType::kProtoPack) {
		_engine->_sound->stopSamples();
		_engine->_actor->setBehaviour(HeroBehaviourType::kNormal);
	}

	_behaviourEntity = &_engine->_scene->_sceneHero->_entityDataPtr->getBody(_engine->_scene->_sceneHero->_body);

	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kNormal] = _engine->_actor->_heroAnimIdxNORMAL;
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kAthletic] = _engine->_actor->_heroAnimIdxATHLETIC;
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kAggressive] = _engine->_actor->_heroAnimIdxAGGRESSIVE;
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kDiscrete] = _engine->_actor->_heroAnimIdxDISCRETE;

	_engine->_movements->initRealAngle(_engine->_scene->_sceneHero->_beta, _engine->_scene->_sceneHero->_beta - LBAAngles::ANGLE_90, LBAAngles::ANGLE_17, &_moveMenu);

	_engine->saveFrontBuffer();

	TextBankId tmpTextBank = _engine->_scene->_sceneTextBank;
	_engine->_scene->_sceneTextBank = TextBankId::None;

	_engine->_text->initDial(TextBankId::Options_and_menus);

	// quick actions to change behaviour don't show the menu in classic edition
	if (!behaviourMenu && _engine->isLba1Classic()) {
		char text[256];
		_engine->_text->getMenuText(_engine->_actor->getTextIdForBehaviour(), text, sizeof(text));
		_engine->_redraw->setRenderText(text);
	} else {
		const int32 left = _engine->width() / 2 - 220;
		const int32 top = _engine->height() / 2 - 140;
		drawBehaviourMenu(left, top, _engine->_scene->_sceneHero->_beta);

		HeroBehaviourType tmpHeroBehaviour = _engine->_actor->_heroBehaviour;

		const int animIdx = _engine->_actor->_heroAnimIdx[(byte)_engine->_actor->_heroBehaviour];
		_engine->_animations->setAnimObjet(_behaviourAnimState[(byte)_engine->_actor->_heroBehaviour], _engine->_resources->_animData[animIdx], *_behaviourEntity, &_behaviourAnimData[(byte)_engine->_actor->_heroBehaviour]);

		int32 tmpTime = _engine->timerRef;

		ScopedCursor scopedCursor(_engine);
#ifdef __3DS__
		// The 3DS uses the physical controls and the native lower-screen HUD here.
		// ScopedCursor is useful on desktop, but on 3DS it paints a mouse pointer
		// over the behaviour cards as soon as Y is held.
		CursorMan.showMouse(false);
#endif
		ScopedKeyMap scopedKeyMap(_engine, uiKeyMapId);
		while (_engine->_input->isActionActive(TwinEActionType::BehaviourMenu) || _engine->_input->isQuickBehaviourActionActive()) {
			FrameMarker frame(_engine, 50);
			_engine->readKeys();
			if (_engine->shouldQuit()) {
				break;
			}

			if (isBehaviourHovered(left, top, HeroBehaviourType::kNormal)) {
				_engine->_actor->_heroBehaviour = HeroBehaviourType::kNormal;
			} else if (isBehaviourHovered(left, top, HeroBehaviourType::kAthletic)) {
				_engine->_actor->_heroBehaviour = HeroBehaviourType::kAthletic;
			} else if (isBehaviourHovered(left, top, HeroBehaviourType::kAggressive)) {
				_engine->_actor->_heroBehaviour = HeroBehaviourType::kAggressive;
			} else if (isBehaviourHovered(left, top, HeroBehaviourType::kDiscrete)) {
				_engine->_actor->_heroBehaviour = HeroBehaviourType::kDiscrete;
			}

			int heroBehaviour = (int)_engine->_actor->_heroBehaviour;
			if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
				heroBehaviour--;
			} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight)) {
				heroBehaviour++;
			}

			if (heroBehaviour < (int)HeroBehaviourType::kNormal) {
				heroBehaviour = (int)HeroBehaviourType::kDiscrete;
			} else if (heroBehaviour >= (int)HeroBehaviourType::kProtoPack) {
				heroBehaviour = (int)HeroBehaviourType::kNormal;
			}

			_engine->_actor->_heroBehaviour = (HeroBehaviourType)heroBehaviour;

			if (tmpHeroBehaviour != _engine->_actor->_heroBehaviour) {
				drawBehaviour(left, top, tmpHeroBehaviour, _engine->_scene->_sceneHero->_beta, true);
				tmpHeroBehaviour = _engine->_actor->_heroBehaviour;
				_engine->_movements->initRealAngle(_engine->_scene->_sceneHero->_beta, _engine->_scene->_sceneHero->_beta - LBAAngles::ANGLE_90, LBAAngles::ANGLE_17, &_moveMenu);
				const int tmpAnimIdx = _engine->_actor->_heroAnimIdx[(byte)_engine->_actor->_heroBehaviour];
				_engine->_animations->setAnimObjet(_behaviourAnimState[(byte)_engine->_actor->_heroBehaviour], _engine->_resources->_animData[tmpAnimIdx], *_behaviourEntity, &_behaviourAnimData[(byte)_engine->_actor->_heroBehaviour]);
			}

			drawBehaviour(left, top, _engine->_actor->_heroBehaviour, -1, true);

			// increase the timer to play the animations
			_engine->timerRef++;
			debugC(3, kDebugLevels::kDebugTimers, "Behaviour time: %i", _engine->timerRef);
		}

		_engine->timerRef = tmpTime;

		_engine->_gameState->init3DGame();
	}
	_engine->_actor->setBehaviour(_engine->_actor->_heroBehaviour);

	_engine->_scene->_sceneTextBank = tmpTextBank;
	_engine->_text->initSceneTextBank();
}

#ifdef __3DS__
void Menu::beginLowerScreenScratch() {
	const Common::Rect scratchRect(0, 0, 320, 240);
	if (_lowerScreenBackup.w != 320 || _lowerScreenBackup.h != 240) {
		_lowerScreenBackup.free();
		_lowerScreenBackup.create(320, 240, _engine->_frontVideoBuffer.format);
	}
	const Graphics::PixelFormat overlayFormat = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, overlayFormat);
	}
	_lowerScreenBackup.blitFrom(_engine->_frontVideoBuffer, scratchRect, _lowerScreenBackup.getBounds());
	_engine->_frontVideoBuffer.fillRect(scratchRect, COLOR_BLACK);
}

void Menu::presentLowerScreenScratch() {
	const Common::Rect scratchRect(0, 0, 320, 240);
	const Graphics::PixelFormat overlayFormat = g_system->getOverlayFormat();
	byte palette[256 * 3];
	if (_upperPauseOverlayActive) {
		memcpy(palette, _upperPausePalette, sizeof(palette));
	} else {
		_engine->_frontVideoBuffer.getPalette(palette, 0, 256);
	}
	for (int y = 0; y < 240; ++y) {
		const byte *src = (const byte *)_engine->_frontVideoBuffer.getBasePtr(0, y);
		uint32 *dst = (uint32 *)_lowerScreenSurface.getBasePtr(0, y);
		for (int x = 0; x < 320; ++x) {
			const byte *rgb = &palette[(uint32)src[x] * 3];
			dst[x] = overlayFormat.ARGBToColor(255, rgb[0], rgb[1], rgb[2]);
		}
	}

	_engine->_frontVideoBuffer.blitFrom(_lowerScreenBackup, _lowerScreenBackup.getBounds(), scratchRect);
	_engine->_frontVideoBuffer.addDirtyRect(scratchRect);
	_engine->_gameState->init3DGame();

	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::drawLowerScreenFrame(const Common::Rect &rect, uint32 color) {
	_lowerScreenSurface.fillRect(Common::Rect(rect.left, rect.top, rect.right, rect.top + 1), color);
	_lowerScreenSurface.fillRect(Common::Rect(rect.left, rect.bottom - 1, rect.right, rect.bottom), color);
	_lowerScreenSurface.fillRect(Common::Rect(rect.left, rect.top, rect.left + 1, rect.bottom), color);
	_lowerScreenSurface.fillRect(Common::Rect(rect.right - 1, rect.top, rect.right, rect.bottom), color);
}

static void drawLowerMagicBallIcon(Graphics::ManagedSurface &surface, const Graphics::PixelFormat &fmt, int magicLevel) {
	uint32 dark;
	uint32 mid;
	uint32 light;
	if (magicLevel >= 4) {
		dark = fmt.RGBToColor(142, 31, 18);
		mid = fmt.RGBToColor(244, 82, 24);
		light = fmt.RGBToColor(255, 215, 63);
	} else if (magicLevel == 3) {
		dark = fmt.RGBToColor(116, 24, 35);
		mid = fmt.RGBToColor(214, 55, 68);
		light = fmt.RGBToColor(255, 155, 145);
	} else if (magicLevel == 2) {
		dark = fmt.RGBToColor(28, 105, 54);
		mid = fmt.RGBToColor(67, 190, 91);
		light = fmt.RGBToColor(171, 247, 164);
	} else {
		dark = fmt.RGBToColor(146, 102, 15);
		mid = fmt.RGBToColor(239, 190, 45);
		light = fmt.RGBToColor(255, 239, 135);
	}

	// Small native-resolution orb. The inventory model is always yellow, while
	// the real projectile changes with the magic level, so draw this one directly
	// in RGB to keep the lower-screen HUD in sync with gameplay.
	surface.fillRect(Common::Rect(25, 175, 39, 193), dark);
	surface.fillRect(Common::Rect(22, 179, 42, 189), dark);
	surface.fillRect(Common::Rect(25, 177, 39, 191), mid);
	surface.fillRect(Common::Rect(24, 180, 40, 188), mid);
	surface.fillRect(Common::Rect(27, 178, 34, 184), light);
	surface.fillRect(Common::Rect(28, 179, 32, 182), fmt.RGBToColor(255, 255, 220));

	if (magicLevel >= 4) {
		// A few asymmetric flame tongues make the final level visibly different
		// from the plain red ball without adding another texture/resource.
		surface.fillRect(Common::Rect(28, 171, 32, 177), mid);
		surface.fillRect(Common::Rect(34, 173, 38, 179), light);
		surface.fillRect(Common::Rect(21, 176, 26, 181), mid);
	}
}

void Menu::drawLowerLbaText(const char *text, int x, int y, int width, uint32 color, bool centered) {
	if (!text || !*text || width <= 0 || x >= 320 || y >= 240) {
		return;
	}

	const int srcW = _engine->_frontVideoBuffer.w;
	const int srcH = _engine->_frontVideoBuffer.h;
	if (srcW <= 0 || srcH <= 0) {
		return;
	}

	if (_lowerScreenTextBackup.w != srcW ||
	    _lowerScreenTextBackup.h != srcH ||
	    _lowerScreenTextBackup.format != _engine->_frontVideoBuffer.format) {
		_lowerScreenTextBackup.free();
		_lowerScreenTextBackup.create(srcW, srcH, _engine->_frontVideoBuffer.format);
	}
	_lowerScreenTextBackup.blitFrom(_engine->_frontVideoBuffer);

	// Render the original LBA bitmap font into a clean native-size mask, then
	// downsample that mask onto the 320x240 lower-screen overlay. This keeps all
	// custom 3DS captions visually consistent with the game without replacing
	// the already-composited RGB artwork underneath.
	_engine->_frontVideoBuffer.fillRect(_engine->_frontVideoBuffer.getBounds(), COLOR_BLACK);
	_engine->_text->setFont(INTER_LEAVE, INTER_SPACE);
	_engine->_text->setFontColor(COLOR_WHITE);

	const int nativeX = x * srcW / 320;
	const int nativeY = y * srcH / 240;
	const int nativeWidth = MAX(1, width * srcW / 320);
	int drawX = nativeX;
	if (centered) {
		const int textWidth = _engine->_text->sizeFont(text);
		drawX += MAX(0, (nativeWidth - textWidth) / 2);
	}
	_engine->_text->drawText(drawX, nativeY, text);

	const int left = CLIP<int>(x, 0, 319);
	const int right = CLIP<int>(x + width, left + 1, 320);
	const int top = CLIP<int>(y, 0, 239);
	const int bottom = CLIP<int>(y + 24, top + 1, 240);

	for (int dy = top; dy < bottom; ++dy) {
		uint32 *dst = (uint32 *)_lowerScreenSurface.getBasePtr(0, dy);
		const int sy0 = dy * srcH / 240;
		const int sy1 = MAX(sy0 + 1, (dy + 1) * srcH / 240);
		for (int dx = left; dx < right; ++dx) {
			const int sx0 = dx * srcW / 320;
			const int sx1 = MAX(sx0 + 1, (dx + 1) * srcW / 320);
			int inkPixels = 0;
			int samplePixels = 0;
			for (int sy = sy0; sy < MIN(sy1, srcH); ++sy) {
				const byte *src = (const byte *)_engine->_frontVideoBuffer.getBasePtr(0, sy);
				for (int sx = sx0; sx < MIN(sx1, srcW); ++sx) {
					++samplePixels;
					if (src[sx] != COLOR_BLACK) {
						++inkPixels;
					}
				}
			}
			// The previous "any source pixel wins" downsample made every 1px
			// edge expand into the 320x240 result. Requiring at least half of
			// the source footprint keeps the original LBA font visibly thinner.
			if (samplePixels > 0 && inkPixels * 2 >= samplePixels) {
				dst[dx] = color;
			}
		}
	}

	_engine->_frontVideoBuffer.blitFrom(_lowerScreenTextBackup);
}

int Menu::drawLowerLbaWrappedText(const char *text, int x, int y, int width, int maxLines, uint32 color, int firstLine) {
	if (!text || !*text || maxLines <= 0 || width <= 0) {
		return 0;
	}

	_engine->_text->setFont(INTER_LEAVE, INTER_SPACE);
	const int nativeWidth = MAX(1, width * _engine->_frontVideoBuffer.w / 320);
	const char *p = text;
	Common::String line;
	int logicalLine = 0;
	int drawnLines = 0;
	const int lineHeight = 17;

	auto emitLine = [&](const Common::String &value) {
		if (logicalLine >= firstLine && drawnLines < maxLines) {
			drawLowerLbaText(value.c_str(), x, y + drawnLines * lineHeight, width, color, false);
			++drawnLines;
		}
		++logicalLine;
	};

	while (*p) {
		while (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t') {
			if (*p == '\n' && !line.empty()) {
				emitLine(line);
				line.clear();
			}
			++p;
		}
		if (!*p) {
			break;
		}

		const char *wordStart = p;
		while (*p && *p != ' ' && *p != '\r' && *p != '\n' && *p != '\t') {
			++p;
		}
		const Common::String word(wordStart, p - wordStart);
		const Common::String candidate = line.empty() ? word : line + " " + word;
		if (!line.empty() && _engine->_text->sizeFont(candidate.c_str()) > nativeWidth) {
			emitLine(line);
			line = word;
		} else {
			line = candidate;
		}
	}

	if (!line.empty()) {
		emitLine(line);
	}
	return logicalLine;
}

void Menu::drawLowerScreenHud() {
	ActorStruct *hero = _engine->_scene->_sceneHero;
	if (!hero || hero->_body == -1 || !hero->_entityDataPtr) {
		showLowerScreenIdlePanel();
		return;
	}

	beginLowerScreenScratch();
	_behaviourEntity = &hero->_entityDataPtr->getBody(hero->_body);
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kNormal] = _engine->_actor->_heroAnimIdxNORMAL;
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kAthletic] = _engine->_actor->_heroAnimIdxATHLETIC;
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kAggressive] = _engine->_actor->_heroAnimIdxAGGRESSIVE;
	_engine->_actor->_heroAnimIdx[(byte)HeroBehaviourType::kDiscrete] = _engine->_actor->_heroAnimIdxDISCRETE;

	// Render the four behaviour portraits at native lower-screen resolution.
	// Use the perspective renderer with a fixed 45-degree angle and a larger
	// camera distance: the original menu renderer is sized for 640x480 and
	// clips Twinsen badly inside the much smaller 3DS cards.
	for (int i = 0; i < 4; ++i) {
		const HeroBehaviourType behaviour = (HeroBehaviourType)i;
		const int x = 3 + i * 79;
		const Common::Rect portraitRect(x + 4, 57, x + 73, 128);
		const int animIdx = _engine->_actor->_heroAnimIdx[(byte)behaviour];
		_engine->_animations->setAnimObjet(0, _engine->_resources->_animData[animIdx], *_behaviourEntity, &_behaviourAnimData[i]);
		_engine->_interface->setClip(portraitRect);
		// v11 was intentionally conservative and ended up too small on real 3DS
		// hardware. A lower camera-distance value makes the model roughly 1.5x
		// larger while the card clip still guarantees it cannot bleed outside.
		_engine->_renderer->draw3dObject(x + 38, 95, *_behaviourEntity, LBAAngles::ANGLE_45, 8200);
		_engine->_interface->unsetClip();
	}

	// Five language-independent quick-action icons from the real inventory models.
	// Each model has very different proportions, so use per-item zoom values
	// instead of one oversized scale that clips sabre/protopack on 3DS.
	static const InventoryItems quickItems[5] = {kiMagicBall, kiUseSabre, kiHolomap, kiPenguin, kiProtoPack};
	static const int quickZoom[5] = {21000, 27000, 23000, 23500, 25500};
	for (int i = 0; i < 5; ++i) {
		const InventoryItems item = quickItems[i];
		if (!_engine->_gameState->hasItem(item) || _engine->_gameState->inventoryDisabled()) {
			continue;
		}
		// The stock inventory Magic Ball model is always yellow. Its native 3DS
		// HUD icon is drawn later in RGB using the current magic level.
		if (item == kiMagicBall) {
			continue;
		}
		const int x = i * 64;
		const Common::Rect iconRect(x + 12, 160, x + 52, 207);
		_engine->_interface->setClip(iconRect);
		_engine->_renderer->draw3dObject(x + 32, 183, _engine->_resources->_inventoryTable[item], LBAAngles::ANGLE_45, quickZoom[i]);
		_engine->_interface->unsetClip();
	}

	presentLowerScreenScratch();

	// Draw the HUD chrome directly in 320x240 RGB space so it stays crisp.
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	const uint32 bg = fmt.RGBToColor(13, 15, 27);
	const uint32 panel = fmt.RGBToColor(27, 30, 48);
	const uint32 border = fmt.RGBToColor(104, 110, 143);
	const uint32 text = fmt.RGBToColor(230, 232, 241);
	const uint32 muted = fmt.RGBToColor(115, 120, 144);
	const uint32 selected = fmt.RGBToColor(67, 158, 220);
	const uint32 gold = fmt.RGBToColor(230, 184, 60);
	const uint32 goldDark = fmt.RGBToColor(153, 105, 25);
	const uint32 hpColor = fmt.RGBToColor(218, 63, 91);
	const uint32 mpColor = fmt.RGBToColor(62, 153, 218);
	const uint32 leafGreen = fmt.RGBToColor(70, 176, 91);

	// Replace pure-black scratch background with the native HUD background.
	for (int y = 0; y < 240; ++y) {
		uint32 *row = (uint32 *)_lowerScreenSurface.getBasePtr(0, y);
		for (int x = 0; x < 320; ++x) {
			if (row[x] == fmt.RGBToColor(0, 0, 0)) {
				row[x] = bg;
			}
		}
	}


	// Status area. The bar colours make HP/MP labels redundant on a handheld HUD.
	_lowerScreenSurface.fillRect(Common::Rect(3, 3, 317, 52), panel);
	drawLowerScreenFrame(Common::Rect(3, 3, 317, 52), border);
	const int barLeft = 8;
	const int barRight = 193;
	const int barWidth = barRight - barLeft;
	const int hpValue = CLIP<int>(_engine->_scene->_sceneHero->_lifePoint, 0, _engine->getMaxLife());
	const int hpFill = barLeft + barWidth * hpValue / MAX(1, _engine->getMaxLife());
	_lowerScreenSurface.fillRect(Common::Rect(barLeft, 9, barRight, 20), fmt.RGBToColor(44, 47, 64));
	_lowerScreenSurface.fillRect(Common::Rect(barLeft, 9, hpFill, 20), hpColor);
	drawLowerScreenFrame(Common::Rect(barLeft, 9, barRight, 20), border);

	// Match the original upper-screen magic bar: its *capacity* grows by one
	// quarter for each acquired magic level instead of always spanning the full
	// HUD width. The fill itself is based on the original absolute 0..80 MP
	// scale, exactly like drawMagicPointsBar() above.
	const int magicLevel = CLIP<int>(_engine->_gameState->_magicLevelIdx, 0, 4);
	if (magicLevel > 0) {
		const int mpCapacityRight = barLeft + barWidth * magicLevel / 4;
		const int mpFill = MIN(mpCapacityRight,
		                       barLeft + barWidth * CLIP<int>(_engine->_gameState->_magicPoint, 0, 80) / 80);
		_lowerScreenSurface.fillRect(Common::Rect(barLeft, 31, mpCapacityRight, 42), fmt.RGBToColor(44, 47, 64));
		_lowerScreenSurface.fillRect(Common::Rect(barLeft, 31, mpFill, 42), mpColor);
		drawLowerScreenFrame(Common::Rect(barLeft, 31, mpCapacityRight, 42), border);
	}

	// Compact fairy-tale status icons: coin, little key and (once obtained) fuel.
	_lowerScreenSurface.fillRect(Common::Rect(198, 9, 208, 19), goldDark);
	_lowerScreenSurface.fillRect(Common::Rect(200, 7, 206, 21), goldDark);
	_lowerScreenSurface.fillRect(Common::Rect(199, 9, 207, 19), gold);
	_lowerScreenSurface.fillRect(Common::Rect(201, 8, 205, 20), gold);
	_lowerScreenSurface.fillRect(Common::Rect(201, 10, 203, 16), fmt.RGBToColor(250, 221, 105));

	// Key: ring + shaft + two teeth, matching the simple pixel-art coin style.
	const uint32 keyLight = fmt.RGBToColor(235, 220, 154);
	const uint32 keyDark = fmt.RGBToColor(137, 117, 67);
	_lowerScreenSurface.fillRect(Common::Rect(246, 9, 253, 16), keyDark);
	_lowerScreenSurface.fillRect(Common::Rect(247, 10, 252, 15), keyLight);
	_lowerScreenSurface.fillRect(Common::Rect(249, 12, 251, 14), panel);
	_lowerScreenSurface.fillRect(Common::Rect(252, 12, 260, 14), keyLight);
	_lowerScreenSurface.fillRect(Common::Rect(257, 14, 259, 17), keyLight);
	_lowerScreenSurface.fillRect(Common::Rect(254, 14, 256, 16), keyLight);

	const bool showGas = _engine->_gameState->hasItem(InventoryItems::kGasItem) || _engine->_gameState->_inventoryNumGas > 0;
	if (showGas) {
		const uint32 gasColor = fmt.RGBToColor(95, 190, 166);
		const uint32 gasDark = fmt.RGBToColor(44, 103, 91);
		// Tiny jerry can / fuel bottle.
		_lowerScreenSurface.fillRect(Common::Rect(282, 9, 291, 19), gasDark);
		_lowerScreenSurface.fillRect(Common::Rect(283, 10, 290, 19), gasColor);
		_lowerScreenSurface.fillRect(Common::Rect(285, 7, 289, 10), gasColor);
		_lowerScreenSurface.fillRect(Common::Rect(286, 8, 289, 9), panel);
		_lowerScreenSurface.fillRect(Common::Rect(290, 11, 293, 13), gasColor);
	}

	drawLowerLbaText(Common::String::format("%d", _engine->_gameState->_goldPieces).c_str(), 210, 5, 34, gold, true);
	drawLowerLbaText(Common::String::format("%d", _engine->_gameState->_nbLittleKeys).c_str(), 260, 5, 20, text, true);
	if (showGas) {
		drawLowerLbaText(Common::String::format("%d", _engine->_gameState->_inventoryNumGas).c_str(), 293, 5, 26, text, true);
	}

	// Clover boxes mirror the original behaviour screen: owned leaves are filled,
	// available boxes without a leaf stay empty.
	const int leafBoxes = CLIP<int>(_engine->_gameState->_inventoryNumLeafsBox, 0, 10);
	const int leafs = CLIP<int>(_engine->_gameState->_inventoryNumLeafs, 0, leafBoxes);
	for (int i = 0; i < leafBoxes; ++i) {
		const int x = 202 + i * 11;
		const Common::Rect box(x, 31, x + 9, 42);
		_lowerScreenSurface.fillRect(Common::Rect(box.left + 1, box.top + 1, box.right - 1, box.bottom - 1),
		                             i < leafs ? leafGreen : fmt.RGBToColor(36, 39, 55));
		drawLowerScreenFrame(box, i < leafs ? gold : muted);
		if (i < leafs) {
			// Minimal four-leaf mark that remains readable at 3DS resolution.
			const uint32 leafLight = fmt.RGBToColor(112, 211, 113);
			_lowerScreenSurface.fillRect(Common::Rect(x + 3, 34, x + 5, 36), leafLight);
			_lowerScreenSurface.fillRect(Common::Rect(x + 5, 34, x + 7, 36), leafLight);
			_lowerScreenSurface.fillRect(Common::Rect(x + 3, 36, x + 5, 38), leafLight);
			_lowerScreenSurface.fillRect(Common::Rect(x + 5, 36, x + 7, 38), leafLight);
		}
	}

	// Behaviour cards.
	static const char *behaviourLabels[4] = {"NORMAL", "SPORT", "AGGRO", "SNEAK"};
	for (int i = 0; i < 4; ++i) {
		const int x = 3 + i * 79;
		const Common::Rect card(x, 56, x + 76, 143);
		drawLowerScreenFrame(card, _engine->_actor->_heroBehaviour == (HeroBehaviourType)i ? selected : border);
		drawLowerLbaText(behaviourLabels[i], x + 2, 124, 72,
		                 _engine->_actor->_heroBehaviour == (HeroBehaviourType)i ? selected : text, true);
	}

	// Quick actions.
	static const char *quickLabels[5] = {"BALL", "SABER", "MAP", "PENG", "JET"};
	for (int i = 0; i < 5; ++i) {
		const int x = i * 64 + 2;
		const Common::Rect card(x, 149, x + 60, 237);
		const InventoryItems item = quickItems[i];
		const bool available = _engine->_gameState->hasItem(item) && !_engine->_gameState->inventoryDisabled();
		bool active = false;
		if (item == kiMagicBall) active = available && !_engine->_gameState->_weapon && _engine->_actor->_heroBehaviour != HeroBehaviourType::kProtoPack;
		if (item == kiUseSabre) active = available && _engine->_gameState->_weapon;
		if (item == kiProtoPack) active = available && _engine->_actor->_heroBehaviour == HeroBehaviourType::kProtoPack;
		drawLowerScreenFrame(card, active ? gold : (available ? border : muted));
		if (!available) {
			_lowerScreenSurface.fillRect(Common::Rect(card.left + 2, card.top + 2, card.right - 2, card.bottom - 2), fmt.RGBToColor(20, 22, 34));
		}
		drawLowerLbaText(quickLabels[i], x + 1, 213, 58,
		                 active ? gold : (available ? text : muted), true);
	}

	if (_engine->_gameState->hasItem(kiMagicBall) && !_engine->_gameState->inventoryDisabled()) {
		drawLowerMagicBallIcon(_lowerScreenSurface, fmt, _engine->_gameState->_magicLevelIdx);
	}

	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::show3DSPauseOverlay() {
	if (_upperPauseOverlayActive) {
		return;
	}

	const Graphics::ManagedSurface &front = _engine->_frontVideoBuffer;
	if (_upperPauseBackup.w != front.w || _upperPauseBackup.h != front.h ||
	    _upperPauseBackup.format != front.format) {
		_upperPauseBackup.free();
		_upperPauseBackup.create(front.w, front.h, front.format);
	}
	_upperPauseBackup.blitFrom(front);
	_engine->_frontVideoBuffer.getPalette(_upperPausePalette, 0, 256);

	// Freeze the current scene on the upper LCD and darken it without touching
	// the scene pixels themselves. This keeps the pause view readable and makes
	// restoration exact even for scenes that use a non-standard palette.
	byte darkPalette[256 * 3];
	for (int i = 0; i < 256 * 3; ++i) {
		darkPalette[i] = (byte)((uint32)_upperPausePalette[i] * 52U / 100U);
	}
	// Keep the game's white text genuinely white over the dimmed scene.
	darkPalette[COLOR_WHITE * 3 + 0] = 255;
	darkPalette[COLOR_WHITE * 3 + 1] = 255;
	darkPalette[COLOR_WHITE * 3 + 2] = 255;
	darkPalette[COLOR_BLACK * 3 + 0] = 0;
	darkPalette[COLOR_BLACK * 3 + 1] = 0;
	darkPalette[COLOR_BLACK * 3 + 2] = 0;
	_engine->_frontVideoBuffer.setPalette(darkPalette, 0, 256);

	const char *pauseText = "PAUSE";
	_engine->_text->setFontCrossColor(COLOR_BLACK);
	_engine->_text->setFontColor(COLOR_WHITE);
	const int textWidth = _engine->_text->sizeFont(pauseText);
	const int x = (_engine->width() - textWidth) / 2;
	const int y = (_engine->height() - _engine->_text->lineHeight) / 2;
	_engine->_text->drawText(x, y, pauseText);

	_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
	_engine->_frontVideoBuffer.forceUpdate();
	_upperPauseOverlayActive = true;
}

void Menu::hide3DSPauseOverlay() {
	if (!_upperPauseOverlayActive) {
		return;
	}

	if (_upperPauseBackup.w == _engine->_frontVideoBuffer.w &&
	    _upperPauseBackup.h == _engine->_frontVideoBuffer.h) {
		_engine->_frontVideoBuffer.blitFrom(_upperPauseBackup);
	}
	_engine->_frontVideoBuffer.setPalette(_upperPausePalette, 0, 256);
	_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
	_engine->_frontVideoBuffer.forceUpdate();
	_upperPauseOverlayActive = false;
}

void Menu::updateLowerScreenBehaviourPanel() {
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}

	// The HUD is static most of the time. Previously it re-rendered four 3D
	// Twinsen models and five inventory models every ~150 ms even when nothing
	// changed. Keep the overlay as our cache and redraw only when visible state
	// actually changes (or a mode transition explicitly sets lastUpdate to 0).
	ActorStruct *hero = _engine->_scene->_sceneHero;
	if (!hero) {
		return;
	}
	uint32 signature = 2166136261U;
	auto mix = [&signature](uint32 value) {
		signature ^= value;
		signature *= 16777619U;
	};
	mix((uint32)hero->_lifePoint);
	mix((uint32)_engine->_gameState->_magicPoint);
	mix((uint32)_engine->_gameState->_magicLevelIdx);
	mix((uint32)_engine->_gameState->_goldPieces);
	mix((uint32)_engine->_gameState->_nbLittleKeys);
	mix((uint32)_engine->_gameState->_inventoryNumLeafs);
	mix((uint32)_engine->_gameState->_inventoryNumLeafsBox);
	mix((uint32)_engine->_gameState->_inventoryNumGas);
	mix((uint32)_engine->_gameState->hasItem(InventoryItems::kGasItem));
	mix((uint32)_engine->_actor->_heroBehaviour);
	mix((uint32)_engine->_gameState->_weapon);
	mix((uint32)_engine->_gameState->inventoryDisabled());
	static const InventoryItems quickItems[5] = {kiMagicBall, kiUseSabre, kiHolomap, kiPenguin, kiProtoPack};
	for (int i = 0; i < 5; ++i) {
		mix((uint32)_engine->_gameState->hasItem(quickItems[i]));
	}

	const bool overlayWasLost = !g_system->isOverlayVisible();
	if (!overlayWasLost && !_lowerScreenFadeInPending && _lowerScreenLastUpdate != 0 && signature == _lowerScreenHudSignature) {
		return;
	}
	// ScummVM's GUI hides the overlay when it returns to the engine. If any
	// external GUI/modal screen ever slips through, force our native lower HUD
	// back immediately instead of leaving a duplicate of the upper framebuffer.
	_lowerScreenHudSignature = signature;
	_lowerScreenLastUpdate = g_system->getMillis();
	drawLowerScreenHud();
	if (_lowerScreenFadeInPending) {
		_lowerScreenFadeInPending = false;
		fadeLowerScreenFromBlack();
	}
}

void Menu::drawLowerScreenInventory() {
	static const int gridLeft = 5;
	static const int gridTop = 4;
	static const int cellW = 44;
	static const int cellH = 38;

	beginLowerScreenScratch();
	for (int item = 0; item < NUM_INVENTORY_ITEMS; ++item) {
		if (!_engine->_gameState->hasItem((InventoryItems)item) || (_engine->_gameState->inventoryDisabled() && item != InventoryItems::kiCloverLeaf)) {
			continue;
		}
		const int col = item / 4;
		const int row = item % 4;
		const int x = gridLeft + col * cellW;
		const int y = gridTop + row * cellH;
		const Common::Rect iconRect(x + 4, y + 2, x + 40, y + 34);
		_engine->_interface->setClip(iconRect);
		const int32 angle = item == _inventorySelectedItem ? _lowerScreenInventoryAngle : LBAAngles::ANGLE_45;
		_engine->_renderer->draw3dObject(x + 22, y + 18, _engine->_resources->_inventoryTable[item], angle, 26000);
		_engine->_interface->unsetClip();
	}
	presentLowerScreenScratch();

	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	const uint32 bg = fmt.RGBToColor(13, 15, 27);
	const uint32 panel = fmt.RGBToColor(24, 27, 43);
	const uint32 border = fmt.RGBToColor(104, 110, 143);
	const uint32 selected = fmt.RGBToColor(67, 158, 220);
	const uint32 text = fmt.RGBToColor(230, 232, 241);
	const uint32 muted = fmt.RGBToColor(105, 110, 132);
	for (int y = 0; y < 240; ++y) {
		uint32 *row = (uint32 *)_lowerScreenSurface.getBasePtr(0, y);
		for (int x = 0; x < 320; ++x) {
			if (row[x] == fmt.RGBToColor(0, 0, 0)) row[x] = bg;
		}
	}
	for (int item = 0; item < NUM_INVENTORY_ITEMS; ++item) {
		const int col = item / 4;
		const int row = item % 4;
		const int x = gridLeft + col * cellW;
		const int y = gridTop + row * cellH;
		const bool available = _engine->_gameState->hasItem((InventoryItems)item) && (!_engine->_gameState->inventoryDisabled() || item == InventoryItems::kiCloverLeaf);
		drawLowerScreenFrame(Common::Rect(x, y, x + 42, y + 36), item == _inventorySelectedItem ? selected : (available ? border : muted));
	}

	const Common::Rect descriptionRect(4, 159, 316, 237);
	_lowerScreenSurface.fillRect(Common::Rect(descriptionRect.left + 1, descriptionRect.top + 1,
	                                         descriptionRect.right - 1, descriptionRect.bottom - 1), panel);
	drawLowerScreenFrame(descriptionRect, border);
	char description[768];
	description[0] = '\0';
	const bool available = _inventorySelectedItem >= 0 && _inventorySelectedItem < NUM_INVENTORY_ITEMS &&
	                       _engine->_gameState->hasItem((InventoryItems)_inventorySelectedItem) &&
	                       (!_engine->_gameState->inventoryDisabled() || _inventorySelectedItem == InventoryItems::kiCloverLeaf);
	if (available) {
		_engine->_text->getInventoryText((InventoryItems)_inventorySelectedItem, description, sizeof(description));
	}
	_lowerScreenInventoryDescriptionPageCount = 1;
	if (description[0] != '\0') {
		const int firstLine = _lowerScreenInventoryDescriptionPage * 3;
		const int totalLines = drawLowerLbaWrappedText(description, 9, 162, 302, 3, text, firstLine);
		_lowerScreenInventoryDescriptionPageCount = MAX(1, (totalLines + 2) / 3);
		if (_lowerScreenInventoryDescriptionPage >= _lowerScreenInventoryDescriptionPageCount) {
			_lowerScreenInventoryDescriptionPage = 0;
		}
	}
	if (_lowerScreenInventoryDescriptionPageCount > 1) {
		const Common::String hint = Common::String::format("TEXT %d/%d: TAP    A: USE    B: BACK",
		                                                       _lowerScreenInventoryDescriptionPage + 1,
		                                                       _lowerScreenInventoryDescriptionPageCount);
		drawLowerLbaText(hint.c_str(), 7, 217, 306, muted, true);
	} else {
		drawLowerLbaText("TOUCH AGAIN: USE     B: BACK", 7, 217, 306, muted, true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::inventory3DS() {
	if (_engine->_gameState->_inventoryNumLeafs > 0) {
		_engine->_gameState->giveItem(InventoryItems::kiCloverLeaf);
	} else {
		_engine->_gameState->removeItem(InventoryItems::kiCloverLeaf);
	}

	// The original inventory descriptions live in this bank. Keep the scene itself
	// untouched on the upper screen and render the selected description below.
	_engine->_text->initDial(TextBankId::Inventory_Intro_and_Holomap);

	if (_inventorySelectedItem < 0 || _inventorySelectedItem >= NUM_INVENTORY_ITEMS ||
	    !_engine->_gameState->hasItem((InventoryItems)_inventorySelectedItem)) {
		for (int item = 0; item < NUM_INVENTORY_ITEMS; ++item) {
			if (_engine->_gameState->hasItem((InventoryItems)item)) {
				_inventorySelectedItem = item;
				break;
			}
		}
	}

	// On 3DS the inventory owns the controls while it is open. Do not leave the
	// gameplay keymap enabled at the same time: A would otherwise be both
	// "confirm item" and Twinsen's behaviour action when the inventory closes.
	_engine->_input->enableKeyMap(uiKeyMapId);
	_engine->_input->resetActionStates();
	_engine->_input->clearLowerScreenTap();

	int lastTouchSelection = -1;
	bool usedWithA = false;
	_lowerScreenInventoryDescriptionPage = 0;
	_lowerScreenInventoryDescriptionPageCount = 1;
	_lowerScreenInventoryAngle = LBAAngles::ANGLE_45;
	_lowerScreenInventoryRotateAt = g_system->getMillis();
	drawLowerScreenInventory();

	for (;;) {
		FrameMarker frame(_engine, 60);
		_engine->readKeys();

		// Match the original inventory feel: only the focused item rotates. A
		// modest ~13 fps refresh keeps the motion smooth enough on the small LCD
		// without continuously re-rendering the inventory at 60 fps.
		const uint32 now = g_system->getMillis();
		if (now - _lowerScreenInventoryRotateAt >= 75) {
			_lowerScreenInventoryRotateAt = now;
			_lowerScreenInventoryAngle = ClampAngle(_lowerScreenInventoryAngle + LBAAngles::ANGLE_11_25);
			drawLowerScreenInventory();
		}

		if (_engine->_input->toggleAbortAction() || _engine->shouldQuit()) {
			break;
		}

		Common::Point tap;
		if (_engine->_input->consumeLowerScreenTap(tap)) {
			if (tap.x >= 5 && tap.x < 313 && tap.y >= 4 && tap.y < 156) {
				const int col = (tap.x - 5) / 44;
				const int row = (tap.y - 4) / 38;
				const int item = col * 4 + row;
				const bool available = item >= 0 && item < NUM_INVENTORY_ITEMS &&
				                       _engine->_gameState->hasItem((InventoryItems)item) &&
				                       (!_engine->_gameState->inventoryDisabled() || item == InventoryItems::kiCloverLeaf);
				if (available) {
					if (_inventorySelectedItem == item && lastTouchSelection == item) {
						_engine->_loopInventoryItem = item;
						break;
					}
					_inventorySelectedItem = item;
					_lowerScreenInventoryDescriptionPage = 0;
					lastTouchSelection = item;
					drawLowerScreenInventory();
				}
			} else if (tap.x >= 4 && tap.x < 316 && tap.y >= 159 && tap.y < 237 &&
			           _lowerScreenInventoryDescriptionPageCount > 1) {
				_lowerScreenInventoryDescriptionPage =
				    (_lowerScreenInventoryDescriptionPage + 1) % _lowerScreenInventoryDescriptionPageCount;
				lastTouchSelection = -1;
				drawLowerScreenInventory();
			}
		}

		const int previous = _inventorySelectedItem;
		if (_engine->_input->toggleActionIfActive(TwinEActionType::UIDown)) {
			_inventorySelectedItem = (_inventorySelectedItem + 1) % NUM_INVENTORY_ITEMS;
			lastTouchSelection = -1;
		} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIUp)) {
			_inventorySelectedItem = (_inventorySelectedItem + NUM_INVENTORY_ITEMS - 1) % NUM_INVENTORY_ITEMS;
			lastTouchSelection = -1;
		} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UILeft)) {
			_inventorySelectedItem = (_inventorySelectedItem + NUM_INVENTORY_ITEMS - 4) % NUM_INVENTORY_ITEMS;
			lastTouchSelection = -1;
		} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIRight)) {
			_inventorySelectedItem = (_inventorySelectedItem + 4) % NUM_INVENTORY_ITEMS;
			lastTouchSelection = -1;
		}
		if (previous != _inventorySelectedItem) {
			_lowerScreenInventoryDescriptionPage = 0;
			_lowerScreenInventoryAngle = LBAAngles::ANGLE_45;
			_lowerScreenInventoryRotateAt = g_system->getMillis();
			drawLowerScreenInventory();
		}

		if (_engine->_input->toggleActionIfActive(TwinEActionType::UIEnter) &&
		    _engine->_gameState->hasItem((InventoryItems)_inventorySelectedItem) &&
		    (!_engine->_gameState->inventoryDisabled() || _inventorySelectedItem == InventoryItems::kiCloverLeaf)) {
			_engine->_loopInventoryItem = _inventorySelectedItem;
			usedWithA = true;
			break;
		}
	}

	// If A closed the inventory, keep the UI keymap active until the physical
	// button-release event has arrived. This prevents that same press from
	// becoming a jump/punch/action in gameplay (and prevents stuck repeats).
	if (usedWithA) {
		while (_engine->_input->is3DSConfirmHeld() && !_engine->shouldQuit()) {
			FrameMarker frame(_engine, 60);
			_engine->readKeys();
		}
	}

	// Inventory text must never leave the global dialogue state armed: lower
	// screen taps in gameplay are otherwise mistaken for "next dialogue page".
	_engine->_text->closeDial();
	_engine->_text->initSceneTextBank();

	_engine->_input->clearLowerScreenTap();
	_engine->_input->resetActionStates();
	_engine->_input->enableKeyMap(mainKeyMapId);

	_lowerScreenLastUpdate = 0;
	drawLowerScreenHud();
}

void Menu::showLowerScreenDialoguePanel() {
	_lowerScreenLastUpdate = 0;
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, fmt);
	}
	const uint32 bg = fmt.RGBToColor(13, 15, 27);
	const uint32 panel = fmt.RGBToColor(27, 30, 48);
	const uint32 border = fmt.RGBToColor(67, 158, 220);
	const uint32 text = fmt.RGBToColor(235, 237, 244);
	const uint32 muted = fmt.RGBToColor(145, 150, 171);
	_lowerScreenSurface.fillRect(_lowerScreenSurface.getBounds(), bg);
	_lowerScreenSurface.fillRect(Common::Rect(18, 62, 302, 178), panel);
	drawLowerScreenFrame(Common::Rect(18, 62, 302, 178), border);
	drawLowerLbaText("TOUCH TO CONTINUE", 20, 91, 280, text, true);
	drawLowerLbaText("D-PAD RIGHT: NEXT    B: BACK", 20, 136, 280, muted, true);
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::showLowerScreenKeyboard(const char *typedText, int selectedX, int selectedY, bool uppercase) {
	_lowerScreenLastUpdate = 0;
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}

	// Render the keyboard with TwinE/LBA's native CLUT8 renderer instead of
	// ScummVM GUI fonts. Reuse the cached classic ship/menu background so no
	// HQR file is decoded every keyboard frame.
	ensureLowerScreenMenuBase();

	_engine->_frontVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
	_engine->_workVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
	_engine->_frontVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
	_engine->_workVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
	_engine->setPalette(0, 256, _lowerScreenMenuPalette);

	_engine->_text->setFontColor(15);
	_engine->_text->setFont(INTER_LEAVE, INTER_SPACE);

	// Use the localized game's own prompt, but keep the entered name inside the
	// original-style animated plasma field so white letters never disappear into
	// the bright sky background.
	char title[256];
	_engine->_text->getMenuText(TextId::kEnterYourName, title, sizeof(title));

	int textWidth = _engine->_text->sizeFont(title);
	_engine->_text->drawText((_engine->width() - textWidth) / 2, 16, title);

	const Common::Rect nameRect(45, 58, _engine->width() - 45, 104);
	// Match the original LBA name-entry field: live plasma underneath a shaded
	// inner box. The focused keyboard key itself remains the original solid red.
	processPlasmaEffect(nameRect, 32);
	Common::Rect nameBox(nameRect);
	nameBox.grow(-1);
	drawRectBorders(nameRect);
	_engine->_interface->shadeBox(nameBox, 3);

	const char *name = typedText ? typedText : "";
	textWidth = _engine->_text->sizeFont(name);
	_engine->_text->drawText((_engine->width() - textWidth) / 2, nameRect.top + 10, name);

	static const char *const rows[] = {
		"1234567890",
		"QWERTYUIOP",
		"ASDFGHJKL",
		"ZXCVBNM"
	};

	const int keyW = 58;
	const int keyH = 52;
	const int firstTop = 116;
	const int rowStep = 56;

	auto drawClassicKey = [&](const Common::Rect &rect, const char *label, bool selected) {
		// Match the original PC keyboard: normal cells are a slightly darkened
		// view of the artwork and the focused key is a solid red selection.
		_engine->blitWorkToFront(rect);
		if (selected) {
			_engine->_interface->box(rect, COLOR_91);
		} else {
			_engine->_interface->shadeBox(rect, 4);
		}
		drawRectBorders(rect);

		_engine->_text->setFontColor(COLOR_WHITE);
		const int width = _engine->_text->sizeFont(label);
		_engine->_text->drawText(
			rect.left + (rect.width() - width) / 2,
			rect.top + 12,
			label
		);
	};

	for (int row = 0; row < 4; ++row) {
		const int count = strlen(rows[row]);
		const int left = (_engine->width() - count * keyW) / 2;
		const int top = firstTop + row * rowStep;

		for (int col = 0; col < count; ++col) {
			char key = rows[row][col];
			if (!uppercase && key >= 'A' && key <= 'Z') {
				key = (char)(key + ('a' - 'A'));
			}

			char label[2] = { key, '\0' };

			const Common::Rect rect(
				left + col * keyW,
				top,
				left + (col + 1) * keyW - 4,
				top + keyH
			);

			drawClassicKey(rect, label, selectedY == row && selectedX == col);
		}
	}

	// Bottom row follows exactly the current 3DS touch hitboxes, only rendered
	// with the original LBA font/buttons.
	const Common::Rect caseRect(30, 384, 136, 440);
	const Common::Rect spaceRect(142, 384, 356, 440);
	const Common::Rect delRect(362, 384, 476, 440);
	const Common::Rect okRect(482, 384, 610, 440);

	drawClassicKey(caseRect, uppercase ? "abc" : "ABC",
	               selectedY == 4 && selectedX == 0);
	drawClassicKey(spaceRect, "SPACE",
	               selectedY == 4 && selectedX == 1);
	drawClassicKey(delRect, "DEL",
	               selectedY == 4 && selectedX == 2);
	drawClassicKey(okRect, "OK",
	               selectedY == 4 && selectedX == 3);

	// Transfer the native 640x480 LBA frame to the 320x240 touch LCD.
	copyFrontBufferToLowerScreen();

	// Rendering the lower screen borrows the native TwinE framebuffer.
	// Never allow the keyboard/ship frame to leak to the upper LCD.
	restoreUpperScreenLogo();
}

void Menu::showLowerScreenSkipPanel() {
	_lowerScreenLastUpdate = 0;
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, fmt);
	}
	const uint32 bg = fmt.RGBToColor(13, 15, 27);
	const uint32 panel = fmt.RGBToColor(27, 30, 48);
	const uint32 border = fmt.RGBToColor(67, 158, 220);
	const uint32 text = fmt.RGBToColor(235, 237, 244);
	const uint32 muted = fmt.RGBToColor(145, 150, 171);
	_lowerScreenSurface.fillRect(_lowerScreenSurface.getBounds(), bg);
	_lowerScreenSurface.fillRect(Common::Rect(18, 62, 302, 178), panel);
	drawLowerScreenFrame(Common::Rect(18, 62, 302, 178), border);
	drawLowerLbaText("TOUCH TO SKIP", 20, 91, 280, text, true);
	drawLowerLbaText("B: SKIP", 20, 136, 280, muted, true);
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::showLowerScreenGameFrame(const char *hint) {
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, fmt);
	}
	byte palette[256 * 3];
	if (_upperPauseOverlayActive) {
		memcpy(palette, _upperPausePalette, sizeof(palette));
	} else {
		_engine->_frontVideoBuffer.getPalette(palette, 0, 256);
	}
	const int srcW = _engine->_frontVideoBuffer.w;
	const int srcH = _engine->_frontVideoBuffer.h;
	for (int y = 0; y < 240; ++y) {
		const int sy = y * srcH / 240;
		const byte *src = (const byte *)_engine->_frontVideoBuffer.getBasePtr(0, sy);
		uint32 *dst = (uint32 *)_lowerScreenSurface.getBasePtr(0, y);
		for (int x = 0; x < 320; ++x) {
			const int sx = x * srcW / 320;
			const byte *rgb = &palette[(uint32)src[sx] * 3];
			dst[x] = fmt.ARGBToColor(255, rgb[0], rgb[1], rgb[2]);
		}
	}
	if (hint && *hint) {
		const uint32 bar = fmt.RGBToColor(13, 15, 27);
		const uint32 border = fmt.RGBToColor(67, 158, 220);
		const uint32 text = fmt.RGBToColor(235, 237, 244);
		_lowerScreenSurface.fillRect(Common::Rect(0, 217, 320, 240), bar);
		drawLowerScreenFrame(Common::Rect(0, 217, 320, 240), border);
		drawLowerLbaText(hint, 4, 218, 312, text, true);
	}
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::showLowerScreenHolomapFrame(const char *hint) {
	// Force a HUD refresh when Holomap mode returns to gameplay.
	_lowerScreenLastUpdate = 0;
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, fmt);
	}

	const uint32 space = fmt.RGBToColor(3, 5, 16);
	const uint32 star = fmt.RGBToColor(188, 205, 235);
	const uint32 brightStar = fmt.RGBToColor(235, 241, 255);
	_lowerScreenSurface.fillRect(_lowerScreenSurface.getBounds(), space);

	// Deterministic star field: no allocations, no RNG state and no flicker.
	for (int i = 0; i < 56; ++i) {
		const int x = (i * 73 + 19) % 320;
		const int y = (i * 41 + 11) % 210;
		const uint32 c = (i % 9 == 0) ? brightStar : star;
		_lowerScreenSurface.fillRect(Common::Rect(x, y, x + 1, y + 1), c);
		if (i % 17 == 0 && x < 319 && y < 209) {
			_lowerScreenSurface.fillRect(Common::Rect(x + 1, y, x + 2, y + 1), c);
		}
	}

	// The LBA holomap itself is rendered into a 300x280 rectangle around the
	// centre of the 640x480 game framebuffer. Crop that region and enlarge it
	// 1.5x relative to the old full-screen downscale, while treating near-black
	// pixels as transparent so the globe floats naturally in space.
	byte palette[256 * 3];
	// Holomap installs its own palette after the upper PAUSE overlay has been
	// created. Using _upperPausePalette here would apply the gameplay palette
	// to the globe and corrupt its colours. The lower LCD should always use the
	// Holomap's current, live palette.
	_engine->_frontVideoBuffer.getPalette(palette, 0, 256);
	const int srcW = _engine->_frontVideoBuffer.w;
	const int srcH = _engine->_frontVideoBuffer.h;
	const int cropW = MIN(300, srcW);
	const int cropH = MIN(280, srcH);
	const int cropX = (srcW - cropW) / 2;
	const int cropY = MIN(50, MAX(0, srcH - cropH));
	const int dstW = 225;
	// Leave a little more vertical room for a two-line location caption.
	// The previous 210 px globe left only 33 px for text, so the second line
	// was clipped on the real 3DS even though wrapping itself was working.
	const int dstH = 196;
	const int dstX = (320 - dstW) / 2;
	const int dstY = 3;

	for (int y = 0; y < dstH; ++y) {
		const int sy = cropY + y * cropH / dstH;
		const byte *src = (const byte *)_engine->_frontVideoBuffer.getBasePtr(0, sy);
		uint32 *dst = (uint32 *)_lowerScreenSurface.getBasePtr(0, dstY + y);
		for (int x = 0; x < dstW; ++x) {
			const int sx = cropX + x * cropW / dstW;
			const byte *rgb = &palette[(uint32)src[sx] * 3];
			if ((int)rgb[0] + (int)rgb[1] + (int)rgb[2] > 18) {
				dst[dstX + x] = fmt.ARGBToColor(255, rgb[0], rgb[1], rgb[2]);
			}
		}
	}

	if (hint && *hint) {
		const uint32 bar = fmt.RGBToColor(13, 15, 27);
		const uint32 border = fmt.RGBToColor(67, 158, 220);
		const uint32 text = fmt.RGBToColor(235, 237, 244);
		const Common::Rect captionRect(0, 196, 320, 240);
		_lowerScreenSurface.fillRect(captionRect, bar);
		drawLowerScreenFrame(captionRect, border);
		// Reserve a genuine two-line caption area. Use the original LBA bitmap
		// font for both place names and our 3DS control hint.
		const char *backMarker = strstr(hint, "B:");
		if (backMarker) {
			Common::String location(hint, backMarker - hint);
			while (!location.empty() && (location.lastChar() == ' ' || location.lastChar() == '\t')) {
				location.deleteLastChar();
			}

			const char *islandMarker = strstr(location.c_str(), " @ ");
			if (islandMarker) {
				Common::String place(location.c_str(), islandMarker - location.c_str());
				Common::String island(islandMarker + 3);
				drawLowerLbaText(place.c_str(), 7, 197, 228, text, false);
				drawLowerLbaText(island.c_str(), 7, 214, 228, text, false);
			} else {
				drawLowerLbaWrappedText(location.c_str(), 7, 197, 228, 2, text);
			}
			drawLowerLbaText(backMarker, 242, 214, 72, text, true);
		} else {
			drawLowerLbaWrappedText(hint, 7, 197, 306, 2, text);
		}
	}

	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

static void copyFrontBufferScaledToSurface(TwinEEngine *engine, Graphics::ManagedSurface &dst) {
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (dst.w != 320 || dst.h != 240 || dst.format != fmt) {
		dst.free();
		dst.create(320, 240, fmt);
	}
	byte palette[256 * 3];
	engine->_frontVideoBuffer.getPalette(palette, 0, 256);
	const int srcW = engine->_frontVideoBuffer.w;
	const int srcH = engine->_frontVideoBuffer.h;
	for (int y = 0; y < 240; ++y) {
		const int sy = y * srcH / 240;
		const byte *src = (const byte *)engine->_frontVideoBuffer.getBasePtr(0, sy);
		uint32 *out = (uint32 *)dst.getBasePtr(0, y);
		for (int x = 0; x < 320; ++x) {
			const byte idx = src[x * srcW / 320];
			const byte *rgb = &palette[(uint32)idx * 3];
			out[x] = fmt.RGBToColor(rgb[0], rgb[1], rgb[2]);
		}
	}
}

static void copyFrontBufferRectScaledToSurface(TwinEEngine *engine, Graphics::ManagedSurface &dst, const Common::Rect &srcRect) {
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (dst.w != 320 || dst.h != 240 || dst.format != fmt) {
		dst.free();
		dst.create(320, 240, fmt);
	}
	byte palette[256 * 3];
	engine->_frontVideoBuffer.getPalette(palette, 0, 256);
	const int srcW = engine->_frontVideoBuffer.w;
	const int srcH = engine->_frontVideoBuffer.h;
	const int dstLeft = CLIP<int>(srcRect.left * 320 / srcW, 0, 319);
	const int dstRight = CLIP<int>((srcRect.right * 320 + srcW - 1) / srcW, 1, 320);
	const int dstTop = CLIP<int>(srcRect.top * 240 / srcH, 0, 239);
	const int dstBottom = CLIP<int>((srcRect.bottom * 240 + srcH - 1) / srcH, 1, 240);
	for (int y = dstTop; y < dstBottom; ++y) {
		const int sy = y * srcH / 240;
		const byte *src = (const byte *)engine->_frontVideoBuffer.getBasePtr(0, sy);
		uint32 *out = (uint32 *)dst.getBasePtr(0, y);
		for (int x = dstLeft; x < dstRight; ++x) {
			const int sx = x * srcW / 320;
			const byte idx = src[sx];
			const byte *rgb = &palette[(uint32)idx * 3];
			out[x] = fmt.RGBToColor(rgb[0], rgb[1], rgb[2]);
		}
	}
}

void Menu::copyFrontBufferToLowerScreen() {
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	copyFrontBufferScaledToSurface(_engine, _lowerScreenSurface);
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::ensureUpperScreenLogo() {
	if (_upperScreenLogoReady &&
	    _upperScreenLogoNativeBase.w == _engine->_frontVideoBuffer.w &&
	    _upperScreenLogoNativeBase.h == _engine->_frontVideoBuffer.h &&
	    _upperScreenLogoNativeBase.format == _engine->_frontVideoBuffer.format) {
		return;
	}
	_engine->_screens->loadImage(_engine->_resources->lbaLogo());
	_upperScreenLogoNativeBase.free();
	_upperScreenLogoNativeBase.create(_engine->_frontVideoBuffer.w, _engine->_frontVideoBuffer.h,
	                                  _engine->_frontVideoBuffer.format);
	_upperScreenLogoNativeBase.blitFrom(_engine->_frontVideoBuffer);
	_engine->_frontVideoBuffer.getPalette(_upperScreenLogoPalette, 0, 256);
	_upperScreenLogoReady = true;
}

void Menu::restoreUpperScreenLogo() {
	ensureUpperScreenLogo();
	_engine->_frontVideoBuffer.blitFrom(_upperScreenLogoNativeBase);
	_engine->_workVideoBuffer.blitFrom(_upperScreenLogoNativeBase);
	_engine->_frontVideoBuffer.setPalette(_upperScreenLogoPalette, 0, 256);
	_engine->_workVideoBuffer.setPalette(_upperScreenLogoPalette, 0, 256);
	_engine->setPalette(0, 256, _upperScreenLogoPalette);
	_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
}

void Menu::ensureLowerScreenMenuBase() {
	if (_lowerScreenMenuBaseReady &&
	    _lowerScreenMenuNativeBase.w == _engine->_frontVideoBuffer.w &&
	    _lowerScreenMenuNativeBase.h == _engine->_frontVideoBuffer.h &&
	    _lowerScreenMenuNativeBase.format == _engine->_frontVideoBuffer.format) {
		return;
	}

	// Cache the upper logo first, because loadMenuImage() temporarily replaces
	// both the pixels and palette used by the physical top LCD.
	ensureUpperScreenLogo();
	_engine->_screens->loadMenuImage(false);
	_lowerScreenMenuNativeBase.free();
	_lowerScreenMenuNativeBase.create(_engine->_frontVideoBuffer.w, _engine->_frontVideoBuffer.h,
	                                  _engine->_frontVideoBuffer.format);
	_lowerScreenMenuNativeBase.blitFrom(_engine->_frontVideoBuffer);
	_engine->_frontVideoBuffer.getPalette(_lowerScreenMenuPalette, 0, 256);
	_lowerScreenMenuBaseReady = true;
	restoreUpperScreenLogo();
}

enum SaveSelectorMode3DS {
	kSaveSelectorNormal = 0,
	kSaveSelectorCopySource,
	kSaveSelectorCopyTarget,
	kSaveSelectorCopyConfirm,
	kSaveSelectorDeleteTarget,
	kSaveSelectorDeleteConfirm
};

void Menu::refresh3DSSaveSlotCache() {
	for (int slot = 0; slot < 3; ++slot) {
		_3DSSaveSlotNames[slot] = "NEW GAME";
		_3DSSaveSlotOccupied[slot] = false;
	}
	for (const SaveStateDescriptor &save : _engine->getSaveSlots()) {
		const int slot = save.getSaveSlot();
		if (slot >= 0 && slot < 3) {
			_3DSSaveSlotOccupied[slot] = true;
			_3DSSaveSlotNames[slot] = save.getDescription();
		}
	}
	_3DSSaveSlotCacheValid = true;
	// Save names are part of the static menu artwork, so any refresh invalidates
	// that artwork as well.
	_lowerScreenSaveStaticMode = -1;
}

void Menu::animateLowerScreenSaveSelector() {
	if (_lowerScreenSaveSelectedItem < 0 || _lowerScreenSaveStaticMode < 0) {
		return;
	}

	// Render the selected button exactly like the original TwinE menu on every
	// animation tick. Do not replay a short cache of plasma snapshots: that made
	// the flame pattern visibly loop/stutter on the 3DS lower screen.
	MenuSettings profiles;
	profiles.setButtonsBoxHeight(230);
	for (int slot = 0; slot < 3; ++slot) {
		profiles.addButton(_3DSSaveSlotNames[slot].c_str(), slot);
	}

	Common::String action1;
	Common::String action2;
	switch (_lowerScreenSaveStaticMode) {
	case kSaveSelectorCopySource:
		action1 = "SELECT SOURCE";
		action2 = "CANCEL";
		break;
	case kSaveSelectorCopyTarget:
		action1 = _lowerScreenSaveStaticSource >= 0 && _lowerScreenSaveStaticSource < 3
		              ? Common::String::format("COPY %s TO...", _3DSSaveSlotNames[_lowerScreenSaveStaticSource].c_str())
		              : Common::String("SELECT DESTINATION");
		action2 = "CANCEL";
		break;
	case kSaveSelectorCopyConfirm:
		action1 = _lowerScreenSaveStaticTarget >= 0 && _lowerScreenSaveStaticTarget < 3
		              ? Common::String::format("COPY TO %s", _3DSSaveSlotNames[_lowerScreenSaveStaticTarget].c_str())
		              : Common::String("COPY");
		action2 = "CANCEL";
		break;
	case kSaveSelectorDeleteTarget:
		action1 = "SELECT FILE TO DELETE";
		action2 = "CANCEL";
		break;
	case kSaveSelectorDeleteConfirm:
		action1 = _lowerScreenSaveStaticTarget >= 0 && _lowerScreenSaveStaticTarget < 3
		              ? Common::String::format("DELETE %s", _3DSSaveSlotNames[_lowerScreenSaveStaticTarget].c_str())
		              : Common::String("DELETE");
		action2 = "CANCEL";
		break;
	case kSaveSelectorNormal:
	default:
		action1 = "COPY";
		action2 = "DELETE";
		break;
	}
	profiles.addButton(action1.c_str(), 3);
	profiles.addButton(action2.c_str(), 4);
	profiles.setActiveButton(_lowerScreenSaveSelectedItem);

	_engine->_frontVideoBuffer.blitFrom(_lowerScreenSaveNativeStatic);
	_engine->_frontVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
	drawButtons(&profiles, true);

	_lowerScreenSurface.blitFrom(_lowerScreenSaveStaticSurface);
	copyFrontBufferRectScaledToSurface(_engine, _lowerScreenSurface, _lowerScreenSaveActiveRect);
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
	restoreUpperScreenLogo();
}

static int lowerScreenMenuButtonAt(const MenuSettings *menuSettings, const Common::Point &tap, int sourceW, int sourceH) {
	const int count = menuSettings->getButtonCount();
	if (count <= 0) {
		return -1;
	}
	int topHeight = menuSettings->getButtonBoxHeight();
	if (topHeight == 0) {
		topHeight = 35;
	} else {
		topHeight -= (((count - 1) * 6) + (count * HEIGHT_STANDARD)) / 2;
	}
	for (int i = 0; i < count; ++i) {
		const Common::Rect srcRect(45, topHeight - HEIGHT_STANDARD / 2, sourceW - 45, topHeight + HEIGHT_STANDARD / 2);
		const Common::Rect dstRect(srcRect.left * 320 / sourceW, srcRect.top * 240 / sourceH,
		                           srcRect.right * 320 / sourceW, srcRect.bottom * 240 / sourceH);
		if (dstRect.contains(tap)) {
			return i;
		}
		topHeight += HEIGHT_STANDARD + MENU_SPACE;
	}
	return -1;
}

void Menu::fadeLowerScreenToBlack() {
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240 ||
	    g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		showLowerScreenBlack();
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenFadeBase.w != 320 || _lowerScreenFadeBase.h != 240) {
		_lowerScreenFadeBase.free();
		_lowerScreenFadeBase.create(320, 240, fmt);
	}
	_lowerScreenFadeBase.blitFrom(_lowerScreenSurface);
	for (int step = 7; step >= 0; --step) {
		for (int y = 0; y < 240; ++y) {
			const byte *src = (const byte *)_lowerScreenFadeBase.getBasePtr(0, y);
			byte *dst = (byte *)_lowerScreenSurface.getBasePtr(0, y);
			for (int x = 0; x < 320; ++x) {
				uint32 color = 0;
				memcpy(&color, src + x * fmt.bytesPerPixel, fmt.bytesPerPixel);
				uint8 r, g, b;
				fmt.colorToRGB(color, r, g, b);
				const uint32 faded = fmt.RGBToColor((uint8)(r * step / 7), (uint8)(g * step / 7), (uint8)(b * step / 7));
				memcpy(dst + x * fmt.bytesPerPixel, &faded, fmt.bytesPerPixel);
			}
		}
		g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
		g_system->updateScreen();
		g_system->delayMillis(12);
	}
}

void Menu::fadeLowerScreenFromBlack() {
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240 ||
	    g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenFadeBase.w != 320 || _lowerScreenFadeBase.h != 240) {
		_lowerScreenFadeBase.free();
		_lowerScreenFadeBase.create(320, 240, fmt);
	}
	_lowerScreenFadeBase.blitFrom(_lowerScreenSurface);
	for (int step = 0; step <= 7; ++step) {
		for (int y = 0; y < 240; ++y) {
			const byte *src = (const byte *)_lowerScreenFadeBase.getBasePtr(0, y);
			byte *dst = (byte *)_lowerScreenSurface.getBasePtr(0, y);
			for (int x = 0; x < 320; ++x) {
				uint32 color = 0;
				memcpy(&color, src + x * fmt.bytesPerPixel, fmt.bytesPerPixel);
				uint8 r, g, b;
				fmt.colorToRGB(color, r, g, b);
				const uint32 faded = fmt.RGBToColor((uint8)(r * step / 7), (uint8)(g * step / 7), (uint8)(b * step / 7));
				memcpy(dst + x * fmt.bytesPerPixel, &faded, fmt.bytesPerPixel);
			}
		}
		g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
		g_system->updateScreen();
		g_system->delayMillis(12);
	}
}

void Menu::showLowerScreenBlack() {
	_lowerScreenLastUpdate = 0;
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}
	const Graphics::PixelFormat fmt = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, fmt);
	}
	_lowerScreenSurface.fillRect(_lowerScreenSurface.getBounds(), fmt.RGBToColor(0, 0, 0));
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}

void Menu::showLowerScreenStartPanel() {
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}

	ensureLowerScreenMenuBase();

	// Use the cached original ship image as a scratch canvas, draw only the
	// original LBA font on top, then move the finished frame to the touch LCD.
	_engine->_frontVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
	_engine->_workVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
	_engine->_frontVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
	_engine->_workVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);

	const char *label = "PRESS START";
	_engine->_text->setFontColor(COLOR_WHITE);
	_engine->_text->setFont(INTER_LEAVE, INTER_SPACE);
	const int32 textWidth = _engine->_text->sizeFont(label);
	_engine->_text->drawText((_engine->width() - textWidth) / 2, 350, label);
	copyFrontBufferToLowerScreen();

	// Always finish on the cached LBA title image and, crucially, its palette.
	// Present immediately after the lower overlay has already been queued, so
	// PRESS START appears on both LCDs in the same refresh instead of the upper
	// logo flashing one frame earlier.
	restoreUpperScreenLogo();
	_engine->_frontVideoBuffer.forceUpdate();
	g_system->updateScreen();
}

void Menu::showLowerScreenSaveSelector(int selectedItem, int mode, int sourceSlot, int targetSlot) {
	_lowerScreenLastUpdate = 0;
	if (_engine->shouldQuit()) {
		return;
	}
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}

	ensureLowerScreenMenuBase();
	if (!_3DSSaveSlotCacheValid) {
		refresh3DSSaveSlotCache();
	}

	MenuSettings profiles;
	profiles.setButtonsBoxHeight(230);
	for (int slot = 0; slot < 3; ++slot) {
		profiles.addButton(_3DSSaveSlotNames[slot].c_str(), slot);
	}

	Common::String action1;
	Common::String action2;
	switch (mode) {
	case kSaveSelectorCopySource:
		action1 = "SELECT SOURCE";
		action2 = "CANCEL";
		break;
	case kSaveSelectorCopyTarget:
		action1 = sourceSlot >= 0 && sourceSlot < 3
		              ? Common::String::format("COPY %s TO...", _3DSSaveSlotNames[sourceSlot].c_str())
		              : Common::String("SELECT DESTINATION");
		action2 = "CANCEL";
		break;
	case kSaveSelectorCopyConfirm:
		action1 = targetSlot >= 0 && targetSlot < 3
		              ? Common::String::format("COPY TO %s", _3DSSaveSlotNames[targetSlot].c_str())
		              : Common::String("COPY");
		action2 = "CANCEL";
		break;
	case kSaveSelectorDeleteTarget:
		action1 = "SELECT FILE TO DELETE";
		action2 = "CANCEL";
		break;
	case kSaveSelectorDeleteConfirm:
		action1 = targetSlot >= 0 && targetSlot < 3
		              ? Common::String::format("DELETE %s", _3DSSaveSlotNames[targetSlot].c_str())
		              : Common::String("DELETE");
		action2 = "CANCEL";
		break;
	case kSaveSelectorNormal:
	default:
		action1 = "COPY";
		action2 = "DELETE";
		break;
	}
	profiles.addButton(action1.c_str(), 3);
	profiles.addButton(action2.c_str(), 4);
	const int activeItem = CLIP<int>(selectedItem, 0, 4);

	const bool layoutChanged = _lowerScreenSaveStaticMode != mode ||
	                           _lowerScreenSaveStaticSource != sourceSlot ||
	                           _lowerScreenSaveStaticTarget != targetSlot ||
	                           _lowerScreenSaveStaticSurface.w != 320 ||
	                           _lowerScreenSaveStaticSurface.h != 240;
	if (layoutChanged) {
		profiles.setActiveButton(-1);
		_engine->_frontVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
		_engine->_workVideoBuffer.blitFrom(_lowerScreenMenuNativeBase);
		_engine->_frontVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
		_engine->_workVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
		drawButtons(&profiles, false);

		if (_lowerScreenSaveNativeStatic.w != _engine->_frontVideoBuffer.w ||
		    _lowerScreenSaveNativeStatic.h != _engine->_frontVideoBuffer.h ||
		    _lowerScreenSaveNativeStatic.format != _engine->_frontVideoBuffer.format) {
			_lowerScreenSaveNativeStatic.free();
			_lowerScreenSaveNativeStatic.create(_engine->_frontVideoBuffer.w, _engine->_frontVideoBuffer.h,
			                                      _engine->_frontVideoBuffer.format);
		}
		_lowerScreenSaveNativeStatic.blitFrom(_engine->_frontVideoBuffer);
		copyFrontBufferScaledToSurface(_engine, _lowerScreenSaveStaticSurface);
		_lowerScreenSaveStaticMode = mode;
		_lowerScreenSaveStaticSource = sourceSlot;
		_lowerScreenSaveStaticTarget = targetSlot;
	}

	int topHeight = profiles.getButtonBoxHeight();
	topHeight -= (((profiles.getButtonCount() - 1) * 6) + (profiles.getButtonCount() * HEIGHT_STANDARD)) / 2;
	const int activeCenter = topHeight + activeItem * (HEIGHT_STANDARD + MENU_SPACE);
	_lowerScreenSaveActiveRect = Common::Rect(45, activeCenter - HEIGHT_STANDARD / 2,
	                                         _engine->width() - 45, activeCenter + HEIGHT_STANDARD / 2);
	_lowerScreenSaveSelectedItem = activeItem;

	// Build exactly one selected frame immediately. The remaining plasma frames
	// are generated later by animateLowerScreenSaveSelector() while idle.
	profiles.setActiveButton(activeItem);
	_engine->_frontVideoBuffer.blitFrom(_lowerScreenSaveNativeStatic);
	_engine->_frontVideoBuffer.setPalette(_lowerScreenMenuPalette, 0, 256);
	drawButtons(&profiles, true);
	const Graphics::PixelFormat overlayFmt = g_system->getOverlayFormat();
	if (_lowerScreenSaveAnimFrames[0].w != 320 || _lowerScreenSaveAnimFrames[0].h != 240 ||
	    _lowerScreenSaveAnimFrames[0].format != overlayFmt) {
		_lowerScreenSaveAnimFrames[0].free();
		_lowerScreenSaveAnimFrames[0].create(320, 240, overlayFmt);
	}
	_lowerScreenSaveAnimFrames[0].blitFrom(_lowerScreenSaveStaticSurface);
	copyFrontBufferRectScaledToSurface(_engine, _lowerScreenSaveAnimFrames[0], _lowerScreenSaveActiveRect);
	_lowerScreenSaveAnimGenerated = 1;
	_lowerScreenSaveAnimFrame = 0;
	_lowerScreenSurface.blitFrom(_lowerScreenSaveAnimFrames[0]);
	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
	restoreUpperScreenLogo();
}

void Menu::beginLowerScreenLoading() {
	// Keep the current lower-screen panel visible during ordinary scene changes.
	// Callers that really need a black touch LCD (startup, save loading, island
	// arrival) blank it explicitly before entering the synchronous load.
	_lowerScreenLoadingActive = true;
	_lowerScreenLoadingLastUpdate = 0;
	_lowerScreenLoadingAngle = LBAAngles::ANGLE_0;
	memcpy(_lowerScreenLoadingPalette, _engine->_screens->_ptrPal.data(), sizeof(_lowerScreenLoadingPalette));

	// Keep the actual upper framebuffer black for the whole synchronous load.
	// Earlier passes restored the last gameplay frame immediately after drawing
	// the ball, so FrameMarker simply put that old frame back on the LCD.
	_engine->_frontVideoBuffer.fillRect(_engine->_frontVideoBuffer.getBounds(), COLOR_BLACK);
	_engine->_frontVideoBuffer.setPalette(_lowerScreenLoadingPalette, 0, 256);
	_engine->setPalette(_engine->_screens->_ptrPal);
	_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
	updateLowerScreenLoadingIndicator(true);
}

void Menu::updateLowerScreenLoadingIndicator(bool force) {
	if (!_lowerScreenLoadingActive) {
		return;
	}
	const uint32 now = g_system->getMillis();
	if (!force && _lowerScreenLoadingLastUpdate != 0 && now - _lowerScreenLoadingLastUpdate < 80) {
		return;
	}
	_lowerScreenLoadingLastUpdate = now;
	_lowerScreenLoadingAngle = ClampAngle(_lowerScreenLoadingAngle + LBAAngles::ANGLE_45);

	const Common::Rect screenRect = _engine->_frontVideoBuffer.getBounds();
	_engine->_frontVideoBuffer.fillRect(screenRect, COLOR_BLACK);
	_engine->_frontVideoBuffer.setPalette(_lowerScreenLoadingPalette, 0, 256);
	_engine->setPalette(_engine->_screens->_ptrPal);

	const int cx = _engine->width() - 24;
	const int cy = _engine->height() - 22;
	// The previous distance made the sphere larger than the clip rectangle, so
	// the LCD showed a solid yellow square. Keep a generous clip but render the
	// actual 3D ball much farther away: ~12-14 px visible diameter.
	const Common::Rect iconRect(cx - 14, cy - 14, cx + 14, cy + 14);
	_engine->_interface->setClip(iconRect);
	_engine->_renderer->draw3dObject(cx, cy, _engine->_resources->_inventoryTable[InventoryItems::kiMagicBall],
	                                _lowerScreenLoadingAngle, 30000);
	_engine->_interface->unsetClip();
	_engine->_frontVideoBuffer.addDirtyRect(screenRect);
	// g_system->updateScreen() alone does not upload TwinE's ManagedSurface.
	// Also, TwineScreen::update() normally suppresses a second present in the
	// same engine frame. Scene loading is synchronous, so bypass that guard and
	// push every black/spinner frame to the physical upper LCD immediately.
	_engine->_frontVideoBuffer.forceUpdate();
	_engine->_gameState->init3DGame();
}

void Menu::endLowerScreenLoading() {
	_lowerScreenLoadingActive = false;
	_lowerScreenLoadingLastUpdate = 0;
	_lowerScreenLastUpdate = 0;

	// loadGameState()/scene resource setup may have touched indexed pixels or the
	// active palette while the loading marker was visible. Re-establish a clean
	// black gameplay frame *after* all loading work, otherwise one title/menu
	// framebuffer can flash for a frame with the gameplay palette.
	_engine->_screens->_flagPalettePcx = false;
	_engine->_frontVideoBuffer.fillRect(_engine->_frontVideoBuffer.getBounds(), COLOR_BLACK);
	_engine->_frontVideoBuffer.setPalette(_engine->_screens->_ptrPal.data(), 0, 256);
	_engine->_workVideoBuffer.fillRect(_engine->_workVideoBuffer.getBounds(), COLOR_BLACK);
	_engine->_workVideoBuffer.setPalette(_engine->_screens->_ptrPal.data(), 0, 256);
	_engine->setPalette(_engine->_screens->_ptrPal);
	_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
	_engine->_frontVideoBuffer.forceUpdate();
}

EngineState Menu::run3DSFrontEnd() {
	_engine->_input->enableKeyMap(uiKeyMapId);
	_engine->_input->resetActionStates();
	_engine->_input->clearLowerScreenTap();

	// Cache the title image once. Do not present it yet: the PRESS START helper
	// queues the lower LCD first and then presents both screens together.
	ensureUpperScreenLogo();

	if (!_3DSStartGateShown) {
		showLowerScreenStartPanel();
		bool startWasDown = (hidKeysHeld() & KEY_START) != 0;
		for (;;) {
			FrameMarker frame(_engine, 60);
			_engine->readKeys();
			if (_engine->shouldQuit()) {
				return EngineState::QuitGame;
			}
			const bool startDown = (hidKeysHeld() & KEY_START) != 0;
			Common::Point tap;
			const bool touched = _engine->_input->consumeLowerScreenTap(tap);
			if ((startDown && !startWasDown) || touched) {
				_3DSStartGateShown = true;
				break;
			}
			startWasDown = startDown;
		}
		_engine->_input->resetActionStates();
		_engine->_input->clearLowerScreenTap();
	}

	int selectedItem = CLIP<int>(_engine->_active3DSSaveSlot, 0, 2);
	if (_engine->_active3DSSaveSlot < 0) {
		selectedItem = 0;
	}
	int selectorMode = kSaveSelectorNormal;
	int copySourceSlot = -1;
	int targetSlot = -1;

	_3DSSaveSlotCacheValid = false;
	refresh3DSSaveSlotCache();
	auto getSlotInfo = [&](int slot, Common::String *name) -> bool {
		if (slot < 0 || slot >= 3 || !_3DSSaveSlotOccupied[slot]) {
			if (name) {
				name->clear();
			}
			return false;
		}
		if (name) {
			*name = _3DSSaveSlotNames[slot];
		}
		return true;
	};

	auto isSelectable = [&](int item) -> bool {
		if (item < 0 || item > 4) {
			return false;
		}
		switch (selectorMode) {
		case kSaveSelectorCopySource:
		case kSaveSelectorDeleteTarget:
			return item == 4 || (item >= 0 && item < 3 && getSlotInfo(item, nullptr));
		case kSaveSelectorCopyTarget:
			return item == 4 || (item >= 0 && item < 3 && item != copySourceSlot);
		case kSaveSelectorCopyConfirm:
		case kSaveSelectorDeleteConfirm:
			return item == 3 || item == 4;
		case kSaveSelectorNormal:
		default:
			return true;
		}
	};

	auto chooseFirstSelectable = [&](int preferred) {
		if (isSelectable(preferred)) {
			selectedItem = preferred;
			return;
		}
		for (int item = 0; item < 5; ++item) {
			if (isSelectable(item)) {
				selectedItem = item;
				return;
			}
		}
		selectedItem = 4;
	};

	showLowerScreenSaveSelector(selectedItem, selectorMode, copySourceSlot, targetSlot);

	uint32 lastAnimationFrame = g_system->getMillis();
	for (;;) {
		FrameMarker frame(_engine, 60);
		_engine->readKeys();
		if (_engine->shouldQuit()) {
			return EngineState::QuitGame;
		}

		bool redraw = false;
		auto moveSelection = [&](int direction) {
			int next = selectedItem;
			for (int i = 0; i < 5; ++i) {
				next = (next + direction + 5) % 5;
				if (isSelectable(next)) {
					selectedItem = next;
					redraw = true;
					return;
				}
			}
		};

		if (_engine->_input->toggleActionIfActive(TwinEActionType::UIDown)) {
			moveSelection(1);
		} else if (_engine->_input->toggleActionIfActive(TwinEActionType::UIUp)) {
			moveSelection(-1);
		}

		bool activate = _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter);
		Common::Point tap;
		if (_engine->_input->consumeLowerScreenTap(tap)) {
			// Five original LBA buttons rendered at 640x480 and scaled to 320x240.
			// The list is intentionally raised: first row now starts at y=34.
			// First tap highlights an entry; a second tap on it confirms it.
			for (int item = 0; item < 5; ++item) {
				const int top = 34 + item * 28;
				if (tap.x >= 22 && tap.x < 298 && tap.y >= top && tap.y < top + 25 && isSelectable(item)) {
					if (item == selectedItem) {
						activate = true;
					} else {
						selectedItem = item;
						redraw = true;
					}
					break;
				}
			}
		}

		if (_engine->_input->toggleAbortAction()) {
			if (selectorMode != kSaveSelectorNormal) {
				selectorMode = kSaveSelectorNormal;
				selectedItem = copySourceSlot >= 0 ? copySourceSlot : (targetSlot >= 0 ? targetSlot : 0);
				copySourceSlot = -1;
				targetSlot = -1;
				redraw = true;
			} else {
				_3DSStartGateShown = false;
				_engine->_input->resetActionStates();
				_engine->_input->clearLowerScreenTap();
				return EngineState::Menu;
			}
		}

		const uint32 now = g_system->getMillis();
		if (redraw) {
			showLowerScreenSaveSelector(selectedItem, selectorMode, copySourceSlot, targetSlot);
			lastAnimationFrame = now;
		} else if (now - lastAnimationFrame >= 50) {
			// Advance the original live LBA plasma at ~20 fps on the touch LCD.
			animateLowerScreenSaveSelector();
			lastAnimationFrame = now;
		}
		if (!activate) {
			continue;
		}

		// COPY / DELETE are part of the same profile screen, Zelda-style. The
		// source/target/confirmation steps keep destructive actions deliberate.
		if (selectorMode == kSaveSelectorNormal && selectedItem == 3) {
			selectorMode = kSaveSelectorCopySource;
			copySourceSlot = -1;
			targetSlot = -1;
			chooseFirstSelectable(0);
			showLowerScreenSaveSelector(selectedItem, selectorMode, copySourceSlot, targetSlot);
			continue;
		}
		if (selectorMode == kSaveSelectorNormal && selectedItem == 4) {
			selectorMode = kSaveSelectorDeleteTarget;
			copySourceSlot = -1;
			targetSlot = -1;
			chooseFirstSelectable(0);
			showLowerScreenSaveSelector(selectedItem, selectorMode, copySourceSlot, targetSlot);
			continue;
		}
		if (selectorMode == kSaveSelectorCopySource) {
			if (selectedItem == 4) {
				selectorMode = kSaveSelectorNormal;
				selectedItem = 3;
				showLowerScreenSaveSelector(selectedItem, selectorMode);
				continue;
			}
			copySourceSlot = selectedItem;
			selectorMode = kSaveSelectorCopyTarget;
			targetSlot = -1;
			chooseFirstSelectable((copySourceSlot + 1) % 3);
			showLowerScreenSaveSelector(selectedItem, selectorMode, copySourceSlot, targetSlot);
			continue;
		}
		if (selectorMode == kSaveSelectorCopyTarget) {
			if (selectedItem == 4) {
				selectorMode = kSaveSelectorNormal;
				selectedItem = copySourceSlot >= 0 ? copySourceSlot : 3;
				copySourceSlot = -1;
				showLowerScreenSaveSelector(selectedItem, selectorMode);
				continue;
			}
			targetSlot = selectedItem;
			selectorMode = kSaveSelectorCopyConfirm;
			selectedItem = 3;
			showLowerScreenSaveSelector(selectedItem, selectorMode, copySourceSlot, targetSlot);
			continue;
		}
		if (selectorMode == kSaveSelectorCopyConfirm) {
			if (selectedItem == 3 && copySourceSlot >= 0 && targetSlot >= 0) {
				Common::String sourceName;
				getSlotInfo(copySourceSlot, &sourceName);
				if (_engine->copySaveSlot(copySourceSlot, targetSlot)) {
					_3DSSaveSlotCacheValid = false;
					refresh3DSSaveSlotCache();
					if (_engine->_active3DSSaveSlot == targetSlot) {
						_engine->_active3DSSaveName = sourceName;
					}
					selectedItem = targetSlot;
				} else {
					selectedItem = copySourceSlot;
				}
			} else {
				selectedItem = copySourceSlot >= 0 ? copySourceSlot : 3;
			}
			selectorMode = kSaveSelectorNormal;
			copySourceSlot = -1;
			targetSlot = -1;
			showLowerScreenSaveSelector(selectedItem, selectorMode);
			continue;
		}
		if (selectorMode == kSaveSelectorDeleteTarget) {
			if (selectedItem == 4) {
				selectorMode = kSaveSelectorNormal;
				selectedItem = 4;
				showLowerScreenSaveSelector(selectedItem, selectorMode);
				continue;
			}
			targetSlot = selectedItem;
			selectorMode = kSaveSelectorDeleteConfirm;
			selectedItem = 3;
			showLowerScreenSaveSelector(selectedItem, selectorMode, -1, targetSlot);
			continue;
		}
		if (selectorMode == kSaveSelectorDeleteConfirm) {
			if (selectedItem == 3 && targetSlot >= 0) {
				_engine->wipeSaveSlot(targetSlot);
				_3DSSaveSlotCacheValid = false;
				refresh3DSSaveSlotCache();
				if (_engine->_active3DSSaveSlot == targetSlot) {
					_engine->_active3DSSaveSlot = -1;
					_engine->_active3DSSaveName.clear();
				}
				selectedItem = targetSlot;
			} else {
				selectedItem = targetSlot >= 0 ? targetSlot : 4;
			}
			selectorMode = kSaveSelectorNormal;
			targetSlot = -1;
			showLowerScreenSaveSelector(selectedItem, selectorMode);
			continue;
		}

		// Normal slot activation: load an occupied profile or create a new one in
		// an empty slot. The player-entered name becomes the permanent profile name.
		if (selectorMode != kSaveSelectorNormal || selectedItem < 0 || selectedItem > 2) {
			continue;
		}
		const int selectedSlot = selectedItem;
		Common::String profileName;
		const bool occupied = getSlotInfo(selectedSlot, &profileName);
		_engine->_active3DSSaveSlot = selectedSlot;

		if (occupied) {
			_engine->_active3DSSaveName = profileName;
			// Do not animate the cached title image through another palette when a
			// profile is selected. Switch the upper LCD to an explicit black frame
			// first, then keep it black behind the loading ball until gameplay has
			// produced a complete destination frame.
			fadeLowerScreenToBlack();
			requestLowerScreenFadeIn();
			_engine->_screens->_flagPalettePcx = false;
			_engine->_frontVideoBuffer.fillRect(_engine->_frontVideoBuffer.getBounds(), COLOR_BLACK);
			_engine->_frontVideoBuffer.setPalette(_engine->_screens->_ptrPal.data(), 0, 256);
			_engine->_workVideoBuffer.fillRect(_engine->_workVideoBuffer.getBounds(), COLOR_BLACK);
			_engine->_workVideoBuffer.setPalette(_engine->_screens->_ptrPal.data(), 0, 256);
			_engine->setPalette(_engine->_screens->_ptrPal);
			_engine->_frontVideoBuffer.addDirtyRect(_engine->_frontVideoBuffer.getBounds());
			_engine->_frontVideoBuffer.forceUpdate();
			beginLowerScreenLoading();
			const Common::Error state = _engine->loadGameState(selectedSlot);
			endLowerScreenLoading();
			if (state.getCode() == Common::kNoError) {
				return EngineState::LoadedGame;
			}
			restoreUpperScreenLogo();
			showLowerScreenSaveSelector(selectedItem, selectorMode);
			continue;
		}

		_engine->_gameState->_endGameItems = false;
		if (_engine->_menuOptions->newGameMenu()) {
			_engine->_active3DSSaveName = _engine->_menuOptions->_saveGameName;
			showLowerScreenBlack();
			requestLowerScreenFadeIn();
			return EngineState::GameLoop;
		}
		_engine->_active3DSSaveName.clear();
		restoreUpperScreenLogo();
		_engine->_input->enableKeyMap(uiKeyMapId);
		_engine->_input->resetActionStates();
		_engine->_input->clearLowerScreenTap();
		showLowerScreenSaveSelector(selectedItem, selectorMode);
	}
}

void Menu::showLowerScreenIdlePanel() {
	_lowerScreenLastUpdate = 0;
	if (g_system->getOverlayWidth() != 320 || g_system->getOverlayHeight() != 240) {
		return;
	}

	const Graphics::PixelFormat overlayFormat = g_system->getOverlayFormat();
	if (_lowerScreenSurface.w != 320 || _lowerScreenSurface.h != 240) {
		_lowerScreenSurface.free();
		_lowerScreenSurface.create(320, 240, overlayFormat);
	}

	// Use artwork that already belongs to the user's LBA data instead of
	// shipping an extra image with LBA Classic 3DS. RESS.HQR entries 15/16 are the
	// first illustrated Twinsun intro screen and its 256-colour palette.
	Graphics::ManagedSurface &art = _engine->_imageBuffer;
	Graphics::Palette artPalette;
	const int32 artSize = HQR::getEntry((uint8 *)art.getPixels(), Resources::HQR_RESS_FILE, 15);
	const bool haveArt = artSize > 0 && HQR::getPaletteEntry(artPalette, Resources::HQR_RESS_FILE, 16);

	if (haveArt && art.w > 0 && art.h > 0) {
		// The original picture is 640x480 in LBA1. Scale it directly to the
		// native 320x240 lower screen without touching the upper framebuffer or
		// its palette, so opening the main menu cannot flash the top LCD.
		for (int y = 0; y < 240; ++y) {
			const int sy = y * art.h / 240;
			uint32 *dst = (uint32 *)_lowerScreenSurface.getBasePtr(0, y);
			for (int x = 0; x < 320; ++x) {
				const int sx = x * art.w / 320;
				const byte colorIndex = *(const byte *)art.getBasePtr(sx, sy);
				byte r, g, b;
				artPalette.get(colorIndex, r, g, b);
				dst[x] = overlayFormat.RGBToColor(r, g, b);
			}
		}
	} else {
		// Keep a harmless fallback for non-standard data sets where the classic
		// RESS image is absent.
		_lowerScreenSurface.fillRect(_lowerScreenSurface.getBounds(), overlayFormat.RGBToColor(8, 10, 20));
		drawLowerLbaText("LBA CLASSIC 3DS", 0, 101, 320,
		                 overlayFormat.RGBToColor(230, 232, 241), true);
	}

	if (!g_system->isOverlayVisible()) {
		g_system->showOverlay(true);
	}
	g_system->copyRectToOverlay(_lowerScreenSurface.getPixels(), _lowerScreenSurface.pitch, 0, 0, 320, 240);
}
#endif

Common::Rect Menu::calcItemRect(int32 left, int32 top, int32 item, int32 *centerX, int32 *centerY) const {
	const int32 itemWidth = SIZE_INV_OBJ_X;
	const int32 itemHeight = SIZE_INV_OBJ_Y;
	const int32 itemPadding = 11;
	const int32 itemWidthHalf = itemWidth / 2;
	const int32 itemHeightHalf = itemHeight / 2;
	const int32 itemX = (item / 4) * (itemWidth + itemPadding) + left + itemWidthHalf + itemPadding - 1;
	const int32 itemY = (item % 4) * (itemHeight + itemPadding) + top + itemHeightHalf + itemPadding - 1;
	if (centerX) {
		*centerX = itemX;
	}
	if (centerY) {
		*centerY = itemY;
	}
	return Common::Rect(itemX - itemWidthHalf, itemY - itemHeightHalf, itemX + itemWidthHalf, itemY + itemHeightHalf);
}

void Menu::drawOneInventory(int32 left, int32 top, int32 item) {
	int32 itemX, itemY;
	const Common::Rect rect = calcItemRect(left, top, item, &itemX, &itemY);
	const int32 color = _inventorySelectedItem == item ? _inventorySelectedColor : COLOR_BLACK;

	_engine->_interface->box(rect, color);

	if (item < NUM_INVENTORY_ITEMS && _engine->_gameState->hasItem((InventoryItems)item) && (!_engine->_gameState->inventoryDisabled() || item == InventoryItems::kiCloverLeaf)) {
		_itemAngle[item] += LBAAngles::ANGLE_2;
		_engine->_interface->setClip(rect);
		_engine->_renderer->draw3dObject(itemX, itemY, _engine->_resources->_inventoryTable[item], _itemAngle[item], 15000);
		_engine->_interface->unsetClip();
		if (item == InventoryItems::kGasItem) {
			_engine->_text->setFontColor(COLOR_WHITE);
			const Common::String &inventoryNumGas = Common::String::format("%d", _engine->_gameState->_inventoryNumGas);
			_engine->_text->drawText(rect.left + 3, rect.bottom - 32, inventoryNumGas.c_str());
		}
	}

	drawRectBorders(rect);
}

void Menu::drawListInventory(int32 left, int32 top) {
	constexpr int w = (SIZE_INV_OBJ_X + 10) * 7 + 10;
	constexpr int h = (SIZE_INV_OBJ_Y + 10) * 4 + 10;
	const Common::Rect rect(left, top, left + w, top + h);
	_engine->_interface->shadeBox(rect, 4);
	drawRectBorders(rect);
	for (int32 item = 0; item < NUM_INVENTORY_ITEMS; item++) {
		drawOneInventory(left, top, item);
	}
	_engine->_interface->unsetClip();
}

void Menu::inventory() {
#ifdef __3DS__
	if (g_system->getOverlayWidth() == 320 && g_system->getOverlayHeight() == 240) {
		inventory3DS();
		return;
	}
#endif
	int32 tmpAlphaLight = _engine->_scene->_alphaLight;
	int32 tmpBetaLight = _engine->_scene->_betaLight;

	_engine->saveFrontBuffer();

	_engine->_renderer->setLightVector(LBAAngles::ANGLE_315, LBAAngles::ANGLE_334, LBAAngles::ANGLE_0);

	_inventorySelectedColor = COLOR_SELECT_MENU;

	if (_engine->_gameState->_inventoryNumLeafs > 0) {
		_engine->_gameState->giveItem(InventoryItems::kiCloverLeaf);
	} else {
		_engine->_gameState->removeItem(InventoryItems::kiCloverLeaf);
	}

	const int32 left = _engine->width() / 2 - 303;
	const int32 top = _engine->height() / 2 - 230;
	drawListInventory(left, top);

	_engine->_text->initDial(TextBankId::Inventory_Intro_and_Holomap);

	_engine->_text->setFontCrossColor(COLOR_BRIGHT_BLUE);
	_engine->_text->initDialWindow();

	ProgressiveTextState dialstate = ProgressiveTextState::ContinueRunning;
	bool updateItemText = true;

	ScopedCursor scopedCursor(_engine);
	ScopedKeyMap scopedKeyMap(_engine, uiKeyMapId);
	for (;;) {
		FrameMarker frame(_engine, 66);
		_engine->readKeys();
		int32 prevSelectedItem = _inventorySelectedItem;

		if (_engine->_input->toggleAbortAction() || _engine->shouldQuit()) {
			break;
		}

		for (int32 item = 0; item < NUM_INVENTORY_ITEMS; item++) {
			const Common::Rect &rect = calcItemRect(left, top, item);
			if (_engine->_input->isMouseHovering(rect)) {
				_inventorySelectedItem = item;
				drawOneInventory(left, top, prevSelectedItem);
				updateItemText = true;
				break;
			}
		}

		const bool cursorDown = _engine->_input->toggleActionIfActive(TwinEActionType::UIDown);
		const bool cursorUp = _engine->_input->toggleActionIfActive(TwinEActionType::UIUp);
		const bool cursorLeft = _engine->_input->toggleActionIfActive(TwinEActionType::UILeft);
		const bool cursorRight = _engine->_input->toggleActionIfActive(TwinEActionType::UIRight);

		if (cursorDown) {
			_inventorySelectedItem++;
			if (_inventorySelectedItem >= NUM_INVENTORY_ITEMS) {
				_inventorySelectedItem = 0;
			}
			drawOneInventory(left, top, prevSelectedItem);
			updateItemText = true;
		} else if (cursorUp) {
			_inventorySelectedItem--;
			if (_inventorySelectedItem < 0) {
				_inventorySelectedItem = NUM_INVENTORY_ITEMS - 1;
			}
			drawOneInventory(left, top, prevSelectedItem);
			updateItemText = true;
		} else if (cursorLeft) {
			_inventorySelectedItem -= 4;
			if (_inventorySelectedItem < 0) {
				_inventorySelectedItem += NUM_INVENTORY_ITEMS;
			}
			drawOneInventory(left, top, prevSelectedItem);
			updateItemText = true;
		} else if (cursorRight) {
			_inventorySelectedItem += 4;
			if (_inventorySelectedItem >= NUM_INVENTORY_ITEMS) {
				_inventorySelectedItem -= NUM_INVENTORY_ITEMS;
			}
			drawOneInventory(left, top, prevSelectedItem);
			updateItemText = true;
		}

		if (updateItemText) {
			_engine->_text->secondInitDialWindow();
			if (_inventorySelectedItem < NUM_INVENTORY_ITEMS && _engine->_gameState->hasItem((InventoryItems)_inventorySelectedItem) && !_engine->_gameState->inventoryDisabled()) {
				_engine->_text->initInventoryText((InventoryItems)_inventorySelectedItem);
			} else {
				_engine->_text->initInventoryText(InventoryItems::MaxInventoryItems);
			}
			dialstate = ProgressiveTextState::End;
			updateItemText = false;
		}

		if (dialstate != ProgressiveTextState::NextPage) {
			dialstate = _engine->_text->nextDialChar();
		}

		if (_engine->_input->toggleActionIfActive(TwinEActionType::UINextPage)) {
			// restart the item description to appear from the beginning
			if (dialstate == ProgressiveTextState::End) {
				updateItemText = true;
			}
			if (dialstate == ProgressiveTextState::NextPage) {
				_engine->_text->secondInitDialWindow();
				dialstate = ProgressiveTextState::ContinueRunning;
			}
		}

		drawOneInventory(left, top, _inventorySelectedItem);

		if (_inventorySelectedItem < NUM_INVENTORY_ITEMS && _engine->_input->toggleActionIfActive(TwinEActionType::UIEnter) && _engine->_gameState->hasItem((InventoryItems)_inventorySelectedItem) && !_engine->_gameState->inventoryDisabled()) {
			_engine->_loopInventoryItem = _inventorySelectedItem;
			_inventorySelectedColor = COLOR_91;
			drawOneInventory(left, top, _inventorySelectedItem);
			break;
		}
	}

	_engine->_text->closeDial();

	_engine->_scene->_alphaLight = tmpAlphaLight;
	_engine->_scene->_betaLight = tmpBetaLight;

	_engine->_gameState->init3DGame();

	_engine->_text->initSceneTextBank();

	// this is a hack to 'fix' https://bugs.scummvm.org/ticket/13677
	_engine->_input->toggleActionIfActive(TwinEActionType::RecenterScreenOnTwinsen);
}

} // namespace TwinE

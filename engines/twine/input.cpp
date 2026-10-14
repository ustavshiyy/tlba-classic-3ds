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

#ifdef __3DS__
#include <3ds.h>
#endif
#include "twine/input.h"
#include "backends/keymapper/keymapper.h"
#include "common/events.h"
#include "common/system.h"
#include "twine/scene/actor.h"
#include "twine/text.h"
#include "twine/twine.h"

namespace TwinE {

const char *mainKeyMapId = "mainKeyMap";
const char *uiKeyMapId = "uiKeyMap";
const char *cutsceneKeyMapId = "cutsceneKeyMap";
const char *holomapKeyMapId = "holomapKeyMap";
const char *dialogKeyMapId = "dialogKeyMap";

ScopedKeyMap::ScopedKeyMap(TwinEEngine *engine, const char *id) : _engine(engine) {
	_changed = _engine->_input->enableAdditionalKeyMap(id, true);
	_keymap = id;
}

ScopedKeyMap::~ScopedKeyMap() {
	if (_changed) {
		_engine->_input->enableAdditionalKeyMap(_keymap.c_str(), false);
	}
}

Input::Input(TwinEEngine *engine) : _engine(engine) {
	resetLastHoveredMousePosition();
}

bool Input::isActionActive(TwinEActionType actionType, bool onlyFirstTime) const {
	if (onlyFirstTime) {
		return _actionStates[actionType] == 1;
	}
	return _actionStates[actionType] > 0;
}

bool Input::toggleActionIfActive(TwinEActionType actionType) {
	if (_actionStates[actionType] > 0) {
		_actionStates[actionType] = 0;
		return true;
	}
	return false;
}

void Input::resetActionStates() {
	for (int i = 0; i < TwinEActionType::Max; ++i) {
		_actionStates[i] = false;
	}
}

bool Input::toggleAbortAction() {
	bool abortState = false;
	abortState |= toggleActionIfActive(TwinEActionType::CutsceneAbort);
	abortState |= toggleActionIfActive(TwinEActionType::UIAbort);
	abortState |= toggleActionIfActive(TwinEActionType::Escape);
	abortState |= toggleActionIfActive(TwinEActionType::HolomapAbort);
	return abortState;
}

bool Input::isQuickBehaviourActionActive() const {
	return isActionActive(TwinEActionType::QuickBehaviourNormal) || isActionActive(TwinEActionType::QuickBehaviourAthletic) || isActionActive(TwinEActionType::QuickBehaviourAggressive) || isActionActive(TwinEActionType::QuickBehaviourDiscreet);
}

bool Input::isMoveOrTurnActionActive() const {
	return isActionActive(TwinEActionType::TurnLeft) || isActionActive(TwinEActionType::TurnRight) || isActionActive(TwinEActionType::MoveBackward) || isActionActive(TwinEActionType::MoveForward);
}

#ifdef __3DS__
bool Input::isCirclePadMoving() const {
	if (_currentKeyMap != mainKeyMapId) {
		return false;
	}
	// Radial 20% dead zone, including diagonal directions.
	const int32 deadZone = Common::JOYAXIS_MAX / 5;
	const int64 x = _circlePadX;
	const int64 y = _circlePadY;
	return x * x + y * y > (int64)deadZone * deadZone;
}

bool Input::consumeLowerScreenTap(Common::Point &point) {
	if (!_lowerScreenTapPending) {
		return false;
	}
	point = _lowerScreenTapPos;
	_lowerScreenTapPending = false;
	return true;
}

bool Input::consumeLowerScreenDrag(Common::Point &delta) {
	if (!_lowerScreenDragPending) {
		return false;
	}
	delta = _lowerScreenDragDelta;
	_lowerScreenDragDelta = Common::Point();
	_lowerScreenDragPending = false;
	return true;
}

void Input::clearLowerScreenTap() {
	_lowerScreenTapPending = false;
	_lowerScreenDragPending = false;
	_lowerScreenDragDelta = Common::Point();
	// Sync the edge detector with the current physical state so a finger that
	// is still resting on the screen cannot become a fresh tap in the next mode.
	_lowerScreenTouchWasDown = isLowerScreenTouchHeld();
	if (_lowerScreenTouchWasDown) {
		touchPosition touch;
		hidTouchRead(&touch);
		_lowerScreenTouchPos = Common::Point(touch.px, touch.py);
	}
}

bool Input::isLowerScreenTouchHeld() const {
	return (hidKeysHeld() & KEY_TOUCH) != 0;
}

bool Input::is3DSConfirmHeld() const {
	return (hidKeysHeld() & KEY_A) != 0;
}

bool Input::is3DSMagicBallHeld() const {
	return (hidKeysHeld() & KEY_R) != 0;
}

void Input::pollLowerScreenTouch() {
	// The platform event thread already calls hidScanInput(). Keep raw touch
	// state independent from dialogue/menu state: each lower-screen mode decides
	// what a tap or drag means. This avoids stale dialogue flags swallowing HUD
	// and inventory touches after a map/menu transition.
	const bool touchDown = isLowerScreenTouchHeld();
	if (touchDown) {
		touchPosition touch;
		hidTouchRead(&touch);
		const Common::Point pos(touch.px, touch.py);
		if (!_lowerScreenTouchWasDown) {
			_lowerScreenTouchPos = pos;
			_lowerScreenTapPos = pos;
			_lowerScreenTapPending = true;
			_lowerScreenDragPending = false;
			_lowerScreenDragDelta = Common::Point();
		} else {
			const Common::Point delta(pos.x - _lowerScreenTouchPos.x, pos.y - _lowerScreenTouchPos.y);
			_lowerScreenTouchPos = pos;
			if (ABS(delta.x) + ABS(delta.y) >= 2) {
				_lowerScreenDragDelta.x += delta.x;
				_lowerScreenDragDelta.y += delta.y;
				_lowerScreenDragPending = true;
				_lowerScreenTapPending = false;
			}
		}
	}
	_lowerScreenTouchWasDown = touchDown;
}
#endif

bool Input::isHeroActionActive() const {
	return isActionActive(TwinEActionType::ExecuteBehaviourAction);
}

bool Input::resetHeroActions() {
	return toggleActionIfActive(TwinEActionType::ExecuteBehaviourAction) || toggleActionIfActive(TwinEActionType::SpecialAction);
}

bool Input::enableAdditionalKeyMap(const char *id, bool enable) {
	Common::Keymapper *keymapper = g_system->getEventManager()->getKeymapper();
	Common::Keymap *keymap = keymapper->getKeymap(id);
	if (keymap == nullptr) {
		return false;
	}
	const bool changed = keymap->isEnabled() != enable;
	keymap->setEnabled(enable);
	return changed;
}

void Input::enableKeyMap(const char *id) {
	if (_currentKeyMap == id) {
		return;
	}

	Common::Keymapper *keymapper = g_system->getEventManager()->getKeymapper();
	const Common::KeymapArray &keymaps = keymapper->getKeymaps();
	for (Common::Keymap *keymap : keymaps) {
		const Common::String &keymapId = keymap->getId();
		if (keymapId == mainKeyMapId || keymapId == uiKeyMapId || keymapId == cutsceneKeyMapId || keymapId == holomapKeyMapId || keymapId == dialogKeyMapId) {
			keymap->setEnabled(keymapId == id);
		}
	}
	_currentKeyMap = id;
#ifdef __3DS__
	if (_currentKeyMap != mainKeyMapId) {
		_circlePadX = _circlePadY = 0;
	}
	// Sync the direct START edge detector across modal keymap transitions so
	// holding START while a menu closes cannot immediately reopen another menu.
	_startWasDown = (hidKeysHeld() & KEY_START) != 0;
#endif
	debugC(1, TwinE::kDebugInput, "enable keymap %s", id);
}

void Input::processCustomEngineEventStart(const Common::Event &event) {
	_actionStates[event.customType] = 1 + event.kbdRepeat;
	debugC(2, TwinE::kDebugInput, "twine custom event type start: %i", event.customType);
}

void Input::processCustomEngineEventEnd(const Common::Event &event) {
	_actionStates[event.customType] = 0;
	debugC(2, TwinE::kDebugInput, "twine custom event type end: %i", event.customType);
}

void Input::readKeys() {
	Common::Event event;
#ifdef __3DS__
	// Keep detailed timing of the backend queue without doing any file I/O here.
	// If another 1-2 second input hitch occurs, the session log can tell whether
	// one pollEvent() call blocked or a large burst of events consumed the frame.
	_lastPollEventCount = 0;
	_lastPollEventMs = 0;
	_lastPollEventMaxMs = 0;
	_lastPollEventHitLimit = false;

	static const int kMaxEventsPerFrame = 32;
	for (;;) {
		if (_lastPollEventCount >= kMaxEventsPerFrame) {
			_lastPollEventHitLimit = true;
			break;
		}

		const uint32 pollStart = g_system->getMillis();
		const bool haveEvent = g_system->getEventManager()->pollEvent(event);
		const uint32 pollMs = g_system->getMillis() - pollStart;
		_lastPollEventMs += pollMs;
		_lastPollEventMaxMs = MAX(_lastPollEventMaxMs, pollMs);

		if (!haveEvent) {
			break;
		}
		++_lastPollEventCount;
#else
	while (g_system->getEventManager()->pollEvent(event)) {
#endif
		switch (event.type) {
		case Common::EVENT_CUSTOM_ENGINE_ACTION_END:
			processCustomEngineEventEnd(event);
			break;
		case Common::EVENT_CUSTOM_ENGINE_ACTION_START:
			processCustomEngineEventStart(event);
			break;
#ifdef __3DS__
		case Common::EVENT_LBUTTONDOWN:
			// 3DS touch is read directly from HID after the event queue below.
			break;
#endif
		default:
			break;
		}
	}
#ifdef __3DS__
	// The 3DS event thread already scans HID. Read the latest physical state
	// directly; Circle Pad motion is intentionally not mirrored into the generic
	// ScummVM event queue in this standalone build.
	circlePosition pad;
	hidCircleRead(&pad);

	_circlePadX = (int16)CLIP<int32>(
	    (int32)pad.dx * Common::JOYAXIS_MAX / 160,
	    Common::JOYAXIS_MIN,
	    Common::JOYAXIS_MAX
	);

	_circlePadY = (int16)CLIP<int32>(
	    (int32)pad.dy * Common::JOYAXIS_MAX / 160,
	    Common::JOYAXIS_MIN,
	    Common::JOYAXIS_MAX
	);

	// START belongs to LBA, not to ScummVM.
	const bool startDown = (hidKeysHeld() & KEY_START) != 0;
	if (_currentKeyMap == mainKeyMapId && startDown && !_startWasDown) {
		_actionStates[TwinEActionType::OptionsMenu] = 1;
	}
	_startWasDown = startDown;

	pollLowerScreenTouch();
#endif
}

Common::Point Input::getMousePositions() const {
	return g_system->getEventManager()->getMousePos();
}

bool Input::isMouseHovering(const Common::Rect &rect, bool onlyIfMoved) {
	if (!_engine->_cfgfile.Mouse) {
		return false;
	}
	const Common::Point &point = getMousePositions();
	if (onlyIfMoved && _lastMousePos == point) {
		return false;
	}
	if (rect.contains(point)) {
		_lastMousePos = point;
		return true;
	}
	return false;
}

void Input::resetLastHoveredMousePosition() {
	_lastMousePos = Common::Point(-1, -1);
}

} // namespace TwinE

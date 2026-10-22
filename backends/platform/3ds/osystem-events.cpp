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

#define FORBIDDEN_SYMBOL_EXCEPTION_time_h

#include "backends/platform/3ds/osystem.h"

#include "backends/keymapper/action.h"
#include "backends/keymapper/keymapper-defaults.h"
#include "backends/keymapper/hardware-input.h"
#include "backends/keymapper/keymap.h"
#include "backends/keymapper/keymapper.h"
#include "backends/keymapper/standard-actions.h"
#include "backends/timer/default/default-timer.h"
#include "common/translation.h"
#include "engines/engine.h"
#include "gui/gui-manager.h"

namespace N3DS {

static Common::Mutex *eventMutex;
static InputMode inputMode = MODE_DRAG;
static InputMode savedInputMode = MODE_DRAG;
static aptHookCookie cookie;

static const Common::HardwareInputTableEntry ctrJoystickButtons[] = {
	{ "JOY_A",              Common::JOYSTICK_BUTTON_A,              _s("A")           },
	{ "JOY_B",              Common::JOYSTICK_BUTTON_B,              _s("B")           },
	{ "JOY_X",              Common::JOYSTICK_BUTTON_X,              _s("X")           },
	{ "JOY_Y",              Common::JOYSTICK_BUTTON_Y,              _s("Y")           },
	{ "JOY_BACK",           Common::JOYSTICK_BUTTON_BACK,           _s("Select")      },
	{ "JOY_START",          Common::JOYSTICK_BUTTON_START,          _s("Start")       },
	{ "JOY_LEFT_STICK",     Common::JOYSTICK_BUTTON_LEFT_STICK,     _s("ZL")          },
	{ "JOY_RIGHT_STICK",    Common::JOYSTICK_BUTTON_RIGHT_STICK,    _s("ZR")          },
	{ "JOY_LEFT_SHOULDER",  Common::JOYSTICK_BUTTON_LEFT_SHOULDER,  _s("L")           },
	{ "JOY_RIGHT_SHOULDER", Common::JOYSTICK_BUTTON_RIGHT_SHOULDER, _s("R")           },
	{ "JOY_UP",             Common::JOYSTICK_BUTTON_DPAD_UP,        _s("D-pad up")    },
	{ "JOY_DOWN",           Common::JOYSTICK_BUTTON_DPAD_DOWN,      _s("D-pad down")  },
	{ "JOY_LEFT",           Common::JOYSTICK_BUTTON_DPAD_LEFT,      _s("D-pad left")  },
	{ "JOY_RIGHT",          Common::JOYSTICK_BUTTON_DPAD_RIGHT,     _s("D-pad right") },
	{ nullptr,              0,                                      nullptr           }
};

static const Common::AxisTableEntry ctrJoystickAxes[] = {
	{ "JOY_LEFT_STICK_X", Common::JOYSTICK_AXIS_LEFT_STICK_X, Common::kAxisTypeFull, _s("Circle Pad X") },
	{ "JOY_LEFT_STICK_Y", Common::JOYSTICK_AXIS_LEFT_STICK_Y, Common::kAxisTypeFull, _s("Circle Pad Y") },
	{ nullptr,            0,                                  Common::kAxisTypeFull, nullptr       }
};

const Common::HardwareInputTableEntry ctrMouseButtons[] = {
	{ "MOUSE_LEFT",   Common::MOUSE_BUTTON_LEFT,   _s("Touch") },
	{ nullptr,        0,                           nullptr     }
};

static void pushEventQueue(Common::Queue<Common::Event> *queue, Common::Event &event) {
	Common::StackLock lock(*eventMutex);
	queue->push(event);
}

static void doJoyEvent(Common::Queue<Common::Event> *queue, u32 keysPressed, u32 keysReleased, u32 ctrKey, uint8 svmButton) {
	if (keysPressed & ctrKey || keysReleased & ctrKey) {
		Common::Event event;
		event.type = (keysPressed & ctrKey) ? Common::EVENT_JOYBUTTON_DOWN : Common::EVENT_JOYBUTTON_UP;
		event.joystick.button = svmButton;

		pushEventQueue(queue, event);
	}
}

static void eventThreadFunc(void *arg) {
	OSystem_3DS *osys = dynamic_cast<OSystem_3DS *>(g_system);
	Common::Queue<Common::Event> *eventQueue = (Common::Queue<Common::Event> *)arg;

	uint32 touchStartTime = osys->getMillis();
	touchPosition  lastTouch  = {0, 0};
	int borderSnapZone = 6;
	Common::Event event;

	while (!osys->exiting) {
		do {
			osys->delayMillis(10);
		} while (osys->sleeping && !osys->exiting);
		if (osys->exiting) {
			break;
		}

		hidScanInput();
		u32 held = hidKeysHeld();
		u32 keysPressed = hidKeysDown();
		u32 keysReleased = hidKeysUp();

		// Touch screen events
		if (held & KEY_TOUCH) {
			touchPosition touch;
			hidTouchRead(&touch);
			if (osys->_snapToBorder) {
				if (touch.px < borderSnapZone) {
					touch.px = 0;
				}
				if (touch.px > 319 - borderSnapZone) {
					touch.px = 319;
				}
				if (touch.py < borderSnapZone) {
					touch.py = 0;
				}
				if (touch.py > 239 - borderSnapZone) {
					touch.py = 239;
				}
			}

			osys->transformPoint(touch);

			event.mouse.x = touch.px;
			event.mouse.y = touch.py;

			if (keysPressed & KEY_TOUCH) {
				touchStartTime = osys->getMillis();
				if (inputMode == MODE_DRAG) {
					event.type = Common::EVENT_LBUTTONDOWN;
					pushEventQueue(eventQueue, event);
				}
			} else if (touch.px != lastTouch.px || touch.py != lastTouch.py) {
				event.type = Common::EVENT_MOUSEMOVE;
				pushEventQueue(eventQueue, event);
			}

			lastTouch = touch;
		} else if (keysReleased & KEY_TOUCH) {
			event.mouse.x = lastTouch.px;
			event.mouse.y = lastTouch.py;
			if (inputMode == MODE_DRAG) {
				event.type = Common::EVENT_LBUTTONUP;
				pushEventQueue(eventQueue, event);
			} else if (osys->getMillis() - touchStartTime < 200) {
				// Process click in MODE_HOVER
				event.type = Common::EVENT_MOUSEMOVE;
				pushEventQueue(eventQueue, event);
				event.type = Common::EVENT_LBUTTONDOWN;
				pushEventQueue(eventQueue, event);
				event.type = Common::EVENT_LBUTTONUP;
				pushEventQueue(eventQueue, event);
			}
		}

		// LBA Classic 3DS reads the Circle Pad directly from HID in TwinE.
		// Do not mirror it into ScummVM's generic joystick event queue: after a
		// long scene/script stall those 100 Hz axis events used to accumulate and
		// cause a second 1-2 second hitch while the queue was drained.

		// Button events
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_L,      Common::JOYSTICK_BUTTON_LEFT_SHOULDER);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_R,      Common::JOYSTICK_BUTTON_RIGHT_SHOULDER);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_A,      Common::JOYSTICK_BUTTON_A);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_B,      Common::JOYSTICK_BUTTON_B);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_X,      Common::JOYSTICK_BUTTON_X);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_Y,      Common::JOYSTICK_BUTTON_Y);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_DUP,    Common::JOYSTICK_BUTTON_DPAD_UP);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_DDOWN,  Common::JOYSTICK_BUTTON_DPAD_DOWN);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_DLEFT,  Common::JOYSTICK_BUTTON_DPAD_LEFT);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_DRIGHT, Common::JOYSTICK_BUTTON_DPAD_RIGHT);
		// START and SELECT are handled directly by TwinE and open LBA's own pause
		// menu. Keep both out of the generic keymapper: JOY_START would open the
		// ScummVM global menu and JOY_BACK would open the virtual keyboard.
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_ZL,     Common::JOYSTICK_BUTTON_LEFT_STICK);
		doJoyEvent(eventQueue, keysPressed, keysReleased, KEY_ZR,     Common::JOYSTICK_BUTTON_RIGHT_STICK);
	}
}

void setAudioSleeping(bool sleeping);
void requestAudioExit();
void shutdownAudioForExit();

// HOME -> X aggressively tears down worker stacks. Track the custom event/timer
// threads independently so the APT exit hook can join them before returning.
static Thread timerThreadForExit = 0;
static Thread eventThreadForExit = 0;

static void shutdownEventThreadsForExit() {
	if (timerThreadForExit) {
		threadJoin(timerThreadForExit, U64_MAX);
		threadFree(timerThreadForExit);
		timerThreadForExit = 0;
	}
	if (eventThreadForExit) {
		threadJoin(eventThreadForExit, U64_MAX);
		threadFree(eventThreadForExit);
		eventThreadForExit = 0;
	}
}

static void aptHookFunc(APT_HookType hookType, void *param) {
	OSystem_3DS *osys = dynamic_cast<OSystem_3DS *>(g_system);

	switch (hookType) {
		case APTHOOK_ONSUSPEND:
		case APTHOOK_ONSLEEP:
			if (g_engine) {
				osys->_sleepPauseToken = g_engine->pauseEngine();
			}
			setAudioSleeping(true);
			osys->sleeping = true;
			if (R_SUCCEEDED(gspLcdInit())) {
				GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);
				gspLcdExit();
			}
			break;
		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			if (g_engine) {
				osys->_sleepPauseToken.clear();
			}
			setAudioSleeping(false);
			osys->sleeping = false;
			osys->updateBacklight();
			osys->updateConfig();
			osys->refreshDisplayAfterResume();
			break;
		case APTHOOK_ONEXIT:
			// HOME -> X invalidates the audio worker stack before the normal backend
			// destructor gets a chance to join it. Finish just the audio worker here;
			// timer/event threads only receive the exit flag and are joined later by
			// destroyEvents(), avoiding the libctru GSP deadlock seen when all workers
			// were joined from inside the APT callback.
			osys->exiting = true;
			shutdownAudioForExit();
			break;
		default:
			warning("Unhandled APT hook, type: %d", hookType);
	}
}

static void timerThreadFunc(void *arg) {
	OSystem_3DS *osys = (OSystem_3DS *)arg;
	DefaultTimerManager *tm = (DefaultTimerManager *)osys->getTimerManager();
	while (!osys->exiting) {
		g_system->delayMillis(10);
		if (osys->exiting) {
			break;
		}
		tm->handler();
	}
}

Common::HardwareInputSet *OSystem_3DS::getHardwareInputSet() {
	using namespace Common;

	CompositeHardwareInputSet *inputSet = new CompositeHardwareInputSet();
	// Touch input sends mouse events for now, so we need to declare we have a mouse...
	inputSet->addHardwareInputSet(new MouseHardwareInputSet(ctrMouseButtons));
	inputSet->addHardwareInputSet(new JoystickHardwareInputSet(ctrJoystickButtons, ctrJoystickAxes));

	return inputSet;
}

void OSystem_3DS::initEvents() {
	eventMutex = new Common::Mutex();
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	_timerThread = threadCreate(&timerThreadFunc, this, 32 * 1024, prio - 1, -2, false);
	timerThreadForExit = _timerThread;
	_eventThread = threadCreate(&eventThreadFunc, &_eventQueue, 32 * 1024, prio - 1, -2, false);
	eventThreadForExit = _eventThread;

	aptHook(&cookie, aptHookFunc, this);
	_eventManager->getEventDispatcher()->registerObserver(this, 10, false);
}

void OSystem_3DS::destroyEvents() {
	_eventManager->getEventDispatcher()->unregisterObserver(this);
	aptUnhook(&cookie);

	shutdownEventThreadsForExit();
	delete eventMutex;
}

void OSystem_3DS::transformPoint(touchPosition &point) {
	if (!_overlayInGUI) {
		point.px = static_cast<float>(point.px) / _gameBottomTexture.getScaleX() - _gameBottomTexture.getPosX();
		point.py = static_cast<float>(point.py) / _gameBottomTexture.getScaleY() - _gameBottomTexture.getPosY();
	}

	clipPoint(point);
}

void OSystem_3DS::clipPoint(touchPosition &point) {
	if (_overlayInGUI) {
		point.px = CLIP<uint16>(point.px, 0, getOverlayWidth()  - 1);
		point.py = CLIP<uint16>(point.py, 0, getOverlayHeight() - 1);
	} else {
		point.px = CLIP<uint16>(point.px, 0, _gameTopTexture.actualWidth  - 1);
		point.py = CLIP<uint16>(point.py, 0, _gameTopTexture.actualHeight - 1);
	}
}

enum _3DSCustomEvent {
	k3DSEventToggleDragMode,
	k3DSEventToggleMagnifyMode
};

Common::KeymapArray OSystem_3DS::getGlobalKeymaps() {
	using namespace Common;

	Keymap *keymap = new Keymap(Keymap::kKeymapTypeGlobal, "3ds", "3DS");

	Action *act;

	act = new Action("DRAGM", _("Toggle Drag Mode"));
	act->setCustomBackendActionEvent(k3DSEventToggleDragMode);
	act->addDefaultInputMapping("JOY_RIGHT_SHOULDER");
	keymap->addAction(act);

	act = new Action("MAGM", _("Toggle Magnify Mode"));
	act->setCustomBackendActionEvent(k3DSEventToggleMagnifyMode);
	act->addDefaultInputMapping("JOY_LEFT_SHOULDER");
	keymap->addAction(act);

	return Keymap::arrayOf(keymap);
}

Common::KeymapperDefaultBindings *OSystem_3DS::getKeymapperDefaultBindings() {
	Common::KeymapperDefaultBindings *keymapperDefaultBindings = new Common::KeymapperDefaultBindings();

	// Unmap the main menu standard action so LEFT_SHOULDER can be used for drag mode
	keymapperDefaultBindings->setDefaultBinding("engine-default", Common::kStandardActionOpenMainMenu, "");

	return keymapperDefaultBindings;
}

bool OSystem_3DS::pollEvent(Common::Event &event) {
	if (!aptMainLoop()) {
		// HOME -> X can make the homebrew process close immediately after
		// aptMainLoop() reports the close request. Stop every worker while their
		// stacks are still mapped instead of waiting for the backend destructor.
		// The crash dumps from this path fault on a write immediately below SP,
		// which is consistent with a worker still running on a torn-down stack.
		if (_sleepPauseToken.isActive()) {
			_sleepPauseToken.clear();
		}
		exiting = true;
		setAudioSleeping(false);
		shutdownAudioForExit();
		shutdownEventThreadsForExit();

		event.type = Common::EVENT_QUIT;
		return true;
	}

	// If magnify mode is on when returning to Launcher, turn it off
	if (_eventManager->shouldReturnToLauncher()) {
		if (_magnifyMode == MODE_MAGON) {
			_magnifyMode = MODE_MAGOFF;
			updateSize();
			if (savedInputMode == MODE_DRAG) {
				inputMode = savedInputMode;
				displayMessageOnOSD(_("Magnify Mode Off. Reactivating Drag Mode.\nReturning to Launcher..."));
			} else {
				displayMessageOnOSD(_("Magnify Mode Off. Returning to Launcher..."));
			}
		}
	}

	Common::StackLock lock(*eventMutex);

	if (_eventQueue.empty()) {
		return false;
	}

	event = _eventQueue.pop();

	if (Common::isMouseEvent(event)) {
		warpMouse(event.mouse.x, event.mouse.y);
	}

	return true;
}

bool OSystem_3DS::notifyEvent(const Common::Event &event) {
	if (event.type != Common::EVENT_CUSTOM_BACKEND_ACTION_START
	        && event.type != Common::EVENT_CUSTOM_BACKEND_ACTION_END) {
		return false; // We're only interested in custom backend events
	}

	if (event.type == Common::EVENT_CUSTOM_BACKEND_ACTION_END) {
		return true; // We'll say we have handled the event so it is not propagated
	}

	switch ((_3DSCustomEvent)event.customType) {
	case k3DSEventToggleDragMode:
		if (inputMode == MODE_DRAG) {
			inputMode = savedInputMode = MODE_HOVER;
			displayMessageOnOSD(_("Hover Mode"));
		} else {
			if (_magnifyMode == MODE_MAGOFF) {
				inputMode = savedInputMode = MODE_DRAG;
				displayMessageOnOSD(_("Drag Mode"));
			} else {
				displayMessageOnOSD(_("Cannot Switch to Drag Mode while Magnify Mode is On"));
			}
		}
		return true;

	case k3DSEventToggleMagnifyMode:
		if (_overlayVisible) {
			displayMessageOnOSD(_("Magnify Mode cannot be activated in menus."));
		} else if (_screen != kScreenBoth && _magnifyMode == MODE_MAGOFF) {
			// TODO: Automatically enable both screens while magnify mode is on
			displayMessageOnOSD(_("Magnify Mode can only be activated\n when both screens are enabled."));
		} else if (_gameWidth <= 400 && _gameHeight <= 240) {
			displayMessageOnOSD(_("In-game resolution too small to magnify."));
		} else {
			if (_magnifyMode == MODE_MAGOFF) {
				_magnifyMode = MODE_MAGON;
				if (inputMode == MODE_DRAG) {
					inputMode = MODE_HOVER;
					displayMessageOnOSD(_("Magnify Mode On. Switching to Hover Mode..."));
				} else {
					displayMessageOnOSD(_("Magnify Mode On"));
				}
			} else {
				_magnifyMode = MODE_MAGOFF;
				updateSize();
				if (savedInputMode == MODE_DRAG) {
					inputMode = savedInputMode;
					displayMessageOnOSD(_("Magnify Mode Off. Reactivating Drag Mode..."));
				} else {
					displayMessageOnOSD(_("Magnify Mode Off"));
				}
			}
		}
		return true;
	}

	return false;
}

} // namespace N3DS

#include "graphics/presentation/window/hostInput.h"

#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_mouse.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "common/assert.h"
#include "common/emulatorConfig.h"
#include "common/logging/log.h"
#include "audio-probe.h"
#include "frame-gen.h"
#include "libs/controller.h"

#include <atomic>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <string_view>

namespace Libs::Graphics {

namespace {

struct ControlInfo {
	std::string_view name;
	uint32_t         button   = 0;
	Controller::Axis axis     = Controller::Axis::AxisMax;
	bool             positive = false;
	float            touch_x  = 0.0f;
};

static constexpr std::array CONTROL_INFO = {
    ControlInfo {"L3", Controller::PAD_BUTTON_L3},
    ControlInfo {"R3", Controller::PAD_BUTTON_R3},
    ControlInfo {"Options", Controller::PAD_BUTTON_OPTIONS},
    ControlInfo {"Up", Controller::PAD_BUTTON_UP},
    ControlInfo {"Right", Controller::PAD_BUTTON_RIGHT},
    ControlInfo {"Down", Controller::PAD_BUTTON_DOWN},
    ControlInfo {"Left", Controller::PAD_BUTTON_LEFT},
    ControlInfo {"L2", Controller::PAD_BUTTON_L2},
    ControlInfo {"R2", Controller::PAD_BUTTON_R2},
    ControlInfo {"L1", Controller::PAD_BUTTON_L1},
    ControlInfo {"R1", Controller::PAD_BUTTON_R1},
    ControlInfo {"Triangle", Controller::PAD_BUTTON_TRIANGLE},
    ControlInfo {"Circle", Controller::PAD_BUTTON_CIRCLE},
    ControlInfo {"Cross", Controller::PAD_BUTTON_CROSS},
    ControlInfo {"Square", Controller::PAD_BUTTON_SQUARE},
    ControlInfo {"TouchPad", Controller::PAD_BUTTON_TOUCH_PAD, Controller::Axis::AxisMax, false,
                 0.25f},
    ControlInfo {"TouchPadRight", Controller::PAD_BUTTON_TOUCH_PAD, Controller::Axis::AxisMax,
                 false, 0.75f},
    ControlInfo {"LeftStickLeft", 0, Controller::Axis::LeftX, false},
    ControlInfo {"LeftStickRight", 0, Controller::Axis::LeftX, true},
    ControlInfo {"LeftStickUp", 0, Controller::Axis::LeftY, false},
    ControlInfo {"LeftStickDown", 0, Controller::Axis::LeftY, true},
    ControlInfo {"RightStickLeft", 0, Controller::Axis::RightX, false},
    ControlInfo {"RightStickRight", 0, Controller::Axis::RightX, true},
    ControlInfo {"RightStickUp", 0, Controller::Axis::RightY, false},
    ControlInfo {"RightStickDown", 0, Controller::Axis::RightY, true},
};

constexpr std::size_t INVALID_CONTROL = CONTROL_INFO.size();

struct Binding {
	SDL_Keycode key          = SDLK_UNKNOWN;
	uint8_t     mouse_button = 0;
	std::size_t control      = INVALID_CONTROL;
};

constexpr int              MOUSE_POLL_INTERVAL_MS = 33;
constexpr std::string_view MOUSE_SENSITIVITY      = "MouseSensitivity=";

struct MouseJoystickState {
	bool     enabled   = false;
	bool     output    = false;
	uint64_t next_poll = 0;
};

MouseJoystickState g_mouse;

std::size_t ControlFromName(std::string_view name) {
	const auto info = std::find_if(CONTROL_INFO.begin(), CONTROL_INFO.end(),
	                               [name](const auto& item) { return item.name == name; });
	return static_cast<std::size_t>(std::distance(CONTROL_INFO.begin(), info));
}

SDL_Keycode NormalizeKey(SDL_Keycode key) {
	switch (key) {
		case SDLK_RSHIFT: return SDLK_LSHIFT;
		case SDLK_RCTRL: return SDLK_LCTRL;
		case SDLK_RALT: return SDLK_LALT;
		case SDLK_RGUI: return SDLK_LGUI;
		default: return key;
	}
}

uint8_t MouseButtonFromName(std::string_view name) {
	static constexpr std::array buttons = {
	    std::pair {std::string_view("Mouse:Left"), uint8_t {SDL_BUTTON_LEFT}},
	    std::pair {std::string_view("Mouse:Middle"), uint8_t {SDL_BUTTON_MIDDLE}},
	    std::pair {std::string_view("Mouse:Right"), uint8_t {SDL_BUTTON_RIGHT}},
	    std::pair {std::string_view("Mouse:X1"), uint8_t {SDL_BUTTON_X1}},
	    std::pair {std::string_view("Mouse:X2"), uint8_t {SDL_BUTTON_X2}},
	};

	const auto button = std::find_if(buttons.begin(), buttons.end(),
	                                 [name](const auto& item) { return item.first == name; });
	return button != buttons.end() ? button->second : 0;
}

bool Conflicts(const Binding& first, const Binding& second) {
	return first.control == second.control ||
	       (first.key != SDLK_UNKNOWN && first.key == second.key) ||
	       (first.mouse_button != 0 && first.mouse_button == second.mouse_button);
}

class InputMap {
public:
	InputMap() {
		for (const auto& value: Config::GetKeymap()) {
			const std::string_view entry = value;
			if (entry.starts_with(MOUSE_SENSITIVITY)) {
				const float sensitivity =
				    std::strtof(value.c_str() + MOUSE_SENSITIVITY.size(), nullptr);
				m_mouse_sensitivity =
				    std::clamp(std::isfinite(sensitivity) ? sensitivity : 1.0f, 0.1f, 5.0f);
				continue;
			}
			const auto split = entry.find('=');

			Binding binding;
			if (split != std::string_view::npos) {
				binding.control       = ControlFromName(entry.substr(0, split));
				const auto host_input = entry.substr(split + 1);
				binding.mouse_button  = MouseButtonFromName(host_input);
				if (binding.mouse_button == 0) {
					binding.key = NormalizeKey(SDL_GetKeyFromName(std::string(host_input).c_str()));
				}
			}

			const bool reserved = binding.key == SDLK_ESCAPE || binding.key == SDLK_PAUSE ||
			                      binding.key == SDLK_F1 || binding.key == SDLK_F7 ||
			                      binding.key == SDLK_F9 || binding.key == SDLK_F11;
			if (binding.control == INVALID_CONTROL || reserved ||
			    (binding.key == SDLK_UNKNOWN && binding.mouse_button == 0)) {
				EXIT("Invalid input mapping: %s\n", value.c_str());
			}
			Add(binding);
		}
	}
	[[nodiscard]] bool  Custom() const { return m_size != 0; }
	[[nodiscard]] float MouseSensitivity() const { return m_mouse_sensitivity; }

	[[nodiscard]] std::size_t FindKey(int key_code) const {
		key_code = NormalizeKey(static_cast<SDL_Keycode>(key_code));
		const auto binding =
		    std::find_if(m_bindings.begin(), m_bindings.begin() + m_size,
		                 [key_code](const auto& item) { return item.key == key_code; });
		return binding != m_bindings.begin() + m_size ? binding->control : INVALID_CONTROL;
	}

	[[nodiscard]] std::size_t FindMouseButton(uint8_t mouse_button) const {
		const auto binding = std::find_if(
		    m_bindings.begin(), m_bindings.begin() + m_size,
		    [mouse_button](const auto& item) { return item.mouse_button == mouse_button; });
		return binding != m_bindings.begin() + m_size ? binding->control : INVALID_CONTROL;
	}

private:
	void Add(const Binding& binding) {
		for (std::size_t index = 0; index < m_size;) {
			if (Conflicts(m_bindings[index], binding)) {
				m_bindings[index] = m_bindings[--m_size];
			} else {
				index++;
			}
		}
		EXIT_IF(m_size >= m_bindings.size());
		m_bindings[m_size++] = binding;
	}

	std::array<Binding, CONTROL_INFO.size()> m_bindings {};
	std::size_t                              m_size              = 0;
	float                                    m_mouse_sensitivity = 1.0f;
};

const InputMap& GetInputMap() {
	static const InputMap map;
	return map;
}

void SetButton(uint32_t button, bool down) {
	if (button == Controller::PAD_BUTTON_L2) {
		Controller::SetAxis(Controller::HOST_INPUT_CONTROLLER_ID, Controller::Axis::TriggerLeft,
		                    down ? 255 : 0);
	} else if (button == Controller::PAD_BUTTON_R2) {
		Controller::SetAxis(Controller::HOST_INPUT_CONTROLLER_ID, Controller::Axis::TriggerRight,
		                    down ? 255 : 0);
	} else if (button != 0) {
		Controller::SetButton(Controller::HOST_INPUT_CONTROLLER_ID, button, down);
	}
}

void SetTouchPad(float x, bool down) {
	Controller::SetTouchPad(Controller::HOST_INPUT_CONTROLLER_ID, 0, down, x, 0.5f);
}

uint32_t DefaultKeyboardButton(int key_code) {
	switch (NormalizeKey(static_cast<SDL_Keycode>(key_code))) {
		case SDLK_UP: return Controller::PAD_BUTTON_UP;
		case SDLK_LEFT: return Controller::PAD_BUTTON_LEFT;
		case SDLK_DOWN: return Controller::PAD_BUTTON_DOWN;
		case SDLK_RIGHT: return Controller::PAD_BUTTON_RIGHT;
		case SDLK_j: return Controller::PAD_BUTTON_CROSS;
		case SDLK_i: return Controller::PAD_BUTTON_TRIANGLE;
		case SDLK_k: return Controller::PAD_BUTTON_SQUARE;
		case SDLK_l: return Controller::PAD_BUTTON_CIRCLE;
		case SDLK_q: return Controller::PAD_BUTTON_L1;
		case SDLK_e: return Controller::PAD_BUTTON_R1;
		case SDLK_LSHIFT: return Controller::PAD_BUTTON_L3;
		case SDLK_LCTRL: return Controller::PAD_BUTTON_R3;
		case SDLK_RETURN:
		case SDLK_RETURN2: return Controller::PAD_BUTTON_OPTIONS;
		default: return 0;
	}
}

struct StickKeys {
	bool left  = false;
	bool right = false;
	bool up    = false;
	bool down  = false;
};

void SetStickAxis(Controller::Axis axis, bool negative, bool positive) {
	int value = 128;
	if (negative && !positive) {
		value = 0;
	} else if (positive && !negative) {
		value = 255;
	}
	Controller::SetAxis(Controller::HOST_INPUT_CONTROLLER_ID, axis, value);
}

void SetControl(std::size_t control, bool down) {
	if (control == INVALID_CONTROL) {
		return;
	}

	const auto& info = CONTROL_INFO[control];
	if (info.button == Controller::PAD_BUTTON_TOUCH_PAD) {
		SetTouchPad(info.touch_x, down);
		return;
	}
	if (info.button != 0) {
		SetButton(info.button, down);
		return;
	}

	struct AxisKeys {
		bool negative = false;
		bool positive = false;
	};
	static std::array<AxisKeys, 4> axes;

	const auto axis = static_cast<std::size_t>(info.axis);
	EXIT_IF(axis >= axes.size());
	if (info.positive) {
		axes[axis].positive = down;
	} else {
		axes[axis].negative = down;
	}
	SetStickAxis(info.axis, axes[axis].negative, axes[axis].positive);
}

void DefaultKeyboardInput(int key_code, bool down) {
	static StickKeys left;
	static StickKeys right;

	switch (NormalizeKey(static_cast<SDL_Keycode>(key_code))) {
		case SDLK_BACKSPACE: SetTouchPad(0.25f, down); return;
		case SDLK_TAB: SetTouchPad(0.75f, down); return;
		case SDLK_a:
			left.left = down;
			SetStickAxis(Controller::Axis::LeftX, left.left, left.right);
			return;
		case SDLK_d:
			left.right = down;
			SetStickAxis(Controller::Axis::LeftX, left.left, left.right);
			return;
		case SDLK_w:
			left.up = down;
			SetStickAxis(Controller::Axis::LeftY, left.up, left.down);
			return;
		case SDLK_s:
			left.down = down;
			SetStickAxis(Controller::Axis::LeftY, left.up, left.down);
			return;
		case SDLK_f:
			right.left = down;
			SetStickAxis(Controller::Axis::RightX, right.left, right.right);
			return;
		case SDLK_h:
			right.right = down;
			SetStickAxis(Controller::Axis::RightX, right.left, right.right);
			return;
		case SDLK_t:
			right.up = down;
			SetStickAxis(Controller::Axis::RightY, right.up, right.down);
			return;
		case SDLK_g:
			right.down = down;
			SetStickAxis(Controller::Axis::RightY, right.up, right.down);
			return;
		default: SetButton(DefaultKeyboardButton(key_code), down); return;
	}
}

void MouseToJoystick(int delta_x, int delta_y) {
	const double distance = std::hypot(delta_x, delta_y);
	const double scale =
	    std::clamp(distance * GetInputMap().MouseSensitivity() + 16.0, 64.0, 128.0) / distance;
	const auto map_axis = [scale](int delta) {
		return std::clamp(128 + static_cast<int>(std::lround(delta * scale)), 0, 255);
	};
	Controller::SetRightStick(Controller::HOST_INPUT_CONTROLLER_ID, map_axis(delta_x),
	                          map_axis(delta_y));
}

void CenterMouseStick() {
	if (!g_mouse.output) {
		return;
	}
	Controller::SetRightStick(Controller::HOST_INPUT_CONTROLLER_ID, 128, 128);
	g_mouse.output = false;
}

bool SetRelativeMouseMode(bool enabled) {
	if (SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) == 0) {
		return true;
	}
	LOGF("Mouse-to-joystick relative mode failed: %s\n", SDL_GetError());
	return false;
}
} // namespace

// KYTY_INPUT_SCRIPT="<start s>:<every s>:<for s>:<control>[:<hold ms>][;...]": synthetic pad
// input for unattended runs (keyboard injection needs window focus, which Windows denies after
// the user has been idle). From <start> for <for> seconds, press <control> (a DualSense control
// name: Cross, Options, LeftStickUp, ...) for <hold> ms (default 150) every <every> seconds.
// "20:3:180:Cross" walks the title screens; "240:12:120:LeftStickUp:5000" walks forward.
// KYTY_INPUT_SCRIPT_START=<file>: a step whose <start> begins with '@' counts from the moment that
// file appears (the harness creates it once the level has loaded and settled) instead of from
// launch, and every step without '@' ends at that moment (no more menu presses in the level).
// KYTY_INPUT_SCRIPT_STOP=<file>: the steps without '@' also end when this file appears (the
// harness creates it as soon as the level loads, before it measures and starts the walk).
// A step whose hold is followed by ":fog" (e.g. "20:3:900:Cross:150:fog") ends by itself: once
// the game has run its title (25+ fps for 2 s after 20 s) the step presses eight more times at
// most (intro, title, Continue, and slack) and stops, or earlier when a loading screen follows
// (15 fps or less for 3 s). The presses stop before the level is up, whatever a build's start-up
// takes and however short its loading screen is.
// "20:3:600:Cross;@0:12:120:LeftStickUp:5000" presses Cross until the level is up, then walks.
std::atomic<uint64_t> g_presented_frames {0};
void HostInputNotePresentedFrame() {
	g_presented_frames.fetch_add(1, std::memory_order_relaxed);
}

static std::size_t ControlIndex(std::string_view name) {
	for (std::size_t i = 0; i < CONTROL_INFO.size(); i++) {
		if (CONTROL_INFO[i].name == name) return i;
	}
	return CONTROL_INFO.size();
}

static void StartScriptedInput() {
	const char* script = std::getenv("KYTY_INPUT_SCRIPT");
	if (script == nullptr || script[0] == 0) {
		return;
	}
	struct Step {
		double      start = 0, every = 3, duration = 0;
		std::size_t control = 0;
		uint32_t    hold_ms = 150;
		bool        from_start_file = false; // '@': the clock starts when the start file appears
		bool        until_fog       = false; // ":fog": ends once the title ran and a loading screen followed
	};
	const char*       start_env = std::getenv("KYTY_INPUT_SCRIPT_START");
	const std::string start_file(start_env != nullptr ? start_env : "");
	const char*       stop_env = std::getenv("KYTY_INPUT_SCRIPT_STOP");
	const std::string stop_file(stop_env != nullptr ? stop_env : "");
	std::vector<Step> steps;
	std::string       text(script);
	size_t            pos = 0;
	while (pos < text.size()) {
		const auto end  = text.find(';', pos);
		auto       item = text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
		pos             = end == std::string::npos ? text.size() : end + 1;
		Step   step;
		if (!item.empty() && item[0] == '@') {
			step.from_start_file = true;
			item.erase(0, 1);
		}
		size_t a = item.find(':'), b = item.find(':', a + 1), c = item.find(':', b + 1);
		if (a == std::string::npos || b == std::string::npos || c == std::string::npos) continue;
		const size_t e    = item.find(':', c + 1);
		step.start        = std::atof(item.substr(0, a).c_str());
		step.every        = std::max(0.2, std::atof(item.substr(a + 1, b - a - 1).c_str()));
		step.duration     = std::atof(item.substr(b + 1, c - b - 1).c_str());
		const auto name   = item.substr(c + 1, e == std::string::npos ? std::string::npos : e - c - 1);
		step.control      = ControlIndex(name);
		if (e != std::string::npos) {
			auto rest = item.substr(e + 1);
			if (const auto f = rest.find(":fog"); f != std::string::npos) {
				step.until_fog = true;
				rest.erase(f);
			}
			if (!rest.empty()) step.hold_ms = static_cast<uint32_t>(std::max(1, std::atoi(rest.c_str())));
		}
		if (step.control < CONTROL_INFO.size()) steps.push_back(step);
	}
	if (steps.empty()) {
		return;
	}
	std::thread([steps, start_file, stop_file] {
		const auto t0 = std::chrono::steady_clock::now();
		std::vector<double> next(steps.size());
		for (size_t i = 0; i < steps.size(); i++) next[i] = steps[i].start;
		bool   started = false; // the start file has appeared
		bool   stopped = false; // the stop file has appeared
		bool   fogged  = false; // the title ran, then a loading screen (":fog")
		double t_start = 0;     // when the start file appeared
		int    polls   = 0;
		// Frame-rate watch for ":fog": presented frames over the last second.
		uint64_t frames_before = g_presented_frames.load(std::memory_order_relaxed);
		double   t_rate = 0, title_s = 0, fog_s = 0, level_s = 0;
		bool     title_seen = false;
		// The level: the G-buffer vertex shaders drawn every second for 5 s (3D gameplay; the loading
		// screen's frame rate no longer tells it from the level since the warm-up made loads fast).
		uint64_t gameplay_before = FrameGen::g_gameplay_draws.load(std::memory_order_relaxed);
		static const bool trace = [] {
			const char* value = std::getenv("KYTY_INPUT_SCRIPT_TRACE");
			return value != nullptr && value[0] == '1';
		}();
		int      presses_after_title = 0;
		const auto seen = [](const std::string& path) {
			if (path.empty()) return false;
			FILE* f = std::fopen(path.c_str(), "rb");
			if (f == nullptr) return false;
			std::fclose(f);
			std::remove(path.c_str());
			return true;
		};
		for (;;) {
			const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
			if (t - t_rate >= 1.0) {
				const auto frames_now = g_presented_frames.load(std::memory_order_relaxed);
				const auto fps        = static_cast<double>(frames_now - frames_before) / (t - t_rate);
				frames_before         = frames_now;
				t_rate                = t;
				title_s               = (fps >= 25 && t >= 20) ? title_s + 1 : 0; // a title with a warm-up behind it
				if (title_s >= 2) title_seen = true;
				fog_s = (title_seen && fps <= 15 && t >= 40) ? fog_s + 1 : 0;
				if (!fogged && title_seen && fog_s >= 3) {
					fogged = true;
					std::printf("HostInput: scripted input saw the loading screen at %.0f s\n", t);
					std::fflush(stdout);
				}
				const auto gameplay_now = FrameGen::g_gameplay_draws.load(std::memory_order_relaxed);
				const auto gameplay     = gameplay_now - gameplay_before;
				gameplay_before         = gameplay_now;
				level_s                 = (gameplay >= 20 && fps >= 10) ? level_s + 1 : 0;
				if (!fogged && title_seen && level_s >= 5) {
					fogged = true;
					std::printf("HostInput: scripted input saw the level at %.0f s\n", t);
					std::fflush(stdout);
				}
				if (trace) {
					std::printf("HostInput: t=%.0f fps=%.0f gameplay_draws=%llu title=%d fog=%d\n", t, fps,
					            static_cast<unsigned long long>(gameplay), title_seen ? 1 : 0, fogged ? 1 : 0);
					std::fflush(stdout);
				}
			}
			if ((++polls % 5) == 0) {
				if (!started && seen(start_file)) {
					started = true;
					t_start = t;
					std::printf("HostInput: scripted input start file seen at %.0f s\n", t);
					std::fflush(stdout);
				}
				if (!stopped && seen(stop_file)) {
					stopped = true;
					std::printf("HostInput: scripted input stop file seen at %.0f s\n", t);
					std::fflush(stdout);
				}
			}
			bool pending = false;
			for (size_t i = 0; i < steps.size(); i++) {
				const auto& s = steps[i];
				if (s.from_start_file) {
					if (!start_file.empty() && !started) {
						pending = true; // waiting for the level
						continue;
					}
					const double ts = t - t_start;
					if (ts > s.start + s.duration) continue;
					pending = true;
					if (ts >= next[i]) {
						AudioProbe::NotePress(CONTROL_INFO[s.control].name);
						SetControl(s.control, true);
						SDL_Delay(s.hold_ms);
						SetControl(s.control, false);
						next[i] = ts + s.every + s.hold_ms / 1000.0;
					}
					continue;
				}
				if (started || stopped || (s.until_fog && (fogged || presses_after_title >= 200)) ||
				    t > s.start + s.duration)
					continue;
				pending = true;
				if (t >= next[i]) {
					SetControl(s.control, true);
					SDL_Delay(s.hold_ms);
					SetControl(s.control, false);
					next[i] = t + s.every + s.hold_ms / 1000.0;
					if (s.until_fog && title_seen && ++presses_after_title >= 200) {
						std::printf("HostInput: scripted menu presses done at %.0f s\n", t);
						std::fflush(stdout);
					}
				}
			}
			if (!pending) return;
			SDL_Delay(100);
		}
	}).detach();
	LOGF("HostInput: scripted input active (%zu step(s))\n", steps.size());
}

void HostInputInit() {
	StartScriptedInput();
	GetInputMap();
}

void HostInputKey(int key_code, bool down) {
	const auto& map = GetInputMap();
	if (map.Custom()) {
		SetControl(map.FindKey(key_code), down);
	} else {
		DefaultKeyboardInput(key_code, down);
	}
}

void HostInputMouseButton(uint8_t mouse_button, bool down) {
	const auto& map = GetInputMap();
	if (map.Custom() && mouse_button != 0) {
		SetControl(map.FindMouseButton(mouse_button), down);
	}
}

void HostInputToggleMouseToJoystick() {
	if (g_mouse.enabled) {
		SetRelativeMouseMode(false);
		CenterMouseStick();
		g_mouse = {};
		LOGF("Mouse to right stick: disabled\n");
		return;
	}

	if (!SetRelativeMouseMode(true)) {
		return;
	}
	int ignored_x = 0;
	int ignored_y = 0;
	SDL_GetRelativeMouseState(&ignored_x, &ignored_y);
	g_mouse.enabled   = true;
	g_mouse.next_poll = SDL_GetTicks64() + MOUSE_POLL_INTERVAL_MS;
	LOGF("Mouse to right stick: enabled (F7 to release)\n");
}

int PollMouse(uint64_t now_ms) {
	if (now_ms < g_mouse.next_poll) {
		return static_cast<int>(g_mouse.next_poll - now_ms);
	}
	g_mouse.next_poll = now_ms + MOUSE_POLL_INTERVAL_MS;

	int delta_x = 0;
	int delta_y = 0;
	SDL_GetRelativeMouseState(&delta_x, &delta_y);
	if (delta_x == 0 && delta_y == 0) {
		CenterMouseStick();
		return MOUSE_POLL_INTERVAL_MS;
	}

	MouseToJoystick(delta_x, delta_y);
	g_mouse.output = true;
	return MOUSE_POLL_INTERVAL_MS;
}

bool HostInputWaitEvent(SDL_Event* event) {
	if (!g_mouse.enabled || SDL_GetKeyboardFocus() == nullptr) {
		CenterMouseStick();
		if (SDL_WaitEvent(event) == 0) {
			EXIT("%s\n", SDL_GetError());
		}
		return true;
	}

	const int timeout_ms = PollMouse(SDL_GetTicks64());
	SDL_ClearError();
	if (SDL_WaitEventTimeout(event, timeout_ms) != 0) {
		return true;
	}
	if (SDL_GetError()[0] != '\0') {
		EXIT("%s\n", SDL_GetError());
	}
	return false;
}

} // namespace Libs::Graphics

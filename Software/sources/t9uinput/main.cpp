/*
    This file is part of t9uinput.
    Copyright (C) 2022 Reimu NotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_set>
#include <unordered_map>

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdbool>
#include <cinttypes>

#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/poll.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define SAMEKEY_DURATION_MS	500

auto ts_keypress = std::chrono::steady_clock::now();

struct key_def {
	const char *name = "ERROR";
	std::vector<uint32_t> keycodes{};
	std::vector<input_event> input_events{};
};

std::unordered_map<uint16_t, std::vector<key_def>> key_mapping;

int fd_ev = -1, fd_ui = -1;

static void uinput_setup(int fd) {
	static const int ev_list[] = {EV_KEY, EV_REL};

	for (int i=0; i<sizeof(ev_list)/sizeof(int); i++) {
		if (ioctl(fd, UI_SET_EVBIT, ev_list[i])) {
			fprintf(stderr, "UI_SET_EVBIT %d failed\n", i);

		}
	}

	static const int key_list[] = {KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER, KEY_LEFTCTRL, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, KEY_KPASTERISK, KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KPMINUS, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KPPLUS, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP0, KEY_KPDOT, KEY_ZENKAKUHANKAKU, KEY_102ND, KEY_F11, KEY_F12, KEY_RO, KEY_KATAKANA, KEY_HIRAGANA, KEY_HENKAN, KEY_KATAKANAHIRAGANA, KEY_MUHENKAN, KEY_KPJPCOMMA, KEY_KPENTER, KEY_RIGHTCTRL, KEY_KPSLASH, KEY_SYSRQ, KEY_RIGHTALT, KEY_LINEFEED, KEY_HOME, KEY_UP, KEY_PAGEUP, KEY_LEFT, KEY_RIGHT, KEY_END, KEY_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, KEY_MACRO, KEY_MUTE, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_POWER, KEY_KPEQUAL, KEY_KPPLUSMINUS, KEY_PAUSE, KEY_SCALE, KEY_KPCOMMA, KEY_HANGEUL, KEY_HANGUEL, KEY_HANJA, KEY_YEN, KEY_LEFTMETA, KEY_RIGHTMETA, KEY_COMPOSE, KEY_STOP, KEY_AGAIN, KEY_PROPS, KEY_UNDO, KEY_FRONT, KEY_COPY, KEY_OPEN, KEY_PASTE, KEY_FIND, KEY_CUT, KEY_HELP, KEY_MENU, KEY_CALC, KEY_SETUP, KEY_SLEEP, KEY_WAKEUP, KEY_FILE, KEY_SENDFILE, KEY_DELETEFILE, KEY_XFER, KEY_PROG1, KEY_PROG2, KEY_WWW, KEY_MSDOS, KEY_COFFEE, KEY_SCREENLOCK, KEY_ROTATE_DISPLAY, KEY_DIRECTION, KEY_CYCLEWINDOWS, KEY_MAIL, KEY_BOOKMARKS, KEY_COMPUTER, KEY_BACK, KEY_FORWARD, KEY_CLOSECD, KEY_EJECTCD, KEY_EJECTCLOSECD, KEY_NEXTSONG, KEY_PLAYPAUSE, KEY_PREVIOUSSONG, KEY_STOPCD, KEY_RECORD, KEY_REWIND, KEY_PHONE, KEY_ISO, KEY_CONFIG, KEY_HOMEPAGE, KEY_REFRESH, KEY_EXIT, KEY_MOVE, KEY_EDIT, KEY_SCROLLUP, KEY_SCROLLDOWN, KEY_KPLEFTPAREN, KEY_KPRIGHTPAREN, KEY_NEW, KEY_REDO, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24, KEY_PLAYCD, KEY_PAUSECD, KEY_PROG3, KEY_PROG4, KEY_DASHBOARD, KEY_SUSPEND, KEY_CLOSE, KEY_PLAY, KEY_FASTFORWARD, KEY_BASSBOOST, KEY_PRINT, KEY_HP, KEY_CAMERA, KEY_SOUND, KEY_QUESTION, KEY_EMAIL, KEY_CHAT, KEY_SEARCH, KEY_CONNECT, KEY_FINANCE, KEY_SPORT, KEY_SHOP, KEY_ALTERASE, KEY_CANCEL, KEY_BRIGHTNESSDOWN, KEY_BRIGHTNESSUP, KEY_MEDIA, KEY_SWITCHVIDEOMODE, KEY_KBDILLUMTOGGLE, KEY_KBDILLUMDOWN, KEY_KBDILLUMUP, KEY_SEND, KEY_REPLY, KEY_FORWARDMAIL, KEY_SAVE, KEY_DOCUMENTS, KEY_BATTERY, KEY_BLUETOOTH, KEY_WLAN, KEY_UWB, KEY_UNKNOWN, KEY_VIDEO_NEXT, KEY_VIDEO_PREV, KEY_BRIGHTNESS_CYCLE, KEY_BRIGHTNESS_AUTO, KEY_BRIGHTNESS_ZERO, KEY_DISPLAY_OFF, KEY_WWAN, KEY_WIMAX, KEY_RFKILL, KEY_MICMUTE, KEY_OK, KEY_SELECT, KEY_GOTO, KEY_CLEAR, KEY_POWER2, KEY_OPTION, KEY_INFO, KEY_TIME, KEY_VENDOR, KEY_ARCHIVE, KEY_PROGRAM, KEY_CHANNEL, KEY_FAVORITES, KEY_EPG, KEY_PVR, KEY_MHP, KEY_LANGUAGE, KEY_TITLE, KEY_SUBTITLE, KEY_ANGLE, KEY_ZOOM, KEY_MODE, KEY_KEYBOARD, KEY_SCREEN, KEY_PC, KEY_TV, KEY_TV2, KEY_VCR, KEY_VCR2, KEY_SAT, KEY_SAT2, KEY_CD, KEY_TAPE, KEY_RADIO, KEY_TUNER, KEY_PLAYER, KEY_TEXT, KEY_DVD, KEY_AUX, KEY_MP3, KEY_AUDIO, KEY_VIDEO, KEY_DIRECTORY, KEY_LIST, KEY_MEMO, KEY_CALENDAR, KEY_RED, KEY_GREEN, KEY_YELLOW, KEY_BLUE, KEY_CHANNELUP, KEY_CHANNELDOWN, KEY_FIRST, KEY_LAST, KEY_AB, KEY_NEXT, KEY_RESTART, KEY_SLOW, KEY_SHUFFLE, KEY_BREAK, KEY_PREVIOUS, KEY_DIGITS, KEY_TEEN, KEY_TWEN, KEY_VIDEOPHONE, KEY_GAMES, KEY_ZOOMIN, KEY_ZOOMOUT, KEY_ZOOMRESET, KEY_WORDPROCESSOR, KEY_EDITOR, KEY_SPREADSHEET, KEY_GRAPHICSEDITOR, KEY_PRESENTATION, KEY_DATABASE, KEY_NEWS, KEY_VOICEMAIL, KEY_ADDRESSBOOK, KEY_MESSENGER, KEY_DISPLAYTOGGLE, KEY_BRIGHTNESS_TOGGLE, KEY_SPELLCHECK, KEY_LOGOFF, KEY_DOLLAR, KEY_EURO, KEY_FRAMEBACK, KEY_FRAMEFORWARD, KEY_CONTEXT_MENU, KEY_MEDIA_REPEAT, KEY_10CHANNELSUP, KEY_10CHANNELSDOWN, KEY_IMAGES, KEY_DEL_EOL, KEY_DEL_EOS, KEY_INS_LINE, KEY_DEL_LINE, KEY_FN, KEY_FN_ESC, KEY_FN_F1, KEY_FN_F2, KEY_FN_F3, KEY_FN_F4, KEY_FN_F5, KEY_FN_F6, KEY_FN_F7, KEY_FN_F8, KEY_FN_F9, KEY_FN_F10, KEY_FN_F11, KEY_FN_F12, KEY_FN_1, KEY_FN_2, KEY_FN_D, KEY_FN_E, KEY_FN_F, KEY_FN_S, KEY_FN_B, KEY_BRL_DOT1, KEY_BRL_DOT2, KEY_BRL_DOT3, KEY_BRL_DOT4, KEY_BRL_DOT5, KEY_BRL_DOT6, KEY_BRL_DOT7, KEY_BRL_DOT8, KEY_BRL_DOT9, KEY_BRL_DOT10, KEY_NUMERIC_0, KEY_NUMERIC_1, KEY_NUMERIC_2, KEY_NUMERIC_3, KEY_NUMERIC_4, KEY_NUMERIC_5, KEY_NUMERIC_6, KEY_NUMERIC_7, KEY_NUMERIC_8, KEY_NUMERIC_9, KEY_NUMERIC_STAR, KEY_NUMERIC_POUND, KEY_NUMERIC_A, KEY_NUMERIC_B, KEY_NUMERIC_C, KEY_NUMERIC_D, KEY_CAMERA_FOCUS, KEY_WPS_BUTTON, KEY_TOUCHPAD_TOGGLE, KEY_TOUCHPAD_ON, KEY_TOUCHPAD_OFF, KEY_CAMERA_ZOOMIN, KEY_CAMERA_ZOOMOUT, KEY_CAMERA_UP, KEY_CAMERA_DOWN, KEY_CAMERA_LEFT, KEY_CAMERA_RIGHT, KEY_ATTENDANT_ON, KEY_ATTENDANT_OFF, KEY_ATTENDANT_TOGGLE, KEY_LIGHTS_TOGGLE, KEY_ALS_TOGGLE, KEY_BUTTONCONFIG, KEY_TASKMANAGER, KEY_JOURNAL, KEY_CONTROLPANEL, KEY_APPSELECT, KEY_SCREENSAVER, KEY_VOICECOMMAND, KEY_BRIGHTNESS_MIN, KEY_BRIGHTNESS_MAX, KEY_KBDINPUTASSIST_PREV, KEY_KBDINPUTASSIST_NEXT, KEY_KBDINPUTASSIST_PREVGROUP, KEY_KBDINPUTASSIST_NEXTGROUP, KEY_KBDINPUTASSIST_ACCEPT, KEY_KBDINPUTASSIST_CANCEL, KEY_RIGHT_UP, KEY_RIGHT_DOWN, KEY_LEFT_UP, KEY_LEFT_DOWN, KEY_ROOT_MENU, KEY_MEDIA_TOP_MENU, KEY_NUMERIC_11, KEY_NUMERIC_12, KEY_AUDIO_DESC, KEY_3D_MODE, KEY_NEXT_FAVORITE, KEY_STOP_RECORD, KEY_PAUSE_RECORD, KEY_VOD, KEY_UNMUTE, KEY_FASTREVERSE, KEY_SLOWREVERSE, KEY_DATA, KEY_MIN_INTERESTING, BTN_MISC, BTN_0, BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9, BTN_MOUSE, BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_SIDE, BTN_EXTRA, BTN_FORWARD, BTN_BACK, BTN_TASK, BTN_JOYSTICK, BTN_TRIGGER, BTN_THUMB, BTN_THUMB2, BTN_TOP, BTN_TOP2, BTN_PINKIE, BTN_BASE, BTN_BASE2, BTN_BASE3, BTN_BASE4, BTN_BASE5, BTN_BASE6, BTN_DEAD, BTN_GAMEPAD, BTN_SOUTH, BTN_A, BTN_EAST, BTN_B, BTN_C, BTN_NORTH, BTN_X, BTN_WEST, BTN_Y, BTN_Z, BTN_TL, BTN_TR, BTN_TL2, BTN_TR2, BTN_SELECT, BTN_START, BTN_MODE, BTN_THUMBL, BTN_THUMBR, BTN_DIGI, BTN_TOOL_PEN, BTN_TOOL_RUBBER, BTN_TOOL_BRUSH, BTN_TOOL_PENCIL, BTN_TOOL_AIRBRUSH, BTN_TOOL_FINGER, BTN_TOOL_MOUSE, BTN_TOOL_LENS, BTN_TOOL_QUINTTAP, BTN_TOUCH, BTN_STYLUS, BTN_STYLUS2, BTN_TOOL_DOUBLETAP, BTN_TOOL_TRIPLETAP, BTN_TOOL_QUADTAP, BTN_WHEEL, BTN_GEAR_DOWN, BTN_GEAR_UP, BTN_DPAD_UP, BTN_DPAD_DOWN, BTN_DPAD_LEFT, BTN_DPAD_RIGHT, BTN_TRIGGER_HAPPY, BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2, BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4, BTN_TRIGGER_HAPPY5, BTN_TRIGGER_HAPPY6, BTN_TRIGGER_HAPPY7, BTN_TRIGGER_HAPPY8, BTN_TRIGGER_HAPPY9, BTN_TRIGGER_HAPPY10, BTN_TRIGGER_HAPPY11, BTN_TRIGGER_HAPPY12, BTN_TRIGGER_HAPPY13, BTN_TRIGGER_HAPPY14, BTN_TRIGGER_HAPPY15, BTN_TRIGGER_HAPPY16, BTN_TRIGGER_HAPPY17, BTN_TRIGGER_HAPPY18, BTN_TRIGGER_HAPPY19, BTN_TRIGGER_HAPPY20, BTN_TRIGGER_HAPPY21, BTN_TRIGGER_HAPPY22, BTN_TRIGGER_HAPPY23, BTN_TRIGGER_HAPPY24, BTN_TRIGGER_HAPPY25, BTN_TRIGGER_HAPPY26, BTN_TRIGGER_HAPPY27, BTN_TRIGGER_HAPPY28, BTN_TRIGGER_HAPPY29, BTN_TRIGGER_HAPPY30, BTN_TRIGGER_HAPPY31, BTN_TRIGGER_HAPPY32, BTN_TRIGGER_HAPPY33, BTN_TRIGGER_HAPPY34, BTN_TRIGGER_HAPPY35, BTN_TRIGGER_HAPPY36, BTN_TRIGGER_HAPPY37, BTN_TRIGGER_HAPPY38, BTN_TRIGGER_HAPPY39, BTN_TRIGGER_HAPPY40};

	for (int i=0; i<sizeof(key_list)/sizeof(int); i++) {
		if (ioctl(fd, UI_SET_KEYBIT, key_list[i])) {
			fprintf(stderr, "UI_SET_KEYBIT %d failed\n", i);

		}
	}

	static const int rel_list[] = {REL_X, REL_Y, REL_Z, REL_WHEEL, REL_HWHEEL};

	for (int i=0; i<sizeof(rel_list)/sizeof(int); i++) {
		if (ioctl(fd, UI_SET_RELBIT, rel_list[i])) {
			fprintf(stderr, "UI_SET_RELBIT %d failed\n", i);

		}
	}

	static const struct uinput_setup usetup = {
		{
			.bustype = BUS_VIRTUAL,
			.vendor = 0x2333,
			.product = 0x6666,
			.version = 1
		},
		"Notkia virtual keypad",
	};

	if (ioctl(fd, UI_DEV_SETUP, &usetup)) {
		perror("UI_DEV_SETUP ioctl failed");
		abort();
	}

	if (ioctl(fd, UI_DEV_CREATE)) {
		perror("UI_DEV_CREATE ioctl failed");
		abort();
	}

}


void init_key_mappings() {
	auto &m = key_mapping;

	m[KEY_1] = {
		{"/", {KEY_SLASH}}, {"-", {KEY_MINUS}}, {".", {KEY_DOT}}, {",", {KEY_COMMA}}, {"=", {KEY_EQUAL}}, {"\\", {KEY_BACKSLASH}}, {"1", {KEY_1}},
	};

	m[KEY_2] = {
		{"a", {KEY_A}}, {"b", {KEY_B}}, {"c", {KEY_C}}, {"2", {KEY_2}}
	};

	m[KEY_3] = {
		{"d", {KEY_D}}, {"e", {KEY_E}}, {"f", {KEY_F}}, {"3", {KEY_3}}
	};

	m[KEY_4] = {
		{"g", {KEY_G}}, {"h", {KEY_H}}, {"i", {KEY_I}}, {"4", {KEY_4}}
	};

	m[KEY_5] = {
		{"j", {KEY_J}}, {"k", {KEY_K}}, {"l", {KEY_L}}, {"5", {KEY_5}}
	};

	m[KEY_6] = {
		{"m", {KEY_M}}, {"n", {KEY_N}}, {"o", {KEY_O}}, {"6", {KEY_6}}
	};

	m[KEY_7] = {
		{"p", {KEY_P}}, {"q", {KEY_Q}}, {"r", {KEY_R}}, {"s", {KEY_S}}, {"7", {KEY_7}}
	};

	m[KEY_8] = {
		{"t", {KEY_T}}, {"u", {KEY_U}}, {"v", {KEY_V}}, {"8", {KEY_8}}
	};

	m[KEY_9] = {
		{"w", {KEY_W}}, {"x", {KEY_X}}, {"y", {KEY_Y}}, {"z", {KEY_Z}}, {"9", {KEY_9}}
	};

	m[KEY_0] = {
		{" ", {KEY_SPACE}},
		{"0", {KEY_0}},
		{"^A", {KEY_HOME}},
		{"^E", {KEY_END}},
	};


	m[KEY_F1] = {
		{"\\t", {KEY_TAB}},
		{"^R", {}, {
				   {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 1},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_R, .value = 1},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_R, .value = 0},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 0},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
			   }},
		{"F1", {KEY_F1}},
	};


	m[KEY_F2] = {
		{"<-", {KEY_BACKSPACE}},
		{"F2", {KEY_F2}},
		{"^W", {}, {
				   {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 1},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_W, .value = 1},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_W, .value = 0},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 0},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
			   }},
	};

	m[KEY_F4] = {
		{"^C", {}, {
				   {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 1},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_C, .value = 1},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_C, .value = 0},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
				   {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = 0},
				   {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
			   }},
		{"F4", {KEY_F4}},
	};

//	keys_ignored = {KEY_0, KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_F1, KEY_F2, KEY_F3, KEY_F4,
//			KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
}

void time_save() {
	ts_keypress = std::chrono::steady_clock::now();
}

unsigned long time_save_compare() {

	auto ts_keypress_new = std::chrono::steady_clock::now();

	unsigned long diff = std::chrono::duration_cast<std::chrono::milliseconds>(ts_keypress_new - ts_keypress).count();

	std::swap(ts_keypress_new, ts_keypress);

	return diff;
}

void prompt(key_def *kdef) {
	size_t kn_len = strlen(kdef->name);
	char buf[128];

	int blen = sprintf(buf, "\033[%zuD", kn_len);

	write(STDOUT_FILENO, kdef->name, kn_len);
	write(STDOUT_FILENO, buf, blen);
}

void prompt_clear(key_def *kdef) {
	size_t kn_len = strlen(kdef->name);
	char buf[128];

	int blen = sprintf(buf, "\033[%zuD", kn_len);

	for (size_t i=0; i<kn_len; i++) {
		write(STDOUT_FILENO, " ", 1);
	}

	write(STDOUT_FILENO, buf, blen);
}

void emit(key_def *kdef) {
//	printf("emit: %s\n", kdef->name);

	prompt_clear(kdef);

	const input_event evs = {
		.type = EV_SYN,
		.code = SYN_REPORT,
		.value = 0
	};

	input_event ev = {
		.type = EV_KEY
	};


	for (auto &it: kdef->keycodes) {
		uint16_t sleep_time1 = (it >> 16) & 0xff;
		uint16_t sleep_time2 = (it >> 24) & 0xff;

		ev.code = it & 0xffff;
		ev.value = 1;
		write(fd_ui, &ev, sizeof(ev));
		write(fd_ui, &evs, sizeof(evs));

		if (sleep_time1) {
			usleep(1000 * sleep_time1);
		}

		ev.value = 0;
		write(fd_ui, &ev, sizeof(ev));
		write(fd_ui, &evs, sizeof(evs));

		if (sleep_time2) {
			usleep(1000 * sleep_time2);
		}
	}

	for (auto &it: kdef->input_events) {
		if (it.type == 0xffff) {
			usleep(it.value);
		} else {
//				printf("-- t: %u, c: %u, v: %d\n", it.type, it.code, it.value);
			write(fd_ui, &it, sizeof(it));
		}
	}

	sched_yield();

}

int main(int argc, char **argv) {

	if (argc != 3) {
		puts("Usage: t9uinput </dev/input/eventX> <grab:0|1>");
		exit(1);
	}

	sleep(1);

	fd_ev = open(argv[1], O_RDWR);
	assert(fd_ev > 0);

	fd_ui = open("/dev/uinput", O_RDWR);
	assert(fd_ui > 0);

	uinput_setup(fd_ui);

	struct pollfd pfds[1];

	pfds[0].fd = fd_ev;
	pfds[0].events = POLLIN;

	input_event cur_iev{};

//	bool keypress_inpr = false;
	uint16_t selected_key = 0;
	size_t selected_key_subkey_index = 0;
	key_def *selected_key_subkey = nullptr;

	init_key_mappings();

	if (strtol(argv[2], nullptr, 10)) {
		ioctl(fd_ev, EVIOCGRAB, (void *) 1);
	}

	while (1) {
		int poll_timeout = SAMEKEY_DURATION_MS;
		int ret = poll(pfds, 1, poll_timeout);

		if (ret > 0) {
			if (pfds[0].revents & POLLIN) {
				read(fd_ev, &cur_iev, sizeof(cur_iev));

				auto it_km = key_mapping.find(cur_iev.code);

				if (it_km == key_mapping.end()) {
					write(fd_ui, &cur_iev, sizeof(cur_iev));
				} else {
					auto &target_key_kc_list = it_km->second;

//					printf("t: %u, c: %u, v: %d\n", cur_iev.type, cur_iev.code, cur_iev.value);

					if (cur_iev.value == 0) { // Release
						if (selected_key == cur_iev.code) {
							selected_key_subkey_index++;
							selected_key_subkey_index %= target_key_kc_list.size();
						} else {
							if (selected_key) {
								emit(selected_key_subkey);
							}
							selected_key_subkey_index = 0;
							selected_key = cur_iev.code;
						}

						selected_key_subkey = &target_key_kc_list[selected_key_subkey_index];

						prompt(selected_key_subkey);

//						printf("cur kckm: %u[%zu] -> \"%s\"\n", selected_key, selected_key_subkey_index, selected_key_subkey->name);
					}

				}

			}

		} else if (ret == 0) {
			if (selected_key) {
				emit(selected_key_subkey);

				selected_key = 0;
				selected_key_subkey_index = 0;
				selected_key_subkey = nullptr;
			}
		} else {
			puts("putain!");
		}
	}

	return 0;
}

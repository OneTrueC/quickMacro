/* simple and fast x11 macro program                                     *\
|* Copyright (C) 2024 Andrew Charles Marino                              *|
|*                                                                       *|
|* This program is free software: you can redistribute it and/or modify  *|
|* it under the terms of the GNU General Public License as published by  *|
|* the Free Software Foundation, either version 3 of the License, or     *|
|* (at your option) any later version.                                   *|
|*                                                                       *|
|* This program is distributed in the hope that it will be useful,       *|
|* but WITHOUT ANY WARRANTY; without even the implied warranty of        *|
|* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *|
|* GNU General Public License for more details.                          *|
|*                                                                       *|
|* You should have received a copy of the GNU General Public License     *|
\* along with this program.  If not, see <https://www.gnu.org/licenses/>.*/
#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>

#define EPLAYRECORD die(22, "cannot play and record at the same time")
#define ERECORDLOOP die(22, "cannot loop while recording")

/* config */
	/* keycode of start/end key */
const KeyCode QUITKEY = 134;
/* end config */

enum Type {
	BUTTON,
	KEY,
	MOVE,
};

struct Event {
	enum Type type;
	Time time;
	unsigned long keyCodeOrButtonOrX;
	int yOrIsPress;
	int screen;
};

enum State {
	NONE,
	PLAY,
	RECORD,
};

const char* shortopts = "hvlr:p:";
const struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{"loop", no_argument, NULL, 'l'},
	{"record", required_argument, NULL, 'r'},
	{"play", required_argument, NULL, 'p'},
	{NULL, 0, NULL, 0}
};

int running = 1;
int xiOpcode;

void die(int ecode, const char* message, ...);
enum State handleargs(int argc, char** argv, FILE** file, int* loop);
int isfile(char* file);
void* loopPlay(void* dpy);
void play(FILE* file, Display* dpy);
void record(FILE* file, Display* dpy);
void usage(void);
void version(void);

int
main(int argc, char** argv)
{
	Display* dpy;
	FILE* file;
	int loop, dummy;
	pthread_t thid;
	enum State state;

	state = NONE;
	loop = 0;

	state = handleargs(argc, argv, &file, &loop);

	if (file == NULL)
		die (errno, "file open error: %s", strerror(errno));

	if ((dpy = XOpenDisplay(NULL)) == NULL)
		die(11, "falure connecting to X Server");

	if (!XQueryExtension(dpy, "XInputExtension", &xiOpcode, &dummy, &dummy))
		die(11, "X Input extension not available");

	if (!XQueryExtension(dpy, "XTEST", &dummy, &dummy, &dummy))
		die(11, "XTEST not available");

	switch (state) {
	case NONE:
		usage();
		die(22, "missing mode");
		break;
	case RECORD:
		record(file, dpy);
		break;
	case PLAY:
		if (loop) {
			if ((errno = pthread_create(&thid, NULL, loopPlay, dpy)) != 0)
				die(errno, "loop thread creation failed: %s", strerror(errno));

			while (running) {
				play(file, dpy);
				rewind(file);
			}
			pthread_join(thid, NULL);
		} else
			play(file, dpy);
		break;
	}

	fclose(file);

	XCloseDisplay(dpy);

	return 0;
}

void
die(int ecode, const char* message, ...)
{
	char messagep[strlen(message) + 13];
	va_list ap;

	strcpy(messagep, "quickMacro: ");
	strcpy(messagep + 12, message);

	va_start(ap, message);
	vfprintf(stderr, messagep, ap);
	va_end(ap);

	exit(ecode);
}

enum State
handleargs(int argc, char** argv, FILE** file, int* loop)
{
	char ch;
	enum State state;

	state = NONE;

	while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (ch) {
		case 'h':
			usage();
			exit(0);
		case 'v':
			version();
			exit(0);
		case 'l':
			if (state == RECORD) {
				usage();
				ERECORDLOOP;
			}
			*loop = 1;
			break;
		case 'p':
			if (state == RECORD) {
				usage();
				EPLAYRECORD;
			}
			if (!isfile(optarg))
				die(21, "file specified is not a regular file");
			state = PLAY;
			*file = fopen(optarg, "r");
			break;
		case 'r':
			if (state == PLAY) {
				usage();
				EPLAYRECORD;
			}
			if (*loop) {
				usage();
				ERECORDLOOP;
			}
			state = RECORD;
			*file = fopen(optarg, "w");
			break;
		case '?':
			usage();
			die(22, "unknown argument");
			break;
		case ':':
			usage();
			die(22, "need a file to %s",
			    (optopt == 'p') ? "play from" : "record to");
			break;
		default:
			usage();
			exit(0);
		}
	}

	return state;
}

int
isfile(char* file)
{
	struct stat file_stat;
	stat(file, &file_stat);
	return S_ISREG(file_stat.st_mode);
}

void*
loopPlay(void* dpy)
{
	XEvent ev;
	XGenericEventCookie* cookie;
	XIEventMask mask;
	XIRawEvent* rawEv;

	cookie = (XGenericEventCookie*)&ev.xcookie;

	mask.deviceid = XIAllMasterDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = calloc(mask.mask_len, sizeof(char));
	XISetMask(mask.mask, XI_RawKeyPress);
	XISetMask(mask.mask, XI_RawKeyRelease);
	XISetMask(mask.mask, XI_RawButtonPress);
	XISetMask(mask.mask, XI_RawButtonRelease);

	XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);
	XSync(dpy, False);

	free(mask.mask);

	while (running) {
		XNextEvent(dpy, &ev);

		if (XGetEventData(dpy, cookie) && cookie->type == GenericEvent &&
		   cookie->extension == xiOpcode && cookie->evtype == XI_RawKeyPress) {
			rawEv = cookie->data;
			if (rawEv->detail == QUITKEY)
				running = 0;
		}
		XFreeEventData(dpy, cookie);
	}

	exit(0);
}

void
play(FILE* file, Display* dpy)
{
	struct Event event;
	Time time;
	struct timespec ts;
	
	time = 0;

	while (fread(&event, sizeof(struct Event), 1, file)) {
		switch (event.type) {
		case BUTTON:
			XTestFakeButtonEvent(dpy, event.keyCodeOrButtonOrX, event.yOrIsPress,
			                     event.time);
			break;
		case KEY:
			XTestFakeKeyEvent(dpy, event.keyCodeOrButtonOrX, event.yOrIsPress,
			                  event.time);
			break;
		case MOVE:
			XTestFakeMotionEvent(dpy, event.screen, event.keyCodeOrButtonOrX,
			                     event.yOrIsPress, event.time);
			break;
		}
		time += event.time;
	}

	ts.tv_sec = time / 1000;
	ts.tv_nsec = (time % 1000) * 1000000;

	nanosleep(&ts, &ts);

	return;
}

void
record(FILE* file, Display* dpy)
{
	struct Event event;
	Time offset;
	XEvent ev;
	XGenericEventCookie* cookie;
	XIDeviceEvent* devEv;
	XIEventMask mask[2];
	XIRawEvent* rawEv;

	cookie = (XGenericEventCookie*)&ev.xcookie;

	mask[0].deviceid = XIAllMasterDevices;
	mask[0].mask_len = XIMaskLen(XI_LASTEVENT);
	mask[0].mask = calloc(mask[0].mask_len, sizeof(char));
	XISetMask(mask[0].mask, XI_RawKeyPress);
	XISetMask(mask[0].mask, XI_RawKeyRelease);
	XISetMask(mask[0].mask, XI_RawButtonPress);
	XISetMask(mask[0].mask, XI_RawButtonRelease);

	mask[1].deviceid = XIAllDevices;
	mask[1].mask_len = XIMaskLen(XI_LASTEVENT);
	mask[1].mask = calloc(mask[1].mask_len, sizeof(char));
	XISetMask(mask[1].mask, XI_Motion);

	XISelectEvents(dpy, DefaultRootWindow(dpy), mask, 2);
	XSync(dpy, False);

	free(mask[0].mask);
	free(mask[1].mask);

	while (1) {
		XNextEvent(dpy, &ev);

		if (XGetEventData(dpy, cookie) && cookie->type == GenericEvent &&
		   cookie->extension == xiOpcode && cookie->evtype == XI_RawKeyRelease) {
			rawEv = cookie->data;
			if (rawEv->detail == QUITKEY) {
				offset = rawEv->time;
				break;
			}
		}
		XFreeEventData(dpy, cookie);
	}

	while (running) {
		XNextEvent(dpy, &ev);

		if (XGetEventData(dpy, cookie) && cookie->type == GenericEvent &&
		   cookie->extension == xiOpcode) {
			switch (cookie->evtype) {
			case XI_RawKeyPress:
			case XI_RawKeyRelease:
				rawEv = cookie->data;
				if (rawEv->detail == QUITKEY) {
					running = 0;
					continue;
				}
				event.type = KEY;
				break;
			case XI_RawButtonPress:
			case XI_RawButtonRelease:
				rawEv = cookie->data;
				event.type = BUTTON;
				break;
			case XI_Motion:
				devEv = cookie->data;
				event.type = MOVE;
				break;
			}

			switch (cookie->evtype) {
			case XI_RawKeyPress:
			case XI_RawButtonPress:
				event.yOrIsPress = 1;
				event.keyCodeOrButtonOrX = rawEv->detail;
				event.time = rawEv->time - offset;
				break;
			case XI_RawKeyRelease:
			case XI_RawButtonRelease:
				event.yOrIsPress = 0;
				event.keyCodeOrButtonOrX = rawEv->detail;
				event.time = rawEv->time - offset;
				break;
			case XI_Motion:
				event.keyCodeOrButtonOrX = devEv->root_x;
				event.yOrIsPress = devEv->root_y;
				event.screen = devEv->detail;
				event.time = devEv->time - offset;
				break;
			}
			fwrite(&event, sizeof(struct Event), 1, file);

			offset = event.time + offset;
		}
		XFreeEventData(dpy, cookie);
	}
	XFreeEventData(dpy, cookie);

	return;
}


void
usage(void)
{
	puts("Usage: quickMacro MODE FILE [LOOP]\n"
	     "\n"
	     "  -p,  --play     play macro file\n"
	     "  -r,  --record   record macro file\n"
	     "  -l,  --loop     loop macro until QUITKEY is pressed\n"
	     "  -h,  --help     display this help and exit\n"
	     "  -v,  --version  output version information and exit\n");
}

void
version(void)
{
	puts("quickMacro (v"VERSION")\n"
	     "\n"
	     "written and gpl'd by Drew Marino (OneTrueC)\n");
}

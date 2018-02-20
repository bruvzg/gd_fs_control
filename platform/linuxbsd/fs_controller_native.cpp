/*************************************************************************/
/*  fs_controller_native.cpp                                             */
/*************************************************************************/

#include "fs_controller_native.h"

#include "core/os/input.h"
#include "core/os/os.h"

#include <climits>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

struct FSControllerNative::VideoMode {
	::Display *x11_display;
	RRMode native;
};

void FSControllerNative::get_screen_mode_list_native(List<Vector3> *p_list, int p_screen) const {

	if (p_screen < OS::get_singleton()->get_screen_count()) {
		XRRScreenResources *sr = XRRGetScreenResourcesCurrent(native_mode->x11_display, DefaultRootWindow(native_mode->x11_display));
		XRROutputInfo *oi = NULL;
		XRRCrtcInfo *ci = NULL;
		Point2i curren_screen_pos = OS::get_singleton()->get_screen_position(p_screen);

		for (int i = 0; i < sr->noutput; i++) {
			oi = XRRGetOutputInfo(native_mode->x11_display, sr, sr->outputs[i]);
			if (oi->connection != RR_Connected || oi->crtc == None) {
				XRRFreeOutputInfo(oi);
				oi = NULL;
				continue;
			}
			ci = XRRGetCrtcInfo(native_mode->x11_display, sr, oi->crtc);
			if (curren_screen_pos.x == ci->x && curren_screen_pos.y == ci->y) {
				break;
			}
			XRRFreeCrtcInfo(ci);
		}
		if (oi == NULL || ci == NULL) {
			return;
		}

		for (int i = 0; i < oi->nmode; i++) {

			const XRRModeInfo *mi = NULL;
			int mode_width = 0;
			int mode_height = 0;

			for (int j = 0; j < sr->nmode; j++) {
				if (sr->modes[j].id == oi->modes[i]) {
					mi = sr->modes + j;
					break;
				}
			}

			if (mi == NULL)
				continue;

			//Skip unsupported modes
			if ((mi->modeFlags & RR_Interlace) != 0)
				continue;

			if (ci->rotation == RR_Rotate_90 || ci->rotation == RR_Rotate_270) {
				mode_width = mi->height;
				mode_height = mi->width;
			} else {
				mode_width = mi->width;
				mode_height = mi->height;
			}

			//Skip duplicated
			bool skip = false;
			Vector3 vmode = Vector3(mode_width, mode_height, (mi->hTotal && mi->vTotal) ? (int) ((double) mi->dotClock / ((double) mi->hTotal * (double) mi->vTotal)) : 0);
			for (List<Vector3>::Element *E = p_list->front(); E; E = E->next()) {
				if (E->get() == vmode)
					skip = true;
			}

			if (!skip)
				p_list->push_back(vmode);
		}

		XRRFreeOutputInfo(oi);
		XRRFreeCrtcInfo(ci);
		XRRFreeScreenResources(sr);
	}
};

void FSControllerNative::_set_video_mode(VideoMode *p_mode) {

	XRRScreenResources *sr = XRRGetScreenResourcesCurrent(native_mode->x11_display, DefaultRootWindow(native_mode->x11_display));
	if (sr == NULL) {
		WARN_PRINT("Failed to set video mode");
		return;
	}
	XRROutputInfo *oi = NULL;
	XRRCrtcInfo *ci = NULL;
	Point2i curren_screen_pos = OS::get_singleton()->get_screen_position(OS::get_singleton()->get_current_screen());
	for (int i = 0; i < sr->noutput; i++) {
		oi = XRRGetOutputInfo(native_mode->x11_display, sr, sr->outputs[i]);
		if (oi->connection != RR_Connected || oi->crtc == None) {
			XRRFreeOutputInfo(oi);
			oi = NULL;
			continue;
		}
		ci = XRRGetCrtcInfo(native_mode->x11_display, sr, oi->crtc);
		if (curren_screen_pos.x == ci->x && curren_screen_pos.y == ci->y) {
			break;
		}
		XRRFreeCrtcInfo(ci);
	}
	if (oi == NULL || ci == NULL) {
		WARN_PRINT("Failed to set video mode");
		XRRFreeScreenResources(sr);
		return;
	}

	XRRSetCrtcConfig(native_mode->x11_display, sr, oi->crtc, CurrentTime, ci->x, ci->y, p_mode->native, ci->rotation, ci->outputs, ci->noutput);

	XRRFreeOutputInfo(oi);
	XRRFreeCrtcInfo(ci);
	XRRFreeScreenResources(sr);
};

void FSControllerNative::activate_video_mode() {

	if (current_mode->native != None) {
		_set_video_mode(current_mode);
	}
};

void FSControllerNative::deactivate_video_mode() {

	if (native_mode->native != None) {
		_set_video_mode(native_mode);
	}
};

void FSControllerNative::restore_video_mode() {

	if (native_mode->native != None) {
		_set_video_mode(native_mode);

		current_mode->native = None;
		native_mode->native = None;
	}
};

void FSControllerNative::set_video_mode(const Vector3 &p_mode) {

	XRRScreenResources *sr = XRRGetScreenResourcesCurrent(native_mode->x11_display, DefaultRootWindow(native_mode->x11_display));
	if (sr == NULL) {
		WARN_PRINT("Failed to set video mode");
		return;
	}
	XRROutputInfo *oi = NULL;
	XRRCrtcInfo *ci = NULL;
	Point2i curren_screen_pos = OS::get_singleton()->get_screen_position(OS::get_singleton()->get_current_screen());

	for (int i = 0; i < sr->noutput; i++) {
		oi = XRRGetOutputInfo(native_mode->x11_display, sr, sr->outputs[i]);
		if (oi->connection != RR_Connected || oi->crtc == None) {
			XRRFreeOutputInfo(oi);
			oi = NULL;
			continue;
		}
		ci = XRRGetCrtcInfo(native_mode->x11_display, sr, oi->crtc);
		if (curren_screen_pos.x == ci->x && curren_screen_pos.y == ci->y) {
			break;
		}
		XRRFreeCrtcInfo(ci);
	}
	if (oi == NULL || ci == NULL) {
		WARN_PRINT("Failed to set video mode");
		return;
	}

	RRMode selected_mode = None;

	unsigned int size_diff, least_size_diff = UINT_MAX, rate_diff, least_rate_diff = UINT_MAX;

	for (int i = 0; i < oi->nmode; i++) {

		const XRRModeInfo *mi = NULL;
		int mode_width = 0;
		int mode_height = 0;

		for (int j = 0; j < sr->nmode; j++) {
			if (sr->modes[j].id == oi->modes[i]) {
				mi = sr->modes + j;
				break;
			}
		}

		if (mi == NULL)
			continue;

		//Skip unsupported modes
		if ((mi->modeFlags & RR_Interlace) != 0)
			continue;

		if (ci->rotation == RR_Rotate_90 || ci->rotation == RR_Rotate_270) {
			mode_width = mi->height;
			mode_height = mi->width;
		} else {
			mode_width = mi->width;
			mode_height = mi->height;
		}

		//Compare modes
		rate_diff = abs(((mi->hTotal && mi->vTotal) ? (int) ((double) mi->dotClock / ((double) mi->hTotal * (double) mi->vTotal)) : 0) - p_mode.z);
		size_diff = abs((mode_width - p_mode.x) * (mode_width - p_mode.x) +
						(mode_height - p_mode.y) * (mode_height - p_mode.y));

		if ((size_diff < least_size_diff) || (size_diff == least_size_diff && rate_diff < least_rate_diff)) {
			selected_mode = mi->id;
			least_size_diff = size_diff;
			least_rate_diff = rate_diff;
		}
	}

	if (selected_mode != None) {
		if (native_mode->native == None)
			native_mode->native = ci->mode;
		current_mode->native = selected_mode;
		_set_video_mode(current_mode);
	}

	XRRFreeOutputInfo(oi);
	XRRFreeCrtcInfo(ci);
	XRRFreeScreenResources(sr);

};

FSControllerNative::FSControllerNative() {

	native_mode = new FSControllerNative::VideoMode();
	current_mode = new FSControllerNative::VideoMode();

	native_mode->x11_display = XOpenDisplay(NULL);
	native_mode->native = None;
	current_mode->native = None;
};

FSControllerNative::~FSControllerNative() = default;

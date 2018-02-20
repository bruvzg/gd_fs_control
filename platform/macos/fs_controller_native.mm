/*************************************************************************/
/*  fs_controller_native.mm                                              */
/*************************************************************************/

#include "fs_controller_native.h"

#include "core/os/input.h"
#include "core/os/os.h"

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVDisplayLink.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDLib.h>

struct FSControllerNative::VideoMode {
	CGDisplayModeRef native;
};

void FSControllerNative::get_screen_mode_list_native(List<Vector3> *p_list, int p_screen) const {

	NSArray *screenArray = [NSScreen screens];

	if (p_screen < [screenArray count]) {
		CVDisplayLinkRef link;

		CGDirectDisplayID display_id = [[[[[NSScreen screens] objectAtIndex:p_screen] deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];

		CVDisplayLinkCreateWithCGDisplay(display_id, &link);

		CFArrayRef modes = CGDisplayCopyAllDisplayModes(display_id, NULL);
		CFIndex count = CFArrayGetCount(modes);

		for (CFIndex i = 0; i < count; i++) {
			CGDisplayModeRef dm = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

			//Skip non 32-bit modes
			CFStringRef format = CGDisplayModeCopyPixelEncoding(dm);
			if (CFStringCompare(format, CFSTR(IO32BitDirectPixels), 0) != kCFCompareEqualTo) {
				CFRelease(format);
				continue;
			}
			CFRelease(format);

			//Skip unsupported modes
			uint32_t flags = CGDisplayModeGetIOFlags(dm);
			if (!(flags & kDisplayModeValidFlag) || !(flags & kDisplayModeSafeFlag))
				continue;
			if (flags & kDisplayModeInterlacedFlag)
				continue;
			if (flags & kDisplayModeStretchedFlag)
				continue;

			//Skip duplicated
			bool skip = false;
			Vector3 vmode = Vector3((int)CGDisplayModeGetWidth(dm), (int)CGDisplayModeGetHeight(dm), (int)CGDisplayModeGetRefreshRate(dm));
			for (List<Vector3>::Element *E = p_list->front(); E; E = E->next()) {
				if (E->get() == vmode)
					skip = true;
			}

			if (!skip)
				p_list->push_back(vmode);
		}

		CFRelease(modes);
		CVDisplayLinkRelease(link);
	}
};

void FSControllerNative::_set_video_mode(VideoMode *p_mode) {

	CGDirectDisplayID display_id = [[[[[NSScreen screens] objectAtIndex:OS::get_singleton()->get_current_screen()] deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];

	CGDisplayFadeReservationToken token = kCGDisplayFadeReservationInvalidToken;

	if (CGAcquireDisplayFadeReservation(5, &token) == kCGErrorSuccess) {
		CGDisplayFade(token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);
	}

	CGError err = CGDisplaySetDisplayMode(display_id, p_mode->native, NULL);
	if (err) {
		WARN_PRINT("Failed to set video mode");
	}

	if (token != kCGDisplayFadeReservationInvalidToken) {
		CGDisplayFade(token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
		CGReleaseDisplayFadeReservation(token);
	}
};

void FSControllerNative::activate_video_mode() {

	if (current_mode->native != NULL) {
		_set_video_mode(current_mode);
	}
};

void FSControllerNative::deactivate_video_mode() {

	if (native_mode->native != NULL) {
		_set_video_mode(native_mode);
	}
};

void FSControllerNative::restore_video_mode() {

	if (native_mode->native != NULL) {
		_set_video_mode(native_mode);

		CGDisplayModeRelease(current_mode->native);
		current_mode->native = NULL;

		CGDisplayModeRelease(native_mode->native);
		native_mode->native = NULL;
	}
};

void FSControllerNative::set_video_mode(const Vector3 &p_mode) {

	CVDisplayLinkRef link;
	CGDisplayModeRef selected_mode = NULL;

	CGDirectDisplayID display_id = [[[[[NSScreen screens] objectAtIndex:OS::get_singleton()->get_current_screen()] deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];

	CVDisplayLinkCreateWithCGDisplay(display_id, &link);

	CFArrayRef modes = CGDisplayCopyAllDisplayModes(display_id, NULL);
	CFIndex count = CFArrayGetCount(modes);
	unsigned int size_diff, least_size_diff = UINT_MAX, rate_diff, least_rate_diff = UINT_MAX;

	for (CFIndex i = 0; i < count; i++) {
		CGDisplayModeRef dm = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

		//Skip non 32-bit modes
		CFStringRef format = CGDisplayModeCopyPixelEncoding(dm);
		if (CFStringCompare(format, CFSTR(IO32BitDirectPixels), 0) != kCFCompareEqualTo) {
			CFRelease(format);
			continue;
		}
		CFRelease(format);

		//Skip unsupported modes
		uint32_t flags = CGDisplayModeGetIOFlags(dm);
		if (!(flags & kDisplayModeValidFlag) || !(flags & kDisplayModeSafeFlag))
			continue;
		if (flags & kDisplayModeInterlacedFlag)
			continue;
		if (flags & kDisplayModeStretchedFlag)
			continue;

		//Compare modes
		rate_diff = abs((int)CGDisplayModeGetRefreshRate(dm) - p_mode.z);
		size_diff = abs(((int)CGDisplayModeGetWidth(dm) - p_mode.x) * ((int)CGDisplayModeGetWidth(dm) - p_mode.x) +
						((int)CGDisplayModeGetHeight(dm) - p_mode.y) * ((int)CGDisplayModeGetHeight(dm) - p_mode.y));

		if ((size_diff < least_size_diff) || (size_diff == least_size_diff && rate_diff < least_rate_diff)) {
			selected_mode = dm;
			least_size_diff = size_diff;
			least_rate_diff = rate_diff;
		}
	}

	if (selected_mode != NULL) {
		if (native_mode->native == NULL)
			native_mode->native = CGDisplayCopyDisplayMode(display_id);
		current_mode->native = selected_mode;
		_set_video_mode(current_mode);
	}

	CFRelease(modes);
	CVDisplayLinkRelease(link);
};

FSControllerNative::FSControllerNative() {

	native_mode = new FSControllerNative::VideoMode();
	current_mode = new FSControllerNative::VideoMode();

	native_mode->native = NULL;
	current_mode->native = NULL;
};

FSControllerNative::~FSControllerNative() = default;
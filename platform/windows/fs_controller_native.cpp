/*************************************************************************/
/*  fs_controller_native.cpp                                             */
/*************************************************************************/

#include "fs_controller_native.h"

#include "core/os/input.h"
#include "core/os/os.h"

#include <windows.h>
#include <windowsx.h>

struct FSControllerNative::VideoMode {
	DEVMODEW native;
};

void FSControllerNative::get_screen_mode_list_native(List<Vector3> *p_list, int p_screen) const {

	if (p_screen < OS::get_singleton()->get_screen_count()) {
		DISPLAY_DEVICEW device;
		ZeroMemory(&device, sizeof(device));
		device.cb = sizeof(device);

		EnumDisplayDevicesW(NULL, p_screen, &device, 0);

		int i = 0;

		DEVMODEW dm;
		ZeroMemory(&dm, sizeof(dm));
		dm.dmSize = sizeof(dm);

		while (EnumDisplaySettingsW(device.DeviceName, i, &dm)) {

			//Skip non 32-bit modes
			if (dm.dmBitsPerPel != 32)
				continue;

			//Skip unsupported modes
			if (ChangeDisplaySettingsExW(device.DeviceName, &dm, NULL, CDS_TEST, NULL) != DISP_CHANGE_SUCCESSFUL)
				continue;

			bool skip = false;
			Vector3 vmode = Vector3(dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency);
			for (List<Vector3>::Element *E = p_list->front(); E; E = E->next()) {
				if (E->get() == vmode)
					skip = true;
			}

			if (!skip)
				p_list->push_back(vmode);

			ZeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			i++;
		}
	}
};

void FSControllerNative::_set_video_mode(VideoMode *p_mode) {

	int display_id = OS::get_singleton()->get_current_screen();

	DISPLAY_DEVICEW device;
	ZeroMemory(&device, sizeof(device));
	device.cb = sizeof(device);

	EnumDisplayDevicesW(NULL, display_id, &device, 0);

	LONG result = ChangeDisplaySettingsExW(device.DeviceName, (p_mode->native.dmSize > 0) ? &p_mode->native : NULL, NULL, CDS_FULLSCREEN, NULL);
	if (result != DISP_CHANGE_SUCCESSFUL) {
		WARN_PRINT("Failed to set video mode");
	}
};

void FSControllerNative::activate_video_mode() {

	if (current_mode->native.dmSize != 0) {
		_set_video_mode(current_mode);
	}
};

void FSControllerNative::deactivate_video_mode() {

	if (current_mode->native.dmSize != 0) {
		_set_video_mode(native_mode);
	}
};

void FSControllerNative::restore_video_mode() {

	if (current_mode->native.dmSize != 0) {
		_set_video_mode(native_mode);
		ZeroMemory(&current_mode->native, sizeof(current_mode->native));
	}
};

void FSControllerNative::set_video_mode(const Vector3 &p_mode) {

	int display_id = OS::get_singleton()->get_current_screen();

	DISPLAY_DEVICEW device;
	ZeroMemory(&device, sizeof(device));
	device.cb = sizeof(device);

	EnumDisplayDevicesW(NULL, display_id, &device, 0);

	int i = 0;

	DEVMODEW dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);

	DEVMODEW selected_mode;
	selected_mode.dmSize = 0;

	unsigned int size_diff, least_size_diff = UINT_MAX, rate_diff, least_rate_diff = UINT_MAX;

	while (EnumDisplaySettingsW(device.DeviceName, i, &dm)) {

		//Skip non 32-bit modes
		if (dm.dmBitsPerPel != 32)
			continue;

		//Skip unsupported modes
		if (ChangeDisplaySettingsExW(device.DeviceName, &dm, NULL, CDS_TEST, NULL) != DISP_CHANGE_SUCCESSFUL)
			continue;

		//Compare modes
		rate_diff = abs(dm.dmDisplayFrequency - p_mode.z);
		size_diff = abs((dm.dmPelsWidth - p_mode.x) * (dm.dmPelsWidth - p_mode.x) +
						(dm.dmPelsHeight - p_mode.y) * (dm.dmPelsHeight - p_mode.y));

		if ((size_diff < least_size_diff) || (size_diff == least_size_diff && rate_diff < least_rate_diff)) {
			CopyMemory(&selected_mode, &dm, sizeof(dm));
			least_size_diff = size_diff;
			least_rate_diff = rate_diff;
		}

		ZeroMemory(&dm, sizeof(dm));
		dm.dmSize = sizeof(dm);
		i++;
	}

	if (selected_mode.dmSize != 0) {
		CopyMemory(&current_mode->native, &selected_mode, sizeof(selected_mode));
		_set_video_mode(current_mode);
	}
};

FSControllerNative::FSControllerNative() {

	native_mode = new FSControllerNative::VideoMode();
	current_mode = new FSControllerNative::VideoMode();

	ZeroMemory(&current_mode->native, sizeof(current_mode->native));
	ZeroMemory(&native_mode->native, sizeof(native_mode->native));
};

FSControllerNative::~FSControllerNative() = default;
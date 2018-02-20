/*************************************************************************/
/*  fs_controller.cpp                                                    */
/*************************************************************************/

#include "fs_controller.h"

void FSController::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_screen_mode_list", "screen"), &FSController::get_screen_mode_list, DEFVAL(-1));

	ClassDB::bind_method(D_METHOD("change_mode_windowed", "size", "position", "screen"), &FSController::change_mode_windowed, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("change_mode_borderless_fullscreen", "screen"), &FSController::change_mode_borderless_fullscreen, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("change_mode_exclusive_fullscreen", "mode", "screen"), &FSController::change_mode_exclusive_fullscreen, DEFVAL(0));

};

void FSController::_notification(int p_what) {

	switch (p_what) {
		case MainLoop::NOTIFICATION_WM_FOCUS_OUT: {

			native_hnd.deactivate_video_mode();
		} break;
		case MainLoop::NOTIFICATION_WM_FOCUS_IN: {

			native_hnd.activate_video_mode();
		} break;
		default: {
			//NOP
		};
	}
};

Array FSController::get_screen_mode_list(int p_screen) const {

	if (p_screen == -1) {
		p_screen = OS::get_singleton()->get_current_screen();
	}

	Array vmarr;
	List<Vector3> vmlist;
	native_hnd.get_screen_mode_list_native(&vmlist, p_screen);
	for (List<Vector3>::Element *E = vmlist.front(); E; E = E->next()) {
		vmarr.push_back(E->get());
	}

	return vmarr;
};

void FSController::change_mode_windowed(const Vector2 &p_mode, const Point2 &p_position, int p_screen) {

	OS::get_singleton()->set_window_resizable(true);
	native_hnd.restore_video_mode();
	OS::get_singleton()->set_window_fullscreen(false);
	OS::get_singleton()->set_current_screen(p_screen);
	OS::get_singleton()->set_window_size(Vector2(p_mode.x, p_mode.y));
	OS::get_singleton()->set_window_resizable(false);
};

void FSController::change_mode_borderless_fullscreen(int p_screen) {

	OS::get_singleton()->set_window_resizable(true);
	native_hnd.restore_video_mode();
	OS::get_singleton()->set_window_fullscreen(false);
	OS::get_singleton()->set_current_screen(p_screen);
	OS::get_singleton()->set_window_fullscreen(true);
	OS::get_singleton()->set_window_resizable(false);
};

void FSController::change_mode_exclusive_fullscreen(const Vector3 &p_mode, int p_screen) {

	OS::get_singleton()->set_window_resizable(true);
	OS::get_singleton()->set_window_fullscreen(false);
	OS::get_singleton()->set_current_screen(p_screen);
	native_hnd.set_video_mode(p_mode);
	OS::get_singleton()->set_window_fullscreen(true);
	OS::get_singleton()->set_window_resizable(false);
};

FSController::FSController() {

	OS::get_singleton()->set_window_resizable(false);
};

FSController::~FSController() {

	native_hnd.restore_video_mode();
};

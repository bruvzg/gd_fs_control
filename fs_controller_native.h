/*************************************************************************/
/*  fs_controller_native.h                                               */
/*************************************************************************/

#ifndef FS_CTL_NAT_H
#define FS_CTL_NAT_H

#include "core/list.h"
#include "core/math/vector3.h"

class FSControllerNative {

private:
	struct VideoMode;

	VideoMode *native_mode;
	VideoMode *current_mode;

	void _set_video_mode(VideoMode *p_mode);

public:
	void get_screen_mode_list_native(List<Vector3> *p_list, int p_screen) const;
	
	void activate_video_mode();
	void deactivate_video_mode();
	void restore_video_mode();
	void set_video_mode(const Vector3 &p_mode);

	FSControllerNative();
	~FSControllerNative();
};

#endif
/*************************************************************************/
/*  fs_controller.h                                                      */
/*************************************************************************/

#ifndef FS_CTL_H
#define FS_CTL_H

#include "core/os/input.h"
#include "core/os/os.h"

#include "core/list.h"
#include "core/math/vector3.h"
#include "scene/main/node.h"
#include "fs_controller_native.h"

void _get_screen_mode_list_native(List<Vector3> *p_list, int p_screen);

class FSController : public Node {

	GDCLASS(FSController, Node);

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	FSControllerNative native_hnd;

public:
	Array get_screen_mode_list(int p_screen) const;

	void change_mode_windowed(const Vector2 &p_size, const Point2 &p_position, int p_screen);
	void change_mode_borderless_fullscreen(int p_screen);
	void change_mode_exclusive_fullscreen(const Vector3 &p_mode, int p_screen);

	FSController();
	~FSController();
};

#endif
/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"
#include "fs_controller.h"

static FSController *fs_controller = NULL;

void register_fs_control_types() {

	fs_controller = memnew(FSController);

	ClassDB::register_class<FSController>();

	Engine::get_singleton()->add_singleton(Engine::Singleton("FSController", FSController::get_singleton()));
};

void unregister_fs_control_types() {

	memdelete(fs_controller);
	fs_controller = NULL;
};

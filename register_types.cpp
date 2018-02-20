/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"
#include "fs_controller.h"

void register_fs_control_types() {

	ClassDB::register_class<FSController>();
};

void unregister_fs_control_types() {

	//NOP
};

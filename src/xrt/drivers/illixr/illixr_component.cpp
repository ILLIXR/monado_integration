extern "C" {
//#include "ogl/ogl_api.h"
//#include <GLFW/glfw3.h>
#include "xrt/xrt_device.h"
}

#include <iostream>
#include "common/plugin.hpp"
#include "common/phonebook.hpp"
#include "common/switchboard.hpp"
#include "common/data_format.hpp"
#include "common/pose_prediction.hpp"

using namespace ILLIXR;

/// Dummy plugin class for an instance during phonebook registration
class illixr_plugin : public plugin {
public:
	illixr_plugin(std::string name_, phonebook* pb_)
		: plugin{name_, pb_}
		, sb{pb->lookup_impl<switchboard>()}
		, sb_pose{pb->lookup_impl<const pose_prediction>()}
		, sb_eyebuffer{sb->publish<rendered_frame_alt>("eyebuffer")}
	{ }

	const std::shared_ptr<switchboard> sb;
	const std::shared_ptr<const pose_prediction> sb_pose;
	const std::unique_ptr<writer<rendered_frame_alt>> sb_eyebuffer;
	pose_type prev_pose; /* stores a copy of pose_type each time illixr_read_pose() is called */
	std::chrono::time_point<std::chrono::system_clock> sample_time; /* when prev_pose was stored */
};

static illixr_plugin* illixr_plugin_obj = nullptr;

extern "C" plugin* illixr_monado_create_plugin(phonebook* pb) {
	// "borrowed" from common/plugin.hpp PLUGIN_MAIN
	illixr_plugin_obj = new illixr_plugin {"illixr_plugin", pb};
	illixr_plugin_obj->start();
	return illixr_plugin_obj;
}

extern "C" struct xrt_pose illixr_read_pose() {
	assert(illixr_plugin_obj && "illixr_plugin_obj must be initialized first.");

	const pose_type pose = illixr_plugin_obj->sb_pose->get_fast_pose();
	if (!illixr_plugin_obj->sb_pose->fast_pose_reliable()) {
		std::cerr << "Pose not reliable yet; returning best guess" << std::endl;
	}
	struct xrt_pose ret;

	// record when the pose was read for use in write_frame
	illixr_plugin_obj->sample_time = std::chrono::system_clock::now();

	ret.orientation.x = pose.orientation.x();
	ret.orientation.y = pose.orientation.y();
	ret.orientation.z = pose.orientation.z();
	ret.orientation.w = pose.orientation.w();
	ret.position.x = pose.position.x();
	ret.position.y = pose.position.y();
	ret.position.z = pose.position.z();

	// store pose in static variable for use in write_frame
	illixr_plugin_obj->prev_pose = pose; // copy member variables

	return ret;
}

extern "C" void illixr_write_frame(unsigned int left,
								   unsigned int right) {
	assert(illixr_plugin_obj && "illixr_plugin_obj must be initialized first.");

	rendered_frame_alt* frame = new rendered_frame_alt();

	frame->texture_handles[0] = left;
	frame->texture_handles[1] = right;
	frame->render_pose = illixr_plugin_obj->prev_pose;
	frame->sample_time = illixr_plugin_obj->sample_time;

	illixr_plugin_obj->sb_eyebuffer->put(frame);
}

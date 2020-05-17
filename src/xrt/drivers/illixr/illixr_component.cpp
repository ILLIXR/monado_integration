extern "C" {
//#include "ogl/ogl_api.h"
//#include <GLFW/glfw3.h>
#include "xrt/xrt_device.h"
}

#include "common/plugin.hh"
#include "common/switchboard.hh"
#include "common/data_format.hh"

using namespace ILLIXR;
using std::unique_ptr;

/// Dummy plugin class for an instance during phonebook registration
class illixr_plugin : public plugin {
/*private:
	virtual void _p_start() override {}
	virtual void _p_stop() override {}*/
};

static phonebook *pb;
static switchboard *sb;
static unique_ptr<writer<rendered_frame_alt>> sb_eyebuffer;
static unique_ptr<reader_latest<pose_type>> sb_pose;

static pose_type prev_pose; /* stores a copy of pose_type each time illixr_read_pose() is called */
static std::chrono::time_point<std::chrono::system_clock> sample_time; /* when prev_pose was stored */

extern "C" void* illixr_monado_create_plugin(void *pbptr) {
	pb = (phonebook *)pbptr;
	sb = pb->lookup_impl<switchboard>();
	sb_eyebuffer = sb->publish<rendered_frame_alt>("eyebuffer");
	sb_pose = sb->subscribe_latest<pose_type>("fast_pose");
	// sb_config = sb->subscribe_latest<global_config>("global_config");
	return new illixr_plugin;
}

extern "C" struct xrt_pose illixr_read_pose() {
	const pose_type* pose = sb_pose->get_latest_ro();
	struct xrt_pose ret;
	if (!pose) {
		// Query failed. Return default pose
		ret = {
			.orientation = {.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f},
			.position = {.x = 0.0f, .y = 0.0f, .z = 0.0f}
		};
		return ret;
	}

	// record when the pose was read for use in write_frame
	sample_time = std::chrono::system_clock::now();

	ret.orientation.x = pose->orientation.x();
	ret.orientation.y = pose->orientation.y();
	ret.orientation.z = pose->orientation.z();
	ret.orientation.w = pose->orientation.w();
	ret.position.x = pose->position.x();
	ret.position.y = pose->position.y();
	ret.position.z = pose->position.z();

	// store pose in static variable for use in write_frame
	prev_pose = *pose; // copy member variables

	return ret;
}

extern "C" void illixr_write_frame(unsigned int left,
								   unsigned int right) {
	rendered_frame_alt* frame = new rendered_frame_alt();

	frame->texture_handles[0] = left;
	frame->texture_handles[1] = right;
	frame->render_pose = prev_pose;
	frame->sample_time = sample_time;

	sb_eyebuffer->put(frame);
}

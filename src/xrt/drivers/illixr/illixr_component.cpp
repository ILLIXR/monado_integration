extern "C" {
#include "xrt/xrt_device.h"
}

#include "common/component.hh"
#include "common/switchboard.hh"
#include "common/data_format.hh"

using namespace ILLIXR;
using std::unique_ptr;

/// Dummy component.
class illixr_component : public component {
private:
	virtual void _p_start() override {}
	virtual void _p_stop() override {}
};

static switchboard *sb;
static unique_ptr<writer<rendered_frame>> sb_eyebuffer;
static unique_ptr<reader_latest<pose_sample>> sb_pose;
static unique_ptr<reader_latest<global_config>> sb_config;

static pose_sample prev_pose; /* stores a copy of pose_sample each time illixr_read_pose() is called */
static std::chrono::time_point<std::chrono::system_clock> sample_time; /* when prev_pose was stored */

extern "C" void* illixr_monado_create_component(void *sbptr) {
	sb = (switchboard *)sbptr;
	sb_eyebuffer = sb->publish<rendered_frame>("eyebuffer");
	sb_pose = sb->subscribe_latest<pose_sample>("pose");
	sb_config = sb->subscribe_latest<global_config>("global_config");
	return new illixr_component;
}

extern "C" struct xrt_pose illixr_read_pose() {
	const pose_sample* pose = sb_pose->get_latest_ro();
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

	ret.orientation.x = pose->pose.orientation.x;
	ret.orientation.y = pose->pose.orientation.y;
	ret.orientation.z = pose->pose.orientation.z;
	ret.orientation.w = pose->pose.orientation.w;
	ret.position.x = pose->pose.position.x;
	ret.position.y = pose->pose.position.y;
	ret.position.z = pose->pose.position.z;

	// store pose in static variable for use in write_frame
	prev_pose = *pose; // copy member variables


	return ret;
}

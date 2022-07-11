extern "C" {
//#include "ogl/ogl_api.h"
//#include <GLFW/glfw3.h>
#include "xrt/xrt_device.h"
}

#include <iostream>
#include <array>

#include "common/plugin.hpp"
#include "common/phonebook.hpp"
#include "common/switchboard.hpp"
#include "common/data_format.hpp"
#include "common/extended_window.hpp"
#include "common/pose_prediction.hpp"
#include "common/relative_clock.hpp"

using namespace ILLIXR;

static constexpr duration VSYNC_PERIOD {freq2period(60.0)};

/// Dummy plugin class for an instance during phonebook registration
class illixr_plugin : public plugin {
public:
	illixr_plugin(std::string name_, phonebook* pb_)
		: plugin{name_, pb_}
		, sb{pb->lookup_impl<switchboard>()}
		, sb_pose{pb->lookup_impl<pose_prediction>()}
		, _m_clock{pb->lookup_impl<RelativeClock>()}
		, sb_image_handle{sb->get_writer<image_handle>("image_handle")}
		, sb_eyebuffer{sb->get_writer<rendered_frame>("eyebuffer")}
		, sb_vsync_estimate{sb->get_reader<switchboard::event_wrapper<time_point>>("vsync_estimate")}
	{}

	const std::shared_ptr<switchboard> sb;
	const std::shared_ptr<pose_prediction> sb_pose;
	std::shared_ptr<RelativeClock> _m_clock;
	switchboard::writer<image_handle> sb_image_handle;
	switchboard::writer<rendered_frame> sb_eyebuffer;
	switchboard::reader<switchboard::event_wrapper<time_point>> sb_vsync_estimate;
	fast_pose_type prev_pose; /* stores a copy of pose each time illixr_read_pose() is called */
	time_point sample_time; /* when prev_pose was stored */
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

	if (!illixr_plugin_obj->sb_pose->fast_pose_reliable()) {
		std::cerr << "Pose not reliable yet; returning best guess" << std::endl;
	}
	struct xrt_pose ret;
	const fast_pose_type fast_pose = illixr_plugin_obj->sb_pose->get_fast_pose();
	const pose_type pose = fast_pose.pose;

	// record when the pose was read for use in write_frame
	illixr_plugin_obj->sample_time = illixr_plugin_obj->_m_clock->now();

	ret.orientation.x = pose.orientation.x();
	ret.orientation.y = pose.orientation.y();
	ret.orientation.z = pose.orientation.z();
	ret.orientation.w = pose.orientation.w();
	ret.position.x = pose.position.x();
	ret.position.y = pose.position.y();
	ret.position.z = pose.position.z();

	// store pose in static variable for use in write_frame
	illixr_plugin_obj->prev_pose = fast_pose; // copy member variables

	return ret;
}

extern "C" void illixr_publish_gl_image_handle(GLuint handle, int num_images, int swapchain_index) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");
	illixr_plugin_obj->sb_image_handle.put(illixr_plugin_obj->sb_image_handle.allocate<image_handle>(
		image_handle {
			handle,
			num_images,
			swapchain_index
		}
	));
}



extern "C" void illixr_publish_vk_image_handle(int fd, int64_t format, size_t size, uint32_t width, uint32_t height, uint32_t num_images, uint32_t swapchain_index) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");
	illixr_plugin_obj->sb_image_handle.put(illixr_plugin_obj->sb_image_handle.allocate<image_handle>(
		image_handle {
			fd,
			format,
			size,
			width,
			height,
			num_images,
			swapchain_index
		}
	));
}

extern "C" void illixr_write_frame(int left,
								   int right) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");

    static unsigned int buffer_to_use = 0U;

	illixr_plugin_obj->sb_eyebuffer.put(illixr_plugin_obj->sb_eyebuffer.allocate<rendered_frame>(
	    rendered_frame {
	        std::array<int, 2>{ left, right },
	        std::array<GLuint, 2>{ buffer_to_use, buffer_to_use }, // .data() deleted FIXME
            illixr_plugin_obj->prev_pose,
            illixr_plugin_obj->sample_time,
			illixr_plugin_obj->_m_clock->now()
        }
    ));

    buffer_to_use = (buffer_to_use == 0U) ? 1U : 0U;
}

extern "C" int64_t illixr_get_vsync_ns() {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");

    switchboard::ptr<const switchboard::event_wrapper<time_point>> vsync_estimate = illixr_plugin_obj->sb_vsync_estimate.get_ro_nullable();
	
	time_point target_time = vsync_estimate == nullptr ? illixr_plugin_obj->_m_clock->now() + VSYNC_PERIOD : **vsync_estimate;

	return std::chrono::nanoseconds{target_time.time_since_epoch()}.count();
}

extern "C" int64_t illixr_get_now_ns() {
	//assert(illixr_plugin_obj && "illixr_plugin_obj must be initialized first.");
	return std::chrono::duration_cast<std::chrono::nanoseconds>((illixr_plugin_obj->_m_clock->now()).time_since_epoch()).count();
}

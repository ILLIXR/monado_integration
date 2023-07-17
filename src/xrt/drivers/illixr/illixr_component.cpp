extern "C" {
//#include "ogl/ogl_api.h"
//#include <GLFW/glfw3.h>
#include "GL/gl.h"
#include "xrt/xrt_device.h"
}

#include <iostream>
#include <array>

#include "os/os_threading.h"

#include "common/plugin.hpp"
#include "common/phonebook.hpp"
#include "common/switchboard.hpp"
#include "common/data_format.hpp"
#include "common/pose_prediction.hpp"
#include "common/relative_clock.hpp"

using namespace ILLIXR;

/// Dummy plugin class for an instance during phonebook registration
class illixr_plugin : public plugin {
public:
	illixr_plugin(std::string name_, phonebook* pb_)
		: plugin{name_, pb_}
		, sb{pb->lookup_impl<switchboard>()}
		, sb_pose{pb->lookup_impl<pose_prediction>()}
		, _m_clock{pb->lookup_impl<RelativeClock>()}
		, sb_image_handle{sb->get_writer<image_handle>("image_handle")}
		, sb_semaphore_handle{sb->get_writer<semaphore_handle>("semaphore_handle")}
		, sb_eyebuffer{sb->get_writer<rendered_frame>("eyebuffer")}
		, sb_vsync_estimate{sb->get_writer<switchboard::event_wrapper<time_point>>("vsync_estimate")}
		, sb_signal_quad{sb->get_reader<signal_to_quad>("signal_quad")}
//		, ullong signal_quad{0}
	{
		signal_quad = 0;
	}

	const std::shared_ptr<switchboard> sb;
	const std::shared_ptr<pose_prediction> sb_pose;
	std::shared_ptr<RelativeClock> _m_clock;
	switchboard::writer<image_handle> sb_image_handle;
	switchboard::writer<semaphore_handle> sb_semaphore_handle;
	switchboard::writer<rendered_frame> sb_eyebuffer;
	switchboard::writer<switchboard::event_wrapper<time_point>> sb_vsync_estimate;
	switchboard::reader<signal_to_quad> sb_signal_quad;
	ullong signal_quad;
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

extern "C" struct xrt_pose illixr_read_pose(bool monado_Call) {
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
	if (!monado_Call){
		illixr_plugin_obj->prev_pose = fast_pose; // copy member variables
	}

	return ret;
}

extern "C" void illixr_publish_vk_image_handle(int fd, int64_t format, size_t size, uint32_t width, uint32_t height, uint32_t num_images, int usage) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");
	
	swapchain_usage image_usage;
	switch (usage) {
		case 0: {
			image_usage = swapchain_usage::LEFT_SWAPCHAIN;
			break;
		}
		case 1: {
			image_usage = swapchain_usage::RIGHT_SWAPCHAIN;
			break;
		}
		case 2: {
			image_usage = swapchain_usage::LEFT_RENDER;
			break;
		}
		case 3: {
			image_usage = swapchain_usage::RIGHT_RENDER;
			break;
		}
		default: {
			image_usage = swapchain_usage::NA;
			assert(false && "Invalid swapchain usage!");
		}
	}

	illixr_plugin_obj->sb_image_handle.put(illixr_plugin_obj->sb_image_handle.allocate<image_handle>(
		image_handle {
			fd,
			format,
			size,
			width,
			height,
			num_images,
			image_usage
		}
	));
}

extern "C" void illixr_write_frame(GLuint left,
								   GLuint right) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");

    static unsigned int buffer_to_use = 0U;

	illixr_plugin_obj->sb_eyebuffer.put(illixr_plugin_obj->sb_eyebuffer.allocate<rendered_frame>(
	    rendered_frame {
	        std::array<GLuint, 2>{ left, right },
	        std::array<GLuint, 2>{ buffer_to_use, buffer_to_use }, // .data() deleted FIXME
            illixr_plugin_obj->prev_pose,
            illixr_plugin_obj->sample_time,
			illixr_plugin_obj->_m_clock->now()
        }
    ));

    buffer_to_use = (buffer_to_use == 0U) ? 1U : 0U;

	switchboard::ptr<const signal_to_quad> signal = illixr_plugin_obj->sb_signal_quad.get_ro_nullable();
	while(signal == nullptr || signal->seq <= illixr_plugin_obj->signal_quad ) {
		signal = illixr_plugin_obj->sb_signal_quad.get_ro_nullable();
	}
	illixr_plugin_obj->signal_quad = signal->seq;
}

extern "C" void illixr_estimate_vsync_ns(uint64_t estimated_vsync) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");

	uint64_t now_ns = os_monotonic_get_ns();
	duration time_to_vsync = std::chrono::nanoseconds(estimated_vsync - now_ns);
    illixr_plugin_obj->sb_vsync_estimate.put(illixr_plugin_obj->sb_vsync_estimate.allocate<switchboard::event_wrapper<time_point>>(illixr_plugin_obj->_m_clock->now() + time_to_vsync));
}

extern "C" int64_t illixr_get_now_ns() {
	assert(illixr_plugin_obj && "illixr_plugin_obj must be initialized first.");
	return std::chrono::duration_cast<std::chrono::nanoseconds>((illixr_plugin_obj->_m_clock->now()).time_since_epoch()).count();
}

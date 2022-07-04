extern "C" {
//#include "ogl/ogl_api.h"
//#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <vulkan/vulkan.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_compositor.h"
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
		, xwin{pb->lookup_impl<xlib_gl_extended_window>()}
		, _m_clock{pb->lookup_impl<RelativeClock>()}
		, sb_image_handle{sb->get_writer<image_handle>("image_handle")}
		, sb_eyebuffer{sb->get_writer<rendered_frame>("eyebuffer")}
		, sb_vsync_estimate{sb->get_reader<switchboard::event_wrapper<time_point>>("vsync_estimate")}
	{
		// these were some tests to try to perform the Vulkan interop inside of the Monado plugin
		// create an OpenGL context here that's shared with the extended window
        GLint attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        XVisualInfo *vi;
 
        /* open display */
        if ( ! (dpy = XOpenDisplay(NULL)) ) {
                fprintf(stderr, "cannot connect to X server\n\n");
                exit(1);
        }
		std::cout << glGetError() << std::endl;
 
        /* get root window */
        root = DefaultRootWindow(dpy);
 
        /* get visual matching attr */
        if( ! (vi = glXChooseVisual(dpy, 0, attr)) ) {
                fprintf(stderr, "no appropriate visual found\n\n");
                exit(1);
        }
		std::cout << glGetError() << std::endl;
 
        /* create a context using the root window and share it with extended window's context */
        if ( ! (glc = glXCreateContext(dpy, vi, xwin->glc, GL_TRUE)) ){
                fprintf(stderr, "failed to create context\n\n");
                exit(1);
        }
		std::cout << glGetError() << std::endl;
	}

	const std::shared_ptr<switchboard> sb;
	const std::shared_ptr<pose_prediction> sb_pose;
	const std::shared_ptr<xlib_gl_extended_window> xwin;
	std::shared_ptr<RelativeClock> _m_clock;
	switchboard::writer<image_handle> sb_image_handle;
	switchboard::writer<rendered_frame> sb_eyebuffer;
	switchboard::reader<switchboard::event_wrapper<time_point>> sb_vsync_estimate;
	fast_pose_type prev_pose; /* stores a copy of pose each time illixr_read_pose() is called */
	time_point sample_time; /* when prev_pose was stored */
	
	// used for VulkanGL interop
	Display* dpy;
	Window root;
	GLXContext glc;
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

extern "C" void illixr_publish_gl_image_handle(unsigned int handle, int num_images, int swapchain_index) {
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");
	illixr_plugin_obj->sb_image_handle.put(illixr_plugin_obj->sb_image_handle.allocate<image_handle>(
		image_handle {
			handle,
			num_images,
			swapchain_index
		}
	));
}



extern "C" void illixr_publish_vk_image_handle(int fd, uint64_t size, uint64_t format, int width, int height, int num_images, int swapchain_index) {
	// these were some tests for VulkanGL interop inside of the Monado plugin
	assert(illixr_plugin_obj != nullptr && "illixr_plugin_obj must be initialized first.");
	const bool gl_result = (bool) glXMakeCurrent(illixr_plugin_obj->dpy, illixr_plugin_obj->root, illixr_plugin_obj->glc);
	assert(gl_result && "glXMakeCurrent should not fail");
	assert(GLEW_EXT_memory_object_fd && "[timewarp_gl] Missing object memory extensions for Vulkan-GL interop");

	// first convert the VK format to GL
	uint64_t gl_format = 0;
	switch (format) {
		case VK_FORMAT_R8G8B8A8_UNORM: {
			gl_format = GL_RGBA8;
			break;
		}
		case VK_FORMAT_R8G8B8A8_SRGB: {
			gl_format = GL_SRGB8_ALPHA8;
			break;
		}
	}

	// now get the memory handle of the vulkan object
	GLuint memory_handle;
	GLint dedicated = GL_TRUE;
	glCreateMemoryObjectsEXT(1, &memory_handle);
	assert(glIsMemoryObjectEXT(memory_handle) && "GL memory handle must be created correctly");
	glMemoryObjectParameterivEXT(memory_handle, GL_DEDICATED_MEMORY_OBJECT_EXT, &dedicated);
	printf("Importing memory\n");
	printf("Swapchain index: %d\n", swapchain_index);
	printf("Num images: %d\n", num_images);
	glImportMemoryFdEXT(memory_handle, size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);

	// then use the imported memory as the opengl texture
	GLuint gl_handle;
	glGenTextures(1, &gl_handle);
	glBindTexture(GL_TEXTURE_2D, gl_handle);
	glTextureStorageMem2DEXT(gl_handle, 1, gl_format, width, height, memory_handle, 0);

	illixr_plugin_obj->sb_image_handle.put(illixr_plugin_obj->sb_image_handle.allocate<image_handle>(
		image_handle {
			// fd,
			// format,
			// size,
			// width,
			// height,
			gl_handle,
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

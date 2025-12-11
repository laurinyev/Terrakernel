#ifndef STD_FB_HPP
#define STD_FB_HPP 1

#include <cstddef>

void fb_test_iter();

namespace driver::video::std_fb {

void initialise();
void poll_fb();
void fb_play_video_iter(void* video_base, int fps, size_t frame_width, size_t frame_height, size_t total_frames);

}

#endif
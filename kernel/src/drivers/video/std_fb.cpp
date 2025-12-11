#include "std_fb.hpp"
#include <tmpfs/tmpfs.hpp>
#include <limine.h>
#include <drivers/serial/print.hpp>
#include <mem/mem.hpp>
#include <drivers/timers/pit.hpp>

static volatile uint32_t* fb;
static uint16_t x, y;
static uint16_t pitch;
static size_t fb_size;

static uint32_t* backbuffer = nullptr;

extern volatile struct limine_framebuffer_request fb_request;

static bool initialised = false;

void fb_test_iter() {
    if (!initialised) return;

    int fd = tmpfs::open("/dev/fb", O_RDWR);
    if (fd < 0) return;

    static uint16_t shift = 0;

    for (uint16_t row = 0; row < y; row++) {
        for (uint16_t col = 0; col < x; col++) {
            uint8_t hue = ((row + shift) * 6 / y) % 6;
            uint8_t bright = 255 - (col * 255 / x);

            uint8_t r = 0, g = 0, b = 0;

            switch (hue) {
                case 0: r = 255; g = col * 255 / x; b = 0; break;
                case 1: r = 255 - (row * 255 / y); g = 255; b = 0; break;
                case 2: r = 0; g = 255; b = col * 255 / x; break;
                case 3: r = 0; g = 255 - (row * 255 / y); b = 255; break;
                case 4: r = col * 255 / x; g = 0; b = 255; break;
                case 5: r = 255; g = 0; b = 255 - (row * 255 / y); break;
            }

            r = r * bright / 255;
            g = g * bright / 255;
            b = b * bright / 255;

            backbuffer[row * x + col] = (r << 16) | (g << 8) | b;
        }
    }

    tmpfs::pwrite(fd, backbuffer, fb_size, 0);

    shift = ((shift + 1) % y);

    tmpfs::close(fd);
}

namespace driver::video::std_fb {

void initialise() {
    fb = reinterpret_cast<volatile uint32_t*>(fb_request.response->framebuffers[0]->address);
    x = fb_request.response->framebuffers[0]->width;
    y = fb_request.response->framebuffers[0]->height;
    pitch = fb_request.response->framebuffers[0]->pitch;

    fb_size = pitch * y;

    backbuffer = (uint32_t*)mem::heap::malloc(x * y * sizeof(uint32_t));
    if (!backbuffer) {
        Log::errf("Failed to allocate framebuffer backbuffer... Halting");
        asm volatile("cli;hlt;");
    }

    int fd = tmpfs::open("/dev/fb", O_CREAT | O_RDWR | O_BUILTIN_DEVICE_FILE, 0644, "/dev/fb");
    if (fd < 0) {
        Log::errf("Failed to create framebuffer device... Halting");
        asm volatile("cli;hlt;");
    }

    tmpfs::pwrite(fd, (const void*)fb, fb_size, 0);
    tmpfs::close(fd);

    initialised = true;
}

void poll_fb() {
    if (!initialised) return;

    int fd = tmpfs::open("/dev/fb", O_RDWR);
    tmpfs::pread(fd, (void*)fb, fb_size, 0);
    tmpfs::lseek(fd, 0, SEEK_SET);
    tmpfs::close(fd);
}

static size_t fb_iter_frame_index = 0;
static int fb_fd = -1;

void fb_play_video_iter(void* video_base, int fps, size_t frame_width, size_t frame_height, size_t total_frames) {
    if (!initialised) return;

    if (fb_fd < 0) {
        fb_fd = tmpfs::open("/dev/fb", O_RDWR);
        if (fb_fd < 0) return;
    }

    if (fb_iter_frame_index >= total_frames) return;

    size_t frame_size = frame_width * frame_height * 4;
    uint8_t* src = reinterpret_cast<uint8_t*>(video_base) + fb_iter_frame_index * frame_size;
    uint8_t* dst = reinterpret_cast<uint8_t*>(backbuffer);

    size_t copy_width = (frame_width < x) ? frame_width : x;
    size_t copy_height = (frame_height < y) ? frame_height : y;

    for (size_t row = 0; row < copy_height; row++) {
        mem::memcpy(dst + row * x * 4, src + row * frame_width * 4, copy_width * 4);
        for (size_t col = copy_width; col < x; col++) {
            reinterpret_cast<uint32_t*>(dst + row * x * 4)[col] = 0;
        }
    }
    for (size_t row = copy_height; row < y; row++) {
        mem::memset(dst + row * x * 4, 0, x * 4);
    }

    tmpfs::pwrite(fb_fd, backbuffer, fb_size, 0);

    fb_iter_frame_index++;

    driver::pit::sleep_ms(1000 / fps);

    poll_fb();
}

} 

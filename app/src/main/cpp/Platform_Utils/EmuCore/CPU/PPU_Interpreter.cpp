#include "PPU_Interpreter.h"
#include <cstring>
#include <cmath>

namespace com::arenax3 {

PPU_Interpreter::PPU_Interpreter()
    : frame_buffer(nullptr)
    , width(0)
    , height(0)
    , buffer_size(0)
    , current_frame(0)
    , fps_target(60)
    , fps_actual(0.0f)
    , vblank_counter(0)
    , hblank_counter(0)
    , scanline(0)
    , cycle_count(0)
    , render_enabled(true)
    , vblank_interrupt_enabled(false)
    , hblank_interrupt_enabled(false)
    , vblank_pending(false)
    , hblank_pending(false)
    , bg_color(0xFF000000)
    , layers_enabled(0x0F)
    , h_resolution(640)
    , v_resolution(480)
    , frame_counter(0)
{
    // Initialize color LUT with default values
    for (int i = 0; i < 256; i++) {
        color_lut[i] = 0xFF000000 | (i << 16) | (i << 8) | i;
    }
}

PPU_Interpreter::~PPU_Interpreter() {
    if (frame_buffer) {
        delete[] frame_buffer;
        frame_buffer = nullptr;
    }
}

bool PPU_Interpreter::initialize(uint32_t screen_width, uint32_t screen_height) {
    width = screen_width;
    height = screen_height;
    h_resolution = screen_width;
    v_resolution = screen_height;
    buffer_size = width * height;
    
    if (frame_buffer) {
        delete[] frame_buffer;
    }
    
    frame_buffer = new uint32_t[buffer_size];
    if (!frame_buffer) {
        return false;
    }
    
    clear_frame_buffer();
    return true;
}

void PPU_Interpreter::shutdown() {
    if (frame_buffer) {
        delete[] frame_buffer;
        frame_buffer = nullptr;
    }
    width = 0;
    height = 0;
    buffer_size = 0;
}

void PPU_Interpreter::reset() {
    current_frame = 0;
    vblank_counter = 0;
    hblank_counter = 0;
    scanline = 0;
    cycle_count = 0;
    vblank_pending = false;
    hblank_pending = false;
    frame_counter = 0;
    fps_actual = 0.0f;
    clear_frame_buffer();
    
    if (frame_buffer) {
        memset(frame_buffer, 0, buffer_size * sizeof(uint32_t));
    }
}

void PPU_Interpreter::render_frame() {
    if (!render_enabled || !frame_buffer) {
        return;
    }
    
    // Simulate PPU rendering pipeline
    for (uint32_t y = 0; y < height; y++) {
        // Horizontal blank period at start of scanline
        hblank_pending = true;
        if (hblank_interrupt_enabled) {
            trigger_hblank_interrupt();
        }
        
        for (uint32_t x = 0; x < width; x++) {
            uint32_t pixel = bg_color;
            
            // Render layers in priority order (background -> sprite -> overlay)
            if (layers_enabled & 0x01) {
                pixel = render_background_pixel(x, y);
            }
            if (layers_enabled & 0x02) {
                uint32_t sprite_pixel = render_sprite_pixel(x, y);
                if ((sprite_pixel & 0xFF000000) != 0) {
                    pixel = sprite_pixel;
                }
            }
            if (layers_enabled & 0x04) {
                uint32_t overlay_pixel = render_overlay_pixel(x, y);
                if ((overlay_pixel & 0xFF000000) != 0) {
                    pixel = overlay_pixel;
                }
            }
            
            frame_buffer[y * width + x] = pixel;
            cycle_count++;
        }
        
        hblank_pending = false;
        
        // End of scanline
        if (y == height - 1) {
            vblank_pending = true;
            if (vblank_interrupt_enabled) {
                trigger_vblank_interrupt();
            }
        }
    }
    
    current_frame++;
    frame_counter++;
    
    // Update FPS counter every second
    if (frame_counter >= fps_target) {
        fps_actual = static_cast<float>(frame_counter);
        frame_counter = 0;
    }
}

void PPU_Interpreter::clear_frame_buffer() {
    if (!frame_buffer) {
        return;
    }
    
    for (uint32_t i = 0; i < buffer_size; i++) {
        frame_buffer[i] = bg_color;
    }
}

void PPU_Interpreter::set_background_color(uint32_t color) {
    bg_color = color;
}

void PPU_Interpreter::set_color_lut_entry(uint8_t index, uint32_t color) {
    if (index < 256) {
        color_lut[index] = color;
    }
}

void PPU_Interpreter::enable_layer(uint8_t layer_mask, bool enable) {
    if (enable) {
        layers_enabled |= layer_mask;
    } else {
        layers_enabled &= ~layer_mask;
    }
}

void PPU_Interpreter::set_resolution(uint32_t screen_width, uint32_t screen_height) {
    if (screen_width > 0 && screen_height > 0) {
        initialize(screen_width, screen_height);
    }
}

void PPU_Interpreter::set_fps_target(uint32_t fps) {
    if (fps > 0 && fps <= 240) {
        fps_target = fps;
    }
}

uint32_t PPU_Interpreter::get_frame_buffer_size() const {
    return buffer_size;
}

uint32_t PPU_Interpreter::get_current_frame() const {
    return current_frame;
}

float PPU_Interpreter::get_actual_fps() const {
    return fps_actual;
}

uint32_t PPU_Interpreter::render_background_pixel(uint32_t x, uint32_t y) {
    // Simple checkerboard pattern for demonstration
    // In real implementation, this would fetch from tilemap/bitmap data
    uint32_t pattern = ((x / 16) + (y / 16)) % 2;
    if (pattern == 0) {
        return color_lut[64];
    } else {
        return color_lut[128];
    }
}

uint32_t PPU_Interpreter::render_sprite_pixel(uint32_t x, uint32_t y) {
    // Placeholder - would process sprite list and return pixel if sprite covers this coordinate
    // Return transparent pixel by default
    return 0x00000000;
}

uint32_t PPU_Interpreter::render_overlay_pixel(uint32_t x, uint32_t y) {
    // Placeholder for UI/overlay rendering
    // Return transparent pixel by default
    return 0x00000000;
}

void PPU_Interpreter::trigger_vblank_interrupt() {
    vblank_pending = true;
    // In a real system, this would signal the CPU
}

void PPU_Interpreter::trigger_hblank_interrupt() {
    hblank_pending = true;
    // In a real system, this would signal the CPU
}

void PPU_Interpreter::acknowledge_vblank_interrupt() {
    vblank_pending = false;
}

void PPU_Interpreter::acknowledge_hblank_interrupt() {
    hblank_pending = false;
}

void PPU_Interpreter::enable_vblank_interrupt(bool enable) {
    vblank_interrupt_enabled = enable;
}

void PPU_Interpreter::enable_hblank_interrupt(bool enable) {
    hblank_interrupt_enabled = enable;
}

bool PPU_Interpreter::is_vblank_pending() const {
    return vblank_pending;
}

bool PPU_Interpreter::is_hblank_pending() const {
    return hblank_pending;
}

void PPU_Interpreter::enable_rendering(bool enable) {
    render_enabled = enable;
}

uint32_t* PPU_Interpreter::get_frame_buffer() const {
    return frame_buffer;
}

} // namespace com::arenax3
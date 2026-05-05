#pragma once

#include <cstdint>
#include <atomic>
#include <array>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/select.h>
#include <cmath>
#include <cstring>

namespace com {
namespace arenax3 {

class Gamepad_Handler {
public:
    static constexpr int MAX_AXIS_VALUE = 32767;
    static constexpr float DEADZONE = 0.15f;
    static constexpr int DEFAULT_BUTTON_COUNT = 17;
    static constexpr int DEFAULT_AXIS_COUNT = 6;
    static constexpr std::chrono::milliseconds POLL_INTERVAL{1};
    static constexpr std::chrono::milliseconds CONNECTION_CHECK_INTERVAL{1000};
    static constexpr int MAX_RECONNECT_ATTEMPTS = 5;

    enum class Button : uint8_t {
        A = 0,
        B = 1,
        X = 2,
        Y = 3,
        LEFT_BUMPER = 4,
        RIGHT_BUMPER = 5,
        BACK = 6,
        START = 7,
        GUIDE = 8,
        LEFT_STICK = 9,
        RIGHT_STICK = 10,
        DPAD_UP = 11,
        DPAD_DOWN = 12,
        DPAD_LEFT = 13,
        DPAD_RIGHT = 14,
        HOME = 15,
        TOUCHPAD = 16
    };

    enum class Axis : uint8_t {
        LEFT_STICK_X = 0,
        LEFT_STICK_Y = 1,
        LEFT_TRIGGER = 2,
        RIGHT_STICK_X = 3,
        RIGHT_STICK_Y = 4,
        RIGHT_TRIGGER = 5,
        DPAD_X = 6,
        DPAD_Y = 7
    };

    struct GamepadState {
        std::array<float, 8> axes{};
        std::array<bool, 32> buttons{};
        std::string device_path;
        std::string device_name;
        int device_index = -1;
        bool connected = false;
        uint64_t timestamp = 0;
        uint32_t packet_counter = 0;
    };

    struct VibrationEffect {
        float left_motor = 0.0f;
        float right_motor = 0.0f;
        std::chrono::milliseconds duration{0};
        bool active = false;
    };

    using ButtonCallback = std::function<void(int device_index, Button button, bool pressed)>;
    using AxisCallback = std::function<void(int device_index, Axis axis, float value)>;
    using ConnectionCallback = std::function<void(int device_index, bool connected, const std::string& name)>;

    Gamepad_Handler() = default;
    ~Gamepad_Handler();

    Gamepad_Handler(const Gamepad_Handler&) = delete;
    Gamepad_Handler& operator=(const Gamepad_Handler&) = delete;
    Gamepad_Handler(Gamepad_Handler&&) = delete;
    Gamepad_Handler& operator=(Gamepad_Handler&&) = delete;

    bool initialize();
    void shutdown();

    void start_monitoring();
    void stop_monitoring();

    void poll_events();

    GamepadState get_state(int device_index) const;
    bool is_button_pressed(int device_index, Button button) const;
    bool was_button_just_pressed(int device_index, Button button);
    bool was_button_just_released(int device_index, Button button);
    float get_axis_value(int device_index, Axis axis) const;
    bool is_connected(int device_index) const;
    int get_connected_gamepad_count() const;

    bool set_vibration(int device_index, float left_motor, float right_motor,
                       std::chrono::milliseconds duration = std::chrono::milliseconds(0));
    void stop_vibration(int device_index);
    void stop_all_vibrations();

    void set_deadzone(float deadzone);
    float get_deadzone() const;

    void register_button_callback(ButtonCallback callback);
    void register_axis_callback(AxisCallback callback);
    void register_connection_callback(ConnectionCallback callback);
    void clear_callbacks();

    std::vector<std::string> enumerate_devices();

    static constexpr int MAX_GAMEPADS = 4;

private:
    struct DeviceInfo {
        int fd = -1;
        std::string path;
        std::string name;
        int index = -1;
        bool connected = false;
        uint8_t button_count = 0;
        uint8_t axis_count = 0;
        std::array<bool, 32> button_map{};
        std::array<int16_t, 8> axis_map{};
        std::chrono::steady_clock::time_point last_event;
        bool reconnect_pending = false;
    };

    struct ButtonStateTracker {
        bool current = false;
        bool previous = false;
    };

    mutable std::mutex mutex_;
    std::array<DeviceInfo, MAX_GAMEPADS> devices_;
    std::array<std::array<float, 8>, MAX_GAMEPADS> axis_states_{};
    std::array<std::array<bool, 32>, MAX_GAMEPADS> button_states_{};
    std::array<std::array<ButtonStateTracker, 32>, MAX_GAMEPADS> button_trackers_{};
    std::array<VibrationEffect, MAX_GAMEPADS> vibration_effects_{};
    std::array<bool, MAX_GAMEPADS> vibration_thread_running_{};
    std::array<std::unique_ptr<std::thread>, MAX_GAMEPADS> vibration_threads_;

    std::atomic<bool> monitoring_{false};
    std::atomic<bool> running_{false};
    std::atomic<float> deadzone_{DEADZONE};
    std::unique_ptr<std::thread> monitor_thread_;
    std::condition_variable cv_;

    ButtonCallback button_callback_;
    AxisCallback axis_callback_;
    ConnectionCallback connection_callback_;

    void detect_and_assign_devices();
    int find_free_slot() const;
    void open_device(const std::string& device_path);
    void close_device(int index);
    void handle_button_event(int device_index, uint8_t button, bool pressed);
    void handle_axis_event(int device_index, uint8_t axis, int16_t value);
    void process_event(int device_index, const js_event& event);
    float normalize_axis(int16_t value) const;
    void vibration_thread_func(int device_index);
    void monitor_loop();
    void attempt_reconnect(int device_index);
    bool write_effect_to_device(int device_index, int16_t left, int16_t right);
    std::string sanitize_device_name(const std::string& name) const;
};

} // namespace arenax3
} // namespace com
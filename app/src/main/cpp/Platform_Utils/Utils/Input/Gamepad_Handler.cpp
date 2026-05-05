#include "Gamepad_Handler.h"
#include <iostream>
#include <algorithm>

namespace com {
namespace arenax3 {

Gamepad_Handler::Gamepad_Handler()
    : m_isInitialized(false)
    , m_currentGamepadIndex(-1)
    , m_deadzone(0.15f)
    , m_vibrationLeft(0.0f)
    , m_vibrationRight(0.0f)
{
    for (int i = 0; i < 4; ++i) {
        m_axisStates[i] = 0.0f;
        m_prevAxisStates[i] = 0.0f;
    }
    
    for (int i = 0; i < 16; ++i) {
        m_buttonStates[i] = false;
        m_prevButtonStates[i] = false;
        m_buttonPressCount[i] = 0;
    }
}

Gamepad_Handler::~Gamepad_Handler() {
    shutdown();
}

bool Gamepad_Handler::initialize(int gamepadIndex) {
    if (m_isInitialized) {
        shutdown();
    }
    
    if (gamepadIndex < 0 || gamepadIndex >= 4) {
        std::cerr << "Invalid gamepad index: " << gamepadIndex << std::endl;
        return false;
    }
    
    m_currentGamepadIndex = gamepadIndex;
    
    // Simulate gamepad connection detection
    if (!detectGamepad()) {
        std::cerr << "No gamepad detected at index " << gamepadIndex << std::endl;
        m_currentGamepadIndex = -1;
        return false;
    }
    
    m_isInitialized = true;
    resetStates();
    
    std::cout << "Gamepad " << gamepadIndex << " initialized successfully" << std::endl;
    return true;
}

void Gamepad_Handler::shutdown() {
    if (m_isInitialized) {
        stopVibration();
        m_isInitialized = false;
        m_currentGamepadIndex = -1;
        std::cout << "Gamepad handler shutdown" << std::endl;
    }
}

bool Gamepad_Handler::update() {
    if (!m_isInitialized) {
        return false;
    }
    
    // Save previous states
    for (int i = 0; i < 16; ++i) {
        m_prevButtonStates[i] = m_buttonStates[i];
    }
    
    for (int i = 0; i < 4; ++i) {
        m_prevAxisStates[i] = m_axisStates[i];
    }
    
    // Simulate reading gamepad input (replace with actual API calls)
    readGamepadInput();
    
    // Update button press counts
    for (int i = 0; i < 16; ++i) {
        if (m_buttonStates[i]) {
            if (!m_prevButtonStates[i]) {
                m_buttonPressCount[i]++;
            }
        } else {
            m_buttonPressCount[i] = 0;
        }
    }
    
    // Apply deadzone to axes
    applyDeadzone();
    
    return true;
}

float Gamepad_Handler::getAxis(GamepadAxis axis) const {
    if (!m_isInitialized) {
        return 0.0f;
    }
    
    int index = static_cast<int>(axis);
    if (index >= 0 && index < 4) {
        return m_axisStates[index];
    }
    return 0.0f;
}

bool Gamepad_Handler::isButtonPressed(GamepadButton button) const {
    if (!m_isInitialized) {
        return false;
    }
    
    int index = static_cast<int>(button);
    if (index >= 0 && index < 16) {
        return m_buttonStates[index];
    }
    return false;
}

bool Gamepad_Handler::isButtonJustPressed(GamepadButton button) const {
    if (!m_isInitialized) {
        return false;
    }
    
    int index = static_cast<int>(button);
    if (index >= 0 && index < 16) {
        return m_buttonStates[index] && !m_prevButtonStates[index];
    }
    return false;
}

bool Gamepad_Handler::isButtonJustReleased(GamepadButton button) const {
    if (!m_isInitialized) {
        return false;
    }
    
    int index = static_cast<int>(button);
    if (index >= 0 && index < 16) {
        return !m_buttonStates[index] && m_prevButtonStates[index];
    }
    return false;
}

int Gamepad_Handler::getButtonPressCount(GamepadButton button) const {
    if (!m_isInitialized) {
        return 0;
    }
    
    int index = static_cast<int>(button);
    if (index >= 0 && index < 16) {
        return m_buttonPressCount[index];
    }
    return 0;
}

void Gamepad_Handler::setVibration(float leftMotor, float rightMotor) {
    if (!m_isInitialized) {
        return;
    }
    
    m_vibrationLeft = std::clamp(leftMotor, 0.0f, 1.0f);
    m_vibrationRight = std::clamp(rightMotor, 0.0f, 1.0f);
    
    // Simulate vibration (replace with actual API call)
    applyVibration(m_vibrationLeft, m_vibrationRight);
}

void Gamepad_Handler::stopVibration() {
    if (!m_isInitialized) {
        return;
    }
    
    m_vibrationLeft = 0.0f;
    m_vibrationRight = 0.0f;
    applyVibration(0.0f, 0.0f);
}

void Gamepad_Handler::setDeadzone(float deadzone) {
    m_deadzone = std::clamp(deadzone, 0.0f, 1.0f);
}

float Gamepad_Handler::getDeadzone() const {
    return m_deadzone;
}

void Gamepad_Handler::resetStates() {
    for (int i = 0; i < 4; ++i) {
        m_axisStates[i] = 0.0f;
        m_prevAxisStates[i] = 0.0f;
    }
    
    for (int i = 0; i < 16; ++i) {
        m_buttonStates[i] = false;
        m_prevButtonStates[i] = false;
        m_buttonPressCount[i] = 0;
    }
}

bool Gamepad_Handler::detectGamepad() const {
    // Simulate gamepad detection logic
    // Replace with actual hardware detection
    return (m_currentGamepadIndex >= 0 && m_currentGamepadIndex < 4);
}

void Gamepad_Handler::readGamepadInput() {
    // Simulate reading from actual gamepad API
    // This is a placeholder - replace with platform-specific implementation
    
    // For simulation purposes, we'll set some default states
    // In a real implementation, you would call platform APIs like:
    // - SDL_GameControllerGetAxis()
    // - XInputGetState() on Windows
    // - IOHIDManager on macOS
    // - evdev on Linux
    
    static float time = 0.0f;
    time += 0.016f; // Simulate 60 FPS
    
    // Simulate idle state (no input) as a demonstration
    // Real implementation would read actual hardware here
}

void Gamepad_Handler::applyDeadzone() {
    for (int i = 0; i < 4; ++i) {
        float absValue = std::abs(m_axisStates[i]);
        if (absValue < m_deadzone) {
            m_axisStates[i] = 0.0f;
        } else {
            // Rescale from deadzone to 1.0
            float scaled = (absValue - m_deadzone) / (1.0f - m_deadzone);
            m_axisStates[i] = (m_axisStates[i] > 0) ? scaled : -scaled;
        }
    }
}

void Gamepad_Handler::applyVibration(float left, float right) {
    // Simulate vibration output
    // Replace with actual platform-specific vibration API
    // e.g., XInputSetState(), SDL_GameControllerRumble(), etc.
    
    if (left > 0.0f || right > 0.0f) {
        // In real implementation, this would trigger hardware vibration
        // std::cout << "Vibration: Left=" << left << ", Right=" << right << std::endl;
    }
}

bool Gamepad_Handler::isConnected() const {
    return m_isInitialized && detectGamepad();
}

float Gamepad_Handler::getLeftTrigger() const {
    return getAxis(GamepadAxis::LEFT_TRIGGER);
}

float Gamepad_Handler::getRightTrigger() const {
    return getAxis(GamepadAxis::RIGHT_TRIGGER);
}

float Gamepad_Handler::getLeftStickX() const {
    return getAxis(GamepadAxis::LEFT_STICK_X);
}

float Gamepad_Handler::getLeftStickY() const {
    return getAxis(GamepadAxis::LEFT_STICK_Y);
}

float Gamepad_Handler::getRightStickX() const {
    return getAxis(GamepadAxis::RIGHT_STICK_X);
}

float Gamepad_Handler::getRightStickY() const {
    return getAxis(GamepadAxis::RIGHT_STICK_Y);
}

bool Gamepad_Handler::isLeftStickPressed() const {
    return isButtonPressed(GamepadButton::LEFT_STICK);
}

bool Gamepad_Handler::isRightStickPressed() const {
    return isButtonPressed(GamepadButton::RIGHT_STICK);
}

bool Gamepad_Handler::isDPadUpPressed() const {
    return isButtonPressed(GamepadButton::DPAD_UP);
}

bool Gamepad_Handler::isDPadDownPressed() const {
    return isButtonPressed(GamepadButton::DPAD_DOWN);
}

bool Gamepad_Handler::isDPadLeftPressed() const {
    return isButtonPressed(GamepadButton::DPAD_LEFT);
}

bool Gamepad_Handler::isDPadRightPressed() const {
    return isButtonPressed(GamepadButton::DPAD_RIGHT);
}

bool Gamepad_Handler::isFaceButtonTopPressed() const {
    return isButtonPressed(GamepadButton::FACE_TOP);
}

bool Gamepad_Handler::isFaceButtonBottomPressed() const {
    return isButtonPressed(GamepadButton::FACE_BOTTOM);
}

bool Gamepad_Handler::isFaceButtonLeftPressed() const {
    return isButtonPressed(GamepadButton::FACE_LEFT);
}

bool Gamepad_Handler::isFaceButtonRightPressed() const {
    return isButtonPressed(GamepadButton::FACE_RIGHT);
}

bool Gamepad_Handler::isLeftBumperPressed() const {
    return isButtonPressed(GamepadButton::LEFT_BUMPER);
}

bool Gamepad_Handler::isRightBumperPressed() const {
    return isButtonPressed(GamepadButton::RIGHT_BUMPER);
}

bool Gamepad_Handler::isStartPressed() const {
    return isButtonPressed(GamepadButton::START);
}

bool Gamepad_Handler::isBackPressed() const {
    return isButtonPressed(GamepadButton::BACK);
}

bool Gamepad_Handler::isGuidePressed() const {
    return isButtonPressed(GamepadButton::GUIDE);
}

} // namespace arenax3
} // namespace com
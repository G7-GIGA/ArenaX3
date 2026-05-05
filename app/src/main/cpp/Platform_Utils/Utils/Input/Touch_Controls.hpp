#ifndef TOUCH_CONTROLS_HPP
#define TOUCH_CONTROLS_HPP

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <functional>
#include <string>
#include <vector>

namespace com {
namespace arenax3 {

enum class TouchButton {
    NONE,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    JUMP,
    ACTION,
    DODGE,
    SPECIAL,
    PAUSE
};

struct TouchButtonConfig {
    sf::IntRect bounds;
    sf::Color normalColor;
    sf::Color pressedColor;
    std::string texturePath;
    bool visible;
    float opacity;
};

class TouchControls {
public:
    static TouchControls& getInstance() {
        static TouchControls instance;
        return instance;
    }

    void initialize(const sf::Vector2u& screenSize);
    void handleTouchEvent(const sf::Event& event);
    void update(float deltaTime);
    void render(sf::RenderWindow& window);
    
    bool isButtonPressed(TouchButton button) const;
    bool isButtonJustPressed(TouchButton button) const;
    sf::Vector2f getVirtualJoystickDelta() const;
    
    void setButtonCallback(TouchButton button, std::function<void()> callback);
    void setVisible(bool visible);
    void setOpacity(float opacity);
    void resize(const sf::Vector2u& newScreenSize);
    void resetInputState();
    
    // Customization methods
    void setButtonConfig(TouchButton button, const TouchButtonConfig& config);
    void enableVibration(bool enable);
    void setHapticFeedback(bool enable);

private:
    TouchControls();
    ~TouchControls() = default;
    TouchControls(const TouchControls&) = delete;
    TouchControls& operator=(const TouchControls&) = delete;
    
    void loadTextures();
    void createDefaultLayout(const sf::Vector2u& screenSize);
    sf::Vector2f calculateJoystickPosition(const sf::Vector2f& touchPos) const;
    TouchButton hitTest(const sf::Vector2i& touchPos) const;
    void triggerHapticFeedback() const;
    
    struct ButtonState {
        bool isPressed = false;
        bool wasPressed = false;
        TouchButtonConfig config;
        sf::Sprite sprite;
        std::function<void()> callback;
        
        ButtonState() {
            config.normalColor = sf::Color(100, 100, 100, 180);
            config.pressedColor = sf::Color(150, 150, 150, 220);
            config.visible = true;
            config.opacity = 0.8f;
        }
    };
    
    struct JoystickState {
        bool active = false;
        sf::Vector2f center;
        sf::Vector2f currentPos;
        sf::Vector2f delta;
        float radius = 60.0f;
        sf::CircleShape baseShape;
        sf::CircleShape thumbShape;
        
        JoystickState() {
            baseShape.setRadius(radius);
            baseShape.setFillColor(sf::Color(80, 80, 80, 180));
            thumbShape.setRadius(radius * 0.4f);
            thumbShape.setFillColor(sf::Color(120, 120, 120, 220));
        }
    };
    
    std::unordered_map<TouchButton, ButtonState> m_buttons;
    JoystickState m_joystick;
    sf::Vector2u m_screenSize;
    bool m_visible;
    float m_globalOpacity;
    bool m_vibrationEnabled;
    bool m_hapticEnabled;
    std::vector<TouchButton> m_activeTouches;
    float m_doubleTapTimer;
    TouchButton m_lastTappedButton;
    float m_buttonPressFeedbackTime;
    std::unordered_map<TouchButton, float> m_feedbackTimers;
};

} // namespace arenax3
} // namespace com

#endif // TOUCH_CONTROLS_HPP
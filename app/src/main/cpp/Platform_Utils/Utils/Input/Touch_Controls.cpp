// Touch_Controls.cpp
// Implementation of touch controls for ArenaX3

#include "Touch_Controls.h"
#include <cmath>
#include <algorithm>

namespace com {
namespace arenax3 {

// TouchPoint implementation
TouchPoint::TouchPoint()
    : id(0)
    , x(0.0f)
    , y(0.0f)
    , pressure(1.0f)
    , radius(0.0f)
    , isActive(false)
    , timestamp(0.0)
{
}

TouchPoint::TouchPoint(int touchId, float posX, float posY, float touchPressure, float touchRadius, double time)
    : id(touchId)
    , x(posX)
    , y(posY)
    , pressure(touchPressure)
    , radius(touchRadius)
    , isActive(true)
    , timestamp(time)
{
}

// TouchGesture implementation
TouchGesture::TouchGesture()
    : type(GestureType::NONE)
    , centerX(0.0f)
    , centerY(0.0f)
    , deltaX(0.0f)
    , deltaY(0.0f)
    , scale(1.0f)
    , rotation(0.0f)
    , velocity(0.0f)
    , numberOfTouches(0)
{
}

// TouchControls implementation
TouchControls::TouchControls()
    : m_width(0)
    , m_height(0)
    , m_maxTouchPoints(10)
    , m_tapTimeThreshold(0.2)
    , m_tapDistanceThreshold(10.0f)
    , m_swipeVelocityThreshold(100.0f)
    , m_pinchThreshold(0.1f)
    , m_longPressTimeThreshold(0.5)
    , m_lastTapTime(0.0)
    , m_tapCount(0)
{
}

TouchControls::~TouchControls()
{
    m_activeTouches.clear();
    m_gestureHistory.clear();
}

void TouchControls::initialize(int screenWidth, int screenHeight, int maxTouches)
{
    m_width = screenWidth;
    m_height = screenHeight;
    m_maxTouchPoints = maxTouches;
    m_activeTouches.clear();
    m_gestureHistory.clear();
    m_gestureHistory.reserve(50);
}

void TouchControls::update(double currentTime)
{
    // Update gesture history
    updateGestureHistory(currentTime);
    
    // Process continuous gestures
    processPinchGesture(currentTime);
    processPanGesture(currentTime);
    processRotateGesture(currentTime);
    
    // Detect long press
    detectLongPress(currentTime);
}

void TouchControls::handleTouchBegin(int touchId, float x, float y, float pressure, float radius, double timestamp)
{
    if (m_activeTouches.size() >= m_maxTouchPoints) {
        return;
    }
    
    TouchPoint newTouch(touchId, x, y, pressure, radius, timestamp);
    m_activeTouches[touchId] = newTouch;
    
    // Reset tap tracking for new touch sequence
    if (m_activeTouches.size() == 1) {
        m_lastTapTime = timestamp;
        m_tapCount = 0;
    }
    
    // Begin gesture tracking
    m_currentGesture.type = GestureType::NONE;
    m_currentGesture.centerX = x;
    m_currentGesture.centerY = y;
    m_currentGesture.numberOfTouches = static_cast<int>(m_activeTouches.size());
}

void TouchControls::handleTouchMove(int touchId, float x, float y, float pressure, float radius, double timestamp)
{
    auto it = m_activeTouches.find(touchId);
    if (it != m_activeTouches.end()) {
        TouchPoint& touch = it->second;
        touch.x = x;
        touch.y = y;
        touch.pressure = pressure;
        touch.radius = radius;
        touch.timestamp = timestamp;
    }
    
    updateGestureCenter();
}

void TouchControls::handleTouchEnd(int touchId, float x, float y, float pressure, float radius, double timestamp)
{
    auto it = m_activeTouches.find(touchId);
    if (it != m_activeTouches.end()) {
        TouchPoint endedTouch = it->second;
        m_activeTouches.erase(it);
        
        // Detect tap gesture
        if (m_activeTouches.empty()) {
            detectTapGesture(endedTouch, timestamp);
            detectSwipeGesture(endedTouch, timestamp);
        }
    }
    
    updateGestureCenter();
}

void TouchControls::handleTouchCancel(int touchId, double timestamp)
{
    auto it = m_activeTouches.find(touchId);
    if (it != m_activeTouches.end()) {
        m_activeTouches.erase(it);
    }
    
    if (m_activeTouches.empty()) {
        m_currentGesture.type = GestureType::NONE;
        m_currentGesture.deltaX = 0.0f;
        m_currentGesture.deltaY = 0.0f;
    }
    
    updateGestureCenter();
}

bool TouchControls::isTouchActive(int touchId) const
{
    return m_activeTouches.find(touchId) != m_activeTouches.end();
}

int TouchControls::getActiveTouchCount() const
{
    return static_cast<int>(m_activeTouches.size());
}

TouchPoint TouchControls::getTouchPoint(int touchId) const
{
    auto it = m_activeTouches.find(touchId);
    if (it != m_activeTouches.end()) {
        return it->second;
    }
    return TouchPoint();
}

std::vector<TouchPoint> TouchControls::getAllTouchPoints() const
{
    std::vector<TouchPoint> points;
    points.reserve(m_activeTouches.size());
    for (const auto& pair : m_activeTouches) {
        points.push_back(pair.second);
    }
    return points;
}

TouchGesture TouchControls::getCurrentGesture() const
{
    return m_currentGesture;
}

std::vector<TouchGesture> TouchControls::getGestureHistory() const
{
    return m_gestureHistory;
}

void TouchControls::clearGestureHistory()
{
    m_gestureHistory.clear();
}

void TouchControls::setTapThresholds(double timeThreshold, float distanceThreshold)
{
    m_tapTimeThreshold = timeThreshold;
    m_tapDistanceThreshold = distanceThreshold;
}

void TouchControls::setSwipeThreshold(float velocityThreshold)
{
    m_swipeVelocityThreshold = velocityThreshold;
}

void TouchControls::setLongPressThreshold(double timeThreshold)
{
    m_longPressTimeThreshold = timeThreshold;
}

void TouchControls::updateGestureCenter()
{
    if (m_activeTouches.empty()) {
        return;
    }
    
    float sumX = 0.0f;
    float sumY = 0.0f;
    
    for (const auto& pair : m_activeTouches) {
        sumX += pair.second.x;
        sumY += pair.second.y;
    }
    
    m_currentGesture.centerX = sumX / m_activeTouches.size();
    m_currentGesture.centerY = sumY / m_activeTouches.size();
    m_currentGesture.numberOfTouches = static_cast<int>(m_activeTouches.size());
}

void TouchControls::processPinchGesture(double currentTime)
{
    if (m_activeTouches.size() >= 2) {
        float currentDistance = calculateTouchDistance();
        
        if (m_currentGesture.type == GestureType::NONE) {
            m_currentGesture.type = GestureType::PINCH;
            m_previousPinchDistance = currentDistance;
            m_currentGesture.scale = 1.0f;
        } else if (m_currentGesture.type == GestureType::PINCH) {
            float delta = currentDistance - m_previousPinchDistance;
            if (std::abs(delta) > m_pinchThreshold) {
                m_currentGesture.scale = currentDistance / m_previousPinchDistance;
                m_currentGesture.deltaX = delta;
                m_previousPinchDistance = currentDistance;
            }
        }
    }
}

void TouchControls::processPanGesture(double currentTime)
{
    if (m_activeTouches.size() == 1 && m_currentGesture.type == GestureType::NONE) {
        static float lastX = 0.0f, lastY = 0.0f;
        static bool firstMove = true;
        
        auto it = m_activeTouches.begin();
        if (it != m_activeTouches.end()) {
            if (firstMove) {
                lastX = it->second.x;
                lastY = it->second.y;
                firstMove = false;
            } else {
                float deltaX = it->second.x - lastX;
                float deltaY = it->second.y - lastY;
                
                if (std::abs(deltaX) > 5.0f || std::abs(deltaY) > 5.0f) {
                    m_currentGesture.type = GestureType::PAN;
                    m_currentGesture.deltaX = deltaX;
                    m_currentGesture.deltaY = deltaY;
                    lastX = it->second.x;
                    lastY = it->second.y;
                }
            }
        }
    } else {
        if (m_currentGesture.type == GestureType::PAN) {
            m_currentGesture.type = GestureType::NONE;
        }
    }
}

void TouchControls::processRotateGesture(double currentTime)
{
    if (m_activeTouches.size() >= 2) {
        float currentAngle = calculateTouchAngle();
        
        if (m_currentGesture.type == GestureType::NONE) {
            m_previousRotationAngle = currentAngle;
            m_currentGesture.rotation = 0.0f;
        } else if (m_currentGesture.type == GestureType::ROTATE || 
                   m_currentGesture.type == GestureType::PINCH) {
            float delta = currentAngle - m_previousRotationAngle;
            if (std::abs(delta) > 0.01f) {
                m_currentGesture.type = GestureType::ROTATE;
                m_currentGesture.rotation = delta;
                m_previousRotationAngle = currentAngle;
            }
        }
    }
}

void TouchControls::detectTapGesture(const TouchPoint& touch, double currentTime)
{
    double timeSinceLastTap = currentTime - m_lastTapTime;
    float distanceFromStart = calculateDistance(touch.x, touch.y, touch.x, touch.y);
    
    if (distanceFromStart <= m_tapDistanceThreshold && 
        (currentTime - touch.timestamp) <= m_tapTimeThreshold) {
        
        if (timeSinceLastTap <= m_tapTimeThreshold) {
            m_tapCount++;
        } else {
            m_tapCount = 1;
        }
        
        m_lastTapTime = currentTime;
        
        TouchGesture tapGesture;
        tapGesture.type = GestureType::TAP;
        tapGesture.centerX = touch.x;
        tapGesture.centerY = touch.y;
        tapGesture.numberOfTouches = 1;
        
        if (m_tapCount == 1) {
            tapGesture.type = GestureType::TAP;
        } else if (m_tapCount == 2) {
            tapGesture.type = GestureType::DOUBLE_TAP;
            m_tapCount = 0;
        }
        
        m_gestureHistory.push_back(tapGesture);
        if (m_gestureHistory.size() > 50) {
            m_gestureHistory.erase(m_gestureHistory.begin());
        }
    }
}

void TouchControls::detectSwipeGesture(const TouchPoint& touch, double currentTime)
{
    float deltaX = touch.x - m_currentGesture.centerX;
    float deltaY = touch.y - m_currentGesture.centerY;
    float velocity = calculateDistance(deltaX, deltaY, 0.0f, 0.0f) / 
                    static_cast<float>(currentTime - touch.timestamp);
    
    if (velocity > m_swipeVelocityThreshold) {
        TouchGesture swipeGesture;
        swipeGesture.type = GestureType::SWIPE;
        swipeGesture.centerX = touch.x;
        swipeGesture.centerY = touch.y;
        swipeGesture.deltaX = deltaX;
        swipeGesture.deltaY = deltaY;
        swipeGesture.velocity = velocity;
        swipeGesture.numberOfTouches = 1;
        
        // Determine swipe direction
        if (std::abs(deltaX) > std::abs(deltaY)) {
            swipeGesture.type = (deltaX > 0) ? GestureType::SWIPE_RIGHT : GestureType::SWIPE_LEFT;
        } else {
            swipeGesture.type = (deltaY > 0) ? GestureType::SWIPE_DOWN : GestureType::SWIPE_UP;
        }
        
        m_gestureHistory.push_back(swipeGesture);
        if (m_gestureHistory.size() > 50) {
            m_gestureHistory.erase(m_gestureHistory.begin());
        }
    }
}

void TouchControls::detectLongPress(double currentTime)
{
    if (m_activeTouches.size() == 1 && m_currentGesture.type == GestureType::NONE) {
        auto it = m_activeTouches.begin();
        if (it != m_activeTouches.end()) {
            double pressDuration = currentTime - it->second.timestamp;
            if (pressDuration >= m_longPressTimeThreshold) {
                TouchGesture longPressGesture;
                longPressGesture.type = GestureType::LONG_PRESS;
                longPressGesture.centerX = it->second.x;
                longPressGesture.centerY = it->second.y;
                longPressGesture.numberOfTouches = 1;
                
                m_gestureHistory.push_back(longPressGesture);
                if (m_gestureHistory.size() > 50) {
                    m_gestureHistory.erase(m_gestureHistory.begin());
                }
                
                m_currentGesture.type = GestureType::LONG_PRESS;
            }
        }
    }
}

void TouchControls::updateGestureHistory(double currentTime)
{
    if (m_currentGesture.type != GestureType::NONE) {
        m_gestureHistory.push_back(m_currentGesture);
        if (m_gestureHistory.size() > 50) {
            m_gestureHistory.erase(m_gestureHistory.begin());
        }
    }
}

float TouchControls::calculateTouchDistance() const
{
    if (m_activeTouches.size() < 2) {
        return 0.0f;
    }
    
    auto it = m_activeTouches.begin();
    const TouchPoint& touch1 = it->second;
    ++it;
    const TouchPoint& touch2 = it->second;
    
    return calculateDistance(touch1.x, touch1.y, touch2.x, touch2.y);
}

float TouchControls::calculateTouchAngle() const
{
    if (m_activeTouches.size() < 2) {
        return 0.0f;
    }
    
    auto it = m_activeTouches.begin();
    const TouchPoint& touch1 = it->second;
    ++it;
    const TouchPoint& touch2 = it->second;
    
    return std::atan2(touch2.y - touch1.y, touch2.x - touch1.x);
}

float TouchControls::calculateDistance(float x1, float y1, float x2, float y2) const
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace arenax3
} // namespace com
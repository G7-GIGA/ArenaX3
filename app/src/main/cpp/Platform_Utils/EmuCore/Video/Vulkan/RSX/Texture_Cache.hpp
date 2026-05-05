#ifndef TEXTURE_CACHE_HPP
#define TEXTURE_CACHE_HPP

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <optional>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace com {
namespace arenax3 {

struct TextureData {
    GLuint id;
    int width;
    int height;
    int channels;
    std::string path;
    
    TextureData() : id(0), width(0), height(0), channels(0) {}
    
    TextureData(GLuint texId, int w, int h, int c, const std::string& p)
        : id(texId), width(w), height(h), channels(c), path(p) {}
    
    ~TextureData() {
        if (id != 0) {
            glDeleteTextures(1, &id);
        }
    }
    
    // Non-copyable
    TextureData(const TextureData&) = delete;
    TextureData& operator=(const TextureData&) = delete;
    
    // Movable
    TextureData(TextureData&& other) noexcept
        : id(other.id), width(other.width), height(other.height),
          channels(other.channels), path(std::move(other.path)) {
        other.id = 0;
    }
    
    TextureData& operator=(TextureData&& other) noexcept {
        if (this != &other) {
            if (id != 0) glDeleteTextures(1, &id);
            id = other.id;
            width = other.width;
            height = other.height;
            channels = other.channels;
            path = std::move(other.path);
            other.id = 0;
        }
        return *this;
    }
};

class TextureCache {
public:
    static TextureCache& getInstance() {
        static TextureCache instance;
        return instance;
    }
    
    // Load texture from file
    std::shared_ptr<TextureData> loadTexture(const std::string& filePath, bool flipVertical = true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cache.find(filePath);
        if (it != m_cache.end()) {
            return it->second;
        }
        
        // Load image using stb_image
        stbi_set_flip_vertically_on_load(flipVertical);
        
        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
        
        if (!data) {
            // Return empty shared_ptr on failure
            return nullptr;
        }
        
        GLenum format;
        if (channels == 1) format = GL_RED;
        else if (channels == 2) format = GL_RG;
        else if (channels == 3) format = GL_RGB;
        else format = GL_RGBA;
        
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(data);
        
        auto textureData = std::make_shared<TextureData>(textureId, width, height, channels, filePath);
        m_cache[filePath] = textureData;
        
        return textureData;
    }
    
    // Load texture from memory
    std::shared_ptr<TextureData> loadTextureFromMemory(const unsigned char* data, size_t size, 
                                                       const std::string& key, bool flipVertical = true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }
        
        stbi_set_flip_vertically_on_load(flipVertical);
        
        int width, height, channels;
        unsigned char* imgData = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 0);
        
        if (!imgData) {
            return nullptr;
        }
        
        GLenum format;
        if (channels == 1) format = GL_RED;
        else if (channels == 2) format = GL_RG;
        else if (channels == 3) format = GL_RGB;
        else format = GL_RGBA;
        
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, imgData);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(imgData);
        
        auto textureData = std::make_shared<TextureData>(textureId, width, height, channels, key);
        m_cache[key] = textureData;
        
        return textureData;
    }
    
    // Create blank texture
    std::shared_ptr<TextureData> createBlankTexture(int width, int height, const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }
        
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        auto textureData = std::make_shared<TextureData>(textureId, width, height, 4, key);
        m_cache[key] = textureData;
        
        return textureData;
    }
    
    // Get texture from cache
    std::shared_ptr<TextureData> getTexture(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Check if texture exists
    bool hasTexture(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.find(key) != m_cache.end();
    }
    
    // Remove texture from cache
    bool removeTexture(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.erase(key) > 0;
    }
    
    // Clear all textures
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
    }
    
    // Get cache size
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }
    
private:
    TextureCache() = default;
    ~TextureCache() = default;
    
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;
    
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<TextureData>> m_cache;
};

} // namespace arenax3
} // namespace com

#endif // TEXTURE_CACHE_HPP
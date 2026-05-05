#include "Texture_Cache.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <cassert>

namespace com {
namespace arenax3 {

struct TextureCache::Impl {
    std::unordered_map<std::string, std::shared_ptr<Texture>> cache;
    std::mutex mutex;
    size_t maxSize = 256;
};

TextureCache::TextureCache() : pImpl(std::make_unique<Impl>()) {}

TextureCache::~TextureCache() = default;

TextureCache::TextureCache(TextureCache&&) noexcept = default;
TextureCache& TextureCache::operator=(TextureCache&&) noexcept = default;

std::shared_ptr<Texture> TextureCache::get(const std::string& path) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    auto it = pImpl->cache.find(path);
    if (it != pImpl->cache.end()) {
        return it->second;
    }
    return nullptr;
}

void TextureCache::insert(const std::string& path, std::shared_ptr<Texture> texture) {
    if (!texture) return;
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    
    if (pImpl->cache.size() >= pImpl->maxSize) {
        pImpl->cache.clear();
    }
    
    pImpl->cache[path] = std::move(texture);
}

void TextureCache::remove(const std::string& path) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->cache.erase(path);
}

void TextureCache::clear() {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->cache.clear();
}

bool TextureCache::contains(const std::string& path) const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    return pImpl->cache.find(path) != pImpl->cache.end();
}

size_t TextureCache::size() const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    return pImpl->cache.size();
}

void TextureCache::setMaxSize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->maxSize = maxSize;
    while (pImpl->cache.size() > pImpl->maxSize) {
        pImpl->cache.clear();
        break;
    }
}

size_t TextureCache::getMaxSize() const {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    return pImpl->maxSize;
}

} // namespace arenax3
} // namespace com
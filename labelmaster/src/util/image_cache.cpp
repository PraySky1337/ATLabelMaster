/**
 * @file image_cache.cpp
 * @brief Implementation of image cache
 */

#include "image_cache.hpp"
#include <QMutexLocker>
#include <QDebug>

namespace labelmaster::util {

ImageCache& ImageCache::instance() {
    static ImageCache instance;
    return instance;
}

ImageCache::ImageCache() {
    // Default: 100MB cache, max 100 images
    setMaxSizeMB(100);
    setMaxCount(100);
}

void ImageCache::setMaxSizeMB(int mb) {
    qint64 bytes = static_cast<qint64>(mb) * 1024 * 1024;
    QMutexLocker locker(&mutex_);
    cache_.setMaxCost(bytes);
}

int ImageCache::maxSizeMB() const {
    QMutexLocker locker(&mutex_);
    return static_cast<int>(cache_.maxCost() / (1024 * 1024));
}

void ImageCache::setMaxCount(int count) {
    QMutexLocker locker(&mutex_);
    // QCache uses maxCost and count based on inserted items
    // We track max count separately
    max_count_ = count;
}

int ImageCache::maxCount() const {
    QMutexLocker locker(&mutex_);
    return max_count_;
}

qint64 ImageCache::imageSize(const QImage& image) const {
    // Approximate memory size of image
    return image.sizeInBytes();
}

void ImageCache::insert(const QString& key, const QImage& image) {
    if (key.isEmpty() || image.isNull()) {
        return;
    }

    qint64 size = imageSize(image);

    QMutexLocker locker(&mutex_);

    // Check if this would replace existing item
    bool wasExisting = cache_.contains(key);

    // Insert the image (cache takes ownership)
    QImage* copy = new QImage(image);
    if (!cache_.insert(key, copy, size)) {
        // Insert failed (item too large or cache full)
        delete copy;
        stats_.evictions++;
        emit itemEvicted(key);
    } else {
        if (wasExisting) {
            stats_.totalSize -= size;
        }
        stats_.totalSize += size;
        stats_.currentCount = cache_.size();
    }
}

QImage ImageCache::get(const QString& key) {
    QMutexLocker locker(&mutex_);

    QImage* cached = cache_.object(key);
    if (cached) {
        stats_.hits++;
        return *cached; // Return a copy
    }

    stats_.misses++;
    return QImage(); // Return null image
}

bool ImageCache::contains(const QString& key) const {
    QMutexLocker locker(&mutex_);
    return cache_.contains(key);
}

void ImageCache::remove(const QString& key) {
    QMutexLocker locker(&mutex_);

    QImage* removed = cache_.take(key);
    if (removed) {
        stats_.totalSize -= imageSize(*removed);
        stats_.currentCount = cache_.size();
        delete removed;
    }
}

void ImageCache::clear() {
    QMutexLocker locker(&mutex_);
    cache_.clear();
    stats_.totalSize = 0;
    stats_.currentCount = 0;
    emit cacheCleared();
}

CacheStats ImageCache::stats() const {
    QMutexLocker locker(&mutex_);
    return stats_;
}

void ImageCache::resetStats() {
    QMutexLocker locker(&mutex_);
    stats_ = CacheStats();
}

//=============================================================================
// ImageCacheGuard
//=============================================================================

ImageCacheGuard::ImageCacheGuard(const QString& key, const QImage& image)
    : key_(key) {
    ImageCache::instance().insert(key_, image);
}

ImageCacheGuard::~ImageCacheGuard() {
    // Note: We don't remove from cache on destruction
    // The cache manages its own lifecycle via LRU
}

QImage ImageCacheGuard::image() const {
    return ImageCache::instance().get(key_);
}

} // namespace labelmaster::util

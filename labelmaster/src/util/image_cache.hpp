/**
 * @file image_cache.hpp
 * @brief LRU image cache for ATLabelMaster
 *
 * Provides efficient caching of loaded images to reduce
 * memory usage and improve performance.
 */

#ifndef LABELMASTER_IMAGE_CACHE_HPP
#define LABELMASTER_IMAGE_CACHE_HPP

#include <QObject>
#include <QString>
#include <QImage>
#include <QMutex>
#include <QCache>
#include <QPair>

namespace labelmaster::util {

/**
 * @brief Image cache statistics
 */
struct CacheStats {
    int hits = 0;
    int misses = 0;
    int evictions = 0;
    qint64 totalSize = 0;
    int currentCount = 0;
};

/**
 * @brief LRU image cache
 *
 * Thread-safe cache for images with configurable size limits.
 * Uses LRU (Least Recently Used) eviction policy.
 */
class ImageCache : public QObject {
    Q_OBJECT

public:
    static ImageCache& instance();

    /**
     * @brief Set maximum cache size in MB
     */
    void setMaxSizeMB(int mb);

    /**
     * @brief Get maximum cache size in MB
     */
    int maxSizeMB() const;

    /**
     * @brief Set maximum number of cached items
     */
    void setMaxCount(int count);

    /**
     * @brief Get maximum number of cached items
     */
    int maxCount() const;

    /**
     * @brief Insert an image into the cache
     * @param key Unique identifier for the image
     * @param image The image to cache
     */
    void insert(const QString& key, const QImage& image);

    /**
     * @brief Retrieve an image from the cache
     * @param key Unique identifier for the image
     * @return Cached image, or null image if not found
     */
    QImage get(const QString& key);

    /**
     * @brief Check if an image is in the cache
     */
    bool contains(const QString& key) const;

    /**
     * @brief Remove an image from the cache
     */
    void remove(const QString& key);

    /**
     * @brief Clear all cached images
     */
    void clear();

    /**
     * @brief Get cache statistics
     */
    CacheStats stats() const;

    /**
     * @brief Reset statistics
     */
    void resetStats();

signals:
    /**
     * @brief Emitted when an item is evicted from cache
     */
    void itemEvicted(const QString& key);

    /**
     * @brief Emitted when cache is cleared
     */
    void cacheCleared();

private:
    ImageCache();
    ~ImageCache() override = default;

    // Delete copy/move
    ImageCache(const ImageCache&) = delete;
    ImageCache& operator=(const ImageCache&) = delete;

    qint64 imageSize(const QImage& image) const;

private:
    QCache<QString, QImage> cache_;
    mutable QMutex mutex_;
    int max_count_ = 100;

    // Statistics
    CacheStats stats_;
};

/**
 * @brief RAII helper for cache operations
 */
class ImageCacheGuard {
public:
    ImageCacheGuard(const QString& key, const QImage& image);
    ~ImageCacheGuard();

    QImage image() const;

private:
    QString key_;
};

} // namespace labelmaster::util

#endif // LABELMASTER_IMAGE_CACHE_HPP

/**
 * @file detector_interface.hpp
 * @brief Abstract interface for armor detectors
 *
 * Provides a common interface for different detector implementations
 * (traditional computer vision, AI/ML-based, etc.).
 */

#ifndef LABELMASTER_DETECTOR_INTERFACE_HPP
#define LABELMASTER_DETECTOR_INTERFACE_HPP

#include <QImage>
#include <QVector>
#include <QString>
#include <opencv2/core.hpp>
#include "types.hpp"

namespace labelmaster::detector {

/**
 * @brief Abstract interface for armor detectors
 *
 * This interface defines the contract for all detector implementations.
 * It supports both QImage and cv::Mat inputs for flexibility.
 */
class IDetector {
public:
    virtual ~IDetector() = default;

    /**
     * @brief Set up the detector model
     * @param modelPath Path to the model file (for AI detectors) or config (for traditional)
     * @return true if setup succeeded, false otherwise
     */
    virtual bool setupModel(const QString& modelPath) = 0;

    /**
     * @brief Detect armors in a QImage
     * @param image Input image
     * @return Vector of detected armors
     */
    virtual QVector<Armor> detect(const QImage& image) = 0;

    /**
     * @brief Detect armors in a cv::Mat
     * @param mat OpenCV matrix (BGR/RGB format)
     * @return Vector of detected armors
     */
    virtual QVector<Armor> detectMat(const cv::Mat& mat) = 0;

    /**
     * @brief Get the detector name
     * @return Human-readable detector name
     */
    virtual QString name() const = 0;

    /**
     * @brief Check if detector is ready
     * @return true if detector is initialized and ready
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Reset/reinitialize the detector
     * @return true if reset succeeded, false otherwise
     */
    virtual bool reset() = 0;
};

} // namespace labelmaster::detector

#endif // LABELMASTER_DETECTOR_INTERFACE_HPP

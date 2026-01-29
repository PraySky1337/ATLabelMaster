/**
 * @file label_io.hpp
 * @brief Label file I/O operations
 *
 * Extracted from FileService to separate label read/write concerns.
 */

#ifndef LABELMASTER_LABEL_IO_HPP
#define LABELMASTER_LABEL_IO_HPP

#include <QImage>
#include <QVector>
#include "dataset_format_handler.hpp"

namespace labelmaster::service {

/**
 * @brief Handles label file input/output
 *
 * Provides a clean interface for reading and writing label files
 * using different format handlers.
 */
class LabelIO {
public:
    LabelIO();

    /**
     * @brief Set the format handler
     */
    void setFormatHandler(DatasetFormatHandler* handler);

    /**
     * @brief Read label file
     * @param filePath Path to label file
     * @param imgSize Image size for denormalization
     * @return Vector of armors
     */
    QVector<Armor> readLabelFile(const QString& filePath, const QSize& imgSize);

    /**
     * @brief Write label file
     * @param filePath Path to label file
     * @param armors Armors to write
     * @param imgSize Image size for normalization
     * @return true if successful
     */
    bool writeLabelFile(
        const QString& filePath,
        const QVector<Armor>& armors,
        const QSize& imgSize);

    /**
     * @brief Get label file path for image
     * @param imagePath Path to image file
     * @return Path to corresponding label file
     */
    static QString labelPathForImage(const QString& imagePath);

private:
    DatasetFormatHandler* formatHandler_ = nullptr;
};

} // namespace labelmaster::service

#endif // LABELMASTER_LABEL_IO_HPP

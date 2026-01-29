/**
 * @file dataset_format_handler.hpp
 * @brief Strategy pattern for dataset format handlers
 *
 * Abstracts different dataset label formats for extensibility.
 */

#ifndef LABELMASTER_DATASET_FORMAT_HANDLER_HPP
#define LABELMASTER_DATASET_FORMAT_HANDLER_HPP

#include <QTextStream>
#include <QVector>
#include "types.hpp"

namespace labelmaster::service {

/**
 * @brief Abstract base class for dataset format handlers
 *
 * Uses Strategy pattern to support multiple dataset formats.
 */
class DatasetFormatHandler {
public:
    virtual ~DatasetFormatHandler() = default;

    /**
     * @brief Read label file
     * @param stream Input text stream
     * @param armors Output vector of armors
     * @param imgSize Image size for normalization
     * @return true if successful
     */
    virtual bool read(
        QTextStream& stream,
        QVector<Armor>& armors,
        const QSize& imgSize) = 0;

    /**
     * @brief Write label file
     * @param stream Output text stream
     * @param armors Input vector of armors
     * @param imgSize Image size for normalization
     * @return true if successful
     */
    virtual bool write(
        QTextStream& stream,
        const QVector<Armor>& armors,
        const QSize& imgSize) = 0;

    /**
     * @brief Get format name
     */
    virtual QString name() const = 0;

    /**
     * @brief Get format file extension
     */
    virtual QString extension() const = 0;
};

/**
 * @brief LabelMaster2 format handler
 *
 * Format: color size cls x0 y0 x1 y1 x2 y2 x3 y3
 * Normalized coordinates [0,1]
 */
class LabelMaster2Format : public DatasetFormatHandler {
public:
    bool read(QTextStream& stream, QVector<Armor>& armors, const QSize& imgSize) override;
    bool write(QTextStream& stream, const QVector<Armor>& armors, const QSize& imgSize) override;
    QString name() const override { return "LabelMaster2"; }
    QString extension() const override { return "txt"; }
};

/**
 * @brief HITSZ format handler
 *
 * South Engineering Eagle format with different coordinate system
 */
class HITSZFormat : public DatasetFormatHandler {
public:
    bool read(QTextStream& stream, QVector<Armor>& armors, const QSize& imgSize) override;
    bool write(QTextStream& stream, const QVector<Armor>& armors, const QSize& imgSize) override;
    QString name() const override { return "HITSZ"; }
    QString extension() const override { return "txt"; }
};

/**
 * @brief Extended format with bounding box
 *
 * Format: color size cls x y w h x0 y0 x1 y1 x2 y2 x3 y3
 * Includes both bbox and keypoints
 */
class ExtendedFormat : public DatasetFormatHandler {
public:
    bool read(QTextStream& stream, QVector<Armor>& armors, const QSize& imgSize) override;
    bool write(QTextStream& stream, const QVector<Armor>& armors, const QSize& imgSize) override;
    QString name() const override { return "Extended"; }
    QString extension() const override { return "txt"; }
};

/**
 * @brief LabelMasterV4 format with class token first
 *
 * Format: cls x_c y_c w h x0 y0 x1 y1 x2 y2 x3 y3
 * cls: color*size*num (e.g., "R1", "B2", "G3")
 * x_c, y_c, w, h: normalized bounding box (center format) [0,1]
 * x0,y0,x1,y1,x2,y2,x3,y3: normalized four corner points [0,1]
 */
class LabelMasterV4Format : public DatasetFormatHandler {
public:
    bool read(QTextStream& stream, QVector<Armor>& armors, const QSize& imgSize) override;
    bool write(QTextStream& stream, const QVector<Armor>& armors, const QSize& imgSize) override;
    QString name() const override { return "LabelMasterV4"; }
    QString extension() const override { return "txt"; }
};

} // namespace labelmaster::service

#endif // LABELMASTER_DATASET_FORMAT_HANDLER_HPP

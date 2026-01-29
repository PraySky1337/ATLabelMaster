/**
 * @file annotation_manager.hpp
 * @brief Manages annotation data
 *
 * Extracted from ImageCanvas to separate data management concerns.
 */

#ifndef LABELMASTER_ANNOTATION_MANAGER_HPP
#define LABELMASTER_ANNOTATION_MANAGER_HPP

#include <QVector>
#include "types.hpp"

namespace labelmaster::ui {

/**
 * @brief Manages armor annotation data
 *
 * Handles storage, retrieval, and manipulation of armor annotations.
 */
class AnnotationManager {
public:
    AnnotationManager() = default;

    // Data access
    const QVector<Armor>& detections() const { return dets_; }
    void setDetections(const QVector<Armor>& dets);
    void clearDetections();

    // Selection management
    int selectedIndex() const { return selectedIndex_; }
    bool setSelectedIndex(int index);
    const Armor* selectedArmor() const;

    // Hover management
    int hoverIndex() const { return hoverIndex_; }
    void setHoverIndex(int index);

    // CRUD operations
    void createNewDetection();
    void addDetection(const Armor& armor);
    void updateDetection(int index, const Armor& armor);
    void removeDetection(int index);

    // Current class for new detections
    QString currentClass() const { return currentClass_; }
    void setCurrentClass(const QString& cls) { currentClass_ = cls; }

    // Update selected detection properties
    bool setSelectedClass(const QString& cls);
    bool setSelectedInfo(const QString& cls, const QString& color, int size);

private:
    QVector<Armor> dets_;
    int selectedIndex_ = -1;
    int hoverIndex_ = -1;
    QString currentClass_;
};

} // namespace labelmaster::ui

#endif // LABELMASTER_ANNOTATION_MANAGER_HPP

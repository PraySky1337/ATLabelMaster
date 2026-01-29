/**
 * @file annotation_manager.cpp
 * @brief Implementation of AnnotationManager
 */

#include "annotation_manager.hpp"

namespace labelmaster::ui {

void AnnotationManager::setDetections(const QVector<Armor>& dets) {
    dets_ = dets;
    // Reset selection if out of bounds
    if (selectedIndex_ >= dets_.size()) {
        selectedIndex_ = dets_.isEmpty() ? -1 : 0;
    }
}

void AnnotationManager::clearDetections() {
    dets_.clear();
    selectedIndex_ = -1;
    hoverIndex_ = -1;
}

bool AnnotationManager::setSelectedIndex(int index) {
    if (index < -1 || index >= dets_.size())
        return false;
    selectedIndex_ = index;
    return true;
}

const Armor* AnnotationManager::selectedArmor() const {
    if (selectedIndex_ < 0 || selectedIndex_ >= dets_.size())
        return nullptr;
    return &dets_[selectedIndex_];
}

void AnnotationManager::setHoverIndex(int index) {
    if (index >= -1 && index < dets_.size()) {
        hoverIndex_ = index;
    }
}

void AnnotationManager::createNewDetection() {
    Armor newArmor;
    newArmor.cls = currentClass_;
    dets_.append(newArmor);
    selectedIndex_ = dets_.size() - 1;
}

void AnnotationManager::addDetection(const Armor& armor) {
    dets_.append(armor);
}

void AnnotationManager::updateDetection(int index, const Armor& armor) {
    if (index >= 0 && index < dets_.size()) {
        dets_[index] = armor;
    }
}

void AnnotationManager::removeDetection(int index) {
    if (index >= 0 && index < dets_.size()) {
        dets_.remove(index);
        // Update selection
        if (selectedIndex_ == index) {
            selectedIndex_ = -1;
        } else if (selectedIndex_ > index) {
            selectedIndex_--;
        }
    }
}

bool AnnotationManager::setSelectedClass(const QString& cls) {
    if (selectedIndex_ < 0 || selectedIndex_ >= dets_.size())
        return false;
    dets_[selectedIndex_].cls = cls;
    return true;
}

bool AnnotationManager::setSelectedInfo(const QString& cls, const QString& color, int size) {
    if (selectedIndex_ < 0 || selectedIndex_ >= dets_.size())
        return false;
    dets_[selectedIndex_].cls = cls;
    dets_[selectedIndex_].color = color;
    dets_[selectedIndex_].size = size;
    return true;
}

} // namespace labelmaster::ui

#pragma once
#include <QStringList>

namespace classes {
inline const QStringList& all() {
    static QStringList k = {"armor", "rune", "robot", "buff", "base"};
    return k;
}
inline int idOf(const QString& name) {
    const auto& k = all();
    int idx = static_cast<int>(k.indexOf(name));
    return idx < 0 ? 0 : idx;
}
}

#include "qstring.h"
namespace IdConvert {
// ---------- 工具：token 规范化 ----------
inline QString colorLetter2Token(const QString& letter) {
    const QString L = letter.trimmed().left(1).toUpper();
    if (L == "B")
        return "BLUE";
    if (L == "R")
        return "RED";
    if (L == "G")
        return "GRAY";
    if (L == "P")
        return "PURPLE";
    const QString U = letter.trimmed().toUpper();
    if (U == "BLUE" || U == "RED" || U == "GRAY" || U == "PURPLE")
        return U;
    return "GRAY";
}
inline QString colorToken2Letter(const QString& tk) {
    const QString U = tk.trimmed().toUpper();
    if (U == "BLUE")
        return "B";
    if (U == "RED")
        return "R";
    if (U == "GRAY")
        return "G";
    if (U == "PURPLE")
        return "P";
    if (U == "B" || U == "R" || U == "G" || U == "P")
        return U;
    return "G";
}
inline QString colorId2Letter(int id) {
    switch (id) {
    case 0: return "B"; // BLUE
    case 1: return "R"; // RED
    case 2: return "G"; // GRAY
    case 3: return "P"; // PURPLE
    default: return "G";
    }
}
inline int colorToken2Id(const QString& token) {
    const QString l = token.trimmed().toUpper();
    if (l == "BLUE")
        return 0;
    if (l == "RED")
        return 1;
    if (l == "PURPLE")
        return 3;
    return 2;
}
// 颜色字母(B/R/G/P) → id(0/1/2/3)
inline int colorLetter2Id(const QString& letter) {
    const QChar c = letter.trimmed().isEmpty() ? QChar() : letter.trimmed().at(0).toUpper();
    if (c == 'B')
        return 0; // BLUE
    if (c == 'R')
        return 1; // RED
    if (c == 'G')
        return 2; // GRAY
    if (c == 'P') // PURPLE
        return 3;
    return 2;     // 默认 GRAY
}
inline int classToken2Id(const QString& NormalizedToken) {
    int len = NormalizedToken.length();
    char ch = NormalizedToken.at(0).toLatin1();
    // G 和 B 通过 size 区分大小
    if (len == 1) {
        switch (ch) {
        case 'G': return 0;  // G 哨兵（通过 size 区分大小）
        case '1':
        case '2':
        case '3':
        case '4':
        case '5': return (ch - '0');
        case 'O': return 6;
        case 'B': return 7;  // B 基地（通过 size 区分大小）
        }
    }
    return 0; // 默认返回 0 (G)
}
inline QString idCollect2Token(const int& classId) {
    switch (classId) {
    case 0: return "G";   // 哨兵（通过 size 区分大小）
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: return QString(QChar(classId + '0'));
    case 6: return "O";
    case 7: return "B";   // 基地（通过 size 区分大小）
    }
    return QString(QChar(classId + '0'));
}
inline QString normalizeClasslToken(const QString& cls) { // 归一化cls
    const QString u = cls.trimmed().toUpper();
    if (u == "G") {
        return "G";   // G 哨兵（通过 size 区分大小）
    }
    if (u == "O") {
        return "O";
    }
    if (u == "B" || u == "BS" || u == "BB") {
        return "B";   // B, Bs, Bb → B (基地，通过 size 区分大小)
    }
    if (u == "1" || u == "2" || u == "3" || u == "4" || u == "5") {
        return u;
    }
    return u;
}

} // namespace IdConvert

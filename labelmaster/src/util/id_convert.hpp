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
    // switch (ch) {
    // case 'G':
    //     if (len == 2) {
    //         res[0] = 0;
    //         switch (NormalizedToken.at(1).toLatin1()) {
    //         case 's': res[1] = 0; break;
    //         case 'b': res[1] = 1; break;
    //         }
    //     }
    //     break;
    // case '1':
    // case '2':
    //     if (len == 1) {
    //         res[0] = ch - '0';
    //     }
    //     break;
    // case '3':
    // case '4':
    // case '5':
    //     if (len == 2 && NormalizedToken.at(0).toLatin1() == 'B') {
    //         res[0] = ch - '0';
    //         res[1] = 1;
    //     } else if (len == 1) {
    //         res[0] = ch - '0';
    //         res[1] = 0;
    //     }
    //     break;
    // case 'O':
    //     if (len == 1) {
    //         res[0] = 6;
    //     }
    //     break;
    // case 'B':
    //     if (len == 2) {
    //         res[0] = 7;
    //         switch (NormalizedToken.at(1).toLatin1()) {
    //         case 's': res[1] = 0; break;
    //         case 'b': res[1] = 1; break;
    //         }
    //     }
    //     break;
    // }
    // V3
    if (NormalizedToken.length() == 1) {
        switch (ch) {
        case 'G': return 0;
        case '1':
        case '2':
        case '3':
        case '4': return (ch - '0');
        case 'O': return 5;
        case 'B': return 6;
        }
    }
    // V2
    //  if (NormalizedToken.length() == 1) {
    //      switch (ch) {
    //      case 'G': return 0;
    //      case '1':
    //      case '2':
    //      case '3':
    //      case '4':
    //      case '5': return (ch - '0');
    //      case 'O': return 6;
    //      case 'B': return 7;
    //      }
    //  }
    //  return 0; // 默认哨兵
}
inline QString idCollect2Token(const int& classId) {
    // V3
    switch (classId) {
    case 0: return "G";
    case 1:
    case 2:
    case 3:
    case 4: return QString(QChar(classId + '0'));
    case 5: return "O";
    case 6: return "B";
    }
    // V2
    //  switch (classId) {
    //  case 0: return "G";
    //  case 1:
    //  case 2:
    //  case 3:
    //  case 4:
    //  case 5: return QString(QChar(classId + '0'));
    //  case 6: return "O";
    //  case 7: return "B";
    //  }
    return QString(QChar(classId + '0'));
    // switch (classId) {
    // case 0:
    //     switch (sizeId) {
    //     case 0: return "Gs";
    //     default: return "Gb";
    //     }
    // case 1:
    // case 2: return QString(QChar(classId + '0'));
    // case 6: {
    //     return "O";
    // };
    // case 7: {
    //     switch (sizeId) {
    //     case 0: return "Bs";
    //     default: return "Bb";
    //     }
    // };
    // default:
    //     switch (sizeId) {
    //     case 0: return QString(QChar(classId + '0'));
    //     default: return QString(QChar(classId + '0')) + "B";
    //     };
    // }
}
inline QString normalizeClasslToken(const QString& cls) { // 归一化cls
    const QString u = cls.trimmed().toUpper();
    if (u == "G") {
        return "G";
    }
    if (u == "O") {
        return "O";
    }
    if (u == "B") {
        return "B";
    }
    // if (u == "O")
    //     return "O";
    // if (u == "BS")
    //     return "Bs";
    // if (u == "BB")
    //     return "Bb";
    // if (u == "GS")
    //     return "Gs";
    // if (u == "GB")
    //     return "Gb";
    // if (u == "O")
    //     return "O";
    // if (u == "BS")
    //     return "Bs";
    // if (u == "BB")
    //     return "Bb";
    if (u == "1" || u == "2" || u == "3" || u == "4" || u == "5") {
        return u;
    }
    // if (u.at(1) == 'B') {
    //     bool ok = true;
    //     switch (u.at(0).toLatin1() - 48) {
    //     case 3: return "3B"; break;
    //     case 4: return "4B"; break;
    //     case 5: return "5B"; break;
    //     default: break;
    //     }
    // }
    // // if (s == "Bs" || s == "Bb")
    // //     return s;
    return u;
}

} // namespace IdConvert
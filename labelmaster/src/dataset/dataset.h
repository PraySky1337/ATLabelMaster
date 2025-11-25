
enum class DataSet : unsigned char { // 数据集格式
    LabelMaster = 0,
    LabelMaster2,                    // 默认格式
    HITSZ,                           // 南工骁鹰
    UPC,                              // RPS
    NWPU,                              //西北工业大学
    //color * 16 + size * 8 + class
};
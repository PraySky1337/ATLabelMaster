
enum class DataSet : unsigned char { // 数据集格式
    LabelMaster = 0,
    LabelMaster2,                    // 默认格式 (11字段: color size cls pts)
    HITSZ,                           // 南工骁鹰
    //     贴纸 	ID
    // G（哨兵） 	0
    // 1（一号） 	1
    // 2（二号） 	2
    // 3（三号） 	3
    // 4（四号） 	4
    // 5（五号） 	5
    // O（前哨站） 	6
    // Bs（基地） 	7
    // Bb（基地大装甲） 	8
    // L3（三号平衡） 	9
    // L4（四号平衡） 	10
    // L5（五号平衡） 	11
    UPC,  // RPS
    NWPU, // 西北工业大学
    // color * 16 + size * 8 + class
    LabelMaster3,                    // 新增: 15字段 (color size cls xywh pts)
};
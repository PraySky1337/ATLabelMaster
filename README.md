# AT LabelMaster

Actor Thinker 数据集标注工具

## 待完善
(简单是简单，就是好折磨)

## 数据集格式
color label x1 y1 x2 y2 x3 y3 x4 y4

### Color
| int | color |
| :---: | :---: |
| 0 | BLUE |
| 1 | RED |
| 2 | GRAY |
| 3 | PURPLE |

### Label V1.0
| color | label |
| :---: | :---: |
| 0 |  | G |
| 1 |  | 1 |
| 2 |  | 2 |
| 3 |  | 3 |
| 4 |  | 4 |
| 5  | O(前哨站) |
| 6 | Bs(基地小装甲) | 
| 7 | Bb(基地大装甲) |

### Label V2.0
| color | size | label |
| :---: | :---: | :---: |
| 0 | 0 / 1 | Gs / Gb |
| 1 | 0     | 1 |
| 2 | 0     | 2 |
| 3 | 0 / 1 | 3 / 3B |
| 4 | 0 / 1 | 4 / 4B |
| 5 | 0 / 1 | 5 / 5B |
| 6 | 0     | O(前哨站) |
| 7 | 0 / 1 |Bs(基地小装甲) / Bb(基地大装甲) | 


### Point
从左上角开始逆时针排列

## 鸣谢 
华南师范大学 chenjunn [rm_auto_aim](https://github.com/chenjunnn/rm_auto_aim.git)

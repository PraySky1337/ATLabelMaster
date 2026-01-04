# AT LabelMaster

Actor Thinker 数据集标注工具

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![C++](https://img.shields.io/badge/C++-20-blue.svg)
![Qt](https://img.shields.io/badge/Qt-5/6-41cd52.svg)
![OpenVINO](https://img.shields.io/badge/OpenVINO-Toolkit-00285e.svg)
![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C.svg)

## 快捷键
- ctrl : Mask 遮蔽某片区域
- alt : 梯形模式
- shift : 平行四边形模式
- F1 : 统计信息
- s : 保存
- esc : 设置
- q : 上一张图片 (如果打开自动保存，则进行自动保存)
- e : 下一张图片 (如果打开自动保存，则进行自动保存)
- space(空格) : *智能标注* (必须进行微调)
- 左键 : 新建一个目标
- 右键 : 删除一个目标
- 中键 : 移动视图
- 滚轮 : 缩放视图

## AT 数据集格式
color label x1 y1 x2 y2 x3 y3 x4 y4
同样的，经典的交龙数据集格式我们同样支持，只需要在软件左上角切换数据集格式，就可以导入
 [交龙数据集](https://github.com/xinyang-go/LabelRoboMaster?tab=readme-ov-file#%E8%A3%85%E7%94%B2%E6%9D%BF%E7%B1%BB%E5%88%AB%E5%91%BD%E5%90%8D%E4%B8%8E%E7%B1%BB%E5%88%AB%E7%BC%96%E5%8F%B7)

我们拆分了检测头，让Label也可读一些。

不过在该标注工具中，我们完全移除了对5号的支持，并删除了3 4 5号大装甲。

新标注的数据集也不再希望支持五号，我们对所有五号装甲板都进行了Mask操作

后续标注时遇到轨道哨兵，3/4/5号进行mask操作即可。

这样做的理由是保护我们无辜的标注人员，不要陷入标注地狱啊()

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
| 0 |  G |
| 1 |  1 |
| 2 |  2 |
| 3 |  3 |
| 4 |  4 |
| 5  | O(前哨站) |
| 6 | Bs(基地小装甲) | 
| 7 | Bb(基地大装甲) |

### Points
从左上角开始逆时针旋转
- 左上角点归一化坐标x
- 左上角点归一化坐标y
- 左下角点归一化坐标x
- 左下角点归一化坐标y
- 右下角点归一化坐标x
- 右下角点归一化坐标y
- 右上角点归一化坐标x
- 右上角点归一化坐标y

## 标注规范 & 小灯必读

### 目标
在 RoboMaster(以下简称RM) 中，我们需要对上述数据集格式中的所有装甲板进行识别，一共是 **4 * 8 = 32** 种装甲板。

拆分了检测头的情况下，参考其他学校的实现，每一个类别大概需要2000张左右，才有希望训练出足够鲁棒的模型。也就是说，总数据集大概需要2w张

神经网络识别的最好情况无非是和预先的标注完全重合，所以标注的精度很大程度决定了识别精度的上限，所以标注时需要非常认真。

蒙版的图样的绿线需要作为识别图案的黑白交界处的分界线。


### 典型的标注错误

强品硬凑蒙版，忽略灯条，应以灯条为主

![错误1](docs/zh_cn/error1.png)

![错误2](docs/zh_cn/error2.png)

强拼硬凑蒙版，香蕉线(标的什么玩意)

![错误3](docs/zh_cn/error3.png)

左右不分

![错误4](docs/zh_cn/error4.png)

### TBD


## 其他
更多标注标准，参照哈工深的标注工具。都是非常宝贵的标注经验。

同时，有意愿交换数据集的学校可以联系我: 3159890292@qq.com

## 鸣谢
我们hard fork了SJTU的标注工具，精简了功能，提高了可能的可维护性（其实我也不知道算不算，只不过我们队内维护起来比较顺手）

[上海交通大学 LabelRoboMaster](https://github.com/xinyang-go/LabelRoboMaster)

[哈尔滨工业大学深圳 LabelRoboMaster](https://github.com/MonthMoonBird/LabelRoboMaster)

## License
MIT License
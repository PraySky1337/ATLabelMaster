#pragma once
#include <QDebug>
#include <QFile>
#include <QVector>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <opencv2/imgproc.hpp>
#include <openvino/openvino.hpp>
#include <types.hpp>                                             // Armor 定义

namespace ai {

struct Detector {
    Detector() = default;

    enum class Mode { OV_INT8_CPU, OV_FP32_CPU };

    void setupModel(const QString& assets_path) {
        label_map_[0] = "0";
        label_map_[1] = "1";
        label_map_[2] = "2";
        label_map_[3] = "3";
        label_map_[4] = "4";
        label_map_[5] = "5";
        label_map_[6] = "5";
        label_map_[7] = "5";
        label_map_[8] = "Bb";
        label_map_[9] = "Bs";
        label_map_[10] = "Bs";
        label_map_[11] = "Bs";
        label_map_[12] = "Bs";
        label_map_[12] = "13";

        const QString dir = assets_path + "/models/";
        try {
            const QString xml = dir + "model-opt-int8.xml";
            if (QFile::exists(xml)) {
                model_    = core_.read_model(xml.toStdString()); // 自动加载同名 .bin
                compiled_ = core_.compile_model(model_, "CPU");
                request_  = compiled_.create_infer_request();
                mode_     = Mode::OV_INT8_CPU;
                return;
            }
        } catch (const std::exception& e) {
            qWarning() << "OpenVINO INT8 failed:" << e.what();
        }
        try {
            const QString onnx = dir + "model-opt.onnx";
            if (!QFile::exists(onnx)) {
                qWarning() << "ONNX model not found:" << onnx;
                return;
            }
            model_    = core_.read_model(onnx.toStdString());
            compiled_ = core_.compile_model(model_, "CPU");
            request_  = compiled_.create_infer_request();
            mode_     = Mode::OV_FP32_CPU;
        } catch (const std::exception& e) {
            qWarning() << "OpenVINO FP32 failed:" << e.what();
        }
    }

    QVector<Armor> detect(cv::Mat& img) {
        QVector<Armor> results;
        if (!compiled_) {
            qWarning() << "SmartDetector not initialized.";
            return results;
        }

        // —— 1) 预处理（与 SmartModel 一致：640、左上角贴入、灰底=127）——
        constexpr int IN  = 640;
        const float scale = IN / float(std::max(img.cols, img.rows));
        cv::Mat resized;
        cv::resize(
            img, resized, {int(std::round(img.cols * scale)), int(std::round(img.rows * scale))});
        cv::Mat input(IN, IN, CV_8UC3, cv::Scalar(127, 127, 127));
        resized.copyTo(input(cv::Rect(0, 0, resized.cols, resized.rows)));

        // INT8：BGR、[0..255]；FP32：RGB、/255
        if (mode_ == Mode::OV_FP32_CPU)
            cv::cvtColor(input, input, cv::COLOR_BGR2RGB);

        // —— 2) 打包 NCHW float32 Tensor（INT8 也走 float32 但不 /255）——
        cv::Mat f32;
        const double sf = (mode_ == Mode::OV_INT8_CPU) ? 1.0 : (1.0 / 255.0);
        input.convertTo(f32, CV_32F, sf);
        ov::Tensor in_tensor(ov::element::f32, {1, 3, IN, IN});
        {
            std::vector<cv::Mat> ch(3);
            cv::split(f32, ch);
            float* dst         = in_tensor.data<float>();
            const size_t plane = size_t(IN) * IN;
            std::memcpy(dst + 0 * plane, ch[0].ptr<float>(), plane * sizeof(float));
            std::memcpy(dst + 1 * plane, ch[1].ptr<float>(), plane * sizeof(float));
            std::memcpy(dst + 2 * plane, ch[2].ptr<float>(), plane * sizeof(float));
        }

        // —— 3) 推理 ——
        request_.set_input_tensor(in_tensor);
        request_.infer();

        // —— 4) 读取输出（假设 [1, N, D]，兼容 {N,D}）——
        ov::Tensor out    = request_.get_output_tensor();
        const auto shp    = out.get_shape();
        const float* data = out.data<float>();
        int N = 0, D = 0;
        if (shp.size() == 3) {
            N = int(shp[1]);
            D = int(shp[2]);
        } else if (shp.size() == 2) {
            N = int(shp[0]);
            D = int(shp[1]);
        } else {
            qWarning() << "Unexpected output shape rank:" << int(shp.size());
            return results;
        }
        if (D < 22) {
            qWarning() << "Output D too small:" << D;
            return results;
        }

        auto sigmoid     = [](float x) { return 1.f / (1.f + std::exp(-x)); };
        auto inv_sigmoid = [](float x) { return -std::log(1 / x - 1); };
        const float th   = inv_sigmoid(0.5f);

        // —— 5) 解析行，与 SmartModel 完全一致 ——
        QVector<Armor> cand;
        cand.reserve(N);
        for (int i = 0; i < N; ++i) {
            const float* r = data + i * D;
            if (r[8] < th)
                continue;            // logit 阈值
            Armor a;
            a.score = sigmoid(r[8]); // 置信度

            // 四角点（原图坐标 = /scale；左上贴入，无偏移）
            a.p0 = QPointF(r[0] / scale, r[1] / scale);
            a.p1 = QPointF(r[2] / scale, r[3] / scale);
            a.p2 = QPointF(r[4] / scale, r[5] / scale);
            a.p3 = QPointF(r[6] / scale, r[7] / scale);

            // 颜色 4 类 & 标签 9 类
            const int color_id = argmax(r + 9, 4);
            const int tag_id   = argmax(r + 13, 9);
            a.color =
                (color_id == 0   ? "B"
                 : color_id == 1 ? "R"
                 : color_id == 2 ? "G"
                                 : "P");
            a.cls = label_map_[tag_id];

            cand.push_back(a);
        }

        // —— 6) NMS：按四角点外接矩形重叠即抑制（thres=0 等价）——
        std::sort(cand.begin(), cand.end(), [](const Armor& A, const Armor& B) {
            return A.score > B.score;
        });
        std::vector<char> removed(cand.size(), 0);
        for (int i = 0; i < cand.size(); ++i) {
            if (removed[i])
                continue;
            results.push_back(cand[i]);
            for (int j = i + 1; j < cand.size(); ++j) {
                if (removed[j])
                    continue;
                if (isOverlap(cand[i], cand[j]))
                    removed[j] = 1;
            }
        }
        return results;
    }

private:
    Mode mode_{Mode::OV_FP32_CPU};
    ov::Core core_;
    std::shared_ptr<ov::Model> model_;
    ov::CompiledModel compiled_;
    ov::InferRequest request_;
    QHash<int, QString> label_map_;

    static int argmax(const float* p, int len) {
        int k = 0;
        for (int i = 1; i < len; ++i)
            if (p[i] > p[k])
                k = i;
        return k;
    }

    static bool isOverlap(const Armor& a, const Armor& b) {
        auto rect = [](const Armor& s) {
            const float xmin = std::min({s.p0.x(), s.p1.x(), s.p2.x(), s.p3.x()});
            const float xmax = std::max({s.p0.x(), s.p1.x(), s.p2.x(), s.p3.x()});
            const float ymin = std::min({s.p0.y(), s.p1.y(), s.p2.y(), s.p3.y()});
            const float ymax = std::max({s.p0.y(), s.p1.y(), s.p2.y(), s.p3.y()});
            return cv::Rect2f(xmin, ymin, xmax - xmin, ymax - ymin);
        };
        return (rect(a) & rect(b)).area() > 0;
    }
};

} // namespace ai

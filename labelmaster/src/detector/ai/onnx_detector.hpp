#pragma once
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <opencv2/imgproc.hpp>
#include <openvino/openvino.hpp>
#include <types.hpp>  // Armor 定义

namespace ai {

/**
 * OpenVINO-based YOLO26 end-to-end pose detector.
 * Uses best.onnx (end2end) model with YOLO26 preprocessing/postprocessing.
 *
 * Output format: (batch_size, max_det, 6 + nk)
 *   - [x, y, w, h, max_class_prob, class_index, keypoints...]
 *   - keypoints: nkpt * ndim (typically 4 points * 2 coords = 8 values)
 */
struct ONNXDetector {
    ONNXDetector() = default;

    ~ONNXDetector() = default;

    void setupModel(const QString& assets_path) {
        // Map 64 classes to armor labels
        // Format: {Color}{Size}{Number} - B/R/G/P = Blue/Red/Gray/Purple, s/b = small/big, 0-7
        // We map to simplified armor format: color + cls + size
        initClassMapping();

        const QString dir = assets_path + "/models/";
        const QString onnx_path = dir + "best.onnx";

        if (!QFile::exists(onnx_path)) {
            qWarning() << "ONNX model not found:" << onnx_path;
            return;
        }

        try {
            // Load ONNX model using OpenVINO
            model_ = core_.read_model(onnx_path.toStdString());
            compiled_ = core_.compile_model(model_, "CPU");
            request_ = compiled_.create_infer_request();

            // Get input/output info
            auto input_shape = model_->input().get_shape();
            auto output_shape = model_->output().get_shape();

            input_shape_.assign(input_shape.begin(), input_shape.end());
            output_shape_.assign(output_shape.begin(), output_shape.end());

            // Print model info
            qInfo() << "ONNX Model loaded:" << onnx_path;
            qInfo() << "Input shape:" << QVector<size_t>(input_shape_.begin(), input_shape_.end());
            qInfo() << "Output shape:" << QVector<size_t>(output_shape_.begin(), output_shape_.end());

            initialized_ = true;

        } catch (const ov::Exception& e) {
            qWarning() << "OpenVINO error:" << e.what();
        } catch (const std::exception& e) {
            qWarning() << "Failed to load ONNX model:" << e.what();
        }
    }

    QVector<Armor> detect(cv::Mat& img) {
        QVector<Armor> results;
        if (!initialized_ || !compiled_) {
            qWarning() << "ONNXDetector not initialized.";
            return results;
        }

        try {
            // 1. Preprocess image
            auto prep_result = preprocessImage(img);

            // 2. Run inference
            auto output_tensor = runInference(prep_result);

            // 3. Postprocess results
            results = postprocess(output_tensor, prep_result.orig_shape, prep_result.ratio_pad);

        } catch (const std::exception& e) {
            qWarning() << "Detection error:" << e.what();
        }

        return results;
    }

private:
    struct PreprocessResult {
        std::vector<float> data;
        std::array<int, 2> orig_shape;  // {h, w}
        std::array<float, 4> ratio_pad;  // {ratio, ratio, pad_w, pad_h}
    };

    PreprocessResult preprocessImage(const cv::Mat& img) {
        constexpr int imgsz = 640;
        const int orig_h = img.rows;
        const int orig_w = img.cols;

        // Calculate letterbox parameters (same as ultralytics LetterBox)
        // stride = 32, auto = false
        const float ratio = std::min(static_cast<float>(imgsz) / orig_h,
                                     static_cast<float>(imgsz) / orig_w);
        const int new_unpad_w = std::round(orig_w * ratio);
        const int new_unpad_h = std::round(orig_h * ratio);

        // Make sure padding is divisible by stride
        const float pad_w = std::round((imgsz - new_unpad_w) / 2.0f);
        const float pad_h = std::round((imgsz - new_unpad_h) / 2.0f);

        // Resize image
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(new_unpad_w, new_unpad_h));

        // Letterbox with padding (gray background = 114, 114, 114)
        cv::Mat letterboxed(imgsz, imgsz, CV_8UC3, cv::Scalar(114, 114, 114));
        resized.copyTo(letterboxed(cv::Rect(pad_w, pad_h, new_unpad_w, new_unpad_h)));

        // BGR to RGB and normalize to [0, 1]
        cv::Mat rgb;
        cv::cvtColor(letterboxed, rgb, cv::COLOR_BGR2RGB);

        // Convert to float and normalize
        cv::Mat float_img;
        rgb.convertTo(float_img, CV_32F, 1.0 / 255.0);

        // Convert to CHW format
        std::vector<float> data;
        data.reserve(3 * imgsz * imgsz);

        std::vector<cv::Mat> channels(3);
        cv::split(float_img, channels);

        for (int c = 0; c < 3; ++c) {
            const float* ptr = channels[c].ptr<float>();
            data.insert(data.end(), ptr, ptr + imgsz * imgsz);
        }

        return PreprocessResult{
            std::move(data),
            {orig_h, orig_w},
            {ratio, ratio, pad_w, pad_h}
        };
    }

    std::vector<float> runInference(const PreprocessResult& prep) {
        constexpr int imgsz = 640;

        // Create input tensor
        ov::Tensor input_tensor(ov::element::f32, {1, 3, imgsz, imgsz});
        std::memcpy(input_tensor.data<float>(), prep.data.data(),
                    prep.data.size() * sizeof(float));

        // Run inference
        request_.set_input_tensor(input_tensor);
        request_.infer();

        // Get output
        ov::Tensor output_tensor = request_.get_output_tensor();
        size_t output_size = output_tensor.get_size();

        std::vector<float> output_data(output_size);
        const float* output_ptr = output_tensor.data<const float>();
        std::memcpy(output_data.data(), output_ptr, output_size * sizeof(float));

        return output_data;
    }

    QVector<Armor> postprocess(const std::vector<float>& output,
                               const std::array<int, 2>& orig_shape,
                               const std::array<float, 4>& ratio_pad) {
        QVector<Armor> results;
        constexpr float conf_thres = 0.25f;
        constexpr int imgsz = 640;

        // Output shape: (1, max_det, 6 + nk)
        // max_det = output_shape_[1], nk = output_shape_[2] - 6
        if (output_shape_.size() != 3 || output_shape_[0] != 1) {
            qWarning() << "Unexpected output shape";
            return results;
        }

        const size_t max_det = output_shape_[1];
        const size_t feature_dim = output_shape_[2];
        const size_t nk = feature_dim - 6;  // Number of keypoint values (should be 8 for 4 points)

        if (nk < 8) {
            qWarning() << "Not enough keypoint values:" << nk;
            return results;
        }

        const int orig_h = orig_shape[0];
        const int orig_w = orig_shape[1];
        const float ratio = ratio_pad[0];
        const float pad_w = ratio_pad[2];
        const float pad_h = ratio_pad[3];

        // Process each detection
        for (size_t i = 0; i < max_det; ++i) {
            const float* det = &output[i * feature_dim];

            // Format: [x, y, w, h, max_class_prob, class_index, kpts...]
            const float x = det[0];
            const float y = det[1];
            const float w = det[2];
            const float h = det[3];
            const float conf = det[4];
            const int cls_idx = static_cast<int>(std::round(det[5]));

            // Confidence filter
            if (conf < conf_thres) {
                continue;
            }

            // Extract keypoints (4 points, 2 coords each)
            std::vector<std::array<float, 2>> kpts(4);
            for (size_t j = 0; j < 4; ++j) {
                kpts[j][0] = det[6 + j * 2];     // x
                kpts[j][1] = det[6 + j * 2 + 1]; // y
            }

            // Scale keypoints back to original image
            // Formula: (coord - pad) / ratio
            Armor armor;
            armor.score = conf;

            // Map 64 classes to armor format
            mapClassToArmor(cls_idx, armor);

            // Convert and scale 4 corner points
            // Keypoint order: typically top-left, top-right, bottom-right, bottom-left
            // We'll map them to p0, p1, p2, p3 in the expected order
            armor.p0 = scalePoint(kpts[0][0], kpts[0][1], ratio, pad_w, pad_h);
            armor.p1 = scalePoint(kpts[1][0], kpts[1][1], ratio, pad_w, pad_h);
            armor.p2 = scalePoint(kpts[2][0], kpts[2][1], ratio, pad_w, pad_h);
            armor.p3 = scalePoint(kpts[3][0], kpts[3][1], ratio, pad_w, pad_h);

            // Validate keypoints are within image bounds
            if (isValidArmor(armor, orig_w, orig_h)) {
                results.append(armor);
            }
        }

        return results;
    }

    QPointF scalePoint(float x, float y, float ratio, float pad_w, float pad_h) {
        return QPointF(
            (x - pad_w) / ratio,
            (y - pad_h) / ratio
        );
    }

    bool isValidArmor(const Armor& armor, int img_w, int img_h) {
        // Check if all corners are within reasonable bounds
        auto points = {armor.p0, armor.p1, armor.p2, armor.p3};
        for (const auto& pt : points) {
            if (pt.x() < -10 || pt.x() > img_w + 10 ||
                pt.y() < -10 || pt.y() > img_h + 10) {
                return false;
            }
        }
        return true;
    }

    void initClassMapping() {
        // YOLO26 has 64 classes: {Color}{Size}{Number}
        // Color: B(Blue), R(Red), G(Gray), P(Purple)
        // Size: s(small), b(big)
        // Number: 0-7

        // Map to our simplified armor format
        // color: B/R/G/P
        // cls: G/1/2/3/4/5/O/B (Sentinel/1/2/3/4/5/Outpost/Base)
        // size: true for big armor

        for (int color = 0; color < 4; ++color) {
            for (int size = 0; size < 2; ++size) {
                for (int num = 0; num < 8; ++num) {
                    int cls_idx = color * 16 + size * 8 + num;

                    QString color_str;
                    switch (color) {
                        case 0: color_str = "B"; break;  // Blue
                        case 1: color_str = "R"; break;  // Red
                        case 2: color_str = "G"; break;  // Gray
                        case 3: color_str = "P"; break;  // Purple
                    }

                    QString cls_str;
                    switch (num) {
                        case 0: cls_str = "G"; break;  // Sentinel/0
                        case 1: cls_str = "1"; break;  // Number 1 (big armor)
                        case 2: cls_str = "2"; break;
                        case 3: cls_str = "3"; break;
                        case 4: cls_str = "4"; break;
                        case 5: cls_str = "5"; break;
                        case 6: cls_str = "O"; break;  // Outpost
                        case 7: cls_str = "B"; break;  // Base
                    }

                    class_map_[cls_idx] = ClassInfo{
                        color_str,
                        cls_str,
                        size == 1 || num == 1 || num == 7
                        // Big armor: size='b' (model output) OR numbers 1,7 (always big)
                        // Note: Sentry(0), 3, 4, 5 use the model's size field directly
                    };
                }
            }
        }
    }

    void mapClassToArmor(int cls_idx, Armor& armor) {
        auto it = class_map_.find(cls_idx);
        if (it != class_map_.end()) {
            armor.color = it->color;
            armor.cls = it->cls;
            armor.size = it->is_big;
        } else {
            // Default fallback
            armor.color = "B";
            armor.cls = "G";
            armor.size = false;
        }
    }

    struct ClassInfo {
        QString color;
        QString cls;
        bool is_big;
    };

    ov::Core core_;
    std::shared_ptr<ov::Model> model_;
    ov::CompiledModel compiled_;
    ov::InferRequest request_;
    std::vector<size_t> input_shape_;
    std::vector<size_t> output_shape_;
    QHash<int, ClassInfo> class_map_;
    bool initialized_ = false;
};

} // namespace ai

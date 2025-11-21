#include "detectors/Defect.h"
#include "utils/Logger.h"
#include <iostream>
#include <cassert>

using namespace inspection;

/**
 * @brief Defect構造体のテスト
 */
void testDefectBasic() {
    std::cout << "=== Test 1: Defect Basic Construction ===" << std::endl;

    // デフォルトコンストラクタのテスト
    Defect defect1;
    assert(defect1.type == DefectType::Unknown);
    assert(defect1.confidence == 0.0);
    assert(!defect1.isValid());
    std::cout << "✓ Default constructor works" << std::endl;

    // パラメータ付きコンストラクタのテスト
    cv::Rect bbox(100, 200, 50, 30);
    Defect defect2(DefectType::Scratch, bbox, 0.95);
    assert(defect2.type == DefectType::Scratch);
    assert(defect2.bbox == bbox);
    assert(defect2.confidence == 0.95);
    assert(defect2.center.x == 125.0f);  // 100 + 50/2
    assert(defect2.center.y == 215.0f);  // 200 + 30/2
    assert(defect2.area == 1500.0);      // 50 * 30
    assert(defect2.isValid());
    std::cout << "✓ Parametrized constructor works" << std::endl;
}

/**
 * @brief DefectTypeの文字列変換テスト
 */
void testDefectTypeConversion() {
    std::cout << "\n=== Test 2: DefectType Conversion ===" << std::endl;

    // enum → string
    assert(defectTypeToString(DefectType::Scratch) == "Scratch");
    assert(defectTypeToString(DefectType::Stain) == "Stain");
    assert(defectTypeToString(DefectType::Discoloration) == "Discoloration");
    assert(defectTypeToString(DefectType::Deformation) == "Deformation");
    assert(defectTypeToString(DefectType::Unknown) == "Unknown");
    std::cout << "✓ DefectType to string conversion works" << std::endl;

    // string → enum
    assert(stringToDefectType("Scratch") == DefectType::Scratch);
    assert(stringToDefectType("Stain") == DefectType::Stain);
    assert(stringToDefectType("Discoloration") == DefectType::Discoloration);
    assert(stringToDefectType("Deformation") == DefectType::Deformation);
    assert(stringToDefectType("Unknown") == DefectType::Unknown);
    assert(stringToDefectType("InvalidType") == DefectType::Unknown);
    std::cout << "✓ String to DefectType conversion works" << std::endl;
}

/**
 * @brief 色マッピングのテスト
 */
void testDefectColor() {
    std::cout << "\n=== Test 3: Defect Color Mapping ===" << std::endl;

    cv::Scalar scratchColor = getDefectColor(DefectType::Scratch);
    assert(scratchColor == cv::Scalar(0, 0, 255));  // 赤
    std::cout << "✓ Scratch color: Red (0,0,255)" << std::endl;

    cv::Scalar stainColor = getDefectColor(DefectType::Stain);
    assert(stainColor == cv::Scalar(0, 165, 255));  // オレンジ
    std::cout << "✓ Stain color: Orange (0,165,255)" << std::endl;

    Defect defect(DefectType::Scratch, cv::Rect(0, 0, 10, 10), 0.9);
    assert(defect.getColor() == scratchColor);
    std::cout << "✓ Defect::getColor() works" << std::endl;
}

/**
 * @brief JSON変換のテスト
 */
void testDefectJsonSerialization() {
    std::cout << "\n=== Test 4: JSON Serialization ===" << std::endl;

    // Defect作成
    Defect original(DefectType::Stain, cv::Rect(50, 100, 80, 60), 0.87);
    original.circularity = 0.75;
    original.contour = {
        cv::Point(50, 100),
        cv::Point(130, 100),
        cv::Point(130, 160),
        cv::Point(50, 160)
    };

    // JSON変換
    json j = original.toJson();
    std::cout << "JSON output:\n" << j.dump(2) << std::endl;

    // JSON検証
    assert(j["type"] == "Stain");
    assert(j["bbox"]["x"] == 50);
    assert(j["bbox"]["y"] == 100);
    assert(j["bbox"]["width"] == 80);
    assert(j["bbox"]["height"] == 60);
    assert(j["confidence"] == 0.87);
    assert(j["center"]["x"] == 90.0f);  // 50 + 80/2
    assert(j["center"]["y"] == 130.0f); // 100 + 60/2
    assert(j["area"] == 4800.0);        // 80 * 60
    assert(j["circularity"] == 0.75);
    assert(j["contour"].size() == 4);
    std::cout << "✓ toJson() works correctly" << std::endl;

    // JSONから復元
    Defect restored = Defect::fromJson(j);
    assert(restored.type == original.type);
    assert(restored.bbox == original.bbox);
    assert(restored.confidence == original.confidence);
    assert(restored.center.x == original.center.x);
    assert(restored.center.y == original.center.y);
    assert(restored.area == original.area);
    assert(restored.circularity == original.circularity);
    assert(restored.contour.size() == original.contour.size());
    std::cout << "✓ fromJson() works correctly" << std::endl;
}

/**
 * @brief 複数欠陥のJSON変換テスト
 */
void testDefectsJsonSerialization() {
    std::cout << "\n=== Test 5: Multiple Defects JSON Serialization ===" << std::endl;

    // 複数の欠陥を作成
    Defects defects;
    defects.push_back(Defect(DefectType::Scratch, cv::Rect(10, 20, 30, 40), 0.95));
    defects.push_back(Defect(DefectType::Stain, cv::Rect(100, 200, 50, 60), 0.85));
    defects.push_back(Defect(DefectType::Discoloration, cv::Rect(300, 400, 70, 80), 0.75));

    // JSON配列に変換
    json j = defectsToJson(defects);
    std::cout << "Defects JSON:\n" << j.dump(2) << std::endl;

    assert(j.is_array());
    assert(j.size() == 3);
    assert(j[0]["type"] == "Scratch");
    assert(j[1]["type"] == "Stain");
    assert(j[2]["type"] == "Discoloration");
    std::cout << "✓ defectsToJson() works correctly" << std::endl;

    // JSON配列から復元
    Defects restored = defectsFromJson(j);
    assert(restored.size() == 3);
    assert(restored[0].type == DefectType::Scratch);
    assert(restored[1].type == DefectType::Stain);
    assert(restored[2].type == DefectType::Discoloration);
    assert(restored[0].confidence == 0.95);
    assert(restored[1].confidence == 0.85);
    assert(restored[2].confidence == 0.75);
    std::cout << "✓ defectsFromJson() works correctly" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Defect Data Structure Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    try {
        testDefectBasic();
        testDefectTypeConversion();
        testDefectColor();
        testDefectJsonSerialization();
        testDefectsJsonSerialization();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}

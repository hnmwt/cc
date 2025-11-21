#include "utils/ConfigManager.h"
#include <fstream>
#include <iostream>

namespace inspection {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& configPath) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Error: Cannot open config file: " << configPath << std::endl;
        std::cerr << "[ConfigManager] Loading default configuration..." << std::endl;
        config_ = createDefaultConfig();
        loaded_ = true;
        return false;
    }

    try {
        file >> config_;
        loaded_ = true;
        std::cout << "[ConfigManager] Successfully loaded config from: " << configPath << std::endl;
        return true;
    } catch (const json::parse_error& e) {
        std::cerr << "[ConfigManager] Error: Failed to parse JSON: " << e.what() << std::endl;
        std::cerr << "[ConfigManager] Loading default configuration..." << std::endl;
        config_ = createDefaultConfig();
        loaded_ = true;
        return false;
    }
}

bool ConfigManager::saveConfig(const std::string& configPath) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "[ConfigManager] Error: Cannot open file for writing: " << configPath << std::endl;
        return false;
    }

    try {
        // インデント付きで保存（読みやすさのため）
        file << config_.dump(2);
        std::cout << "[ConfigManager] Successfully saved config to: " << configPath << std::endl;
        return true;
    } catch (const json::exception& e) {
        std::cerr << "[ConfigManager] Error: Failed to write JSON: " << e.what() << std::endl;
        return false;
    }
}

json ConfigManager::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void ConfigManager::setConfig(const json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    loaded_ = true;
}

bool ConfigManager::isLoaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loaded_;
}

void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.clear();
    loaded_ = false;
}

void ConfigManager::loadDefaultConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = createDefaultConfig();
    loaded_ = true;
    std::cout << "[ConfigManager] Loaded default configuration" << std::endl;
}

json ConfigManager::createDefaultConfig() {
    return json{
        {"application", {
            {"name", "InspectionApp"},
            {"version", "1.0.0"},
            {"log_level", "info"}
        }},
        {"camera", {
            {"device_id", 0},
            {"width", 1920},
            {"height", 1080},
            {"fps", 30},
            {"auto_exposure", true},
            {"exposure", 0},
            {"gain", 0},
            {"white_balance", "auto"}
        }},
        {"image_processing", {
            {"default_pipeline", json::array({
                {
                    {"type", "grayscale"},
                    {"enabled", true}
                },
                {
                    {"type", "gaussian_blur"},
                    {"enabled", true},
                    {"params", {
                        {"kernel_size", 5},
                        {"sigma", 1.0}
                    }}
                },
                {
                    {"type", "threshold"},
                    {"enabled", false},
                    {"params", {
                        {"threshold", 128},
                        {"max_value", 255},
                        {"method", "binary"}
                    }}
                }
            })}
        }},
        {"detection", {
            {"detectors", json::array({
                {
                    {"type", "template_matcher"},
                    {"enabled", true},
                    {"params", {
                        {"threshold", 0.85},
                        {"method", "cv::TM_CCOEFF_NORMED"}
                    }}
                },
                {
                    {"type", "feature_detector"},
                    {"enabled", true},
                    {"params", {
                        {"min_area", 100},
                        {"max_area", 10000},
                        {"min_circularity", 0.5}
                    }}
                }
            })},
            {"defect_types", json::array({
                {
                    {"type", "scratch"},
                    {"color", json::array({0, 0, 255})},
                    {"min_confidence", 0.7}
                },
                {
                    {"type", "stain"},
                    {"color", json::array({0, 255, 255})},
                    {"min_confidence", 0.75}
                },
                {
                    {"type", "discoloration"},
                    {"color", json::array({0, 255, 0})},
                    {"min_confidence", 0.8}
                },
                {
                    {"type", "deformation"},
                    {"color", json::array({255, 0, 0})},
                    {"min_confidence", 0.85}
                }
            })}
        }},
        {"server", {
            {"http", {
                {"enabled", true},
                {"host", "0.0.0.0"},
                {"port", 8080},
                {"api_base_path", "/api/v1"}
            }},
            {"websocket", {
                {"enabled", true},
                {"port", 8081},
                {"ping_interval_ms", 30000}
            }},
            {"external_trigger", {
                {"enabled", true},
                {"protocol", "tcp"},
                {"host", "0.0.0.0"},
                {"port", 5000},
                {"timeout_ms", 5000}
            }}
        }},
        {"data_output", {
            {"csv", {
                {"enabled", true},
                {"output_dir", "./data/output/csv"},
                {"filename_format", "inspection_%Y-%m-%d.csv"},
                {"encoding", "utf-8-bom"},
                {"auto_save", true}
            }},
            {"images", {
                {"enabled", true},
                {"output_dir", "./data/output/images"},
                {"save_original", true},
                {"save_processed", true},
                {"save_marked", true},
                {"format", "jpg"},
                {"quality", 95}
            }}
        }},
        {"reference_images", {
            {"directory", "./data/reference"},
            {"auto_load", true}
        }},
        {"ui", {
            {"window_width", 1280},
            {"window_height", 720},
            {"show_fps", true},
            {"show_processing_time", true}
        }},
        {"performance", {
            {"max_threads", 4},
            {"max_queue_size", 100},
            {"enable_gpu", false}
        }}
    };
}

} // namespace inspection

#include "pipeline/Pipeline.h"
#include "utils/Logger.h"
#include <algorithm>

namespace inspection {

void Pipeline::addFilter(std::unique_ptr<FilterBase> filter) {
    if (!filter) {
        LOG_WARN("Attempted to add null filter to pipeline");
        return;
    }

    LOG_INFO("Adding filter to pipeline: {}", filter->getName());
    filters_.push_back(std::move(filter));
}

bool Pipeline::removeFilter(size_t index) {
    if (index >= filters_.size()) {
        LOG_WARN("Filter index out of range: {}", index);
        return false;
    }

    LOG_INFO("Removing filter from pipeline: {}", filters_[index]->getName());
    filters_.erase(filters_.begin() + index);
    return true;
}

void Pipeline::clear() {
    LOG_INFO("Clearing pipeline (removed {} filters)", filters_.size());
    filters_.clear();
}

cv::Mat Pipeline::process(const cv::Mat& input) {
    if (input.empty()) {
        LOG_ERROR("Pipeline::process - Input image is empty");
        return cv::Mat();
    }

    if (filters_.empty()) {
        LOG_WARN("Pipeline is empty, returning input image unchanged");
        return input.clone();
    }

    cv::Mat current = input;

    for (auto& filter : filters_) {
        if (!filter->isEnabled()) {
            LOG_DEBUG("Skipping disabled filter: {}", filter->getName());
            continue;
        }

        try {
            current = filter->process(current);

            if (current.empty()) {
                LOG_ERROR("Filter '{}' produced empty output", filter->getName());
                return cv::Mat();
            }

        } catch (const std::exception& e) {
            LOG_ERROR("Exception in filter '{}': {}", filter->getName(), e.what());
            return cv::Mat();
        }
    }

    return current;
}

Pipeline::ProcessingResult Pipeline::processWithIntermediates(const cv::Mat& input) {
    ProcessingResult result;

    if (input.empty()) {
        LOG_ERROR("Pipeline::processWithIntermediates - Input image is empty");
        result.success = false;
        result.errorMessage = "Input image is empty";
        return result;
    }

    if (filters_.empty()) {
        LOG_WARN("Pipeline is empty, returning input image unchanged");
        result.finalImage = input.clone();
        result.intermediateImages.push_back(input.clone());
        return result;
    }

    auto startTotal = std::chrono::high_resolution_clock::now();

    cv::Mat current = input;
    result.intermediateImages.push_back(input.clone());

    for (auto& filter : filters_) {
        if (!filter->isEnabled()) {
            LOG_DEBUG("Skipping disabled filter: {}", filter->getName());
            continue;
        }

        try {
            cv::Mat output;
            double processingTime = applyFilterTimed(filter.get(), current, output);

            if (output.empty()) {
                LOG_ERROR("Filter '{}' produced empty output", filter->getName());
                result.success = false;
                result.errorMessage = "Filter '" + filter->getName() + "' produced empty output";
                return result;
            }

            current = output;
            result.intermediateImages.push_back(output.clone());
            result.filterNames.push_back(filter->getName());
            result.processingTimes.push_back(processingTime);

            LOG_DEBUG("Filter '{}' processed in {:.2f} ms", filter->getName(), processingTime);

        } catch (const std::exception& e) {
            LOG_ERROR("Exception in filter '{}': {}", filter->getName(), e.what());
            result.success = false;
            result.errorMessage = "Exception in filter '" + filter->getName() + "': " + e.what();
            return result;
        }
    }

    auto endTotal = std::chrono::high_resolution_clock::now();
    result.totalTime = std::chrono::duration<double, std::milli>(endTotal - startTotal).count();

    result.finalImage = current;

    LOG_INFO("Pipeline processing complete: {} filters applied in {:.2f} ms",
             result.filterNames.size(), result.totalTime);

    return result;
}

size_t Pipeline::getFilterCount() const {
    return filters_.size();
}

FilterBase* Pipeline::getFilter(size_t index) const {
    if (index >= filters_.size()) {
        return nullptr;
    }
    return filters_[index].get();
}

std::vector<std::string> Pipeline::getFilterNames() const {
    std::vector<std::string> names;
    names.reserve(filters_.size());

    for (const auto& filter : filters_) {
        names.push_back(filter->getName());
    }

    return names;
}

json Pipeline::toJson() const {
    json pipelineJson = json::array();

    for (const auto& filter : filters_) {
        json filterJson;
        filterJson["type"] = filter->getType();
        filterJson["name"] = filter->getName();
        filterJson["enabled"] = filter->isEnabled();
        filterJson["params"] = filter->getParameters();

        pipelineJson.push_back(filterJson);
    }

    return pipelineJson;
}

void Pipeline::fromJson(const json& config) {
    // Note: This is a placeholder implementation
    // Full implementation would require a FilterFactory to create filters by type
    LOG_WARN("Pipeline::fromJson is not fully implemented yet. Requires FilterFactory.");

    // Example of what would be implemented:
    // for (const auto& filterConfig : config) {
    //     std::string type = filterConfig["type"];
    //     auto filter = FilterFactory::createFilter(type);
    //     if (filter) {
    //         filter->setParameters(filterConfig["params"]);
    //         filter->setEnabled(filterConfig["enabled"]);
    //         addFilter(std::move(filter));
    //     }
    // }
}

bool Pipeline::isEmpty() const {
    return filters_.empty();
}

double Pipeline::applyFilterTimed(FilterBase* filter, const cv::Mat& input, cv::Mat& output) {
    auto start = std::chrono::high_resolution_clock::now();

    output = filter->process(input);

    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace inspection

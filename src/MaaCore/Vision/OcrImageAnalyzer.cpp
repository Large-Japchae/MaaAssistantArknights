#include "OcrImageAnalyzer.h"

#include <regex>
#include <unordered_map>

#include "Config/Miscellaneous/OcrConfig.h"
#include "Config/Miscellaneous/OcrPack.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

using namespace asst;

OcrImageAnalyzer::ResultsVecOpt OcrImageAnalyzer::analyze() const
{
    OcrPack* ocr_ptr = nullptr;
    if (m_params.use_char_model) {
        ocr_ptr = &CharOcr::get_instance();
    }
    else {
        ocr_ptr = &WordOcr::get_instance();
    }
    ResultsVec raw_results = ocr_ptr->recognize(make_roi(m_image, m_roi), m_params.without_det);
    ocr_ptr = nullptr;

    /* post process */
    ResultsVec results_vec;
    for (Result& res : raw_results) {
        if (res.text.empty() || std::isnan(res.score) || std::isinf(res.score)) {
            continue;
        }

        postproc_rect_(res);
        postproc_trim_(res);
        postproc_equivalence_(res);
        postproc_replace_(res);

        if (!filter_and_replace_by_required_(res)) {
            continue;
        }

        results_vec.emplace_back(std::move(res));
    }

    Log.trace("Proced", results_vec);

    if (results_vec.empty()) {
        return std::nullopt;
    }

    m_result = std::move(results_vec);
    return m_result;
}

void OcrImageAnalyzer::postproc_rect_(Result& res) const
{
    if (m_params.without_det) {
        res.rect = m_roi;
    }
    else {
        res.rect.x += m_roi.x;
        res.rect.y += m_roi.y;
    }
}

void OcrImageAnalyzer::postproc_trim_(Result& res) const
{
    utils::string_trim(res.text);
}

void OcrImageAnalyzer::postproc_equivalence_(Result& res) const
{
    auto& ocr_config = OcrConfig::get_instance();
    res.text = ocr_config.process_equivalence_class(res.text);
}

void OcrImageAnalyzer::postproc_replace_(Result& res) const
{
    if (m_params.replace.empty()) {
        return;
    }

    for (const auto& [regex, new_str] : m_params.replace) {
        if (m_params.replace_full) {
            if (std::regex_search(res.text, std::regex(regex))) {
                res.text = new_str;
            }
        }
        else {
            res.text = std::regex_replace(res.text, std::regex(regex), new_str);
        }
    }
}

bool OcrImageAnalyzer::filter_and_replace_by_required_(Result& res) const
{
    if (m_params.required.empty()) {
        return true;
    }

    if (m_params.full_match) {
        return ranges::find(m_params.required, res.text) != m_params.required.cend();
    }
    else {
        auto is_sub = [&res](const std::string& str) -> bool {
            if (res.text.find(str) == std::string::npos) {
                return false;
            }
            res.text = str;
            return true;
        };
        return ranges::find_if(m_params.required, is_sub) != m_params.required.cend();
    };
}

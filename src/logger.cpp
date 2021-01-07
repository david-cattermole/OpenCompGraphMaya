/*
 * Copyright (C) 2020 David Cattermole.
 *
 * This file is part of OpenCompGraphMaya.
 *
 * OpenCompGraphMaya is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OpenCompGraphMaya is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenCompGraphMaya.  If not, see <https://www.gnu.org/licenses/>.
 * ====================================================================
 *
 * Integrate the spdlog utility with Maya's output streams
 * (MStreamUtils).
 */

// STL
#include <memory>
#include <mutex>

// Maya
#include <maya/MStreamUtils.h>

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/version.h>
#include <spdlog/sinks/base_sink.h>

namespace open_comp_graph_maya {
namespace log {

namespace internal {

template<typename Mutex>
class LoggerMayaSink final : public spdlog::sinks::base_sink<Mutex>
{
public:
    explicit LoggerMayaSink() {}
    LoggerMayaSink(const LoggerMayaSink &) = delete;
    LoggerMayaSink &operator=(const LoggerMayaSink &) = delete;

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

        MStreamUtils::stdErrorStream().write(
            formatted.data(), static_cast<std::streamsize>(formatted.size()));
    }
    void flush_() override {
        MStreamUtils::stdErrorStream() << std::flush;
    }
};

using maya_sink_mt = LoggerMayaSink<std::mutex>;
using maya_sink_st = LoggerMayaSink<spdlog::details::null_mutex>;

} // namespace internal

const char kLOGGER_NAME[] = "open_comp_graph_maya_logger";

void initialize() {
    auto maya_sink = std::make_shared<internal::LoggerMayaSink<std::mutex>>();
    auto logger = std::make_shared<spdlog::logger>(kLOGGER_NAME, maya_sink);
    spdlog::register_logger(logger);
}

void deinitialize() {
    spdlog::shutdown();
}

void set_level(const char *level_name) {
    if (level_name == "error") {
        spdlog::set_level(spdlog::level::err);
    } else if (level_name == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (level_name == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (level_name == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::warn("Invalid logging level given.");
    }
    return;
}

std::shared_ptr<spdlog::logger> get_logger() {
    return spdlog::get(kLOGGER_NAME);
}

} // namespace log
} // namespace open_comp_graph_maya

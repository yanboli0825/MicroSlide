#pragma once

#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <memory>
#include <string>

/**
 * @brief 日志管理器
 *
 * 提供项目全局的日志服务，支持：
 * - 控制台输出（彩色）
 * - 按日期分类的文件输出
 * - 统一的日志级别和格式控制
 * - 线程安全（spdlog 内部处理）
 *
 * 使用示例：
 *   Logger::Init("./log");
 *   LOGGER_INFO("Application started");
 *   LOGGER_ERROR("An error occurred: {}", error_msg);
 */
class Logger
{
public:
    /**
     * @brief 初始化日志系统
     *
     * @param logDir 日志文件存储目录（相对或绝对路径）
     *               默认为 "./log"，如果不存在会自动创建
     * @param level 日志级别（trace, debug, info, warn, err, critical, off）
     *              默认为 info
     *
     * @return true 初始化成功；false 初始化失败
     */
    static bool init(const std::string& logDir = "./log", const std::string& level = "info");

    /**
     * @brief 获取日志器实例（内部使用）
     */
    static std::shared_ptr<spdlog::logger> get();

    /**
     * @brief 关闭日志系统
     */
    static void shutdown();

private:
    static std::shared_ptr<spdlog::logger> m_logger;
    static bool m_initialized;
};

// ============ 便利宏定义 ============
// 使用 LOGGER_* 宏可以避免每次都调用 Logger::get()
// 同时支持格式化字符串，类似 fmt 库

#define LOGGER_TRACE(...) Logger::get()->trace(__VA_ARGS__)
#define LOGGER_DEBUG(...) Logger::get()->debug(__VA_ARGS__)
#define LOGGER_INFO(...) Logger::get()->info(__VA_ARGS__)
#define LOGGER_WARN(...) Logger::get()->warn(__VA_ARGS__)
#define LOGGER_ERROR(...) Logger::get()->error(__VA_ARGS__)
#define LOGGER_CRITICAL(...) Logger::get()->critical(__VA_ARGS__)

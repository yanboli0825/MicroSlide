#include "Logger.h"

#include "spdlog/cfg/env.h"

#include <filesystem>
#include <iostream>

// 静态成员初始化
std::shared_ptr<spdlog::logger> Logger::m_logger = nullptr;
bool Logger::m_initialized = false;

bool Logger::init(const std::string& logDir, const std::string& level)
{
    if (m_initialized)
    {
        return true; // 已初始化，直接返回
    }

    try
    {
        // 确保日志目录存在
        std::filesystem::path logPath(logDir);
        if (!std::filesystem::exists(logPath))
        {
            std::filesystem::create_directories(logPath);
        }

        // ============ 创建 Sinks ============
        // 1. 控制台输出 Sink（彩色）
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l]%$ %v");

        // 2. 按日期分类的文件 Sink
        //    文件名格式：logs/microslide.log
        //    自动在每天午夜创建新文件
        auto fileSink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logDir + "/microslide.log", 0, 0);
        fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        // ============ 创建日志器 ============
        spdlog::sinks_init_list sinkList = {consoleSink, fileSink};
        m_logger = std::make_shared<spdlog::logger>("MicroSlide", sinkList);

        // ============ 配置日志级别 ============
        // 可选的级别：trace, debug, info, warn, err, critical, off
        if (level == "trace")
        {
            m_logger->set_level(spdlog::level::trace);
        }
        else if (level == "debug")
        {
            m_logger->set_level(spdlog::level::debug);
        }
        else if (level == "warn")
        {
            m_logger->set_level(spdlog::level::warn);
        }
        else if (level == "err")
        {
            m_logger->set_level(spdlog::level::err);
        }
        else if (level == "critical")
        {
            m_logger->set_level(spdlog::level::critical);
        }
        else
        {
            // 默认为 info
            m_logger->set_level(spdlog::level::info);
        }

        // ============ 注册为全局日志器 ============
        spdlog::register_logger(m_logger);
        spdlog::set_default_logger(m_logger);

        // ============ 启用刷新策略 ============
        // 每条日志后立即刷新到文件（可选，性能会稍差但更安全）
        // spdlog::flush_on(spdlog::level::info);
        // 或者使用异步日志（推荐，性能更好）
        // 这里使用默认的定期刷新（更平衡）

        m_initialized = true;
        LOGGER_INFO("Logger initialized. Log directory: {}", logDir);
        return true;
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        return false;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Unexpected error during logger initialization: " << ex.what() << std::endl;
        return false;
    }
}

std::shared_ptr<spdlog::logger> Logger::get()
{
    if (!m_initialized || !m_logger)
    {
        // 如果还未初始化，进行自动初始化
        // 这样即使忘记调用 init()，程序也不会崩溃
        init();
    }
    return m_logger;
}

void Logger::shutdown()
{
    if (m_initialized && m_logger)
    {
        LOGGER_INFO("Logger shutdown");
        spdlog::drop_all();
        m_logger = nullptr;
        m_initialized = false;
    }
}

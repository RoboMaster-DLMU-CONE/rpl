/**
 * @file Error.hpp
 * @brief RPL库的错误处理定义
 *
 * 此文件定义了RPL库中使用的错误码和错误结构体。
 *
 * @author WindWeaver
 */

#ifndef RPL_ERROR_HPP
#define RPL_ERROR_HPP

#include <string>
#include <utility>

namespace RPL
{
    /**
     * @brief 错误码枚举
     *
     * 定义了RPL库中可能出现的各种错误类型
     */
    enum class ErrorCode
    {
        Again,            ///< 需要再次尝试
        InsufficientData, ///< 数据不足
        NoFrameHeader,    ///< 没有找到帧头
        InvalidFrameHeader, ///< 无效的帧头
        CrcMismatch,      ///< CRC校验不匹配
        BufferOverflow,   ///< 缓冲区溢出
        InternalError,    ///< 内部错误
        InvalidCommand,   ///< 无效命令
    };

    /**
     * @brief 错误结构体
     *
     * 包含错误码和错误消息的结构体
     */
    struct Error
    {
        /**
         * @brief 构造函数
         *
         * 使用错误码和错误消息构造错误对象
         *
         * @param c 错误码
         * @param msg 错误消息
         */
        Error(const ErrorCode c, std::string msg) : message(std::move(msg)), code(c)
        {
        }

        std::string message;  ///< 错误消息
        ErrorCode code;       ///< 错误码
    };
}

#endif //RPL_ERROR_HPP

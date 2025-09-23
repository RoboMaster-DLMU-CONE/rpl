#ifndef RPL_ERROR_HPP
#define RPL_ERROR_HPP

#include <string>
#include <utility>

namespace RPL
{
    enum class ErrorCode
    {
        Again,
        InsufficientData,
        NoFrameHeader,
        InvalidFrameHeader,
        CrcMismatch,
        BufferOverflow,
        InternalError,
        InvalidCommand,
    };

    struct Error
    {
        Error(const ErrorCode c, std::string msg) : message(std::move(msg)), code(c)
        {
        }

        std::string message;
        ErrorCode code;
    };
}

#endif //RPL_ERROR_HPP

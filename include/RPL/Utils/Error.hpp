#ifndef RPL_ERROR_HPP
#define RPL_ERROR_HPP

#include <string>
#include <utility>

namespace rpl
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
    };

    struct error
    {
        error(const ErrorCode c, std::string msg) : message(std::move(msg)), code(c)
        {
        }

        std::string message;
        ErrorCode code;
    };
}

#endif //RPL_ERROR_HPP

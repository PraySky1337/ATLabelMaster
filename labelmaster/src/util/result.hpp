/**
 * @file result.hpp
 * @brief Result type for error handling without exceptions
 *
 * Provides a Result<T> type that can either contain a value T or an error message.
 * This enables explicit error handling without throwing exceptions.
 */

#ifndef LABELMASTER_RESULT_HPP
#define LABELMASTER_RESULT_HPP

#include <QString>
#include <variant>
#include <optional>

namespace labelmaster::util {

/**
 * @brief Result type that can hold either a value T or an error QString
 *
 * This type enables explicit error handling without exceptions.
 * Usage:
 *   Result<int> parseValue(const QString& str) {
 *       bool ok;
 *       int value = str.toInt(&ok);
 *       if (!ok) {
 *           return Result<int>::error("Invalid number");
 *       }
 *       return Result<int>::ok(value);
 *   }
 *
 *   auto result = parseValue("42");
 *   if (result.isSuccess()) {
 *       int value = result.value();
 *   } else {
 *       QString error = result.error();
 *   }
 */
template<typename T>
class Result {
public:
    /**
     * @brief Create a successful result with a value
     */
    static Result ok(const T& value) {
        return Result(value);
    }

    /**
     * @brief Create a successful result with a moved value
     */
    static Result ok(T&& value) {
        return Result(std::move(value));
    }

    /**
     * @brief Create an error result
     */
    static Result error(const QString& error) {
        return Result(error);
    }

    /**
     * @brief Check if result is successful
     */
    bool isSuccess() const {
        return std::holds_alternative<T>(data_);
    }

    /**
     * @brief Check if result is an error
     */
    bool isError() const {
        return std::holds_alternative<QString>(data_);
    }

    /**
     * @brief Get the value (undefined if isError())
     */
    const T& value() const {
        return std::get<T>(data_);
    }

    /**
     * @brief Get the value (undefined if isError())
     */
    T& value() {
        return std::get<T>(data_);
    }

    /**
     * @brief Get the error message (empty if isSuccess())
     */
    const QString& error() const {
        static const QString empty;
        if (isSuccess()) {
            return empty;
        }
        return std::get<QString>(data_);
    }

    /**
     * @brief Get the value or a default if error
     */
    T valueOr(const T& defaultValue) const {
        if (isSuccess()) {
            return value();
        }
        return defaultValue;
    }

    /**
     * @brief Get the value or a default if error
     */
    T valueOr(T&& defaultValue) const {
        if (isSuccess()) {
            return value();
        }
        return std::move(defaultValue);
    }

    /**
     * @brief Map the value to another type (only if successful)
     */
    template<typename F, typename R = std::invoke_result_t<F, const T&>>
    Result<R> map(F&& func) const {
        if (isSuccess()) {
            return Result<R>::ok(func(value()));
        }
        return Result<R>::error(error());
    }

    /**
     * @brief Chain another operation (only if successful)
     */
    template<typename F, typename R = std::invoke_result_t<F, const T&>>
    R andThen(F&& func) const {
        if (isSuccess()) {
            return func(value());
        }
        return R::error(error());
    }

private:
    explicit Result(const T& value) : data_(value) {}
    explicit Result(T&& value) : data_(std::move(value)) {}
    explicit Result(const QString& error) : data_(error) {}

    std::variant<T, QString> data_;
};

/**
 * @brief Specialization for void results
 */
template<>
class Result<void> {
public:
    static Result ok() {
        return Result(true);
    }

    static Result error(const QString& error) {
        return Result(error);
    }

    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }

    const QString& error() const { return error_; }

private:
    explicit Result(bool success) : success_(success), error_() {}
    explicit Result(const QString& error) : success_(false), error_(error) {}

    bool success_;
    QString error_;
};

/**
 * @brief Convenience alias for Result<bool> where false could be valid
 */
using BoolResult = Result<bool>;

} // namespace labelmaster::util

#endif // LABELMASTER_RESULT_HPP

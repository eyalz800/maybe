#pragma once
#include <variant>
#include <type_traits>
#include <string_view>

namespace zpp
{

/**
 * Returns the error category for a given error code enumeration type,
 * using an argument dependent lookup of a user implemented category function.
 */
template <typename ErrorCode>
decltype(auto) category()
{
    return category(ErrorCode{});
}

/**
 * The error category which responsible for translating error codes to 
 * error messages.
 */
class error_category
{
public:
    /**
     * Returns the error category name.
     */
    virtual std::string_view name() const = 0;

    /**
     * Return the error message for a given error code.
     * Return an empty string view for a success code.
     * All other codes must return non empty view.
     */
    virtual std::string_view message(int code) const = 0;

protected:
    /**
     * Destroys the error category.
     */
    ~error_category() = default;
};

/**
 * Creates an error category, 
 */
template <typename ErrorCode, typename Messages>
const auto & make_error_category(std::string_view name, Messages && messages)
{
    // Create a category with the name and messages.
    static class category : public zpp::error_category,
        private std::remove_reference_t<Messages>
    {
    public:
        category(std::string_view name, Messages && messages) :
            std::remove_reference_t<Messages>(std::forward<Messages>(messages)),
            m_name(name)
        {
        }

        std::string_view name() const override
        {
            return m_name;
        }

        std::string_view message(int code) const override
        {
            return this->operator()(ErrorCode{ code });
        }

    private:
        std::string_view m_name;
    } category(name, std::forward<Messages>(messages));

    // Return the category.
    return category;
}

/**
 * Represents an error to be initialized from an error code
 * enumeration.
 * The error code enumeration must have 'int' as underlying type.
 * Defining an error code enum and a category for it goes as follows.
 * Example:
 * ~~~
 * namespace my_namespace
 * {
 * enum class my_error : int
 * {
 *     success = 0,
 *     something_bad = 1,
 *     something_really_bad = 2,
 * };
 * 
 * const zpp::error_category & category(my_error)
 * {
 *     return zpp::make_error_category<my_error>("my_category",
 *         [](auto code) -> std::string_view {
 *             switch (code) {
 *                 case my_error::success:
 *                     return {};
 *                 case my_code:something_bad:
 *                     return "Something bad happened.";
 *                 case my_error::something_really_bad:
 *                     return "Something really bad happened.";
 *                 default:
 *                     return "Unknown error occurred.";
 *             }
 *         }
 *     );
 * }
 * } // my_namespace
 * ~~~
 */
class error
{
public:
    /**
     * Disables default construction.
     */
    error() = delete;

    /**
     * Constructs an error from an error code enumeration, the category
     * is looked by using argument dependent lookup on a function named 'category'
     * that receives the error code enumeration value.
     */
    template <typename ErrorCode>
    error(ErrorCode error_code) :
        m_category(std::addressof(zpp::category<ErrorCode>())),
        m_code(std::underlying_type_t<ErrorCode>(error_code))
    {
    }

    /**
     * Constructs an error from an error code enumeration, the category
     * is given explicitly in this overload.
     */
    template <typename ErrorCode>
    error(ErrorCode error_code, const error_category & category) :
        m_category(std::addressof(category)),
        m_code(std::underlying_type_t<ErrorCode>(error_code))
    {
    }

    /**
     * Returns the error category.
     */
    const error_category & category() const
    {
        return *m_category;
    }

    /**
     * Returns the error code.
     */
    int code() const
    {
        return m_code;
    }

    /**
     * Returns the error message.
     */
    std::string_view message() const
    {
        return m_category->message(m_code);
    }

    /**
     * Returns true if the error indicates success, else false.
     */
    explicit operator bool() const
    {
        return message().empty();
    }

private:
    /**
     * The error category.
     */
    const error_category * m_category{};

    /**
     * The error code.
     */
    int m_code{};
};

/**
 * Represents a value-or-error object.
 * An object of this type may have a value stored inside or an error.
 * The error is represented as zpp::error, and can be queried accessed 
 * using the 'error' member function.
 * Example:
 * ~~~
 * // After defining my_error and the category of it as described 
 * // in the sample above for the error class.
 * 
 * zpp::maybe<int> foo(bool value)
 * {
 *     if (!value) {
 *         // Fail with error code.
 *         return my_error::something_bad;
 *     }
 *
 *     // Success
 *     return 1337;
 * }
 *
 * void bar()
 * {
 *     // Change this value to observe different behavior.
 *     bool is_success = true;
 *
 *     if (auto result = foo(is_success)) {
 *         // Success path.
 *         std::cout << "Success path: value is '" << result.value() << "'\n";
 *     } else {
 *         // Fail path.
 *         std::cout << "Error path: error is '" << result.error().code()
 *             << "', '" << result.error().message() << "'\n";
 *     }
 * } 
 * ~~~
 */
template <typename Type>
class maybe : private std::variant<Type, error>
{
public:
    /**
     * The base class.
     */
    using base = std::variant<Type, error>;

    /**
     * The type of value.
     */
    using type = Type;

    /**
     * Alias to the error type.
     */
    using error_type = error;

    /**
     * Disable default construction.
     */
    maybe() = delete;

    /**
     * Inherit base class constructors.
     */
    using std::variant<Type, error>::variant;

    /**
     * Returns the stored error.
     * The behavior is undefined if the object has a stored value.
     */
    auto error() const
    {
        return *std::get_if<error_type>(this);
    }

    /**
     * Returns the stored value.
     * The behavior is undefined if the object has a stored error.
     */
    decltype(auto) value() &&
    {
        return std::move(*std::get_if<type>(this));
    }

    /**
     * Returns the stored value.
     * The behavior is undefined if the object has a stored error.
     */
    decltype(auto) value() &
    {
        return *std::get_if<type>(this);
    }

    /**
     * Returns false if there is a stored error, else, there 
     * is a stored value and the return value is true.
     */
    explicit operator bool() const
    {
        return (0 == this->index());
    }
};

} // zpp

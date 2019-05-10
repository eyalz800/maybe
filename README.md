maybe
=====

A really simple return value based error checking library. Focused on no memory allocations and no exceptions for extreme environments where these are hard or not trivial to do.

Example
-------
```cpp
#include "zpp/maybe.h"
#include <iostream>

namespace my_namespace
{
// Change this to observe a different behavior.
bool is_success = false;

// Use arbitrary error enumeration whose underlying type is int.
enum class my_error : int
{
    success = 0,
    something_bad = 1,
    something_really_bad = 2,
};

// Define the error category and message translation.
inline const zpp::error_category & category(my_error)
{
    constexpr static auto error_category = zpp::make_error_category(
        "my_category",
        my_error::success,
        [](auto code) -> std::string_view {
            switch (code) {
            case my_error::success:
                return zpp::error::no_error;
            case my_error::something_bad:
                return "Something bad happened.";
            case my_error::something_really_bad:
                return "Something really bad happened.";
            default:
                return "Unknown error occurred.";
            }
        });
    return error_category;
}

// Example of function that wants to return an 'int' but
// can fail.
zpp::maybe<int> foo(bool value)
{
    if (!value) {
        // Fail with error code.
        return my_error::something_bad;
    }

    // Success
    return 1337;
}

// This function just returns an error.
zpp::error bar(bool value)
{
    if (!value) {
        return my_error::something_really_bad;
    }

    return my_error::success;
}

// Call foo and check error.
void baz1()
{
    // Call foo.
    if (auto result = foo(is_success)) {
        // Success path.
        std::cout << "Success path: value is '" << result.value() << "'\n";
    } else {
        // Error path.
        std::cout << "Error path: error is '" << result.error().code()
                  << "', '" << result.error().message() << "'\n";
    }
}

// Call bar and check error.
void baz2()
{
    // Call bar.
    if (auto error = bar(is_success); !error) {
        // Error path.
        std::cout << "Error path: error is '" << error.code()
                  << "', '" << error.message() << "'\n";
    } else {
        // Success path.
        std::cout << "Success path: code is '" << error.code() << "'\n";
    }    
}

extern "C" int main()
{
    baz1();
    baz2();
}

} // namespace my_namespace
```

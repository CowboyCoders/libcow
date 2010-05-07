#ifndef ___libcow_exceptions___
#define ___libcow_exceptions___

#include "cow/libcow_def.hpp"
#include <exception>
#include <string>

namespace libcow 
{
    class LIBCOW_EXPORT exception: public std::exception
    {
    public:
        exception(const std::string& message) 
            : msg(message)
        {
        }
        
        virtual const char* what() const throw()
        {
            return msg.c_str();
        }

    private:
        std::string msg;
    };

    class LIBCOW_EXPORT error_message
    {
    public:
        error_message() : msg("Unknown error") {}
        error_message(const std::string& message) : msg(message) {}
        void set(const std::string& message) { msg = message; }
        std::string const& get() const { return msg; }
    private:
        std::string msg;
    };
}


#endif // ___libcow_exceptions___

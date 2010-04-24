#ifndef ___program_table_hpp___
#define ___program_table_hpp___

namespace libcow {


    class LIBCOW_EXPORT program_table 
    {
    public:
        bool load_from_file(const std::string& file_name);

        bool load_from_http(const std::string& url);

        bool load_from_string(const std::string& s);

        void clear();

        size_t size() const;

        void add(const libcow::program_info& entry);

        const libcow::program_info& at(size_t index) const;

    private:

        std::vector<libcow::program_info> entries_;

    };

}

#endif // ___program_table_hpp___
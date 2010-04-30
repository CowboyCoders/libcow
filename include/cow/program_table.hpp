#ifndef ___program_table_hpp___
#define ___program_table_hpp___

namespace libcow {


    class LIBCOW_EXPORT program_table 
    {
    public:
    	typedef program_info_vector::const_iterator const_iterator;
    	typedef program_info_vector::iterator iterator;


        bool load_from_file(const std::string& file_name);

        bool load_from_http(const std::string& url, size_t timeout);

        bool load_from_string(const std::string& s);

        void clear();

        size_t size() const;

        void add(const libcow::program_info& entry);

        const libcow::program_info& at(size_t index) const;
        libcow::program_info& at(size_t index);

        const libcow::program_info& operator[](size_t index) const;
        libcow::program_info& operator[](size_t index);
            
    	iterator begin();
    	const_iterator begin() const;

    	iterator end();
    	const_iterator end() const;

    private:

        program_info_vector entries_;

    };

}

#endif // ___program_table_hpp___

#ifndef ___program_table_hpp___
#define ___program_table_hpp___

namespace libcow {


   /**
    * This class is responsible for retrieving and parsing program info
    * from an XML file. The XML file can either be stored on disk, in a string,
    * or downloaded from a web server.
    */
    class LIBCOW_EXPORT program_table 
    {
    public:
       /**
        * Typedef for a const_iterator.
        */
    	typedef program_info_vector::const_iterator const_iterator;
       /**
        * Typedef for an iterator.
        */
    	typedef program_info_vector::iterator iterator;

       /**
        * Tries to load an XML configuration from file.
        * @param file_name The file to load the configuration from.
        * @return True if it was possible to load the configuration,
        * otherwise false.
        */
        void load_from_file(const std::string& file_name);

       /**
        * Tries to download and load an XML configuration from a web server.
        * @param url The URL to file to download.
        * @param timeout The number of seconds to wait before we should stop trying
        * to download the file.
        * @return True if it was possible to load the configuration,
        * otherwise false.
        */
        void load_from_http(const std::string& url, size_t timeout);

       /**
        * Tries to load an XML configuration from a string.
        * @param s The string to load the configuration from.
        * @return True if it was possible to load the configuration,
        * otherwise false.
        */
        void load_from_string(const std::string& s);

       /**
        * Removes all entries from the program table. 
        */
        void clear();
       /**
        * Returns the number of entries in the program table.
        * @return The number of programs.
        */
        size_t size() const;
        
       /**
        * Adds a program to the list of programs in the table.
        * @param entry The program to add.
        */
        void add(const libcow::program_info& entry);

		/**
		* Return info for program with id program_id.
		* @param program_id Program id to fetch
		* @return A const iterator for the specified program_id.
		*/
		libcow::program_info_vector::const_iterator find(int program_id) const;

       /**
        * Returns the program at the specified index in the program table.
        * @param index The index for the program to retrieve.
        * @return The libcow::program_info for the specified index.
        */
        const libcow::program_info& at(size_t index) const;
       
       /**
        * Returns the program at the specified index in the program table.
        * @param index The index for the program to retrieve.
        * @return The libcow::program_info for the specified index.
        */
        libcow::program_info& at(size_t index);

       /**
        * Returns the program at the specified index in the program table.
        * @param index The index for the program to retrieve.
        * @return The libcow::program_info for the specified index.
        */
        const libcow::program_info& operator[](size_t index) const;
       /**
        * Returns the program at the specified index in the program table.
        * @param index The index for the program to retrieve.
        * @return The libcow::program_info for the specified index.
        */
        libcow::program_info& operator[](size_t index);
       
       /**
        * Returns the iterator for the program table, starting at the beginning.
        * @return An iterator.
        */
        iterator begin();
       /**
        * Returns the iterator for the program table, starting at the beginning.
        * @return An iterator.
        */
    	const_iterator begin() const;

       /**
        * Returns the iterator end iterator for the program table.
        * @return An iterator.
        */
    	iterator end();
       /**
        * Returns the iterator end iterator for the program table.
        * @return An iterator.
        */
    	const_iterator end() const;

    private:

        program_info_vector entries_;

    };

}

#endif // ___program_table_hpp___

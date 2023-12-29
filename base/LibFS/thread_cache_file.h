#ifndef __OZZY_THREAD_CACHE_FILE__
#define __OZZY_THREAD_CACHE_FILE__

#include "protocol.h"
#include <fstream>
#include <vector>

namespace Ozzy::LibFS
{
    class ThreadCacheFile
    {
    public:
        ThreadCacheFile();

        virtual ~ThreadCacheFile();

        void write_frame(const Proto::Frame &frame);

        void sort_file();

        bool initialized_sucessfully() const
        {
            return m_init_success;
        }

    private:
        static std::string generate_filename(std::uint32_t count, const std::string& postfix);

        static bool sort_and_write_chunk(std::ifstream &input_file, std::string &temp_filename_out);

        static bool merge_and_delete_chunk_caches(const std::vector<std::string> &chunk_files);

    private:
        std::string   m_cache_file_name;
        std::ofstream m_cache_file;
        bool          m_init_success;
    };
}

#endif // __OZZY_THREAD_CACHE_FILE__

#include "thread_cache_file.h"
#include "LibLog/logging.h"

#include <string>
#include <random>
#include <algorithm>
#include <filesystem>
#include <iostream>

// Size that server will reserve(or not :) look at the defines below)
// for the processing of the each chunk
//
// It's usefull when compiling on system with low memory specifications.
// But should be mutable, because can be re-compiled on the systems with higher
// memory specifications
//
// Overall chunk processing performance is strictly depends on this value.
// E.g: if this is low, the chunk processing is 100% a performance bottleneck,
// each thread chunk will take forever to process(1,000,000 doubles hasn't finished
// after ~40 seconds)
#ifndef OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES
#   define OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES 1000000
#endif

// The idea is that we don't reserve huge memory arenas if we
// specified large size for the chunk container.
//
// However if the user is really wants and thinks that
// each chunk is indeed will be provided size, we should
// support that also
//
// Another question is reallocating this monstrous memory arenas.. but
// if we are not sure(and we are, because of UDP), i thought this approach
// will be preferable
#ifndef OZZY_USE_LARGE_CHUNK_MEMORY_ARENAS
#   define OZZY_LESS_CHUNK_RESERVE_VALUE 250000000
#else
#   define OZZY_LESS_CHUNK_RESERVE_VALUE OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES
#endif
#define OZZY_LARGE_MEMORY_ARENA_SIZE 500000000

static constexpr const char* LOGGING_NAME     = "[Ozzy::ThreadCacheFileWriter] ";
static const std::string THREAD_CACHE_CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static std::uint32_t THREAD_CACHE_MAGIC   = 0x595A5A4F; // OZZY :)
static std::uint32_t THREAD_CACHE_START_H = 0xDEADBEEF; // Current cache start marker
static std::uint32_t THREAD_CACHE_END_H   = 0xC0FFEE;   // Current cache end marker

static thread_local std::random_device random_device;
static thread_local std::mt19937 random_generator(random_device());
static thread_local std::uniform_int_distribution<std::size_t> distribution(0, THREAD_CACHE_CHARSET.size() - 1);
static std::mutex result_file_locked;

namespace Ozzy::LibFS
{
    std::string ThreadCacheFile::generate_filename(const std::uint32_t count, const std::string &postfix)
    {
        std::stringstream unique_filename;
        bool unique = false;

        while (!unique)
        {
            for (std::size_t i = 0u; i < count; ++i)
            {
                unique_filename << THREAD_CACHE_CHARSET[distribution(random_generator)];
            }
            unique_filename << postfix;

            unique = !std::filesystem::exists(unique_filename.str());
        }

        return unique_filename.str();
    }

    ThreadCacheFile::ThreadCacheFile()
    {
        // 128 unique symbols from the case-sensitive symbolic+numeric alphabet
        m_cache_file_name = generate_filename(128, "_thread_cache.bin");
        m_cache_file = std::ofstream(m_cache_file_name, std::ios::binary | std::ios::app);

        if (!m_cache_file.is_open())
        {
            LibLog::log_print(LOGGING_NAME, "Unable to create the file");
            m_init_success = false;
        }

        m_init_success = true;
    }

    void ThreadCacheFile::write_frame(const Proto::Frame &frame)
    {
        m_cache_file.write(reinterpret_cast<const char*>(frame.payload), frame.length * sizeof(double));
    }

    bool ThreadCacheFile::merge_and_delete_chunk_caches(const std::vector<std::string> &chunk_files)
    {
        LibLog::log_print(LOGGING_NAME, "Start merging thread cache chunks");

        std::vector<std::ifstream> chunk_readers;
        chunk_readers.reserve(chunk_files.size());

        // We create so-called `chunk readers` for each chunk file, to later
        // find minimal element among all chunks and write to the main output
        // file
        for (const auto &chunk_filename: chunk_files)
        {
            chunk_readers.emplace_back(chunk_filename, std::ios::binary);

            if (!chunk_readers.back().is_open())
            {
                LibLog::log_print(LOGGING_NAME,
                                  "Unable to create the chunk reader for chunk cache file " + chunk_filename);
                chunk_readers.pop_back();
            }
        }

        std::ofstream output_file("result.bin", std::ios::binary | std::ios::app);
        if (!output_file)
        {
            LibLog::log_print(LOGGING_NAME, "Unable to open output file output.bin");
            return false;
        }

        // Write the header of the file, if it's the first time its processed
        output_file.seekp(0, std::ios::end);
        std::streampos file_size = output_file.tellp();

        if (file_size == 0)
        {
            output_file.write(reinterpret_cast<char*>(&THREAD_CACHE_MAGIC), sizeof(std::uint32_t));
        }
        output_file.write(reinterpret_cast<char*>(&THREAD_CACHE_START_H), sizeof(std::uint32_t));

        std::vector<double> values;
#if OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES < OZZY_LARGE_MEMORY_ARENA_SIZE
        values.reserve(OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES);
#else
        values.reserve(OZZY_LESS_CHUNK_RESERVE_VALUE);
#endif

        while (!chunk_readers.empty())
        {
            std::ptrdiff_t minimal_reader_index = -1;
            double minimal_value = std::numeric_limits<double>::max();

            // Works only on sorted chunks!
            //
            // Find minimal value among all chunk files(readers) at the same file pointer position
            // and write it to the result output file(thread-safely)
            //
            // Like, image we have 2 chunks('^' means file pointer):
            //
            //  -- Iteration 0 --
            //   [0, 1, 2]
            //    ^
            //   [1, 1, 2]
            //    ^
            //
            //  -- Iteration 1 --
            //   [0, 1, 2]
            //       ^       -> Wrote `1` to result file, iterate 1'st chunk from index 1, second from index 0
            //   [1, 1, 2]
            //    ^
            //
            //  -- Iteration 3 --
            //   [0, 1, 2]
            //       ^       -> Wrote `1` to result file, iterate 2'nd chunk from index 1, second from index 1
            //   [1, 1, 2]
            //       ^
            for (std::size_t i = 0; i < chunk_readers.size(); ++i)
            {
                const std::streampos previous_file_pointer = chunk_readers[i].tellg();
                double value;

                if (!chunk_readers[i].eof())
                {
                    if (chunk_readers[i].read(reinterpret_cast<char*>(&value), sizeof(double)))
                    {
                        if (value <= minimal_value)
                        {
                            // If we found other minimals before, the previous reader's file pointers are moved,
                            // so.. handle it
                            if (minimal_reader_index != -1)
                            {
                                chunk_readers[minimal_reader_index].seekg(
                                    -static_cast<std::streamoff>(sizeof(double)), std::ios::cur);
                            }

                            minimal_value = value;
                            minimal_reader_index = i;
                        }
                        else
                        {
                            // As soon as we read the file, file pointer moved +sizeof(double) bits,
                            // we need to move it back, to later iterate it again.
                            chunk_readers[i].seekg(previous_file_pointer);
                        }
                    }
                }
                else
                {
                    // All data in current chunk has been processed, need to substract i by one
                    // to avoid out-of-bounds memory access
                    chunk_readers[i].close();
                    chunk_readers.erase(chunk_readers.begin() + i);
                    --i;
                }
            }

            if (minimal_reader_index != -1)
            {
                output_file.write(reinterpret_cast<char*>(&minimal_value), sizeof(double));
            }
        }

        // Finish up
        output_file.write(reinterpret_cast<char*>(&THREAD_CACHE_END_H), sizeof(std::uint32_t));
        output_file.close();

        for (auto &chunk_filename: chunk_files)
        {
            std::remove(chunk_filename.c_str());
        }

        LibLog::log_print(LOGGING_NAME, "Finish merging thread cache chunks");
        return true;
    }

    bool ThreadCacheFile::sort_and_write_chunk(std::ifstream &input_file, std::string &temp_filename_out)
    {
        temp_filename_out = generate_filename(128, "_thread_chunk.bin");
        std::ofstream temp_file(temp_filename_out, std::ios::binary);

        if (!input_file.is_open() || !temp_file.is_open())
        {
            LibLog::log_print(LOGGING_NAME, "Unable to open files, required for the cache file sorting");
            return false;
        }

        std::vector<double> chunk;
#if OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES < OZZY_LARGE_MEMORY_ARENA_SIZE
        chunk.reserve(OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES);
#else
        chunk.reserve(OZZY_LESS_CHUNK_RESERVE_VALUE);
#endif

        {
            // Read chunk file until it's eof'ed. The amount of bits that we read
            // totally depends on system specs
            double value;

            for (int i = 0; !input_file.eof() && i < OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES; ++i)
            {
                if (input_file.read(reinterpret_cast<char*>(&value), sizeof(value)))
                {
                    chunk.push_back(value);
                }
            }
        }

        if (!chunk.empty())
        {
            std::sort(chunk.begin(), chunk.end());

            for (auto &element: chunk)
            {
                temp_file.write(reinterpret_cast<const char*>(&element), sizeof(double));
            }
        }

        temp_file.close();
        return input_file.eof();
    }

    void ThreadCacheFile::sort_file()
    {
        m_cache_file.seekp(0);

        const std::string output_name = "output.bin";
        std::vector<std::string> chunk_files;
        std::string temp_filename;

        std::ifstream input_file(m_cache_file_name, std::ios::binary);

        for (;;)
        {
            bool chunk_processing_should_end = sort_and_write_chunk(input_file, temp_filename);
            chunk_files.push_back(temp_filename);

            if (chunk_processing_should_end)
            {
                break;
            }
        }

        std::lock_guard<std::mutex> lock(result_file_locked);
        merge_and_delete_chunk_caches(chunk_files);
    }

    ThreadCacheFile::~ThreadCacheFile()
    {
        if (m_cache_file.is_open())
        {
            m_cache_file.close();
        }

        std::remove(m_cache_file_name.c_str());
    }
}

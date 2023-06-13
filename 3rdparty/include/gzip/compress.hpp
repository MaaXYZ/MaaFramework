#include <gzip/config.hpp>

// zlib
#include <zlib.h>

// std
#include <limits>
#include <stdexcept>
#include <string>

namespace gzip {

class Compressor
{
    std::size_t max_;
    int level_;

  public:
    Compressor(int level = Z_DEFAULT_COMPRESSION,
               std::size_t max_bytes = 2000000000) // by default refuse operation if uncompressed data is > 2GB
        : max_(max_bytes),
          level_(level)
    {
    }

    template <typename InputType>
    void compress(InputType& output,
                  const char* data,
                  std::size_t size) const
    {

#ifdef DEBUG
        // Verify if size input will fit into unsigned int, type used for zlib's avail_in
        if (size > std::numeric_limits<unsigned int>::max())
        {
            throw std::runtime_error("size arg is too large to fit into unsigned int type");
        }
#endif
        if (size > max_)
        {
            throw std::runtime_error("size may use more memory than intended when decompressing");
        }

        z_stream deflate_s;
        deflate_s.zalloc = Z_NULL;
        deflate_s.zfree = Z_NULL;
        deflate_s.opaque = Z_NULL;
        deflate_s.avail_in = 0;
        deflate_s.next_in = Z_NULL;

        // The windowBits parameter is the base two logarithm of the window size (the size of the history buffer).
        // It should be in the range 8..15 for this version of the library.
        // Larger values of this parameter result in better compression at the expense of memory usage.
        // This range of values also changes the decoding type:
        //  -8 to -15 for raw deflate
        //  8 to 15 for zlib
        // (8 to 15) + 16 for gzip
        // (8 to 15) + 32 to automatically detect gzip/zlib header (decompression/inflate only)
        constexpr int window_bits = 15 + 16; // gzip with windowbits of 15

        constexpr int mem_level = 8;
        // The memory requirements for deflate are (in bytes):
        // (1 << (window_bits+2)) +  (1 << (mem_level+9))
        // with a default value of 8 for mem_level and our window_bits of 15
        // this is 128Kb

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (deflateInit2(&deflate_s, level_, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            throw std::runtime_error("deflate init failed");
        }
#pragma GCC diagnostic pop

        deflate_s.next_in = reinterpret_cast<z_const Bytef*>(data);
        deflate_s.avail_in = static_cast<unsigned int>(size);

        std::size_t size_compressed = 0;
        do
        {
            size_t increase = size / 2 + 1024;
            if (output.size() < (size_compressed + increase))
            {
                output.resize(size_compressed + increase);
            }
            // There is no way we see that "increase" would not fit in an unsigned int,
            // hence we use static cast here to avoid -Wshorten-64-to-32 error
            deflate_s.avail_out = static_cast<unsigned int>(increase);
            deflate_s.next_out = reinterpret_cast<Bytef*>((&output[0] + size_compressed));
            // From http://www.zlib.net/zlib_how.html
            // "deflate() has a return value that can indicate errors, yet we do not check it here.
            // Why not? Well, it turns out that deflate() can do no wrong here."
            // Basically only possible error is from deflateInit not working properly
            deflate(&deflate_s, Z_FINISH);
            size_compressed += (increase - deflate_s.avail_out);
        } while (deflate_s.avail_out == 0);

        deflateEnd(&deflate_s);
        output.resize(size_compressed);
    }
};

inline std::string compress(const char* data,
                            std::size_t size,
                            int level = Z_DEFAULT_COMPRESSION)
{
    Compressor comp(level);
    std::string output;
    comp.compress(output, data, size);
    return output;
}

} // namespace gzip

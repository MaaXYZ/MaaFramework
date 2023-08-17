#ifndef _WIN32

inline os_library_handle library_load(const os_string& path)
{
    return dlopen(path.c_str(), RTLD_LAZY);
}

inline os_library_function library_get_address(os_library_handle handle, const std::string& name)
{
    return reinterpret_cast<os_library_function>(dlsym(handle, name.c_str()));
}

inline void library_free(os_library_handle handle)
{
    dlclose(handle);
}

inline static size_t get_page_size()
{
    return (size_t)sysconf(_SC_PAGESIZE);
}

inline static const size_t page_size = get_page_size();

inline void* aligned_alloc(size_t len, size_t align)
{
    return ::aligned_alloc(len, align);
}

inline void aligned_free(void* ptr)
{
    ::free(ptr);
}

inline os_string_view to_osstring(std::string_view utf8_str)
{
    return utf8_str;
}

inline std::string_view from_osstring(const os_string_view& os_str)
{
    return os_str;
}

inline std::string path_to_utf8_string(const std::filesystem::path& path)
{
    return path.native();
}

inline std::string path_to_ansi_string(const std::filesystem::path& path)
{
    return path.native();
}

inline std::string path_to_crt_string(const std::filesystem::path& path)
{
    return path.native();
}

#endif

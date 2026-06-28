/*
 * SharedMemory.hpp - Cross-platform shared memory header library
 *
 * Class SharedMemory:
 *  Constructor SharedMemory(name, size, create)
 *      - name: Shared memory name (unique identifier)
 *      - size: Shared memory size (bytes)
 *      - create: true to create, false to open existing shared memory
 *  ~SharedMemory()
 *      - Automatically unmap and close handles
 *  void* data() const
 *      - Returns pointer to the start of shared memory
 *  void write(T* arr, size_t count)
 *      - Writes array to shared memory
 *  size_t readSize()
 *      - Reads array size from shared memory
 *  T* read(size_t count)
 *      - Reads array pointer from shared memory
 *  size_t size() const
 *      - Returns shared memory size
 *  void resetOffset()
 *      - Resets read/write offset to start position
 *  void close()
 *      - Explicitly close mapping and handles (optional)
 *  void unlink()
 *      - Delete shared memory object (POSIX only)
 *
 * Thread safety:
 *  - Shared memory itself is not locked; the caller must ensure read/write synchronization
 *
 * Dependencies:
 *  - C++11
 *  - Windows: Uses WinAPI CreateFileMapping etc.
 *  - Linux/macOS: Uses shm_open + mmap
 */


#pragma once
#include <string>
#include <stdexcept>
#include <cstddef>
#include <random>

#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <cstring>
#endif

class SharedMemory {
public:
    SharedMemory(const std::string& name, size_t size, bool create)
        : shm_name(name), shm_size(size) {

#ifdef _WIN32
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            static_cast<DWORD>(size >> 32), static_cast<DWORD>(size & 0xFFFFFFFF),
            name.c_str());

        if (!hMapFile) {
            throw std::runtime_error("CreateFileMappingA failed: " + std::to_string(GetLastError()));
        }

        shm_ptr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
        if (!shm_ptr) {
            CloseHandle(hMapFile);
            throw std::runtime_error("MapViewOfFile failed");
        }

#else
        int flags = create ? (O_CREAT | O_RDWR) : O_RDWR;
        shm_fd = shm_open(name.c_str(), flags, 0666);
        if (shm_fd < 0)
            throw std::runtime_error("shm_open failed");

        if (create && ftruncate(shm_fd, size) != 0)
            throw std::runtime_error("ftruncate failed");

        shm_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shm_ptr == MAP_FAILED)
            throw std::runtime_error("mmap failed");
#endif
    }

    ~SharedMemory() {
        close();
    }

    void* data() { return shm_ptr; }
    size_t size() const { return shm_size; }
    void write(void* ptr, size_t count) {
        memcpy(static_cast<char*>(shm_ptr) + offset, ptr, count);
        offset += count;
    }
    void writeSize(size_t size) {
        memcpy(static_cast<char*>(shm_ptr) + offset, &size, sizeof(size_t));
        offset += sizeof(size_t);
    }
    void resetOffset() { offset = 0; }
    void read(void* ptr, size_t count) {
        memcpy(ptr, static_cast<char*>(shm_ptr) + offset, count);
        offset += count;
    }
    void close() {
#ifdef _WIN32
        if (shm_ptr) {
            UnmapViewOfFile(shm_ptr);
            shm_ptr = nullptr;
        }
        if (hMapFile) {
            CloseHandle(hMapFile);
            hMapFile = nullptr;
        }
#else
        if (shm_ptr) {
            munmap(shm_ptr, shm_size);
            shm_ptr = nullptr;
        }
        if (shm_fd >= 0) {
            ::close(shm_fd);
            shm_fd = -1;
        }
#endif
    }

    void unlink() {
#ifdef _WIN32
        // Windows: unlink not needed
#else
        shm_unlink(shm_name.c_str());
#endif
    }

private:
    std::string shm_name;
    size_t shm_size;
    void* shm_ptr = nullptr;
    size_t offset = 0;

#ifdef _WIN32
    void* hMapFile = nullptr;
#else
    int shm_fd = -1;
#endif
};


inline std::string generate_unique_shm_name() {
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    return "/shm_" + std::to_string(dist(rng));
}

template<typename T>
static size_t getBytes(T* arr, size_t count) {
    return count * sizeof(T);
}
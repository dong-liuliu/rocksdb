#ifndef _NVM_FILES_
#define _NVM_FILES_

#include "nvm.h"

namespace rocksdb
{

class NVMFileLock : public FileLock
{
    public:
	int fd_;
	std::string filename;
};

class NVMSequentialFile: public SequentialFile
{
    private:
	std::string filename_;

	FILE* file_;

	int fd_;

	bool use_os_buffer_;

    public:
	NVMSequentialFile(const std::string& fname, FILE* f, const EnvOptions& options);
	virtual ~NVMSequentialFile();

	virtual Status Read(size_t n, Slice* result, char* scratch) override;
	virtual Status Skip(uint64_t n) override;
	virtual Status InvalidateCache(size_t offset, size_t length) override;
};

class NVMRandomAccessFile: public RandomAccessFile
{
    private:
	std::string filename_;

	int fd_;

	bool use_os_buffer_;

    public:
	NVMRandomAccessFile(const std::string& fname, int fd, const EnvOptions& options);
	virtual ~NVMRandomAccessFile();

	virtual Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override;

#ifdef OS_LINUX
	virtual size_t GetUniqueId(char* id, size_t max_size) const override;
#endif

	virtual void Hint(AccessPattern pattern) override;

	virtual Status InvalidateCache(size_t offset, size_t length) override;
};

class NVMMmapReadableFile : public RandomAccessFile
{
    private:
	int fd_;

	std::string filename_;

	void* mmapped_region_;

	size_t length_;

    public:
	// base[0,length-1] contains the mmapped contents of the file.
	NVMMmapReadableFile(const int fd, const std::string& fname, void* base, size_t length, const EnvOptions& options);
	virtual ~NVMMmapReadableFile();

	virtual Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override;
	virtual Status InvalidateCache(size_t offset, size_t length) override;
};

class NVMMmapFile : public WritableFile
{
    private:
	std::string filename_;

	int fd_;

	size_t page_size_;
	size_t map_size_;       // How much extra memory to map at a time

	char* base_;            // The mapped region
	char* limit_;           // Limit of the mapped region
	char* dst_;             // Where to write next  (in range [base_,limit_])
	char* last_sync_;       // Where have we synced up to

	uint64_t file_offset_;  // Offset of base_ in file

	// Have we done an munmap of unsynced data?
	bool pending_sync_;

#ifdef ROCKSDB_FALLOCATE_PRESENT

	bool fallocate_with_keep_size_;

#endif

	// Roundup x to a multiple of y
	static size_t Roundup(size_t x, size_t y);

	size_t TruncateToPageBoundary(size_t s);

	Status UnmapCurrentRegion();
	Status MapNewRegion();

    public:
	NVMMmapFile(const std::string& fname, int fd, size_t page_size, const EnvOptions& options);
	~NVMMmapFile();

	virtual Status Append(const Slice& data) override;
	virtual Status Close() override;
	virtual Status Flush() override;
	virtual Status Sync() override;

	/**
	 * Flush data as well as metadata to stable storage.
	 */
	virtual Status Fsync() override;

	/**
	 * Get the size of valid data in the file. This will not match the
	 * size that is returned from the filesystem because we use mmap
	 * to extend file by map_size every time.
	 */
	virtual uint64_t GetFileSize() override;

	virtual Status InvalidateCache(size_t offset, size_t length) override;

#ifdef ROCKSDB_FALLOCATE_PRESENT

	virtual Status Allocate(off_t offset, off_t len) override;
#endif
};

// Use nvm write to write data to a file.
class NVMWritableFile : public WritableFile
{
    private:
	const std::string filename_;

	int fd_;

	size_t cursize_;      // current size of cached data in buf_
	size_t capacity_;     // max size of buf_

	unique_ptr<char[]> buf_;           // a buffer to cache writes

	uint64_t filesize_;

	bool pending_sync_;
	bool pending_fsync_;

	uint64_t last_sync_size_;
	uint64_t bytes_per_sync_;

#ifdef ROCKSDB_FALLOCATE_PRESENT

	bool fallocate_with_keep_size_;

#endif
	RateLimiter* rate_limiter_;

	inline size_t RequestToken(size_t bytes);

    public:
	NVMWritableFile(const std::string& fname, int fd, size_t capacity, const EnvOptions& options);
	~NVMWritableFile();

	virtual Status Append(const Slice& data) override;
	virtual Status Close() override;

	// write out the cached data to the OS cache
	virtual Status Flush() override;
	virtual Status Sync() override;
	virtual Status Fsync() override;

	virtual uint64_t GetFileSize() override;

	virtual Status InvalidateCache(size_t offset, size_t length) override;

#ifdef ROCKSDB_FALLOCATE_PRESENT

	virtual Status Allocate(off_t offset, off_t len) override;

	virtual Status RangeSync(off_t offset, off_t nbytes) override;

	virtual size_t GetUniqueId(char* id, size_t max_size) const override;

#endif
};

class NVMRandomRWFile : public RandomRWFile
{
    private:
	const std::string filename_;

	int fd_;

	bool pending_sync_;
	bool pending_fsync_;

#ifdef ROCKSDB_FALLOCATE_PRESENT

	bool fallocate_with_keep_size_;

#endif

    public:
	NVMRandomRWFile(const std::string& fname, int fd, const EnvOptions& options);
	~NVMRandomRWFile();

	virtual Status Write(uint64_t offset, const Slice& data) override;
	virtual Status Read(uint64_t offset, size_t n, Slice* result, char* scratch) const override;
	virtual Status Close() override;

	virtual Status Sync() override;
	virtual Status Fsync() override;

#ifdef ROCKSDB_FALLOCATE_PRESENT

	virtual Status Allocate(off_t offset, off_t len) override;
#endif
};

class NVMDirectory : public Directory
{
    private:
	int fd_;

    public:
	explicit NVMDirectory(int fd);
	~NVMDirectory();

	virtual Status Fsync() override;
};

}

#endif
#ifndef _NVM_TYPEDEFS_H_
#define _NVM_TYPEDEFS_H_

typedef unsigned long long sector_t;

struct nba_channel
{
    unsigned long int lun_idx;
    unsigned short int chnl_idx;

    unsigned int gran_write;
    unsigned int gran_read;
    unsigned int gran_erase;
};

struct nba_block
{
    unsigned long lun;

    sector_t phys_addr;

    unsigned long id;

    void *internals;
};

struct nvm_channel
{
    unsigned int gran_write;
    unsigned int gran_read;
    unsigned int gran_erase;
};

struct nvm_page
{
    unsigned long lun_id;
    unsigned long block_id;
    unsigned long id;

    bool allocated;
    bool erased;

    unsigned int sizes_no;
    unsigned int *sizes;
};

struct nvm_block
{
    bool has_stale_pages;

    struct nba_block *block;
    struct nvm_page *pages;
};

struct nvm_lun
{
    unsigned long nr_blocks;

    struct nvm_block *blocks;

    unsigned long nr_pages_per_blk;

    unsigned long nchannels;

    struct nvm_channel *channels;
};

class list_node
{
    private:
	void *data;

	list_node *prev;
	list_node *next;

    public:
	list_node(void *_data);
	~list_node();

	list_node *GetNext();
	list_node *GetPrev();

	void *GetData();
	void *SetData(void *_data);
	void *SetNext(list_node *_next);
	void *SetPrev(list_node *_prev);
};

struct next_page_to_allocate
{
    unsigned long lun_id;
    unsigned long block_id;
    unsigned long page_id;
};

class nvm
{
    public:
	unsigned long nr_luns;
	unsigned long max_alloc_try_count;

	struct nvm_lun *luns;

	int fd;

	std::string location;

	nvm();
	~nvm();

	void ReclaimPage(struct nvm_page *page);
	struct nvm_page *RequestPage();

	const char *GetLocation();

    private:
	next_page_to_allocate next_page;

	pthread_mutex_t allocate_page_mtx;
	pthread_mutexattr_t allocate_page_mtx_attr;

	int open_nvm_device(const char *file);
	int ioctl_initialize();

};

#endif
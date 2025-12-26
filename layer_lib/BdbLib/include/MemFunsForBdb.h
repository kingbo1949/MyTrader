#pragma once

namespace Bdb
{
    class CMemFunsForBdb : public IceUtil::Shared
    {
    public:
        CMemFunsForBdb(void)
        {

        }
        virtual ~CMemFunsForBdb(void)
        {

        }

        static void*			Malloc_BDB(size_t bytes)
        {
            return malloc(bytes);
        }
        static void*			Realloc_BDB(void* mem_address, size_t newsize)
        {
            return realloc(mem_address, newsize);
        }
        static void				Free_BDB(void* ptr)
        {
            free(ptr);
        }

    };

}

#include "RocksWriteBatch.h"
#include <stdexcept>

CRocksWriteBatch::CRocksWriteBatch(CRocksEnv& env)
    : m_env(env)
{
}

void CRocksWriteBatch::Put(rocksdb::ColumnFamilyHandle* cfHandle,
                           const CRocksKey& key, const rocksdb::Slice& value)
{
    m_batch.Put(cfHandle, key.ToSlice(), value);
}

void CRocksWriteBatch::Delete(rocksdb::ColumnFamilyHandle* cfHandle, const CRocksKey& key)
{
    m_batch.Delete(cfHandle, key.ToSlice());
}

void CRocksWriteBatch::Commit()
{
    const auto& wo = m_env.GetWriteOptions();
    auto s = m_env.GetDB()->Write(wo, &m_batch);
    if (!s.ok()) {
        throw std::runtime_error("WriteBatch commit failed: " + s.ToString());
    }
    m_batch.Clear();
}

void CRocksWriteBatch::Clear()
{
    m_batch.Clear();
}

int CRocksWriteBatch::Count() const
{
    return m_batch.Count();
}

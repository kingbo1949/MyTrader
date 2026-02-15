#include "RocksDb.h"
#include <stdexcept>

CRocksDb::CRocksDb(CRocksEnv& env, const std::string& cfName)
    : m_env(env)
    , m_cfName(cfName)
    , m_cfHandle(env.GetOrCreateColumnFamily(cfName))
{
}

void CRocksDb::Put(const CRocksKey& key, const rocksdb::Slice& value)
{
    rocksdb::WriteOptions wo;
    auto s = m_env.GetDB()->Put(wo, m_cfHandle, key.ToSlice(), value);
    if (!s.ok()) {
        throw std::runtime_error("Put failed [" + m_cfName + "]: " + s.ToString());
    }
}

bool CRocksDb::Get(const CRocksKey& key, std::string* value)
{
    rocksdb::ReadOptions ro;
    auto s = m_env.GetDB()->Get(ro, m_cfHandle, key.ToSlice(), value);
    if (s.ok()) {
        return true;
    }
    if (s.IsNotFound()) {
        return false;
    }
    throw std::runtime_error("Get failed [" + m_cfName + "]: " + s.ToString());
}

void CRocksDb::Delete(const CRocksKey& key)
{
    rocksdb::WriteOptions wo;
    auto s = m_env.GetDB()->Delete(wo, m_cfHandle, key.ToSlice());
    if (!s.ok()) {
        throw std::runtime_error("Delete failed [" + m_cfName + "]: " + s.ToString());
    }
}

void CRocksDb::ScanBySymbol(const std::string& symbol,
                             const std::function<bool(const CRocksKey&, const rocksdb::Slice&)>& callback)
{
    CRocksKey prefixKey(symbol, 0);
    rocksdb::Slice prefix = prefixKey.SymbolPrefix();

    rocksdb::ReadOptions ro;
    std::unique_ptr<rocksdb::Iterator> it(m_env.GetDB()->NewIterator(ro, m_cfHandle));

    for (it->Seek(prefixKey.ToSlice()); it->Valid(); it->Next()) {
        // 检查前缀是否仍匹配（前 64 字节为合约名）
        if (it->key().size() != sizeof(CRocksKey) ||
            std::memcmp(it->key().data(), prefix.data(), prefix.size()) != 0) {
            break;
        }

        CRocksKey currentKey = CRocksKey::FromSlice(it->key());
        if (!callback(currentKey, it->value())) {
            break;
        }
    }

    if (!it->status().ok()) {
        throw std::runtime_error("ScanBySymbol failed [" + m_cfName + "]: " + it->status().ToString());
    }
}

void CRocksDb::ScanByRange(const std::string& symbol, time_t startTime, time_t endTime,
                            const std::function<bool(const CRocksKey&, const rocksdb::Slice&)>& callback)
{
    CRocksKey startKey(symbol, startTime);
    CRocksKey endKey(symbol, endTime);
    rocksdb::Slice prefix = startKey.SymbolPrefix();

    rocksdb::ReadOptions ro;
    std::unique_ptr<rocksdb::Iterator> it(m_env.GetDB()->NewIterator(ro, m_cfHandle));

    for (it->Seek(startKey.ToSlice()); it->Valid(); it->Next()) {
        if (it->key().size() != sizeof(CRocksKey)) {
            break;
        }

        // 检查合约名前缀
        if (std::memcmp(it->key().data(), prefix.data(), prefix.size()) != 0) {
            break;
        }

        CRocksKey currentKey = CRocksKey::FromSlice(it->key());

        // 超过结束时间则停止
        if (currentKey.GetTimestamp() > endTime) {
            break;
        }

        if (!callback(currentKey, it->value())) {
            break;
        }
    }

    if (!it->status().ok()) {
        throw std::runtime_error("ScanByRange failed [" + m_cfName + "]: " + it->status().ToString());
    }
}

void CRocksDb::DeleteRange(const std::string& symbol, time_t startTime, time_t endTime)
{
    // RocksDB::DeleteRange 区间为 [beginKey, endKey)，左闭右开
    // endTime + 1 使得 endTime 本身也被包含在删除范围内
    CRocksKey beginKey(symbol, startTime);
    CRocksKey endKey(symbol, endTime + 1);

    rocksdb::WriteOptions wo;
    auto s = m_env.GetDB()->DeleteRange(wo, m_cfHandle, beginKey.ToSlice(), endKey.ToSlice());
    if (!s.ok()) {
        throw std::runtime_error("DeleteRange failed [" + m_cfName + "]: " + s.ToString());
    }
}

const std::string& CRocksDb::GetColumnFamilyName() const
{
    return m_cfName;
}

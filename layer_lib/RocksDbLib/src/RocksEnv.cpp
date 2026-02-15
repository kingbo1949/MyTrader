#include "RocksEnv.h"
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/cache.h>
#include <stdexcept>

CRocksEnv::CRocksEnv(const std::string& dbPath)
    : m_dbPath(dbPath)
{
    // --- DB 级选项 ---
    m_options.create_if_missing = true;
    m_options.db_write_buffer_size = 512 * 1024 * 1024;   // 全局 memtable 上限 512MB
    m_options.max_background_jobs = 4;                     // flush + compaction 并发线程

    // --- 列族选项（通过 Options 继承 ColumnFamilyOptions，应用到所有列族）---
    m_options.write_buffer_size = 32 * 1024 * 1024;        // 每个 CF 32MB memtable
    m_options.compression = rocksdb::kSnappyCompression;   // Snappy 压缩，CPU 开销低
    m_options.optimize_filters_for_hits = true;             // 最底层不建 Bloom，减少空间

    // --- 表级选项（Bloom Filter + Block Cache）---
    rocksdb::BlockBasedTableOptions table_options;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
    table_options.block_cache = rocksdb::NewLRUCache(512 * 1024 * 1024);  // 512MB 共享读缓存
    m_options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    // 尝试列出已有列族
    std::vector<std::string> existingCfNames;
    auto listStatus = rocksdb::DB::ListColumnFamilies(m_options, m_dbPath, &existingCfNames);

    if (listStatus.ok() && !existingCfNames.empty())
    {
        // 数据库已存在，用所有已有列族打开，统一使用相同的列族选项
        std::vector<rocksdb::ColumnFamilyDescriptor> cfDescs;
        for (const auto& name : existingCfNames)
        {
            cfDescs.emplace_back(name, m_options);  // Options 隐式转换为 ColumnFamilyOptions
        }

        std::vector<rocksdb::ColumnFamilyHandle*> cfHandles;
        auto s = rocksdb::DB::Open(m_options, m_dbPath, cfDescs, &cfHandles, &m_db);
        if (!s.ok())
        {
            throw std::runtime_error("Failed to open RocksDB: " + s.ToString());
        }

        for (size_t i = 0; i < existingCfNames.size(); ++i)
        {
            m_cfHandles[existingCfNames[i]] = cfHandles[i];
        }
    }
    else
    {
        // 新建数据库，default 列族自动继承 Options 中的列族选项
        auto s = rocksdb::DB::Open(m_options, m_dbPath, &m_db);
        if (!s.ok())
        {
            throw std::runtime_error("Failed to create RocksDB: " + s.ToString());
        }
        m_cfHandles[rocksdb::kDefaultColumnFamilyName] = m_db->DefaultColumnFamily();
    }
}

rocksdb::ColumnFamilyHandle* CRocksEnv::GetOrCreateColumnFamily(const std::string& cfName)
{
    // 已有则直接返回
    auto it = m_cfHandles.find(cfName);
    if (it != m_cfHandles.end()) {
        return it->second;
    }

    // 创建新列族，使用与已有列族相同的选项
    rocksdb::ColumnFamilyHandle* handle = nullptr;
    auto s = m_db->CreateColumnFamily(m_options, cfName, &handle);
    if (!s.ok()) {
        throw std::runtime_error("Failed to create column family '" + cfName + "': " + s.ToString());
    }

    m_cfHandles[cfName] = handle;
    return handle;
}

rocksdb::ColumnFamilyHandle* CRocksEnv::GetColumnFamilyHandle(const std::string& cfName) const
{
    auto it = m_cfHandles.find(cfName);
    return (it != m_cfHandles.end()) ? it->second : nullptr;
}

void CRocksEnv::DropColumnFamily(const std::string& cfName)
{
    if (cfName == rocksdb::kDefaultColumnFamilyName) {
        throw std::runtime_error("Cannot drop the default column family");
    }

    auto it = m_cfHandles.find(cfName);
    if (it == m_cfHandles.end()) {
        throw std::runtime_error("Column family '" + cfName + "' not found");
    }

    auto s = m_db->DropColumnFamily(it->second);
    if (!s.ok()) {
        throw std::runtime_error("Failed to drop column family '" + cfName + "': " + s.ToString());
    }

    m_db->DestroyColumnFamilyHandle(it->second);
    m_cfHandles.erase(it);
}

std::vector<std::string> CRocksEnv::GetAllColumnFamilyNames() const
{
    std::vector<std::string> names;
    names.reserve(m_cfHandles.size());
    for (const auto& [name, handle] : m_cfHandles) {
        names.push_back(name);
    }
    return names;
}

void CRocksEnv::Flush()
{
    rocksdb::FlushOptions flushOptions;
    for (const auto& [name, handle] : m_cfHandles) {
        auto s = m_db->Flush(flushOptions, handle);
        if (!s.ok()) {
            throw std::runtime_error("Failed to flush column family '" + name + "': " + s.ToString());
        }
    }
}

void CRocksEnv::Flush(const std::string& cfName)
{
    auto it = m_cfHandles.find(cfName);
    if (it == m_cfHandles.end()) {
        throw std::runtime_error("Column family '" + cfName + "' not found");
    }

    rocksdb::FlushOptions flushOptions;
    auto s = m_db->Flush(flushOptions, it->second);
    if (!s.ok()) {
        throw std::runtime_error("Failed to flush column family '" + cfName + "': " + s.ToString());
    }
}

rocksdb::DB* CRocksEnv::GetDB() const
{
    return m_db;
}

const std::string& CRocksEnv::GetDbPath() const
{
    return m_dbPath;
}

CRocksEnv::~CRocksEnv()
{
    // 关闭所有列族句柄（default 列族由 DB 析构自动处理，但显式关闭更安全）
    for (auto& [name, handle] : m_cfHandles) {
        if (handle) {
            m_db->DestroyColumnFamilyHandle(handle);
        }
    }
    m_cfHandles.clear();

    delete m_db;
    m_db = nullptr;
}

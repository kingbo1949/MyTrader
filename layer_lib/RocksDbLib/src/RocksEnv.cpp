#include "RocksEnv.h"
#include <rocksdb/db.h>
#include <stdexcept>

CRocksEnv::CRocksEnv(const std::string& dbPath)
    : m_dbPath(dbPath)
{
    m_options.create_if_missing = true;

    // 尝试列出已有列族
    std::vector<std::string> existingCfNames;
    auto listStatus = rocksdb::DB::ListColumnFamilies(m_options, m_dbPath, &existingCfNames);

    if (listStatus.ok() && !existingCfNames.empty()) {
        // 数据库已存在，用所有已有列族打开
        std::vector<rocksdb::ColumnFamilyDescriptor> cfDescs;
        for (const auto& name : existingCfNames) {
            cfDescs.emplace_back(name, rocksdb::ColumnFamilyOptions());
        }

        std::vector<rocksdb::ColumnFamilyHandle*> cfHandles;
        auto s = rocksdb::DB::Open(m_options, m_dbPath, cfDescs, &cfHandles, &m_db);
        if (!s.ok()) {
            throw std::runtime_error("Failed to open RocksDB: " + s.ToString());
        }

        for (size_t i = 0; i < existingCfNames.size(); ++i) {
            m_cfHandles[existingCfNames[i]] = cfHandles[i];
        }
    } else {
        // 新建数据库，只有 default 列族
        auto s = rocksdb::DB::Open(m_options, m_dbPath, &m_db);
        if (!s.ok()) {
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

    // 创建新列族
    rocksdb::ColumnFamilyHandle* handle = nullptr;
    auto s = m_db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), cfName, &handle);
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

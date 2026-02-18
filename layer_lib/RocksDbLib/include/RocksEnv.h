#pragma once

#include <string>
#include <map>
#include <vector>
#include <rocksdb/db.h>
#include <rocksdb/options.h>


/// RocksDB 环境管理器
/// 管理一个 RocksDB 数据库实例的生命周期，包括打开数据库、管理列族、关闭和刷盘
class CRocksEnv {
public:
    /// 构造函数：指定数据库目录路径并打开 RocksDB 实例
    /// @param dbPath 数据库目录路径，RocksDB 会在此目录创建/打开数据库文件
    ///               若目录不存在会自动创建；若已有数据库则加载其所有已有列族
    explicit CRocksEnv(const std::string& dbPath);

    /// 析构函数：关闭所有列族句柄并释放数据库实例
    ~CRocksEnv();

    /// 获取已有列族句柄，若不存在则创建新列族
    /// @param cfName 列族名称（如 "M1", "M5", "D1" 等时间周期标识）
    /// @return 对应的列族句柄指针，生命周期由 CRocksEnv 管理，调用者不应 delete
    /// @throws std::runtime_error 创建列族失败时抛出
    rocksdb::ColumnFamilyHandle* GetOrCreateColumnFamily(const std::string& cfName);

    /// 获取已有列族句柄，不存在时返回 nullptr（不会自动创建）
    /// @param cfName 列族名称
    /// @return 列族句柄指针，不存在返回 nullptr
    rocksdb::ColumnFamilyHandle* GetColumnFamilyHandle(const std::string& cfName) const;

    /// 删除指定列族及其所有数据
    /// @param cfName 要删除的列族名称，不允许删除 "default" 列族
    /// @throws std::runtime_error 删除失败或尝试删除 default 列族时抛出
    void DropColumnFamily(const std::string& cfName);

    /// 获取当前数据库中所有列族的名称列表
    /// @return 列族名称的 vector，包含 "default"
    std::vector<std::string> GetAllColumnFamilyNames() const;

    /// 将所有列族的 memtable 刷写到磁盘 SST 文件，确保数据持久化
    /// @throws std::runtime_error 刷盘失败时抛出
    void Flush();

    /// 将指定列族的 memtable 刷写到磁盘
    /// @param cfName 目标列族名称
    /// @throws std::runtime_error 列族不存在或刷盘失败时抛出
    void Flush(const std::string& cfName);

    /// 获取底层 rocksdb::DB 原始指针，供其他类（如 CRocksDb）直接操作数据库
    /// @return DB 指针，生命周期由 CRocksEnv 管理，调用者不应 delete
    rocksdb::DB* GetDB() const;

    /// 获取数据库目录路径
    /// @return 构造时传入的 dbPath
    const std::string& GetDbPath() const;

    /// 获取共享的 WriteOptions（disableWAL = true，无压缩场景下最大化写入性能）
    const rocksdb::WriteOptions& GetWriteOptions() const { return m_writeOptions; }

    /// 获取 RocksDB 运行状态摘要，用于性能诊断
    /// @return 包含 write stall、L0 文件数、pending compaction、memtable 使用量等关键指标的字符串
    std::string DumpStats() const;

    // 禁止拷贝
    CRocksEnv(const CRocksEnv&) = delete;
    CRocksEnv& operator=(const CRocksEnv&) = delete;

private:
    std::string m_dbPath;
    rocksdb::DB* m_db = nullptr;
    rocksdb::Options m_options;
    rocksdb::WriteOptions m_writeOptions;
    /// 列族名 -> 列族句柄 的映射，包含 default 列族
    std::map<std::string, rocksdb::ColumnFamilyHandle*> m_cfHandles;
};

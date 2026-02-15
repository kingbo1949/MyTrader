#pragma once

#include "RocksEnv.h"
#include "RocksKey.h"
#include <string>
#include <vector>
#include <functional>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/iterator.h>

/// 单个列族上的数据读写操作类
/// 绑定一个 CRocksEnv 和一个列族，提供 Put/Get/Delete/Scan 等操作
/// 业务层为每个列族（如 KLINE_M5、MACD_D1）各持有一个 CRocksDb 实例
class CRocksDb {
public:
    /// 构造函数：绑定数据库环境和列族
    /// @param env   CRocksEnv 引用，生命周期必须长于本对象
    /// @param cfName 列族名称（如 "KLINE_M5"），若不存在会自动创建
    CRocksDb(CRocksEnv& env, const std::string& cfName);

    ~CRocksDb() = default;

    // 禁止拷贝
    CRocksDb(const CRocksDb&) = delete;
    CRocksDb& operator=(const CRocksDb&) = delete;

    /// 写入一条 key-value 数据
    /// @param key   CRocksKey（合约+时间戳）
    /// @param value 序列化后的数据（如 Ice marshal 的字节流）
    /// @throws std::runtime_error 写入失败时抛出
    void Put(const CRocksKey& key, const rocksdb::Slice& value);

    /// 读取一条数据
    /// @param key   CRocksKey（合约+时间戳）
    /// @param[out] value 读取到的数据存入此 string
    /// @return true 找到数据，false key 不存在
    /// @throws std::runtime_error 读取出错（非 NotFound 的错误）时抛出
    bool Get(const CRocksKey& key, std::string* value);

    /// 删除一条数据
    /// @param key CRocksKey（合约+时间戳）
    /// @throws std::runtime_error 删除失败时抛出
    void Delete(const CRocksKey& key);

    /// 前缀扫描：获取指定合约的所有数据
    /// @param symbol  合约名称（如 "TSLA"）
    /// @param callback 对每条记录调用的回调函数，参数为 (CRocksKey, value Slice)
    ///                 回调返回 true 继续扫描，返回 false 提前终止
    void ScanBySymbol(const std::string& symbol,
                      const std::function<bool(const CRocksKey&, const rocksdb::Slice&)>& callback);

    /// 范围扫描：获取指定合约在时间范围内的数据
    /// @param symbol  合约名称
    /// @param startTime 起始时间（含）
    /// @param endTime   结束时间（含）
    /// @param callback  同 ScanBySymbol 的回调
    void ScanByRange(const std::string& symbol, time_t startTime, time_t endTime,
                     const std::function<bool(const CRocksKey&, const rocksdb::Slice&)>& callback);

    /// 删除指定合约在时间范围内的所有数据
    /// @param symbol    合约名称
    /// @param startTime 起始时间（含）
    /// @param endTime   结束时间（含）
    /// @throws std::runtime_error 删除失败时抛出
    void DeleteRange(const std::string& symbol, time_t startTime, time_t endTime);

    /// 获取绑定的列族名称
    const std::string& GetColumnFamilyName() const;

private:
    CRocksEnv& m_env;
    std::string m_cfName;
    rocksdb::ColumnFamilyHandle* m_cfHandle;  ///< 启动时缓存，避免运行时字符串查找
};

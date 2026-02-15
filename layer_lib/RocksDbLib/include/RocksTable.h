#pragma once

#include "RocksEnv.h"
#include "RocksKey.h"
#include "RocksSerializer.h"
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>

/// 模板化的表操作类，自动处理 VALUE 的序列化/反序列化
/// POD 类型自动使用 memcpy，Ice 类型通过 ROCKS_ICE_SERIALIZER 宏特化
///
/// 用法示例：
///   CRocksTable<KLineData> klineTable(env, "KLINE_M5");
///   klineTable.Put(key, klineData);     // 自动序列化
///   KLineData data;
///   klineTable.Get(key, &data);         // 自动反序列化
template<typename VALUE>
class CRocksTable {
public:
    /// 构造函数：绑定数据库环境和列族
    /// @param env    CRocksEnv 引用，生命周期必须长于本对象
    /// @param cfName 列族名称（如 "KLINE_M5"），若不存在会自动创建
    CRocksTable(CRocksEnv& env, const std::string& cfName)
        : m_env(env)
        , m_cfName(cfName)
        , m_cfHandle(env.GetOrCreateColumnFamily(cfName))
    {
    }

    /// 写入一条数据（自动序列化 VALUE）
    /// @param key   CRocksKey（合约+时间戳）
    /// @param value 业务对象，由 RocksSerializer<VALUE> 序列化
    void Put(const CRocksKey& key, const VALUE& value)
    {
        std::string data = RocksSerializer<VALUE>::Serialize(value);
        rocksdb::WriteOptions wo;
        auto s = m_env.GetDB()->Put(wo, m_cfHandle, key.ToSlice(), data);
        if (!s.ok()) {
            throw std::runtime_error("Put failed [" + m_cfName + "]: " + s.ToString());
        }
    }

    /// 读取一条数据（自动反序列化为 VALUE）
    /// @param key        CRocksKey（合约+时间戳）
    /// @param[out] value 反序列化后的业务对象
    /// @return true 找到数据，false key 不存在
    bool Get(const CRocksKey& key, VALUE* value)
    {
        std::string data;
        rocksdb::ReadOptions ro;
        auto s = m_env.GetDB()->Get(ro, m_cfHandle, key.ToSlice(), &data);
        if (s.ok()) {
            *value = RocksSerializer<VALUE>::Deserialize(
                rocksdb::Slice(data));
            return true;
        }
        if (s.IsNotFound()) {
            return false;
        }
        throw std::runtime_error("Get failed [" + m_cfName + "]: " + s.ToString());
    }

    /// 删除一条数据
    /// @param key CRocksKey（合约+时间戳）
    void Delete(const CRocksKey& key)
    {
        rocksdb::WriteOptions wo;
        auto s = m_env.GetDB()->Delete(wo, m_cfHandle, key.ToSlice());
        if (!s.ok()) {
            throw std::runtime_error("Delete failed [" + m_cfName + "]: " + s.ToString());
        }
    }

    /// 前缀扫描：获取指定合约的所有数据
    /// @param symbol  合约名称
    /// @param callback 回调函数 (CRocksKey, VALUE)，返回 true 继续，false 终止
    void ScanBySymbol(const std::string& symbol,
                      const std::function<bool(const CRocksKey&, const VALUE&)>& callback)
    {
        CRocksKey prefixKey(symbol, 0);
        rocksdb::Slice prefix = prefixKey.SymbolPrefix();

        rocksdb::ReadOptions ro;
        std::unique_ptr<rocksdb::Iterator> it(m_env.GetDB()->NewIterator(ro, m_cfHandle));

        for (it->Seek(prefixKey.ToSlice()); it->Valid(); it->Next()) {
            if (it->key().size() != sizeof(CRocksKey) ||
                std::memcmp(it->key().data(), prefix.data(), prefix.size()) != 0) {
                break;
            }

            CRocksKey currentKey = CRocksKey::FromSlice(it->key());
            VALUE val = RocksSerializer<VALUE>::Deserialize(it->value());
            if (!callback(currentKey, val)) {
                break;
            }
        }

        if (!it->status().ok()) {
            throw std::runtime_error("ScanBySymbol failed [" + m_cfName + "]: " + it->status().ToString());
        }
    }

    /// 范围扫描：获取指定合约在时间范围内的数据
    /// @param symbol    合约名称
    /// @param startTime 起始时间（含）
    /// @param endTime   结束时间（含）
    /// @param callback  回调函数 (CRocksKey, VALUE)
    void ScanByRange(const std::string& symbol, time_t startTime, time_t endTime,
                     const std::function<bool(const CRocksKey&, const VALUE&)>& callback)
    {
        CRocksKey startKey(symbol, startTime);
        rocksdb::Slice prefix = startKey.SymbolPrefix();

        rocksdb::ReadOptions ro;
        std::unique_ptr<rocksdb::Iterator> it(m_env.GetDB()->NewIterator(ro, m_cfHandle));

        for (it->Seek(startKey.ToSlice()); it->Valid(); it->Next()) {
            if (it->key().size() != sizeof(CRocksKey) ||
                std::memcmp(it->key().data(), prefix.data(), prefix.size()) != 0) {
                break;
            }

            CRocksKey currentKey = CRocksKey::FromSlice(it->key());
            if (currentKey.GetTimestamp() > endTime) {
                break;
            }

            VALUE val = RocksSerializer<VALUE>::Deserialize(it->value());
            if (!callback(currentKey, val)) {
                break;
            }
        }

        if (!it->status().ok()) {
            throw std::runtime_error("ScanByRange failed [" + m_cfName + "]: " + it->status().ToString());
        }
    }

    /// 删除指定合约在时间范围内的所有数据
    /// @param symbol    合约名称
    /// @param startTime 起始时间（含）
    /// @param endTime   结束时间（含）
    void DeleteRange(const std::string& symbol, time_t startTime, time_t endTime)
    {
        CRocksKey beginKey(symbol, startTime);
        CRocksKey endKey(symbol, endTime + 1);

        rocksdb::WriteOptions wo;
        auto s = m_env.GetDB()->DeleteRange(wo, m_cfHandle, beginKey.ToSlice(), endKey.ToSlice());
        if (!s.ok()) {
            throw std::runtime_error("DeleteRange failed [" + m_cfName + "]: " + s.ToString());
        }
    }

    /// 逆向查找：从截止时间往前取 N 条记录
    /// @param symbol  合约名称
    /// @param endTime 截止时间（含），从此时间点往前回溯
    /// @param count   最多取多少条记录
    /// @param[out] results 结果按时间正序存入（最早的在前）
    void ScanBackwardN(const std::string& symbol, time_t endTime, size_t count,
                      std::vector<std::pair<CRocksKey, VALUE>>& results)
    {
        results.clear();
        CRocksKey endKey(symbol, endTime);
        rocksdb::Slice prefix = endKey.SymbolPrefix();

        rocksdb::ReadOptions ro;
        std::unique_ptr<rocksdb::Iterator> it(m_env.GetDB()->NewIterator(ro, m_cfHandle));

        // SeekForPrev 定位到 <= endKey 的最后一条
        it->SeekForPrev(endKey.ToSlice());

        for (; it->Valid() && results.size() < count; it->Prev()) {
            if (it->key().size() != sizeof(CRocksKey) ||
                std::memcmp(it->key().data(), prefix.data(), prefix.size()) != 0) {
                break;
            }

            CRocksKey currentKey = CRocksKey::FromSlice(it->key());
            VALUE val = RocksSerializer<VALUE>::Deserialize(it->value());
            results.emplace_back(currentKey, std::move(val));
        }

        if (!it->status().ok()) {
            throw std::runtime_error("ScanBackward failed [" + m_cfName + "]: " + it->status().ToString());
        }

        // 逆向收集的结果翻转为时间正序
        std::reverse(results.begin(), results.end());
    }

    /// 逆向查找：从截止时间往前回溯到起始时间
    /// @param symbol    合约名称
    /// @param startTime 起始时间（含），回溯到此时间停止
    /// @param endTime   截止时间（含），从此时间点开始往前
    /// @param[out] results 结果按时间正序存入（最早的在前）
    void ScanBackwardRange(const std::string& symbol, time_t startTime, time_t endTime,
                      std::vector<std::pair<CRocksKey, VALUE>>& results)
    {
        results.clear();
        CRocksKey endKey(symbol, endTime);
        rocksdb::Slice prefix = endKey.SymbolPrefix();

        rocksdb::ReadOptions ro;
        std::unique_ptr<rocksdb::Iterator> it(m_env.GetDB()->NewIterator(ro, m_cfHandle));

        it->SeekForPrev(endKey.ToSlice());

        for (; it->Valid(); it->Prev()) {
            if (it->key().size() != sizeof(CRocksKey) ||
                std::memcmp(it->key().data(), prefix.data(), prefix.size()) != 0) {
                break;
            }

            CRocksKey currentKey = CRocksKey::FromSlice(it->key());
            if (currentKey.GetTimestamp() < startTime) {
                break;
            }

            VALUE val = RocksSerializer<VALUE>::Deserialize(it->value());
            results.emplace_back(currentKey, std::move(val));
        }

        if (!it->status().ok()) {
            throw std::runtime_error("ScanBackward failed [" + m_cfName + "]: " + it->status().ToString());
        }

        std::reverse(results.begin(), results.end());
    }

    /// 获取列族句柄（供 CRocksWriteBatch 跨列族批量写入时使用）
    /// @return 列族句柄指针
    rocksdb::ColumnFamilyHandle* GetCFHandle() const { return m_cfHandle; }

    /// 获取列族名称
    const std::string& GetColumnFamilyName() const { return m_cfName; }

private:
    CRocksEnv& m_env;
    std::string m_cfName;
    rocksdb::ColumnFamilyHandle* m_cfHandle;
};

#pragma once

#include "RocksEnv.h"
#include "RocksKey.h"
#include <rocksdb/write_batch.h>
#include <rocksdb/options.h>

/// 批量写入封装
/// 将多条 Put/Delete 操作缓存到 WriteBatch 中，调用 Commit 一次性原子提交
/// 支持跨列族操作——同一个 batch 可以同时写入 KLINE_M5 和 MACD_M5
///
/// 典型用法：
///   CRocksWriteBatch batch(env);
///   batch.Put(klineHandle, key1, value1);
///   batch.Put(macdHandle, key2, value2);
///   batch.Commit();
class CRocksWriteBatch {
public:
    /// 构造函数：绑定数据库环境
    /// @param env CRocksEnv 引用，生命周期必须长于本对象
    explicit CRocksWriteBatch(CRocksEnv& env);

    ~CRocksWriteBatch() = default;

    // 禁止拷贝
    CRocksWriteBatch(const CRocksWriteBatch&) = delete;
    CRocksWriteBatch& operator=(const CRocksWriteBatch&) = delete;

    /// 向 batch 中添加一条写入操作
    /// @param cfHandle 目标列族句柄（从 CRocksEnv::GetOrCreateColumnFamily 获取）
    /// @param key      CRocksKey（合约+时间戳）
    /// @param value    序列化后的数据
    void Put(rocksdb::ColumnFamilyHandle* cfHandle,
             const CRocksKey& key, const rocksdb::Slice& value);

    /// 向 batch 中添加一条删除操作
    /// @param cfHandle 目标列族句柄
    /// @param key      要删除的 CRocksKey
    void Delete(rocksdb::ColumnFamilyHandle* cfHandle, const CRocksKey& key);

    /// 将 batch 中所有操作一次性原子提交到数据库
    /// 提交后自动清空 batch，可重复使用
    /// @throws std::runtime_error 提交失败时抛出
    void Commit();

    /// 清空 batch 中未提交的操作（不写入数据库）
    void Clear();

    /// 获取当前 batch 中待提交的操作数量
    /// @return 操作条数
    int Count() const;

private:
    CRocksEnv& m_env;
    rocksdb::WriteBatch m_batch;
};

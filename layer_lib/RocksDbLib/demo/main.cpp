#include "RocksEnv.h"
#include "RocksTable.h"
#include "RocksWriteBatch.h"
#include "RocksDb.h"
#include <iostream>
#include <cassert>
#include <filesystem>

/// 模拟 KLine 数据的 POD 结构
#pragma pack(push, 1)
struct KLineData {
    double open;
    double high;
    double low;
    double close;
    int64_t volume;
};
#pragma pack(pop)

static const std::string TEST_DB_PATH = "/tmp/rocksdb_demo_test";

/// 清理测试数据库
void CleanUp()
{
    std::filesystem::remove_all(TEST_DB_PATH);
}

/// 测试1：CRocksEnv 基本操作
void TestEnv()
{
    std::cout << "=== Test CRocksEnv ===" << std::endl;

    CRocksEnv env(TEST_DB_PATH);

    // 创建列族
    auto* h1 = env.GetOrCreateColumnFamily("KLINE_M5");
    auto* h2 = env.GetOrCreateColumnFamily("MACD_M5");
    assert(h1 != nullptr);
    assert(h2 != nullptr);

    // 重复获取应返回同一 handle
    auto* h1_again = env.GetOrCreateColumnFamily("KLINE_M5");
    assert(h1 == h1_again);

    // 只读获取
    auto* h3 = env.GetColumnFamilyHandle("KLINE_M5");
    assert(h3 == h1);
    auto* h_null = env.GetColumnFamilyHandle("NOT_EXIST");
    assert(h_null == nullptr);

    // 列族列表
    auto names = env.GetAllColumnFamilyNames();
    std::cout << "  Column families: ";
    for (const auto& n : names) std::cout << n << " ";
    std::cout << std::endl;
    assert(names.size() >= 3); // default + KLINE_M5 + MACD_M5

    // 删除列族
    env.DropColumnFamily("MACD_M5");
    assert(env.GetColumnFamilyHandle("MACD_M5") == nullptr);

    // 刷盘
    env.Flush();

    std::cout << "  PASSED" << std::endl;
}

/// 测试2：CRocksKey 字节序与排序
void TestKey()
{
    std::cout << "=== Test CRocksKey ===" << std::endl;

    CRocksKey k1("TSLA", 1000);
    CRocksKey k2("TSLA", 2000);
    CRocksKey k3("AAPL", 1000);

    // 验证 round-trip
    assert(k1.GetTimestamp() == 1000);
    assert(k2.GetTimestamp() == 2000);
    assert(k3.GetSymbol() == "AAPL");

    // 验证大端序排序：同一合约，时间小的 key 字节序更小
    auto s1 = k1.ToSlice();
    auto s2 = k2.ToSlice();
    assert(s1.compare(s2) < 0); // k1 < k2

    // FromSlice round-trip
    auto k1_copy = CRocksKey::FromSlice(s1);
    assert(k1_copy.GetSymbol() == "TSLA");
    assert(k1_copy.GetTimestamp() == 1000);

    // sizeof 检查
    static_assert(sizeof(CRocksKey) == 72);

    std::cout << "  PASSED" << std::endl;
}

/// 测试3：CRocksTable Put/Get/Delete
void TestTableBasic()
{
    std::cout << "=== Test CRocksTable Put/Get/Delete ===" << std::endl;

    CRocksEnv env(TEST_DB_PATH);
    CRocksTable<KLineData> table(env, "KLINE_M5");

    CRocksKey key("TSLA", 1707900000);
    KLineData kline{100.0, 105.0, 99.0, 103.0, 50000};

    // Put
    table.Put(key, kline);

    // Get
    KLineData result{};
    bool found = table.Get(key, &result);
    assert(found);
    assert(result.open == 100.0);
    assert(result.high == 105.0);
    assert(result.low == 99.0);
    assert(result.close == 103.0);
    assert(result.volume == 50000);

    // Get 不存在的 key
    CRocksKey missing("TSLA", 9999);
    found = table.Get(missing, &result);
    assert(!found);

    // Delete
    table.Delete(key);
    found = table.Get(key, &result);
    assert(!found);

    std::cout << "  PASSED" << std::endl;
}

/// 测试4：ScanBySymbol 和 ScanByRange
void TestTableScan()
{
    std::cout << "=== Test CRocksTable Scan ===" << std::endl;

    CRocksEnv env(TEST_DB_PATH);
    CRocksTable<KLineData> table(env, "KLINE_M5");

    // 写入 TSLA 10 条数据 (时间 1000~10000，步长 1000)
    for (int i = 1; i <= 10; ++i) {
        CRocksKey key("TSLA", i * 1000);
        KLineData kline{double(i), double(i + 1), double(i - 1), double(i), int64_t(i * 100)};
        table.Put(key, kline);
    }

    // 写入 AAPL 5 条数据
    for (int i = 1; i <= 5; ++i) {
        CRocksKey key("AAPL", i * 1000);
        KLineData kline{double(i * 10), 0, 0, 0, 0};
        table.Put(key, kline);
    }

    // ScanBySymbol: TSLA 应该有 10 条
    int tslaCount = 0;
    table.ScanBySymbol("TSLA", [&](const CRocksKey& k, const KLineData& v) {
        ++tslaCount;
        return true;
    });
    assert(tslaCount == 10);
    std::cout << "  ScanBySymbol TSLA: " << tslaCount << " records" << std::endl;

    // ScanBySymbol: AAPL 应该有 5 条
    int aaplCount = 0;
    table.ScanBySymbol("AAPL", [&](const CRocksKey& k, const KLineData& v) {
        ++aaplCount;
        return true;
    });
    assert(aaplCount == 5);
    std::cout << "  ScanBySymbol AAPL: " << aaplCount << " records" << std::endl;

    // ScanByRange: TSLA 时间 3000~7000，应有 5 条 (3000,4000,5000,6000,7000)
    int rangeCount = 0;
    table.ScanByRange("TSLA", 3000, 7000, [&](const CRocksKey& k, const KLineData& v) {
        ++rangeCount;
        assert(k.GetTimestamp() >= 3000 && k.GetTimestamp() <= 7000);
        return true;
    });
    assert(rangeCount == 5);
    std::cout << "  ScanByRange TSLA [3000,7000]: " << rangeCount << " records" << std::endl;

    std::cout << "  PASSED" << std::endl;
}

/// 测试5：ScanBackwardN 和 ScanBackwardRange
void TestTableScanBackward()
{
    std::cout << "=== Test CRocksTable ScanBackward ===" << std::endl;

    CRocksEnv env(TEST_DB_PATH);
    CRocksTable<KLineData> table(env, "KLINE_M5");

    // 数据已在 TestTableScan 中写入 (TSLA: 1000~10000)

    // ScanBackwardN: 从 8000 往前取 3 条，应得到 6000,7000,8000
    std::vector<std::pair<CRocksKey, KLineData>> results;
    table.ScanBackwardN("TSLA", 8000, 3, results);
    assert(results.size() == 3);
    assert(results[0].first.GetTimestamp() == 6000);
    assert(results[1].first.GetTimestamp() == 7000);
    assert(results[2].first.GetTimestamp() == 8000);
    std::cout << "  ScanBackwardN(8000, 3): ";
    for (const auto& [k, v] : results) std::cout << k.GetTimestamp() << " ";
    std::cout << std::endl;

    // ScanBackwardRange: 从 8000 回溯到 5000，应得到 5000,6000,7000,8000
    table.ScanBackwardRange("TSLA", 5000, 8000, results);
    assert(results.size() == 4);
    assert(results[0].first.GetTimestamp() == 5000);
    assert(results[3].first.GetTimestamp() == 8000);
    std::cout << "  ScanBackwardRange(5000,8000): ";
    for (const auto& [k, v] : results) std::cout << k.GetTimestamp() << " ";
    std::cout << std::endl;

    std::cout << "  PASSED" << std::endl;
}

/// 测试6：CRocksWriteBatch 批量写入
void TestWriteBatch()
{
    std::cout << "=== Test CRocksWriteBatch ===" << std::endl;

    CRocksEnv env(TEST_DB_PATH);
    auto* cfHandle = env.GetOrCreateColumnFamily("BATCH_TEST");

    CRocksWriteBatch batch(env);

    // 批量写入 5 条
    for (int i = 1; i <= 5; ++i) {
        CRocksKey key("ES", i * 100);
        KLineData kline{double(i), 0, 0, 0, 0};
        std::string data = RocksSerializer<KLineData>::Serialize(kline);
        batch.Put(cfHandle, key, data);
    }
    assert(batch.Count() == 5);

    // 提交
    batch.Commit();
    assert(batch.Count() == 0); // 提交后清空

    // 验证数据已写入
    CRocksTable<KLineData> table(env, "BATCH_TEST");
    int count = 0;
    table.ScanBySymbol("ES", [&](const CRocksKey& k, const KLineData& v) {
        ++count;
        return true;
    });
    assert(count == 5);
    std::cout << "  Batch wrote " << count << " records" << std::endl;

    std::cout << "  PASSED" << std::endl;
}

/// 测试7：DeleteRange
void TestDeleteRange()
{
    std::cout << "=== Test DeleteRange ===" << std::endl;

    CRocksEnv env(TEST_DB_PATH);
    CRocksTable<KLineData> table(env, "KLINE_M5");

    // 删除 TSLA 时间 3000~7000 的数据
    table.DeleteRange("TSLA", 3000, 7000);

    // 验证：剩余 TSLA 应为 5 条 (1000,2000,8000,9000,10000)
    std::vector<time_t> remaining;
    table.ScanBySymbol("TSLA", [&](const CRocksKey& k, const KLineData& v) {
        remaining.push_back(k.GetTimestamp());
        return true;
    });
    assert(remaining.size() == 5);
    assert(remaining[0] == 1000);
    assert(remaining[1] == 2000);
    assert(remaining[2] == 8000);
    assert(remaining[3] == 9000);
    assert(remaining[4] == 10000);

    std::cout << "  Remaining TSLA after DeleteRange(3000,7000): ";
    for (auto ts : remaining) std::cout << ts << " ";
    std::cout << std::endl;

    // AAPL 应不受影响，仍为 5 条
    int aaplCount = 0;
    table.ScanBySymbol("AAPL", [&](const CRocksKey& k, const KLineData& v) {
        ++aaplCount;
        return true;
    });
    assert(aaplCount == 5);
    std::cout << "  AAPL unaffected: " << aaplCount << " records" << std::endl;

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "RocksDbLib Demo & Test" << std::endl;
    std::cout << "DB path: " << TEST_DB_PATH << std::endl << std::endl;

    // 清理旧数据
    CleanUp();

    try {
        TestEnv();
        TestKey();
        TestTableBasic();
        TestTableScan();
        TestTableScanBackward();
        TestWriteBatch();
        TestDeleteRange();

        std::cout << std::endl << "All tests PASSED!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << std::endl;
        CleanUp();
        return 1;
    }

    CleanUp();
    return 0;
}

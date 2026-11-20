import pickle
from datetime import datetime, timedelta
from pathlib import Path

TARGET_DT      = datetime(2024, 1, 3, 5, 10)
CACHE_FILE     = Path(__file__).resolve().parents[1] / "datadb" / "ES_M5_20240101_20260601.pkl"
MIN_DIF_ABS    = 1.0
COOLDOWN_HOURS = 1
MAX_POSITIONS  = 100

with open(CACHE_FILE, "rb") as f:
    bars = pickle.load(f)

# ── 目标 bar 信息 ─────────────────────────────────────────────────────────────
idx = next((i for i, b in enumerate(bars) if b.datetime == TARGET_DT), None)
if idx is None:
    print(f"找不到 bar: {TARGET_DT}")
    exit()

bar = bars[idx]
print(f"=== 目标 bar ===")
print(f"datetime  : {bar.datetime}")
print(f"is_uturn  : {bar.is_uturn}")
print(f"div_type  : {bar.div_type}")
print(f"dif       : {bar.dif:.4f}  (abs={abs(bar.dif):.4f})")
print()

# ── 从头模拟入场，追踪 _last_entry_dt 和持仓数 ───────────────────────────────
last_entry_dt = None
open_positions = 0

for b in bars[:idx]:
    # 模拟到期（简化：忽略具体到期时刻，只统计持仓数用于调试）
    is_signal = (b.is_uturn
                 and b.div_type in ('Bottom', 'BottomSub')
                 and abs(b.dif) >= MIN_DIF_ABS)
    if not is_signal:
        continue
    cooldown_ok = (last_entry_dt is None
                   or b.datetime >= last_entry_dt + timedelta(hours=COOLDOWN_HOURS))
    pos_ok = (MAX_POSITIONS <= 0 or open_positions < MAX_POSITIONS)
    if cooldown_ok and pos_ok:
        last_entry_dt = b.datetime
        open_positions += 1

print(f"=== 模拟到达目标 bar 前的状态 ===")
print(f"last_entry_dt  : {last_entry_dt}")
print(f"open_positions : {open_positions}  (max={MAX_POSITIONS})")
print()

# ── 逐条件诊断 ────────────────────────────────────────────────────────────────
print("=== 目标 bar 开仓条件诊断 ===")
c1 = bar.is_uturn
c2 = bar.div_type in ('Bottom', 'BottomSub')
c3 = abs(bar.dif) >= MIN_DIF_ABS
c4 = (last_entry_dt is None
      or bar.datetime >= last_entry_dt + timedelta(hours=COOLDOWN_HOURS))
c5 = (MAX_POSITIONS <= 0 or open_positions < MAX_POSITIONS)

print(f"  is_uturn               : {'✓' if c1 else '✗'}  ({bar.is_uturn})")
print(f"  div_type               : {'✓' if c2 else '✗'}  ({bar.div_type})")
print(f"  abs(dif) >= {MIN_DIF_ABS}       : {'✓' if c3 else '✗'}  ({abs(bar.dif):.4f})")
if last_entry_dt:
    elapsed = (bar.datetime - last_entry_dt).total_seconds() / 3600
    print(f"  cooldown ({COOLDOWN_HOURS}h)          : {'✓' if c4 else '✗'}  "
          f"(last={last_entry_dt}, elapsed={elapsed:.2f}h)")
else:
    print(f"  cooldown ({COOLDOWN_HOURS}h)          : ✓  (无历史入场)")
print(f"  max_positions ({MAX_POSITIONS})    : {'✓' if c5 else '✗'}  (当前={open_positions})")
print()
print("结论:", "✓ 所有条件满足，应该开仓" if all([c1,c2,c3,c4,c5]) else "✗ 有条件未满足（见上方 ✗ 项）")

"""
从优化报告中读取指定排名的参数，写入 strategies.yaml 并启用对应策略。

用法:
    python copy_params_to_mainbacktest.py <opt_filename> <rank>

示例:
    python copy_params_to_mainbacktest.py UTurnBSync_DebitCallSpread_SMH_M5_opt_NQ-SMH.txt 2
"""
import ast
import re
import sys
from pathlib import Path

from ruamel.yaml import YAML

ROOT          = Path(__file__).resolve().parents[1]
OUTPUT_DIR    = ROOT / "output"
STRATEGIES_YAML = ROOT / "config" / "strategies.yaml"


# ── 文件查找 ──────────────────────────────────────────────────────────────────

def _find_opt_file(filename: str) -> Path:
    for p in OUTPUT_DIR.rglob(filename):
        return p
    raise FileNotFoundError(f"找不到优化文件: {filename}（搜索目录: {OUTPUT_DIR}）")


# ── 解析优化报告 ──────────────────────────────────────────────────────────────

def _extract_task_id(txt_path: Path) -> str:
    with open(txt_path, encoding="utf-8") as f:
        for line in f:
            m = re.search(r'参数优化报告:\s+(\S+)', line)
            if m:
                return m.group(1)
    raise ValueError(f"无法从文件提取 task_id: {txt_path}")


def _extract_params_at_rank(txt_path: Path, rank: int) -> dict:
    with open(txt_path, encoding="utf-8") as f:
        for line in f:
            m = re.match(r'^(\d+)\s+(\{.+\})\s', line)
            if m and int(m.group(1)) == rank:
                return ast.literal_eval(m.group(2))
    raise ValueError(f"找不到排名 {rank} 的参数（文件: {txt_path}）")


# ── task_id 解析 ──────────────────────────────────────────────────────────────

def _collect_known_classes(strategies: list) -> list[str]:
    seen, result = set(), []
    for s in strategies:
        cls = s.get("class", "")
        if cls and cls not in seen:
            seen.add(cls)
            result.append(cls)
    return result


def _parse_task_id(task_id: str, known_classes: list[str]) -> tuple[str, str, str, str]:
    """从 task_id（{class}_{code_id}_{interval}_{direction}）解析四元组。"""
    parts      = task_id.split('_')
    direction  = parts[-1]
    interval   = parts[-2]
    class_and_code = '_'.join(parts[:-2])
    for cls in sorted(known_classes, key=len, reverse=True):
        if class_and_code.startswith(cls + '_') or class_and_code == cls:
            code_id = class_and_code[len(cls):].lstrip('_')
            return cls, code_id, interval, direction
    raise ValueError(f"无法从 task_id 解析策略类名: {task_id}\n已知类名: {known_classes}")


# ── sync_signals 解析 ────────────────────────────────────────────────────────

def _parse_sync_signals(filename: str) -> list[dict]:
    """从文件名后缀解析 sync_signals，如 _NQ-SMH → [{code_id:NQ,...},{code_id:SMH,...}]。"""
    stem = Path(filename).stem
    if '_opt_' not in stem:
        return []
    suffix = stem.split('_opt_', 1)[1]
    return [{'code_id': cid, 'min_dif_abs': 0.0} for cid in suffix.split('-') if cid]


# ── strategies.yaml 匹配与更新 ─────────────────────────────────────────────────

def _find_target_index(strategies: list, cls: str) -> int:
    matches = [i for i, s in enumerate(strategies) if str(s.get("class")) == cls]
    if not matches:
        raise ValueError(f"strategies.yaml 中找不到策略: class={cls}")
    return matches[0]


def _apply_params(strategies: list, target_idx: int, opt_params: dict) -> None:
    for i, s in enumerate(strategies):
        s["enable"] = (i == target_idx)
    params = strategies[target_idx].get("params") or {}
    for k, v in opt_params.items():
        params[k] = v
    strategies[target_idx]["params"] = params


def _load_yaml(path: Path):
    ryaml = YAML()
    ryaml.preserve_quotes = True
    with open(path, encoding="utf-8") as f:
        return ryaml, ryaml.load(f)


def _save_yaml(ryaml, doc, path: Path) -> None:
    with open(path, "w", encoding="utf-8") as f:
        ryaml.dump(doc, f)


# ── 主流程 ────────────────────────────────────────────────────────────────────

def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)

    filename = sys.argv[1]
    rank     = int(sys.argv[2])

    txt_path   = _find_opt_file(filename)
    task_id    = _extract_task_id(txt_path)
    opt_params = _extract_params_at_rank(txt_path, rank)

    print(f"文件  : {txt_path}")
    print(f"任务  : {task_id}")
    print(f"排名  : {rank}")
    print(f"参数  : {opt_params}")

    ryaml, doc = _load_yaml(STRATEGIES_YAML)
    strategies  = doc["strategies"]

    known_classes = _collect_known_classes(strategies)
    cls, code_id, interval, direction = _parse_task_id(task_id, known_classes)
    print(f"匹配  : class={cls}  code_id={code_id}  interval={interval}  direction={direction}")

    sync_signals = _parse_sync_signals(filename)
    if sync_signals:
        opt_params['sync_signals'] = sync_signals
        print(f"同步信号: {[s['code_id'] for s in sync_signals]}")

    target_idx = _find_target_index(strategies, cls)
    _apply_params(strategies, target_idx, opt_params)
    _save_yaml(ryaml, doc, STRATEGIES_YAML)

    print(f"完成  : 策略 #{target_idx+1} 已启用，其余 {len(strategies)-1} 个已禁用")
    print(f"写入参数: {list(opt_params.keys())}")


if __name__ == "__main__":
    main()

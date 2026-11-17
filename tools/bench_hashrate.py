#!/usr/bin/env python3
"""Measure NerdMiner hashrate from the serial log, for A/B (ABBA) comparisons.

Resets the board, requires the startup KAT to pass, skips a warm-up, then fits a
least-squares line through cumulative hash counters:

  * STATS lines (firmware built with -D DEBUG_HASH_STATS=1): exact SW/HW nonce
    counters on the device clock, plus CPU cycles busy-waiting on the SHA engine
  * otherwise the total "Khashes" of the ">>> Completed" lines on the host clock

If the firmware also logs NONCE lines (-D DEBUG_NONCE_LOG=1) they are validated
with tools/validate_nonces.py in the same run.

Usage:
  python tools/bench_hashrate.py --port /dev/ttyUSB0 --label O2 --csv bench.csv
"""

import argparse
import csv
import datetime
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from validate_nonces import Validator, read_serial  # noqa: E402

CPU_HZ = 240e6
RE_STATS = re.compile(r"STATS ms=(\d+) sw=(\d+) hw=(\d+) hw_wait_cyc=(\d+)")
RE_DONE = re.compile(r">>> Completed \S+ share\(s\), (\d+) Khashes")


def slope(points):
    """Least-squares slope of (x, y) points."""
    n = len(points)
    if n < 2:
        return None
    mx = sum(p[0] for p in points) / n
    my = sum(p[1] for p in points) / n
    sxx = sum((p[0] - mx) ** 2 for p in points)
    return sum((p[0] - mx) * (p[1] - my) for p in points) / sxx if sxx else None


def unwrap32(points):
    """Device counters are uint32: undo wrap-arounds."""
    out, offset, prev = [], 0, None
    for x, y in points:
        if prev is not None and y < prev:
            offset += 1 << 32
        prev = y
        out.append((x, y + offset))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--warmup", type=float, default=45, help="seconds after reset to ignore (default 45)")
    ap.add_argument("--duration", type=float, default=120, help="measured seconds after warm-up (default 120)")
    ap.add_argument("--label", default="", help="variant name for the report / CSV")
    ap.add_argument("--csv", help="append the result to this CSV file")
    args = ap.parse_args()

    v = Validator(quiet=True)
    start = time.time()
    stats, done = [], []
    for line in read_serial(args.port, args.baud, True, args.warmup + args.duration):
        v.feed(line)
        if v.stats["kat_fail"]:
            print("KAT FAILED - aborting benchmark", file=sys.stderr)
            sys.exit(2)
        if time.time() - start < args.warmup:
            continue
        m = RE_STATS.search(line)
        if m:
            stats.append(tuple(int(g) for g in m.groups()))
            continue
        m = RE_DONE.search(line)
        if m:
            done.append((time.time(), int(m.group(1))))

    if v.stats["kat_ok"] == 0:
        print("WARNING: no '[KAT] ... OK' line seen (old firmware or log started late)", file=sys.stderr)

    row = {"date": datetime.datetime.now().isoformat(timespec="seconds"), "label": args.label,
           "total_khs": "", "sw_khs": "", "hw_khs": "", "hw_wait_cyc_per_nonce": "", "hw_wait_pct": "",
           "samples": 0, "nonces_ok": v.stats["nonce_S_ok"] + v.stats["nonce_H_ok"],
           "nonces_bad": v.stats["nonce_S_bad"] + v.stats["nonce_H_bad"]}
    if len(stats) >= 3:
        sw = slope(unwrap32([(ms / 1000.0, s) for ms, s, _, _ in stats])) / 1000.0
        hw = slope(unwrap32([(ms / 1000.0, h) for ms, _, h, _ in stats])) / 1000.0
        dh = stats[-1][2] - stats[0][2]
        wait = (stats[-1][3] - stats[0][3]) / dh if dh > 0 else 0.0
        # share of one core's cycles the HW task spends busy-waiting
        wait_pct = 100.0 * wait * hw * 1000.0 / CPU_HZ
        row.update(total_khs=f"{sw + hw:.2f}", sw_khs=f"{sw:.2f}", hw_khs=f"{hw:.2f}",
                   hw_wait_cyc_per_nonce=f"{wait:.0f}" if wait else "",
                   hw_wait_pct=f"{wait_pct:.1f}" if wait else "", samples=len(stats))
    elif len(done) >= 3:
        row.update(total_khs=f"{slope(done):.2f}", samples=len(done))
    else:
        print("not enough samples - is the miner connected to the pool?", file=sys.stderr)
        sys.exit(2)

    print(" ".join(f"{k}={row[k]}" for k in row))
    if args.csv:
        new = not os.path.exists(args.csv)
        with open(args.csv, "a", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(row))
            if new:
                w.writeheader()
            w.writerow(row)
    sys.exit(1 if row["nonces_bad"] else 0)


if __name__ == "__main__":
    main()

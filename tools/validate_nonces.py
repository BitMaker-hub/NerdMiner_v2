#!/usr/bin/env python3
"""Validate NerdMiner nonces and shares on the host from the serial log.

Rebuilds the block header from the pool's own job data (extranonce1 from the
subscribe reply, mining.notify) exactly as a pool does, recomputes sha256d with
hashlib and compares it with what the device reported:

  * `NONCE ...` lines  - every batch's best 16-bit candidate, SW and HW path
                         (firmware built with -D DEBUG_NONCE_LOG=1)
  * `mining.submit` + `TX SHARE` - real shares (any build), incl. the 8-hex-digit nonce format
  * `[KAT]` lines      - startup self-test result

Usage:
  python tools/validate_nonces.py --port /dev/ttyUSB0  # live (COMx on Windows), Ctrl+C for summary
  python tools/validate_nonces.py --port /dev/ttyUSB0 --duration 300
  python tools/validate_nonces.py logs/device-monitor-*.log
  ... --en1 <hex>     extranonce1 if the log starts after the subscribe
  ... --quiet         print only problems and the summary

Exit code 1 if any mismatch or KAT failure was seen.
"""

import argparse
import hashlib
import json
import re
import struct
import sys
import time
from collections import Counter, OrderedDict

DIFF1 = 0xFFFF * 2 ** 208  # Bitcoin difficulty-1 target

RE_EN1 = re.compile(r"\bextranonce1: ([0-9a-fA-F]*)\s*$")
RE_RECV = re.compile(r"Receiving: (\{.*\})")
RE_SUBMIT = re.compile(r"Sending\s*: (\{.*\"mining\.submit\".*\})")
RE_TXSHARE = re.compile(r"TX SHARE: ([0-9a-f]{64})")
RE_SHAREDIFF = re.compile(r"Current diff share: (\S+)")
RE_NONCE = re.compile(r"NONCE src=(\S) job=(\S+) en2=([0-9a-fA-F]+) ntime=([0-9a-fA-F]+) "
                      r"nonce=([0-9a-fA-F]+) diff=(\S+) hash=([0-9a-f]{64})")
RE_KAT = re.compile(r"\[KAT\].*")


def dsha(b):
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()


def build_header(notify, en1, en2, ntime, nonce):
    _job_id, prevhash, coinb1, coinb2, branches, version, nbits = notify[:7]
    root = dsha(bytes.fromhex(coinb1 + en1 + en2 + coinb2))
    for branch in branches:
        root = dsha(root + bytes.fromhex(branch))
    prev = bytes.fromhex(prevhash)
    prev = b"".join(prev[i:i + 4][::-1] for i in range(0, 32, 4))
    return (bytes.fromhex(version)[::-1] + prev + root +
            bytes.fromhex(ntime)[::-1] + bytes.fromhex(nbits)[::-1] +
            struct.pack("<I", nonce))


def hash_diff(h):
    v = int.from_bytes(h, "little")
    return DIFF1 / v if v else float("inf")


class Validator:
    def __init__(self, en1=None, quiet=False):
        self.en1 = en1
        self.quiet = quiet
        self.jobs = OrderedDict()
        self.pool_diff = None
        self.pending_submit = None
        self.pending_diff = None
        self.stats = Counter()

    def out(self, msg, problem=False):
        if problem or not self.quiet:
            print(msg, flush=True)

    def job(self, job_id):
        return self.jobs.get(job_id)

    def feed(self, line):
        m = RE_KAT.search(line)
        if m:
            failed = "FAILED" in line
            if failed or line.rstrip().endswith(": OK"):
                self.stats["kat_fail" if failed else "kat_ok"] += 1
            self.out(m.group(0), problem=failed)
            return
        m = RE_EN1.search(line)
        if m:
            self.en1 = m.group(1)
            self.jobs.clear()
            self.out(f"[conn] extranonce1={self.en1}")
            return
        m = RE_NONCE.search(line)
        if m:
            self.check_nonce(*m.groups())
            return
        m = RE_SUBMIT.search(line)
        if m:
            try:
                self.pending_submit = json.loads(m.group(1))["params"]
            except (ValueError, KeyError):
                pass
            return
        m = RE_SHAREDIFF.search(line)
        if m:
            self.pending_diff = m.group(1)
            return
        m = RE_TXSHARE.search(line)
        if m and self.pending_submit:
            self.check_share(self.pending_submit, m.group(1), self.pending_diff)
            self.pending_submit = self.pending_diff = None
            return
        m = RE_RECV.search(line)
        if m:
            try:
                msg = json.loads(m.group(1))
            except ValueError:
                return
            if msg.get("method") == "mining.notify":
                params = msg["params"]
                self.jobs[params[0]] = params
                while len(self.jobs) > 64:
                    self.jobs.popitem(last=False)
            elif msg.get("method") == "mining.set_difficulty":
                self.pool_diff = float(msg["params"][0])

    def recompute(self, job_id, en2, ntime, nonce):
        notify = self.job(job_id)
        if notify is None or self.en1 is None:
            return None
        return dsha(build_header(notify, self.en1, en2, ntime, nonce))

    def check_nonce(self, src, job_id, en2, ntime, nonce_hex, dev_diff, dev_hash):
        nonce = int(nonce_hex, 16)
        h = self.recompute(job_id, en2, ntime, nonce)
        key = f"nonce_{src}"
        if h is None:
            self.stats[key + "_skip"] += 1
            self.out(f"SKIP     {src} nonce={nonce_hex} (job {job_id} or extranonce1 not in log)")
            return
        problems = []
        if h.hex() != dev_hash:
            problems.append(f"hash differs: host={h.hex()} device={dev_hash}")
        if h[30] or h[31]:
            problems.append("host hash fails the 16-bit pre-filter")
        diff = hash_diff(h)
        try:
            if abs(float(dev_diff) - diff) > 1e-6 * diff:
                problems.append(f"diff differs: host={diff:.9g} device={dev_diff}")
        except ValueError:
            problems.append(f"unparsable diff {dev_diff}")
        if problems:
            self.stats[key + "_bad"] += 1
            self.out(f"MISMATCH {src} job={job_id} nonce={nonce_hex}: " + "; ".join(problems), problem=True)
        else:
            self.stats[key + "_ok"] += 1
            self.out(f"OK       {src} nonce={nonce_hex} diff={diff:.6g}")

    def check_share(self, params, dev_hash, dev_diff):
        _worker, job_id, en2, ntime, nonce_hex = params[:5]
        problems = []
        if len(nonce_hex) != 8:
            problems.append(f"nonce '{nonce_hex}' is not 8 hex digits (pool may reject)")
        h = self.recompute(job_id, en2, ntime, int(nonce_hex, 16))
        if h is None:
            self.stats["share_skip"] += 1
            self.out(f"SKIP     share nonce={nonce_hex} (job {job_id} or extranonce1 not in log)")
            return
        if h.hex() != dev_hash:
            problems.append(f"hash differs: host={h.hex()} device={dev_hash}")
        diff = hash_diff(h)
        if self.pool_diff is not None and diff < self.pool_diff:
            problems.append(f"share diff {diff:.6g} below pool diff {self.pool_diff:g}")
        if problems:
            self.stats["share_bad"] += 1
            self.out(f"MISMATCH share job={job_id} nonce={nonce_hex}: " + "; ".join(problems), problem=True)
        else:
            self.stats["share_ok"] += 1
            self.out(f"OK       share nonce={nonce_hex} diff={diff:.6g} (device {dev_diff})")

    def summary(self):
        s = self.stats
        print("\n==== summary ====")
        print(f"KAT        ok={s['kat_ok']} failed={s['kat_fail']}")
        for src, name in (("S", "SW nonces"), ("H", "HW nonces")):
            print(f"{name:10} ok={s[f'nonce_{src}_ok']} mismatch={s[f'nonce_{src}_bad']} skipped={s[f'nonce_{src}_skip']}")
        print(f"shares     ok={s['share_ok']} mismatch={s['share_bad']} skipped={s['share_skip']}")
        bad = s["kat_fail"] + s["nonce_S_bad"] + s["nonce_H_bad"] + s["share_bad"]
        print("RESULT:", "FAIL" if bad else "PASS")
        return 1 if bad else 0


def read_serial(port, baud, reset, duration):
    import serial  # pyserial, shipped with PlatformIO's python
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, baud, 1
    s.dtr = False  # keep EN/IO0 released on open
    s.rts = False
    s.open()
    if reset:
        s.rts = True
        time.sleep(0.1)
        s.rts = False
    buf = b""
    end = time.time() + duration if duration else None
    while end is None or time.time() < end:
        buf += s.read(4096)
        *lines, buf = buf.split(b"\n")
        for line in lines:
            yield line.decode("ascii", "replace").rstrip("\r")


def read_files(paths):
    for path in paths:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                yield line.rstrip("\r\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("logs", nargs="*", help="log files (pio device monitor log2file output)")
    ap.add_argument("--port", help="serial port for live validation, e.g. /dev/ttyUSB0 or COM3")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--no-reset", action="store_true", help="do not reset the board when opening the port")
    ap.add_argument("--duration", type=float, help="live mode: stop after N seconds and print the summary")
    ap.add_argument("--en1", help="extranonce1 hex, if the subscribe reply is not in the log")
    ap.add_argument("--quiet", action="store_true", help="only print problems and the summary")
    args = ap.parse_args()
    if not args.port and not args.logs:
        ap.error("give --port or at least one log file")

    v = Validator(en1=args.en1, quiet=args.quiet)
    lines = (read_serial(args.port, args.baud, not args.no_reset, args.duration)
             if args.port else read_files(args.logs))
    try:
        for line in lines:
            v.feed(line)
    except KeyboardInterrupt:
        pass
    sys.exit(v.summary())


if __name__ == "__main__":
    main()

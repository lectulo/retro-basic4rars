import re
import subprocess
import sys
from pathlib import Path

def parse_readme(readme_path):
    status = {}
    pat = re.compile(r"^(P\d{3})\.BAS\s*-\s*(Passed|TCE)\s*-\s*(.*)$")
    for line in readme_path.read_text(errors="replace").splitlines():
        m = pat.match(line)
        if m:
            status[m.group(1)] = (m.group(2), m.group(3).strip())
    return status

def split_lines(data):

    lines = [line.rstrip(b" \t") for line in data.split(b"\n")]
    if lines and lines[-1] == b"":
        lines.pop()
    return lines

def normalize(data):
    return b"\n".join(split_lines(data))

_EXP_ZERO = re.compile(rb"E([+-])0(\d)\b")

def tokens(data):

    data = _EXP_ZERO.sub(rb"E\1\2", data)
    return data.split()

def common_prefix_lines(lines_a, lines_b):
    n = 0
    for x, y in zip(lines_a, lines_b):
        if x != y:
            break
        n += 1
    return n

def run_one(binary, bas_path, stdin_data, timeout=5):
    try:
        r = subprocess.run(
            [str(binary), str(bas_path)],
            input=stdin_data,
            capture_output=True,
            timeout=timeout,
        )
        return r.returncode, r.stdout
    except subprocess.TimeoutExpired:
        return None, b""

def main():
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} <build/basic> <test/nbs> <draft_ref_dir>",
              file=sys.stderr)
        return 2

    binary = Path(sys.argv[1])
    nbs_dir = Path(sys.argv[2])
    ref_dir = Path(sys.argv[3])
    run_output = ref_dir / "NBS_run_output"
    run_input = ref_dir / "NBS_run_input"

    status = parse_readme(nbs_dir / "README.NBS")

    buckets = {
        "match": [],
        "error_both": [],
        "no_recovery": [],
        "spacing_only": [],
        "mismatch": [],
        "no_reference": [],
        "tce": [],
    }

    programs = sorted(nbs_dir.glob("P*.BAS"))
    for bas in programs:
        name = bas.stem
        kind, desc = status.get(name, ("?", ""))

        stdin_path = run_input / f"{name}.in"
        stdin_data = stdin_path.read_bytes() if stdin_path.exists() else b""

        code, out = run_one(binary, bas, stdin_data)

        ref_path = run_output / f"{name}.run"
        ref = ref_path.read_bytes() if ref_path.exists() else None

        entry = {
            "name": name, "desc": desc, "code": code,
            "out": out, "ref": ref,
        }

        if kind == "TCE":
            buckets["tce"].append(entry)
        elif ref is None:

            if code == 1:
                buckets["error_both"].append(entry)
            else:
                buckets["no_reference"].append(entry)
        elif normalize(out) == normalize(ref):
            buckets["match"].append(entry)
        elif code == 1:

            our_lines = split_lines(out)
            prefix_ours = our_lines[:-1]
            if common_prefix_lines(prefix_ours, split_lines(ref)) == len(prefix_ours):
                buckets["no_recovery"].append(entry)
            elif tokens(out) == tokens(ref):
                buckets["spacing_only"].append(entry)
            else:
                buckets["mismatch"].append(entry)
        elif tokens(out) == tokens(ref):
            buckets["spacing_only"].append(entry)
        else:
            buckets["mismatch"].append(entry)

    total = len(programs)
    print(f"Всего программ: {total}")
    for key in ("match", "error_both", "no_recovery", "spacing_only", "mismatch", "no_reference", "tce"):
        print(f"  {key:14s} {len(buckets[key])}")

    print("\n--- ERROR_BOTH (оба сочли программу ошибочной) ---")
    for e in buckets["error_both"]:
        print(f"{e['name']}  {e['desc']}")

    print("\n--- NO_RECOVERY (известное отступление §10, не дефект) ---")
    for e in buckets["no_recovery"]:
        print(f"{e['name']}  {e['desc']}")

    print("\n--- MISMATCH (нужен ручной разбор) ---")
    for e in buckets["mismatch"]:
        print(f"{e['name']}  code={e['code']}  {e['desc']}")

    print("\n--- NO_REFERENCE, у нас код 0 (Ham отверг, мы приняли, либо тест не по ошибкам) ---")
    for e in buckets["no_reference"]:
        print(f"{e['name']}  code={e['code']}  {e['desc']}")

    return 0

if __name__ == "__main__":
    sys.exit(main())

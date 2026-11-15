"""Summarize Starfall benchmark CSVs; percentiles use the nearest-rank method."""
import csv
import math
import statistics
import sys
from pathlib import Path


def percentile(values, fraction):
    if not values:
        return None
    ordered = sorted(values)
    return ordered[max(0, math.ceil(fraction * len(ordered)) - 1)]


def summarize(path):
    with Path(path).open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    result = {"frames": len(rows)}
    for column in ("frame_ms", "update_ms", "submission_ms", "gpu_ms"):
        values = [float(row[column]) for row in rows if row[column]]
        result[column] = {
            "count": len(values), "missing": len(rows) - len(values),
            "median": statistics.median(values) if values else None,
            "p95": percentile(values, .95), "p99": percentile(values, .99),
        }
    return result


def main(paths):
    import json
    print(json.dumps({str(path): summarize(path) for path in paths}, indent=2))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("Usage: summarize-benchmark.py run.csv [run.csv ...]")
    main(sys.argv[1:])

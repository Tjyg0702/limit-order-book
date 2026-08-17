import re
import statistics
import sys
from pathlib import Path


PATTERNS = {
    "mean_ns": r"Mean:\s+([\d.]+)\s+ns",
    "p50_ns": r"p50:\s+(\d+)\s+ns",
    "p95_ns": r"p95:\s+(\d+)\s+ns",
    "p99_ns": r"p99:\s+(\d+)\s+ns",
    "p999_ns": r"p99\.9:\s+(\d+)\s+ns",
    "clock_overhead_ns": r"Clock overhead median:\s+(\d+)\s+ns",
    "peak_orders": r"Peak resting orders:\s+(\d+)",
}


def extract_values(text: str, pattern: str) -> list[float]:
    return [
        float(value)
        for value in re.findall(pattern, text)
    ]


def median(values: list[float]) -> float:
    if not values:
        raise ValueError("No benchmark values found")

    return statistics.median(values)


def main() -> None:
    if len(sys.argv) != 2:
        print(
            "Usage: python3 scripts/summarize_latency.py "
            "<latency-output-file>"
        )
        sys.exit(1)

    path = Path(sys.argv[1])

    if not path.exists():
        print(f"File not found: {path}")
        sys.exit(1)

    text = path.read_text()

    results = {
        name: extract_values(text, pattern)
        for name, pattern in PATTERNS.items()
    }

    run_count = len(results["p50_ns"])

    if run_count == 0:
        print("No latency benchmark runs found.")
        sys.exit(1)

    print(f"Runs analyzed: {run_count}")
    print()

    print("Median-of-runs latency summary")
    print("--------------------------------")
    print(
        f"Clock overhead: "
        f"{median(results['clock_overhead_ns']):.0f} ns"
    )
    print(
        f"Mean latency:   "
        f"{median(results['mean_ns']):.3f} ns"
    )
    print(
        f"p50:            "
        f"{median(results['p50_ns']):.0f} ns"
    )
    print(
        f"p95:            "
        f"{median(results['p95_ns']):.0f} ns"
    )
    print(
        f"p99:            "
        f"{median(results['p99_ns']):.0f} ns"
    )
    print(
        f"p99.9:          "
        f"{median(results['p999_ns']):.0f} ns"
    )
    print(
        f"Peak orders:    "
        f"{median(results['peak_orders']):.0f}"
    )


if __name__ == "__main__":
    main()
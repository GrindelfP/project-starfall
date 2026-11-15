import importlib.util
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("summary", Path(__file__).resolve().parents[1] / "scripts" / "summarize-benchmark.py")
summary = importlib.util.module_from_spec(spec)
spec.loader.exec_module(summary)


class SummaryTest(unittest.TestCase):
    def test_percentiles_and_missing_gpu(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.csv"
            path.write_text("frame,frame_ms,update_ms,submission_ms,gpu_ms\n1,1,2,3,0.2\n2,3,4,5,\n3,9,6,7,0.4\n", encoding="utf-8")
            result = summary.summarize(path)
        self.assertEqual(result["frame_ms"]["median"], 3)
        self.assertEqual(result["frame_ms"]["p95"], 9)
        self.assertEqual(result["frame_ms"]["p99"], 9)
        self.assertAlmostEqual(result["gpu_ms"]["median"], 0.3)
        self.assertEqual(result["gpu_ms"]["missing"], 1)

    def test_empty_data(self):
        self.assertIsNone(summary.percentile([], .95))


if __name__ == "__main__":
    unittest.main()

import ast
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class IntegrationContractTests(unittest.TestCase):
    def test_python_syntax(self):
        ast.parse((ROOT / "service.py").read_text(encoding="utf-8"))

    def test_plugin_contract(self):
        doc = json.loads((ROOT / "openclaw-plugin/openclaw.plugin.json").read_text())
        self.assertEqual(doc["id"], "akari-langgraph")
        self.assertEqual(set(doc["contracts"]["tools"]), {
            "akari_delegate", "akari_job_status", "akari_resume",
        })

    def test_only_loopback_ollama(self):
        source = (ROOT / "service.py").read_text()
        self.assertIn("http://127.0.0.1:11434/api/generate", source)
        self.assertNotIn("OLLAMA_HOST", source)


if __name__ == "__main__":
    unittest.main()

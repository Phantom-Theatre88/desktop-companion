import unittest
from canon_guard import CanonError, CURRENT, evaluate, fetch_text


def fixtures():
    return {
        CURRENT["architecture"]: (
            "設計方針LOCK OpenClaw LangGraph "
            "製品採用の最終確定 最終OS、ハードウェア 未決定"
        ),
        CURRENT["goal"]: "Brain Ghost Character",
        CURRENT["stack"]: "| Brain | **Python** | ローカルWebSocket",
        CURRENT["name"]: "統合ロボットの正式名称：AKARI",
    }


class CanonGuardTests(unittest.TestCase):
    def test_current_akari_scope(self):
        result = evaluate(fixtures(), list(CURRENT.values()))
        self.assertEqual(result["facts"]["final_hardware"], "undecided")
        self.assertEqual(result["facts"]["final_openclaw_langgraph_adoption"], "undecided")

    def test_missing_required_canon_blocks(self):
        data = fixtures()
        del data[CURRENT["architecture"]]
        with self.assertRaises(CanonError):
            evaluate(data)

    def test_legacy_pi5_does_not_override_new_scope(self):
        data = fixtures()
        data["LOCK55_Pi5高次認知系の独立実装.md"] = "Raspberry Pi 5"
        self.assertEqual(evaluate(data)["facts"]["final_hardware"], "undecided")

    def test_unreviewed_new_akari_lock_blocks(self):
        with self.assertRaises(CanonError):
            evaluate(fixtures(), list(CURRENT.values()) + ["LOCK_AKARI_NEW_2026-11-15.md"])

    def test_changed_stack_blocks(self):
        data = fixtures()
        data[CURRENT["stack"]] = "| Brain | **JavaScript** |"
        with self.assertRaises(CanonError):
            evaluate(data)

    def test_reject_traversal(self):
        with self.assertRaises(CanonError):
            fetch_text("docs/sacred/../../secrets")


if __name__ == "__main__":
    unittest.main()

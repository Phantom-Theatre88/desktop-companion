# LOCK｜新デスクトップAI 第一技術構成定義（2026-11-09）

- 状態：**第一定義 LOCK（初期実装の基準構成。最終採用・性能検証完了を意味しない）**
- 対象：Brain / Ghost / Character の新デスクトップAI
- 正本：GitHub `Phantom-Theatre88/desktop-companion` の `heart-engine` ブランチ、`docs/sacred/LOCK_DESKTOP_AI_INITIAL_TECH_STACK_2026-11-09.md`
- 控え：Dropbox `/新ロボット/01_聖典/LOCK_DESKTOP_AI_INITIAL_TECH_STACK_2026-11-09.md`

## 1. 第一技術構成【LOCK】

| 柱・接続 | 第一候補・初期実装基準 |
|---|---|
| Brain | **Python** |
| Ghost（Heart / Memory / Time / Relationship / Behavior） | **Python** |
| Character（3D表示・表情・視線・動作） | **Godot**（スクリプトは必要に応じてGodotの標準機能を使う。言語の詳細は未確定） |
| 三本柱の連携 | **ローカルWebSocket** |

本定義は、上記構成を**最初に実装・検証する技術基準**としてLOCKする。類似案との比較のたびに初期方針へ戻らない。重大な不適合が検証で判明した場合は根拠・影響・改善案を提示し、Kimの承認後に変更する。

## 2. 既存聖典との関係

- 上位目的、三本柱の役割分担、Ghostの五中核と既存LOCKを変更しない。
- `LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md` v1.2 の「技術未決定」に対し、本書は**第一実装構成のみ**を追加決定する。完成機種、OS、LLM、通信メッセージ形式、DB、3Dファイル形式、セキュリティ制御の具体方式を確定しない。
- Python／Godot／WebSocketの採用理由や負荷は、実機・統合試験で確認する。未検証の実用性能を保証しない。
- GhostはBrainとは別の責務を維持し、Brainが停止しても基本活動を継続できることを目指す。**別プロセス構成は現時点の実装提案であり、独立したPCの使用やOS選定をLOCKしない。**
- 旧Arduino/C++のGhostコードは設計・挙動の参考資産であり、この第一定義によって旧実装の削除・廃止を行わない。

## 3. 次の小さな完成

- Brainのテスト入力 → Ghostの独立した処理 → Character用のテスト出力を、**ローカルWebSocketで一往復**させる最小統合テストを作る。
- この試験で責務境界、Brain停止時のGhost継続、再接続とエラー時動作を確認する。Characterの3Dモデル完成や本格LLMの導入は前提条件にしない。
- 実行・保存・Pushは実結果で検証する。

**LOCK範囲：Brain＝Python、Ghost＝Python、Character＝Godot、接続＝ローカルWebSocketを「第一実装構成」とすること。その他の技術詳細は未決定。**


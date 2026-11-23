# LOCK｜AKARI v0.5 STEP 2・3 成果保存チェックポイント（2026-11-23）

- **区分**：Kimによる「save・push・LOCK」指示で固定した**作業成果・検証状態・再開地点**。
- **意味**：成果物の版と検証事実を固定する。**三本柱・Ghost/NERVAの既存LOCKを変更しない**。試作の通信スキーマ、人格、学習式を正式採用した意味ではない。
- **基準**：`docs/sacred/LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md` v1.2、`docs/sacred/LOCK_CHARACTER_MOTOR_CONTROL_ARCHITECTURE_2026-11-12.md` および2026-11-23接続監査LOCK。

## 固定する成果の実体

1. **GitHubのソース正本（試験実装）**：ブランチ `codex/akari-v05-autonomy-step2-3-20261123`、`experiments/akari_v05_step2_3/`。Python4本、GDScript3本、統合テスト1本、README1本の**9ファイルをPushし、GitHubから9/9件の読み戻し照合が一致**。
2. **実モデルを含む配布用検証ZIP**：ChatGPT Library `/ロボット/AKARI_v05_STEP2_STEP3_自発行動_20261123_検証版.zip`（21,343,717 bytes）。15日版をベースに組み直した検証成果であり、元版を上書きしていない。
3. **元の保存資産**：ChatGPT Library `/ロボット/AKARI_Ghost_NERVA_Character_検証版.zip`（21,323,290 bytes）は保護して維持。
4. **検証報告**：ChatGPT Library `/ロボット/AKARI_v05_STEP2_STEP3_検証報告_20261123.md` とNotion [監査LOCK・進捗ページ](https://app.notion.com/p/421765a919dc81128333ec012b78eaea)。
5. **3Dモデルの同一性**：作業版GLBのSHA-256は `2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d` として記録。

## 現時点の確認済み／未確認（LOCK：誤って完了扱いしない）

| 境界 | 実装・検証状態 |
| --- | --- |
| v0.5実体の保護・再開基準 | 15日版のZIP実体確保、別作業版保存 |
| Ghost Memory／Relationshipの証拠取得 | `behavior_bridge.py` が既存の経験と関係根拠を参照できる |
| Relationshipを使った本格的な行動変化 | **未完成**：人格・関係スコアを独断決定しない |
| Ghost→NERVA→Ghost の結果応答 | 送信・sequence照合・重複拒否・失敗時idle退避を試験実装。模擬Character WebSocketで往復確認 |
| Character v0.5の脚6ボーン | 11月16日記録から小さな荷重移動を**機能再構成**。同日の元ZIPとバイト同一と断定しない |
| Python自動テスト | **7件合格**（以前のテスト実行記録。GitHubへの保存作業で再実行したわけではない） |
| Mac／Godot実画面・実骨格の動作／足裏接地・自然さ・かわいさ | **未検証** |
| Ghost経験→行動選択→身体実動作→結果→次の選択の全循環 | **未完成** |

## 再開時の優先順（LOCK）

1. **既存v0.5を消さない**。上記ZIPとGitHubの実験ブランチを試験成果正本として再利用。GitHubの古いv0.4をv0.5へ上書きしない。
2. Godot 4でv0.5実体・GDScriptを実行し、通信・骨格・姿勢・視線・呼吸・表情・足裏の挙動を確認する。
3. MemoryとRelationshipの実際の経験根拠がBehavior選択を変える経路を、旧LOCKを読み込んで実装・検証する。
4. NERVAの返信が画面上の実動作と対応することを検証し、その結果がGhostの経験として次の判断に反映されるまでつなぐ。
5. 完了条件を満たすまでは、**STEP 2・3は部分実装／通信試験合格**と表示する。Viewer増設・別機能より上記循環の完成を優先する。

## 参照リンク

- [GitHub試験ソース](https://github.com/Phantom-Theatre88/desktop-companion/tree/codex/akari-v05-autonomy-step2-3-20261123/experiments/akari_v05_step2_3)
- [2026-11-23 自発行動・接続監査LOCK](https://github.com/Phantom-Theatre88/desktop-companion/blob/heart-engine/docs/sacred/LOCK_AKARI_AUTONOMOUS_CONNECTION_AUDIT_2026-11-23.md)
- [Notion進捗記録](https://app.notion.com/p/421765a919dc81128333ec012b78eaea)

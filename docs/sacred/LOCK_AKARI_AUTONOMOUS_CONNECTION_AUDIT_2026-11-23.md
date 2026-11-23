# AKARI 自発行動・接続監査と実装優先方針 LOCK（2026-11-23）

**区分**：Kimの承認による監査結果・実装優先順位の記録LOCK。既存LOCKや人格・通信仕様を変更しない。
**監査基準**：GitHub heart-engine `3626ab9e760644627f69696a8784c2ddd8d24562`、三本柱 v1.2／NERVA正式聖典、Notionの2026-11-18追加決定。
**検証範囲**：コード・既存テスト記録の照合のみ。今回Mac/Godot実画面・最新v0.5 ZIPを実行したわけではない。

## 最上位ゴールの継承【LOCK】
AKARIが誰からも個別の動作を指示されず、Ghostの状態・時間・経験・関係性・状況から自ら行動を選択し、Character内部のNERVAが実際の身体を動かし、結果がGhostの次の判断に還流する相棒になる。Brainの事実・推論・プロジェクト記憶とGhostの経験・感情を混同しない。三本柱とGhost五中核を維持する。

## 接続監査（実装と実証を混同しない）
| 境界 | コード・記録から確認した事実 | 不足・未検証 |
| --- | --- | --- |
| Ghost Heart/Time→Behavior | `integration/ghost_nerva/ghost_auto.py` に旧CoreS3由来6状態と `idle/scan/drowsy/sleep` の時間選択あり | 五中核Ghostの完成ではない |
| Memory→Behavior | `experiments/ghost_pc_spike/ghost_core.py` に経験参照・記録あり | 行動選択への経験反映なし |
| Relationship→Behavior | テストデータの関係根拠境界あり | 関係評価・行動への反映未実装 |
| Ghost→NERVA | `intent_server.py` がローカルWebSocketで `ghost_intent` / `ghost_snapshot` 送信。 `ghost_intent_receiver.gd` に受信コードあり | 形式は試験用。v0.5実画面で今回未検証 |
| NERVA→Character | v0.4参照コードに首・視線・胸・腕・呼吸・瞬きの連続制御あり | v0.4をv0.5に上書き禁止 |
| v0.5 | `character/akari_v05_nerva/STANCE_STAGE1_2026-11-16.md` に46ボーンモデル・脚6ボーン荷重移動・11試験通過の記録 | 本体は別ZIP。足裏接地、Mac実画面、自然さ・かわいさは未検証 |
| NERVA→Ghost | 動作完了・失敗の応答コードを監査対象経路で確認できない | 行動結果のフィードバック未接続 |
| Viewer | 独立診断機能あり | 模擬操作はAKARIの自発性ではない |

## 実装の優先方針【今回LOCK】
1. Viewerの拡張や新しい数値・機能の追加より、**Memory・Relationship→Behavior→NERVA→Character→Ghostへの結果返却**という循環の完成を最優先する。
2. v0.5実モデル・専用スクリプトの版と所在を確認し、既存成果を保護する。v0.4参照コードで上書きしない。NERVAの連続動作生成をゼロから作り直さない。
3. Ghostが行動意図を選び、NERVAが関節・姿勢・視線などの具体的動作を生成する責務を維持する。定型モーション番号の再生だけを完成形としない。
4. 送信、受信、身体実動作、結果返却、次の判断への反映を別々に検証し、未実施を成功と呼ばない。
5. 正式LOCKの変更、人格係数や通信スキーマの独断確定はしない。Brain=Python・Ghost=Python・Character=Godot、第一接続=ローカルWebSocketの既定方針を維持する。
6. 各作業で自発行動の完成にどう寄与するかを明示し、方向のズレは先に報告する。

## 再開地点・完成条件
1. Dropbox/配布ZIPの最新Character v0.5とGitHubテキストソースを差分照合する（原本非変更）。
2. GhostのMemory・RelationshipからBehaviorへの判断材料の通り道を実装・検証する。
3. NERVAから動作開始・完了・失敗の結果をGhostへ返す最小経路を実装・検証する。
4. v0.5実モデルで、人の操作なしに行動を選ぶ→身体が動く→結果により次の行動が変わることをMacの画面とログで確認する。

**完成は未宣言**。現在の監査のみで以上の循環が完成したとは判定しない。

## 正本出典
- `docs/sacred/LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md` v1.2
- `docs/sacred/LOCK_CHARACTER_MOTOR_CONTROL_ARCHITECTURE_2026-11-12.md`
- `integration/ghost_nerva/README.md` / `VIEWER.md` / Python・GDScript実装
- `character/akari_v05_nerva/STANCE_STAGE1_2026-11-16.md`
- Notion「設計の追加決定_2026-11-18」

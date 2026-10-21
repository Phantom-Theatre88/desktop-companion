# Ghost Learning A — Step 1 整合性監査（2026-10-22）

状態：**Step 1（既存LOCK・現行コードの差分特定）完了**。ビルド・実機試験およびLearning A全体の完成を意味しない。
対象：`heart-engine` ブランチ、2026-10-21のGhost三聖典、`firmware/cores3_base/src/ghost/`。既存の実装Step聖典の番号を変更しない。本書のStep 1は今回合意したLearning A作業内の番号。

## 正本と判定原則
- 五中核 Heart / Memory / Time / Relationship / Behavior は維持。Learning Aは横断機能であり第六中核にしない。
- Memory：経験・当時の感情・後からの解釈・想起。Relationship：Memoryを根拠に相手との関係を評価。Heart：現在の状態と学習で変化する平常傾向。Behavior：それらを踏まえ行動を選択。
- 「過去を参照 → 今を感じる → 今回の経験を記録」は論理上の依存関係。既存LOCK 35のイベント開始時HeartContextの不変性は維持し、関数の実行順序と混同しない。
- 危険反射優先、CoreS3単独稼働、Device→Adapter→Nerve Inputを維持。
- 2026-10-21の新しい時間LOCKにより、再起動自体を感情リセット条件としない。電源断中の経過時間に応じて回復させる。

## 現行コードとの照合
| 項目 | 判定 | 根拠・差分 |
| --- | --- | --- |
| 五中核の生成と呼出し | 基盤あり | GhostCore::begin/onNeuron/tick |
| 固定したイベント開始時HeartContext | 実装あり | GhostCore::onNeuron。更新前snapshotを各Engineへ共有 |
| イベント前後のHeartの区別 | **部分実装・未検証** | MemoryRecordのheart_context/outcome_contextとhas_outcome。前回の3ファイル変更 |
| 過去経験の参照 | 未実装 | similar_prior_eventは「直前の同種NeuronType」だけ。検索や想起ではない |
| Memoryの経験蓄積・後からの解釈 | 未実装 | last_candidate_一件は上書きされ、履歴・追記・永続化がない |
| Relationship→Memory参照 | 未実装 | RelationshipEngineのメソッドは空処理 |
| Heart←経験・関係性 | 未実装 | HeartEngine::onNeuronはevent_contextを未使用、固定デルタ。baseline固定 |
| MoodとComfort、SocialとAffectionの独立 | 設計LOCK・未実装 | HeartStateは既存6項目。追加フィールドや係数は未決定 |
| Behavior←Memory・Relationship | 未実装 | Behavior::tickはHeartContextのみ |
| Timeの電源断中経過時間 | 未実装 | TimeEngineはlast_tick_msのみ |
| Learning Aの保存・復元 | 未実装 | HeartのNVSは一部あり、Memory・学習結果の一貫した保存復元なし |
| Reflexの安全優先 | 既存構造を維持 | GhostCore::onReflexResultほか。今回の監査で実機再検証はしていない |

## 前回の3ファイル変更の技術判定
1. **採用可能な基礎**：イベント前HeartContextと反応後HeartContextを区別することは、LOCK 35および当時の感情を残すLOCKと両立する。
2. **実装上の不足**：Memory::onNeuronが更新前の感情を受け取る一方、現在の呼出順はHeart更新→Memory登録。現状は旧情報を明示的に渡しているため破綻しないが、経験参照を追加する際に「今回のイベントを過去として読む」事故を防ぐため、参照時点を明示する必要がある。
3. **意味の過大解釈禁止**：similar_prior_eventは直前の同種入力フラグであり、過去経験の類似検索ではない。学習の根拠にそのまま使わない。
4. **記録の上書きリスク**：last_candidate_は次イベントや反射結果で上書きされる。長期記憶・イベント単位の確定記録とみなさない。
5. **時刻の区別**：SemanticNeuron::timestamp_ms（発生時刻）とHeartContext::captured_ms（処理時刻）の意味を維持。電源断をまたぐ実時間とは別物。
6. **未検証**：この3ファイルのコミットについて、ビルド・CoreS3実機動作を確認した証拠はない。完成扱いしない。

## 次のStepへの引継ぎ（再質問しない）
- Step 2：Memoryの「既存経験の参照」と「今回の経験の確定記録」を分離し、参照結果と前後HeartContextをイベント単位で結び付ける。記憶の保存媒体・容量・忘却式など、聖典で未決定の事項を独断でLOCKしない。
- Step 3：RelationshipはMemoryの経験を根拠に評価し、事実の重複保存を避ける。
- Step 4：Heartが経験・関係性・状況を判断材料に使えるようにする。Mood/Comfort/Social/Affectionの概念区分を崩さない。
- Step 5：BehaviorがHeartだけでなく経験・関係性を反映する。
- Step 6：時間・永続化・再起動復元を検証する。時間回復と学習結果の保持を混同しない。

## 完了判定
- [x] 現行コードのデータフローと三聖典を照合
- [x] 既存LOCK、実装済み、未実装、未決定を区別
- [x] 前回コミットの部分実装と危険箇所を明記
- [x] Step 2以降への引継ぎを固定
- [ ] ビルド・実機試験（Step 1の監査範囲外、実装検証時に必須）

この監査では新しい数式・保存方式・データ所有権を確定せず、既存コードも変更しない。

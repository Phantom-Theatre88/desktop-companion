# AKARI v0.5 生命感ステージ｜通信不具合修正 2026-11-23

## 正式LOCKと完成条件
正式LOCKの優先順位は `Memory・Relationship→Behavior→NERVA→Character→Ghost` の実行結果循環。既存LOCK・人格・三本柱は変更しない。これは実3Dでの自然な動作完了を宣言するものではない。

## 修正箇所（前試験版で見つかった根本問題）
1. `ghost_intent_receiver.gd` が `explore/sit/rise` を受け付けず、Ghost試験モードの全身動作指示を捨てていた。受信許可リストを修正。`state_sync` は相変わらず自発行動の完了証拠として扱わない。
2. `behavior_bridge.py` が `applied_frame` を最終結果扱いし、後続の `completed` を拒否していた。進捗 `progress` と確定結果 `results` を分離。初回の骨格への適用は進捗、実際のNERVA完了通知は別の経験として保存し、これを受けてのみGhost試験モードが次の行動に進む。
3. 重複した `applied_frame`、重複・矛盾する終端通知、未バインドの偽 `completed` を拒否し、完了を捏造しない。経験は既存SQLiteジャーナルに記録する。

## 証拠
- Python 29/29テスト合格（既存25+追加4）。WebSocket実サーバー／模擬クライアントの統合試験で `explore→applied_frame→completed→sit→applied_frame→completed→rise→applied_frame→completed→idle` を検証。完了前の早送りを拒否。
- ただし模擬クライアント試験は実キャラクターが歩いた証拠にはならない。
- 元GLBのSHA-256 `2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d` を維持。

## 未完成
- Mac Godot画面での足裏接地・着座・自然な歩行は未検証。
- この試験の `explore→sit→rise` はGhost Time内の仮の試験系列であり、Memory／Relationshipの実経験から自律選択する本物のBehaviorは未実装。
- 手の90%縮小は可逆的なモデル表示試験であり、3D造形としての完了確認はまだない。
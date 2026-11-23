# AKARI v0.5 STEP 4 継続：経験の永続保存・再起動後想起の試験実装

**位置づけ：開発ブランチの実験的な保存方式。正式LOCK、人格学習式、プロトコルの採用決定ではない。**

## 既存LOCKとの整合

- GhostはHeart/Memory/Time/Relationship/Behaviorを保持する。Brainの長期プロジェクト記憶とは混同しない。
- Memoryが経験した出来事、当時のHeart、解釈を保持。Relationshipは記録のうちactor_idが実際に付与された事実だけを読み、相手を推定して作らない。
- 電源ON/OFF自体を感情のリセットとする設計にはしない。ただし今回の試作は**現時点のHeartを再起動後に復元する方式まで実装していない**。時刻未確認状態で前回の気分を再現したと偽らない。
- Learning Aの変化係数や人別行動選択ルールは未決定であり、ここでは実装しない。

## 実装した差分

- `ghost_server/experience_journal.py`：stdlib SQLite上のappend-only体験ログ。SemanticEventと記録時のHeart before/after、解釈をイベントID単位で保持。既存イベントの上書き防止。
- `ghost_server/behavior_bridge.py`：起動時にMemoryへ記録済みの経験を復元。NERVA結果の記録はDBコミット後にメモリーへ反映。保存エラー時は`ghost_feedback`成功扱いをしない。`run_id`によって複数回の起動時のシーケンス衝突を防ぐ。
- `ghost_server/intent_server.py`：テスト運用向け`--journal /path/to/experiences.sqlite`オプション。デフォルトのPython API呼出しは任意のまま。
- `起動_AKARI.command`：同梱検証版をMacで起動した場合は`ghost_server/state/experiences.sqlite`に経験を蓄積。同フォルダで再起動すると記録を読み戻す。
- `tests/test_experience_journal.py`：再起動、本人が未特定の記録、元のHeartの不変性、重複防止、書込みエラー、WebSocket経由再起動を検証。

## 事実と限界

- Python自動テスト **17件合格**（従来12＋追加5）。
- 模擬Characterの`applied_frame`受信を事実として保存できる。実3Dの見た目・ボーンの連続動作・足裏の接地や可愛らしさは**Godotで未検証**。
- Relationshipからは根拠となる経験を参照できるが、特定の人に対する親密度や信頼度を計算する学習Aの式は未実装。
- Ghostの現在のHeart値を電源断中の経過時間に応じて回復させる処理、頻繁な経験の忘却、データ破損からの復旧、学習結果の永続化、Brainの情報保護・統合は未完成。
- `applied_frame`は「3Dモデルがフレーム処理に到達」の試験結果であり、身体が自然に動いたことの証明ではない。

## 最短の次工程

1. 保存試作の結果を、未決定の性格ルールと混同せず継続使用する。
2. Godot4でGDScript構文・GLBロード・状態同期・NERVA実フレームを検証。
3. 既存LOCKに沿ってLearning Aの最小更新・復帰を別試験で設計・検証し、実際に経験が次の行動にどう影響したかを観測できるようにする。
4. Macの実画面では脚の接地・視線・仕草・可愛らしさを別判定する。
# AKARI 最小統合試験：Ghost → NERVA → Character

状態：**試作／正式LOCKは変更しない**。WORLDの高度化や専用PC購入は不要。

### 2026-11-15 実モデル試験（重要）

Dropboxに**Character v0.5**（46ボーン・改良された視線／呼吸／指の待機動作）が存在することを確認。したがって、このディレクトリにある古い `character/akari_v04_nerva` をそのままv0.5の代用にしてはいけない。

v0.5の`main.gd`・`nerva_controller.gd`を基準にした複製検証パッケージはChatGPT Library「ロボット／AKARI_Ghost_NERVA_Character_検証版.zip」へ保存。Character v0.4の同じ3Dモデルを含む。Godot 4.6.3のヘッドレス試験で **GLB実モデルのメッシュ3件読込と、Ghostの自発意図 `idle → drowsy → sleep` をNERVAが受信**することを確認した。画面での自然さやMac実機は未評価。正式LOCKと元Dropbox版は変更していない。

## 目的

既存Ghostのイベント記録を保持しつつ、意味を持つ試験用意図だけをWebSocketでNERVAへ送る。NERVAはその意図をもとに身体の動きを生成し、Character v0.4の3D身体を動かす。**Ghostは首の角度・アニメーション番号を指定しない。**

既存CoreS3版のGhostにある**無刺激時の退屈・眠気の蓄積と、自発行動の選択**だけをPythonへ移した最小互換試作も含めた。手入力なしで、時間経過により「眠そうにする→眠る」の意図を送る（既存C++の判定順序では、無刺激時は周囲を見るより先に眠気が勝つ）。係数は既存`HeartEngine.cpp`・`BehaviorEngine.cpp`の実装値を参照しており、正式な新しい性格仕様ではない。現行Python版Ghostの完全移植ではない。

## テスト方法

1. リポジトリの`heart-engine`ブランチを取得。
2. Python 3.11以上で、`python3 -m pip install 'websockets>=12,<17'`。
3. `python3 -m unittest discover -s integration/ghost_nerva -v` で契約試験。
4. `python3 integration/ghost_nerva/intent_server.py` でローカル送信機を起動。
   - 端末入力なしで常時試験するなら `python3 integration/ghost_nerva/intent_server.py --headless`（Ctrl+Cで停止）。
   - 時間経過テストを短縮する場合は `python3 integration/ghost_nerva/intent_server.py --time-scale 60`（**検証専用**。実生活の時間加速設定ではない）。
5. Dropboxに保存したCharacter v0.4のGodotプロジェクトを**別フォルダへ複製**。
6. その複製にGitHub `character/akari_v04_nerva/scripts/main.gd` と `nerva_controller.gd` を適用。元版は変更しない。
7. `integration/ghost_nerva/ghost_intent_receiver.gd` を複製したGodotプロジェクトの`res://scripts/`に置き、**既存メインNode3Dの子Nodeにアタッチ**。
8. Godotで起動して、Python端末に`near`、`quiet`、`blink`、`status`を入力。

期待：手入力なしでも、時間経過によって`drowsy`・`sleep`に変わる。`near`でNERVAが小さく顔を傾ける、`quiet`で待機動作が落ち着く、`blink`で瞬く。通信断ではNERVA自身の基本待機を継続し、3秒ごとに再接続を試す。既存の元モデル・骨格・表情・衣装は変更しない。

## 合格条件と制限

- Python契約試験：メッセージに角度指定が含まれず、既知の意図のみを送る。
- Godot試験：実際のAKARIモデルで上記3反応と通信断からの復帰を画面確認する。
- **未完成**：Ghost全体の自発判断（現状はC++実装の無刺激時ロジックの部分移植のみ）、永続記憶、Brain会話、音声、WORLD、実機統合確認。

この試験が成功したら、Python版Ghost全体との正式接続へ進む。新しい性格・感情の係数は本試験では確定しない。

## 2026-11-15｜次のGhost→NERVA統合（最新進捗・従来版は保存）

**目標はViewerの完成ではなく「AKARIのかわいさ・生命感・GhostとNERVAの自然な連携」。** Viewerは現象を観察する補助ツールに限る。既存LOCKは変更しない。

### 最新コードで追加したもの

- `experiments/ghost_pc_spike/ghost_core.py`：今回の経験をMemoryへ確定する**前**に、同種の過去経験とRelationshipの根拠（相手が明示されている場合のみ）を取得する`remember_then_record`。当時のHeart前後snapshotと事実を保持する。**行動の学習や人格の成長式は実装していない**。
- `integration/ghost_nerva/intent_server.py`：試験入力をその新しい経路で記録。記録前に何件を思い出したかを診断用snapshotへ添付。Ghostの退屈・眠気の現在値を1秒ごとに送信（`source=ghost_autonomy_compatibility_spike`と明示）。
- `integration/ghost_nerva/ghost_intent_receiver.gd` と `character/akari_v04_nerva/scripts/nerva_controller.gd`：`ghost_snapshot` の`boredom/sleepiness`をNERVAへ渡し、**視線探索の間隔と待機動作の強さに連続的に反映**。Ghostはボーン角度を決めない。受信途絶時は試験値を解除。睡眠中も呼吸を保ち、起床時は閉眼から再び開く。

### v0.5実モデル配布について

Godotのv0.5実モデルと肌色修正版を基に、上記連携を適用した**ChatGPT会話添付ZIP** `AKARI_Ghost_NERVA_Continuous_State_検証版.zip` を作成した。v0.5のGodotスクリプトは上記v0.4 GitHub版とは別物なので、v0.4スクリプトでv0.5の実装を上書きしないこと。ZIPにはv0.5専用の更新済みNERVA/receiverとPython Ghost試作を同梱する。GitHubの`heart-engine`はテキストソース正本、ZIPは実モデル同梱の検証配布物と区別する。

**確認済み**：同梱Pythonの経験参照→記録／Wake→sleep復帰／Viewer入力／WebSocketでの継続状態配信をローカルPython統合試験で通過。GLB・project.godot・VRM表情マップが肌色補正版とハッシュ一致。

**未確認**：最新版ZIPでのGodot画面描画とMac実機の自然さ・かわいらしさ。Learning Aの長期保持、Relationship評価、心の状態が経験を踏まえて行動選択を変える処理、BrainやWORLDとの一体動作。既存の「退屈・眠気」はCoreS3互換試作であり、五中核Ghost全体の完成とみなさない。

**次の判断基準**：AKARIの視線・呼吸・待機動作の見た目がGhost状態に応じて自然に変わるか。見え方を実機で確かめてから、Memory/Relationship根拠→Behavior→NERVAの実動作経路を拡げる。ViewerのUI拡張は最優先にしない。


## 2026-11-16｜聖典再監査：Mac版GhostにCoreS3既存6状態を部分移植

- 根拠：`docs/sacred/LOCK_GHOST_MOOD_COMFORT_SOCIAL_2026-10-21.md`、`LOCK_GHOST_EXPERIENCE_MEMORY_INTERPRETATION_TIME_2026-10-21.md`、`LOCK_GHOST_LEARNING_A_B_2026-10-21.md`、`LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md`、`LOCK_CHARACTER_MOTOR_CONTROL_ARCHITECTURE_2026-11-12.md`、既存 `HeartEngine.cpp`／`BehaviorEngine.cpp`。
- 旧 `GhostAutonomy` の2項目のみでは不十分だったため、CoreS3 `HeartState` の6項目（mood / affection / curiosity / boredom / sleepiness / attention）と初期値、無刺激時・WAKE_WORD_DETECTED／TOUCHの単発差分、平常値への既存回復式をMac互換試作へ追加。
- `intent_server.py` の `ghost_snapshot.autonomy` に6項目を載せ、`state_origin=CoreS3_HeartState_6_partial_port`で出所を明示。Viewerには6項目を表示する。Comfort／Socialは独立した概念としてLOCK済みだが**数値と更新式は未決定／未実装**。8つの仮数値を作らない。
- Viewerで「触れられた（模擬入力）」を行うと、既存CoreS3の `TOUCH` に相当する単発変化が起きる。実センサー入力ではない。呼びかけ・触覚はViewer試験と明示し、Ghost自発判断に混ぜない。
- Character内のNERVAは好奇心・注意の実数値から既存の視線探索の間隔を調整する。Moodの平常値からの偏りを控えめな既存`happy`表情に連続補間する。NERVAは新しい感情を作らず、関節角度をGhostから受け取らない。
- CoreS3の6つの変数の**完全移植ではない**。現行MacではMood/Affectionの真のセンサ処理・Relationship/Memory学習・電源断中の回復・繰り返し刺激のhabituationが未接続。Mood/Affectionの変化は模擬TOUCHでの検証だけ。Social/Comfortが完成したとは言わない。
- 同梱実モデルv0.5のGLB／表情マップ／project.godotは前配布版とハッシュ一致。v0.5スクリプトの実行版は検証ZIPに保存し、GitHubの `character/akari_v04_nerva` は**旧v0.4参照**であってv0.5コードの代用品ではない。
- ローカル検証（Linux/Python, websockets 16）：8件の単体＋WebSocket試験すべて通過、Viewer JavaScript構文チェック通過。Godot実機表示・かわいさ／自然さの合否は未確認。正式LOCKは変更していない。


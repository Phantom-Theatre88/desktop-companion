# AKARI 最小統合試験：Ghost → NERVA → Character

状態：**試作／正式LOCKは変更しない**。WORLDの高度化や専用PC購入は不要。

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

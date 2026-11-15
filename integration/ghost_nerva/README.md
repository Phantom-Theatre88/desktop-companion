# AKARI 最小統合試験：Ghost → NERVA → Character

状態：**試作／正式LOCKは変更しない**。WORLDの高度化や専用PC購入は不要。

## 目的

既存Ghostのイベント記録を保持しつつ、意味を持つ試験用意図だけをWebSocketでNERVAへ送る。NERVAはその意図をもとに身体の動きを生成し、Character v0.4の3D身体を動かす。**Ghostは首の角度・アニメーション番号を指定しない。**

重要：現行Python GhostのBehaviorはインターフェース試作段階なので、ここでは`near`等の**試験刺激→意味意図**を仮に渡す。自律的なBehavior判断が完成したわけではない。

## テスト方法

1. リポジトリの`heart-engine`ブランチを取得。
2. Python 3.11以上で、`python3 -m pip install 'websockets>=12,<17'`。
3. `python3 -m unittest discover -s integration/ghost_nerva -v` で契約試験。
4. `python3 integration/ghost_nerva/intent_server.py` でローカル送信機を起動。
5. Dropboxに保存したCharacter v0.4のGodotプロジェクトを**別フォルダへ複製**。
6. その複製にGitHub `character/akari_v04_nerva/scripts/main.gd` と `nerva_controller.gd` を適用。元版は変更しない。
7. `integration/ghost_nerva/ghost_intent_receiver.gd` を複製したGodotプロジェクトの`res://scripts/`に置き、**既存メインNode3Dの子Nodeにアタッチ**。
8. Godotで起動して、Python端末に`near`、`quiet`、`blink`、`status`を入力。

期待：`near`でNERVAが小さく顔を傾ける、`quiet`で待機動作が落ち着く、`blink`で瞬く。通信断では自律待機を継続し、3秒ごとに再接続を試す。既存の元モデル・骨格・表情・衣装は変更しない。

## 合格条件と制限

- Python契約試験：メッセージに角度指定が含まれず、既知の意図のみを送る。
- Godot試験：実際のAKARIモデルで上記3反応と通信断からの復帰を画面確認する。
- **未完成**：Ghostの自発的Behavior選択、永続記憶、Brain会話、音声、WORLD、実機統合確認。

この試験が成功したら、試験刺激を既存Ghostの実際のBehavior出力へ置き換える。新しい性格・感情の係数は本試験では確定しない。

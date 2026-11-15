# Ghost Viewer v0.1 — Mac用の独立診断画面（試作）

**状態**：実装したコードの検証用。正式聖典・LOCK・Ghostの人格仕様は変更していない。AKARIの完成、Ghost全体の完成を意味しない。

## 何ができる？

- Macのブラウザで、Ghost接続状態・実装済みの退屈／眠気・自発的行動意図をリアルタイム表示
- Ghost Viewerから `near`（注意）、`quiet`（静かな状態）、`blink`（瞬き）の**試験入力**を送信
- 意図の由来を **「Ghost自発（部分試作）」「Viewer試験」「端末試験」** に区別
- 通信ログをViewerからJSON保存。保存はGhostのMemoryには含まない
- Viewerを終了してもGhost送信機は別プロセスで継続する
- 入力と出力をローカル（127.0.0.1）に限定し、未知のコマンドは受け付けない

## 起動（Mac・リポジトリのルートで）

Python 3.11以上を使用。WebSocket用依存パッケージのみ必要。

```bash
python3 -m pip install 'websockets>=12,<17'
python3 -m unittest discover -s integration/ghost_nerva -v
```

**ターミナル1：Ghost試作を起動**

```bash
python3 integration/ghost_nerva/intent_server.py --headless
```

**ターミナル2：Viewerを起動**

```bash
python3 integration/ghost_nerva/ghost_viewer.py
```

Macのブラウザで **http://127.0.0.1:8767/** を開く。Viewerに接続済みと表示され、退屈・眠気は約1秒ごとに更新される。それぞれ Ctrl+C で停止する。

手早く眠気の推移を確認したい場合は、Ghost試作のみ `--time-scale 60 --headless` で起動する。**テスト専用の時間加速**であり、Ghost本来の生活時間設定ではない。

## 重要な境界と未実装

1. このViewerが読んでいるのは**Python統合試作の実状態**。退屈と眠気は旧CoreS3版に由来する無刺激時処理の部分移植で、Ghost全体ではない。
2. Pythonの `HeartSnapshot.mood` と `comfort` は現状の統合試作では固定初期値。あたかも自然に変化する感情としてゲージ表示しない。
3. 試験入力は実際の出来事の代わりではない。`near` / `quiet` / `blink` は既存の試験用意図へ変換され、本人の感情を直接変更しない。Viewer入力によるイベント記録は揮発性の試験記録。
4. NERVAに意図を送ることと、NERVAが受け取り3D身体に表現したことは異なる。**後者は未確認**。NERVAからの確認応答は次の工程。
5. Memoryの長期保持、Relationshipの関係評価、経験学習、再起動後の感情継続、Brain会話は未完成。
6. 開発用のローカル試験サービスであり、ネットワーク上へ公開したり本番の安全管理として使用しない。
7. Godot Character側は既存のGhost受信スクリプトを使う。通常待機のNERVAはViewerの有無によらず継続する。

## 技術内容

- Ghost試作: `intent_server.py` (127.0.0.1:8766 WebSocket)
- Viewer: `ghost_viewer.py` (127.0.0.1:8767 HTTP) + `ghost_viewer.html` (HTML/JS)
- Viewer受信: `ghost_snapshot`（観測用）、`ghost_intent`（意図＋由来）
- Viewer送信: `viewer_test_event`（許可された3種類のみ）
- Viewer受付応答: `viewer_test_ack`（送信受付でありNERVA動作保証ではない）

通信型は**この試験のための暫定形式**。正式なAKARI統合メッセージ形式としてLOCKしない。

## 対応した既存の聖典

- `docs/sacred/LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md`
- `docs/sacred/LOCK_DESKTOP_AI_INITIAL_TECH_STACK_2026-11-09.md`
- `docs/sacred/LOCK_GHOST_MOOD_COMFORT_SOCIAL_2026-10-21.md`
- `docs/sacred/LOCK_GHOST_EXPERIENCE_MEMORY_INTERPRETATION_TIME_2026-10-21.md`
- `docs/sacred/LOCK_CHARACTER_MOTOR_CONTROL_ARCHITECTURE_2026-11-12.md`

## Step 1 合格条件

- WebSocket初回接続で**現在値**を表示（架空の数値・疑似グラフなし）
- 自発行動とViewer強制試験のログが確実に分離
- GhostとViewerを別々に終了できる
- 禁止コマンドが拒否され、通信断は画面に表示される
- Macの実ブラウザで表示・操作・再接続を目視確認

実行環境での結果は検証後に記録し、未実施なら未実施と記す。

## 睡眠中に目が閉じたままになる場合（2026-11-15）

原因：現行NERVAの `sleep` は `blink_value = 1.0` で閉眼を維持する**設計上の睡眠表現**。肌色修正によるGLB破損ではない（GLB変更差分は肌マテリアル2件のみ）。

Viewerに**「呼びかけて起こす（試験）」**を追加した。既存CoreS3の `WAKE_WORD_DETECTED` に対応する試験入力をGhostの互換試作で処理し、15秒間覚醒を保持する。NERVAへは `idle` を伝達する。睡眠機能は削除・無効化しない。自然時間・長期の睡眠・起床は未完成。認識された実音声ではなく、診断用模擬入力である。

既存Macインストールの更新は、GitHub `heart-engine` から `ghost_auto.py`, `intent_server.py`, `ghost_viewer.html` の3ファイルだけをダウンロードし直し、GhostとViewerを再起動する。GodotのCharacter v0.5・肌色補正のZIPは変更不要。

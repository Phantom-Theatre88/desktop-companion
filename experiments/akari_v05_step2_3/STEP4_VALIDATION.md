# AKARI v0.5｜STEP 4 事前検査・状態同期修正（2026-11-23）

**成果区分：実モデルを保持した試験版。正式な通信スキーマ・人格・学習方式・既存LOCKは変更しない。**

## 基準
- 元実体：ChatGPT Libraryから回収した2026-11-15版AKARI v0.5モデル一式。
- 直前版：2026-11-23 STEP 2・3のPython往復通信試験版。
- 既存のGhost→NERVA→Character三本柱構成・NERVAはCharacter内部という境界を維持。

## 検査で見つけ、修正した接続欠落
**発見**：STEP 2・3版のGhostは自発的な「意図」を送信できるが、6種類のHeart状態がCharacterへ配送されない。このため、11/16の記録にある `boredom` / `sleepiness` に連動した脚の荷重移動が実体のGhost入力に結びつかない。

**改修**：
1. Python Ghostの `ghost_snapshot` をWebSocket接続時と定期更新時に送信。既存6値のみ、0〜1範囲のスカラーで通知（未実装の感情変化は作らない）。
2. Characterの `ghost_intent_receiver.gd` が6値の揃った有限数の状態だけを受理。接続断では状態をクリアし、再接続時に同期。重複意図の受付を抑止。状態同期を実行済みの動作として誤ってACKしない。
3. NERVAは受信した状態のうち `boredom` と `sleepiness` の実値を、既存の小さな脚荷重移動の振幅調整に使用。**新しい性格、行動選択、感情係数の正式仕様は導入しない**。
4. Godot画面にGhost intentと状態値の受信有無を表示。テスト画面で「Ghostは未接続」と固定表示していた誤情報を修正し、診断表示を1秒ごとに更新。

## GLBバイナリ構造解析（表示実行ではない）
- GLB v2として正常にJSONチャンクを読み込み、モデルの3 meshes / 3 skins / 各98 jointsを確認。
- VRM表情マップ上の全骨名がGLBのノード名に存在。
- 表情モーフターゲット57件がGLBに存在し、blink/happy/aa/ih/ou/ee/ohの対応先が一致。
- **モデルGLB自体は無変更**：SHA256 `2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d`。

## 自動テスト

```sh
cd <本ZIPの展開フォルダ>
python3 -m pip install 'websockets>=12,<17'
python3 -m unittest discover -s tests -v
```

- Python 12件合格：経験・関係参照、意図結果の照合、不正ACKと重複拒否、Ghostの時間変化、6状態snapshot、WebSocket模擬Character通信、GLBバイナリ構造・表情・骨の存在、Godotソース上の接続点の静的検査。
- この環境にはGodot 4実行ファイルがないため、GDScript実行やレンダリング、実際のボーン駆動、顔・視線・脚の可愛らしさと足裏接地は**未検証**。
- 通信の返信`applied_frame`は「モデルが結び付いた処理フレーム」であり、画面上の自然な身体動作・終了の証明ではない。

## Macでの自動事前検査と実画面試験（最小手順）

最初に `akari_motion_test/Mac自動検査.command` をダブルクリック。Python自動テストと、GodotがあればヘッドレスGLBインポート／GDScript起動検査を試み、`akari_motion_test/logs/` にログを出す（Macでの実行は未検証）。**ヘッドレス合格だけでは実画面・足裏の検証は完了しない。**

1. ZIPを展開して `akari_motion_test/起動_AKARI.command` をMacで実行。初回はPython3とGodot 4が必要、ネットワークは依存Pythonライブラリ取得のみ。
2. Characterのモデル・表情・呼吸・脚6本の状態、`Ghost state: CONNECTED VALUES`、自発的`drowsy`/`sleep`（現実時間で状態が変わるためすぐには移行しない）を確認。
3. 画面・Godot Outputを録画またはログとして保存し、問題をkibiが修正する。**目視実験は本ZIP作成時点では未実施。**

## 未完成・次の判断境界
- GhostのMemory/Relationshipを真の行動選択へ反映するアルゴリズムは未完成。今回の変更では新しい人格式を仮に確定していない。
- Godot上の動作確認は未実施。Macの実画面で評価するまで「STEP 4 完了」と扱わない。
- 脚の足裏固定・IK・衝突応答・動作の自然さは未確認。
- 永続記憶・Brain統合や外部送信ポリシーは別工程。
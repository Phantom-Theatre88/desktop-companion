# AKARI v0.5｜STEP 2・3・16日差分の検証試作（2026-11-23）

**状態：検証試作。正式な人格・学習仕様・通信スキーマのLOCKは変更しない。**

このフォルダはChatGPT Libraryで回収した**2026-11-15版の実モデル**をコピーし、そこでの資産を保持したまま新規試作処理を追加したもの。11月16日の配布ZIP自体を回収したわけではない。11月16日の脚荷重移動は、GitHubの記録に基づく**機能レベルの再構成**であり元コードと同一との保証はない。

## 実装したもの

1. **STEP 2（部分）**：`ghost_server/behavior_bridge.py` を新規追加。Memoryの経験記録とRelationshipの既存根拠を読み取り、現行CoreS3互換Behavior判断と共に証拠を添付して監査できる。NERVAの動作結果はGhostの経験イベントとして記録。失敗時は同じ行動をただちに繰り返さない安全用`idle`への退避を実装。新しい親密度・感情学習・人別行動アルゴリズムは未確定であり、**Relationshipによって行動が変わる完成品ではない**。
2. **STEP 3（通信試作）**：`ghost_server/intent_server.py` に`nerva_result`受付、sequence照合、重複・不正拒否、`ghost_feedback`配信を追加。`scripts/ghost_intent_receiver.gd`は受信意図をNERVAへ渡した後、実モデルがボーンバインドされて次の処理フレームに進んだ時だけ`applied_frame`を送信。モデルがない場合は`rejected`。`applied_frame`は**3D身体が画面上で自然に動いたことの保証ではない**。永続する`scan/sleep`意図について動作終了を捏造しない。
3. **Characterの差分復元（試験）**：`scripts/nerva_controller.gd`に左右各3本、計6本の脚ボーンを追加し、極小振幅の荷重移動を再構成した。膝と足首も回転し、睡眠中は減衰。既存の視線・腕・指・表情・GLBは維持。IK・足裏固定・歩行・転倒検知は未実装。睡眠中も呼吸を継続。
4. **Heart**：Mac互換試作は旧CoreS3に存在する6状態のフィールドを持つが、この版で自動変化するのは退屈・眠気のみ。他4値を演出的な「実際に変わる感情」と偽らない。

## 自動試験

```
python3 -m pip install 'websockets>=12,<17'
python3 -m unittest discover -s tests -v
```

`tests/test_integration.py`で7件テスト実施：経験／関係根拠の参照、失敗結果の再選択ガード、返信のsequence・重複防止、GLB SHA256・脚6本、拒否入力、自発的睡眠遷移、WebSocket疑似Character応答。

## Mac上での実体試験

`akari_motion_test/起動_AKARI.command` で起動。Python 3.11以上、`websockets>=12,<17`、Godot 4系が必要。GodotのGUIでGLBが正常表示されること、左右の脚・床・表情・呼吸が崩れないことを確認する。Ghostを切断した場合の待機、復旧時の状態再同期、連続稼働時の自然さ・可愛らしさも目視検証が必要。

## 確認済み／未確認

- **確認済み**：回収した実モデルのGLB SHA256 `2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d`を保持。Python 7件テスト成功、模擬NERVAからGhostへの返信と経験記録を検証。
- **未確認**：本環境にGodot実行ファイルがなく、GDScript実行・実際の3D骨格駆動・画面描画・足裏接地・通信断復帰・Mac実機検証ができていない。元の16日版ZIPとのバイト一致、実際のRelationship評価・経験依存の自発行動も未完成。

## 今後の最短経路

- Godot実画面での受信／返信と脚の姿勢破綻を検証する。
- Ghostに本物のSemanticイベントと過去の経験を渡せる実装を既存LOCKと照合して接続する。人間の模擬ボタンを本人の経験と偽らない。
- Relationshipの評価を既存聖典に沿って実装し、それによるBehavior選択を検証する。新しい感情式の独断決定はしない。
- 動作開始／完了の意味を厳密に決めてから、本番プロトコルと永続記憶を検証する。
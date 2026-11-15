# AKARI Character v0.5｜NERVA自発的全身運動 第1段階（2026-11-16）
状態：検証実装／自然さ・足裏接地のMac Godot実画面は未検証。正式LOCK変更なし。

## 正本根拠
- `docs/sacred/LOCK_CHARACTER_MOTOR_CONTROL_ARCHITECTURE_2026-11-12.md`
- `docs/sacred/LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md`
- `docs/sacred/LOCK_GHOST_MOOD_COMFORT_SOCIAL_2026-10-21.md`

## 実モデルへの差分
2026-11-16配布検証ZIP `AKARI_Ghost_NERVA_自発全身動作_第1段階.zip` の `akari_motion_test/scripts/nerva_controller.gd` は、**v0.5実モデルのNERVA**（v0.4のGitHub参照コードとは別物）に以下を組み込み、Python試験11件を通過。
- Skeletonの左／右太もも、膝、足首の計6骨を既存`vrm_expression_map.json`経由で取得。
- Ghostからの既存 `boredom` / `sleepiness` と `sleep` 意図によって姿勢変化の振幅・減衰を変える。
- 骨盤・胸腰・太もも・膝・足首に、時間と状態から生成したゆっくりした荷重移動を適用。Ghostはボーン角度やモーション番号を送らない。
- NERVAが運動側で方向・時間間隔・連続補間を担当。睡眠中は荷重移動を徐々に止め、呼吸・瞬き・視線・表情既存処理を維持。
- `main.gd` にデバッグ表示 `Leg bones: 6/6` / `Stance transfer` を追加。
- GLB、VRM表情マップ、project.godot、Ghost通信レシーバーのファイルバイト列は前配布版と一致を確認。

## 残る限界と次の条件
**接地は保守的な近似。** ルートの並進はせず脚の関節回転を小さくしたが、厳密なIK・足裏固定・衝突応答・転倒防止は未実装。歩行や足踏みとは呼ばない。Godot実機で床への沈み・足の滑り・膝・姿勢の破綻を確かめてから、IKなど必要な根治実装を決める。生命感やかわいさは未評価。テストと挙動を混同しない。

## 成果物
本会話の配布ZIPを検証実行版とする。公開GitHubリポジトリには18MBの実モデルと実機の経験データをPushしない。この記録は仕様正本の変更ではなく、v0.5検証ZIPとGitHub側v0.4参照実装を混同しないための履歴。

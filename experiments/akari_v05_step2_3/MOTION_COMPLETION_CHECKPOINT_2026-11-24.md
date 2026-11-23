# AKARI 2026-11-24｜Mocap実動作の結果判定修正（実装記録）

## LOCKを維持
最上位ゴール：Ghostの状態・経験・関係性からの自発的意思→NERVAの自然な身体動作→実際の動作完了を経験として返却→次の判断。Brain/Ghost/Characterを分離。

## 今回修正
- 旧コードは、着座・起立の3秒経過を動作終了の判断材料にしていた。アニメーションが途中でもGhostにcompletedが返りうる。
- NERVAがGodot AnimationPlayer.animation_finishedイベントを受信し、同じclipの終了を確認するまで、着座・起立のmotion_finishedはfalseとする。歩行clipはループなので到着位置を別に検査する。
- Godot起動前のGLB検査で、walk/sit/riseが単に存在するだけではPASSにせず、各クリップに少なくとも8本のRotation3Dトラックがあることを要求する。
- 既存の実モデル、Ghost意図選択、メモリー循環、正式LOCKは無変更。

## 検証
- Python単体/回帰テスト56件合格（Godot実装の静的検査含む）。
- GODOT実行バイナリ、KinemIQ CC0モーションアーカイブの実体を取得できず、Godotランタイム・自然な歩行・座位・手の動きは未検証。
- 前回の50件合格を自然な動作完成とは解釈しない。今回も同様。

## 必須残件
KinemIQの実GLBを検証付きで取得し、変換後GLBをGodot上で実行して、モーションの身体への適合と接地、座面一致、実完了通知、Ghostの経験還流まで一連で検証する。最終完成は未達。
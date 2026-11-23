# AKARI v0.5｜Ghostが選択・NERVAがMocapで動く統合検証版（2026-11-24）

**区分：実装試作／自然な実動作は未検証。正式LOCK・聖典を変更しない。**

## 達成した実装
- `tools/install_cc0_motion.py`：KinemIQ公開のCC0 GLB ZIP（35種、公開SHA-256 `64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc`）を初回起動時に取得・検証。source/targetの人型ボーン名と親のrest回転を照合し、元モデルを保存したまま別GLBへ最大限関連モーションを移す。出典と素材ハッシュを記録。`walk/sit/rise`が欠ければ失敗。
- `scripts/nerva_controller.gd`：Ghostが意味のある意図（explore/sit/rise 等）を選び、NERVAが対応するクリップ再生、ループ・再生速度・遷移を制御。Mocap中は旧来の手計算腕・脚・胴体制御で上書きしない。手スケール90%の既存可逆試験を維持。
- `scripts/main.gd` / `tools/check_akari_model.gd`：実際のAnimationPlayerと `walk/sit/rise` が確認できない場合は成功を返さない。元GLBは更新せず、生成した `akari_animated.glb` を使用。
- `起動_AKARI.command` / `Mac自動検査.command`：素材取得・生成→Godot再インポート→モデル・モーションクリップ確認→Ghostサーバー→AKARI起動の順序に変更。Macでのネット接続とGodotが必要。取得失敗時は元の不自然な動作を完成形として起動しない。

## 現時点で検証できたこと
- Python自動試験 **50/50 合格**（旧来のGhost/Character通信・経験循環試験46件と、合成GLBによるリターゲット・失敗安全性4件）。合成GLB内 `walk/sit/rise` をAKARIコピーへ組み込み、元メッシュ・skin・GLBを保持。実ソースのテストではない。
- シェル構文2件合格、元モデルSHA256 `2a7fcc66fcdb9a76944a47c666f79911f5f24cd0256685dd7f8c3e66a314b33d` で不変。

## 合格とは言えないこと
- この環境ではKinemIQ本体のネット取得に失敗したため、**正式CC0クリップの実バイナリはこのZIPには含まれない**。Mac側から自動取得・検証する仕組みを用意したが、実取得・本物の66ボーンとの対応は未確認。
- この環境にはGodot実行ファイルがなく、アニメーション付きGLBのインポート、自然な歩行・立位・着座、足裏IK・接地・椅子との衝突は実機で未検証。姿勢違い・骨格比例違いによる違和感が残る可能性がある。
- Character側のモーション土台を変えただけで、Ghost Relationshipの本格反映、Brain連携、WORLD認識や自発生活の最終完成ではない。

## 次の作業の判定基準
- 本物のCC0ソースZIP取得とSHA256一致、変換後のクリップ数・全身骨格動作を実Godotで検査。
- 人間がボタンで選ばず、Ghost状態・Memory・Relationshipから意図が生まれること、その実映像、完了結果と経験への還流を一組で検証。
- テンプレートは身体能力、意思はAKARI自身という役割分担を維持。直前の偽歩行へは戻さない。

資料: https://github.com/Kinemiq/free-game-animations / https://huggingface.co/datasets/Kinemiq/free-game-animations / https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/retargeting_3d_skeletons.html
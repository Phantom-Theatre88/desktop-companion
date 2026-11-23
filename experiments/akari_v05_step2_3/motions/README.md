# AKARI CC0モーション統合（動作検証版）

起動時に `tools/install_cc0_motion.py` がKinemIQ公式CC0の35クリップGLB ZIPを取得し、元モデル`avatar/akari_test.glb`から `avatar/akari_animated.glb` を生成。元GLBは改変禁止。`akari_animated.provenance.json` に出典、SHA、アニメーション一覧を記録。必要な歩行・着座・起立の3種類がなければ処理を失敗させる。外部素材本体はこの配布ZIPに含まない。

**方法**：Source/Targetの対応する人型ボーンについて、親ボーンのrest回転を考慮して回転差分を移し、腰の上下移動を座位用に写す。足裏IKや衝突判定の完成ではない。実クリップのボーン名・座位・歩行姿勢はGodotで実画面検証が必要。

**意思と身体の分離**：Ghostが `explore/sit/rise/...` の意図だけを選び、NERVAはテンプレートと速度・つなぎを制御。モーションがあっても自発意思の完成とは呼ばない。

**ソース**：https://github.com/Kinemiq/free-game-animations 及び https://huggingface.co/datasets/Kinemiq/free-game-animations 。権利 CC0 1.0、出典 Quaternius/ KinemIQ。Godot人型リターゲットの原理は https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/retargeting_3d_skeletons.html 。

## 2026-11-24 取得障害の根本対策（検証中）

配布元のHugging Face上にGLB ZIP実体（2.22 MB）とSHA256 `64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc` が存在することを確認。
この制作環境で `urllib.request.urlopen` を実行すると `URLError [Errno -3] Temporary failure in name resolution` となるため、**制作環境からの取得は失敗**。Mac側の通信環境で同じエラーかは未確認。

起動スクリプトは以下を自動試行する（全経路で上記SHAを必須検査）。

- Hugging Face公式resolve
- Hugging Face公式resolve `?download=true`
- hf-mirror.com（非公式ミラー。検証済みSHA一致時だけ採用）

ダウンロードが止まる場合、ログ `logs/akari_start_*.log` に `MOTION_DOWNLOAD_FAIL` 行で通信／HTTP／SHAの原因が残る。
**手作業による最短の復旧策**：配布元の[モーションZIPページ](https://huggingface.co/datasets/Kinemiq/free-game-animations/blob/main/KinemIQ_Free_Animations_CC0_GLB.zip)から `KinemIQ_Free_Animations_CC0_GLB.zip` をダウンロードし、展開した `akari_motion_test/motions/source/` に置く。再度 `起動_AKARI.command` を起動する。既存のZIPは暗黙に信用せずSHA照合する。Quaterniusの別パックを同名に変更してもSHAが異なるため使用しない。

**未検証**：Macからの実取得、取得した正規GLBのボーン対応、Godot実アニメーション表示と自然な動き。テスト用合成GLBによる変換成功は実素材の検証とは区別する。
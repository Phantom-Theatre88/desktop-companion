# AKARI CC0モーション統合（動作検証版）

起動時に `tools/install_cc0_motion.py` がKinemIQ公式CC0の35クリップGLB ZIPを取得し、元モデル`avatar/akari_test.glb`から `avatar/akari_animated.glb` を生成。元GLBは改変禁止。`akari_animated.provenance.json` に出典、SHA、アニメーション一覧を記録。必要な歩行・着座・起立の3種類がなければ処理を失敗させる。外部素材本体はこの配布ZIPに含まない。

**方法**：Source/Targetの対応する人型ボーンについて、親ボーンのrest回転を考慮して回転差分を移し、腰の上下移動を座位用に写す。足裏IKや衝突判定の完成ではない。実クリップのボーン名・座位・歩行姿勢はGodotで実画面検証が必要。

**意思と身体の分離**：Ghostが `explore/sit/rise/...` の意図だけを選び、NERVAはテンプレートと速度・つなぎを制御。モーションがあっても自発意思の完成とは呼ばない。

**ソース**：https://github.com/Kinemiq/free-game-animations 及び https://huggingface.co/datasets/Kinemiq/free-game-animations 。権利 CC0 1.0、出典 Quaternius/ KinemIQ。Godot人型リターゲットの原理は https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/retargeting_3d_skeletons.html 。
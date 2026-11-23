# AKARI v0.5 モーション素材受け入れ（未統合）

**状態：Mocap受け入れ検査を実装。外部モーションクリップ本体の導入・リターゲット・Godot実動作は未完了。**

## 候補（規約確認のうえ、採用は試験版だけで行う）
- Kinemiq/free-game-animations: CC0 humanoid clips (walking, sit down, stand up). https://github.com/Kinemiq/free-game-animations
- Quaternius Universal Animation Library: CC0 humanoid motion packs. https://quaternius.com/
- Godot official retargeting instructions: https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/retargeting_3d_skeletons.html

## 安全な取り込み手順
1. ダウンロードしたライセンス・出典・バージョンを `source/` 内の各素材と共に記録。正規に入手した GLB/GLTF だけを採用する。出典未確認なら採用しない。
2. `python3 tools/audit_mocap_glb.py motions/source/CLIP.glb` で skeleton joint にアニメーションがあるか事前検査。クリップを自動適用はしない。
3. GodotのImportで、**元クリップとAKARI双方**に `SkeletonProfileHumanoid` / `BoneMap` を設定。モーションクリップを `AnimationLibrary` としてインポートする。骨格比率・ポーズの違いを検査。
4. 歩行はin-place素材を優先し、NERVAがroot移動を担当（root-motionと二重移動禁止）。フットロック・IK・接地を実装し、足滑りがないことをGodot画面で検証する。
5. 座面の高さ・股関節位置を補正して、座る→休む→立つを実画面で確認。腕の動きを既存呼吸・視線とブレンドする。
6. Ghostのsemantic intentだけを入力とし、NERVAが速度・ブレンド・接地を決める。定型モーション再生だけで完成と判断しない。

## 重要
現在の同梱ZIPに第三者の実アニメーション素材は**入っていない**。`source/` と `retargeted/` は受け入れ場所。以前の自作歩行が自然になったと主張しない。
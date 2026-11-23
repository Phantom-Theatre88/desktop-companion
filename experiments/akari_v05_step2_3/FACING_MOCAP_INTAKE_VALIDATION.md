# AKARI v0.5｜後ろ向き歩行の修正とMocap受け入れ準備（2026-11-23）

## 合意済みLOCK
2026-11-23 GitHub `heart-engine/docs/sacred/LOCK_AKARI_AUTONOMOUS_CONNECTION_AUDIT_2026-11-23.md` と Notionの同名LOCKを確認。Character/NERVAは独立、Ghostは行動の意味を決め、NERVAが運動制御を担当。LOCKは変更しない。

## 原因（確認済み）
配布済み `nerva_controller.gd` は目標 -X 方向へ移動させる一方、`motion_root.rotation.y = +1.40` と指定していた。実GLBのFaceメッシュは+Z側、Hairは-Z側に中心があり、顔は+Z向き。+Z向きのモデルを-Xに向けるには約-90°で、前の+1.40 radは逆向きだった。

## 修正済み
- `atan2(移動方向X, 移動方向Z)` から向きを算出。
- 0.8秒方向転換 → 4.4秒の移動 → 0.8秒戻りの段階を明示。向き直る前・後は横移動をしない。
- 歩行位相は実移動距離に連動する既存処理を維持。現在はまだMocapでない。
- CC0歩行・座位モーション採用に向け、GLB/GLTFアニメーションとskin-joint駆動を確認する標準ライブラリのみの `tools/audit_mocap_glb.py` を追加。Godot公式`SkeletonProfileHumanoid`/`BoneMap`に沿う受け入れ手順を `motions/README.md` で明示。
- 第三者モーションファイルは未取得。無断ダウンロードやライセンス不明素材の混入なし。

## 検証
- Python tests **40/40合格**。GLBにアニメーションクリップがないことも検査済み。
- ZIPとMac `.command` 実行権限・GLB SHAを別途検査。
- Godotエンジン未導入の環境につきGodot実画面の方向修正、歩行／着座の自然さ、接地は未検証。
- Memory/RelationshipのBehavior選択完了ではない。

## 次の実作業（既定方針内）
利用ライセンスが確認できるCC0歩行・着座・起立GLBを取得し、AKARIのBoneMapへリターゲット。root motionの二重適用を避けてNERVAに組み込み、足接地・座面一致・腕の同期を実画面検証。Ghostの意味意図→NERVAの運動生成と結果帰還は維持。
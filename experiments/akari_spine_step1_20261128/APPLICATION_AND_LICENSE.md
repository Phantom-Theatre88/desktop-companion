# Spine技術のAKARI適用・利用許諾評価 2026-11-28

既存LOCK維持。Spine基本技術9/9実行確認（RESULT.md）。正式採用・購入・AKARI実装完了ではない。

## 許諾
公式 https://esotericsoftware.com/spine-editor-license および https://en.esotericsoftware.com/spine-runtimes-license の2026-11-14改訂を確認。評価と製品利用を区別。製品組込にはEditorライセンス等の条件がある。Trialは製品利用権を付与しない。Professionalはメッシュ・IK制作に必要（https://ja.esotericsoftware.com/spine-purchase 掲載379 USD、通常449 USD）。購入承認なし。第三者素材・ランタイムの公開転載なし。Godot標準機能の独立利用はMIT条件に従う https://godotengine.org/license/ 。Spineのコード・素材を移植することと一般技術を参考にすることを区別。

## 技術対応
- 骨格：Skeleton2D / Bone2D、共通基準姿勢、身体寸法維持。
- IK：SkeletonModification2DTwoBoneIK / ModificationStack。実験的APIにつき使用バージョンで確認。
- メッシュ：Polygon2Dの骨ウェイト、UV、共通rest pose。肩・衣装を含む全身の連続性を検証。描かれていない方向は描き分けで補う。
- Attachment：状態・角度ごとの素材選択、Sprite2D/Polygon2D表示切替。GodotにSpine Attachmentと同一の専用機能があるという意味ではない。
- 描画順：CanvasItem.z_index等、ポーズ別の重なり制御。
- ブレンド：AnimationTree / AnimationPlayer、身体・表情の同時制御。
- 外部制御：既存NERVAの意味的指令と実行結果。Ghostの責務変更なし。

## 既存資産への適用方針（候補）
合格済み直立原画と82基本パーツLOCKを維持。既存v07原本保全。各素材の透過・輪郭・骨接続・ウェイト・方向別対応を実物で監査してから新規Character候補に統合。Spineboyの体型・画像・モーションをそのまま使わない。腕上げは最初の統合試験であって完成目的ではない。

## 判定
利用条件とAPI対応の確認は完了。AKARI素材への実適用、Godot実ランタイムでの確認、原画との一致・連続動作評価は未完了。Spine購入不要という判断はGodot標準機能を独立利用する候補経路に限る。STEP1全体未完了。次は公式Godot Skeleton2D実動作確認とAKARI素材監査。
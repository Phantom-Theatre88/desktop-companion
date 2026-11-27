# AKARI Character STEP 1 検証記録 2026-11-28
状態：部分実証、未完了。既存LOCK変更なし。
正本：docs/sacred/LOCK_AKARI_CHARACTER_IMPLEMENTATION_APPROACH_2026-11-28.md と三要素統合LOCK。Notion改善方針も再取得。
## 実際の取得・実行
- Spine 4.2公式 Spineboy JSONをGitHubコネクタで取得・JSON解析。blob f3ba20944ba6025e59b8a5dc00ad22b6cd58c73f。67 bones、52 slots、7 IK、11 animations (aim/death/hoverboard/idle/idle-turn/jump/portal/run/run-to-idle/shoot/walk)。
- 公式 Slot.ts blob 1d88d7d8692d2393d106f36f57543f1977cb3d3f の setAttachment メソッドを取得し、型注釈・型キャストのみ除去してJS実行。モックVertexAttachmentに対し同一timelineの変形維持、異なるtimelineの変形初期化、同一素材の再設定無変更、nullによる解除の4条件すべて成功。
- 公式 IkConstraint.ts blob 4f8f0837741b6c17242567d8e0c96b4731d25657 の apply1 メソッドを取得し、型注釈のみ除去してJS実行。親行列単位、モックbone、伸縮無効、長85.481、目標角0/30/55/80/-45度。出力0/29.999999999999993/54.99999999999999/80/-44.99999999999999、scaleX=1、5/5成功。実ボーンの更新はモック受け口であり、2関節IK・全身描画・実際のランタイム起動ではない。
- Live2D CubismWebSamples develop Hiyori.model3.json blob b93e4506b1d6e3728599e06eaef3f0171fafba86 取得・解析。Moc3、テクスチャ2、Physics/Pose、Idle 9、TapBody 1を確認。Cubism CoreとWebGL未実行。
- Godot公式 godot-demo-projects master 2d/skeleton/project.godot blob 8a2603e7470896179610c45f838dc4d9dd7eb3dd 取得。Skeleton2D Demo、main_scene level.tscn、4.7指定。実機起動未実施。
## 制約と採用判断
ローカル環境にNode/tsc/Chromiumあり、Godotなし。npmレジストリ直接接続はDNSエラー、公式Spineのビルド済みJS候補4パスは404。GitHubコネクタによるソース取得で部分検証を実施。Spineランタイム利用・配布はライセンス条件があるため未採用・未購入。公式ソースや画像は本リポジトリに転載しない。
## 次の再開地点
Spineboy公式ランタイム・モデル・アトラス・画像を取得可能な実行環境で全身描画し、外部目標でIK、Attachment、drawOrder、動作ブレンドを同時制御する。Live2D HiyoriとGodot公式Skeleton2Dも実際に起動し同一観点で比較。既存Godot優先LOCK維持。正式採用は未決定。AKARI v07は変更しない。STEP 1は部分実証のみで未合格。
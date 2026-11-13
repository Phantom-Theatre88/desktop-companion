# NERVA v0.1 — 自律待機運動の実装

Godot 4系用の **Character内運動制御モジュール**。Ghostより先に、自律的な身体の待機動作を小さく検証するための独立した実装です。

## 実装範囲

- 呼吸、わずかな胸・肩の動き、重心を連想させる姿勢変化
- 頭・首の揺れと、ランダムに選ばれる次の視線目標（固定モーションの再生なし）
- 非周期の瞬き。顔メッシュに `Blink` ブレンドシェイプが存在する場合は値を書き込む
- 骨の存在確認と、既存の骨の初期姿勢を基準とした相対的な回転
- Ghost不在でも連続動作。Ghostの感情・欲求・行動意図はここで生成しない

時間の更新はGodotの `delta` に基づき、WORLDの天気プレビューや仮想未来時間で動き・経験を飛ばさない。番号付きのモーションクリップを選択する実装はない。

## 既存Characterへの接続（未実施）

1. 実際のCharacter Godotプロジェクトに `nerva_idle.gd` をコピー。
2. CharacterのNode3D子ノードにスクリプトをアタッチ。
3. インスペクターで `skeleton_path` に `Skeleton3D` の相対パスを設定。顔のブレンドシェイプを持つMeshInstance3Dがある場合、`face_mesh_path` を設定。
4. モデルのボーン名に合わせてHead/Neck/Chest/Shoulder/Eyeの各名称を設定（ボーン名を推測で変えない）。
5. 実際のCharacterを再生して、約1分間の瞬き・視線・呼吸・関節破綻の有無を目視確認する。

**重要：このリポジトリの確認対象ブランチにはCharacter v0.4のGodot本体が見当たらないため、既存モデルへの接続・キャラクターとしての実画面表示は未実施。** v0.4の骨格名、表情のブレンドシェイプ名、ローカル実装の状態は未確認。

## テスト

Godot実行環境があればリポジトリのルートから：

```sh
godot --headless --path character/nerva --script tests/test_idle.gd
```

`tests/test_idle.gd` は7ボーンの合成Skeleton3Dを生成し、Ghostなしで80秒の模擬更新を走らせ、関節範囲、瞬き、姿勢変化と経過時間などを検査します。**現在の制作環境にはGodotがなく、テストコードの実行は未確認です。** ファイルのGitHub Pushと再取得を検証しています。

## 今後の統合範囲

- 実キャラクターの骨格への名称マッピングを確認
- 3D画面で関節挙動・視線・表情を目視検証
- 接続断や復帰のGhost連携
- NERVAによる安全な歩行や実空間との接触判定

正式聖典：`docs/sacred/LOCK_CHARACTER_MOTOR_CONTROL_ARCHITECTURE_2026-11-12.md`、`docs/sacred/LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md`。この実装は追加LOCKや既存聖典の仕様変更ではありません。

# Character v0.4 → NERVA 統合版（ソース差し替え）

**正本**：Dropbox `/新ロボット/ProjectA/名称未設定フォルダ/akari_godot_test_v04`

本フォルダの `scripts/main.gd` と `scripts/nerva_controller.gd` を、**v0.4のGodotプロジェクトを複製した作業版**の `scripts/` に配置して起動する。元のDropbox v0.4は変更しない。 `avatar/akari_test.glb`、`avatar/vrm_expression_map.json`、PNG群、`scenes/main.tscn`、`project.godot` は元v0.4のものを使用。

## 変更内容
- v0.4の待機運動、腕の回転符号、視線、首、呼吸、瞬きの数式をNERVAへ移動し、動作式は変更していない
- `main.gd` はUI、GLB読込、顔BlendShape書込、各種診断に集中
- NERVAがボーン姿勢を最終適用し、元コードの競合書込を排除
- Ghost、WORLDには接続しない。Ghostによる感情・意図も生成しない
- UIの「Auto idle」「Arm relaxation（94%）」「Blink now」「Smile」「Mouth A」「Neck Yaw/Pitch」「Eyes Yaw」「Reset pose」を維持
- 正式な再設計の入力はDropbox実ファイルであり、先にGitHubへ作った汎用`character/nerva/nerva_idle.gd`は**併用不可**（同じ骨格を二重に書き換えるため）

## 検証
- Dropbox v0.4 `scripts/main.gd` のコードを取得し、関数・文字列の存在比較と数式の保持を静的に確認
- GitHubへのPushと再取得を確認
- **Godot実行テスト・3D画面の実視認は未実施**。Character v0.4の完全なGodot実ファイルはDropboxにあり、GitHubへはテキスト差し替えファイルのみ保存
- 同じ骨格を別スクリプトが同時に上書きしないか、腕が上がらないか、視線とまばたきが自然かはMacのGodotで確認が必要

## 既存の未修正事項
- v0.4の`main.gd`内に未使用変数や旧バージョンコメントが残るが、今回一括改修では動作式の保存を優先した
- 歩行、IK、WORLDとの衝突、Ghostの意図入力は未実装

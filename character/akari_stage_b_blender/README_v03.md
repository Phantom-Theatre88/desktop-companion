# AKARI Stage B v03｜Blender編集用の造形試験パッケージ

**未承認原型です。完成したAKARIではありません。**

- `AKARI_StageB_V03_NEW_FACE_HAIR_UNAPPROVED.glb`：原画との照合のために実制作した3Dメッシュ。頭部・顔・目・髪は新メッシュ、胴体は独立Stage B v02原型由来。
- `AKARI_StageB_v03_four_views.png`：実3Dを正面・横・後・45°から描画した四方向比較。
- `reference/`：Kimから提供された原画（**公開リポジトリへ転載しない**）。
- `source/`：制作GLBとv02身体原型。原本非変更。
- `tools/setup_stageB_in_blender.py`：BlenderでGLB、参照画像、カメラ、照明を読み込み、`.blend` と四方向レンダーを作る。
- `Blenderでv03制作シーン作成.command`：Mac上のBlenderを背景起動して制作シーンを書き出す。標準配置以外のBlender.appは検出できないことがある。
- `build_stage_b_v03.py`：Python/Trimesh/VTKでv03 GLB・四方向実レンダーを再生成するスクリプト。Mac上で直接使用するにはPython依存ライブラリが必要。**Blender実行には不要**。

## Macでの確認

ZIPを解凍し、`Blenderでv03制作シーン作成.command` を起動。`output/` に `.blend` と四方向画像ができた場合にのみBlender工程成功とみなす。失敗した場合は `output/Blender_実行結果.log` を確認する。Kimに手動モデリングをお願いするものではない。

**造形修正・再生成はkibi担当。** 正本に忠実で可愛い、という品質判定に通るまでStage Bとして完了としない。
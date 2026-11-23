# AKARI CC0モーション実体取得の原因・対策（2026-11-24）

## 確認できた事実
- 既存ZIP `AKARI_v05_実モーション完了検証強化_20261124.zip` に正規モーションZIPは同梱されていなかった。
- 配布元 https://huggingface.co/datasets/Kinemiq/free-game-animations/blob/main/KinemIQ_Free_Animations_CC0_GLB.zip は実在し、配布元ページに容量約2.22MB、SHA256 `64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc` が記載されている。
- この制作コンテナから同URLを `urllib.request.urlopen` したところ、`URLError [Errno -3] Temporary failure in name resolution` で止まった。**ダウンロード先の404ではなく、制作環境から外部DNSを解決できない問題。** Macでの結果は不明。
- `Kinemiq/free-game-animations` のCC0 ZIPは歩行・座る・立つなどを含む。これは公式掲載内容であり実ZIP内容のローカル抽出確認ではない。

## 変更内容
- `tools/install_cc0_motion.py`: 公式本線、公式?download=true、hash-pinnedコミュニティミラーの順に失敗理由を記録して取得。
- バイト改変やZIP損傷を検出した場合は不採用、既存の手元ZIPも必ずSHA256とCRC検査。
- 完全オフラインでは正規ZIPを `motions/source/` に置いて再試行できる。実素材の取得失敗を「AKARIが動いた」と扱わない。
- 従来のretarget処理、Ghostの意思決定、NERVAの役割、AKARI元GLB、正式LOCKは変更していない。

## 検査結果
- Python unittest 60/60 passed。取得先全失敗・2番目へのフェイルオーバー・異常ハッシュ拒否・合成GLB retarget等。
- 外部DNS遮断により**公開ZIPの実ダウンロードは引き続き未達**。
- Godot/Mac実画面・実素材の関節適合・足裏接地・座面接触・自然さ、Relationship反映、Brain統合は未検証／未完成。
- 次は公開ZIPのMac環境からの実取得とGodot再生ログによる判断。取得不能時もエラー原因がログに残る。
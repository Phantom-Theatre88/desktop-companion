# AKARI / NERVA 実体取り込み済みモーション（2026-11-24）

**今版は、Kimがアップロードした実際の35モーションZIPと変換済みAKARI GLBを同梱済み。インターネットで再取得する必要はありません。**

- 原本：`motions/source/KinemIQ_Free_Animations_CC0_GLB.zip`
- ユーザー提供ZIP SHA-256：`55365fc4c2c472ab24fc735139e5ae0d1e2d0e54ef383877623290e0379b947e`
- 配布元掲載値：`64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc`（一致しない。Mac由来のZIP追加メタデータ含有。公式と同一バイト列の保証はできないが、内部35アニメーション・README CC0文面を実際に確認済み）
- ライセンス：アーカイブ内の `README.txt` に記載の CC0 1.0（Quaternius / KinemIQ）
- 変換済み：`avatar/akari_animated.glb`（15種類のAKARI用クリップ、約49関節回転＋腰位置）
- 根拠：`avatar/akari_animated.provenance.json`

対応：`walk`, `sit`, `rise`, `idle`, `seated_idle`, `talk_idle`, `wait`, `interact`, `pick_up`, `jog`, `walk_formal`, `walk_carry`, `walk_crouched`, `crouched_idle`, `dance`。

役割：Ghostは行動の意図を選び、Character内のNERVAがクリップ・速度・つなぎを制御する。外観は元モデルGLBから変更しない。

**未検証**：Godot上での実際の自然さ、つま先接地、椅子への正確な着座・動作の見た目。ここはMac実動作の判断が必要。固定の数式歩行を自律生活の完成品とは見なさない。
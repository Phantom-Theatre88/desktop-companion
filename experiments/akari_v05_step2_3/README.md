## 2026-11-24｜実35モーション内蔵版（最初に読む）

本配布物には、Kim提供のCC0モーションZIPと、原AKARIモデルを保持したまま生成したアニメーション付きGLBが**両方同梱**されています。別途ダウンロードする必要はありません。MacでGodot 4.3以上とPython3を用意した環境なら `起動_AKARI.command` で再変換確認・Godotインポート・Ghost接続起動を試みます。`Mac自動検査.command` は起動前の動作確認用です。ログは `akari_motion_test/logs/` に残ります。

Godot実画面での全身動作の自然さはこの環境では未検証。実際の足裏・座面接触の完成判定は保留です。

---

# AKARI v0.5｜Ghostが選び、NERVAがCC0モーションで動く【開発検証版】

**重要：実CC0素材はこのZIPに同梱していない。** 最初の `起動_AKARI.command` がKinemIQ公式公開物（CC0、GLB約2.22MB）をダウンロードしてSHA-256照合し、AKARI元GLBを変更せず `avatar/akari_animated.glb` を作成する。通信・人格は既存のGhostを使用する。

- **Ghost**：既存のHeart/Time/Memory/Behaviorで `explore / sit / rise / idle` 等の意図を選ぶ。Mocapのクリップ名や関節角度は送らない。
- **NERVA**：`walk / sit / rise / idle / seated_idle` 等の収録動作を選び、遷移時間・root移動を扱う。Mocapを使う間は以前の手計算の全身動作を上書き停止する。
- **Character**：別GLB内の実アニメーションをGodotで再生。既存の眉・顔・衣装・装備・元モデルGLBは保持。手の縮小は既存の可逆試験設定。
- **起動**：新しいフォルダへZIPを解凍 → `akari_motion_test/起動_AKARI.command` を実行。初回はインターネット接続が必要。ダウンロード/素材整合性/骨格適合/アニメーション読み込みに失敗したら、従来の不自然な動きを完成形として表示せず停止する。
- **検査**：`Mac自動検査.command` でも同じ素材を取得し、インポートとメッシュ・骨格・表情・歩行・着座・起立のクリップ検査を行う。

**重要な未達**：KinemIQの実GLBファイルはこの環境から取得できず、実データへの骨格適用やGodot実画面の自然さは未検証。後方歩行・足裏接地・座面衝突、動作の補間・足滑りも現物検査が必要。ここでの自動試験は合成GLBを使った変換機能と既存の通信/経験循環の検証であり、AKARIが自然に歩いた証拠ではない。GhostのRelationshipに基づく行動の完成もまだ未達。

出典：KinemIQ/QuaterniusのCC0 1.0モーション。https://huggingface.co/datasets/Kinemiq/free-game-animations 。ダウンロードした原本の公開SHA-256を照合する。正式LOCKは `heart-engine/docs/sacred/` のままで改変しない。

---

# Akari Godot test v0.4 — arm-direction correction

v0.3で両腕が頭上に上がった問題に対し、上腕の回転符号を左右とも反転しました。元の3Dモデル、テクスチャ、顔・首・自動瞬き等は変更していません。Godot実機確認は未実施です。

**確認事項**: `project.godot` を新しいフォルダからインポートし、実行。Arm relaxation が94%で腕が胴体の横に下がるか確認してください。0%ではTポーズに戻る想定です。

---

# あかり先輩 Character v0.3 — 自律待機モーション

## 操作 (Godot 4.3以降・通常版)
1. このフォルダ内の `project.godot` を Godot Project Manager からインポート。
2. 初回の GLB インポートを完了してから、右上の▶または F6 で `scenes/main.tscn` を実行。
3. モデルが T ポーズから腕を下ろした立ち姿に変わり、視線・瞬き・首・呼吸・小さな上体の揺れを自動生成するか確認。

## コントロール
- **Auto idle ON / OFF**: 自律待機を停止・再開。停止中も腕の基本姿勢は維持。
- **Arm relaxation**: 0=Tポーズ、100=腕を下ろす。初期値94。
- **Neck Yaw / Pitch, Eyes Yaw**: 自律動作に加える手動オフセット。
- **Blink now / Smile / Mouth A**: 表情単体テスト。
- **Show gray material / Front-back**: 表示問題の診断。
- **Reset pose**: 手動オフセットと視線目標を初期化。

## 仕様・限界
- 自律動作はランダムな視線目標と滑らかな補間、複数周期の微小動作を使用。事前収録した固定モーションクリップではない。
- v0.3はCharacter単体試験。GhostからのWebSocket入力は未接続。GhostのMemory/Heart等の変更は行わない。
- 足位置は固定。歩行・IK・接触判定・装備品はまだない。
- GLBモデルの実際の関節向きによって腕の角度などの最終調整が必要になる。首や目の動作は Godot 実機での確認待ち。
- VRM spring bone などのVRM専用拡張は GLB 変換時に削除されている。髪物理はこの試験には含まれない。
- 元データ `あかり先輩(1).vrm` の外見・メッシュ・テクスチャを変更しない。

## 出典方針
GitHub 聖典 `LOCK_DESKTOP_AI_THREE_PILLARS_IMPLEMENTATION_2026-11-09.md` v1.2 および `LOCK_DESKTOP_AI_INITIAL_TECH_STACK_2026-11-09.md` に基づき、Characterの動作をGhostから独立して検証する。

## 変更履歴
v0.3: 自律待機、自然立ち姿、調整UI。
v0.2: GLB読み込み診断、素材・カメラ確認。

## 2026-11-24：足裏・椅子と動作遷移の修正（Kim実画面への対応）

歩行の根移動はMocap1周期約1.3秒へ同期、椅子は通行線の後方へ移動。`tools/contact_calibration.json` は **このユーザー提供CC0 ZIPと元AKARI GLBの組み合わせ専用**の足IK較正表であり、未知のファイルへの使い回しを拒否する。`tools/audit_foot_contact.py` は実GLBの足先FKを検査する。座位は腰の回転と脚を揃えて sit → seated_idle → rise を連続化する。Mac起動スクリプトは元モデルを改変せずアニメーションGLBを再生成する。これは床接地の数値検査であり、Godotの物理接触・自然さの実証ではない。詳細は同梱の `AKARI_接地と動作遷移_修正記録_20261124.md`。
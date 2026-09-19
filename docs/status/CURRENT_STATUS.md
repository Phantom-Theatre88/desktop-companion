# Desktop Companion — CURRENT STATUS

更新日: 2026-09-18

この文書は、このプロジェクトの「現在地」の正本である。

## 1. 最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## 実機の前提

実機は **M5Stack Stack-chan完成機（CoreS3主制御＋内蔵2軸首サーボ）**。CoreS3単体成立はソフトウェアの検証段階であり、首のない別の機体を意味しない。既存Stack-chanソフトを親Repoにしない方針と、Stack-chanの身体を使用することは区別する。

## 2. 現在の正式実装方針

実装基盤はゼロベースを維持する。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

既存OSSを親Repoにしない。

Ghostは後付け機能ではなく、最初から本体中核として接続する。

LOCK 52により、外付け感覚器官を増やす前に、CoreS3単体でDeskRoboとして生きている状態を成立させることを正式順序とした。

2026-09-18、LOCK 53を追加し、**CoreS3単体DeskRobo成立後は、ToF4M / U172より先にCoreS3内蔵カメラをM5Stack側の「反射の目」として仕上げる**ことを正式順序とした。

ToF4M / U172は引き続き最初の**外部感覚**縦貫通対象とする。

## 3. Desktop Robo参照実装

Ghost本体は独自実装のまま維持し、以下を正式な参照実装として採用済み。

- Yuki Desktop Robot：Face / Eye / Mouth / Tracking / Head Motion
- AuraBot：Pi5-CoreS3 Boundary / Perception Architecture / Communication
- KariPom：MicroBehavior / Life Presence

参照実装の都合でGhost構造を曲げない。

## 4. 現在の実装段階

**基盤／OS相当：準備完了**

**Step 4｜Heart / Memory / Reflexの最小本番実装：本番骨格まで完了**

`heart-engine` ブランチの `firmware/cores3_base/` には現在、以下の本番骨格がある。

- `SemanticNeuron`
- `SynapseRouter`
- `GhostCore`
- `HeartEngine`
- `MemoryEngine`
- `TimeEngine`
- `RelationshipEngine`
- `BehaviorEngine`
- `ReflexLayer`
- `DesktopCompanionRuntime`
- `FaceRenderer`
- `HeartPersistence`
- `HeartMicroSdBackup`

LOCK 47〜49に従い、`ReflexResult` はRuntimeからGhostへ戻り、Heart / Memoryへ渡る。

CoreS3実機でこのReflex結果フィードバック追加後のコードについて、コンパイル／書き込み／起動まで確認済み。

LOCK 51に従い、Heart永続化は **NVS主保存＋microSDバックアップ** とする。

NVS主保存の第一段として `HeartPersistence` を実装し、CoreS3実機で以下を確認済み。

- `HeartPersistence.cpp` が実際にビルド対象へ入る
- Arduino `Preferences` がリンクされる
- CoreS3へ正常書き込みできる
- 再起動後に `[HEART][NVS] Existing primary snapshot loaded` が出る
- Heart 6状態をNVS snapshotとして読み出せる起動経路が成立する

これにより、**HeartのNVS主保存／復元の本番骨格は実機確認まで通過**とする。

さらにStep 4のMemory本番境界として、以下を `heart-engine` へ追加した。

- `MemoryRecord`：Semantic eventまたはReflex resultと、その時点のHeart Contextを結び付けられる記録候補
- `MemoryLane`：
  - `INDIVIDUAL_EXPERIENCE`
  - `REPEATED_TREND`
  - `SPECIAL_LONG_TERM`
  - `RELATIONSHIP_IMPACT`
- 各Memory Laneへ将来の保存実装を接続できるhandler境界
- `lastCandidate()`：直近の記憶候補を保持する最小境界

CoreS3でビルド・書き込み・再起動確認まで通過済み。

具体的な保持件数、分類条件、減衰率、長期保存条件は未LOCKのため実装していない。

続いてLOCK 46・50に従い、Memory/GhostからReflexへ経験由来の感度修飾を渡せる入口境界を実装した。

- `ReflexSensitivityDirection`：`BASELINE / HEIGHTEN / RELAX`
- `ReflexSensitivityHint`：対象刺激と経験由来かを示す意味境界
- `ReflexSensitivityProvider`：Reflexが反応選択前にMemory/Ghostへ問い合わせる入口
- `MemoryEngine::reflexSensitivityHint()`：将来の危険記憶から修飾を返す本番境界
- Runtimeで `Memory → Ghost → Reflex` の参照経路を接続

現段階ではMemory側の具体的な危険分類・閾値・増減率・時間定数が未LOCKのため、返す値は `BASELINE` のみ。入口経路だけを本番構造として成立させている。

LOCK 51のmicroSD側については、保存形式やマウント方法を先行固定せず、Heart状態をmicroSD保存実装へ渡せる本番境界として `HeartMicroSdBackup` を追加した。

- `HeartBackupSaveHandler`
- `HeartBackupLoadHandler`
- `HeartMicroSdBackup::save/load`
- `HeartPersistence` からmicroSDバックアップ境界へ接続
- `HeartEngine` から現在Heart状態を保存／バックアップ候補を読込できる公開境界

microSDのファイル形式、パス、世代数、マウント方式、復旧優先順位は未LOCKのため固定していない。

さらにLOCK 20の状態別復元について、数値チューニングに踏み込まず、各Heart項目を別方式で復元する本番境界を追加した。

- `affection`：`PRESERVE_SAVED`
- `mood`：`DECAY_TOWARD_DYNAMIC_BASELINE`
- `curiosity`：`DECAY_TOWARD_DYNAMIC_BASELINE`
- `boredom`：`RECALCULATE_FROM_ELAPSED_CONTEXT`
- `sleepiness`：`RECALCULATE_FROM_TIME_RHYTHM`
- `attention`：`RESET_TO_DYNAMIC_BASELINE`

通常起動時は、具体ルールなしで確定できる `affection` のみ保存値を現在Heartへ戻す。その他5項目は、必要な経過時間・動的平常値・生活リズム等が未実装のため、機械的コピーせず `restorePending()` で未完了を明示する。

CoreS3実機で通常起動時に `[HEART][RESTORE] LOCK20 field plan: PENDING_STEP9_INPUTS` が出力されることを確認済み。

以上により、**Step 4の本番骨格は完了**とする。

## 5. 現在の神経構造

**Hardware → Device Driver → Adapter / Vision → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost / Memory / Pi5 → Behavior → Body**

Neuron＝意味を運ぶ。

Synapse＝どの層へ結ぶかを決める。

Ghost＝Heart / Memory / Time / Relationship / Behaviorの中核。

Camera Raw frameはGhostへ直接渡さず、Vision / Recognitionで意味化してからNerve Inputへ渡す。

ReflexはHeartやPi5を待たず先行でき、完了結果はRuntime → Ghost → Heart / Memoryへ戻せる。

危険・恐怖系の経験が将来成立した場合は、Memory / GhostからReflexへ感度修飾Hintを先行入力できる境界を持つ。

## 6. Heart Engineの扱い

Heart状態は、

- mood
- affection
- curiosity
- boredom
- sleepiness
- attention

を `0.0〜1.0` で保持する。

初回値はLOCK 28を使用する。

Heart Contextはread-only snapshotとし、1イベント処理中は開始時snapshotを固定する。

Heart永続化はLOCK 51に従い、**NVSを主保存、microSDをバックアップ**とする。

NVS主保存のストレージ境界、通常起動時の既存snapshot読込、microSDバックアップ境界、LOCK 20状態別復元Planまで実装済み。

LOCK 20の具体的な減衰率・再計算式は、必要な時間・生活履歴・動的平常値と接続してから決める。

## 7. Memory Engineの扱い

Step 4では、最初から以下へ拡張できる本番境界を持つ。

- 個別経験記憶
- 反復傾向記憶
- 特別長期記憶
- Relationshipへ影響する記憶

Semantic event / Reflex resultとHeart Contextを結び付けた `MemoryRecord` を渡せる境界を実装済み。

分類ルール・保持条件・保存件数はまだ固定しない。

## 8. Reflexの扱い

安全・強い反射はHeart / Pi5より先に動ける。

Reflex結果はHeart / Memoryへ戻る本番境界を実装済み。

繰り返し危険だった刺激による将来のReflex感度修飾については、具体的閾値・増減率を固定せず、Step 4では修飾入口の本番境界までを対象とする。

`Memory → Ghost → ReflexSensitivityProvider → ReflexLayer` の入口境界を実装済み。具体的な感度変化は未実装。

## 9. Face / 表情・単体DeskRoboの現在地

`FaceRenderer` は身体出力の本番境界として接続済み。

Neutralの目形状は、元の参照イメージに合わせて大きさ・輪郭を修正し、実機で「かわいい」方向へ改善した。完成形の表情設計は別工程とし、固定表情画像切替を完成形にはしない。

LOCK 52の第一実装として、`BehaviorEngine` に `MicroBehaviorFrame` を追加し、Heart ContextとTimeから連続的に、

- 開眼度
- 瞬き
- 視線X/Y
- 左右の微小差

を生成する経路を実装した。

CoreS3実機で以下を確認済み。

- `[DESKROBO] Standalone Heart/Time -> Behavior -> Face life loop started` が出る
- 待機中に瞬き・微小な視線移動が継続する
- Canvas描画へ変更後、画面全体の明滅は解消した
- Touch Driver / Adapterが実機で動作し、`TOUCH` SemanticNeuronが発生する
- IMU Driver / Adapterが実機で動作し、`PICKED_UP / SHAKE` SemanticNeuronが発生する
- IMUは状態遷移を `REST → LIFT_CANDIDATE → HELD → SETDOWN_CANDIDATE → REST` とし、SHAKE後のPICKED_UP二重解釈を抑制できた
- Touch / PICKED_UP / SHAKE はBehaviorへ接続され、顔反応の本番経路が成立している

`PICKED_UP` の見開き反応は現状やや分かりにくく、今後の表現調整対象とする。ただしLOCK 52の最低成立判定を妨げるものではない。

以上により、**CoreS3単体DeskRoboの最低成立条件は実機で通過**とする。

### Face表現部品の先行拡張（2026-09-18）

実装コミット: `heart-engine` の `052045e`。

神経接続後の反応を見分けやすくするため、ユーザー指示により `heart-engine` の身体表現境界を先行拡張した。LOCK 52/53の到達判定とCamera → ToF4Mの順序は変更しない。

- 現在の横長・大きめのNeutral輪郭、色、標準寸法を維持。
- 左右独立の幅／高さ／角丸／上まぶた被覆・傾き／下まぶた被覆を `EyeShape` として追加・整理。
- 目の中心間隔、視線と分離した一時揺れを連続パラメータとして接続。
- FaceRendererは有限値・範囲制限、時間ベースの形状補間、Canvas描画のみを担当。瞬き・揺れの時間波形とイベント判断はBehaviorが担当。
- Touch＝下まぶた、PICKED_UP＝幅・高さ、SHAKE＝左右差と減衰揺れの暫定マッピングを追加。係数は人格LOCKではない。
- M5Stack_RoboEyesの形状制御・補間・マスクの考え方を参照。ライブラリ導入／GPLソースコピー／random idle／mood判断の移植は行っていない。

検証：CoreS3向けArduinoビルドおよびホスト回帰テストを実施。ホストではNeutral寸法、独立まぶた、閉眼、範囲制限、更新周期差、時計周回、一時反応終了を確認。実機の見え方・センサー同時稼働は未確認であり、従来の実機通過実績と区別する。

実機確認は `heart-engine:firmware/cores3_base/README.md` の手順に従い、通常顔の維持、完全な瞬き、Touch／持ち上げ／SHAKEの違いと通常への復帰、画面明滅なし、Camera READY／I2C restored YES／Touch・IMU継続を確認する。カメラ撮像中のブロッキングは今回変更しておらず、短い反応が隠れないかも観察する。

## 10. CoreS3内蔵カメラ低次視覚の現在地

LOCK 53により、ToF4Mより先にCoreS3内蔵カメラをM5Stack側の「反射の目」として仕上げる。

2026-09-18、CoreS3実機で以下を確認済み。

- 内蔵GC0308を初期化できる
- 320x240 RGB565フレーム取得
- Camera使用前に内部I2Cを解放し、使用後に復元できる
- 毎回 `[SENSE][CAMERA] Internal I2C restored: YES` を確認
- Camera使用後もTouch / IMUがREADYのまま継続する
- 一回Probeだけでなく、約2秒周期で継続撮像できる
- `CoreS3CameraDriver → CameraFrameView → CameraVisionInput` の本番境界を実装
- Vision層でaverage lumaを取得できる

現在は、**「目が開き、見続ける」Device Driver → Vision経路まで実機成立**した段階。

2026-09-18、`heart-engine` の `5753d75` にVision → Semantic Neuron → Runtime/Synapse → Ghost/Heart入口・Memory・Behaviorの接続を実装した（実機未確認）。

- `MOTION_DETECTED / BRIGHTER / DARKER` を追加。画像はVision内に留める。
- 16×12の輝度差分、全体明暗変化の分離、初回・異常・長時間空白後の基準作り直し、送出間隔制限。
- PICKED_UP/SHAKE時は比較履歴を破棄し、自身の動きに伴う誤検知を抑制。ただし完全な自己運動補償ではない。
- 撮像終了・内部I2C復元後に意味イベントを送出。ブロッキング撮像後の身体更新に新しい時刻を使用。
- Behaviorは既存開眼度へ一時反映。Touch/IMU反応を優先し、新しい表情アニメは追加しない。
- RGB565のバイト順をesp32-cameraの仕様に合わせて修正。旧luma値との単純比較はしない。
- Heartの数値感情更新は引き続き未実装。神経配送と感情変化の完成を混同しない。

ホストテストで静止・明暗・局所変化・送出間隔・異常復帰・時計周回・Runtime配送・Memory記録・Behavior反映／優先を確認。CoreS3向けArduinoビルド成功（Flash 19%、静的RAM 11%）。次は実機で意味イベントの精度とTouch/IMU/Faceとの共存を確認する。

人物・物体・個人識別等の高次認識はPi5側の責務とし、CoreS3側へ持ち込まない。

## 11. 最初の外部感覚縦貫通

最初の外部感覚対象は **Unit ToF4M / U172** のまま維持する。

ただしLOCK 53により、開始条件は **CoreS3単体DeskRobo成立 ＋ CoreS3内蔵カメラ低次視覚経路成立後** とする。

ToF4Mの実機未検出問題はDevice Driver層の問題として保持し、Ghost / Runtimeへ持ち込まない。

ToF単独で人物を断定しない。

## 12. 次の実装

**現在の小ゴールは、実装したVision → Semantic Neuron接続の実機確認。**

次に、

- Runtime READYと23 bindingsを確認
- 静止時の誤発火、動く対象でMOTION_DETECTED、照明変化でBRIGHTER/DARKERを確認
- Camera READY / I2C restored YES / Touch・IMU / 生命ループの継続を確認
- 約2秒の撮像間隔、露出変化、影、自己運動による誤検知と短い動きの見落としを評価

する。人物・顔の検出、Kimの個人識別、方向追従はまだない。M5側の低次ターゲットとPi5側の人物同定は別工程とする。

内蔵首サーボは接続対象。現行独自実装には首Driverがまだなく、Behavior → Neck → Driverを接続する前に、Stack-chanのフィードバック・原点・角度／速度制限・自己運動時のVision抑制を確認する。方向情報のない画像変化を人追従や首振りへ直結しない。外付けサーボを新規に購入・接続する前提ではない。

この低次視覚経路を成立させた後、ToF4M / U172のDevice Driver層へ進む。

Faceの表現部品は先行拡張済み。次に実機で基準形の維持と反応の見やすさを確認する。完成表情の設計は引き続き別工程とし、Camera / ToF工程を止めるブロッカーにはしない。

## 13. 完成判定

Desktop Companion全体としては未完成。

ただし、**神経Runtime / Ghost本番骨格、Heart / Memory / ReflexのStep 4最小本番骨格、LOCK 52のCoreS3単体DeskRobo最低成立条件はCoreS3実機で成立済み。**

現在地は、**LOCK 53に従い、CoreS3内蔵カメラを「反射の目」として低次視覚のSemantic Neuronまで通す工程**とする。

## 14. LOCK 54｜今後の大工程

2026-09-18、現在地からDesktop Companion v1.0へ向かう大工程を以下でLOCKした。

**直近実機確認 → A「感じる」→ B「気分が変わる」→ C「自分から行動する」→ D「昨日と今日をつなぐ」→ E「賢くする」**

直近は、実装済みの **Camera → Vision → Semantic Neuron → Ghost / Memory / Behavior** をCoreS3実機で通す。

その後、

- A：感覚器官をSemantic Neuronまで意味化し、「感じる」を完成
- B：Heartへイベント・時間による実際の変化を接続し、「気分が変わる」を完成
- C：Heart / Memory / Time / 感覚から理由のある自発行動を選び、「自分から行動する」を完成
- D：Time / Memory / Relationshipで過去と現在をつなぎ、「昨日と今日をつなぐ」
- E：Pi5を高次認知・言語・高次Memoryとして接続し、「賢くする」

と進める。

既存Step番号は維持し、この大工程を上位ロードマップとして扱う。

## 15. LOCK 55｜M5単独生命成立を優先

2026-09-19、CoreS3実機で以下を確認した。

- Vision由来の `MOTION_DETECTED`
- `BRIGHTER`
- `DARKER`
- `Ghost=YES Heart=YES Memory=YES Behavior=YES`
- Vision動作中のIMUイベント
- Camera通常動作と内部I2C復元

これにより、直近小ゴールだった **Camera → Vision → Semantic Neuron → Ghost / Heart / Memory / Behavior の実機縦貫通** は通過扱いとする。

次工程はToF4M追加ではなく、M5Stackちゃん単独生命の成立を優先する。

現在の小ゴールは、

**既存の Camera / Touch / IMU を使い、Heartの実値変化を成立させること。**

その後、

**Heart / Time / Memory / 感覚 → Behavior → Face / Neck**

へ進み、自発行動を成立させる。

ToF4Mは最初の外部感覚であるLOCKを維持しつつ、M5単独生命成立後に接続する。

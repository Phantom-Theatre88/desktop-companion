# Desktop Companion — CURRENT STATUS

更新日: 2026-09-17

この文書は、このプロジェクトの「現在地」の正本である。

## 1. 最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## 2. 現在の正式実装方針

実装基盤はゼロベースを維持する。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

既存OSSを親Repoにしない。

Ghostは後付け機能ではなく、最初から本体中核として接続する。

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

2026-09-17、CoreS3実機でこのReflex結果フィードバック追加後のコードについて、コンパイル／書き込み／起動まで確認済み。

LOCK 51に従い、Heart永続化は **NVS主保存＋microSDバックアップ** とする。

NVS主保存の第一段として `HeartPersistence` を実装し、2026-09-17、CoreS3実機で以下を確認済み。

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

2026-09-17、`MemoryEngine.cpp` がCoreS3向けビルド／リンク対象に入り、書き込み・再起動後もRuntime READY、NVS既存snapshot読込、Face表示まで正常であることを実機確認した。

具体的な保持件数、分類条件、減衰率、長期保存条件は未LOCKのため実装していない。

続いてLOCK 46・50に従い、Memory/GhostからReflexへ経験由来の感度修飾を渡せる入口境界を実装した。

- `ReflexSensitivityDirection`：`BASELINE / HEIGHTEN / RELAX`
- `ReflexSensitivityHint`：対象刺激と経験由来かを示す意味境界
- `ReflexSensitivityProvider`：Reflexが反応選択前にMemory/Ghostへ問い合わせる入口
- `MemoryEngine::reflexSensitivityHint()`：将来の危険記憶から修飾を返す本番境界
- Runtimeで `Memory → Ghost → Reflex` の参照経路を接続

現段階ではMemory側の具体的な危険分類・閾値・増減率・時間定数が未LOCKのため、返す値は `BASELINE` のみ。入口経路だけを本番構造として成立させている。

2026-09-17、このReflex感度修飾入口追加後も、`m5stack_cores3` 向けに `ReflexLayer.cpp / MemoryEngine.cpp / DesktopCompanionRuntime.cpp` がビルド・リンク対象へ入り、CoreS3への書き込み、Hash検証、再起動まで正常に通過した。

LOCK 51のmicroSD側については、**保存形式やマウント方法を先行固定せず、Heart状態をmicroSD保存実装へ渡せる本番境界**として `HeartMicroSdBackup` を追加した。

- `HeartBackupSaveHandler`
- `HeartBackupLoadHandler`
- `HeartMicroSdBackup::save/load`
- `HeartPersistence` からmicroSDバックアップ境界へ接続
- `HeartEngine` から現在Heart状態を保存／バックアップ候補を読込できる公開境界

microSDのファイル形式、パス、世代数、マウント方式、復旧優先順位は未LOCKのため固定していない。

2026-09-17、`HeartMicroSdBackup.cpp / HeartPersistence.cpp / HeartEngine.cpp` が `m5stack_cores3` 向けビルド／リンク対象へ入り、CoreS3への書き込み、Hash検証、再起動まで正常に通過した。

さらにLOCK 20の状態別復元について、数値チューニングに踏み込まず、各Heart項目を別方式で復元する本番境界を追加した。

- `affection`：`PRESERVE_SAVED`
- `mood`：`DECAY_TOWARD_DYNAMIC_BASELINE`
- `curiosity`：`DECAY_TOWARD_DYNAMIC_BASELINE`
- `boredom`：`RECALCULATE_FROM_ELAPSED_CONTEXT`
- `sleepiness`：`RECALCULATE_FROM_TIME_RHYTHM`
- `attention`：`RESET_TO_DYNAMIC_BASELINE`

通常起動時は、具体ルールなしで確定できる `affection` のみ保存値を現在Heartへ戻す。その他5項目は、必要な経過時間・動的平常値・生活リズム等が未実装のため、機械的コピーせず `restorePending()` で未完了を明示する。

2026-09-17、CoreS3実機で通常起動時に `[HEART][RESTORE] LOCK20 field plan: PENDING_STEP9_INPUTS` が出力されることを確認した。

これは、LOCK 20の項目別復元Planが本番コードに存在し、Step 9で必要となる生活履歴・動的平常値・時間情報が未投入であることを明示するもの。係数や減衰率を推測実装していない。

以上により、**Step 4の本番骨格は完了**とする。

## 5. 現在の神経構造

**Hardware → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost / Memory / Pi5 → Behavior → Body**

Neuron＝意味を運ぶ。

Synapse＝どの層へ結ぶかを決める。

Ghost＝Heart / Memory / Time / Relationship / Behaviorの中核。

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

LOCK 20の具体的な減衰率・再計算式はStep 9側で必要な時間・生活履歴・動的平常値と接続してから決める。

## 7. Memory Engineの扱い

Step 4では、最初から以下へ拡張できる本番境界を持つ。

- 個別経験記憶
- 反復傾向記憶
- 特別長期記憶
- Relationshipへ影響する記憶

2026-09-17、この4系統を `MemoryLane` として実コード上に置き、Semantic event / Reflex resultとHeart Contextを結び付けた `MemoryRecord` を渡せる境界を追加した。

同日、CoreS3でビルド・書き込み・再起動確認まで通過した。

分類ルール・保持条件・保存件数はまだ固定しない。

## 8. Reflexの扱い

安全・強い反射はHeart / Pi5より先に動ける。

Reflex結果はHeart / Memoryへ戻る本番境界を実装済み。

繰り返し危険だった刺激による将来のReflex感度修飾については、具体的閾値・増減率を固定せず、Step 4では修飾入口の本番境界までを対象とする。

2026-09-17、`Memory → Ghost → ReflexSensitivityProvider → ReflexLayer` の入口境界を実装し、CoreS3向けビルド／書き込み／再起動まで確認済み。具体的な感度変化は未実装。

## 9. Face / 表情の現在地

`FaceRenderer` は現時点では身体出力の接続確認用として正常動作している。

ただし実機観察で、現行の丸い2眼Neutral表示は **「かわいくない」** というUI課題が確認された。

表情の完成はStep 8で扱う。固定画像切替を中心にせず、目の形・開き・傾き・視線・左右差・瞬き等の連続パラメータとHeart / 外界 / 直近履歴を結び付ける。Step 4を中断して表情調整へ逸れないが、この課題は消さない。

## 10. 最初の感覚縦貫通

最初の外部感覚対象は **Unit ToF4M / U172**。

実機未検出問題が残っているため、センサー固有問題をGhost / Runtimeへ持ち込まない。

Step 5では、

**Device → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost → Behavior → Body Output**

を一本通す。

ToF単独で人物を断定しない。

## 11. 次の実装

**Step 4は完了。次はStep 5へ移る。**

最初の対象は **Unit ToF4M / U172**。

まずDevice Driver層で、既知の実機未検出問題をGhostや神経Runtimeから分離したまま扱う。

同じ未検出問題または同系統問題が再発した場合は、LOCK 8の1回ループルールに従い、局所修正を続けず、I2C経路・電源・配線・アドレス・M5Unified側前提・Unit固有条件を根本から再確認する。

## 12. 完成判定

Desktop Companion全体としては未完成。

ただし、**神経Runtime / Ghost本番骨格、Heart / Memory / ReflexのStep 4最小本番骨格はCoreS3実機で成立。現在はStep 5として最初の外部感覚ToF4M / U172の縦貫通へ進む段階**にある。

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

**神経・シナプス層：本番骨格実装済み、Step 4最小本番実装を継続中**

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

現在、NVS主保存のストレージ境界と再起動時の既存snapshot読込確認まで完了している。

LOCK 20の状態別復元のうち、`mood / curiosity` の減衰量、`boredom / sleepiness` の再計算方法など具体式は未固定のため、推測で実装しない。

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

2026-09-17、`Memory → Ghost → ReflexSensitivityProvider → ReflexLayer` の入口境界を実装した。具体的な感度変化は未実装。

## 9. Face / 表情の現在地

`FaceRenderer` は現時点では身体出力の接続確認用として正常動作している。

ただし実機観察で、現行の丸い2眼Neutral表示は **「かわいくない」** というUI課題が確認された。

表情の完成はStep 8で扱う。固定画像切替を中心にせず、目の形・開き・傾き・視線・左右差・瞬き等の連続パラメータとHeart / 外界 / 直近履歴を結び付ける。Step 4を中断して表情調整へ逸れないが、この課題は消さない。

## 10. 最初の感覚縦貫通

最初の外部感覚対象は **Unit ToF4M / U172**。

ただし実機未検出問題が残っているため、センサー固有問題をGhost / Runtimeへ持ち込まない。

Step 5で、

**Device Driver → Adapter → PROXIMITY_* Semantic Neuron**

として再開する。

## 11. 次の実装

現在はStep 4を完了させる。

次は、今回追加した **Reflex感度修飾入口境界** がCoreS3でコンパイル・書き込み・起動できることを確認する。

確認後、Step 4で残る主要項目は次の2点。

1. HeartのmicroSDバックアップ境界
2. LOCK 20状態別復元で、実コード上どうしても必要になった未決事項だけ追加設計

係数・閾値・保存件数・減衰率等は先行固定しない。

Step 4完了後、Step 5としてToF4M / U172のDevice Driver層へ戻る。

## 12. 完成判定

Desktop Companion全体としては未完成。

ただし、**神経Runtime / Ghost本番骨格、Reflex結果の内面フィードバック、Heart NVS主保存の再起動復元境界、Memory本番境界までCoreS3実機で成立。現在はReflex感度修飾入口を実装し、実機確認へ進む段階**にある。

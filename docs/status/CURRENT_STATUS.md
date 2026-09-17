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

具体的な保持件数、分類条件、減衰率、長期保存条件は未LOCKのため実装していない。

このMemory追加分は**次回CoreS3ビルド確認待ち**。

## 5. 現在の神経構造

**Hardware → Device Driver → Adapter → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost / Memory / Pi5 → Behavior → Body**

Neuron＝意味を運ぶ。

Synapse＝どの層へ結ぶかを決める。

Ghost＝Heart / Memory / Time / Relationship / Behaviorの中核。

ReflexはHeartやPi5を待たず先行でき、完了結果はRuntime → Ghost → Heart / Memoryへ戻せる。

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

分類ルール・保持条件・保存件数はまだ固定しない。

## 8. Reflexの扱い

安全・強い反射はHeart / Pi5より先に動ける。

Reflex結果はHeart / Memoryへ戻る本番境界を実装済み。

繰り返し危険だった刺激による将来のReflex感度修飾については、具体的閾値・増減率を固定せず、Step 4では修飾入口の本番境界までを対象とする。

## 9. 最初の感覚縦貫通

最初の外部感覚対象は **Unit ToF4M / U172**。

ただし実機未検出問題が残っているため、センサー固有問題をGhost / Runtimeへ持ち込まない。

Step 5で、

**Device Driver → Adapter → PROXIMITY_* Semantic Neuron**

として再開する。

## 10. 次の実装

現在はStep 4を完了させる。

次は、今回追加した **MemoryRecord / MemoryLane本番境界** がCoreS3でコンパイル・書き込み・起動できることを確認する。

確認後は、Step 4で残る次の項目を順に処理する。

1. Reflex感度修飾の入口境界
2. HeartのmicroSDバックアップ境界
3. LOCK 20状態別復元で、実コード上どうしても必要になった未決事項だけ追加設計

係数・閾値・保存件数・減衰率等は先行固定しない。

Step 4完了後、Step 5としてToF4M / U172のDevice Driver層へ戻る。

## 11. 完成判定

Desktop Companion全体としては未完成。

ただし、**神経Runtime / Ghost本番骨格、Reflex結果の内面フィードバック、Heart NVS主保存の再起動復元境界までCoreS3実機で成立。現在はMemory本番境界の実装確認へ進んだ段階**にある。

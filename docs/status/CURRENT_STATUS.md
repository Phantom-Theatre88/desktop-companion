# Desktop Companion — CURRENT STATUS

更新日: 2026-09-11

この文書は、このプロジェクトの「現在地」の正本である。

## 1. 最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## 2. 現在の正式実装方針

2026-09-11、実装基盤をゼロベースへ正式変更した。

初期構成は、

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

のみ。

既存OSSを親Repoにしない。

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、RoboEyes、Xiaozhi、旧Yuki実装等は初期基盤へ入れない。

## 3. 旧実装の扱い

リポジトリに残っている `firmware/yuki/`、`patches/`、`upstream/`、旧ビルド用 `scripts/`、旧ToF神経コード等は **LEGACY参考資産** とする。

これらは削除しないが、現行実装の正本・親Repo・現在地には含めない。

旧実装で「動いた／部分達成」とされた内容は、新基盤で再確認するまで現行の完成判定へ引き継がない。

## 4. 現在のStep

**Step 0｜ゼロベース基準固定：完了**

**Step 1｜CoreS3素体確認：これから開始**

最初にM5Stackちゃんらしい機能を作らず、CoreS3公式環境だけで各機能を単独確認する。

確認順：

1. DISPLAY
2. TOUCH
3. IMU
4. PROXIMITY
5. MIC
6. SPEAKER
7. CAMERA
8. その他必要ハード

## 5. 現在の小ゴール

**IDE上でゼロから新規プロジェクトを作り、CoreS3 + Arduino + M5Unified + M5GFXだけで最初のハードウェア単体確認を成立させる。**

旧Yuki／StackChanコードから始めない。

## 6. その後の構造

CoreS3素体確認後、

**Hardware → Device Driver → Adapter → Nerve Input → M5Stackちゃん**

の順で独自構築する。

神経系上位構造は、

**外界 → 感覚器官 → Raw Perception → Perception Integration → Semantic Neuron → Synapse → Reflex / Heart / Memory / Pi5高次認知 → Behavior Selector → 身体出力**

を維持する。

## 7. 購入済みハードウェア

### M5Stack側

- Unit ToF4M / U172
- Unit TMOS PIR / U185
- ENV-Pro / U169
- Unit Hub / U006

### Raspberry Pi 5側

- SSD
- Raspberry Pi Camera Module 3 Wide

外部センサー実装はCoreS3素体確認後に進める。

最初の外部感覚縦貫通対象はToF4M / U172のままとする。

## 8. M5Stack／Pi5の役割

### M5Stack

身体＋生命維持できる低次脳＋反射系＋Heart Engineの常時稼働部分。

### Raspberry Pi 5

高次感覚野＋認知脳＋言語＋長期記憶。

Pi5停止時でもM5Stack側の基本生命活動を継続する。

## 9. 失効した旧現在地

以下は現在地として失効した。

- StackChan／Yukiファームを基盤とするESP-IDF 5.5.4改造路線
- 親Repoの基準動作確認
- 旧Yuki上での表情・反射・自発行動の部分達成判定
- ToF4M神経コードを旧Yukiへ直接つなぐ作業
- Xiaozhi経由会話を現行会話基盤とする判断

必要な知見はLEGACY参考資産として参照できるが、現行実装へ自動継承しない。

## 10. 次回最初に行うこと

CoreS3公式環境の新規プロジェクトを作成し、Step 1の最初の対象から実機確認する。

旧OSSや旧ファームを読み込んでから始めない。

## 11. 完成判定

現在は **新基盤の実装開始前／Step 0固定完了**。

Desktop Companion全体としては未完成。

# M5Stack Desktop Companion — 重要LOCK集

更新日: 2026-09-11

この文書は、プロジェクト全体へ常時適用する正式LOCKの索引である。

## LOCK 1｜最上位ゴール

M5Stackを、単なる音声AI端末ではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

として成立させる。

評価基準は機能数ではなく、**「そこにいる感じ」があるか**。

## LOCK 2｜実装基盤はゼロベース

既存OSSを親Repoにしない。

初期構成は以下だけとする。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、M5Stack_RoboEyes、Xiaozhi、旧Yuki実装等は初期基盤へ入れない。

最初にCoreS3公式APIだけで各ハード機能を単独確認し、素体の正常性を確立する。

以後の基本構造は、

**Hardware → Device Driver → Adapter → Nerve Input → M5Stackちゃん**

とする。

## LOCK 3｜旧実装の扱い

リポジトリ内の旧Yuki／StackChan系コード、patch、upstream、旧ビルドスクリプト等は削除せず、**LEGACY参考資産**として保持する。

ただし、

- 現行基盤ではない
- 正常動作の基準にしない
- 親Repoにしない
- 新コードへ自動的に移植しない
- 旧コードの都合で新設計を曲げない

ことを固定する。

## LOCK 4｜ハードウェア単体確認を先行

初期実装ではM5Stackちゃんらしい機能を作らない。

まず、

**DISPLAY → TOUCH → IMU → PROXIMITY → MIC → SPEAKER → CAMERA → その他必要ハード**

を一つずつ独立確認する。

各確認では、対象ハード以外の機能を極力混ぜず、どの層が正常かを明確にする。

## LOCK 5｜神経入力の共通仕様

製品固有の出力を、そのまま神経へ渡さない。

基本構造は、

**Device → Adapter → Nerve Input**

とする。

距離値・座標・加速度等の生値は前段で扱い、神経へは `NEAR`、`APPROACH`、`LEAVE`、`STROKE_DETECTED` 等の製品非依存の共通意味入力を渡す。

カメラは、

**Camera → Vision / Recognition → Nerve Input**

とする。

## LOCK 6｜Heart Engine

LLMそのものを「心」にしない。

Heart Engineは少なくとも、mood、affection、curiosity、boredom、sleepiness、attentionと、その時間変化・イベント変化・行動選択・表情反映・記憶接続を担当する。

Pi5やLLMが停止していても、M5Stack側の身体・感情・基本行動は生き続ける。

## LOCK 7｜実装順

LLMを先に中心へ置かない。

先に、

**見る → 感じる → 反応する → 気分が変わる → 自分から行動する**

を成立させ、その後に言葉を与える。

ただし実コードでは、その前にCoreS3のハードウェア単体確認を完了させる。

## LOCK 8｜デバッグの1回ループルール

`実行 → 修正 → 再実行` で同じ問題または同系統の問題が1回でも再発した時点で、局所修正を止める。

壊れている層・設計前提・データフロー・制御主体・資源競合・上位ゴールとの整合性を根本から見直す。

## LOCK 9｜OSS・ドナー利用

OSSは初期基盤へ入れない。

CoreS3公式環境と独自層が正常に成立した後、必要性が明確な機能だけを候補として評価する。

採用時は、

**ドナー機能 → 構造差分確認 → 適合設計 → 変換／再実装 → 実機確認 → 衝突確認**

の順を必須とする。

「ドナーで動く」は「Desktop Companionで適合する」と同義ではない。

## LOCK 10｜長期プロジェクト判断順

すべての設計・実装・デバッグ・完成判定は、

**最終ゴール → 全体構造 → 現在の小ゴール**

の順で判断する。

## LOCK 11｜M5Stack＋Raspberry Pi 5の役割

- M5Stack：身体＋生命維持できる低次脳＋反射系＋Heartの常時稼働部分
- Raspberry Pi 5：高次感覚野＋認知脳＋言語＋長期記憶

Pi5停止時でもM5Stack側の基本生命活動を継続する。

## LOCK 12｜視覚は2カメラ構成

- M5Stack側カメラ：反射の目
- Raspberry Pi Camera Module 3 Wide：理解する目

Pi5側高次視覚カメラは Raspberry Pi Camera Module 3 Wide を採用LOCKする。

## LOCK 13｜購入済み感覚器官

- M5Stack Unit ToF4M / U172
- M5Stack Unit TMOS PIR / U185
- M5Stack ENV-Pro / U169
- M5Stack Unit Hub / U006
- Raspberry Pi Camera Module 3 Wide
- Raspberry Pi 5用SSD

感覚器官の実装順は、CoreS3本体の単体確認完了後に改めて開始する。

## LOCK 14｜神経系全体構造

上位構造は、

**外界 → 感覚器官 → Raw Perception → Perception Integration → Semantic Neuron → Synapse → Reflex / Heart / Memory / Pi5高次認知 → Behavior Selector → 身体出力**

とする。

神経系は全体構造を先に固定し、その後に感覚1本ずつ縦貫通実装する。

最初の外部感覚の縦貫通対象は ToF4M / U172 とする。ただし、CoreS3素体確認より先には進めない。

## LOCK 15｜行動仲裁・対人Session・身体制御

優先順位は、

1. 安全・強い反射
2. 意味のある対人反応
3. Heart由来の状態表現
4. 自発行動
5. idle

とする。

対人関与は単発行動ではなく対人Sessionとして保持する。身体全体を一括占有せず、競合した身体出力だけBehavior Selectorが仲裁する。

## LOCK 16｜旧方針の失効

以下の旧方針は正式に失効する。

- `stack-chan-ko` を親Repoとする
- 旧Yuki／StackChanファームを現行実装の土台とする
- ESP-IDF 5.5.4旧Yuki構成を現行の必須基盤とする
- ToF4M縦貫通実装をCoreS3素体確認より先に進める
- RoboEyes、Xiaozhi、Dotty、stackchan-local等を最初から組み込む

このLOCK 16に反する過去文書・過去コード・会話記録は、歴史資料またはLEGACY参考資産としてのみ扱う。

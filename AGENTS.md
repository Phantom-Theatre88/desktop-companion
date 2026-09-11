# Desktop Companion — 作業開始時の必読ルール

このリポジトリで設計・実装・デバッグ・移植・レビューを行う前に、必ず以下をこの順で読む。

1. `docs/sacred/PROJECT_LOCKS.md`
2. `docs/sacred/M5Stack Desktop Companion 設計思想 v0.1.md`
3. `docs/sacred/M5Stack Desktop Companion 実装Step聖典 v0.3.md`
4. `docs/sacred/M5Stack Desktop Companion 感覚器官聖典 v0.1.md`
5. `docs/sacred/M5Stack Desktop Companion 神経系全体ブロック図 v0.1.md`
6. `docs/status/CURRENT_STATUS.md`
7. `LEGACY.md`

## 最上位ルール

判断順序は常に、

**最終ゴール → 全体構造 → 現在の小ゴール**

とする。

直近LOCKと聖典が食い違う場合は、より新しい正式LOCKを優先し、必要なら聖典側を同時に更新する。

## 現在の実装基盤 LOCK

現行のDesktop Companionは、既存Stack-chan系OSSを親Repoにしない。

初期実装基盤は以下だけとする。

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 本プロジェクト独自コード

`stack-chan-ko`、M5Stack/StackChan、Dotty StackChan、stackchan-local、RoboEyes、Xiaozhi、旧Yuki実装等は、初期基盤・親Repo・動作正本として使用しない。

リポジトリ内に残る `firmware/yuki/`、`patches/`、`upstream/`、旧ビルド用 `scripts/` 等は **LEGACY参考資産** とする。明示的な採用判断を行うまでは現行実装へ混ぜない。

## 実装順

最初にCoreS3公式環境だけでハードウェアの正常性を個別確認する。

**DISPLAY → TOUCH → IMU → PROXIMITY → MIC → SPEAKER → CAMERA → その他必要ハード**

その後、

**Hardware → Device Driver → Adapter → Nerve Input → M5Stackちゃん**

の順で独自構築する。

初期確認では人格、表情、Heart、神経系を先に載せず、ハードウェア単体の正常性を確立する。

## デバッグ

`実行 → 修正 → 再実行` で同じ問題または同系統の問題が1回でも再発した時点で、局所修正を止める。

壊れている層・設計前提・データフロー・制御主体・資源競合・上位ゴールとの整合性を根本から確認する。

## OSS・ドナー利用

外部OSSは初期基盤へ入れない。

CoreS3公式環境と独自層の正常性が確立した後、必要性が明確な機能だけを参考・部品候補として再評価する。その際は単純コピーせず、構造差分確認 → 適合設計 → 変換／再実装 → 実機確認 → 衝突確認を行う。

## 作業終了時

作業終了時には `docs/status/CURRENT_STATUS.md` を更新する。

## 正本

`docs/sacred/` を設計正本、`docs/status/CURRENT_STATUS.md` を現状正本とする。

会話・過去コード・LEGACY資産と食い違う場合、最新LOCKが明記された聖典とCURRENT_STATUSを優先する。

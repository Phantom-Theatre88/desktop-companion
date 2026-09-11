# Desktop Companion — LEGACY資産の扱い

更新日: 2026-09-11

このリポジトリには、2026-09-11以前のYuki／StackChanベース開発資産が残っている。

これらは削除せず、**歴史資料・参考実装**として保持する。

## LEGACYに分類する主な資産

- `firmware/yuki/`
- `patches/`
- `upstream/stackchan/`
- 旧StackChan向け `scripts/`
- `DEVPOST.md`
- Build Week向けスライド・デモ資料
- Xiaozhi統合コード
- ESP-IDF 5.5.4前提の旧ビルド構成
- 旧Yuki／StackChan上へ追加したToF4M・Neuron・Synapse実装

## やってはいけないこと

LEGACY資産を、

- 現行Desktop Companionの親Repoとみなす
- 現行ビルド手順とみなす
- 現在のStep達成として数える
- 新規コードの開始点にする
- 旧コードの都合で現行アーキテクチャを変更する
- 検証なしにコピーして現行コードへ入れる

ことを禁止する。

## 現行の正本

現行方針は以下を参照する。

1. `docs/sacred/M5Stack Desktop Companion 設計思想 v0.1.md`
2. `docs/sacred/M5Stack Desktop Companion 実装Step聖典 v0.3.md`
3. `docs/sacred/M5Stack Desktop Companion 感覚器官聖典 v0.1.md`
4. `docs/sacred/M5Stack Desktop Companion 神経系全体ブロック図 v0.1.md`
5. `docs/sacred/PROJECT_LOCKS.md`
6. `docs/status/CURRENT_STATUS.md`

## 現行実装基盤

- M5Stack CoreS3
- Arduino
- M5Unified
- M5GFX
- 独自コード

親Repoは置かない。

## LEGACYから再利用する場合

必要性が明確になった時だけ、

**構造差分確認 → 適合設計 → 変換／再実装 → 実機確認 → 衝突確認**

を行う。

LEGACY側で動いていたことは、現行基盤での正常動作を保証しない。

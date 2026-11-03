# Step 0 素体起動記録

日付: 2026-11-03  
基準: 実装Step 聖典 v1.0  
スケッチ: `firmware/step0_cores3_minimal/step0_cores3_minimal.ino`

## 結果

| 確認 | 結果 | 記録 |
|---|---|---|
| 対象ボードのCoreS3コンパイル | 合格 | FQBN `m5stack:esp32:m5stack_cores3`。Flash 518,287 / 3,145,728 bytes（16%）、global RAM 25,768 / 327,680 bytes（7%）。 |
| USBポートの検出 | 合格 | `/dev/cu.usbmodem5101` が列挙された。 |
| CoreS3への書込み | 未実施 | ポートを開く際にOSが `Operation not permitted` を返した。 |
| 画面表示 | 未確認 | 本スケッチの実機起動を観測できていない。 |
| シリアル起動ログ | 未確認 | `[STEP0] CoreS3 minimal base started` を実機から読めていない。 |

## 判定

**Step 0は未完了。** CoreS3向けビルドは通ったが、ファームウェア書込みと実機確認の証拠がない。ビルド成功を実機動作とみなさない。

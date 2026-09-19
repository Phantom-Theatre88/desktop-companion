# CoreS3 Camera Viewer

CoreS3内蔵GC0308カメラの**生映像をMacのブラウザで確認するための診断専用スケッチ**。

本番の `firmware/cores3_base` とは分離している。Ghost / Heart / Behavior / Touch / IMU の診断ではなく、まず「カメラが実際に何を見ているか」を確認するためだけに使う。

## 使い方

1. Arduino IDEで `cores3_camera_viewer.ino` を開く。
2. Board / Portは、通常のCoreS3書き込み時と同じものを選ぶ。
3. 検証 → 書き込み。
4. MacのWi-Fiから次へ接続する。
   - SSID: `DeskRobo-Camera`
   - Password: `deskrobo`
5. Safari / Chromeで `http://192.168.4.1/` を開く。

画面には、
- CoreS3 Cameraのライブ映像
- 現在Visionで使っている16×12相当のグリッド

を重ねて表示する。

## 注意

- このViewer中はCoreS3のカメラを連続使用するため、内部I2Cをカメラへ渡したままにする。
- したがってTouch / IMU / Ghost / Faceの本番動作確認には使わない。
- 診断後は通常の `firmware/cores3_base/cores3_base.ino` を再度書き込めば本番状態へ戻る。
- MacはCoreS3のSoftAPへ接続するので、その間このWi-Fi経由ではインターネットへ出られない場合がある。

## 目的

現在のVision方向推定を調整する前に、

1. 実際の画角
2. 左右反転の有無
3. 自動露出・ゲイン変化
4. 手や影がどの範囲を占めるか
5. 2秒差分で広域変化になりそうな要因

を生映像で直接確認する。

次段階では、このViewerへ16×12 motion maskのオーバーレイを追加できる。

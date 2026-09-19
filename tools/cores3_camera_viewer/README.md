# CoreS3 Camera Viewer

CoreS3内蔵GC0308カメラの**生映像をMacのブラウザで確認するための診断専用スケッチ**。

本番の `firmware/cores3_base` とは分離している。Ghost / Heart / Behavior / Touch / IMU の診断ではなく、まず「カメラが実際に何を見ているか」を確認するためだけに使う。

## 推奨：通常LANへ参加して使う

Wi-Fi資格情報はGitHubへ保存しない。

1. `wifi_secrets.example.h` を同じフォルダ内で `wifi_secrets.h` にコピーする。
2. `wifi_secrets.h` の `WIFI_SSID` / `WIFI_PASSWORD` を自分のWi-Fiへ書き換える。
3. `wifi_secrets.h` は `.gitignore` 対象なのでコミットされない。
4. Arduino IDEで `cores3_camera_viewer.ino` を開き、M5CoreS3へ書き込む。
5. シリアルモニタに表示される `[VIEWER] Open: http://192.168.x.x/` をMacのSafari/Chromeで開く。

この方式ならMacは普段のWi-Fi接続を維持したまま、ChatGPTと会話しながらViewerを見られる。

## 安全側の動作

- ViewerはM5からクラウドや外部サービスへ映像を送信しない。
- HTTP Viewerは同一LAN内のローカルIPで配信する。
- Wi-Fi資格情報はローカルの `wifi_secrets.h` のみ。
- LAN接続が15秒以内に成功しなければ、従来のSoftAP `DeskRobo-Camera` へ自動フォールバックする。

フォールバック時：
- SSID: `DeskRobo-Camera`
- Password: `deskrobo`
- 通常IP: `http://192.168.4.1/`

## Viewer表示

- CoreS3 Cameraのライブ映像
- 現在Visionで使っている16×12相当のグリッド

## 注意

- Viewer中はCoreS3カメラを連続使用するため、内部I2Cをカメラへ渡したままにする。
- Touch / IMU / Ghost / Faceの本番動作確認には使わない。
- 診断後は `firmware/cores3_base/cores3_base.ino` を再書き込みして本番状態へ戻す。
- 同一LAN上の端末からViewerへアクセスできるため、信頼できるLAN上で診断用途に限定して使う。

## 目的

1. 実際の画角
2. 左右反転の有無
3. 自動露出・ゲイン変化
4. 手や影がどの範囲を占めるか
5. 2秒差分で広域変化になりそうな要因

を生映像で直接確認する。

次段階では、このViewerへ16×12 motion maskのオーバーレイを追加できる。

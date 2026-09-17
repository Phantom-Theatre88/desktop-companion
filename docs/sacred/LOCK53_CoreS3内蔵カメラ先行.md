# LOCK 53｜CoreS3内蔵カメラをToF4Mより先に仕上げる

更新日: 2026-09-18

## 決定

CoreS3単体DeskRoboの最低成立後、最初の外付け感覚であるToF4M / U172へ進む前に、**CoreS3内蔵カメラを「M5Stack側の反射の目」として先に成立させる**。

これはToF4Mを最初の「外部感覚縦貫通対象」とする既存LOCKを破棄するものではない。

- CoreS3内蔵カメラ：本体内蔵感覚として先行
- ToF4M / U172：内蔵感覚成立後の最初の外部感覚縦貫通対象

と整理する。

## 理由

最上位ゴールはセンサー数を増やすことではなく、

**「机の上に常にいて、周囲を感じ、反応し、気分が変わり、少しずつ関係性が育つ相棒」**

を成立させることである。

またLOCK 52では、外付けUnitより先にCoreS3本体だけで利用できる感覚をDeskRoboへ接続することを優先している。

視覚についてはLOCK 12で、

- M5Stack側カメラ＝反射の目
- Raspberry Pi Camera Module 3 Wide＝理解する目

と役割分担が確定している。

したがって、CoreS3単体DeskRobo成立後は、ToF4M追加より先にCoreS3内蔵カメラの低次視覚経路を成立させる。

## 実装経路

CoreS3内蔵カメラは、製品固有の生画像をGhostへ直接渡さない。

基本経路は、

**Camera Hardware → Device Driver → Vision / Recognition → Nerve Input → Semantic Neuron → Synapse → Reflex / Ghost → Behavior → Body Output**

とする。

M5Stack側で扱うのは常時稼働可能な低次視覚・反射に必要な意味までとし、人物・物体・状況の高次理解はPi5側へ分離する。

## 内蔵カメラ先行工程の最低到達点

ToF4Mへ進む前に、少なくとも以下を成立させる。

1. CoreS3内蔵カメラを独自Device Driver層から安定して初期化・撮像できる。
2. Camera使用時の内部I2C資源競合を処理し、既存Touch / IMU等を壊さない。
3. 一回限りのProbeではなく、DeskRobo生命ループを維持したまま周期的に撮像できる。
4. Raw frameをGhostへ直接渡さず、Vision層へ渡す本番境界を持つ。
5. Vision層で低次視覚情報を意味化し、将来Semantic Neuronへ接続できる。
6. Pi5が無くてもM5Stack側の「反射の目」として基本視覚が継続できる。

## 現在確認済み

2026-09-18時点でCoreS3実機にて、

- GC0308から320x240 RGB565フレーム取得
- Camera使用前後の内部I2C解放／復元
- Touch / IMUの継続
- 周期撮像
- Device Driver → CameraFrameView → Vision層
- Vision層でaverage lumaを取得

まで確認済み。

## ToF4Mとの関係

ToF4M / U172は引き続き、**最初の外部感覚縦貫通対象**とする。

ただし開始条件を、

**CoreS3単体DeskRobo成立 ＋ CoreS3内蔵カメラの低次視覚経路成立後**

とする。

このLOCKはLOCK 52を補強し、旧Step 5 / Step 6のうち「ToF4Mより後に視覚を置く」順序を上書きする。

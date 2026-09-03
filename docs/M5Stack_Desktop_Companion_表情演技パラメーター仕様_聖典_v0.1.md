# M5Stack Desktop Companion
## 表情・演技パラメーター仕様 聖典 v0.1

**Status: LOCK 🔐**  
**位置づけ:** 目の表情20種LOCK版、イベント演出パーツ設計 聖典 v0.1、演技モーション設計 聖典 v0.2 を実装可能な共通パラメーター体系へ落とし込むための基準書。

---

# 1. 基本方針

M5Stack Desktop Companion の演技は、固定画像や固定動画を大量に持つのではなく、  
**共通パラメーターの組み合わせで生成する。**

1つの演技は、以下の要素の組み合わせで構成する。

- EyeParameters
- Lid / Blink Parameters
- GazeParameters
- ColorParameters
- ScreenParameters
- NeckParameters
- TimingParameters
- EventParameters

これにより、同じ「驚き」「喜び」「眠気」でも強弱や個性を変えられる。

---

# 2. 設計原則

## 2.1 形＝意味
目の形状・まぶた・左右差・視線で、感情や意図の意味を作る。

## 2.2 色＝感情の温度
色は感情の意味そのものではなく、温度感・ニュアンスを補助する。

## 2.3 Glow / Bloom＝感情の強さ
発光量や滲み量は感情強度や興奮度の補助として使う。

## 2.4 画面描写＝物理2軸の限界を補う
物理首は Yaw / Pitch の2軸。Rollは持たない。
「のぞき込む」「引く」「沈む」「弾む」などは、画面内の顔位置・拡大縮小・揺れで補う。

## 2.5 時間変化も表情の一部
同じ形でも、速く出る・ゆっくり戻る・少し止まる、で意味が変わる。
Attack / Hold / Release を必須の設計対象とする。

---

# 3. 正式パラメーター辞書

| ID | グループ | パラメーター | 役割 |
|---|---|---|---|
| P01 | Eye | `eye_width` | 目の横幅 |
| P02 | Eye | `eye_height` | 目の高さ |
| P03 | Eye | `eye_roundness` | 目の角丸量 |
| P04 | Eye | `left_rotation` | 左目の回転角 |
| P05 | Eye | `right_rotation` | 右目の回転角 |
| P06 | Eye | `asymmetry` | 左右差の総量 |
| P07 | Eye | `eye_spacing` | 両目間隔 |
| P08 | Eye | `left_eye_y` | 左目の上下位置 |
| P09 | Eye | `right_eye_y` | 右目の上下位置 |
| P10 | Lid | `eyelid_open` | まぶた開度 |
| P11 | Lid | `upper_lid_angle` | 上まぶた角度 |
| P12 | Lid | `lower_lid_amount` | 下まぶた量 |
| P13 | Blink | `blink_interval` | 瞬き間隔 |
| P14 | Blink | `blink_duration` | 1回の瞬き時間 |
| P15 | Blink | `blink_left_delay` | 左目の瞬き遅延 |
| P16 | Blink | `blink_right_delay` | 右目の瞬き遅延 |
| P17 | Blink | `squint_amount` | 目を細める量 |
| P18 | Gaze | `gaze_x` | 視線左右位置 |
| P19 | Gaze | `gaze_y` | 視線上下位置 |
| P20 | Gaze | `gaze_speed` | 視線移動速度 |
| P21 | Gaze | `gaze_hold` | 視線停止時間 |
| P22 | Gaze | `gaze_overshoot` | 視線の行き過ぎ量 |
| P23 | Gaze | `micro_gaze` | 微細視線動作量 |
| P24 | Color | `base_color` | 基本色 |
| P25 | Color | `secondary_color` | 補助色 |
| P26 | Color | `gradient_amount` | グラデーション量 |
| P27 | Color | `brightness` | 明るさ |
| P28 | Color | `glow_amount` | 発光量 |
| P29 | Color | `bloom_amount` | 滲み量 |
| P30 | Color | `edge_softness` | エッジの柔らかさ |
| P31 | Color | `pulse_amount` | 発光の脈動幅 |
| P32 | Color | `pulse_speed` | 発光の脈動速度 |
| P33 | Screen | `face_x` | 顔全体の左右位置 |
| P34 | Screen | `face_y` | 顔全体の上下位置 |
| P35 | Screen | `face_scale` | 顔全体の拡大縮小 |
| P36 | Screen | `face_rotation` | 画面内の顔全体回転 |
| P37 | Screen | `screen_shake` | 画面揺れ量 |
| P38 | Screen | `screen_bounce` | 弾み量 |
| P39 | Screen | `screen_overshoot` | 画面動作の行き過ぎ量 |
| P40 | Screen | `screen_drift` | ゆっくり漂う量 |
| P41 | Screen | `screen_flicker` | 短時間のちらつき量 |
| P42 | Neck | `neck_yaw` | 首の左右角度 |
| P43 | Neck | `neck_pitch` | 首の上下角度 |
| P44 | Neck | `neck_speed` | 首の移動速度 |
| P45 | Neck | `neck_acceleration` | 首の加速感 |
| P46 | Neck | `neck_hold` | 首位置の保持時間 |
| P47 | Neck | `neck_return_speed` | 基準位置へ戻る速度 |
| P48 | Neck | `neck_repeat` | 首動作の繰り返し回数 |
| P49 | Neck | `neck_micro_motion` | 首の微細動作量 |
| P50 | Timing | `attack` | 演技の立ち上がり時間 |
| P51 | Timing | `hold` | 演技状態の維持時間 |
| P52 | Timing | `release` | 基準状態へ戻る時間 |
| P53 | Timing | `delay` | 演技開始までの遅延 |
| P54 | Timing | `repeat_count` | 繰り返し回数 |
| P55 | Timing | `loop_interval` | 反復間隔 |
| P56 | Timing | `easing` | 動作補間曲線 |
| P57 | Event | `event_id` | イベント種別 |
| P58 | Event | `event_x` | イベント表示位置X |
| P59 | Event | `event_y` | イベント表示位置Y |
| P60 | Event | `event_scale` | イベントサイズ |
| P61 | Event | `event_rotation` | イベント回転 |
| P62 | Event | `event_alpha` | イベント透明度 |
| P63 | Event | `event_glow` | イベント発光量 |
| P64 | Event | `event_duration` | イベント表示時間 |
| P65 | Event | `event_motion_type` | イベントの動き方 |

---

# 4. パラメーター群

## A. EyeParameters
P01〜P09

目的：
- 目そのものの形
- 左右差
- 傾き
- 位置関係

## B. Lid / Blink Parameters
P10〜P17

目的：
- 眠気
- 安心
- 不機嫌
- 驚き
- 自然な瞬き
- 左右差のある瞬き

## C. GazeParameters
P18〜P23

目的：
- 対象を見る
- ちらっと見る
- 見つめる
- 探す
- 二度見
- 微細な存在感

## D. ColorParameters
P24〜P32

目的：
- 感情の温度
- 感情強度
- 発光感
- 質感

## E. ScreenParameters
P33〜P41

目的：
- 物理2軸ではできない演技の補完
- 接近感
- 後退感
- 沈み
- 弾み
- 驚き
- 微細な生命感

## F. NeckParameters
P42〜P49

目的：
- Yaw / Pitch の実首モーション
- 追従
- うなずき
- 否定
- 驚き
- 眠気
- 微細モーション

### LOCK
現行ハードウェアでは Roll パラメーターを持たない。

## G. TimingParameters
P50〜P56

目的：
- 演技の時間設計
- 感情の立ち上がり
- 維持
- 余韻
- 反復

## H. EventParameters
P57〜P65

目的：
- 感情イベント
- 状態イベント
- カレンダーイベント
- 天気イベント
の表示制御。

---

# 5. 演技レシピの記述形式

各演技は将来、以下の形式で定義する。

```yaml
motion_id: Mxx
name: 演技名

eye:
  eye_width:
  eye_height:
  eye_roundness:
  asymmetry:

lid:
  eyelid_open:
  upper_lid_angle:
  lower_lid_amount:

gaze:
  gaze_x:
  gaze_y:
  gaze_speed:
  gaze_hold:

color:
  base_color:
  secondary_color:
  glow_amount:
  bloom_amount:

screen:
  face_x:
  face_y:
  face_scale:
  screen_shake:

neck:
  neck_yaw:
  neck_pitch:
  neck_speed:

timing:
  attack:
  hold:
  release:
  easing:

event:
  event_id:
  event_duration:
  event_motion_type:
```

---

# 6. 例：驚き

「驚き」は固定アニメーションではなく、以下のような組み合わせで表現する。

- P02 `eye_height` を大きく
- P10 `eyelid_open` を大きく
- P27 `brightness` を上げる
- P28 `glow_amount` を強く
- P35 `face_scale` を一瞬小さく
- P37 `screen_shake` を少量
- P43 `neck_pitch` を反射的に変化
- P44 `neck_speed` を速く
- P50 `attack` を短く
- 必要に応じて P57 `event_id` = びっくり

---

# 7. 例：のぞき込む

- P02 `eye_height` を少し大きく
- P18/P19 `gaze_x/y` を対象へ
- P20 `gaze_speed` は中程度
- P35 `face_scale` を少し大きく
- P42/P43 `neck_yaw/pitch` を対象方向へ
- P50 `attack` は急すぎない
- P51 `hold` をやや長め

物理的な前後移動はないため、画面拡大で接近感を補う。

---

# 8. 例：引く

- 驚きまたは警戒系の目
- P35 `face_scale` を少し小さく
- P33/P34 `face_x/y` をわずかに退避方向へ
- P42/P43 `neck_yaw/pitch` を逃がす
- P50 `attack` を短く
- P52 `release` はやや遅く

---

# 9. 現段階で決めないもの

このv0.1では、以下はまだLOCKしない。

- 各パラメーターの最終数値範囲
- 単位
- 正規化方式（0〜1 / 0〜100 / 実角度など）
- 各演技Mxxへの具体数値
- サーボ個体差補正値
- 実機上の最終速度
- 色の最終RGB値
- easing の最終種類

これらは実機検証を伴う次段階で決める。

---

# 10. LOCK事項 🔐

1. 演技は固定画像・固定動画の大量保持ではなく、パラメーター駆動を基本とする。
2. 正式パラメーター辞書を P01〜P65 とする。
3. 演技は Eye / Lid-Blink / Gaze / Color / Screen / Neck / Timing / Event の8群で扱う。
4. 現行首は2軸（Yaw / Pitch）前提とし、Rollは持たない。
5. 物理首で不足する表現は ScreenParameters で補う。
6. 目の形を感情意味の主役とする。
7. 色は感情の温度を補助する。
8. Glow / Bloom は感情強度を補助する。
9. Attack / Hold / Release を演技設計の正式要素とする。
10. EventParameters は主役ではなく、目・首・画面演技の補助とする。
11. 各演技の具体数値は、この聖典の次工程で割り当てる。
12. 将来の演技追加は、原則として新しい固定動画ではなく既存パラメーターの組み合わせを優先する。

---

# 11. 次工程

半LOCK済み工程に従う。

1. イベント演出パーツ設計 v0.1 🔐
2. 演技モーション設計 v0.2 🔐
3. **表情・演技パラメーター仕様 v0.1 ← 今回LOCK**
4. 表情＋首＋画面＋イベントの組み合わせルール
5. 各演技への具体パラメーター割当
6. 実機テスト
7. 調整・正式版LOCK

---

**Document:** `M5Stack_Desktop_Companion_表情演技パラメーター仕様_聖典_v0.1.md`  
**Status:** 🔐 LOCK

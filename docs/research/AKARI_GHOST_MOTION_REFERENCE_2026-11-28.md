# Ghost / NERVA 動作知識再構成 v0.1
既存LOCK継承。技術採用・仕様変更ではなく実装用参考データ。Ghostの状態を決める既存責務は変更しない。
## 取得・確認
- Kugutu README 7a75ed5cd2a65793e7fa256c33978365465b8549、MIT LICENSE d62ae8ed9b0d843897c2855ee2bc68f6362a3cc4。意味スロット、lookAt/setEmotion/playGesture、呼吸・瞬き、JSON+SVG、素材切替。全身人体品質は未実証。
- Procedural Bot Animations README 7422bad86333a0c4dfc03343661e5bbff54f34cb、MIT LICENSE 16617c582d2c141a78c401f4bc4413574c37ac34、motion-cookbook f86a30dfada9e840e73041bea24860fe3f84a575、gaze.ts 5d5750a17aa707e75524cd198fec0a000ba499e9。視線の急速移動・追従・微小変動、瞬き、保持と緩和、感情混合、二次動作。マスコット用数値を人体の正解としない。
- Hiyori m01 motion3 JSON 5adb2295cfbb60bc94e4b2049b8381ae1a649ea6、4.7秒、30fps、31曲線、135区間、374点。頭・眼・体幹・肩・腕・髪・パーツ透明度の同期構成を確認。原曲線は転載しない。個別利用条件確認が必要。
- Greta README c2ec924954bfd81c9bae1bb084b4f322fabfe9a7、master GPLv3、master-lgpl LGPLv3。相槌・うなずき・発話交替・表情連動、重み別途取得。日本語適合未検証。
- BEAT LICENSE a64c51a16f45d75f498119243a5f183c670fa46d はCC BY-NC 4.0。人体動作の研究参考。生データ未取得。
- Asap/BML は動作同期と割込みの設計参考。実装とライセンス未検証。
## 再構成
JSON: docs/research/AKARI_GHOST_MOTION_REFERENCE_2026-11-28.json
自律（瞬き・呼吸）、反応（視線・相槌）、持続姿勢、意識行動、二次動作を同時合成。Ghostは意味と内部状態、NERVAは競合調停・タイミング、Bodyは既定三要素で描画し実結果返却。動作チャンネルごとの競合解決、意識行動中も自律反応を継続、未対応の指令は未対応と返す。
## 次の検証
既存Ghost/NERVAの実インターフェースを確認し、外部コードの取り込み前に対応表を照合。呼吸と視線と意識行動の同時制御、割込み、復帰を実Bodyで検証。今回JSONは参考マッピングであり実行済みモーションデータではない。STEP1未合格、既存LOCK変更なし。
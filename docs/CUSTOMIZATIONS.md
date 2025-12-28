upstream_ref: upstream/main
upstream_commit: 3381b38073185dfff9cd059e4a60d962d9ebd100
custom_commit: 4bf608ffd41a5dd808882750eab3b9f34324c99d

# カスタマイズ概要

## WebConfig
### P5General OLED制御をWebConfigに追加
- 目的: P5General利用時のOLED負荷をWeb UIから抑制・停止できるようにするため。
- 変更点: DisplayConfigページに「P5General OLED間引き設定」「P5General時はOLED更新を抑制」「P5GeneralモードではOLEDを無効化」のスイッチとプルダウンを追加し、説明リストを表示。【F:docs/_customization_evidence/full.patch†L2214-L2369】
- 影響範囲: WebConfig UIと送信されるdisplayOptions値。
- 関連ファイル: www/src/Pages/DisplayConfig.jsx、各言語のDisplayConfigロケール。【F:docs/_customization_evidence/changed_files_name_status.txt†L31-L48】
- 設定項目: disableWhenP5General, p5GeneralOledSafeMode, p5GeneralOledMode。
- 注意点: P5General専用設定のため他モードでは無効。文言はロケール側に集約されている（要多言語反映）。【F:docs/_customization_evidence/full.patch†L13-L40】
- 関連コミット: b500b821, 626fe3d1, 4cdde995, f81464cb, ad98695b。【F:docs/_customization_evidence/commits.txt†L1-L7】

### Grid Gradientプリセット設定UIの追加
- 目的: コントローラ内蔵のGrid GradientアニメーションをWebConfigから配色・速度・プリセットを設定できるようにするため。
- 変更点: LEDConfigページにGrid Gradientセクションを新設し、4色グラデーション、押下色、レバー/ケース色、速度、プリセット、ケースLEDオフセット入力を追加。色選択は共通ピッカーで操作。送信時に数値・配列を整形してAPIへ渡す。表示用の速度ラベルを各ロケールに追加。【F:docs/_customization_evidence/full.patch†L2377-L2764】【F:docs/_customization_evidence/changed_files_name_status.txt†L34-L48】
- 影響範囲: WebConfig LED設定UIと保存されるanimationOptions/ledOptions。
- 関連ファイル: www/src/Pages/LEDConfigPage.jsx、www/src/Services/WebApi.js、www/src/Services/Utilities.ts、www/src/Locales/*/LedConfig.jsx。【F:docs/_customization_evidence/changed_files_name_status.txt†L34-L53】
- 設定項目: gridGradientColorA/B/C/D, gridButtonPressColor, gridGradientSpeed, gridGradientPreset, gridLeverNormalColor, gridLeverPressColor, gridCaseNormalColor, gridCaseLeverPressColor, gridCaseUp/Down/Right/LeftIndices。
- 注意点: ケースLEDインデックスは4要素までを数値または空欄で入力し、負値は送信前に無視される。プリセットは0/1のみ。 
- 関連コミット: e6515608, 9c1b4f35, 84d6ccba, 8281e143, c32fa3d9, 3399e96b, c08bfe9e, 5f1f7a92, 80301f7c, d0efd397, a720899e, 8225eb8a, e1587bdd, 5ad356aa, acf094ca, 34bd00ce, 9948046d, 5ec3b452。【F:docs/_customization_evidence/commits.txt†L8-L29】

## OLED
### P5GeneralモードのOLED描画間引きと安全策
- 目的: PS5 P5Generalドライバ使用時にI2C OLED更新が入力処理と競合するのを防ぎ、必要に応じて表示を無効化するため。
- 変更点: DisplayAddonがP5Generalモードを検出し、disableWhenP5Generalやp5GeneralOledModeに応じてリフレッシュ間隔・ページ分割・アイドル再描画周期・I/O遅延を動的に設定。入力変化で即時描画、認証処理中のdefer、部分描画リセットを追加。【F:docs/_customization_evidence/full.patch†L473-L614】
- 影響範囲: P5General入力モードのOLEDレンダリング周期とI2Cトラフィック。
- 関連ファイル: src/addons/display.cpp、headers/addons/display.h、headers/drivers/p5general/P5GeneralDriver.h、src/drivers/p5general/P5GeneralDriver.cpp、docs/P5General_OLED_notes.md。【F:docs/_customization_evidence/changed_files_name_status.txt†L2-L3】【F:docs/_customization_evidence/changed_files_name_status.txt†L11-L12】【F:docs/_customization_evidence/changed_files_name_status.txt†L17-L24】
- 設定項目: disableWhenP5General, p5GeneralOledSafeMode, p5GeneralOledMode（0:Safe/1:Low/2:Medium/3:High）。
- 注意点: WebConfigに保存されていない場合はMedium(2)とSafeモードがデフォルト。P5General以外では従来の8ms周期を維持。詳細なチューニング値はP5General_OLED_notes.mdを参照。必要に応じ再確認（要確認）。【F:docs/_customization_evidence/full.patch†L13-L40】【F:docs/_customization_evidence/full.patch†L509-L543】
- 関連コミット: b500b821, 626fe3d1, 4cdde995, f81464cb。【F:docs/_customization_evidence/commits.txt†L1-L7】

## LED
### Grid Gradientアニメーションの追加
- 目的: 4色波形を用いた新しいグリッド型LEDアニメーションを内蔵し、ボタン/レバー/ケースの状態に応じた配色と速度を制御するため。
- 変更点: AnimationEffectsにEFFECT_GRID_GRADIENTを追加し、GridGradientクラスで列ごとの波形生成、プリセットB配置、押下/レバー/ケース色の反映、速度列挙を実装。Case RGBがGrid Gradient中は既存アンビエント処理を無効化。プリセットBは7x4配置をハードコードし、速度・間隔のテーブルを定義。【F:docs/_customization_evidence/full.patch†L440-L454】【F:docs/_customization_evidence/full.patch†L760-L980】【F:docs/_customization_evidence/full.patch†L617-L624】
- 影響範囲: LEDアニメーション選択時の挙動、Case RGBとの連携、設定保存フォーマット。
- 関連ファイル: proto/enums.proto, proto/config.proto, src/animationstation/effects/gridgradient.cpp, headers/animationstation/effects/gridgradient.h, src/animationstation/animationstation.cpp, src/addons/neopicoleds.cpp。【F:docs/_customization_evidence/changed_files_name_status.txt†L6-L20】【F:docs/_customization_evidence/changed_files_name_status.txt†L15-L18】
- 設定項目: GridGradientSpeed列挙（very slow/slow/normal/fast/very fast）、グラデーション4色、押下色、レバー/ケース色、ケースLEDオフセット、プリセット選択。
- 注意点: ケースLEDのオフセットはcaseRGBIndexからの相対値で、実行時に範囲外は無視。プリセットBは固定マスク配置。 
- 関連コミット: e6515608以降のGrid Gradient系列。【F:docs/_customization_evidence/commits.txt†L8-L29】

## 入力
### Stick & 11Buttonsレイアウトの追加
- 目的: スティック＋11ボタン構成の表示/マッピングをサポートするため。
- 変更点: 左右レイアウト列挙にBUTTON_LAYOUT_STICK_11BUTTONS/Bを追加し、WebConfigのレイアウト選択肢に表示名を追加。【F:docs/_customization_evidence/full.patch†L2180-L2193】【F:docs/_customization_evidence/full.patch†L48-L52】
- 影響範囲: レイアウト選択UIと対応するレイアウト解釈。
- 関連ファイル: proto/enums.proto, www/src/Locales/en/LayoutConfig.jsxなどロケール群。【F:docs/_customization_evidence/changed_files_name_status.txt†L15-L48】
- 設定項目: buttonLayout/buttonLayoutRightで新レイアウトを選択可能。
- 注意点: 実機配線との対応は要確認。関連コミット群でレイアウト微調整あり（de90e1fd, 1c602f7b, 96052698, 65d3d196）。【F:docs/_customization_evidence/commits.txt†L19-L22】

## ビルド
### GridGradientソースのビルド登録
- 目的: 新規アニメーション効果のソースをビルド対象に含めるため。
- 変更点: CMakeListsにsrc/animationstation/effects/gridgradient.cppを追加。【F:docs/_customization_evidence/full.patch†L1-L10】
- 影響範囲: ファームウェアビルド構成。
- 関連ファイル: CMakeLists.txt。【F:docs/_customization_evidence/changed_files_name_status.txt†L1-L1】
- 設定項目: なし。
- 注意点: 追加ファイル未反映でビルド失敗する可能性があったための登録。関連コミットはGrid Gradient追加系列を参照。【F:docs/_customization_evidence/commits.txt†L8-L29】

## その他
### P5General OLEDテクニカルノートの追加
- 目的: P5GeneralモードにおけるOLED間引きの設計・運用上の注意を文書化するため。
- 変更点: docs/P5General_OLED_notes.mdを追加し、概要、現在のデフォルト確認、スケジューリング方針、TinyUSBとの干渉回避について記載。【F:docs/_customization_evidence/full.patch†L13-L40】
- 影響範囲: ドキュメントのみ。
- 関連ファイル: docs/P5General_OLED_notes.md。【F:docs/_customization_evidence/changed_files_name_status.txt†L2-L2】
- 設定項目: なし。
- 注意点: WebConfig文言と整合させる必要あり（要確認）。
- 関連コミット: ad98695b（ガイダンス整理）。【F:docs/_customization_evidence/commits.txt†L5-L5】

# 付録: 変更ファイル一覧
```
A       docs/P5General_OLED_notes.md
M       CMakeLists.txt
M       headers/addons/display.h
M       headers/addons/neopicoleds.h
M       headers/animationstation/animationstation.h
A       headers/animationstation/effects/gridgradient.h
A       headers/animationstation/effects/noanimation.h
M       headers/buttonlayouts.h
M       headers/display/GPGFX.h
M       headers/display/ui/elements/GPScreen.h
M       headers/drivers/p5general/P5GeneralDriver.h
M       headers/interfaces/i2c/displaybase.h
M       headers/interfaces/i2c/ssd1306/tiny_ssd1306.h
M       headers/layoutmanager.h
M       proto/config.proto
M       proto/enums.proto
M       src/addons/display.cpp
M       src/addons/neopicoleds.cpp
M       src/animationstation/animationstation.cpp
A       src/animationstation/effects/gridgradient.cpp
M       src/config_utils.cpp
M       src/display/GPGFX.cpp
M       src/display/ui/elements/GPScreen.cpp
M       src/drivers/p5general/P5GeneralDriver.cpp
M       src/gp2040aux.cpp
M       src/interfaces/i2c/ssd1306/tiny_ssd1306.cpp
M       src/layoutmanager.cpp
M       src/webconfig.cpp
M       www/src/Icons/Flags/koKR.tsx
M       www/src/Icons/Flags/trTR.tsx
M       www/src/Locales/de-DE/DisplayConfig.jsx
M       www/src/Locales/en/DisplayConfig.jsx
M       www/src/Locales/en/LayoutConfig.jsx
M       www/src/Locales/en/LedConfig.jsx
M       www/src/Locales/es-MX/DisplayConfig.jsx
M       www/src/Locales/es-MX/LayoutConfig.jsx
M       www/src/Locales/fr-FR/DisplayConfig.jsx
M       www/src/Locales/fr-FR/LayoutConfig.jsx
M       www/src/Locales/ja-JP/DisplayConfig.jsx
M       www/src/Locales/ja-JP/LayoutConfig.jsx
M       www/src/Locales/ja-JP/LedConfig.jsx
M       www/src/Locales/ko-KR/DisplayConfig.jsx
M       www/src/Locales/ko-KR/LayoutConfig.jsx
M       www/src/Locales/pt-BR/DisplayConfig.jsx
M       www/src/Locales/tr-TR/DisplayConfig.jsx
M       www/src/Locales/tr-TR/LayoutConfig.jsx
M       www/src/Locales/zh-CN/DisplayConfig.jsx
M       www/src/Locales/zh-CN/LayoutConfig.jsx
M       www/src/Pages/DisplayConfig.jsx
M       www/src/Pages/LEDConfigPage.jsx
M       www/src/Pages/PinMapping.tsx
M       www/src/Services/Utilities.ts
M       www/src/Services/WebApi.js
```

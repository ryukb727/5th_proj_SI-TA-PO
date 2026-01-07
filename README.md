| [Korean 🇰🇷](#korean) | [Japanese 🇯🇵](#japanese) | [Team](#Team) |
| :---: | :---: | :---: |

</div>

---

<div id="korean">

### 🇰🇷 Korean Version

# ⏱️ SI-TA-PO  
## Linux Kernel Device Driver 기반 타이머 · 시계 · 뽀모도로 임베디드 시스템

<img width="2050" height="974" alt="Image" src="https://github.com/user-attachments/assets/745f11eb-b329-4c47-9dd5-5b34204aff43" />

### MODE 1 시계
<p align="left">
  <img src="docs/gif/input3large.gif" width="700">
</p>

### MODE 2 타이머
<p align="left">
  <img src="docs/gif/input2large.gif" width="700">
</p>

### MODE 3 뽀모도로
<p align="left">
  <img src="docs/gif/input1large.gif" width="700">
</p>

---

## 💡 1. 프로젝트 개요

본 프로젝트는 <strong>리눅스 커널 드라이버(v6.1)</strong>를 직접 설계·구현하여,
하드웨어 자원을 커널 레벨에서 정밀하게 제어하고,
보드 레벨의 다양한 입·출력 디바이스를 커널 레벨로 통합 관리하는 <strong>시간 관리 시스템(SI-TA-PO)</strong>입니다.

**SSD1306 OLED**와 **DS1302 RTC**를 데이터시트 기반으로 직접 제어하고,
<strong>FSM(Finite State Machine)</strong> 아키텍처를 도입하여
시계·타이머·뽀모도로 모드 간의 안정적인 상태 전환을 구현했습니다.

특히 <strong>인터럽트 핸들러와 전용 워크큐를 분리 설계(Top/Bottom Half)</strong>하여
저사양 임베디드 환경에서도 **지연 없는 사용자 입력 처리와 안정적인 화면 갱신**을 달성했습니다.

<br>

### 🧩 시스템 블록 다이어그램 (System Architecture)
<img width="2509" height="549" alt="Image" src="https://github.com/user-attachments/assets/142183e5-e9a6-4dac-930f-45f40e4922bb" />

### 🔌 회로도
<img width="1145" height="784" alt="Image" src="https://github.com/user-attachments/assets/15773fd2-9916-49aa-8377-cc9fbe68424c" />

---

## 🛠️ 2. 기술 스택

![C](https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=black)
![Linux](https://img.shields.io/badge/OS-Linux%20Kernel%206.1-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![RaspberryPi](https://img.shields.io/badge/Platform-Raspberry%20Pi%204-A22846?style=for-the-badge&logo=raspberrypi&logoColor=white)
<br>
![I2C](https://img.shields.io/badge/Interface-I2C-555555?style=for-the-badge)
![GPIO](https://img.shields.io/badge/Interface-GPIO%20Interrupt-6DB33F?style=for-the-badge)
<br>
![Workqueue](https://img.shields.io/badge/Kernel%20Mechanism-Workqueue-00599C?style=for-the-badge&logo=linux&logoColor=white)
![FSM](https://img.shields.io/badge/Design-FSM-FF9800?style=for-the-badge)
<br>
![SSD1306](https://img.shields.io/badge/Display-SSD1306%20OLED-000000?style=for-the-badge)
![DS1302](https://img.shields.io/badge/RTC-DS1302-3F51B5?style=for-the-badge)
![RotaryEncoder](https://img.shields.io/badge/Input-Rotary%20Encoder-795548?style=for-the-badge)
![TactSwitch](https://img.shields.io/badge/Input-Tact%20Switch-607D8B?style=for-the-badge)

---

## 🎯 3. 핵심 기능

- **FSM 기반 3 Mode 제어**
  - Clock / Timer / Pomodoro 모드 간 상위 상태 전환
  - 각 모드 내부에서 설정·실행 상태를 FSM으로 분리 관리
  - 상태 계층화를 통해 논리 충돌 및 예외 상황 방지
<img width="4528" height="3466" alt="Image" src="https://github.com/user-attachments/assets/ab2cc4bf-50c0-461c-bd4f-a1bf9bbb443d" />

- **정밀 입력 시스템**
  - 로터리 엔코더 회전 입력을 통한 값(시/분/초/반복 횟수 등) 설정
  - 로터리 엔코더의 키 스위치를 활용한 모드 내부 상태 전환
    - 예: 분 설정 ↔ 시 설정 ↔ 실행 상태 전환
  - 택트 스위치를 이용한 상위 모드(Clock / Timer / Pomodoro) 전환
  - 인터럽트 기반 입력 처리로 즉각적인 사용자 반응성 확보

- **실시간 시각화**
  - 현재 모드, 설정값, 진행 상태를 OLED에 실시간 출력
  - 타이머 종료 시 LED 점멸을 통한 사용자 알림 제공

---

## 📘 4. 기술 구현

### 1) SSD1306 OLED 커널 드라이버
- 커널 I2C 프레임워크 기반 드라이버 구현
- 전용 폰트 데이터(16x10, 5x7) 설계 및 전송
- OLED GDDRAM 구조를 고려한 화면 갱신 로직 설계

### 2) FSM(Finite State Machine) 설계
- Clock / Timer / Pomodoro 상위 모드와 모드별 내부 상태를 분리한 계층적 FSM 구조 설계
- 각 모드는 서로 다른 동작 단계(State Flow)를 가지며, 로터리 엔코더 키 입력에 따라 상태 전이 수행
- 동작 중에도 언제든 초기 설정 상태로 복귀할 수 있도록 설계하여 예외 상황 및 사용자 오동작에 안전한 구조 구현

### 3) Workqueue 기반 Bottom-half 처리
- 인터럽트 핸들러에서 OLED 업데이트 등 무거운 작업을 분리하여 Bottom-half에서 처리
- 전용 워크큐를 구성해 커널 기본 워크큐와의 리소스 경합을 방지
- 커널 인터럽트 부하 감소 및 시스템 응답성 향상

### 4) I2C 통신 최적화
- 1바이트 단위 전송의 비효율성 및 대용량 일괄 전송 시 데이터 손실의 한계를 분석
- SSD1306 GDDRAM 구조를 고려한 **Page 단위(128 Byte) 분할 전송**으로 전환
- 데이터 유실 없이 화면 갱신 정확도와 전송 효율을 동시에 확보

### 5) 입력 디바이스 드라이버
- GPIO 인터럽트를 활용한 로터리 엔코더·스위치 입력 감지
- jiffies 기반 시간 디바운싱과 전기적 안정 대기, GPIO 상태 검증을 결합한 디바운싱 로직으로 저속 회전 시 발생하는 하드웨어 채터링 문제 해결

---

## 👨‍💻 5. 역할 및 기여

- **FSM 아키텍처 설계**
  - 모드(Clock / Timer / Pomodoro)와 동작 단계를 분리한 계층적 FSM 설계
  - 내부 상태를 IDLE / SETTING / RUNNING 개념으로 구조화하여 예외 상황에서도 중단 없는 제어 흐름 확립

- **OLED 전송 알고리즘 최적화**
  - 하드웨어 수신 버퍼 한계를 고려한 페이지 단위 분할 전송
  - 데이터 유실 없는 안정적인 화면 초기화 구현

- **복합 디바운싱 알고리즘 적용**
  - jiffies 기반 시간 디바운싱과 udelay를 이용한 전기적 안정 대기 로직 결합
  - 채터링 구간을 무시하고 Falling Edge가 확정된 경우에만 S2 값을 판독하여 로터리 엔코더의 하드웨어적 채터링을 소프트웨어 레벨에서 안정적으로 해결

- **전용 워크큐(my_workqueue) 설계**
  - system_wq 의존성 제거
  - 독립적인 스케줄링 구조로 리소스 병목 및 지연 문제 해결

---

## 🐞 6. 트러블슈팅

### 1) I2C 대용량 전송 시 OLED 화면 잔상 문제
- **현상**: 화면 클리어 시 1025Byte 일괄 전송 → 하단부 클리어 실패
- **분석**: ftrace 추적 결과, 커널 송신은 정상이나 OLED 수신 버퍼 한계로 데이터 유실 발생
- **해결**: 전송 단위를 페이지(128 Byte) 단위로 분할하여 전송
- **결과**: 전송 안정성과 화면 출력 정확도 동시 확보
<img width="1441" height="1790" alt="Image" src="https://github.com/user-attachments/assets/a9d6618b-b85d-4ef5-b205-5509fe3580ca" />
<br>
### 2) 로터리 엔코더 경계면 채터링 현상
- **현상**: 저속 회전 시 전압 경계면에서 신호 떨림 발생
- **분석**: Falling Edge 진입 시 미세 신호 떨림으로 오작동 확인
- **해결**:  
  - jiffies 기반 5ms 시간 디바운싱 적용
  - 전기적 안정 확보를 위한 짧은 대기 후(GPIO 신호 안정화)
  - S1 Falling Edge 확인 후 S2 값을 읽는 조건부 방향 판별 로직 적용
- **결과**: 저속·고속 회전 환경 모두에서 오작동 없는 안정적인 입력 인식 확보

### 3) 커널 기본 워크큐 병목 현상
- **현상**: LED 점멸 기능 추가 후 OLED 출력 중단
- **분석**: system_wq 공유로 인한 리소스 경합 발생
- **해결**: 전용 워크큐(my_workqueue) 생성으로 독립적인 실행 환경 구성
- **결과**: 실시간성 확보 및 제어 안정성 향상

---

## 📚 7. 배운 점

- **커널 리소스 관리 및 인터럽트 최적화**:
  인터럽트 핸들러와 워크큐를 분리 설계하고 전용 워크큐를 구성하여, 리소스 경합이 시스템 안정성에 미치는 영향을 실제 문제 해결 과정을 통해 체득함
- **하드웨어 데이터시트 분석 및 드라이버 최적화**:
  I2C 통신 흐름을 분석하고 수신 버퍼 한계를 고려한 전송 단위 설계를 통해, 하드웨어 제약 조건 속에서 성능과 데이터 신뢰성을 동시에 확보하는 법을 습득함
- **FSM 기반의 구조적 설계**:
  다중 모드 시스템에 FSM 구조를 도입하여 복잡한 상태 전환을 체계화하고, 예외 상황에서도 예측 가능하고 확장 가능한 제어 구조를 설계하는 경험을 축적함
- **임베디드 소프트웨어 엔지니어의 관점**:
  단순한 기능 구현을 넘어 시스템이 안정적으로 동작하게 만드는 구조적 설계와 자원 관리의 중요성을 실제 개발 경험을 통해 확립함

---

<div align="center">
<a href="#japanese">⬇️ 日本語バージョンへ移動 (Go to Japanese Version) ⬇️</a>
</div>

</div>

---

<div id="japanese">

### 🇯🇵 Japanese Version

# ⏱️ SI-TA-PO  
## Linuxカーネルデバイスドライバベース：タイマー・時計・ポモドーロ組込みシステム

<img width="2050" height="974" alt="Image" src="https://github.com/user-attachments/assets/745f11eb-b329-4c47-9dd5-5b34204aff43" />

### MODE 1 時計 (Clock)
<p align="left">
  <img src="docs/gif/input3large.gif" width="700">
</p>

### MODE 2 タイマー (Timer)
<p align="left">
  <img src="docs/gif/input2large.gif" width="700">
</p>

### MODE 3 ポモドーロ (Pomodoro)
<p align="left">
  <img src="docs/gif/input1large.gif" width="700">
</p>

---

## 💡 1. プロジェクト概要

本プロジェクトは、**Linuxカーネルドライバ (v6.1)** を独自に設計・実装し、ハードウェアリソースをカーネルレベルで精密に制御、ボードレベルの多様な入出力デバイスを統合管理する**時間管理システム (SI-TA-PO)** です。

**SSD1306 OLED** および **DS1302 RTC** をデータシートに基づいて直接制御し、**FSM (Finite State Machine)** アーキテクチャを導入することで、時計・タイマー・ポモドーロの各モード間における安定した状態遷移を実現しました。

特に、**割り込みハンドラと専用ワークキューを分離設計 (Top/Bottom Half)** することにより、低スペックな組み込み環境においても、**遅延のないユーザー入力処理と安定した画面更新**を達成しています。


### 🧩 システムブロック図 (System Architecture)
<img width="2509" height="549" alt="Image" src="https://github.com/user-attachments/assets/142183e5-e9a6-4dac-930f-45f40e4922bb" />

### 🔌 回路図 (Schematic)
<img width="1145" height="784" alt="Image" src="https://github.com/user-attachments/assets/15773fd2-9916-49aa-8377-cc9fbe68424c" />

---

## 🛠️ 2. 技術スタック

![C](https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=black)
![Linux](https://img.shields.io/badge/OS-Linux%20Kernel%206.1-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![RaspberryPi](https://img.shields.io/badge/Platform-Raspberry%20Pi%204-A22846?style=for-the-badge&logo=raspberrypi&logoColor=white)
<br>
![I2C](https://img.shields.io/badge/Interface-I2C-555555?style=for-the-badge)
![GPIO](https://img.shields.io/badge/Interface-GPIO%20Interrupt-6DB33F?style=for-the-badge)
<br>
![Workqueue](https://img.shields.io/badge/Kernel%20Mechanism-Workqueue-00599C?style=for-the-badge&logo=linux&logoColor=white)
![FSM](https://img.shields.io/badge/Design-FSM-FF9800?style=for-the-badge)
<br>
![SSD1306](https://img.shields.io/badge/Display-SSD1306%20OLED-000000?style=for-the-badge)
![DS1302](https://img.shields.io/badge/RTC-DS1302-3F51B5?style=for-the-badge)
![RotaryEncoder](https://img.shields.io/badge/Input-Rotary%20Encoder-795548?style=for-the-badge)
![TactSwitch](https://img.shields.io/badge/Input-Tact%20Switch-607D8B?style=for-the-badge)

---

## 🎯 3. 主要機能

- **FSMベースの3モード制御**
  - Clock / Timer / Pomodoro モード間の上位状態遷移
  - 各モード内部での設定・実行状態をFSMで分離管理
  - 状態の階層化により、ロジックの衝突や例外動作を防止
<img width="4528" height="3466" alt="Image" src="https://github.com/user-attachments/assets/ab2cc4bf-50c0-461c-bd4f-a1bf9bbb443d" />

- **精密な入力システム**
  - ロータリーエンコーダによる値（時/分/秒/反復回数など）の設定
  - ロータリーエンコーダのプッシュスイッチを活用した内部状態遷移
    - 例：分設定 ↔ 時設定 ↔ 実行状態の切り替え
  - タクトスイッチによる上位モードの切り替え
  - 割り込みベースの入力処理による即応性の確保

- **リアルタイム・ビジュアライゼーション**
  - 現在のモード、設定値、進行状況をOLEDへリアルタイム出力
  - タイマー終了時、LED点滅によるユーザー通知機能

---

## 📘 4. 技術実装

### 1) SSD1306 OLED カーネルドライバ
- カーネルI2Cフレームワークに基づくドライバ実装
- 専用フォントデータ（16x10, 5x7）の設計および転送
- OLEDのGDDRAM構造を考慮した画面更新ロジックの設計

### 2) FSM (有限状態機械) 設計
- 上位モードと内部状態を分離した階層型FSM構造の設計
- 各モードで異なる状態遷移（State Flow）を持ち、ロータリーエンコーダの入力に応じた遷移を実行
- 動作中も初期設定状態へ復帰可能に設計し、例外状況や誤操作に強い構造を実現

### 3) WorkqueueによるBottom-half処理
- 割り込みハンドラからOLED更新等の重い処理を分離し、Bottom-halfで実行
- 専用ワークキューを構成し、カーネルのデフォルトワークキューとのリソース競合を防止
- 割り込み負荷の軽減およびレスポンスの向上

### 4) I2C通信の最適化
- 1バイト単位の転送による非効率性と、大容量の一括転送時におけるデータ損失の限界を分析
- SSD1306のGDDRAM構造に合わせ、**ページ単位（128バイト）の分割転送**へ変更
- データ損失を防ぎ、画面更新の正確性と転送効率を両立を実現

### 5) 入力デバイスドライバ
- GPIO割り込みを利用したロータリーエンコーダ・スイッチの入力検出
- jiffiesベースの時間デバウンス、電気的安定待ち、GPIO状態検証を組み合わせたロジックにより、低速回転時に発生するハードウェアチャタリング問題を解決

---

## 👨‍💻 5. 担当役割と貢献

- **FSMアーキテクチャ設計**
  - モードと動作フェーズを分離した階層型FSMの設計
  - 内部状態を IDLE / SETTING / RUNNING として構造化し、安定した制御フローを確立

- **OLED転送アルゴリズムの最適化**
  - ハードウェア受信バッファの限界を考慮したページ単位の分割転送
  - データ欠落のない安定した画面クリアを実現

- **複合デバウンスアルゴリズムの適用**
  - jiffiesによる時間的制約と、udelayによる電気的安定待ちロジックを組み合わせて実装
  - チャタリング区間を無視し、Falling Edgeが確定した条件下でのみS2値を読み取ることで、ハードウェア由来のノイズをソフトウェアレベルで安定的に解消

- **専用ワークキュー (my_workqueue) の設計**
  - `system_wq` への依存を排除
  - 独立したスケジューリング構造により、リソースのボトルネックと遅延問題を解決

---

## 🐞 6. トラブルシューティング

### 1) I2C大量転送時のOLED画面残像問題
- **現象**: 画面クリア時に1024バイト超を一括送信すると、下部領域が正しくクリアされない
- **分析**: `ftrace` の結果、送信自体は正常だが、OLED側の受信バッファ溢れによりデータ消失が発生
- **解決**: 送信単位をページ（128バイト）単位に分割して転送
- **結果**: 送信の安定性と描画の正確性を同時に確保

### 2) ロータリーエンコーダの境界値におけるチャタリング
- **現象**: 低速回転時、電圧の境界付近で信号のバタつきが発生
- **分析**: Falling Edge検出時の微細な信号揺れにより誤作動することを確認
- **解決**:
  - jiffiesベースの5ms時間デバウンスを適用
  - 電気的安定を確保するための短時間待機（GPIO信号の安定化）を挿入
  - S1のFalling Edge確定後にのみS2値を読み取る条件付き方向判別ロジックを実装
- **結果**: 低速・高速回転のいずれの環境においても、誤動作のない安定した入力認識を実現

### 3) カーネル共有ワークキューのボトルネック
- **現象**: LED点滅機能を追加した後、OLEDの出力が停止
- **分析**: `system_wq` 共有によるリソース競合（Resource Contention）の発生
- **解決**: 専用ワークキュー (`my_workqueue`) 生成による実行環境の分離
- **結果**: リアルタイム性の確保および制御の安定性が向上
---

## 📚 7. 学んだこと

- **カーネルリソース管理と割り込みの最適化**:
  割り込みハンドラとワークキューを分離し、専用ワークキューを運用することで、リソース競合が安定性に与える影響を実戦的な問題解決を通じて理解した。
- **データシート分析とドライバ最適化**:
  I2C通信フローの分析に基づき、ハードウェア制約下で性能と信頼性を両立させる設計手法を習得した。
- **FSMによる構造的設計**:
  マルチモードシステムにFSMを導入して複雑な状態遷移を体系化し、例外状況でも予測可能かつ拡張性の高い制御構造を設計する経験を積んだ。
- **組込みソフトウェアエンジニアの視点**:
  単なる機能実装を超え、システムが安定して動作するための「構造的設計」と「リソース管理」の重要性を実際の開発経験を通じて確立した。

---

<div align="center">
<a href="#korean">⬆️ 한국어 버전으로 돌아가기 (Go back to Korean Version) ⬆️</a>
</div>

</div>

---
---

<div align="center">
<a href="#Team">⬇️ Go to Team Version ⬇️</a>
</div>

</div>

---

<div id="Team">

### ⏰ Team

라즈베리파이(또는 리눅스)에서 동작하는 mini device driver 프로젝트입니다.  
아래 디바이스들을 커널 모듈/드라이버 형태로 제어합니다.

## 구성 디바이스
- **Rotary Encoder**: 회전 입력 + 버튼 인터럽트 처리
- **SSD1306 OLED**: 상태/시간/메뉴 출력
- **DS1302 RTC**: 시간 읽기/설정 
- **LED Bar (8ch)**: 상태 표시 (LED00 ~ LED02 모드 1,2,3 표시, LED03 ~ LED07은 모드 2,3 타이머 완료시 3회 점멸)
- **Tact Switch**: 모드 전환/확인 입력

## 핀맵
| 디바이스               |      신호 | GPIO(BCM) | 비고          |
| ------------------ | ------: | --------: | ----------- |
| **DS1302 RTC**     | SDA(IO) |        17 | 3-wire      |
|                    |     SCL |        27 | 3-wire      |
|                    | RST(CE) |        22 | 3-wire      |
| **SSD1306 OLED**   |     SDA |         0 | I2C0 SDA    |
|                    |     SCL |         1 | I2C0 SCL    |
|                    |     RES |         5 | Reset GPIO  |
| **LED Bar (8ch)**  |    LED0 |         7 |             |
|                    |    LED1 |         8 |             |
|                    |    LED2 |         9 |             |
|                    |    LED3 |        10 |             |
|                    |    LED4 |        11 |             |
|                    |    LED5 |        12 |             |
|                    |    LED6 |        13 |             |
|                    |    LED7 |        14 |             |
| **Rotary Encoder** |      S1 |        23 | A           |
|                    |      S2 |        24 | B           |
|                    |     KEY |        25 | 버튼(인터럽트)    |
| **TACT Switch**    |      SW |        20 | 입력(인터럽트/폴링) |


## BOM

| No | Item | Spec / Model | Qty | Purpose | Note |
|---:|------|--------------|---:|---------|------|
| 1 | Main Board | Raspberry Pi 4B | 1 | Kernel driver execution | OS: Debian 13.2 (trixie) / Kernel: 6.1.93-v8+ (aarch64) |
| 2 | Rotary Encoder | EC11 rotary encoder | 1 | Input / value adjustment | Uses S1 / S2 / KEY |
| 3 | Tact Switch | Tact push button 12×12 | 1 | Mode change | Debounce applied |
| 4 | OLED Module | SSD1306 | 1 | Display output | SDA / SCL |
| 5 | RTC Module | DS1302 | 1 | Time keeping | Includes CR2032 |
| 6 | Coin Battery | CR2032 | 1 | RTC backup power | 3V |
| 7 | LED (or LED Bar) | 8 LEDs (or module) | 1 | Status / alert indication | GPIO7 ~ GPIO14 |
| 8 | Resistor | 1 kΩ | 2 | Pull-up resistors | Rotary KEY, Tact SW |
| 9 | Jumper Wires | M-F / M-M | Many | Wiring |  |
| 10 | Breadboard | 165 × 55 | 1 | Circuit assembly |  |



## 하드웨어 이미지
<p align="center">
  <img src="docs/photos/hardware_overview.png" width="800">
</p>

## 회로도
<p align="center">
  <img src="docs/schematic/schematic.png" width="900">
</p>

## 소프트웨어 상태도 (FSM)
프로젝트의 모드 전환 및 동작 로직을 나타내는 상태 다이어그램입니다.
<p align="center">
  <img width="4528" height="3466" alt="Image" src="https://github.com/user-attachments/assets/ab2cc4bf-50c0-461c-bd4f-a1bf9bbb443d" />
</p>

## 주요 기능
- Rotary 입력으로 메뉴 이동 / 값 조절
- Tact switch로 선택/뒤로가기 등 입력 처리
- DS1302에서 읽은 시간 정보를 SSD1306에 표시
- 이벤트(모드/상태)에 따라 8-LED bar 패턴 출력


## Demo (GIF)

### MODE 1 시계
<p align="left">
  <img src="docs/gif/input3large.gif" width="700">
</p>

### MODE 2 타이머
<p align="left">
  <img src="docs/gif/input2large.gif" width="700">
</p>

### MODE 3 뽀모도로
<p align="left">
  <img src="docs/gif/input1large.gif" width="700">
</p>


## 빌드/설치

### Ubuntu (Build Host)
- **OS**: Ubuntu 22.04.5 LTS (Jammy Jellyfish)
- **Kernel**: 6.12.24
- **Arch**: x86_64

### Raspberry Pi (Target)
- **Model**: Raspberry Pi 4 Model B Rev 1.5
- **OS**: Debian GNU/Linux 13.2 (trixie)
- **Kernel**: 6.1.93-v8+

## 빌드/설치

1) Build (Ubuntu)
```bash
make
```
2) Install / Load (Raspberry Pi)
```bash
sudo insmod device_driver_mod.ko
dmesg -w
```
3) Unload (Raspberry Pi)
```bash
sudo rmmod device_driver_mod
```
---

<div align="center">
<a href="#korean">⬆️ 한국어 버전으로 돌아가기 (Go back to Korean Version) ⬆️</a>
</div>

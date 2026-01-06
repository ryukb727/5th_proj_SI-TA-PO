| [Korean 🇰🇷](#korean) | [Japanese 🇯🇵](#japanese) | [Team](#Team) |
| :---: | :---: | :---: |

</div>

---

<div id="korean">

### 🇰🇷 Korean Version

# ⏱️ Linux 커널 디바이스 드라이버 기반 시계-타이머-뽀모도로 3 Mode 시간 관리 시스템

<div align="center">
<a href="#japanese">⬇️ 日本語バージョンへ移動 (Go to Japanese Version) ⬇️</a>
</div>

</div>

---

<div id="japanese">

### 🇯🇵 Japanese Version

# ⏱️ Linuxカーネルデバイスドライバベース時計・タイマー・ポモドーロ3モード時間管理システム


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

### MODE 2 알람
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

# 👻 C3-Phantom

Thiết bị bảo mật RF đa năng dựa trên **ESP32-C3**, phát triển từ dự án Flipper C3 DIY.  
Tích hợp toàn bộ module tấn công không dây: WiFi · Bluetooth · Infrared · Sub-GHz RF · NRF24 2.4GHz.

---

## ✨ Tính năng

### 📡 WiFi (Bruce Deauth Engine)
- **Deauth from Scan** — Quét AP → chọn AP → deauth toàn bộ client
- **Target Station** — Quét AP → sniffer phát hiện client → deauth từng client riêng
- **Deauth Flood** — Tấn công xoay vòng toàn bộ AP trong vùng phủ sóng
- **Deauth by Channel** — Chọn kênh 1–13 → broadcast deauth trên kênh đó
- **Storm Mode** — Tự động bùng phát khi phát hiện hiệu quả
- **Multi-AP Mesh** — Hỗ trợ AP có nhiều BSSID cùng SSID
- **Multi-Band** — 2.4 GHz / 5 GHz / 6 GHz
- **Beacon Spam** — Tạo hàng loạt AP giả mạo
- **WiFi Scan** — Quét mạng lân cận

### 📶 Bluetooth
- **BLE Scan** — Quét thiết bị Bluetooth lân cận
- **Pairing Spam** — Gửi hàng loạt thông báo ghép nối giả (Fastpair, Sour Apple)

### 📺 Infrared
- **TV-B-Gone** — Tắt TV trong phạm vi hồng ngoại (EU / NA)
- **Universal Remote** — Điều khiển TV, máy chiếu đa năng
- **IR Spammer** — Phát liên tục tín hiệu hồng ngoại

### 📻 Sub-GHz RF (CC1101)
- **RF Scan** — Quét và bắt tín hiệu RF
- **RF Send** — Phát lại tín hiệu đã lưu
- **RF Jammer** — Gây nhiễu tần số sub-GHz
- **RF Identifier** — Nhận dạng giao thức RF
- **Tesla Charge Port** — Mở cổng sạc Tesla

### 📡 NRF24 2.4GHz *(MỚI)*
- **Spectrum Analyzer** — Phân tích phổ 2.4GHz theo thời gian thực (80 kênh, hiển thị bar chart trên OLED)
- **Jammer** — Gây nhiễu 2.4GHz với 10 chế độ:
  - Test · WiFi · BLE · BLE Adv Priority
  - Bluetooth · USB · Video Stream · RC
  - Zigbee · Full 2.4GHz
  - Hỗ trợ Sequential và FHSS (tần số nhảy ngẫu nhiên)
- **MouseJack** — Tấn công chuột/bàn phím không dây:
  - Quét thiết bị Microsoft & Logitech dễ bị tấn công
  - Inject keystroke vào thiết bị mục tiêu
  - Payload có sẵn: Calculator, Notepad, CMD, PowerShell, v.v.

---

## 🧩 Yêu cầu phần cứng

| Linh kiện | Model |
|---|---|
| Vi điều khiển | ESP32-C3 (Super Mini hoặc DevKitM-1) |
| Màn hình | SSD1306 OLED 128×64 (I2C) |
| RF Sub-GHz | CC1101 module |
| RF 2.4GHz | NRF24L01+ module |
| Hồng ngoại | IR LED 940nm |
| Nút bấm | 3 nút (LEFT / CENTER / RIGHT) |
| Nguồn NRF24 | Nguồn 3.3V ngoài 500mA (khuyến nghị) |

---

## 🔌 Sơ đồ nối dây

### NRF24L01+ *(MỚI)*
> ⚠️ **Quan trọng:** NRF24L01+ cần nguồn 3.3V riêng, **không** lấy trực tiếp từ GPIO.  
> Thêm tụ 10–22µF giữa VCC và GND của NRF24 để ổn định điện áp.

| NRF24L01+ | ESP32-C3 | Ghi chú |
|---|---|---|
| VCC | 3.3V *(nguồn ngoài)* | Dùng nguồn riêng |
| GND | GND | |
| CE | **GPIO 20** | Chip Enable |
| CSN | **GPIO 0** | SPI Chip Select |
| SCK | **GPIO 4** | SPI Clock |
| MOSI | **GPIO 6** | SPI MOSI |
| MISO | **GPIO 5** | SPI MISO |

```
NRF24L01+          ESP32-C3
┌─────────┐        ┌──────────┐
│ VCC ────┼────────┼ 3.3V ext │
│ GND ────┼────────┼ GND      │
│ CE  ────┼────────┼ GPIO 20  │
│ CSN ────┼────────┼ GPIO 0   │
│ SCK ────┼────────┼ GPIO 4   │
│ MOSI────┼────────┼ GPIO 6   │
│ MISO────┼────────┼ GPIO 5   │
└─────────┘        └──────────┘
         [10-22µF giữa VCC-GND]
```

---

### CC1101
| CC1101 | ESP32-C3 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO 4 |
| MISO | GPIO 5 |
| MOSI | GPIO 6 |
| CSN | GPIO 10 |
| GDO0 | GPIO 7 |

> CC1101 và NRF24L01+ **chia sẻ SPI bus** (SCK/MOSI/MISO), khác nhau ở chân CS.

---

### Màn hình SSD1306 (I2C)
| SSD1306 | ESP32-C3 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO 9 |
| SDA | GPIO 8 |

---

### 3 Nút bấm
| Nút | ESP32-C3 | Kết nối |
|---|---|---|
| BUTTON_LEFT | GPIO 1 | GPIO → GND |
| BUTTON_CENTER | GPIO 2 | GPIO → GND |
| BUTTON_RIGHT | GPIO 3 | GPIO → GND |

---

### IR LED
| IR LED | ESP32-C3 |
|---|---|
| Anode (+) | GPIO 21 (qua điện trở 33Ω) |
| Cathode (−) | GND |

---

## 🗺️ Sơ đồ toàn bộ kết nối

```
                    ┌─────────────────────────────┐
                    │         ESP32-C3            │
                    │                             │
NRF24 CE   ─────────┤ GPIO 0   GPIO 20 ├───── NRF24 CSN (*)
BTN LEFT   ─────────┤ GPIO 1   GPIO 21 ├───── IR LED
BTN CENTER ─────────┤ GPIO 2            │
BTN RIGHT  ─────────┤ GPIO 3            │
NRF24/CC SCK ───────┤ GPIO 4            │
NRF24/CC MISO ──────┤ GPIO 5            │
NRF24/CC MOSI ──────┤ GPIO 6            │
CC1101 GDO0 ────────┤ GPIO 7            │
SSD1306 SDA ────────┤ GPIO 8            │
SSD1306 SCL ────────┤ GPIO 9            │
CC1101 CSN  ────────┤ GPIO 10           │
                    └─────────────────────────────┘
(*) CE và CSN đã hoán đổi so với thứ tự GPIO
```

---

## 📋 Cấu trúc Menu

```
C3-Phantom
├── WiFi
│   ├── Scan
│   │   └── [Chọn AP] → Deauth All / Target Client / Deauth Flood
│   ├── Beacon Spam
│   └── Deauth
│       ├── From Scan
│       ├── Target Station
│       ├── Flood All APs
│       └── By Channel
├── Bluetooth
│   ├── Scan
│   └── Pairing Spam
├── Infrared
│   ├── TV-B-Gone (EU / NA)
│   ├── Spammer
│   └── Universal Remote (TV / Projector)
├── Radio (CC1101)
│   ├── Scan
│   ├── Send
│   ├── Jammer
│   ├── Identifier
│   └── Tesla Charge Port
├── NRF24
│   ├── Spectrum Analyzer
│   ├── Jammer (10 chế độ)
│   ├── MouseJack
│   │   ├── Scan Devices
│   │   └── Attack Target
│   └── Jammer Mode List
└── Settings
    └── Headless Mode
```

---

## 🛠️ Build

### Yêu cầu
- [PlatformIO](https://platformio.org/) (CLI hoặc VS Code extension)
- ESP32 Arduino Core

### Thư viện (tự động cài qua platformio.ini)
| Thư viện | Phiên bản | Dùng cho |
|---|---|---|
| IRremoteESP8266 | ^2.8.6 | Infrared |
| Adafruit SSD1306 | ^2.5.15 | OLED display |
| RadioLib | ^7.3.0 | CC1101 RF |
| AsyncTCP-esphome | ^2.0.0 | WiFi async |
| ESPAsyncWebServer | 3.6.0 | Web server |
| **RF24** | **^1.4.5** | **NRF24L01+** |

### Lệnh build
```bash
# Cài thư viện
platformio lib install

# Build
platformio run -e esp32-c3-devkitm-1

# Upload
platformio run -e esp32-c3-devkitm-1 -t upload

# Monitor serial
platformio device monitor -b 115200
```

---

## ⚠️ Lưu ý pháp lý

- **Deauth / Jamming** vi phạm pháp luật ở hầu hết các quốc gia nếu sử dụng trái phép
- **MouseJack** chỉ sử dụng trên thiết bị bạn sở hữu hoặc được cấp phép kiểm thử
- Chỉ sử dụng trong môi trường kiểm thử có kiểm soát
- Tác giả không chịu trách nhiệm cho việc sử dụng sai mục đích

---

## 📦 Cấu trúc thư mục

```
C3-Phantom/
├── src/
│   ├── main.cpp
│   ├── global.hpp          ← Pin defines (bao gồm NRF24)
│   ├── menu.hpp
│   ├── display_utils.h
│   ├── nrf24/              ← NRF24 module (MỚI)
│   │   ├── nrf24_common.hpp
│   │   ├── nrf24_spectrum.hpp
│   │   ├── nrf24_jammer.hpp
│   │   ├── nrf24_mousejack.hpp
│   │   └── nrf24_menu.hpp
│   ├── wifi/
│   │   ├── deauth_bruce.hpp  ← Bruce deauth engine
│   │   ├── scan.hpp
│   │   └── spam.hpp
│   ├── bluetooth/
│   ├── infrared/
│   └── rf/
└── platformio.ini
```

---

*C3-Phantom — ESP32-C3 · WiFi · BLE · IR · Sub-GHz · NRF24*

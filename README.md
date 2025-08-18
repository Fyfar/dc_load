# 🔋 DIY Electronic Load

This repository contains everything related to my **homemade electronic load** project.  
It’s a personal DIY project – not a commercial or professional design.  
I’m sharing it here mostly to keep things organized and maybe to help someone else who is experimenting with similar ideas.

---

## 📂 What's Inside

- **🖥 Firmware (ESP32)**  
  Code for controlling the load, written for **ESP32-C3 Super Mini**.  
  - No `delay()` calls (uses timers/interrupts instead)  
  - Online logging via **MQTT broker** 📡  
  - Current control via external DAC  
  - Calibration and basic measurement support  

- **📐 KiCad Files**  
  Schematics and PCB design made with KiCad.  
  Currently missing the 4-wire measurement module (will be documented later).  
  Expect changes and improvements over time.

- **📝 Notes & Experiments**  
  Test results, calibration data, and small helper scripts.

---

## 🎯 Project Goals

- Learn more about **electronics, MOSFETs, and op-amps**  
- Build a useful tool for testing power supplies and batteries  
- Experiment with **current regulation, ADC/DAC, and firmware control**  
- Try **4-wire measurements** for better accuracy  
- Keep everything DIY-friendly and fun 🚀  

---

## 🛠 Components Used

- **ADS1115** – external ADC (much more accurate than ESP32’s built-in one)  
- **MCP4725** – external DAC (better resolution and stability)  
- **4x4 Keypad** – for input  
- **PCF8575** – port expander for the keypad  
- **ESP32-C3 Super Mini** – main controller  
- **LCD 20x4 with I²C module** – display  
- **Case Z17** – enclosure  
- **80×80×15 mm 2-pin Fan** – cooling  

---

## ⚡ Features

- Online logging with MQTT  
- External ADC & DAC for better precision  
- 4-wire measurement (documentation coming soon)  
- No blocking delays in firmware  
- Simple but expandable design  

---

## ⚠️ Disclaimer

This is a hobby project.  
I can’t guarantee correctness, safety, or reliability of the design or code.  
If you decide to use any part of it, **do it at your own risk**.

---

## 🤝 Contributions

I don’t expect pull requests, but if you’re also into DIY electronics and want to share ideas or improvements – feel free.

---

## 📜 License

MIT License – do whatever you want with the code and files, but don’t blame me if something goes wrong.

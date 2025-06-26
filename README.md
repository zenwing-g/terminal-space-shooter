# 🚀 Terminal Space Shooter (C++)

A minimalist terminal-based shooter game written in **C++** — no OOP, no external libraries, just **raw I/O, plain vectors**, and good old-fashioned systems programming.

---

## 🔧 Features

- 🔫 Real-time shooter mechanics in terminal
- 🛰️ Enemies fall from the top — survive as long as you can
- 💥 Bullets collide with enemies and clear them
- 🎮 Controls:
  - `a`: Move left
  - `d`: Move right
  - `q`: Quit
  - (Arrows supported if implemented)

---

## ⚡ Highlights

> **Terminal based shooter in C++ without OOP and using plain vectors.**

- No object-oriented structure — everything is raw and procedural.
- Game state is managed with `vector<vector<char>>` and raw input.
- Implements non-blocking keypresses using `termios` and `fcntl`.
- Bullets and enemies are just `pair<int, int>` in `vector`s.

---

## 🛠️ How to Build & Run

```bash
g++ shooter.cpp -o shooter
./shooter
```

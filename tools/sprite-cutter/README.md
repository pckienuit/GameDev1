# 🎨 SpriteCutter

> **The fastest way to extract sprite coordinates from sprite sheets.**
> Built for game developers — works with any sprite sheet format.

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![Python](https://img.shields.io/badge/python-3.11%2B-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

| Feature | Web (Free) | Desktop (Pro) |
|---------|:----------:|:-------------:|
| AI click-to-segment (MobileSAM) | ✅ | ✅ |
| Manual draw regions | ✅ | ✅ |
| Uniform Grid mode | ✅ | ✅ |
| Animation grouping | ✅ | ✅ |
| JSON export/import | ✅ | ✅ |
| C++ snippet copy | ✅ | ✅ |
| OpenCV auto-detect | ❌ | ✅ |
| Full SAM (ViT-H) precision | ❌ | ✅ |
| Works offline | ❌ | ✅ |

---

## Quick Start

### Desktop App
```bash
# Install dependencies
pip install -r requirements.txt

# Run desktop app
python -m desktop.main
```

### Web App
```bash
# Start server
python web/server.py

# Open browser at http://localhost:8000
```

---

## Project Structure

```
sprite-cutter/
├── core/           # Shared: models, detectors, exporter
├── desktop/        # PyQt6 desktop application
├── web/            # FastAPI server + HTML5 frontend
├── landing/        # Marketing landing page
├── models/         # AI model weights (auto-downloaded)
└── tests/          # Unit tests
```

---

## Installation

**Requirements:** Python 3.11+

```bash
pip install -r requirements.txt
```

> **Note:** MobileSAM weights (~10MB) are downloaded automatically on first use.
> Full SAM ViT-H (~2.5GB) is optional and downloaded when selected in Desktop app.

---

## Usage Guide

_(Coming soon — see each module's README for now)_

---

## License

MIT License — free for personal and commercial use.

---

## Credits

Built with:
- [OpenCV](https://opencv.org/) — Computer Vision
- [MobileSAM](https://github.com/ChaoningZhang/MobileSAM) — Lightweight AI Segmentation
- [PyQt6](https://www.riverbankcomputing.com/software/pyqt/) — Desktop GUI
- [FastAPI](https://fastapi.tiangolo.com/) — Web Backend

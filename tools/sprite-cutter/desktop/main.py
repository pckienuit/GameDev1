"""
SpriteCutter Desktop — Entry point.

Run options (both work):
  python -m desktop.main          (from sprite-cutter/ root)
  python main.py                  (from inside desktop/ folder)
"""
import sys
from pathlib import Path

# Ensure sprite-cutter root is on sys.path regardless of CWD
_root = Path(__file__).resolve().parent.parent
if str(_root) not in sys.path:
    sys.path.insert(0, str(_root))
from PyQt6.QtWidgets import QApplication
from PyQt6.QtGui import QIcon
from desktop.main_window import MainWindow


def main() -> None:
    app = QApplication(sys.argv)
    app.setApplicationName("SpriteCutter")
    app.setApplicationVersion("0.1.0")
    app.setOrganizationName("SpriteCutter")

    # Load version
    try:
        from pathlib import Path
        version = Path(__file__).parent.parent / "VERSION"
        app.setApplicationVersion(version.read_text().strip())
    except Exception:
        pass

    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()

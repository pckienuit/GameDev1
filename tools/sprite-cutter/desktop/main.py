"""
SpriteCutter Desktop — Entry point.
Run: python -m desktop.main
"""
import sys
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

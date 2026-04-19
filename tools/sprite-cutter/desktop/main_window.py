"""
MainWindow — QMainWindow shell for SpriteCutter Desktop.

Tasks 6.1–6.5:
  6.1  QMainWindow + menu bar  (File → Open / Export / Import / Quit)
  6.2  Toolbar                 (mode buttons, wired to keyboard shortcuts)
  6.3  Layout                  (center canvas placeholder + right sidebar)
  6.4  Status bar              (coord display, zoom level, sprite count)
  6.5  File → Open dialog      (load image path, update title + status)
"""
from __future__ import annotations

from pathlib import Path
from typing import Optional

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
    QLabel, QFrame, QFileDialog, QStatusBar,
    QMenuBar, QMessageBox, QSizePolicy,
)
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QAction, QKeySequence, QFont, QColor, QPalette

from desktop.toolbar import SpriteCutterToolbar, EditorMode
from desktop.image_viewer import ImageViewer


# ── Placeholder widgets (replaced in later phases) ────────────────────────────

class _PlaceholderCanvas(QFrame):
    """Phase 6: temporary canvas placeholder."""
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setMinimumSize(600, 400)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setStyleSheet("background:#0f0f1e; border:1px solid #2a2a4a;")

        lbl = QLabel("Open an image to begin\n(File → Open Image  or  Ctrl+O)", self)
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl.setStyleSheet("color:#4040a0; font-size:16px;")
        layout = QVBoxLayout(self)
        layout.addWidget(lbl)


class _PlaceholderSidebar(QFrame):
    """Phase 6: temporary right-sidebar placeholder."""
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setFixedWidth(280)
        self.setStyleSheet("background:#12121f; border-left:1px solid #2a2a4a;")

        lbl = QLabel("Sprite list\n& properties\nwill appear here", self)
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl.setStyleSheet("color:#2a2a6a; font-size:13px;")
        layout = QVBoxLayout(self)
        layout.addWidget(lbl)


# ── MainWindow ────────────────────────────────────────────────────────────────

class MainWindow(QMainWindow):
    """
    Top-level application window.

    Phases 7-13 will replace _canvas_placeholder and _sidebar_placeholder
    with real widgets.  Everything else (menus, toolbar, status bar,
    keyboard shortcuts) is fully functional from Phase 6.
    """

    def __init__(self) -> None:
        super().__init__()
        self._image_path: Optional[Path] = None
        self._sprite_count: int = 0
        self._zoom_level: float = 1.0

        self._setup_window()
        self._setup_palette()
        self._setup_menu()       # 6.1
        self._setup_toolbar()    # 6.2
        self._setup_central()    # 6.3
        self._setup_statusbar()  # 6.4
        self._setup_shortcuts()

        self._update_title()
        self._update_statusbar()

    # ── Window basics ─────────────────────────────────────────────────────────

    def _setup_window(self) -> None:
        self.setWindowTitle("SpriteCutter")
        self.resize(1280, 800)
        self.setMinimumSize(900, 600)

    def _setup_palette(self) -> None:
        """Apply dark theme palette."""
        palette = QPalette()
        dark = QColor("#0f0f1e")
        mid  = QColor("#1a1a2e")
        text = QColor("#c8c8e8")
        hi   = QColor("#4040a0")

        palette.setColor(QPalette.ColorRole.Window,          mid)
        palette.setColor(QPalette.ColorRole.WindowText,      text)
        palette.setColor(QPalette.ColorRole.Base,            dark)
        palette.setColor(QPalette.ColorRole.AlternateBase,   mid)
        palette.setColor(QPalette.ColorRole.Text,            text)
        palette.setColor(QPalette.ColorRole.Button,          mid)
        palette.setColor(QPalette.ColorRole.ButtonText,      text)
        palette.setColor(QPalette.ColorRole.Highlight,       hi)
        palette.setColor(QPalette.ColorRole.HighlightedText, QColor("#ffffff"))
        self.setPalette(palette)
        self.setStyleSheet("""
            QMainWindow, QWidget {
                background-color: #1a1a2e;
                color: #c8c8e8;
                font-family: 'Segoe UI', 'Inter', sans-serif;
            }
            QMenuBar {
                background: #12121f;
                color: #c8c8e8;
                border-bottom: 1px solid #2a2a4a;
            }
            QMenuBar::item:selected { background: #2a2a5a; }
            QMenu {
                background: #1a1a2e;
                color: #c8c8e8;
                border: 1px solid #3a3a5c;
            }
            QMenu::item:selected { background: #4040a0; }
            QStatusBar {
                background: #12121f;
                color: #8080c0;
                border-top: 1px solid #2a2a4a;
                font-size: 12px;
            }
            QToolBar {
                background: #12121f;
                border-bottom: 1px solid #2a2a4a;
                spacing: 4px;
                padding: 4px;
            }
        """)

    # ── 6.1  Menu bar ─────────────────────────────────────────────────────────

    def _setup_menu(self) -> None:
        mb = self.menuBar()

        # ── File ──────────────────────────────────────────────────────────────
        file_menu = mb.addMenu("&File")

        self._act_open = QAction("&Open Image…", self)
        self._act_open.setShortcut(QKeySequence.StandardKey.Open)
        self._act_open.setStatusTip("Open a sprite sheet image")
        self._act_open.triggered.connect(self._on_open_image)
        file_menu.addAction(self._act_open)

        file_menu.addSeparator()

        self._act_export = QAction("&Export JSON…", self)
        self._act_export.setShortcut(QKeySequence("Ctrl+S"))
        self._act_export.setStatusTip("Export sprite data to JSON")
        self._act_export.setEnabled(False)
        file_menu.addAction(self._act_export)

        self._act_import = QAction("&Import JSON…", self)
        self._act_import.setShortcut(QKeySequence("Ctrl+Shift+O"))
        self._act_import.setStatusTip("Import sprite data from JSON")
        self._act_import.setEnabled(False)
        file_menu.addAction(self._act_import)

        file_menu.addSeparator()

        act_quit = QAction("&Quit", self)
        act_quit.setShortcut(QKeySequence.StandardKey.Quit)
        act_quit.triggered.connect(self.close)
        file_menu.addAction(act_quit)

        # ── Edit ──────────────────────────────────────────────────────────────
        edit_menu = mb.addMenu("&Edit")

        self._act_undo = QAction("&Undo", self)
        self._act_undo.setShortcut(QKeySequence.StandardKey.Undo)
        self._act_undo.setEnabled(False)
        edit_menu.addAction(self._act_undo)

        self._act_delete = QAction("&Delete Region", self)
        self._act_delete.setShortcut(QKeySequence(Qt.Key.Key_Delete))
        self._act_delete.setEnabled(False)
        edit_menu.addAction(self._act_delete)

        edit_menu.addSeparator()

        act_copy_cpp = QAction("&Copy C++ Snippet", self)
        act_copy_cpp.setShortcut(QKeySequence("Ctrl+Shift+C"))
        act_copy_cpp.setEnabled(False)
        edit_menu.addAction(act_copy_cpp)

        # ── View ──────────────────────────────────────────────────────────────
        view_menu = mb.addMenu("&View")

        act_zoom_in = QAction("Zoom &In", self)
        act_zoom_in.setShortcut(QKeySequence(Qt.Key.Key_Plus))
        view_menu.addAction(act_zoom_in)

        act_zoom_out = QAction("Zoom &Out", self)
        act_zoom_out.setShortcut(QKeySequence(Qt.Key.Key_Minus))
        view_menu.addAction(act_zoom_out)

        act_fit = QAction("&Fit to Window", self)
        act_fit.setShortcut(QKeySequence("Ctrl+0"))
        view_menu.addAction(act_fit)

        # ── Help ──────────────────────────────────────────────────────────────
        help_menu = mb.addMenu("&Help")

        act_about = QAction("&About SpriteCutter", self)
        act_about.triggered.connect(self._on_about)
        help_menu.addAction(act_about)

    # ── 6.2  Toolbar ──────────────────────────────────────────────────────────

    def _setup_toolbar(self) -> None:
        self._toolbar = SpriteCutterToolbar(self)
        self._toolbar.mode_changed.connect(self._on_mode_changed)
        self.addToolBar(Qt.ToolBarArea.TopToolBarArea, self._toolbar)

        # Disable AI until MobileSAM confirmed available
        try:
            import mobile_sam  # noqa: F401
            self._toolbar.set_ai_enabled(True)
        except ImportError:
            self._toolbar.set_ai_enabled(False)

    # ── 6.3  Central layout ───────────────────────────────────────────────────

    def _setup_central(self) -> None:
        central = QWidget()
        self.setCentralWidget(central)

        layout = QHBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # 7.1  Real image viewer (replaces placeholder canvas)
        self._viewer = ImageViewer()
        self._viewer.pixel_hovered.connect(
            lambda x, y: (
                self.set_cursor_coords(x, y),
            )
        )
        self._viewer.pixel_left_image.connect(self.clear_cursor_coords)
        self._viewer.zoom_changed.connect(
            lambda z: self.set_zoom(z)
        )
        layout.addWidget(self._viewer)

        # Sidebar (right, fixed width — replaced in Phase 9)
        self._sidebar_placeholder = _PlaceholderSidebar()
        layout.addWidget(self._sidebar_placeholder)

    # ── 6.4  Status bar ───────────────────────────────────────────────────────

    def _setup_statusbar(self) -> None:
        sb = self.statusBar()

        self._lbl_coords = QLabel("(—, —)")
        self._lbl_coords.setMinimumWidth(90)
        self._lbl_zoom   = QLabel("Zoom: 100%")
        self._lbl_zoom.setMinimumWidth(90)
        self._lbl_count  = QLabel("Sprites: 0")
        self._lbl_count.setMinimumWidth(80)
        self._lbl_mode   = QLabel("Mode: Select")
        self._lbl_mode.setMinimumWidth(100)
        self._lbl_image  = QLabel("No image")
        self._lbl_image.setMinimumWidth(200)

        # permanent right-side widgets
        sb.addPermanentWidget(self._lbl_image)
        sb.addPermanentWidget(_sep())
        sb.addPermanentWidget(self._lbl_count)
        sb.addPermanentWidget(_sep())
        sb.addPermanentWidget(self._lbl_mode)
        sb.addPermanentWidget(_sep())
        sb.addPermanentWidget(self._lbl_zoom)
        sb.addPermanentWidget(_sep())
        sb.addPermanentWidget(self._lbl_coords)

    # ── Keyboard shortcuts (modes: 1-4) ───────────────────────────────────────

    def _setup_shortcuts(self) -> None:
        mode_keys = {
            Qt.Key.Key_1: EditorMode.SELECT,
            Qt.Key.Key_2: EditorMode.DRAW,
            Qt.Key.Key_3: EditorMode.AI,
            Qt.Key.Key_4: EditorMode.GRID,
        }
        for key, mode in mode_keys.items():
            act = QAction(self)
            act.setShortcut(QKeySequence(key))
            act.triggered.connect(lambda _, m=mode: self._toolbar.set_mode(m))
            self.addAction(act)

    # ── Public API (used by later phases) ─────────────────────────────────────

    def set_image_path(self, path: Path) -> None:
        """Called from Phase 7 ImageViewer when image is loaded."""
        self._image_path = path
        self._act_export.setEnabled(True)
        self._act_import.setEnabled(True)
        self._act_undo.setEnabled(True)
        self._update_title()
        self._update_statusbar()

    def set_sprite_count(self, count: int) -> None:
        self._sprite_count = count
        self._lbl_count.setText(f"Sprites: {count}")

    def set_zoom(self, zoom: float) -> None:
        self._zoom_level = zoom
        self._lbl_zoom.setText(f"Zoom: {int(zoom * 100)}%")

    def set_cursor_coords(self, x: int, y: int) -> None:
        self._lbl_coords.setText(f"({x}, {y})")

    def clear_cursor_coords(self) -> None:
        self._lbl_coords.setText("(—, —)")

    # ── Slots ─────────────────────────────────────────────────────────────────

    def _on_open_image(self) -> None:
        """7.1 — File → Open Image: load into real ImageViewer."""
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Open Sprite Sheet",
            str(self._image_path.parent) if self._image_path else "",
            "Images (*.png *.bmp *.jpg *.jpeg *.gif *.tga *.webp);;All Files (*)",
        )
        if not path_str:
            return
        path = Path(path_str)
        ok = self._viewer.load_image(path)
        if not ok:
            self.statusBar().showMessage(f"Failed to load: {path.name}", 4000)
            return
        self.set_image_path(path)
        w, h = self._viewer.image_size()
        self.statusBar().showMessage(
            f"Opened: {path.name}  ({w}×{h} px)", 4000
        )

    def _on_mode_changed(self, mode: EditorMode) -> None:
        self._lbl_mode.setText(f"Mode: {mode.name.capitalize()}")

    def _on_about(self) -> None:
        try:
            from pathlib import Path as P
            ver = (P(__file__).parent.parent / "VERSION").read_text().strip()
        except Exception:
            ver = "0.1.0"
        QMessageBox.about(
            self,
            "About SpriteCutter",
            f"<h2>SpriteCutter v{ver}</h2>"
            "<p>The fastest way to extract sprite coordinates from sprite sheets.</p>"
            "<p>Built with PyQt6 · OpenCV · MobileSAM</p>"
            "<p>MIT License</p>",
        )

    # ── Helpers ───────────────────────────────────────────────────────────────

    def _update_title(self) -> None:
        if self._image_path:
            self.setWindowTitle(f"SpriteCutter — {self._image_path.name}")
        else:
            self.setWindowTitle("SpriteCutter")

    def _update_statusbar(self) -> None:
        if self._image_path:
            self._lbl_image.setText(self._image_path.name)
        else:
            self._lbl_image.setText("No image")


def _sep() -> QLabel:
    """Thin vertical separator for status bar."""
    lbl = QLabel("|")
    lbl.setStyleSheet("color: #2a2a4a; padding: 0 4px;")
    return lbl

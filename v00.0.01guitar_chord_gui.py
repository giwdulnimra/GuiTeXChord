from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QTabWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
    QLabel, QLineEdit, QComboBox, QCheckBox, QPushButton,
    QFileDialog, QMessageBox, QSizePolicy
)
from PyQt6.QtCore import Qt, pyqtSignal
import subprocess
import tempfile
import os
import sys

# --- Custom button for representing fret states ---
class FretButton(QPushButton):
    STATES = ['none', 'tone', 'root', 'mute']

    def __init__(self, string_index, fret, parent=None):
        super().__init__(str(fret), parent)
        self.string_index = string_index
        self.fret = fret
        self.state_index = 0
        self.update_style()
        self.clicked.connect(self.on_click)

    def on_click(self):
        # Cycle state
        self.state_index = (self.state_index + 1) % len(self.STATES)
        self.update_style()
        # Notify parent chord tab to reset other buttons on same string
        self.parent().button_clicked(self)

    def update_style(self):
        state = self.STATES[self.state_index]
        # Simple stylesheet: use background color to distinguish
        styles = {
            'none': 'background: none;',  
            'tone': 'background: black; color: white;',  
            'root': 'background: white; border: 2px solid black;',  
            'mute': 'color: red;'
        }
        self.setStyleSheet(styles[state])

    def reset(self):
        self.state_index = 0
        self.update_style()

    def get_state(self):
        return self.STATES[self.state_index]

# --- Tab representing one tuning ---
class ChordTab(QWidget):
    def __init__(self, strings=6, tuning=None, parent=None):
        super().__init__(parent)
        self.strings = strings
        self.tuning = tuning or []
        self.buttons = []  # 2D list: [string][fret]
        self.init_ui()

    def init_ui(self):
        main_layout = QVBoxLayout(self)
        # Top controls
        ctrl_layout = QHBoxLayout()
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText('Chord Name')
        self.bar_check = QCheckBox('Bar chord')
        self.fret_combo = QComboBox()
        self.fret_combo.addItems([str(i) for i in range(1, 13)])
        ctrl_layout.addWidget(QLabel('Name:'))
        ctrl_layout.addWidget(self.name_edit)
        ctrl_layout.addWidget(self.bar_check)
        ctrl_layout.addWidget(QLabel('Start Fret:'))
        ctrl_layout.addWidget(self.fret_combo)
        main_layout.addLayout(ctrl_layout)
        # Fret grid
        grid = QGridLayout()
        max_frets = 5
        for i, string in enumerate(self.tuning):
            grid.addWidget(QLabel(string), i, 0)
            row_buttons = []
            for fret in range(0, max_frets+1):
                btn = FretButton(i, fret, self)
                btn.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
                grid.addWidget(btn, i, fret+1)
                row_buttons.append(btn)
            self.buttons.append(row_buttons)
        main_layout.addLayout(grid)
        # Bottom controls
        bottom = QHBoxLayout()
        self.format_combo = QComboBox()
        self.format_combo.addItems(['PDF', 'PNG', 'JPG', 'SVG'])
        self.gen_btn = QPushButton('Generate')
        self.save_tex_btn = QPushButton('Save .tex')
        self.save_img_btn = QPushButton('Save Image')
        bottom.addWidget(QLabel('Output:'))
        bottom.addWidget(self.format_combo)
        bottom.addStretch()
        bottom.addWidget(self.save_tex_btn)
        bottom.addWidget(self.save_img_btn)
        bottom.addWidget(self.gen_btn)
        main_layout.addLayout(bottom)
        # Connect actions
        self.gen_btn.clicked.connect(self.on_generate)
        self.save_tex_btn.clicked.connect(self.on_save_tex)
        self.save_img_btn.clicked.connect(self.on_save_image)

    def button_clicked(self, clicked_btn):
        # Reset other buttons on same string if clicked not 'none'
        if clicked_btn.get_state() != 'none':
            for btn in self.buttons[clicked_btn.string_index]:
                if btn is not clicked_btn:
                    btn.reset()
        # Automatically detect bar chord if adjacent strings same fret
        if self.bar_check.isChecked():
            return
        target_fret = clicked_btn.fret
        string_idx = clicked_btn.string_index
        # check neighbors
        for delta in (-1, 1):
            idx = string_idx + delta
            if 0 <= idx < self.strings:
                if self.buttons[idx][target_fret].get_state() != 'none':
                    self.bar_check.setChecked(True)
                    return
        self.bar_check.setChecked(False)

    def collect_data(self):
        data = []
        for i in range(self.strings):
            for fret, btn in enumerate(self.buttons[i]):
                state = btn.get_state()
                if state != 'none':
                    data.append((i, fret, state))
        return data

    def generate_latex(self):
        name = self.name_edit.text() or ''
        start_fret = self.fret_combo.currentText()
        entries = self.collect_data()
        # Build a simple tikz snippet (expand as needed)
        tikz = [r"\\begin{tikzpicture}"]
        tikz.append(f"% Chord: {name}, start fret {start_fret}")
        for string, fret, state in entries:
            cmd = f"% string {string}, fret {fret}, state {state}"
            tikz.append(cmd)
        tikz.append(r"\\end{tikzpicture}")
        return '\\n'.join(tikz)

    def compile_latex(self, tex_code, out_format):
        # Write to temp dir
        with tempfile.TemporaryDirectory() as tmp:
            tex_file = os.path.join(tmp, 'chord.tex')
            with open(tex_file, 'w') as f:
                f.write(tex_code)
            # Run pdflatex
            subprocess.run(['pdflatex', '-interaction=nonstopmode', tex_file], cwd=tmp, check=True)
            pdf = os.path.join(tmp, 'chord.pdf')
            if out_format == 'PDF':
                return pdf
            # Convert to image if needed
            img = os.path.join(tmp, f'chord.{out_format.lower()}')
            subprocess.run(['convert', pdf, img], check=True)
            return img

    def on_generate(self):
        tex = self.generate_latex()
        fmt = self.format_combo.currentText()
        try:
            out = self.compile_latex(tex, fmt)
            QMessageBox.information(self, 'Success', f'Generated {out}')
        except Exception as e:
            QMessageBox.critical(self, 'Error', str(e))

    def on_save_tex(self):
        path, _ = QFileDialog.getSaveFileName(self, 'Save TeX', filter='TeX files (*.tex)')
        if path:
            with open(path, 'w') as f:
                f.write(self.generate_latex())

    def on_save_image(self):
        fmt = self.format_combo.currentText().lower()
        path, _ = QFileDialog.getSaveFileName(self, 'Save Image', filter=f'{fmt.upper()} (*.{fmt})')
        if path:
            tex = self.generate_latex()
            try:
                img = self.compile_latex(tex, fmt.upper())
                os.replace(img, path)
            except Exception as e:
                QMessageBox.critical(self, 'Error', str(e))

# --- Main application window ---
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Guitar Chord Generator')
        self.resize(800, 600)
        self.init_ui()

    def init_ui(self):
        tabs = QTabWidget()
        # Standard and drop tunings
        tabs.addTab(ChordTab(6, ['e', 'h', 'g', 'd', 'A', 'E']), '6-String Std')
        tabs.addTab(ChordTab(6, ['e', 'h', 'g', 'd', 'A', 'D']), '6-String Drop D')
        tabs.addTab(ChordTab(7, ['e', 'h', 'g', 'd', 'A', 'E', 'B']), '7-String Std')
        tabs.addTab(ChordTab(8, ['e', 'h', 'g', 'd', 'A', 'E', 'B', 'F#']), '8-String Std')
        self.setCentralWidget(tabs)

# --- Entry point ---
if __name__ == '__main__':
    app = QApplication(sys.argv)
    win = MainWindow()
    win.show()
    sys.exit(app.exec())

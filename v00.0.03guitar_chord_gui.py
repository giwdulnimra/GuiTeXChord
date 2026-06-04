#https://chatgpt.com/c/685dabf4-896c-800a-bed0-4aa0da24c8cf

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QTabWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
    QLabel, QLineEdit, QComboBox, QCheckBox, QPushButton,
    QFileDialog, QMessageBox, QSizePolicy
)
from PyQt6.QtGui import QPainter, QPen, QColor, QBrush
from PyQt6.QtCore import Qt
import subprocess
import tempfile
import os
import sys

#%% --- Custom button for representing fret states ---
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
        self.state_index = (self.state_index + 1) % len(self.STATES)
        self.update_style()
        self.parent().button_clicked(self)
        # trigger preview redraw
        self.parent().preview.update()

    def update_style(self):
        state = self.STATES[self.state_index]
        styles = {
            'none': 'background: none;',  
            'tone': 'background: none; color: black; font-weight: bold;',  
            'root': 'background: none; color: red; font-weight: bold;',  
            'mute': 'background: none; color: gray; font-style: italic;'
        }
        self.setStyleSheet(styles[state])

    def reset(self):
        self.state_index = 0
        self.update_style()

    def get_state(self):
        return self.STATES[self.state_index]

#%% --- Tab representing one tuning ---
class ChordTab(QWidget):
    def __init__(self, strings=6, tuning=None, parent=None):
        super().__init__(parent)
        self.strings = strings
        self.tuning = tuning or []
        self.buttons = []  # 2D list: [string][fret]
        
        self.main_layout = QVBoxLayout(self)
        ctrl_layout = QVBoxLayout(self)
        sub_layout = QHBoxLayout(self)
        name_layout = QHBoxLayout(self)
        
        self.combox_key1 = QComboBox()
        self.combox_key1.addItems(['A', 'B', 'C', 'D', 'E', 'F', 'G'])
        self.combox_key2 = QComboBox()
        self.combox_key2.addItems(['', '#', 'b'])
        self.ledit_name = QLineEdit()
        self.ledit_name.setPlaceholderText('additions')
        self.label_name = QLabel('')
        name_layout.addWidget(self.combox_key1)
        name_layout.addWidget(self.combox_key2)
        name_layout.addWidget(self.ledit_name)
        name_layout.addWidget(self.label_name)
        
        self.label_mightneedthat = QLabel('')
        self.label_output =QLabel('Output Configuration')
        self.button_tex = QPushButton('Generate TEX-File')
        self.button_tex.clicked.connect(self.generate_tex)
        self.combox_format = QComboBox()
        self.combox_format.addItems(['PDF', 'PNG', 'JPG', 'SVG'])
        self.button_generate = QPushButton('Generate File')
        #self.gen_btn.clicked.connect(self.on_generate)
        #self.save_img_btn.clicked.connect(self.on_save_image)
        ctrl_layout.addWidget(self.label_mightneedthat)
        ctrl_layout.addWidget(self.label_output)
        ctrl_layout.addWidget(self.button_tex)
        ctrl_layout.addWidget(self.combox_format)
        ctrl_layout.addWidget(self.button_generate)
        
        # Fret grid
        grid = QGridLayout()
        max_frets = 5
        self.combox_fret = QComboBox()
        self.combox_fret.addItems([str(i) for i in range(1, 13)])
        self.checkb_bar = QCheckBox('Bar chord')
        grid.addWidget(self.combox_fret, 0, 1, 1, 1)
        grid.addWidget(self.checkb_bar, 0, 2, 1, 4)
        for i, string in enumerate(self.tuning):
            grid.addWidget(QLabel(string), i+1, 0)
            row_buttons = []
            for fret in range(0, max_frets+1):
                btn = FretButton(i+1, fret, self)
                btn.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
                grid.addWidget(btn, i, fret+1)
                row_buttons.append(btn)
            self.buttons.append(row_buttons)
            
        sub_layout.addLayout(grid)
        sub_layout.addLayout(ctrl_layout)
        self.main_layout.addLayout(name_layout)
        self.main_layout.addLayout(sub_layout)


    def button_clicked(self, clicked_btn):
        if clicked_btn.get_state() != 'none':
            for btn in self.buttons[clicked_btn.string_index]:
                if btn is not clicked_btn:
                    btn.reset()
        # auto bar chord
        if self.bar_check.isChecked():
            return
        target_fret = clicked_btn.fret
        si = clicked_btn.string_index
        for delta in (-1, 1):
            ni = si + delta
            if 0 <= ni < self.strings:
                if self.buttons[ni][target_fret].get_state() != 'none':
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
        tikz = [r"\\documentclass[tikz, border=2pt]{standalone}",
                r"\\begin{document}",
                r"\\begin{tikzpicture}"]
        # ... build full diagram here, omitted for brevity
        tikz.append(r"\\end{tikzpicture}")
        tikz.append(r"\\end{document}")
        return '\\n'.join(tikz)

    def compile_latex(self, tex_code, out_format):
        with tempfile.TemporaryDirectory() as tmp:
            tex_file = os.path.join(tmp, 'chord.tex')
            with open(tex_file, 'w') as f:
                f.write(tex_code)
            subprocess.run(['pdflatex', '-interaction=nonstopmode', tex_file], cwd=tmp, check=True)
            pdf = os.path.join(tmp, 'chord.pdf')
            if out_format == 'PDF':
                return pdf
            img = os.path.join(tmp, f'chord.{out_format.lower()}')
            subprocess.run(['convert', pdf, img], check=True)
            return img

    def on_generate(self):
        tex = self.generate_latex()
        fmt = self.format_combo.currentText()
        try:
            out = self.compile_latex(tex, fmt)
            QMessageBox.information(self, 'Success', f'Generated: {out}')
        except Exception as e:
            QMessageBox.critical(self, 'Error', str(e))

    def generate_tex(self):
        path, _ = QFileDialog.getSaveFileName(self, 'Save TeX', self.output_dir, 'TeX Files (*.tex)')
        if path:
            with open(path, 'w') as f:
                f.write(self.generate_latex())

    def on_save_image(self):
        fmt = self.format_combo.currentText().lower()
        path, _ = QFileDialog.getSaveFileName(self, 'Save Image', self.output_dir, f'{fmt.upper()} (*.{fmt})')
        if path:
            tex = self.generate_latex()
            try:
                img = self.compile_latex(tex, fmt.upper())
                os.replace(img, path)
            except Exception as e:
                QMessageBox.critical(self, 'Error', str(e))

#%% --- Main application window ---
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('Guitar Chord Generator')
        self.resize(900, 700)
        self.output_dir = os.getcwd() + '/generate'
        
        
        dir_layout = QHBoxLayout()
        self.dir_edit = QLineEdit(self.output_dir)
        self.dir_edit.setReadOnly(True)
        browse_btn = QPushButton('Browse...')
        browse_btn.clicked.connect(self.select_output_dir)
        dir_layout.addWidget(QLabel('Output Dir:'))
        dir_layout.addWidget(self.dir_edit)
        dir_layout.addWidget(browse_btn)
        
        tabs = QTabWidget()
        tabs.addTab(ChordTab(6, ['e', 'h', 'g', 'd', 'A', 'E']), '6-String Std')
        tabs.addTab(ChordTab(6, ['e', 'h', 'g', 'd', 'A', 'D']), '6-String Drop-D')
        tabs.addTab(ChordTab(7, ['e', 'h', 'g', 'd', 'A', 'E', 'B']), '7-String Std')
        tabs.addTab(ChordTab(7, ['e', 'h', 'g', 'd', 'A', 'E', 'A']), '7-String Drop-A')
        tabs.addTab(ChordTab(8, ['e', 'h', 'g', 'd', 'A', 'E', 'B', 'F#']), '8-String Std')
        
        self.main_layout = QVBoxLayout(self)
        self.main_layout.addLayout(dir_layout)
        self.main_layout.addWidget(tabs)
        self.setLayout(self.main_layout)
        
    def select_output_dir(self):
        path = QFileDialog.getExistingDirectory(self, 'Select Output Directory', self.output_dir)
        if path:
            self.output_dir = path
            self.dir_edit.setText(path)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    win = MainWindow()
    win.show()
    sys.exit(app.exec())

import os
import sys
import numpy as np
import soundfile as sf
from datetime import datetime
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                           QHBoxLayout, QPushButton, QLabel, QLineEdit, QFrame)
from PyQt6.QtCore import Qt
import pyqtgraph as pg
from pyqtgraph import exporters


class AudioProcessorApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("JaDe Audio Processor")
        self._updating = False  # Add flag to prevent recursive updates
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f0f0f0;
            }
            QLabel {
                font-size: 12px;
            }
            QPushButton {
                background-color: #2196F3;
                color: white;
                border: none;
                padding: 5px 15px;
                border-radius: 3px;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #1976D2;
            }
            QPushButton:pressed {
                background-color: #0D47A1;
            }
            QLineEdit {
                padding: 5px;
                border: 1px solid #ccc;
                border-radius: 3px;
                background-color: white;
            }
        """)
        
        # Initialize variables
        self.current_file_idx = 0
        self.wav_files = [f for f in os.listdir('.') if f.lower().endswith('.wav')]
        self.dragging_threshold = False
        
        # Create main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QHBoxLayout(main_widget)
        
        # Create control panel
        control_panel = self.create_control_panel()
        layout.addWidget(control_panel)
        
        # Create plots panel
        plots_panel = self.create_plots_panel()
        layout.addWidget(plots_panel, stretch=1)
        
        # Set window size
        self.resize(1200, 800)
        
        # Load first file if available
        if self.wav_files:
            self.load_wav_file(self.wav_files[self.current_file_idx])
    
    def create_control_panel(self):
        panel = QFrame()
        panel.setFrameStyle(QFrame.Shape.Panel | QFrame.Shadow.Raised)
        panel.setStyleSheet("QFrame { background-color: white; padding: 10px; }")
        layout = QVBoxLayout(panel)
        
        # Parameters section
        params_group = QFrame()
        params_layout = QVBoxLayout(params_group)
        
        # Threshold control
        thresh_layout = QHBoxLayout()
        self.thresh_input = QLineEdit("1.7")
        thresh_layout.addWidget(QLabel("Threshold Multiplier:"))
        thresh_layout.addWidget(self.thresh_input)
        params_layout.addLayout(thresh_layout)
        
        # Length control
        length_layout = QHBoxLayout()
        self.length_input = QLineEdit("0.35")
        length_layout.addWidget(QLabel("Minimum Length (s):"))
        length_layout.addWidget(self.length_input)
        params_layout.addLayout(length_layout)
        
        # Apply button
        apply_btn = QPushButton("Apply Parameters")
        apply_btn.clicked.connect(self.apply_parameters)
        params_layout.addWidget(apply_btn)
        
        layout.addWidget(params_group)
        
        # Metadata section
        metadata_group = QFrame()
        metadata_layout = QVBoxLayout(metadata_group)
        
        # Subject name
        subject_layout = QHBoxLayout()
        self.subject_input = QLineEdit("Subject")
        subject_layout.addWidget(QLabel("Subject Name:"))
        subject_layout.addWidget(self.subject_input)
        metadata_layout.addLayout(subject_layout)
        
        # Experimenter name
        exp_layout = QHBoxLayout()
        self.experimenter_input = QLineEdit("Experimenter")
        exp_layout.addWidget(QLabel("Experimenter:"))
        exp_layout.addWidget(self.experimenter_input)
        metadata_layout.addLayout(exp_layout)
        
        layout.addWidget(metadata_group)
        
        # Action buttons
        actions_group = QFrame()
        actions_layout = QVBoxLayout(actions_group)
        
        save_btn = QPushButton("Save")
        save_btn.clicked.connect(self.save_results)
        actions_layout.addWidget(save_btn)
        
        process_all_btn = QPushButton("Process All Files")
        process_all_btn.clicked.connect(self.process_all_files)
        actions_layout.addWidget(process_all_btn)
        
        layout.addWidget(actions_group)
        
        # Navigation buttons
        nav_group = QFrame()
        nav_layout = QHBoxLayout(nav_group)
        
        prev_btn = QPushButton("Previous File")
        prev_btn.clicked.connect(self.prev_file)
        next_btn = QPushButton("Next File")
        next_btn.clicked.connect(self.next_file)
        
        nav_layout.addWidget(prev_btn)
        nav_layout.addWidget(next_btn)
        
        layout.addWidget(nav_group)
        
        # Status bar
        self.status_label = QLabel("")
        self.status_label.setStyleSheet("QLabel { color: #666; }")
        layout.addWidget(self.status_label)
        
        layout.addStretch()
        return panel
    
    def create_plots_panel(self):
        panel = QFrame()
        layout = QVBoxLayout(panel)
        
        # Waveform plot
        self.wave_plot = pg.PlotWidget()
        self.wave_plot.setBackground('white')
        self.wave_plot.setLabel('left', 'Amplitude')
        self.wave_plot.setLabel('bottom', 'Time (s)')
        self.wave_plot.showGrid(x=True, y=True)
        
        # Add threshold line with enhanced interaction area
        self.threshold_line = pg.InfiniteLine(
            angle=0, 
            movable=True, 
            pen=pg.mkPen('r', width=2),
            hoverPen=pg.mkPen('r', width=3),
            span=(0, 1)  # Make line span full width
        )
        # Create a semi-transparent region around the threshold line for easier interaction
        self.threshold_region = pg.LinearRegionItem(
            orientation=pg.LinearRegionItem.Horizontal,
            brush=(255, 0, 0, 30),
            hoverBrush=(255, 0, 0, 50),
            movable=True
        )
        self.threshold_region.sigRegionChanged.connect(self.on_region_changed)
        self.threshold_line.sigDragged.connect(self.on_threshold_changed)
        
        # Add both items to the plot
        self.wave_plot.addItem(self.threshold_region)
        self.wave_plot.addItem(self.threshold_line)
        
        self.vertical_cursor = pg.InfiniteLine(angle=90, movable=False, pen=pg.mkPen('g', width=1))
        self.wave_plot.addItem(self.vertical_cursor)
        
        # Mouse interaction
        self.wave_plot.scene().sigMouseMoved.connect(self.on_mouse_moved)
        
        layout.addWidget(self.wave_plot)
        
        # Results plot
        self.results_plot = pg.PlotWidget()
        self.results_plot.setBackground('white')
        self.results_plot.setLabel('left', 'Interval (ms)')
        self.results_plot.setLabel('bottom', 'Pulse Sequence Number')
        self.results_plot.showGrid(x=True, y=True)
        
        layout.addWidget(self.results_plot)
        
        return panel
    
    def custom_find_peaks(self, data, threshold, min_distance):
        peaks = []
        i = 0
        while i < len(data):
            if data[i] > threshold:
                # Found potential peak, look for maximum in this region
                peak_val = data[i]
                peak_idx = i
                
                while i < len(data) and data[i] > threshold:
                    if data[i] > peak_val:
                        peak_val = data[i]
                        peak_idx = i
                    i += 1
                
                peaks.append(peak_idx)
                # Skip forward by min_distance
                i = peak_idx + min_distance
            else:
                i += 1
        
        return np.array(peaks)
    
    def process_wav_file(self, wav_path, thresh_var=1.7, length_var=0.35):
        data, sample_rate = sf.read(wav_path)
        if len(data.shape) > 1:
            data = data[:, 0]
        
        threshold = thresh_var * np.mean(np.abs(data))
        min_sample_distance = int(length_var * sample_rate)
        
        peaks = self.custom_find_peaks(data, threshold, min_sample_distance)
        
        self.pulse_times = peaks / sample_rate
        self.intervals = np.diff(self.pulse_times) * 1000
        self.base_name = os.path.splitext(wav_path)[0]
        
        return data, sample_rate, threshold, min_sample_distance, peaks
    
    def load_wav_file(self, filename):
        self.data, self.sample_rate, self.threshold, self.min_distance, self.peaks = self.process_wav_file(
            filename,
            float(self.thresh_input.text()),
            float(self.length_input.text())
        )
        self.update_displays()
        self.status_label.setText(f"Loaded: {filename}")
    
    def update_displays(self):
        # Update waveform plot
        self.wave_plot.clear()
        time = np.arange(len(self.data)) / self.sample_rate
        
        # Plot waveform
        self.wave_plot.plot(time, self.data, pen=pg.mkPen('b', width=1))
        
        # Update threshold line and region
        self.threshold_line.setValue(self.threshold)
        self.threshold_region.setRegion([
            self.threshold - 0.1,  # Height of interaction region
            self.threshold + 0.1
        ])
        self.wave_plot.addItem(self.threshold_region)
        self.wave_plot.addItem(self.threshold_line)
        
        # Add vertical cursor back
        self.wave_plot.addItem(self.vertical_cursor)
        
        # Plot peaks and min_distance regions
        for peak_idx in self.peaks:
            peak_time = peak_idx / self.sample_rate
            # Vertical line at peak
            self.wave_plot.addItem(pg.InfiniteLine(peak_time, angle=90, pen=pg.mkPen('r', width=1, alpha=100)))
            # Shaded region
            end_time = min(peak_time + self.min_distance/self.sample_rate, time[-1])
            region = pg.LinearRegionItem([peak_time, end_time], movable=False, brush=pg.mkBrush(255, 0, 0, 30))
            self.wave_plot.addItem(region)
        
        # Update results plot
        self.results_plot.clear()
        if hasattr(self, 'intervals'):
            self.results_plot.plot(self.intervals, symbol='o', pen=None, symbolBrush='b')
    
    def on_mouse_moved(self, pos):
        mouse_point = self.wave_plot.plotItem.vb.mapSceneToView(pos)
        self.vertical_cursor.setValue(mouse_point.x())
    
    def on_region_changed(self):
        """Update threshold line when region is dragged"""
        if self._updating or not hasattr(self, 'threshold_region'):
            return
            
        try:
            self._updating = True
            # Get the middle of the region as the new threshold
            minY, maxY = self.threshold_region.getRegion()
            middle = (minY + maxY) / 2
            self.threshold_line.setValue(middle)
            self.update_threshold(middle)
        finally:
            self._updating = False
    
    def on_threshold_changed(self):
        """Update threshold and region when threshold line is dragged"""
        if self._updating:
            return
            
        try:
            self._updating = True
            new_threshold = self.threshold_line.value()
            self.update_threshold(new_threshold)
        finally:
            self._updating = False
            
    def update_threshold(self, new_threshold):
        """Central method to handle threshold updates"""
        self.threshold = new_threshold
        self.thresh_input.setText(f"{self.threshold/np.mean(np.abs(self.data)):.2f}")
        
        # Update region position if it exists
        if hasattr(self, 'threshold_region'):
            region_height = 0.1  # Height of the interaction region
            self.threshold_region.setRegion([
                self.threshold - region_height/2,
                self.threshold + region_height/2
            ])
        
        # Reprocess with new threshold
        _, _, _, _, self.peaks = self.process_wav_file(
            self.wav_files[self.current_file_idx],
            float(self.thresh_input.text()),
            float(self.length_input.text())
        )
        self.update_displays()
    
    def apply_parameters(self):
        if not self.wav_files:
            return
        self.load_wav_file(self.wav_files[self.current_file_idx])
    
    def process_all_files(self):
        total = len(self.wav_files)
        for i, wav_file in enumerate(self.wav_files):
            self.status_label.setText(f"Processing {wav_file} ({i+1}/{total})")
            QApplication.processEvents()
            
            # Process and save each file
            self.load_wav_file(wav_file)
            self.save_results()
        
        self.status_label.setText("Finished processing all files")
    
    def save_results(self):
        if not hasattr(self, 'intervals'):
            return
        
        # Save .dat file
        with open(f"{self.base_name}.dat", 'w') as f:
            f.write(f"{self.subject_input.text()}\n")
            timestamp = datetime.now().strftime("%d/%m/%Y   %H:%M")
            f.write(f"\t{self.experimenter_input.text()}/{self.base_name} recorded with JaDe_V2 {timestamp}\n\n")
            for interval in self.intervals:
                f.write(f"{int(round(interval))}\n")
        
        # Save plot
        exporter = exporters.ImageExporter(self.results_plot.plotItem)
        exporter.parameters()['width'] = 1200
        exporter.export(f"{self.base_name}.jpg")
        
        self.status_label.setText(f"Saved {self.base_name}.dat and .jpg")
    
    def next_file(self):
        if self.current_file_idx < len(self.wav_files) - 1:
            self.current_file_idx += 1
            self.load_wav_file(self.wav_files[self.current_file_idx])
    
    def prev_file(self):
        if self.current_file_idx > 0:
            self.current_file_idx -= 1
            self.load_wav_file(self.wav_files[self.current_file_idx])


if __name__ == '__main__':
    # Enable high DPI scaling
    if hasattr(Qt, 'AA_EnableHighDpiScaling'):
        QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    if hasattr(Qt, 'AA_UseHighDpiPixmaps'):
        QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps, True)
    
    app = QApplication(sys.argv)
    window = AudioProcessorApp()
    window.show()
    sys.exit(app.exec())

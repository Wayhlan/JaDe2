## Dependencies

The application requires Python 3.x and the following libraries:

```bash
pip install PyQt6
pip install pyqtgraph
pip install soundfile
pip install numpy
```

## Usage

1. Run the application:
   ```bash
   python main.py
   ```

2. The application will automatically load WAV files from the current directory.

3. Adjust processing parameters:
   - Threshold Multiplier: Controls peak detection sensitivity
   - Minimum Length: Sets minimum time between peaks

4. Interactive features:
   - Drag the red threshold line to adjust detection sensitivity
   - Use Previous/Next buttons to navigate between files
   - Click "Process All Files" for batch processing
   - Save results as .dat and .jpg files

## Data Output

- `.dat` files contain interval measurements and metadata
- `.jpg` files show interval plots for visual analysis

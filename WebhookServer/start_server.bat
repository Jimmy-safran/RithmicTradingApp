@echo off
echo Starting Rithmic Trading Webhook Server...
echo.
echo Make sure you have:
echo 1. Python installed
echo 2. Dependencies installed (pip install -r requirements.txt)
echo 3. RithmicTradingApp compiled and accessible
echo 4. config.ini properly configured
echo.
echo Server will start on http://localhost:5001
echo Press Ctrl+C to stop the server
echo.

python app.py

pause

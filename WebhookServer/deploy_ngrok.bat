@echo off
echo ========================================
echo Rithmic Trading Webhook Server - ngrok Deployment
echo ========================================
echo.

echo Starting Flask webhook server...
echo Server will run on http://localhost:5001
echo.
echo Starting ngrok tunnel...
echo.

REM Start the Flask server in the background
start "Flask Webhook Server" cmd /k "python app.py"

REM Wait a moment for the server to start
timeout /t 3 /nobreak > nul

REM Start ngrok tunnel
echo Starting ngrok tunnel to localhost:5001...
ngrok http 5001

echo.
echo Deployment complete!
echo.
echo Your webhook server is now accessible via the ngrok URL above.
echo.
echo You can test it using:
echo   curl -X POST https://your-ngrok-url.ngrok.io/order ^
echo     -H "Content-Type: application/json" ^
echo     -d "{\"exchange\": \"COMEX\", \"ticker\": \"MGCZ5\", \"side\": \"B\", \"order_type\": \"open_long\", \"price\": 0.0, \"quantity\": 1}"
echo.
pause

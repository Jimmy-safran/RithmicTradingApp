# Rithmic Trading Webhook Server

A Flask-based webhook server that provides a REST API interface to the RithmicTradingApp for placing various types of trading orders.

## Features

- REST API interface for all order types supported by RithmicTradingApp
- Input validation and error handling
- Comprehensive logging
- Health check endpoint
- Order type documentation endpoint

## Setup

1. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Update the RithmicTradingApp path:**
   Edit `app.py` and update the `RITHMIC_APP_PATH` variable to point to your compiled RithmicTradingApp executable:
   ```python
   RITHMIC_APP_PATH = "../bin/RithmicTradingApp.exe"  # Windows
   # RITHMIC_APP_PATH = "../bin/RithmicTradingApp"   # Linux/Mac
   ```

3. **Ensure config.ini is properly configured:**
   Make sure your `config.ini` file contains valid username and password in the `[Login]` section.

## Running the Server

```bash
python app.py
```

The server will start on `http://localhost:5000`

## API Endpoints

### GET /
Returns basic server information and available endpoints.

### GET /health
Health check endpoint.

### GET /order_types
Returns detailed information about all available order types with examples.

### POST /order
Main endpoint for placing orders.

**Request Body (JSON):**
```json
{
    "exchange": "CBOT",
    "ticker": "ZCU5",
    "side": "B",
    "order_type": "open_long",
    "price": 450.25,
    "quantity": 1
}
```

## Order Types

### Basic Orders
- **open_long**: Open long position
- **open_short**: Open short position
- **exit_long**: Exit long position (market order)
- **exit_short**: Exit short position (market order)

### Stop Orders
- **stop_long**: Place stop order for long position
- **stop_short**: Place stop order for short position

### Order Management
- **modify**: Modify existing order
- **cancel**: Cancel existing order

### Advanced Orders
- **bracket**: Bracket order with entry/stop/target
- **trailing_stop**: Trailing stop order
- **time_based**: Time-based order (GTC, IOC, FOK)

## Example Usage

### 1. Market Order (Open Long)
```bash
curl -X POST http://localhost:5000/order \
  -H "Content-Type: application/json" \
  -d '{
    "exchange": "CBOT",
    "ticker": "ZCU5",
    "side": "B",
    "order_type": "open_long",
    "price": 0.0,
    "quantity": 1
  }'
```

### 2. Limit Order (Open Short)
```bash
curl -X POST http://localhost:5000/order \
  -H "Content-Type: application/json" \
  -d '{
    "exchange": "CBOT",
    "ticker": "ZCU5",
    "side": "S",
    "order_type": "open_short",
    "price": 450.25,
    "quantity": 1
  }'
```

### 3. Bracket Order
```bash
curl -X POST http://localhost:5000/order \
  -H "Content-Type: application/json" \
  -d '{
    "exchange": "CBOT",
    "ticker": "ZCU5",
    "side": "B",
    "order_type": "bracket",
    "entry_price": 450.25,
    "stop_price": 440.00,
    "target_price": 460.00,
    "quantity": 1
  }'
```

### 4. Cancel Order
```bash
curl -X POST http://localhost:5000/order \
  -H "Content-Type: application/json" \
  -d '{
    "exchange": "CBOT",
    "ticker": "ZCU5",
    "side": "B",
    "order_type": "cancel",
    "order_num": "177693517"
  }'
```

## Response Format

### Success Response
```json
{
    "success": true,
    "timestamp": "2024-01-15T10:30:00.123456",
    "request": {...},
    "command": "RithmicTradingApp.exe CBOT ZCU5 B open_long 450.25 1",
    "message": "Order executed successfully",
    "output": "..."
}
```

### Error Response
```json
{
    "success": false,
    "timestamp": "2024-01-15T10:30:00.123456",
    "request": {...},
    "command": "RithmicTradingApp.exe CBOT ZCU5 B open_long 450.25 1",
    "error": "Error message",
    "stderr": "..."
}
```

## Notes

- The server executes the RithmicTradingApp as a subprocess
- All orders are executed with a 60-second timeout
- Username and password are read from the config.ini file
- The server logs all requests and responses for debugging
- Make sure the RithmicTradingApp is compiled and accessible from the specified path

## ngrok Deployment

To expose your webhook server to the internet using ngrok:

### 1. Install ngrok
```bash
# Download from https://ngrok.com/download
# Or install via package manager
choco install ngrok  # Windows
```

### 2. Sign up and configure ngrok
```bash
# Sign up at https://ngrok.com/signup
# Get your authtoken from the dashboard
ngrok config add-authtoken YOUR_TOKEN_HERE
```

### 3. Deploy using the provided script
```bash
deploy_ngrok.bat
```

### 4. Test the ngrok URL
```bash
# Copy the ngrok URL from the terminal output
python test_ngrok.py https://your-ngrok-url.ngrok.io
```

### 5. Use the webhook from external systems
```bash
curl -X POST https://your-ngrok-url.ngrok.io/order \
  -H "Content-Type: application/json" \
  -d '{
    "exchange": "COMEX",
    "ticker": "MGCZ5",
    "side": "B",
    "order_type": "open_long",
    "price": 0.0,
    "quantity": 1
  }'
```

## Security Considerations

- This server is designed for development/testing purposes
- For production use, consider adding authentication and authorization
- The server runs on all interfaces (0.0.0.0) - restrict access as needed
- Consider using HTTPS in production environments
- **Important**: When using ngrok, your server is publicly accessible. Consider:
  - Adding API key authentication
  - Rate limiting
  - IP whitelisting if possible
  - Monitoring for unauthorized access

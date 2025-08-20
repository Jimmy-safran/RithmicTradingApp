# RithmicTradingApp Project

A comprehensive C++ trading application that supports all types of orders through the Rithmic API.

## Project Structure

```
RithmicTradingAPI/
├── src/
│   └── RithmicTradingApp.cpp        # Main source file (copied from RAPI SDK)
├── include/
│   └── RApiPlus.h                   # Rithmic API header
├── bin/                             # Compiled executable
├── Makefile                         # Build configuration
├── build.bat                        # Windows build script
├── RithmicTradingApp.sln            # Visual Studio 2022 solution
├── RithmicTradingApp.vcxproj        # Visual Studio 2022 project
├── rithmic_ssl_cert_auth_params     # SSL certificate
└── README.md                        # This file
```

## Building

### Windows

**Option 1: Using Visual Studio 2022 (Recommended)**
1. Open `RithmicTradingApp.sln` in Visual Studio 2022
2. Select `Release` configuration and `x64` platform
3. Build the solution (Ctrl+Shift+B)
4. The executable will be created in `bin\RithmicTradingApp.exe`

**Option 2: Using build.bat (MinGW)**
```cmd
build.bat
```

**Option 3: Using Visual Studio Command Line**
```cmd
# Open Visual Studio Developer Command Prompt
cl src\RithmicTradingApp.cpp /I./include /I../rapi-sdk/13.5.0.0/include /link /LIBPATH:../rapi-sdk/13.5.0.0/win10/lib RApiPlus-optimize.lib OmneStreamEngine-optimize.lib OmneChannel-optimize.lib OmneEngine-optimize.lib _api-optimize.lib _apipoll-stubs-optimize.lib _kit-optimize.lib ssl.lib crypto.lib /OUT:bin\RithmicTradingApp.exe
```

### Linux/macOS

```bash
make
```

## Running

```bash
# Windows
bin\RithmicTradingApp.exe [username] [password] [exchange] [ticker] [side] [order_type] [parameters...]

# Linux/macOS
./bin/RithmicTradingApp [username] [password] [exchange] [ticker] [side] [order_type] [parameters...]
```

## Parameters

- `username`: Your Rithmic username
- `password`: Your Rithmic password
- `exchange`: Exchange (e.g., COMEX)
- `ticker`: Ticker symbol (e.g., MGCZ5)
- `side`: Side (B for buy, S for sell)
- `order_type`: Order type (see supported types below)
- `price`: Order price
- `quantity`: Order quantity
- `additional_params`: Additional parameters for specific order types

## Supported Order Types

| Order Type | Description | Parameters | Example |
|------------|-------------|------------|---------|
| `open_long` | Open long position with limit order | price, quantity | `open_long 3366.7 1` |
| `open_short` | Open short position with limit order | price, quantity | `open_short 3367.0 1` |
| `exit_long` | Exit long position with market order | quantity | `exit_long 1` |
| `exit_short` | Exit short position with market order | quantity | `exit_short 1` |
| `stop_long` | Place stop order for long position | stop_price, quantity | `stop_long 3350.0 1` |
| `stop_short` | Place stop order for short position | stop_price, quantity | `stop_short 3380.0 1` |
| `modify` | Modify existing order | order_number, new_price, new_quantity | `modify 177693517 3366.8 2` |
| `cancel` | Cancel existing order | order_number | `cancel 177693517` |
| `bracket` | Bracket order with entry/stop/target | entry_price, stop_price, target_price, quantity | `bracket 3366.7 3350.0 3380.0 1` |
| `trailing_stop` | Trailing stop order | stop_price, trail_ticks, quantity | `trailing_stop 3350.0 5 1` |
| `time_based` | Time-based order (GTC, IOC, FOK) | price, quantity, duration | `time_based 3366.7 1 GTC` |

## Examples

### Basic Orders
```bash
# Open long position
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B open_long 3366.7 1

# Open short position
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 S open_short 3367.0 1

# Exit long position (market order)
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 S exit_long 1

# Exit short position (market order)
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B exit_short 1
```

### Advanced Orders
```bash
# Stop order for long position
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 S stop_long 3350.0 1

# Stop order for short position
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B stop_short 3380.0 1

# Modify existing order
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B modify 177693517 3366.8 2

# Cancel existing order
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B cancel 177693517

# Bracket order (entry/stop/target)
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B bracket 3366.7 3350.0 3380.0 1

# Trailing stop order
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B trailing_stop 3350.0 5 1

# Time-based order (GTC - Good Till Cancelled)
bin\RithmicTradingApp.exe szewald.a860Paper@amp.com Sim201860 COMEX MGCZ5 B time_based 3366.7 1 GTC
```

## Prerequisites

- Rithmic API SDK (13.5.0.0) in `../rapi-sdk/13.5.0.0/`
- C++ compiler (GCC, Clang, or MSVC)
- SSL certificate file (`rithmic_ssl_cert_auth_params`)

## Troubleshooting

1. **Build errors**: Ensure the RAPI SDK path is correct
2. **SSL certificate missing**: Copy from `../rapi-sdk/13.5.0.0/etc/`
3. **Library not found**: Check that all required libraries are in the SDK lib directory
4. **Login failed**: Verify username/password and connection settings
5. **No trade routes**: Contact your FCM/IB for proper routing setup

## Features

- **Comprehensive Order Support**: All 11 order types implemented
- **Error Handling**: Robust timeout and error handling
- **Real-time Callbacks**: Order status updates and execution reports
- **Exchange Latency Tracking**: Performance monitoring
- **SSL Security**: Secure connections with certificate support
- **Cross-platform**: Windows, Linux, and macOS support

## Order Functions Implemented

The application includes all these order functions from SampleOrder.cpp:

1. `sendOpenLongOrder()` - Open long positions
2. `sendOpenShortOrder()` - Open short positions  
3. `sendRealTimeExitOrder()` - Exit positions with market orders
4. `sendStopOrder()` - Place stop orders
5. `sendModifyOrder()` - Modify existing orders
6. `sendCancelOrder()` - Cancel existing orders
7. `sendBracketOrder()` - Bracket orders with entry/stop/target
8. `sendTrailingStopOrder()` - Trailing stop orders
9. `sendTimeBasedOrder()` - Time-based orders (GTC, IOC, FOK)

All functions are fully implemented and tested with proper error handling and logging.

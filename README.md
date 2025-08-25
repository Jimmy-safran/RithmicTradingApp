# Rithmic Trading DLL

This project creates a DLL wrapper for the Rithmic Trading API.

## Files

- `rithmic_dll_wrapper.cpp` - Main DLL source file with all Rithmic functions
- `RithmicTradingDLL.vcxproj` - Visual Studio project file
- `include/RApiPlus.h` - Rithmic API header
- `lib/` - Directory for Rithmic libraries (RApiPlus.lib, etc.)
- `logs/` - Directory for log files

## Building the DLL

1. Open Visual Studio 2022
2. Open `RithmicTradingDLL.vcxproj`
3. Place your Rithmic libraries in the `lib/` directory:
   - RApiPlus.lib
   - OmneStreamEngine.lib
   - OmneChannel.lib
   - OmneEngine.lib
   - _api.lib
   - _apipoll-stubs.lib
   - _kit.lib
   - ssl.lib
   - crypto.lib
   - z.lib
   - krb5.lib
   - k5crypto.lib
   - com_err.lib
   - resolv.lib
   - m.lib
   - pthread.lib
   - rt.lib
4. Build the project in Release mode (x64)
5. The DLL will be created as `rithmic_trading.dll`

## DLL Functions

The DLL exports these functions:

- `initialize_trading_engine(username, password)` - Initialize trading engine
- `setup_instrument(exchange, ticker)` - Setup instrument for trading
- `send_open_long_order(exchange, ticker, price, quantity)` - Send open long order
- `send_open_short_order(exchange, ticker, price, quantity)` - Send open short order
- `send_exit_order(exchange, ticker, side, price, quantity)` - Send exit order
- `send_stop_order(exchange, ticker, side, stop_price, quantity)` - Send stop order
- `modify_order(exchange, ticker, order_num, new_price, new_quantity)` - Modify order
- `cancel_order(exchange, ticker, order_num)` - Cancel order
- `send_bracket_order(exchange, ticker, side, entry_price, stop_price, target_price, quantity)` - Send bracket order
- `send_trailing_stop_order(exchange, ticker, side, stop_price, trail_ticks, quantity)` - Send trailing stop order
- `send_time_based_order(exchange, ticker, side, price, quantity, duration)` - Send time-based order
- `wait_for_order_completion()` - Wait for order completion
- `cleanup_trading_engine()` - Cleanup trading engine

## Configuration

Create a `config.ini` file with your Rithmic settings:

```ini
[Environment]
MML_DMN_SRVR_ADDR=your_domain_server
MML_DOMAIN_NAME=your_domain
MML_LIC_SRVR_ADDR=your_license_server
MML_LOC_BROK_ADDR=your_broker_address
MML_LOGGER_ADDR=your_logger_address
MML_LOG_TYPE=your_log_type
MML_SSL_CLNT_AUTH_FILE=your_ssl_cert_file
USER=your_username

[Application]
AppName=YourAppName
AppVersion=1.0.0
LogFilePath=./logs/app.log

[Login]
MdCnnctPt=your_market_data_connection
TsCnnctPt=your_trading_system_connection
```

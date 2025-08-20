# RithmicTradingApp - Order Types Reference

This document provides a comprehensive reference for all order types supported by the RithmicTradingApp.

## Command Line Format

```bash
RithmicTradingApp.exe [username] [password] [exchange] [ticker] [side] [order_type] [parameters...]
```

## Supported Order Types

### 1. Basic Position Orders

#### `open_long`
Opens a long position using a limit order.

**Parameters:**
- `price`: Limit price for the order
- `quantity`: Number of contracts

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B open_long 3366.7 1
```

#### `open_short`
Opens a short position using a limit order.

**Parameters:**
- `price`: Limit price for the order
- `quantity`: Number of contracts

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 S open_short 3367.0 1
```

### 2. Exit Orders

#### `exit_long`
Exits a long position using a market order.

**Parameters:**
- `quantity`: Number of contracts to exit

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 S exit_long 1
```

#### `exit_short`
Exits a short position using a market order.

**Parameters:**
- `quantity`: Number of contracts to exit

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B exit_short 1
```

### 3. Stop Orders

#### `stop_long`
Places a stop order to exit a long position.

**Parameters:**
- `stop_price`: Stop price (triggers when market falls to this level)
- `quantity`: Number of contracts

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 S stop_long 3350.0 1
```

#### `stop_short`
Places a stop order to exit a short position.

**Parameters:**
- `stop_price`: Stop price (triggers when market rises to this level)
- `quantity`: Number of contracts

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B stop_short 3380.0 1
```

### 4. Order Management

#### `modify`
Modifies an existing order.

**Parameters:**
- `order_number`: The order number to modify
- `new_price`: New limit price
- `new_quantity`: New quantity

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B modify 177693517 3366.8 2
```

#### `cancel`
Cancels an existing order.

**Parameters:**
- `order_number`: The order number to cancel

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B cancel 177693517
```

### 5. Advanced Orders

#### `bracket`
Places a bracket order with entry, stop, and target prices.

**Parameters:**
- `entry_price`: Entry price for the position
- `stop_price`: Stop loss price
- `target_price`: Take profit price
- `quantity`: Number of contracts

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B bracket 3366.7 3350.0 3380.0 1
```

#### `trailing_stop`
Places a trailing stop order.

**Parameters:**
- `stop_price`: Initial stop price
- `trail_ticks`: Number of ticks to trail behind the market
- `quantity`: Number of contracts

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B trailing_stop 3350.0 5 1
```

#### `time_based`
Places a time-based order with specific duration.

**Parameters:**
- `price`: Limit price
- `quantity`: Number of contracts
- `duration`: Order duration (GTC, IOC, FOK, DAY)

**Example:**
```bash
RithmicTradingApp.exe user pass COMEX MGCZ5 B time_based 3366.7 1 GTC
```

## Order Durations

- `DAY`: Day order (expires at end of trading day)
- `GTC`: Good Till Cancelled (remains active until cancelled)
- `IOC`: Immediate or Cancel (fills immediately or cancels)
- `FOK`: Fill or Kill (fills completely or cancels)

## Side Parameter

- `B`: Buy (for opening long positions or exiting short positions)
- `S`: Sell (for opening short positions or exiting long positions)

## Exchange Examples

- `COMEX`: Commodity Exchange (Gold, Silver, Copper)
- `CBOT`: Chicago Board of Trade (Grains, Treasury Bonds)
- `CME`: Chicago Mercantile Exchange (E-mini S&P 500, Euro FX)
- `ICE`: Intercontinental Exchange (Crude Oil, Natural Gas)

## Ticker Symbol Examples

- `MGCZ5`: Gold Futures, December 2025
- `ZCU5`: Corn Futures, December 2025
- `ESU5`: E-mini S&P 500, September 2025
- `CLU5`: Crude Oil Futures, September 2025

## Error Handling

The application includes comprehensive error handling for:
- Invalid order parameters
- Network connectivity issues
- Authentication failures
- Order placement failures
- Timeout conditions

## Best Practices

1. **Always verify order parameters** before submission
2. **Use appropriate order types** for your trading strategy
3. **Monitor order status** through the callback system
4. **Handle errors gracefully** in production environments
5. **Test with paper trading** before live trading
6. **Understand exchange-specific rules** and trading hours
7. **Use proper risk management** with stop orders and position sizing

## Troubleshooting

### Common Issues

1. **"Login failed"**: Check username/password and connection settings
2. **"No trade routes available"**: Contact your FCM/IB for proper routing setup
3. **"Order rejected"**: Verify order parameters and exchange rules
4. **"Timeout"**: Check network connectivity and server status

### Debug Information

The application provides detailed logging for:
- Login process
- Order placement
- Callback events
- Error conditions
- Exchange latency

Enable debug output by setting appropriate environment variables or compile flags.


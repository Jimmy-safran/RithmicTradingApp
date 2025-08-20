# Configuration System

This application now uses a `config.ini` file to manage all static configuration values, making it easy to switch between different environments without recompiling.

## Configuration File Structure

The `config.ini` file is organized into three main sections:

### [Environment]
Contains all Rithmic connection parameters:
- `MML_DMN_SRVR_ADDR`: Domain server addresses
- `MML_DOMAIN_NAME`: Domain name for the trading environment
- `MML_LIC_SRVR_ADDR`: License server addresses
- `MML_LOC_BROK_ADDR`: Local broker address
- `MML_LOGGER_ADDR`: Logger server addresses
- `MML_LOG_TYPE`: Logging type
- `MML_SSL_CLNT_AUTH_FILE`: SSL certificate authentication file
- `USER`: User email/identifier

### [Application]
Contains application-specific settings:
- `AppName`: Application name identifier
- `AppVersion`: Application version
- `LogFilePath`: Path to the log file

### [Login]
Contains trading system connection points:
- `MdCnnctPt`: Market data connection point
- `TsCnnctPt`: Trading system connection point

## Usage

1. **Default Configuration**: The application comes with a default `config.ini` file configured for Rithmic Paper Trading.

2. **Config File Location**: The application looks for `config.ini` in the following locations (in order):
   - `./config.ini` (same directory as the executable)
   - `../config.ini` (parent directory)
   - `../../config.ini` (grandparent directory)

3. **Environment Switching**: To switch between environments (test, paper, live), simply modify the values in the `config.ini` file:
   - For Paper Trading: Use the current values
   - For Live Trading: Update server addresses and connection points
   - For Test Environment: Update to test server addresses

4. **Validation**: The application validates that all required configuration values are present before starting. If any values are missing, it will display an error message and exit.

## Example Configuration

```ini
[Environment]
MML_DMN_SRVR_ADDR=ritpz01004.01.rithmic.com:65000~ritpz04063.04.rithmic.com:65000
MML_DOMAIN_NAME=rithmic_paper_prod_domain
MML_LIC_SRVR_ADDR=ritpz04063.04.rithmic.com:56000~ritpz01004.01.rithmic.com:56000
MML_LOC_BROK_ADDR=ritpz04063.04.rithmic.com:64100
MML_LOGGER_ADDR=ritpz04063.04.rithmic.com:45454~ritpz01004.01.rithmic.com:45454
MML_LOG_TYPE=log_net
MML_SSL_CLNT_AUTH_FILE=rithmic_ssl_cert_auth_params
USER=your.email@domain.com

[Application]
AppName=alsz:PLIQUERAexecutor
AppVersion=1.0.0.0
LogFilePath=so.log

[Login]
MdCnnctPt=login_agent_tp_paperc
TsCnnctPt=login_agent_op_paperc
```

## Benefits

- **No Recompilation**: Change configuration without rebuilding the application
- **Environment Management**: Easy switching between test, paper, and live environments
- **Centralized Configuration**: All settings in one place
- **Validation**: Automatic validation of required configuration values
- **Maintainability**: Clear separation of configuration from code

## Error Handling

If the `config.ini` file is missing or contains invalid values:
1. The application will display a warning if the file is missing
2. The application will exit with an error if required values are missing
3. All error messages clearly indicate what needs to be fixed

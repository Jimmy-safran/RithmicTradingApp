from flask import Flask, request, jsonify
import subprocess
import os
import sys
import json
import logging
from datetime import datetime

app = Flask(__name__)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Configuration
import os

# Get the absolute path to the RithmicTradingApp executable
current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)
RITHMIC_APP_PATH = os.path.join(parent_dir, "bin", "RithmicTradingApp.exe")  # Windows executable
# RITHMIC_APP_PATH = os.path.join(parent_dir, "bin", "RithmicTradingApp")   # Linux/Mac executable



def convert_webhook_format(data):
    """Convert new webhook format to internal format"""
    converted_data = {}
    
    # Extract basic fields
    converted_data['symbol'] = data.get('symbol', '')
    converted_data['action'] = data.get('action', '')
    converted_data['side'] = data.get('side', '')
    converted_data['strategy'] = data.get('strategy', '')
    converted_data['timestamp'] = data.get('timestamp', '')
    
    # Convert action and side to order_type
    action = data.get('action', '').lower()
    side = data.get('side', '').lower()
    
    if action == 'open' and side == 'long':
        converted_data['order_type'] = 'open_long'
        converted_data['side'] = 'B'  # Buy for long
    elif action == 'open' and side == 'short':
        converted_data['order_type'] = 'open_short'
        converted_data['side'] = 'S'  # Sell for short
    elif action == 'close' and side == 'long':
        converted_data['order_type'] = 'exit_long'
        converted_data['side'] = 'S'  # Sell to exit long
    elif action == 'close' and side == 'short':
        converted_data['order_type'] = 'exit_short'
        converted_data['side'] = 'B'  # Buy to exit short
    elif action == 'modify_stop' and side == 'long':
        converted_data['order_type'] = 'stop_long'
        converted_data['side'] = 'S'  # Sell stop for long position
    elif action == 'modify_stop' and side == 'short':
        converted_data['order_type'] = 'stop_short'
        converted_data['side'] = 'B'  # Buy stop for short position
    else:
        converted_data['order_type'] = None
    
    # Extract price information
    if 'price' in data:
        try:
            converted_data['price'] = float(data['price'])
        except (ValueError, TypeError):
            converted_data['price'] = 0.0  # Default to market order
    
    # Extract stop price for modify_stop actions
    if 'stop_price' in data:
        try:
            converted_data['stop_price'] = float(data['stop_price'])
        except (ValueError, TypeError):
            converted_data['stop_price'] = 0.0
    
    # Extract quantity (default to 1 if not specified)
    converted_data['quantity'] = data.get('quantity', 1)
    
    # Extract reason for logging
    converted_data['reason'] = data.get('reason', '')
    
    return converted_data

def validate_order_request(data):
    """Validate the incoming order request"""
    required_fields = ['action', 'side', 'symbol']
    
    for field in required_fields:
        if field not in data:
            return False, f"Missing required field: {field}"
    
    # Validate action
    valid_actions = ['open', 'close', 'modify_stop']
    if data['action'].lower() not in valid_actions:
        return False, f"Action must be one of: {', '.join(valid_actions)}"
    
    # Validate side
    valid_sides = ['long', 'short']
    if data['side'].lower() not in valid_sides:
        return False, "Side must be 'long' or 'short'"
    
    # Validate symbol
    if not data['symbol']:
        return False, "Symbol cannot be empty"
    
    return True, "Valid"

def build_command_args(data, original_data):
    """Build command line arguments for the RithmicTradingApp"""
    # Extract exchange and ticker from symbol
    symbol = data['symbol']
    
    # Parse symbol to extract exchange and ticker
    # Assuming format like "MGCZ5" where we need to determine exchange
    # For now, we'll use a default exchange (this should be configurable)
    exchange = 'COMEX'  # Default exchange for metals
    ticker = symbol
    
    # You can add more sophisticated symbol parsing here
    # For example, different exchanges based on symbol patterns
    if symbol.startswith('MG'):
        exchange = 'COMEX'
    elif symbol.startswith('GC'):
        exchange = 'COMEX'
    elif symbol.startswith('SI'):
        exchange = 'COMEX'
    elif symbol.startswith('ZC'):
        exchange = 'CBOT'
    elif symbol.startswith('ZS'):
        exchange = 'CBOT'
    elif symbol.startswith('ZW'):
        exchange = 'CBOT'
    elif symbol.startswith('ES'):
        exchange = 'CME'
    elif symbol.startswith('NQ'):
        exchange = 'CME'
    elif symbol.startswith('YM'):
        exchange = 'CBOT'
    
    # Use the converted side ('B' or 'S') for the command line
    args = [
        RITHMIC_APP_PATH,
        exchange,
        ticker, 
        data['side']  # This is 'B' or 'S' after conversion
    ]
    
    # Add order type if specified
    if 'order_type' in data and data['order_type']:
        args.append(data['order_type'])
        
        # Add order-specific parameters
        order_type = data['order_type']
        
        if order_type in ['open_long', 'open_short', 'exit_long', 'exit_short']:
            if 'price' in data:
                args.append(str(data['price']))
            if 'quantity' in data:
                args.append(str(data['quantity']))
                
        elif order_type in ['stop_long', 'stop_short']:
            # For stop orders, use stop_price if available, otherwise use price
            if 'stop_price' in data:
                args.append(str(data['stop_price']))
            if 'quantity' in data:
                args.append(str(data['quantity']))
                
        elif order_type == 'modify':
            if 'order_num' in data:
                args.append(data['order_num'])
            if 'price' in data:
                args.append(str(data['price']))
            if 'quantity' in data:
                args.append(str(data['quantity']))
                
        elif order_type == 'cancel':
            if 'order_num' in data:
                args.append(data['order_num'])
                
        elif order_type == 'bracket':
            if 'entry_price' in data:
                args.append(str(data['entry_price']))
            if 'stop_price' in data:
                args.append(str(data['stop_price']))
            if 'target_price' in data:
                args.append(str(data['target_price']))
            if 'quantity' in data:
                args.append(str(data['quantity']))
                
        elif order_type == 'trailing_stop':
            if 'stop_price' in data:
                args.append(str(data['stop_price']))
            if 'trail_ticks' in data:
                args.append(str(data['trail_ticks']))
            if 'quantity' in data:
                args.append(str(data['quantity']))
                
        elif order_type == 'time_based':
            if 'price' in data:
                args.append(str(data['price']))
            if 'quantity' in data:
                args.append(str(data['quantity']))
            if 'duration' in data:
                args.append(data['duration'])
    
    return args

def execute_order(args):
    """Execute the RithmicTradingApp with the given arguments"""
    try:
        logger.info(f"Executing command: {' '.join(args)}")
        logger.info(f"Working directory: {os.getcwd()}")
        logger.info(f"Executable path: {RITHMIC_APP_PATH}")
        logger.info(f"Executable exists: {os.path.exists(RITHMIC_APP_PATH)}")
        
        # Run the command and capture output
        result = subprocess.run(
            args,
            capture_output=True,
            text=True,
            timeout=60,  # 60 second timeout
            cwd=os.path.dirname(RITHMIC_APP_PATH)  # Set working directory to bin folder
        )
        
        logger.info(f"Return code: {result.returncode}")
        logger.info(f"Stdout: {result.stdout}")
        logger.info(f"Stderr: {result.stderr}")
        
        return {
            'success': result.returncode == 0,
            'return_code': result.returncode,
            'stdout': result.stdout,
            'stderr': result.stderr
        }
        
    except subprocess.TimeoutExpired:
        logger.error("Command timed out after 60 seconds")
        return {
            'success': False,
            'error': 'Command timed out after 60 seconds'
        }
    except FileNotFoundError:
        logger.error(f"RithmicTradingApp not found at {RITHMIC_APP_PATH}")
        return {
            'success': False,
            'error': f'RithmicTradingApp not found at {RITHMIC_APP_PATH}'
        }
    except Exception as e:
        logger.error(f"Execution error: {str(e)}")
        return {
            'success': False,
            'error': f'Execution error: {str(e)}'
        }

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'service': 'Rithmic Trading Webhook Server'
    })


@app.route('/order', methods=['POST'])
def place_order():
    """Main endpoint for placing orders with enhanced content type handling"""
    try:
        # Log request details for debugging
        logger.info(f"Order request received:")
        logger.info(f"  Content-Type: {request.content_type}")
        logger.info(f"  Method: {request.method}")
        logger.info(f"  Data length: {len(request.get_data()) if request.get_data() else 0}")
        
        # Parse data with multiple fallback methods
        data = None
        
        # Method 1: Try JSON content type
        if request.is_json:
            data = request.get_json()
            logger.info("Parsed JSON from application/json content type")
        
        # Method 2: Try parsing raw data as JSON
        if not data:
            try:
                raw_data = request.get_data(as_text=True)
                if raw_data and raw_data.strip():
                    data = json.loads(raw_data)
                    logger.info("Parsed JSON from raw request data")
            except (json.JSONDecodeError, UnicodeDecodeError) as e:
                logger.warning(f"Failed to parse raw data as JSON: {e}")
                logger.info(f"Raw data received: {raw_data[:200]}...")  # Log first 200 chars
        
        # Method 3: Try form data
        if not data:
            try:
                form_data = request.form.to_dict()
                if form_data:
                    data = form_data
                    logger.info("Parsed form data")
            except Exception as e:
                logger.warning(f"Failed to parse form data: {e}")
        
        # Method 4: Try query parameters
        if not data:
            try:
                query_data = request.args.to_dict()
                if query_data:
                    data = query_data
                    logger.info("Parsed query parameters")
            except Exception as e:
                logger.warning(f"Failed to parse query parameters: {e}")
        
        if not data:
            error_response = {
                'success': False,
                'error': 'No valid data found in request. Please ensure the request contains valid JSON, form data, or query parameters.',
                'content_type': request.content_type,
                'data_length': len(request.get_data()) if request.get_data() else 0,
                'raw_data_preview': request.get_data(as_text=True)[:200] if request.get_data() else None,
                'headers': dict(request.headers)
            }
            logger.error(f"Failed to parse request data: {error_response}")
            return jsonify(error_response), 400
        
        # Convert to internal format
        original_data = data.copy()
        data = convert_webhook_format(data)
        
        # Log the conversion for debugging
        logger.info(f"Converted webhook format: {original_data} -> {data}")
        
        # Validate request
        is_valid, message = validate_order_request(original_data)
        if not is_valid:
            return jsonify({
                'success': False,
                'error': message
            }), 400
        
        # Build command arguments
        args = build_command_args(data, original_data)
        
        # Execute order
        result = execute_order(args)
        
        # Return response
        response = {
            'success': result['success'],
            'timestamp': datetime.now().isoformat(),
            'webhook_data': original_data,
            'converted_data': data,
            'command': ' '.join(args)
        }
        
        if result['success']:
            response['message'] = 'Order executed successfully'
            response['output'] = result.get('stdout', '')
            return jsonify(response), 200
        else:
            response['error'] = result.get('error', 'Unknown error')
            response['stderr'] = result.get('stderr', '')
            return jsonify(response), 500
            
    except Exception as e:
        logger.error(f"Error processing order request: {str(e)}")
        return jsonify({
            'success': False,
            'error': f'Internal server error: {str(e)}',
            'timestamp': datetime.now().isoformat()
        }), 500

@app.route('/order_types', methods=['GET'])
def get_order_types():
    """Get available order types and their parameters"""
    order_info = {
        'open_long': {
            'description': 'Open long position (buy)',
            'parameters': ['price (optional, 0.0 for market)', 'quantity'],
            'example': {
                'symbol': 'MGCZ5',
                'action': 'open',
                'side': 'long',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50'
            }
        },
        'open_short': {
            'description': 'Open short position (sell)',
            'parameters': ['price (optional, 0.0 for market)', 'quantity'],
            'example': {
                'symbol': 'MGCZ5',
                'action': 'open',
                'side': 'short',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50'
            }
        },
        'close_long': {
            'description': 'Close long position (sell)',
            'parameters': ['price (optional, 0.0 for market)', 'quantity'],
            'example': {
                'symbol': 'MGCZ5',
                'action': 'close',
                'side': 'long',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50',
                'reason': 'rt_exit'
            }
        },
        'close_short': {
            'description': 'Close short position (buy)',
            'parameters': ['price (optional, 0.0 for market)', 'quantity'],
            'example': {
                'symbol': 'MGCZ5',
                'action': 'close',
                'side': 'short',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50',
                'reason': 'rt_exit'
            }
        },
        'modify_stop_long': {
            'description': 'Modify stop for long position',
            'parameters': ['stop_price'],
            'example': {
                'symbol': 'MGCZ5',
                'action': 'modify_stop',
                'side': 'long',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'stop_price': '2440.00'
            }
        },
        'modify_stop_short': {
            'description': 'Modify stop for short position',
            'parameters': ['stop_price'],
            'example': {
                'symbol': 'MGCZ5',
                'action': 'modify_stop',
                'side': 'short',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'stop_price': '2460.00'
            }
        }
    }
    
    return jsonify({
        'available_order_types': order_info,
        'timestamp': datetime.now().isoformat()
    })



@app.route('/webhook_examples', methods=['GET'])
def webhook_examples():
    """Get examples of the new webhook format"""
    examples = {
        'open_long': {
            'description': 'Open long position (buy)',
            'payload': {
                'symbol': 'MGCZ5',
                'action': 'open',
                'side': 'long',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50'
            }
        },
        'open_short': {
            'description': 'Open short position (sell)',
            'payload': {
                'symbol': 'MGCZ5',
                'action': 'open',
                'side': 'short',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50'
            }
        },
        'close_long': {
            'description': 'Close long position (sell)',
            'payload': {
                'symbol': 'MGCZ5',
                'action': 'close',
                'side': 'long',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50',
                'reason': 'rt_exit'
            }
        },
        'close_short': {
            'description': 'Close short position (buy)',
            'payload': {
                'symbol': 'MGCZ5',
                'action': 'close',
                'side': 'short',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'price': '2450.50',
                'reason': 'rt_exit'
            }
        },
        'modify_stop_long': {
            'description': 'Modify stop for long position',
            'payload': {
                'symbol': 'MGCZ5',
                'action': 'modify_stop',
                'side': 'long',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'stop_price': '2440.00'
            }
        },
        'modify_stop_short': {
            'description': 'Modify stop for short position',
            'payload': {
                'symbol': 'MGCZ5',
                'action': 'modify_stop',
                'side': 'short',
                'strategy': 'combo06.02',
                'timestamp': '2024-01-15T10:30:00Z',
                'stop_price': '2460.00'
            }
        }
    }
    
    return jsonify({
        'webhook_format_examples': examples,
        'supported_actions': ['open', 'close', 'modify_stop'],
        'supported_sides': ['long', 'short'],
        'timestamp': datetime.now().isoformat()
    })

@app.route('/', methods=['GET'])
def index():
    """Root endpoint with basic information"""
    return jsonify({
        'service': 'Rithmic Trading Webhook Server',
        'version': '1.0.0',
        'endpoints': {
            'GET /': 'This information',
            'GET /health': 'Health check',
            'GET /order_types': 'Get available order types and examples',
            'GET /webhook_examples': 'Get webhook format examples',
            'POST /order': 'Place an order (enhanced content type handling)'
        },
        'supported_content_types': [
            'application/json',
            'application/x-www-form-urlencoded',
            'text/plain',
            'raw JSON data'
        ],
        'timestamp': datetime.now().isoformat()
    })

if __name__ == '__main__':
    # Check if RithmicTradingApp exists
    if not os.path.exists(RITHMIC_APP_PATH):
        logger.warning(f"RithmicTradingApp not found at {RITHMIC_APP_PATH}")
        logger.info("Please update RITHMIC_APP_PATH in app.py to point to the correct location")
    
    # Run the Flask app
    app.run(host='0.0.0.0', port=5001, debug=False)

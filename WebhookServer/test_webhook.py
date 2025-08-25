#!/usr/bin/env python3
"""
Test script for the Rithmic Trading Webhook Server
Tests the new webhook format
"""

import requests
import json
import time
from datetime import datetime

# Server configuration
BASE_URL = "http://localhost:5001"

def test_health():
    """Test the health endpoint"""
    print("Testing health endpoint...")
    try:
        response = requests.get(f"{BASE_URL}/health")
        print(f"Status: {response.status_code}")
        print(f"Response: {response.json()}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_webhook_examples():
    """Test the webhook examples endpoint"""
    print("\nTesting webhook examples endpoint...")
    try:
        response = requests.get(f"{BASE_URL}/webhook_examples")
        print(f"Status: {response.status_code}")
        data = response.json()
        print(f"Available examples: {list(data['webhook_format_examples'].keys())}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_order(order_data, description, endpoint="/order", content_type="json"):
    """Test placing an order"""
    print(f"\nTesting {description}...")
    print(f"Endpoint: {endpoint}")
    print(f"Content-Type: {content_type}")
    print(f"Order data: {json.dumps(order_data, indent=2)}")
    
    try:
        if content_type == "json":
            response = requests.post(
                f"{BASE_URL}{endpoint}",
                json=order_data,
                headers={"Content-Type": "application/json"}
            )
        elif content_type == "form":
            response = requests.post(
                f"{BASE_URL}{endpoint}",
                data=order_data,
                headers={"Content-Type": "application/x-www-form-urlencoded"}
            )
        elif content_type == "raw":
            response = requests.post(
                f"{BASE_URL}{endpoint}",
                data=json.dumps(order_data),
                headers={"Content-Type": "text/plain"}
            )
        else:
            print(f"Unknown content type: {content_type}")
            return False
        
        print(f"Status: {response.status_code}")
        result = response.json()
        print(f"Success: {result.get('success', False)}")
        
        if result.get('success'):
            print(f"Message: {result.get('message', 'No message')}")
            print(f"Output: {result.get('output', 'No output')}")
            if 'converted_data' in result:
                print(f"Converted data: {result['converted_data']}")
        else:
            print(f"Error: {result.get('error', 'Unknown error')}")
            print(f"Stderr: {result.get('stderr', 'No stderr')}")
            if 'content_type' in result:
                print(f"Received Content-Type: {result['content_type']}")
            if 'data_length' in result:
                print(f"Data length: {result['data_length']}")
        
        return result.get('success', False)
        
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    """Main test function"""
    print("Rithmic Trading Webhook Server Test - New Format")
    print("=" * 50)
    
    # Test basic endpoints
    if not test_health():
        print("Health check failed. Is the server running?")
        return
    
    if not test_webhook_examples():
        print("Webhook examples endpoint failed.")
        return
    
    # Current timestamp for testing
    current_time = datetime.now().isoformat()
    
    # Test cases using the new webhook format
    test_cases = [
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "open",
        #         "side": "long",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "2450.50"
        #     },
        #     "description": "OPEN LONG / BUY - Limit Order"
        # },
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "open",
        #         "side": "short",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "3419.0"
        #     },
        #     "description": "OPEN SHORT / SELL - Limit Order"
        # },
        {
            "data": {
                "symbol": "MGCZ5",
                "action": "open",
                "side": "long",
                "strategy": "combo06.02",
                "timestamp": current_time,
                "price": "0"
            },
            "description": "OPEN LONG / BUY - Market Order (price=0)"
        },
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "close",
        #         "side": "long",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "3420",
        #         "reason": "rt_exit"
        #     },
        #     "description": "REAL TIME EXIT (INTRABAR) - Close Long"
        # },
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "close",
        #         "side": "short",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "2450.50",
        #         "reason": "rt_exit"
        #     },
        #     "description": "REAL TIME EXIT (INTRABAR) - Close Short"
        # },
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "close",
        #         "side": "long",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "2440.00",
        #         "reason": "end_of_bar"
        #     },
        #     "description": "EXIT END OF BAR - Close Long with specific price"
        # },
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "modify_stop",
        #         "side": "long",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "stop_price": "3419.6"
        #     },
        #     "description": "UPDATE STOP - Modify stop for long position"
        # },
        # {
        #     "data": {
        #         "symbol": "MGCZ5",
        #         "action": "modify_stop",
        #         "side": "short",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "stop_price": "2460.00"
        #     },
        #     "description": "UPDATE STOP - Modify stop for short position"
        # },
        # {
        #     "data": {
        #         "symbol": "GCZ5",
        #         "action": "open",
        #         "side": "long",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "1950.00"
        #     },
        #     "description": "Gold Futures - Open Long"
        # },
        # {
        #     "data": {
        #         "symbol": "ESZ5",
        #         "action": "open",
        #         "side": "short",
        #         "strategy": "combo06.02",
        #         "timestamp": current_time,
        #         "price": "0"
        #     },
        #     "description": "E-mini S&P 500 - Open Short Market Order"
        # }
    ]
    
    print("\nTesting various webhook order types...")
    successful_tests = 0
    total_tests = len(test_cases)
    
    for i, test_case in enumerate(test_cases, 1):
        print(f"\n--- Test {i}/{total_tests} ---")
        success = test_order(test_case["data"], test_case["description"])
        
        if success:
            successful_tests += 1
        
        # Wait a bit between tests to avoid overwhelming the server
        if i < len(test_cases):
            print("Waiting 1 second before next test...")
            time.sleep(1)
    
    # Test different content types with the enhanced /order endpoint
    print(f"\n{'='*50}")
    print("Testing different content types with enhanced /order endpoint...")
    
    content_type_tests = [
        {
            "data": {
                "symbol": "MGCZ5",
                "action": "modify_stop",
                "side": "long",
                "strategy": "combo06.02",
                "timestamp": current_time,
                "stop_price": "3419.6"
            },
            "description": "Test JSON Content-Type",
            "endpoint": "/order",
            "content_type": "json"
        },
        {
            "data": {
                "symbol": "MGCZ5",
                "action": "modify_stop",
                "side": "long",
                "strategy": "combo06.02",
                "timestamp": current_time,
                "stop_price": "3419.6"
            },
            "description": "Test Form Data Content-Type",
            "endpoint": "/order",
            "content_type": "form"
        },
        {
            "data": {
                "symbol": "MGCZ5",
                "action": "modify_stop",
                "side": "long",
                "strategy": "combo06.02",
                "timestamp": current_time,
                "stop_price": "3419.6"
            },
            "description": "Test Raw Text Content-Type",
            "endpoint": "/order",
            "content_type": "raw"
        }
    ]
    
    for i, test_case in enumerate(content_type_tests, 1):
        print(f"\n--- Content Type Test {i}/{len(content_type_tests)} ---")
        success = test_order(
            test_case["data"], 
            test_case["description"],
            test_case["endpoint"],
            test_case["content_type"]
        )
        
        if success:
            successful_tests += 1
        
        if i < len(content_type_tests):
            print("Waiting 1 second before next test...")
            time.sleep(1)
    
    print(f"\n{'='*50}")
    print(f"Test Results: {successful_tests}/{total_tests} tests passed")
    print(f"Success Rate: {(successful_tests/total_tests)*100:.1f}%")
    print("Test completed!")

if __name__ == "__main__":
    main()

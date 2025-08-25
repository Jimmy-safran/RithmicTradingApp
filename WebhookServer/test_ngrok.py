#!/usr/bin/env python3
"""
Test script for the Rithmic Trading Webhook Server via ngrok
"""

import requests
import json
import time
import sys

def test_ngrok_webhook(ngrok_url):
    """Test the webhook server via ngrok URL"""
    
    # Remove trailing slash if present
    if ngrok_url.endswith('/'):
        ngrok_url = ngrok_url[:-1]
    
    print(f"Testing webhook server at: {ngrok_url}")
    print("=" * 60)
    
    # Test health endpoint
    print("1. Testing health endpoint...")
    try:
        response = requests.get(f"{ngrok_url}/health", timeout=10)
        print(f"   Status: {response.status_code}")
        print(f"   Response: {response.json()}")
        if response.status_code == 200:
            print("   ✓ Health check passed")
        else:
            print("   ✗ Health check failed")
            return False
    except Exception as e:
        print(f"   ✗ Health check failed: {e}")
        return False
    
    # Test order types endpoint
    print("\n2. Testing order types endpoint...")
    try:
        response = requests.get(f"{ngrok_url}/order_types", timeout=10)
        print(f"   Status: {response.status_code}")
        if response.status_code == 200:
            data = response.json()
            order_types = list(data['available_order_types'].keys())
            print(f"   Available order types: {order_types}")
            print("   ✓ Order types endpoint working")
        else:
            print("   ✗ Order types endpoint failed")
            return False
    except Exception as e:
        print(f"   ✗ Order types endpoint failed: {e}")
        return False
    
    # Test order placement
    print("\n3. Testing order placement...")
    test_order = {
        "exchange": "COMEX",
        "ticker": "MGCZ5",
        "side": "B",
        "order_type": "open_long",
        "price": 0.0,
        "quantity": 1
    }
    
    print(f"   Order data: {json.dumps(test_order, indent=6)}")
    
    try:
        response = requests.post(
            f"{ngrok_url}/order",
            json=test_order,
            headers={"Content-Type": "application/json"},
            timeout=60  # Longer timeout for order execution
        )
        
        print(f"   Status: {response.status_code}")
        result = response.json()
        print(f"   Success: {result.get('success', False)}")
        
        if result.get('success'):
            print(f"   Message: {result.get('message', 'No message')}")
            print("   ✓ Order executed successfully")
        else:
            print(f"   Error: {result.get('error', 'Unknown error')}")
            print(f"   Stderr: {result.get('stderr', 'No stderr')}")
            print("   ✗ Order execution failed")
        
        return result.get('success', False)
        
    except Exception as e:
        print(f"   ✗ Order placement failed: {e}")
        return False

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python test_ngrok.py <ngrok_url>")
        print("Example: python test_ngrok.py https://abc123.ngrok.io")
        return
    
    ngrok_url = sys.argv[1]
    
    print("Rithmic Trading Webhook Server - ngrok Test")
    print("=" * 60)
    print(f"Testing URL: {ngrok_url}")
    print()
    
    success = test_ngrok_webhook(ngrok_url)
    
    print("\n" + "=" * 60)
    if success:
        print("✓ All tests passed! Your webhook server is working via ngrok.")
    else:
        print("✗ Some tests failed. Check the output above for details.")
    
    print("\nYou can now use this ngrok URL to send webhook requests from external systems.")
    print("Example curl command:")
    print(f'curl -X POST {ngrok_url}/order \\')
    print('  -H "Content-Type: application/json" \\')
    print('  -d \'{"exchange": "COMEX", "ticker": "MGCZ5", "side": "B", "order_type": "open_long", "price": 0.0, "quantity": 1}\'')

if __name__ == "__main__":
    main()

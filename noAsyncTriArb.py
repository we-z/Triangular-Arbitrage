import requests
import config
import time

# Alpaca Constants
API_KEY = config.API_KEY
SECRET_KEY = config.SECRET_KEY

HEADERS = {'APCA-API-KEY-ID': API_KEY, 'APCA-API-SECRET-KEY': SECRET_KEY}

ALPACA_BASE_URL = 'https://paper-api.alpaca.markets'
DATA_URL = 'https://data.alpaca.markets'

min_arb_percent = 0.5

def get_quote(symbol: str):
    '''
    Get quote data from Alpaca API
    '''

    try:
        # make the request
        quote = requests.get(
            '{0}/v1beta3/crypto/us/latest/trades?symbols={1}'
             .format(DATA_URL, symbol), headers=HEADERS
        )
        return quote.json()['trades'][symbol]['p']

    except Exception as e:
        print("There was an issue getting trade quote from Alpaca: {0}".format(e))
        return False

def post_Alpaca_order(symbol, qty, side):
    '''
    Post an order to Alpaca
    '''
    try:
        order = requests.post(
                '{0}/v2/orders'.format(ALPACA_BASE_URL), headers=HEADERS, json={
                'symbol': symbol,
                'qty': qty,
                'side': side,
                'type': 'market',
                'time_in_force': 'gtc',
            })

        if order.status_code != 200:
            print(order.json())

    except Exception as e:
        print("There was an issue posting order to Alpaca: {0}".format(e))
        return False

def liquidate():
    requests.delete('{0}/v2/positions'.format(ALPACA_BASE_URL), headers=HEADERS)

while True:
    '''
    Check to see if an arbitrage condition exists
    '''

    ETH = get_quote("ETH/USD") 
    BTC = get_quote("BTC/USD") 
    ETHBTC = get_quote("ETH/BTC")
    DIV = ETH / BTC 
    BUY_ETH = 1000 / ETH
    BUY_BTC = 1000 / BTC 
   
    # when BTCUSD is cheaper
    if DIV > ETHBTC * (1 + min_arb_percent/100):
        post_Alpaca_order("BTCUSD", BUY_BTC, "buy")
        post_Alpaca_order("ETH/BTC", (BUY_ETH * 0.95),  "buy")
        liquidate()
        print("Done (BTC -> ETH)") 

    # when ETHUSD is cheaper
    elif DIV < ETHBTC * (1 - min_arb_percent/100):
        post_Alpaca_order("ETHUSD", BUY_ETH, "buy")
        post_Alpaca_order("ETH/BTC", (BUY_ETH * 0.9979), "sell")
        liquidate()
        print("Done (ETH -> BTC)")

    time.sleep(1)

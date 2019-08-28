#include "wiegand.h"

static uint32_t _cardTempHigh = 0;
static uint32_t _cardTemp = 0;
static uint32_t _lastWiegand = 0;
static uint32_t _sysTick = 0;
static uint32_t _code = 0;
static uint8_t _bitCount = 0;
static uint8_t _wiegandType = 0;

static int WIEGAND_DoWiegandConversion(void);

uint32_t WIEGAND_getCode(void)
{
    return _code;
}

int WIEGAND_getWiegandType(void)
{
    return _wiegandType;
}

int WIEGAND_available(void)
{
    return WIEGAND_DoWiegandConversion();
}

void WIEGAND_init(void)
{
    _lastWiegand = 0;
    _cardTempHigh = 0;
    _cardTemp = 0;
    _code = 0;
    _wiegandType = 0;
    _bitCount = 0;
    _sysTick = millis();
}

void WIEGAND_ReadD0(void)
{
    _bitCount++;                // Increament bit count for Interrupt connected to D0

    if (_bitCount > 31)         // If bit count more than 31, process high bits
    {
        _cardTempHigh |= ((0x80000000 & _cardTemp) >> 31);  //  shift value to high bits
        _cardTempHigh <<= 1;
        _cardTemp <<= 1;
    }
    else
    {
        _cardTemp <<= 1;        // D0 represent binary 0, so just left shift card data
    }

    _lastWiegand = _sysTick;    // Keep track of last wiegand bit received
}

void WIEGAND_ReadD1(void)
{
    _bitCount ++;               // Increment bit count for Interrupt connected to D1

    if (_bitCount > 31)         // If bit count more than 31, process high bits
    {
        _cardTempHigh |= ((0x80000000 & _cardTemp) >> 31);  // shift value to high bits
        _cardTempHigh <<= 1;
        _cardTemp |= 1;
        _cardTemp <<= 1;
    }
    else
    {
        _cardTemp |= 1;         // D1 represent binary 1, so OR card data with 1 then
        _cardTemp <<= 1;        // left shift card data
    }

    _lastWiegand = _sysTick;    // Keep track of last wiegand bit received
}

static uint32_t WIEGAND_GetCardId(uint32_t* codehigh, uint32_t* codelow, uint8_t bitlength)
{
    uint32_t cardID = 0;

    if (bitlength == 26)                            // EM tag
    {
        cardID = (*codelow & 0x1FFFFFE) >> 1;
    }

    if (bitlength == 34)                            // Mifare
    {
        *codehigh = *codehigh & 0x03;               // only need the 2 LSB of the codehigh
        *codehigh <<= 30;                           // shift 2 LSB to MSB
        *codelow >>= 1;
        cardID = *codehigh | *codelow;
    }

    return cardID;
}

static int WIEGAND_DoWiegandConversion(void)
{
    uint32_t cardID = 0;

    _sysTick = millis();

    if ((_sysTick - _lastWiegand) > 25) // if no more signal coming through after 25ms
    {
        if ((_bitCount == 26) || (_bitCount == 34))
        {
            _cardTemp >>= 1;            // shift right 1 bit to get back the real value - interrupt done 1 left shift in advance

            if (_bitCount > 32)         // bit count more than 32 bits, shift high bits right to make adjustment
            {
                _cardTempHigh >>= 1;
            }

            if ((_bitCount == 26) || (_bitCount == 34)) // wiegand 26 or wiegand 34
            {
                cardID = WIEGAND_GetCardId(&_cardTempHigh, &_cardTemp, _bitCount);
                _wiegandType = _bitCount;
                _bitCount = 0;
                _cardTemp = 0;
                _cardTempHigh = 0;
                _code = cardID;
                return 1;
            }
        }
        else
        {
            _lastWiegand = _sysTick;
            _bitCount = 0;
            _cardTemp = 0;
            _cardTempHigh = 0;
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

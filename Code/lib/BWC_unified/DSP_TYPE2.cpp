#include "DSP_TYPE2.h"

void DSP_6_TYPE2::setup(int dsp_td_pin, int dsp_clk_pin, int dsp_ld_pin, int dsp_audio_pin)
{
  _DSP_TD_PIN = dsp_td_pin;
  _DSP_CLK_PIN = dsp_clk_pin;
  _DSP_LD_PIN = dsp_ld_pin;
  _DSP_AUDIO_PIN = dsp_audio_pin;

  pinMode(_DSP_LD_PIN, OUTPUT);
  pinMode(_DSP_TD_PIN, INPUT);
  pinMode(_DSP_CLK_PIN, OUTPUT);
  pinMode(_DSP_AUDIO_PIN, OUTPUT);
  digitalWrite(_DSP_LD_PIN, HIGH);   //idle high
  digitalWrite(_DSP_CLK_PIN, HIGH); //shift on falling, latch on rising
  digitalWrite(_DSP_AUDIO_PIN, LOW);
}

void DSP_6_TYPE2::stop()
{
    noTone(_DSP_AUDIO_PIN);
}

uint8_t DSP_6_TYPE2::charTo7SegmCode(char c)
{
    for (unsigned int index = 0; index < sizeof(CHARS); index++) {
        if (c == CHARS[index])
        {
            return CHARCODES[index];
        }
    }
    return 0x00;  //no match, return 'space'
}

void DSP_6_TYPE2::_sendBitsToDSP(uint32_t outBits, int bitsToSend)
{
  for (int i = 0; i < bitsToSend; i++) {
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(_DSP_LD_PIN, outBits & (1 << i));
    delayMicroseconds(CLKPW-5);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
  }
}

uint16_t DSP_6_TYPE2::_receiveBitsFromDSP()
{
    return 0;
}

void DSP_6_TYPE2::clearpayload()
{
    for(unsigned int i = 0; i < sizeof(_payload); i++)
    {
        _payload[i] = 0;
    }
}

Buttons DSP_6_TYPE2::getPressedButton()
{
    if(millis() - _dsp_getbutton_last_time < 20) return _old_button;

    uint16_t newButtonCode = 0;
    Buttons newButton;

    _dsp_getbutton_last_time = millis();
    //startbit
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    //clock in 8 data bits
    for(int i = 0; i < 8; i++){
        digitalWrite(_DSP_CLK_PIN, LOW);
        delayMicroseconds(CLKPW);
        digitalWrite(_DSP_CLK_PIN, HIGH);
        newButtonCode |= digitalRead(_DSP_TD_PIN)<<i;
        delayMicroseconds(CLKPW);
    }
    //stop bit
    digitalWrite(_DSP_CLK_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);

    newButton = buttonCodeToIndex(newButtonCode);
    //work around for glitches. Only register change after two consecutive and equal values
    if(newButton == _prev_button){
        _old_button = newButton;
    } else {
        _prev_button = newButton;
    }
    return (_old_button);
}

void DSP_6_TYPE2::setStates(const sStates& toDspStates)
{
    _to_dsp_states = toDspStates;
    // _from_dsp_states = toDspStates;
    
    if(toDspStates.text.length())
    {
        _payload[getDGT1_IDX()] = charTo7SegmCode(toDspStates.text[0]);
        toDspStates.text.length() > 1 ? _payload[getDGT2_IDX()] = charTo7SegmCode(toDspStates.text[1]) : _payload[getDGT2_IDX()] = 1;
        toDspStates.text.length() > 2 ? _payload[getDGT3_IDX()] = charTo7SegmCode(toDspStates.text[2]) : _payload[getDGT3_IDX()] = 1;
    }
    else
    {
        _payload[getDGT1_IDX()] = charTo7SegmCode(toDspStates.char1);
        _payload[getDGT2_IDX()] = charTo7SegmCode(toDspStates.char2);
        _payload[getDGT3_IDX()] = charTo7SegmCode(toDspStates.char3);
    }

    if(toDspStates.power)
    {
        _payload[getLCK_IDX()] &= ~(1<<getLCK_BIT());
        _payload[getLCK_IDX()] |= toDspStates.locked << getLCK_BIT();

        _payload[getTMRBTNLED_IDX()] &= ~(1<<getTMRBTNLED_BIT());
        _payload[getTMRBTNLED_IDX()] |= toDspStates.timerbuttonled << getTMRBTNLED_BIT();
//
        _payload[getTMR1_IDX()] &= ~(1<<getTMR1_BIT());
        _payload[getTMR1_IDX()] |= toDspStates.timerled1 << getTMR1_BIT();

        _payload[getTMR2_IDX()] &= ~(1<<getTMR2_BIT());
        _payload[getTMR2_IDX()] |= toDspStates.timerled2 << getTMR2_BIT();
//
        _payload[getREDHTR_IDX()] &= ~(1<<getREDHTR_BIT());
        _payload[getREDHTR_IDX()] |= toDspStates.heatred << getREDHTR_BIT();

        _payload[getGRNHTR_IDX()] &= ~(1<<getGRNHTR_BIT());
        _payload[getGRNHTR_IDX()] |= toDspStates.heatgrn << getGRNHTR_BIT();

        _payload[getAIR_IDX()] &= ~(1<<getAIR_BIT());
        _payload[getAIR_IDX()] |= toDspStates.bubbles << getAIR_BIT();

        _payload[getFLT_IDX()] &= ~(1<<getFLT_BIT());
        _payload[getFLT_IDX()] |= toDspStates.pump << getFLT_BIT();

        _payload[getC_IDX()] &= ~(1<<getC_BIT());
        _payload[getC_IDX()] |= toDspStates.unit << getC_BIT();

        _payload[getF_IDX()] &= ~(1<<getF_BIT());
        _payload[getF_IDX()] |= !toDspStates.unit << getF_BIT();

        _payload[getPWR_IDX()] &= ~(1<<getPWR_BIT());
        _payload[getPWR_IDX()] |= toDspStates.power << getPWR_BIT();

        _payload[getHJT_IDX()] &= ~(1<<getHJT_BIT());
        _payload[getHJT_IDX()] |= toDspStates.jets << getHJT_BIT();
    }
    else
    {
        clearpayload();
    }
    if(toDspStates.audiofrequency)
        tone(getAUDIO(), toDspStates.audiofrequency);
    else
        noTone(getAUDIO());

    uploadPayload(toDspStates.brightness);
}

/*Send payload[] to display*/
void DSP_6_TYPE2::uploadPayload(uint8_t brightness)
{
   //refresh display with ~10Hz
    if(millis() -_dsp_last_refreshtime < 100) return;
    _dsp_last_refreshtime = millis();
        uint8_t enableLED = 0;
    if(brightness > 0)
    {
        enableLED = DSP_DIM_ON;
        brightness -= 1;
    }
    digitalWrite(_DSP_LD_PIN, LOW); //start of packet
    delayMicroseconds(CLKPW);
    _sendBitsToDSP(CMD1, 8);
    //end of packet: clock low, make sure LD is low before rising clock then LD
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);

    digitalWrite(_DSP_LD_PIN, LOW); //start of packet
    delayMicroseconds(CLKPW);
    _sendBitsToDSP(CMD2, 8);
    for(unsigned int i=0; i<sizeof(_payload); i++){
        _sendBitsToDSP(_payload[i], 8);
    }
    //end of packet: clock low, make sure LD is low before rising clock then LD
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);

    digitalWrite(_DSP_LD_PIN, LOW); //start of packet
    delayMicroseconds(CLKPW);
    _sendBitsToDSP((CMD3 & 0xF8)|enableLED|brightness, 8);

    //end of packet: clock low, make sure LD is low before raising clock then LD
    digitalWrite(_DSP_CLK_PIN, LOW);
    digitalWrite(_DSP_LD_PIN, LOW);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_CLK_PIN, HIGH);
    delayMicroseconds(CLKPW);
    digitalWrite(_DSP_LD_PIN, HIGH);
    delayMicroseconds(CLKPW);
}


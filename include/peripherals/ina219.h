#pragma once

template<uint8_t ADDRESS>
class INA219 : public i2c::Device<ADDRESS> {
	using i2c::Device<ADDRESS>::read8;
	using i2c::Device<ADDRESS>::readRegister8;
	using i2c::Device<ADDRESS>::readRegister16;
	using i2c::Device<ADDRESS>::write8;
	using i2c::Device<ADDRESS>::writeRegister8;
	using i2c::Device<ADDRESS>::writeRegister16;
public:
/*    enum class Gain : uint8_t {
	    X1 = 1,
	    X2 = 2,
	    X4 = 4,
	    X8 = 8,
    };
	
	static const Gain X1 = Gain::X1;
	static const Gain X2 = Gain::X2;
	static const Gain X4 = Gain::X4;
	static const Gain X8 = Gain::X8; */
	
	void calibrate() {
		/* 
		VBUS_MAX = 16V
		VSHUNT_MAX = 0.04 (Gain 1, 40mV)
		RSHUNT = 0.01
		MaxPossibleI = VSHUNT_MAX / RSHUNT
		MaxPossibleI = 4A
		MaxExpectedI = 4A
		MinimumLSB = MaxExpectedI/32768
		MinimumLSB = 0.000122
		MaximumLSB = MaxExpectedI/4096
		MaximumLSB = 0.0009765
	    CurrentLSB = 0.000200 (roundish number close to MinimumLSB) (0.2mA per bit)
		Calibration = trunc ( 0.04096 / (CurrentLSB * RSHUNT))
		Calibration = 20480
		PowerLSB = 20 * CurrentLSB
		PowerLSB = 0.004 (4mW per bit)
		*/
		writeRegister16(CALIBRATION, 20480);
   	    writeRegister16(CONFIG, 0b00000100110011111);
		   
	}
	
/*	void calibrate(double rshunt, Gain gain) {
		LOG(F("INA219 address ") << ADDRESS << F(" rshunt") << rshunt << F(" and gain ") << static_cast<uint8_t>(gain) << endl);
		double max_i = 0.040 * static_cast<uint8_t>(gain) / rshunt;
		uint32_t minLSB = (max_i / 32767) * 1000000;
		// we now have minLSB in uA, find first greatest
		uint32_t lsb = 10;
		while (minLSB > lsb)
		    lsb *= 10;
		LOG(F("INA219 lsb set to ") << lsb << " uA" << endl);
		uint16_t cal = trunc(0.04096 / (static_cast<double>(lsb) / 1000000 * rshunt));
		LOG(F("INA219 calibration ") << cal << endl);
		writeRegister16(CALIBRATION, cal);
		switch (gain) {
		case Gain::X1:
			writeRegister16(CONFIG, 0b00000100110011111);
			break;
		case Gain::X2:
			writeRegister16(CONFIG, 0b00001100110011111);
			break;
		case Gain::X4:
			writeRegister16(CONFIG, 0b00010100110011111);
			break;
		case Gain::X8:
			writeRegister16(CONFIG, 0b00011100110011111);
			break;
		}
	} */

    /** Returns the voltage in mV. 
	 */
	uint16_t voltage() {
		return (readRegister16(BUS_VOLTAGE) >> 3) * 4;
	}
	
	uint16_t shuntVoltage() {
		return (readRegister16(SHUNT_VOLTAGE));
	}
	
	uint16_t power() {
		return readRegister16(POWER);
	}
	
	/** Returns the current in mA. 
	 */
	uint16_t current() {
		return readRegister16(CURRENT) / 5;
	}

private:

	const uint8_t CONFIG = 0x0;
	const uint8_t SHUNT_VOLTAGE = 0x01;
	const uint8_t BUS_VOLTAGE = 0x02;
	const uint8_t POWER = 0x03;
	const uint8_t CURRENT = 0x04;
	const uint8_t CALIBRATION = 0x05;
};


		/*
264
265   // Calibration which uses the highest precision for
266   // current measurement (0.1mA), at the expense of
267   // only supporting 16V at 400mA max.
268

269   // VBUS_MAX = 16V
270   // VSHUNT_MAX = 0.04          (Assumes Gain 1, 40mV)
271   // RSHUNT = 0.1               (Resistor value in ohms)
272
273   // 1. Determine max possible current
274   // MaxPossible_I = VSHUNT_MAX / RSHUNT
275   // MaxPossible_I = 0.4A
276

277   // 2. Determine max expected current
278   // MaxExpected_I = 0.4A
279
280   // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
281   // MinimumLSB = MaxExpected_I/32767
282   // MinimumLSB = 0.0000122              (12uA per bit)
283   // MaximumLSB = MaxExpected_I/4096
284   // MaximumLSB = 0.0000977              (98uA per bit)
285
286   // 4. Choose an LSB between the min and max values
287   //    (Preferrably a roundish number close to MinLSB)
288   // CurrentLSB = 0.00005 (50uA per bit)
289
290   // 5. Compute the calibration register
291   // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
292   // Cal = 8192 (0x2000)
293

294   ina219_calValue = 8192;
295

296   // 6. Calculate the power LSB
297   // PowerLSB = 20 * CurrentLSB
298   // PowerLSB = 0.001 (1mW per bit)
299
300   // 7. Compute the maximum current and shunt voltage values before overflow
301   //
302   // Max_Current = Current_LSB * 32767
303   // Max_Current = 1.63835A before overflow
304   //
305   // If Max_Current > Max_Possible_I then
306   //    Max_Current_Before_Overflow = MaxPossible_I
307   // Else
308   //    Max_Current_Before_Overflow = Max_Current
309   // End If
310   //
311   // Max_Current_Before_Overflow = MaxPossible_I
312   // Max_Current_Before_Overflow = 0.4
313   //
314   // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
315   // Max_ShuntVoltage = 0.04V
316   //
317   // If Max_ShuntVoltage >= VSHUNT_MAX
318   //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
319   // Else
320   //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
321   // End If
322   //
323   // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
324   // Max_ShuntVoltage_Before_Overflow = 0.04V
325
326   // 8. Compute the Maximum Power
327   // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
328   // MaximumPower = 0.4 * 16V
329   // MaximumPower = 6.4W
330
331   // Set multipliers to convert raw current/power values
332   ina219_currentDivider_mA = 20;  // Current LSB = 50uA per bit (1000/50 = 20)
333   ina219_powerDivider_mW = 1;     // Power LSB = 1mW per bit
334

335   // Set Calibration register to 'Cal' calculated above
336   wireWriteRegister(INA219_REG_CALIBRATION, ina219_calValue);
337
338   // Set Config register to take into account the settings above
339   uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
340                     INA219_CONFIG_GAIN_1_40MV |
341                     INA219_CONFIG_BADCRES_12BIT |
342                     INA219_CONFIG_SADCRES_12BIT_1S_532US |
343                     INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
344   wireWriteRegister(INA219_REG_CONFIG, config);
345 } */
			
		



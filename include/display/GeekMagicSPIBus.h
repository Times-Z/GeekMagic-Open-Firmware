#ifndef HELLO_CUBIC_SPI_BUS_H
#define HELLO_CUBIC_SPI_BUS_H

#include <Arduino_GFX_Library.h>
#include <SPI.h>

/**
 * @brief Determines whether the Chip Select (CS) line should remain asserted (active low) between SPI transactions
 */
static constexpr bool LCD_KEEP_CS_ASSERTED = true;

/**
 * @class GeekMagicSPIBus
 * @brief Custom SPI bus class for GeekMagicSPIBus display
 *
 * This class extends Arduino_DataBus to provide SPI communication with custom Chip Select (CS) handling for the
 * GeekMagicSPIBus display
 */
class GeekMagicSPIBus : public Arduino_DataBus {
   public:
    GeekMagicSPIBus(int8_t dataCmdPin, int8_t csPin, bool csActiveHigh, int32_t defaultSpeed, int8_t defaultDataMode);

    bool begin(int32_t speed = GFX_NOT_DEFINED, int8_t dataMode = GFX_NOT_DEFINED) override;
    void beginWrite() override;
    void endWrite() override;

    virtual ~GeekMagicSPIBus() {}

    void writeCommand(uint8_t c) override { _spi.writeCommand(c); }
    void writeCommand16(uint16_t c) override { _spi.writeCommand16(c); }
    void writeCommandBytes(uint8_t* data, uint32_t len) override { _spi.writeCommandBytes(data, len); }
    void write(uint8_t d) override { _spi.write(d); }
    void write16(uint16_t d) override { _spi.write16(d); }
    void writeRepeat(uint16_t p, uint32_t len) override { _spi.writeRepeat(p, len); }
    void writeBytes(uint8_t* data, uint32_t len) override { _spi.writeBytes(data, len); }
    void writePixels(uint16_t* data, uint32_t len) override { _spi.writePixels(data, len); }

   private:
    Arduino_HWSPI _spi;
    int8_t _cs;
    bool _csActiveHigh;
    int32_t _defaultSpeed;
    int8_t _defaultDataMode;
};

#endif

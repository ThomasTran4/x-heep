#include "x_spi.h"

//new version using sdk to read flash

#include <stdlib.h>

#include "x-heep.h"
#include "spi_sdk.h"

#include "fast_intr_ctrl.h"
#include "csr.h"
#include "csr_registers.h"
#include "tables.h"
#include "r_state.h"

// =========================== VARS & DEFS ==================================
// Flash w25q128jw SPI commands
#define FC_WE      0x06 /** Write Enable */
#define FC_RD      0x03 /** Read Data */ 
#define FC_PP      0x02 /** Page Program */
#define FC_SE      0x20 /** Sector Erase 4kb */
#define FC_RSR1    0x05 /** Read Status Register 1 */

#define START_ADDRESS 0
#define READ_LEN 4                      // Amount words to read
#define FLASH_MAX_FREQ (133*1000*1000)  // Device max spi frequency
#define FIC_FLASH_MEIE 21               // SPI Flash fast interrupt bit enable
#define CSR_INTR_EN    0x08             // CPU Global interrupt enable
#define PAGE_LEN       256              // Length bytes of a page

#define WAD_LENGTH  0x800000

spi_t spi_flash;
uint32_t next_loc = WAD_START_ADDRESS + WAD_LENGTH + 1024; 

#define SECT_ADDRESS   (START_ADDRESS & 0xfffff000) // Start sector address

uint8_t sect_data[SECT_LEN];

uint32_t flash_original_1024B[PAGE_LEN] = {
    0x76543211, 0xfedcba99, 0x579a6f91, 0x657d5bef, 0x758ee420, 0x01234568, 0xfedbca97, 0x89abde00,
    0x76543212, 0xfedcba9a, 0x579a6f92, 0x657d5bf0, 0x758ee421, 0x01234569, 0xfedbca98, 0x89abde01,
    0x76543213, 0xfedcba9b, 0x579a6f93, 0x657d5bf1, 0x758ee422, 0x0123456a, 0xfedbca99, 0x89abde02,
    0x76543214, 0xfedcba9c, 0x579a6f94, 0x657d5bf2, 0x758ee423, 0x0123456b, 0xfedbca9a, 0x89abde03,
    0x76543215, 0xfedcba9d, 0x579a6f95, 0x657d5bf3, 0x758ee424, 0x0123456c, 0xfedbca9b, 0x89abde04,
    0x76543216, 0xfedcba9e, 0x579a6f96, 0x657d5bf4, 0x758ee425, 0x0123456d, 0xfedbca9c, 0x89abde05,
    0x76543217, 0xfedcba9f, 0x579a6f97, 0x657d5bf5, 0x758ee426, 0x0123456e, 0xfedbca9d, 0x89abde06,
    0x76543218, 0xfedcbaa0, 0x579a6f98, 0x657d5bf6, 0x758ee427, 0x0123456f, 0xfedbca9e, 0x89abde07,
    0x76543219, 0xfedcbaa1, 0x579a6f99, 0x657d5bf7, 0x758ee428, 0x01234570, 0xfedbca9f, 0x89abde08,
    0x7654321a, 0xfedcbaa2, 0x579a6f9a, 0x657d5bf8, 0x758ee429, 0x01234571, 0xfedbcaa0, 0x89abde09,
    0x7654321b, 0xfedcbaa3, 0x579a6f9b, 0x657d5bf9, 0x758ee42a, 0x01234572, 0xfedbcaa1, 0x89abde0a,
    0x7654321c, 0xfedcbaa4, 0x579a6f9c, 0x657d5bfa, 0x758ee42b, 0x01234573, 0xfedbcaa2, 0x89abde0b,
    0x7654321d, 0xfedcbaa5, 0x579a6f9d, 0x657d5bfb, 0x758ee42c, 0x01234574, 0xfedbcaa3, 0x89abde0c,
    0x7654321e, 0xfedcbaa6, 0x579a6f9e, 0x657d5bfc, 0x758ee42d, 0x01234575, 0xfedbcaa4, 0x89abde0d,
    0x7654321f, 0xfedcbaa7, 0x579a6f9f, 0x657d5bfd, 0x758ee42e, 0x01234576, 0xfedbcaa5, 0x89abde0e,
    0x76543220, 0xfedcbaa8, 0x579a6fa0, 0x657d5bfe, 0x758ee42f, 0x01234577, 0xfedbcaa6, 0x89abde0f,
    0x76543221, 0xfedcbaa9, 0x579a6fa1, 0x657d5bff, 0x758ee430, 0x01234578, 0xfadbcaa7, 0x89abde10,
    0x76543222, 0xfedcbaaa, 0x579a6fa2, 0x657d5c00, 0x758ee431, 0x01234579, 0xfadbcaa8, 0x89abde11,
    0x76543223, 0xfedcbaab, 0x579a6fa3, 0x657d5c01, 0x758ee432, 0x0123457a, 0xfadbcaa9, 0x89abde12,
    0x76543224, 0xfedcbaac, 0x579a6fa4, 0x657d5c02, 0x758ee433, 0x0123457b, 0xfadbcaaa, 0x89abde13,
    0x76543225, 0xfedcbaad, 0x579a6fa5, 0x657d5c03, 0x758ee434, 0x0123457c, 0xfadbcaab, 0x89abde14,
    0x76543226, 0xfedcbaae, 0x579a6fa6, 0x657d5c04, 0x758ee435, 0x0123457d, 0xfadbcaac, 0x89abde15,
    0x76543227, 0xfedcbaaf, 0x579a6fa7, 0x657d5c05, 0x758ee436, 0x0123457e, 0xfadbcaad, 0x89abde16,
    0x76543228, 0xfedcbab0, 0x579a6fa8, 0x657d5c06, 0x758ee437, 0x0123457f, 0xfadbcaae, 0x89abde17,
    0x76543220, 0xfedcbaa8, 0x579a6fa0, 0x657d5bfe, 0x758ee42f, 0x01234577, 0xfedbcaa6, 0x89abde0f,
    0x76543221, 0xfedcbaa9, 0x579a6fa1, 0x657d5bff, 0x758ee430, 0x01234578, 0xfadbcaa7, 0x89abde10,
    0x76543222, 0xfedcbaaa, 0x579a6fa2, 0x657d5c00, 0x758ee431, 0x01234579, 0xfadbcaa8, 0x89abde11,
    0x76543223, 0xfedcbaab, 0x579a6fa3, 0x657d5c01, 0x758ee432, 0x0123457a, 0xfadbcaa9, 0x89abde12,
    0x76543224, 0xfedcbaac, 0x579a6fa4, 0x657d5c02, 0x758ee433, 0x0123457b, 0xfadbcaaa, 0x89abde13,
    0x76543225, 0xfedcbaad, 0x579a6fa5, 0x657d5c03, 0x758ee434, 0x0123457c, 0xfadbcaab, 0x89abde14,
    0x76543226, 0xfedcbaae, 0x579a6fa6, 0x657d5c04, 0x758ee435, 0x0123457d, 0xfadbcaac, 0x89abde15,
    0x76543227, 0xfedcbaaf, 0x579a6fa7, 0x657d5c05, 0x758ee436, 0x0123457e, 0xfadbcaad, 0x89abde16,
    0x76543228, 0xfedcbab0, 0x579a6fa8, 0x657d5c06, 0x758ee437, 0x0123457f, 0xfadbcaae, 0x89abde17
};

// ====================== PROTOTYPES ======================
bool flash_read(spi_t* spi, uint32_t addr, uint32_t* dest_buff, uint32_t len); 
bool flash_erase_sector(spi_t* spi, uint32_t addr); 
bool flash_write_sector(spi_t* spi, uint32_t addr, uint32_t* src_buff); 
bool flash_write_enable(spi_t* spi); 
void flash_wait(spi_t* spi); 
// ========================= FUNCTIONS =========================

void X_init_spi()
{
    spi_slave_t slave = SPI_SLAVE(0, FLASH_MAX_FREQ);
    
    spi_flash = spi_init(SPI_IDX_FLASH, slave);
    if (!spi_flash.init) {
        PRINTF("\nFailed to initialize spi\n");
        return EXIT_FAILURE;
    }

    PRINTF("\nSPI initialized\n");
    
    CSR_SET_BITS(CSR_REG_MSTATUS, CSR_INTR_EN);
    // Set mie.MEIE bit to one to enable machine-level fast spi_flash interrupt
    const uint32_t mask = 1 << FIC_FLASH_MEIE;
    CSR_SET_BITS(CSR_REG_MIE, mask);

    X_test_read();
}

void X_spi_read(uint32_t address, uint32_t *data, uint32_t len)
{
    if (!flash_read(&spi_flash, address, data, 4*len))
    {
        PRINTF("\nFailed to read flash\n");
        return EXIT_FAILURE;   
    }
}

void X_test_read()
{
    uint32_t rxbuffer[READ_LEN] = {0};
    X_spi_read(WAD_START_ADDRESS, rxbuffer, READ_LEN);
    for (int i = 0; i < READ_LEN; i++)
    {
        PRINTF("0x%08X: %08X\n", WAD_START_ADDRESS+4*i, rxbuffer[i]);
    }
    
}

void X_spi_erase_sector(uint32_t addr)
{
    if (!flash_erase_sector(&spi_flash, addr)) return EXIT_FAILURE;
}

//This has not been tested yet ! 
void X_spi_write(uint32_t loc, void* buffer, uint32_t size)
{
    uint8_t* buffer_ptr = (uint8_t*)buffer;
    uint32_t address = loc;
    uint32_t bytes_remaining = size;

    while (bytes_remaining > 0)
    {
        uint32_t sector_buffer[SECT_LEN] = {0};

        uint32_t chunk_size = (bytes_remaining >= SECT_LEN) ? SECT_LEN : bytes_remaining;

        memcpy(&sector_buffer, buffer_ptr, chunk_size);

        if (!flash_write_sector(&spi_flash, address, sector_buffer)) return EXIT_FAILURE;

        // Update pointers and counters
        buffer_ptr += chunk_size;
        address += SECT_LEN;
        bytes_remaining -= chunk_size;
    }

}

void X_spi_write_sector(uint32_t address, uint32_t* sect_data)
{
    if (!flash_write_sector(&spi_flash, address, sect_data)) return EXIT_FAILURE;
} 

bool flash_read(spi_t* spi, uint32_t addr, uint32_t* dest_buff, uint32_t len) {
    // Transaction segments
    spi_segment_t segments[2] = { SPI_SEG_TX(4), SPI_SEG_RX(len) };

    // TX SPI command to send to flash for read
    // Flash uses Big Endian, CPU Little Endian, hence swap bytes
    uint32_t read_byte_cmd = ((bitfield_byteswap32(addr & 0x00ffffff)) | FC_RD);

    // PRINTF("Blocking Reading %4i Bytes at 0x%08X\n", len, addr);

    // Start transaction
    spi_codes_e error = spi_execute(spi, segments, 2, &read_byte_cmd, dest_buff);  

    if (error) {
        PRINTF("FAILED! Error Code: %i\n", error);
        return false;
    }
    return true;
}

bool flash_erase_sector(spi_t* spi, uint32_t addr) {
    // Sector start address
    const uint32_t sect_start = addr & 0xfffff000;
    // Sector erase command
    // Flash uses Big Endian, CPU Little Endian, hence swap bytes
    const uint32_t cmd = ((bitfield_byteswap32(sect_start & 0x00ffffff)) | FC_SE);

    PRINTF("Blocking Erasing 4096 Bytes at 0x%08X\n", sect_start);

    // Enable write before erase
    if (!flash_write_enable(spi)) return false;

    // Start TX Only transaction
    spi_codes_e error = spi_transmit(spi, &cmd, 4);
    if (error) {
        PRINTF("FAILED! Error Code: %i\n", error);
        return false;
    }

    // Wait flash finished processing
    flash_wait(spi);

    return true;
}

bool flash_write_sector(spi_t* spi, uint32_t addr, uint32_t* src_buff) {
    // Sector start address
    uint32_t sect = addr & 0xfffff000;
    // Current page (256 bytes) address (can't write more than one page at a time...)
    uint32_t curr_addr = sect;

    for (int i = 0; i < SECT_LEN / PAGE_LEN; i++)
    {
        // Our TX Buffer
        uint32_t wbuff[PAGE_LEN/4 + 1] = {0};
        // Our segments for the SPI transaction
        spi_segment_t segments[2] = { SPI_SEG_TX(4), SPI_SEG_TX(PAGE_LEN) };

        // Flash uses Big Endian, CPU Little Endian, hence swap bytes
        wbuff[0] = ((bitfield_byteswap32(curr_addr & 0x00ffffff)) | FC_PP);
        memcpy(&wbuff[1], &src_buff[i * (PAGE_LEN/4)], PAGE_LEN);

        if (!flash_write_enable(spi)) return false;

        // Start transaction
        // Note that since segments are only TX we could have used spi_transmit
        // instead of spi_execute, but both work perfectly fine
        spi_codes_e error = spi_execute(spi, segments, 2, wbuff, NULL);
        if (error) {
            PRINTF("FAILED! Error Code: %i\n", error);
            return false;
        }
        curr_addr += PAGE_LEN;

        PRINTF("\rBlocking Written %4i/4096 Bytes at 0x%08X", (i+1) * PAGE_LEN, sect);

        // Wait flash finished processing
        flash_wait(spi);
    }

    PRINTF("\n");
    
    return true;
}

bool flash_write_enable(spi_t* spi) {
    // SPI Flash command
    const uint32_t cmd = FC_WE;
    // Start TX Only transaction
    spi_codes_e error = spi_transmit(spi, &cmd, 1);
    if (error) {
        PRINTF("FAILED! Error Code: %i\n", error);
        return false;
    }
    return true;
}

void flash_wait(spi_t* spi) {
    // Response buffer
    uint32_t resp;
    // SPI Flash command
    uint32_t cmd = FC_RSR1;
    // Here we have to use segments and execute since our transaction is composed
    // of a TX Only part and thereafter a RX Only
    spi_segment_t segments[2] = {SPI_SEG_TX(1), SPI_SEG_RX(2)};
    // Flash busy flag
    bool busy = true;
    while (busy)
    {
        spi_execute(spi, segments, 2, &cmd, &resp);
        busy = resp & 0x01;
    }
}

uint32_t X_spi_alloc_sector()
{
    uint32_t loc = next_loc; 
    next_loc += SECT_LEN; 
    return loc; 
}

int32_t read_finesine(uint32_t index) {
    int32_t value;
    //uint32_t *test_buffer_flash = heep_get_flash_address_offset(finesine[index]);
    X_spi_read(&finesine[index], &value, sizeof(value)/4);
    return value;
}

int32_t read_finecosine(uint32_t index)
{
    return read_finesine(index + FINEANGLES/4); 
}

int32_t read_finetangent(uint32_t index) {
    int32_t value;
    X_spi_read(&finetangent[index], &value, sizeof(value)/4);
    return value;
}

int32_t read_viewangletox(uint32_t index)
{
    int32_t value;
    X_spi_read(&viewangletox[index], &value, sizeof(value)/4);
    return value;
}

//old version 

/*

#include "x-heep.h"
#include "w25q128jw.h"

spi_host_t *spi_flash_device;

//private function declarations
void X_test_read();

//public function definitions
void X_init_spi()
{
    //spi_flash_device->base_addr = mmio_region_from_addr((uintptr_t)SPI_HOST_START_ADDRESS);
    
    w25q128jw_init(spi_flash_device);
    PRINTF("X_SPI: init flash complete\n");
    PRINTF("X_SPI: Testing read\n");
    X_test_read(); //-> This function doesn't work. But it needs to work for the program to work !!!
    PRINTF("X_SPI: Finished testing read\n");
}

void X_spi_read(uint32_t address, uint32_t *data, uint32_t len)
{
    PRINTF("X_SPI: Reading data from WAD\n");
    w25q128jw_read_standard(address, data, len);
    PRINTF("X_SPI: Finished reading data from WAD\n");
}

//private function definitions
void X_test_read()
{
    uint32_t data[4];
    //X_spi_read(WAD_START_ADDRESS, data, 4);
    X_spi_read(0, data, 4);
    PRINTF("Data at WAD start address: %x %x %x %x\n", data[0], data[1], data[2], data[3]);
}
*/

/*
*   installer.c
*/

#include "installer.h"
#include "memory.h"
#include "fs.h"
#include "crypto.h"
#include "screeninit.h"
#include "draw.h"
#include "utils.h"
#include "fatfs/sdmmc/sdmmc.h"

static const u8 sectorHash[0x20] = {
    0x82, 0xF2, 0x73, 0x0D, 0x2C, 0x2D, 0xA3, 0xF3, 0x01, 0x65, 0xF9, 0x87, 0xFD, 0xCC, 0xAC, 0x5C,
    0xBA, 0xB2, 0x4B, 0x4E, 0x5F, 0x65, 0xC9, 0x81, 0xCD, 0x7B, 0xE6, 0xF4, 0x38, 0xE6, 0xD9, 0xD3
};

static const u8 firm0Hash[0x20] = {
    0x6E, 0x4D, 0x14, 0xAD, 0x51, 0x50, 0xA5, 0x9A, 0x87, 0x59, 0x62, 0xB7, 0x09, 0x0A, 0x3C, 0x74,
    0x4F, 0x72, 0x4B, 0xBD, 0x97, 0x39, 0x33, 0xF2, 0x11, 0xC9, 0x35, 0x22, 0xC8, 0xBB, 0x1C, 0x7D
};

static const u8 firm0A9lhHash[0x20] = {
    0x79, 0x3D, 0x35, 0x7B, 0x8F, 0xF1, 0xFC, 0xF0, 0x8F, 0xB6, 0xDB, 0x51, 0x31, 0xD4, 0xA7, 0x74,
    0x8E, 0xF0, 0x4A, 0xB1, 0xA6, 0x7F, 0xCD, 0xAB, 0x0C, 0x0A, 0xC0, 0x69, 0xA7, 0x9D, 0xC5, 0x04
};

static const u8 firm10_2Hash[0x20] = {
    0xD2, 0x53, 0xC1, 0xCC, 0x0A, 0x5F, 0xFA, 0xC6, 0xB3, 0x83, 0xDA, 0xC1, 0x82, 0x7C, 0xFB, 0x3B,
    0x2D, 0x3D, 0x56, 0x6C, 0x6A, 0x1A, 0x8E, 0x52, 0x54, 0xE3, 0x89, 0xC2, 0x95, 0x06, 0x23, 0xE5
};

static const u8 firm10_0Hash[0x20] = {
    0xD8, 0x2D, 0xB7, 0xB4, 0x38, 0x2B, 0x07, 0x88, 0x99, 0x77, 0x91, 0x0C, 0xC6, 0xEC, 0x6D, 0x87,
    0x7D, 0x21, 0x79, 0x23, 0xD7, 0x60, 0xAF, 0x4E, 0x8B, 0x3A, 0xAB, 0xB2, 0x63, 0xE4, 0x21, 0xC6
};

static const u8 branchSled[0x2C] = { // Loaded at 0x80FD0F8. Jumps to a9lh stage 1.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x4A, 0xFE, 0xEA
};

int posY;

u32 console;

void main(void)
{
    initScreens();
    sdmmc_sdcard_init();

    //Detect the console being used
    console = PDN_MPCORE_CFG == 7;
    
    if (console)
    {
        drawString(TITLE, 10, 10, COLOR_TITLE);
        posY = drawString("Thanks to delebile, #cakey and StandardBus", 10, 40, COLOR_WHITE);
        posY = drawString("Press SELECT to unsafely install A9LH.", 10, posY + SPACING_Y, COLOR_WHITE);
        posY = drawString("Note: You will reboot twice, if you do.", 10, posY + SPACING_Y, COLOR_WHITE);
        posY = drawString("Press any other button to shutdown", 10, posY, COLOR_WHITE);
        u32 pressed = waitInput();
        if(pressed == BUTTON_SELECT) installer();
    }
    else
    {
        posY = drawString("Thanks to delebile, #cakey and StandardBus", 10, 40, COLOR_WHITE);
        posY = drawString("UnsafeA9LHInstaller is N3DS only", 10, posY + SPACING_Y, COLOR_WHITE);
        posY = drawString("Press any button to shutdown", 10, posY, COLOR_WHITE);
        waitInput();
    }

    shutdown(0, NULL);
}

static inline void installer(void)
{
    if(!mountSD())
        shutdown(1, "Error: failed to mount the SD card");

    const char *path;
    u32 updatea9lh = 0;

    // Hey, Doc, we better back up. We don't have enough OTP to get up to 88.
    /*
        if(!a9lhBoot)
        {
            // Prefer OTP from memory if available
            const u8 zeroes[256] = {0};
            path = "a9lh/otp.bin";
            if(memcmp((void *)OTP_FROM_MEM, zeroes, 256) == 0)
            {
                // Read OTP from file
                if(fileRead((void *)OTP_OFFSET, path) != 256)
                {
                    shutdown(1, "Error: otp.bin doesn't exist and can't be dumped");            
                }
            }
            else
            {
                // Write OTP from memory to file
                fileWrite((void *)OTP_FROM_MEM, path, 256);
                memcpy((void *)OTP_OFFSET, (void *)OTP_FROM_MEM, 256);
            }
        }
    */

    // OTP? Where we're going, we don't need OTP.
    // setupKeyslot0x11(a9lhBoot, (void *)OTP_OFFSET);

    //Calculate the CTR for the 3DS partitions
    getNandCTR();

    //Get NAND FIRM0 and test that the CTR is correct
    readFirm0((u8 *)FIRM0_OFFSET, FIRM0_SIZE);
    if(memcmp((void *)FIRM0_OFFSET, "FIRM", 4) != 0)
        shutdown(1, "Error: failed to setup FIRM encryption");

    // We're on an N3DS. Sector decryption isn't allowed.
    getEncryptedSector((u8 *)SECTOR_OFFSET);
         
    const u8 bad_sector[256] = {0};
         
    if(memcmp((void *)SECTOR_OFFSET, bad_sector, 256) == 0)
        shutdown(1, "Error: Failed to read encrypted key sector.");
    
    //Generate and encrypt a per-console A9LH key sector
    generateSector((u8 *)SECTOR_OFFSET);

    //Read FIRM0
    path = "a9lh/firm0.bin";
    if(fileRead((void *)FIRM0_OFFSET, path) != FIRM0_SIZE)
        shutdown(1, "Error: firm0.bin doesn't exist or has a wrong size");

    if(!verifyHash((void *)FIRM0_OFFSET, FIRM0_SIZE, firm0Hash))
        shutdown(1, "Error: firm0.bin is invalid or corrupted");

    //Read 10.2 FIRM, verify it's good
    path = "a9lh/firm1.bin";
    if(fileRead((void *)FIRM1_OFFSET, path) != FIRM1_SIZE)
        shutdown(1, "Error: firm1.bin doesn't exist or has a wrong size");

    if(!verifyHash((void *)FIRM1_OFFSET, FIRM1_SIZE, firm10_2Hash))
        shutdown(1, "Error: firm1.bin is invalid or corrupted");
    
    //Read 10.0 FIRM, verify it's good
    path = "a9lh/10_0_firm.bin";
    if(fileRead((void *)FIRM1_OFFSET, path) != FIRM1_SIZE)
        shutdown(1, "Error: 10_0_firm.bin doesn't exist or has a wrong size");

    if(!verifyHash((void *)FIRM1_OFFSET, FIRM1_SIZE, firm10_0Hash))
        shutdown(1, "Error: 10_0_firm.bin is invalid or corrupted");
    
    //Payload, yo
    path = "a9lh/otpless_payload.bin";
    u32 size = fileRead((void *)OTPLESS_OFFSET, path);

    if(!size || size > MAX_OTPLESS_SIZE)
        shutdown(1, "Error: otpless_payload.bin doesn't exist or\nexceeds max size");
    
    // Hijack arm9loaderhax.bin. Sorry, users.
    fileWrite((void *)OTPLESS_OFFSET, "arm9loaderhax.bin", size); 

    //Inject stage1
    memset32((void *)STAGE1_OFFSET, 0, MAX_STAGE1_SIZE);
    path = "a9lh/payload_stage1.bin";
    size = fileRead((void *)STAGE1_OFFSET, path);
    if(!size || size > MAX_STAGE1_SIZE)
        shutdown(1, "Error: payload_stage1.bin doesn't exist or\nexceeds max size");

    const u8 zeroes[688] = {0};
    if(memcmp(zeroes, (void *)STAGE1_OFFSET, 688) == 0)
        shutdown(1, "Error: the payload_stage1.bin you're attempting\nto install is not compatible");

    //Read stage2
    memset32((void *)STAGE2_OFFSET, 0, MAX_STAGE2_SIZE);
    path = "a9lh/payload_stage2.bin";
    size = fileRead((void *)STAGE2_OFFSET, path);
    if(!size || size > MAX_STAGE2_SIZE)
        shutdown(1, "Error: payload_stage2.bin doesn't exist or\nexceeds max size");
    
    // Copy in branch sled
    memcpy((void *)BRANCHCODE_OFFSET, branchSled, 0x2C);

    posY = drawString("All checks passed, #yolo...", 10, posY + SPACING_Y, COLOR_WHITE);

    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x5C000, MAX_STAGE2_SIZE / 0x200, (vu8 *)STAGE2_OFFSET);
    writeFirm((u8 *)FIRM1_OFFSET, 1, FIRM1_SIZE);
    sdmmc_nand_writesectors(0x96, 1, (vu8 *)SECTOR_OFFSET);
    writeFirm((u8 *)FIRM0_OFFSET, 0, FIRM0_SIZE);

    reboot();
}

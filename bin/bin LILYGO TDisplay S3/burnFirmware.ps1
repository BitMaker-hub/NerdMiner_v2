#GET CURRENT COMPORT IF MORE THAN ONE (NOT RESOLVED)
$PORT = [System.IO.Ports.SerialPort]::getportnames()


#BURN BOOTLOADER, FIRMWARE
python -m esptool -p $PORT -b 460800 --before default_reset --after hard_reset --chip esp32s3  write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x0000 0x0000_bootloader.bin 0x8000 0x8000_partitions.bin 0xe000 0xe000_boot_app0.bin 0x10000 0x10000_firmware.bin

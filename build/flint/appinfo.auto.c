#include "pebble_process_info.h"
#include "src/resource_ids.auto.h"

const PebbleProcessInfo __pbl_app_info __attribute__ ((section (".pbl_header"))) = {
  .header = "PBLAPP",
  .struct_version = { PROCESS_INFO_CURRENT_STRUCT_VERSION_MAJOR, PROCESS_INFO_CURRENT_STRUCT_VERSION_MINOR },
  .sdk_version = { PROCESS_INFO_CURRENT_SDK_VERSION_MAJOR, PROCESS_INFO_CURRENT_SDK_VERSION_MINOR },
  .process_version = { 1, 0 },
  .load_size = 0xb6b6,
  .offset = 0xb6b6b6b6,
  .crc = 0xb6b6b6b6,
  .name = "UnlockThePlock",
  .company = "h3nry.xyz",
  .icon_resource_id = RESOURCE_ID_IMAGE_APP_ICON,
  .sym_table_addr = 0xA7A7A7A7,
  .flags = PROCESS_INFO_PLATFORM_FLINT,
  .num_reloc_entries = 0xdeadcafe,
  .uuid = { 0x9A, 0x21, 0x57, 0xCC, 0x39, 0x5B, 0x4D, 0x0E, 0x95, 0x2B, 0x8E, 0xC6, 0xBC, 0xB7, 0x8B, 0xB1 },
  .virtual_size = 0xb6b6
};

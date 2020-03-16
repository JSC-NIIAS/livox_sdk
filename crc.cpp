#include "crc.h"

#include "crc_tables.h"

//=======================================================================================

#define crc_n4(crc, data, table) crc ^= data; \
    crc = (table[(crc & 0xff) + 0x300]) ^		\
          (table[((crc >> 8) & 0xff) + 0x200]) ^	\
          (table[((data >> 16) & 0xff) + 0x100]) ^	\
          (table[data >> 24]);

#define crc_n4d(crc, data, table) crc ^= data; \
    crc = (table[(crc & 0xff) + 0x300]) ^	\
          (table[((crc >> 8) & 0xff) + 0x200]) ^	\
          (table[((crc >> 16) & 0xff) + 0x100]) ^	\
          (table[(crc >> 24) & 0xff]);

#define crcsm_n4d(crc, data, table) crc ^= data; \
    crc = (crc >> 8) ^ (table[crc & 0xff]); \
    crc = (crc >> 8) ^ (table[crc & 0xff]); \
    crc = (crc >> 8) ^ (table[crc & 0xff]); \
    crc = (crc >> 8) ^ (table[crc & 0xff]);


#if CRC_BIGTABLES
#define CRC_TABLE_CRC32 crc_table_crc32_big
#else
#define CRC_TABLE_CRC32 crc_table_crc32
#endif

//=======================================================================================
CRC_16::CRC_16( const uint16_t seed )
{
    _seed = seed;
}
//=======================================================================================
uint16_t CRC_16::mcrf4xx_calc( const uint8_t* data, uint16_t length )
{
    uint16_t crc = _seed;

    while ( ( ( uintptr_t ) data & 3 ) && length )
    {
        crc = ( crc >> 8 ) ^ crc_table_mcrf4xx[ ( crc & 0xff ) ^ *data++ ];
        length--;
    }

    while ( length >= 16 )
    {
        length -= 16;

        crc_n4( crc, ( ( uint32_t * ) data )[0], crc_table_mcrf4xx );
        crc_n4( crc, ( ( uint32_t * ) data )[1], crc_table_mcrf4xx );
        crc_n4( crc, ( ( uint32_t * ) data )[2], crc_table_mcrf4xx );
        crc_n4( crc, ( ( uint32_t * ) data )[3], crc_table_mcrf4xx );

        data += 16;
    }

    while ( length-- )
        crc = ( crc >> 8 ) ^ crc_table_mcrf4xx[ ( crc & 0xff ) ^ *data++ ];

    return crc;
}
//=======================================================================================


//=======================================================================================
CRC_32::CRC_32( const uint32_t seed )
{
    _seed = seed;
}
//=======================================================================================
uint32_t CRC_32::crc32_calc( const uint8_t* data, uint16_t length )
{
    uint32_t crc = _seed ^ 0xffffffff;

    while ( ( ( uintptr_t ) data & 3 ) && length )
    {
        crc = ( crc >> 8 ) ^ CRC_TABLE_CRC32 [ ( crc & 0xff ) ^ *data++ ];
        length--;
    }

    while ( length >= 16 )
    {
        length -= 16;

#if CRC_BIGTABLES
        crc_n4d(crc, ((uint32_t *)data)[0], CRC_TABLE_CRC32);
        crc_n4d(crc, ((uint32_t *)data)[1], CRC_TABLE_CRC32);
        crc_n4d(crc, ((uint32_t *)data)[2], CRC_TABLE_CRC32);
        crc_n4d(crc, ((uint32_t *)data)[3], CRC_TABLE_CRC32);
#else
        crcsm_n4d(crc, ((uint32_t *)data)[0], CRC_TABLE_CRC32);
        crcsm_n4d(crc, ((uint32_t *)data)[1], CRC_TABLE_CRC32);
        crcsm_n4d(crc, ((uint32_t *)data)[2], CRC_TABLE_CRC32);
        crcsm_n4d(crc, ((uint32_t *)data)[3], CRC_TABLE_CRC32);
#endif

        data += 16;
    }

    while ( length-- )
        crc = ( crc >> 8 ) ^ CRC_TABLE_CRC32 [ ( crc & 0xff ) ^ *data++ ];

    crc ^= 0xffffffff;

    return crc;
}
//=======================================================================================

#include "data_head.h"

uint8_t GLOBAL_DATA_POOLS[64] = {0};

//float angle        = 0;
//float temperature  = 0;
//float ruler        = 0; 

void data_pools_write(uint8_t addr,uint8_t len,void *data)
{
    uint8_t * dap = data;
    for(uint8_t i=0;i<len;i++)
        GLOBAL_DATA_POOLS[addr+i] = dap[i];
}

void data_pools_read(uint8_t addr,uint8_t len,void *data)
{
    uint8_t *dap = data; 
    
    for(uint8_t i=0;i<len;i++)
        dap[i] = GLOBAL_DATA_POOLS[addr+i];
}

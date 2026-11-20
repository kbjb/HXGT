#include "keys_head.h"



KEY_STATE KEYS_LOGIC_CHECK(KEY_PARAM_STR * key_Param_str)
{
    if(key_Param_str->now_level == key_Param_str->target_level)//按键按下
    {
        key_Param_str->countDown_cnt = key_Param_str->countDown_thre;
        if(key_Param_str->last_level != key_Param_str->now_level)
            key_Param_str->cnt++;
        key_Param_str->time++;
//        if(key_Param_str->time == key_Param_str->time_thre && key_Param_str->state != long_down)
//        {
//            key_Param_str->state = long_down;
//        }
        if(key_Param_str->time >= key_Param_str->time_thre)
        {
            key_Param_str->state = long_down;
        }
        else
            key_Param_str->state = void_down;
    }
    else//按键未按下
    {
        if(key_Param_str->cnt!=0)//确定按键被按下过
        {
            if(key_Param_str->countDown_cnt >0)//开始倒计时
            {
                key_Param_str->countDown_cnt --;
                key_Param_str->state = void_down;
            }
            else if(key_Param_str->countDown_cnt == 0)
            {
                switch(key_Param_str->cnt)
                {
                    case 1:
                        key_Param_str->state = short_down;
                        break;
                    case 2:
                        key_Param_str->state = cnt2_down;
                        break;
                    case 3:
                        key_Param_str->state = cnt3_down;
                        break;
                    default:
                        key_Param_str->state = cnt4_down;
                        break;
                }
            }
        }
        else
            key_Param_str->state = none_down;
    }
    if(key_Param_str->state != void_down&&key_Param_str->state != long_down)
    {
        //初始化参数
        key_Param_str->cnt = 0;
        key_Param_str->time = 0;
    }
    key_Param_str->last_level = key_Param_str->now_level;
    return key_Param_str->state;
}




void KEYS_NORMAL_INIT(KEY_NORMAL_STR * key_normal_str)
{
    key_normal_str->gpio_str->str_init(key_normal_str->gpio_str);
    key_normal_str->key_param->last_level = key_normal_str->check(key_normal_str);
    key_normal_str->key_param->now_level = key_normal_str->key_param->now_level;
}

BitAction KEYS_NORMAL_STATE_CHECK(KEY_NORMAL_STR * key_normal_str)
{
    key_normal_str->key_param->now_level =  key_normal_str->gpio_str->read(key_normal_str->gpio_str);
    return key_normal_str->key_param->now_level;
}

KEY_STATE KEYS_NORMAL_LOGIC_CHECK(KEY_NORMAL_STR * key_normal_strs)
{
    return key_normal_strs->key_param->logic_check(key_normal_strs->key_param);
}

void KEYS_NORMAL_STR_INIT(KEY_NORMAL_STR * key_normal_str)
{
    key_normal_str->init = KEYS_NORMAL_INIT;
    key_normal_str->check = KEYS_NORMAL_STATE_CHECK;
    key_normal_str->logic = KEYS_NORMAL_LOGIC_CHECK;
    key_normal_str->init(key_normal_str);
}





void KEYS_MATRTX_ADD_PARAM(KEY_MATRIX_STR *key_matrix,KEY_PARAM_STR *key_param)
{
    key_matrix->keys_param[key_param->num] = key_param;
}


void KEYS_GPIO_ADD(KEY_MATRIX_STR *key_matrix,GPIO_STR *gpio_str,uint8_t t)
{
    gpio_str->str_init(gpio_str);
    if(t == 0)
    {
        if(key_matrix->high_i<key_matrix->high)
        {
            key_matrix->high_gpio_str[key_matrix->high_i] = gpio_str;
            key_matrix->high_i++;
        }
    }
    else
    {
        if(key_matrix->width_i<key_matrix->width)
        {
            gpio_str->write(gpio_str,Bit_RESET);
            key_matrix->width_gpio_str[key_matrix->width_i] = gpio_str;
            key_matrix->width_i++;
        }
    }
}


void KEYS_MATRIX_CHECK(KEY_MATRIX_STR *key_matrix)
{
    uint8_t addr;
    for(uint8_t i=0;i<key_matrix->width;i++)
    {
        key_matrix->width_gpio_str[i]->write(key_matrix->width_gpio_str[i],Bit_SET);
        vTaskDelay(2);
        for(uint8_t j=0;j< key_matrix->high;j++)
        {
            addr = (i<<1)+j;
            key_matrix->keys_param[addr]->now_level = key_matrix->high_gpio_str[j] ->read(key_matrix->high_gpio_str[j]);
        }
        key_matrix->width_gpio_str[i]->write(key_matrix->width_gpio_str[i],Bit_RESET);
    }
}


void KEYS_MATRIX_LOGIC_CHECK(KEY_MATRIX_STR *key_matrix)
{
    for(uint8_t i=0;i<(key_matrix->high*key_matrix->width);i++)
        key_matrix->keys_param[i]->logic_check(key_matrix->keys_param[i]);
}


void KEYS_MATRIX_INIT(KEY_MATRIX_STR *key_matrix)
{
    key_matrix->keys_param = (KEY_PARAM_STR**)malloc(sizeof(KEY_PARAM_STR*)*(key_matrix->high*key_matrix->width));
    key_matrix->high_gpio_str = (GPIO_STR **)malloc(sizeof(GPIO_STR*)*key_matrix->high);
    key_matrix->width_gpio_str = (GPIO_STR **)malloc(sizeof(GPIO_STR*)*key_matrix->width);
    
    for(uint8_t i=0;i<key_matrix->high_i;i++)
        key_matrix->high_gpio_str[i]->str_init(key_matrix->high_gpio_str[i]);
    for(uint8_t i=0;i<key_matrix->width_i;i++)
        key_matrix->width_gpio_str[i]->str_init(key_matrix->width_gpio_str[i]);
}










void KEYS_MATRIX_STR_INIT(KEY_MATRIX_STR*key_matrix)
{
    key_matrix->add_param = KEYS_MATRTX_ADD_PARAM;
    key_matrix->add_gpio  = KEYS_GPIO_ADD;
    key_matrix->init = KEYS_MATRIX_INIT;
    key_matrix->check = KEYS_MATRIX_CHECK;
    key_matrix->logic = KEYS_MATRIX_LOGIC_CHECK;
    key_matrix->init(key_matrix);
}



















/********************************************************OS**********************************************************************************/

/*普通按键创建*/
GPIO_STR       onoff_gpio     = GPIO_CLASS_CREATE(ONOFF_GPIO_PORT,ONOFF_GPIO_PIN,GPIO_Mode_IN_FLOATING,GPIO_Speed_50MHz);
KEY_PARAM_STR  onoff_param    = KEY_PARAM_CREATE(0,Bit_RESET,40,200);
KEY_NORMAL_STR onoff_key      = KEY_NORMAL_CLASS_CREATE(&onoff_gpio,&onoff_param); 

GPIO_STR       lfgi_gpio      = GPIO_CLASS_CREATE(LFRI_GPIO_PORT,LFRI_GPIO_PIN,GPIO_Mode_IN_FLOATING,GPIO_Speed_50MHz);
KEY_PARAM_STR  lfgi_param     = KEY_PARAM_CREATE(0,Bit_RESET,40,200);
KEY_NORMAL_STR lfri_key       = KEY_NORMAL_CLASS_CREATE(&lfgi_gpio,&lfgi_param);

/*矩阵按键创建*/
GPIO_STR       width1_gpio    = GPIO_CLASS_CREATE(WIDTH1_GPIO_PORT,WIDTH1_GPIO_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR       width2_gpio    = GPIO_CLASS_CREATE(WIDTH2_GPIO_PORT,WIDTH2_GPIO_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR       width3_gpio    = GPIO_CLASS_CREATE(WIDTH3_GPIO_PORT,WIDTH3_GPIO_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR       high1_gpio     = GPIO_CLASS_CREATE(HIGH1_GPIO_PORT,HIGH1_GPIO_PIN,GPIO_Mode_IPD,GPIO_Speed_50MHz);
GPIO_STR       high2_gpio     = GPIO_CLASS_CREATE(HIGH2_GPIO_PORT,HIGH2_GPIO_PIN,GPIO_Mode_IPD,GPIO_Speed_50MHz);

KEY_PARAM_STR  set_param      = KEY_PARAM_CREATE(0,Bit_SET,30,100);
KEY_PARAM_STR  left_param     = KEY_PARAM_CREATE(1,Bit_SET,30,100);
KEY_PARAM_STR  up_param       = KEY_PARAM_CREATE(2,Bit_SET,30,200);
KEY_PARAM_STR  down_param     = KEY_PARAM_CREATE(3,Bit_SET,30,200);
KEY_PARAM_STR  back_param     = KEY_PARAM_CREATE(4,Bit_SET,30,200);
KEY_PARAM_STR  right_param    = KEY_PARAM_CREATE(5,Bit_SET,30,200);

KEY_MATRIX_STR key_matrix_str = KEY_MATRIX_CLASS_CREATE(3,2);




void key_task(void *pvParameters)
{
//    KEY_STATE temp;
    uint16_t temp;
    /*普通按键*/
    onoff_key.str_init(&onoff_key);
    lfri_key.str_init(&lfri_key);
    /*矩阵按键*/
    key_matrix_str.str_init(&key_matrix_str);
    key_matrix_str.add_gpio(&key_matrix_str,&width1_gpio,KEY_MATRIX_WIDTH_ADD);
    key_matrix_str.add_gpio(&key_matrix_str,&width2_gpio,KEY_MATRIX_WIDTH_ADD);
    key_matrix_str.add_gpio(&key_matrix_str,&width3_gpio,KEY_MATRIX_WIDTH_ADD);
    key_matrix_str.add_gpio(&key_matrix_str,&high1_gpio,KEY_MATRIX_HIGH_ADD);
    key_matrix_str.add_gpio(&key_matrix_str,&high2_gpio,KEY_MATRIX_HIGH_ADD);
    key_matrix_str.add_param(&key_matrix_str,&set_param);
    key_matrix_str.add_param(&key_matrix_str,&left_param);
    key_matrix_str.add_param(&key_matrix_str,&up_param);
    key_matrix_str.add_param(&key_matrix_str,&down_param);
    key_matrix_str.add_param(&key_matrix_str,&back_param);
    key_matrix_str.add_param(&key_matrix_str,&right_param);
    while(1)
    {
        onoff_key.check(&onoff_key);
        lfri_key.check(&lfri_key);
        key_matrix_str.check(&key_matrix_str);
        
        
        onoff_key.logic(&onoff_key);
        lfri_key.logic(&lfri_key);
        key_matrix_str.logic(&key_matrix_str);
        
        
        
        if(onoff_key.key_param->state != void_down && onoff_key.key_param->state != none_down)
        {
            if(onoff_key.key_param->state == short_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == MAIN_PAGE_ID)
                {
                    temp = SHORTCUT_MENU_PAGE_ID;
                    data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                }
            }
        }
        if(up_param.state != void_down && up_param.state != none_down)
        {
            if(up_param.state == short_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == MAIN_PAGE_ID)
                {
                    temp = CHECKUP_PAGE_ID;
                    data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                }
                else if(temp == SHORTCUT_MENU_PAGE_ID)
                {
                    data_pools_read(SHORTCUT_MENU_ID_ADDR,SHORTCUT_MENU_ID_LEN,&temp);
                    if(temp == (SHORTCUT_MENU_NUM-1))temp = 0;
                    else temp++;
                    data_pools_write(SHORTCUT_MENU_ID_ADDR,SHORTCUT_MENU_ID_LEN,&temp);
                }
            }
        }
        if(down_param.state != void_down && down_param.state != none_down)
        {
            if(down_param.state == short_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == SHORTCUT_MENU_PAGE_ID)
                {
                    data_pools_read(SHORTCUT_MENU_ID_ADDR,SHORTCUT_MENU_ID_LEN,&temp);
                    if(temp == 0)temp = SHORTCUT_MENU_NUM-1;
                    else temp--;
                    data_pools_write(SHORTCUT_MENU_ID_ADDR,SHORTCUT_MENU_ID_LEN,&temp);
                }
            }
            else if(down_param.state == cnt3_down)
            {
                    temp = CHECKUP_PAGE_ID;
                    data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
            }
        }
        if(back_param.state != void_down && back_param.state != none_down)
        {
            if(back_param.state == short_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == SHORTCUT_MENU_PAGE_ID)
                {
                    temp = MAIN_PAGE_ID;
                    data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                }
                else if(temp == CHECKUP_PAGE_ID)
                {
                    temp = MAIN_PAGE_ID;
                    data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                }
                else if(temp == MUBIAO_PAGE_ID)
                {
                    temp = MAIN_PAGE_ID;
                    data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                }
            }
        }
        if(left_param.state != void_down && left_param.state != none_down)
        {
            if(left_param.state == short_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == MUBIAO_PAGE_ID)
                {
                    data_pools_read(MUBIAO_SELECT_ADDR,MUBIAO_SELECT_LEN,&temp);
                    if(temp == 0)temp = 0;
                    else temp--;
                    data_pools_write(MUBIAO_SELECT_ADDR,MUBIAO_SELECT_LEN,&temp);
                }
            }
            else if(left_param.state == long_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == MAIN_PAGE_ID)
                {
                    if(set_param.state == long_down)
                    {
                        temp = MUBIAO_PAGE_ID;
                        data_pools_write(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                    }
                }
            }
        }
        if(right_param.state != void_down && right_param.state != none_down)
        {
            if(right_param.state == short_down)
            {
                data_pools_read(POOLS_PAGE_ID_ADDR,POOLS_PAGE_ID_LEN,&temp);
                if(temp == MUBIAO_PAGE_ID)
                {
                    data_pools_read(MUBIAO_SELECT_ADDR,MUBIAO_SELECT_LEN,&temp);
                    if(temp == 8)temp = 8;
                    else temp++;
                    data_pools_write(MUBIAO_SELECT_ADDR,MUBIAO_SELECT_LEN,&temp);
                }
            }
        }
        vTaskDelay(4);
    }
}

 
void Key_Task_Create(void)
{
    //任务句柄
    TaskHandle_t KeyTask_Handler;
        //创建LED1任务
    xTaskCreate((TaskFunction_t )key_task,
                (const char*    )"key_task",
                (uint16_t       )KEY_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )KEY_TASK_PRIO,
                (TaskHandle_t*  )&KeyTask_Handler);
}

























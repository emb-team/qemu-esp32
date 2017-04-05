/*
 * ESP32 I2C controller
 *
 * Copyright (c) 2017 Olof Astrand
 *
 * This file is derived from i2c/versatile_i2c.c by CodeSourcery & 
 *    Oskar Andero <oskar.andero@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/i2c_esp32.h"
#include "qemu/log.h"

#define BIT(nr)                 (1UL << (nr))


#define  I2C_TXFIFO_EMPTY_INT  BIT(1)
#define  I2C_TRANS_COMPLETE_INT  BIT(7)
#define  I2C_TRANS_START_INT  (BIT(9))

#define DEFINE_BITS(prefix, reg, field, shift, len) \
    prefix##_##reg##_##field##_SHIFT = shift, \
    prefix##_##reg##_##field##_LEN = len, \
    prefix##_##reg##_##field = ((~0U >> (32 - (len))) << (shift))


enum {
     I2C_SCL_LOW_PERIOD_REG=0,
     I2C_CTR_REG,
     I2C_SR_REG,
     I2C_TO_REG,
     I2C_SLAVE_ADDR_REG,
     I2C_RXFIFO_ST_REG,
     I2C_FIFO_CONF_REG,  // 0x18
     I2C_SCL_UNUSED1,    //0x1c     
     I2C_INT_RAW_REG,
     I2C_INT_CLR_REG,
     I2C_INT_ENA_REG,
     I2C_INT_STATUS_REG,
     I2C_SDA_HOLD_REG,
     I2C_SDA_SAMPLE_REG,
     I2C_SCL_HIGH_PERIOD_REG,
     I2C_SCL_UNUSED2,    //0x3c
     I2C_SCL_START_HOLD_REG,
     I2C_SCL_RSTART_SETUP_REG,
     I2C_SCL_STOP_HOLD_REG,
     I2C_SCL_STOP_SETUP_REG,
     I2C_SCL_FILTER_CFG_REG,
     I2C_SDA_FILTER_CFG_REG,
     I2C_COMD0_REG,
     I2C_COMD1_REG,
     I2C_COMD2_REG,
     I2C_COMD3_REG,
     I2C_COMD4_REG,
     I2C_COMD5_REG,
     I2C_COMD6_REG,
     I2C_COMD7_REG,
     I2C_COMD8_REG,
     I2C_COMD9_REG,
     I2C_COMD10_REG,
     I2C_COMD11_REG,
     I2C_COMD12_REG,
     I2C_COMD13_REG,
     I2C_COMD14_REG,
     I2C_COMD15_REG,     
     I2C_FIFO_START = 0x100/4  
 };

const char *I2C_REG_NAME[]= {
    "I2C_SCL_LOW_PERIOD_REG",
    "I2C_CTR_REG",
    "I2C_SR_REG",
    "I2C_TO_REG",
    "I2C_SLAVE_ADDR_REG",
    "I2C_RXFIFO_ST_REG",
    "I2C_FIFO_CONF_REG",  
    "I2C_SCL_UNUSED1",         
    "I2C_INT_RAW_REG",
    "I2C_INT_CLR_REG",
    "I2C_INT_ENA_REG",
    "I2C_INT_STATUS_REG",
    "I2C_SDA_HOLD_REG",
    "I2C_SDA_SAMPLE_REG",
    "I2C_SCL_HIGH_PERIOD_REG",
    "I2C_SCL_UNUSED2",    
    "I2C_SCL_START_HOLD_REG",
    "I2C_SCL_RSTART_SETUP_REG",
    "I2C_SCL_STOP_HOLD_REG",
    "I2C_SCL_STOP_SETUP_REG",
    "I2C_SCL_FILTER_CFG_REG",
    "I2C_SDA_FILTER_CFG_REG",
    "I2C_COMD0_REG",
    "I2C_COMD1_REG",
    "I2C_COMD2_REG",
    "I2C_COMD3_REG",
    "I2C_COMD4_REG",
    "I2C_COMD5_REG",
    "I2C_COMD6_REG",
    "I2C_COMD7_REG",
    "I2C_COMD8_REG",
    "I2C_COMD9_REG",
    "I2C_COMD10_REG",
    "I2C_COMD11_REG",
    "I2C_COMD12_REG",
    "I2C_COMD13_REG",
    "I2C_COMD14_REG",
    "I2C_COMD15_REG"    
};



#define ESP32_I2C(obj) \
    OBJECT_CHECK(Esp32I2CState, (obj), TYPE_ESP32_I2C)

typedef struct Esp32I2CState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    I2CBus   *bus;
    unsigned int command_reg[16];
    unsigned int  i2c_ctr_reg;
    unsigned int  i2c_sr_reg;
    unsigned int  i2c_fifo_conf_reg;

    //unsigned int data_address;
    unsigned int int_status;
    int num_out;
    int out;
    int in;
} Esp32I2CState;


// TODO, Let the esp32_i2c driver own the apb data

unsigned int data_offset=0;

unsigned int apb_data[128];


//        qemu_irq_raise(s->irq);
//        qemu_irq_lower(s->irq);

void esp32_i2c_fifo_dataSet(int offset,unsigned int data) {
    //if (offset==0) 
    {
        apb_data[data_offset++]=data;
    }
    //if (offset<128)
    //   apb_data[offset]=data;
}


// The interrupt is dynamically allocated by writing to DPORT_PRO_I2C_EXT0_INTR_MAP_REG
qemu_irq irq;

void esp32_i2c_interruptSet(qemu_irq new_irq) 
{
    irq=new_irq;
}


static uint64_t esp32_i2c_read(void *opaque, hwaddr offset,
                                   unsigned size)
{
    Esp32I2CState *s = (Esp32I2CState *)opaque;

    //qemu_log_mask(LOG_GUEST_ERROR,
    //                "%s: %s regnum 0x%x\n", __func__,I2C_REG_NAME[offset/4], (int)offset);


    switch (offset/4) {
        case I2C_FIFO_CONF_REG:
             return(s->i2c_fifo_conf_reg);
             break;
        case I2C_SR_REG:
             //return(s->i2c_sr_reg);
             return(-1);
             break;
        //Arduino driver
        case I2C_INT_RAW_REG:
             return(0);
             break;
        case I2C_INT_STATUS_REG:

            qemu_log_mask(LOG_GUEST_ERROR,
                    "%s: I2C_INT_STATUS_REG 0x%x\n", __func__, (int)s->int_status);

            // This is not true but we need to get of int loop

            return(s->int_status);
            break;

        case I2C_INT_CLR_REG:

             return(0);
             break;


        case I2C_CTR_REG:
            //s->i2c_ctr_reg=0;
           // qemu_log_mask(LOG_GUEST_ERROR,
           //         "%s: I2C_CTR_REGt 0x%x\n", __func__, (int)s->i2c_ctr_reg);

            return(s->i2c_ctr_reg);
            break;

        case I2C_COMD2_REG:
            return(0);
            break;

        case I2C_COMD0_REG:
        case I2C_COMD1_REG:
        //case I2C_COMD2_REG:
        case I2C_COMD3_REG:
        case I2C_COMD4_REG:
        case I2C_COMD5_REG:
        case I2C_COMD6_REG:
        case I2C_COMD7_REG:
        case I2C_COMD8_REG:
        case I2C_COMD9_REG:
        case I2C_COMD10_REG:
        case I2C_COMD11_REG:
        case I2C_COMD12_REG:
        case I2C_COMD13_REG:
        case I2C_COMD14_REG:
        case I2C_COMD15_REG:
            // I2c comand done, master mode
            return(s->command_reg[(offset-(I2C_COMD0_REG*4))/4]);
            break;
    }

    //qemu_log_mask(LOG_GUEST_ERROR,
    //                "%s: Offset 0x%x\n", __func__, (int)offset);
    return -1;
}

static void esp32_i2c_write(void *opaque, hwaddr offset,
                                uint64_t value, unsigned size)
{
    Esp32I2CState *s = (Esp32I2CState *)opaque;
    unsigned long u32_value=value & 0xffff;

    qemu_log_mask(LOG_GUEST_ERROR,
                    "%s: %s regnum 0x%x val 0x%x\n", __func__,I2C_REG_NAME[offset/4], (int)offset,u32_value);


    switch (offset/4) {
        
    // Used by arduino driver
    case I2C_SCL_UNUSED1:
        {
            apb_data[data_offset++]=value;

            qemu_log_mask(LOG_GUEST_ERROR,
                            "%s: %s DATA!! 0x%x val 0x%x\n", __func__,I2C_REG_NAME[offset/4], (int)offset,u32_value);
        }
        break;

    case I2C_SR_REG:
        s->i2c_sr_reg=value;
        break;

    case I2C_INT_CLR_REG:
         s->int_status=0;
         qemu_irq_lower(irq);
        break;
        
    break;
    case I2C_CTR_REG:
        {
            int ix=0;
            if ((u32_value & BIT(5)) == BIT(5)) {
                //qemu_log_mask(LOG_GUEST_ERROR,
                //        "%s: start transmit 0x%x\n", __func__, s->num_out);   
                int buffer_ix=0;
                int data_ix;
                bool start_transfer=false;

                for (ix=0;ix<s->num_out;ix++) {
                            char opcode= (s->command_reg[ix] & 0x3800) >> 11;
                            char len=(s->command_reg[ix] & 0xf);
                            //qemu_log_mask(LOG_GUEST_ERROR,
                            //   "%s: execute OPCODE 0x%x\n" , __func__,opcode);


                            // Set command done
                            s->command_reg[ix]=s->command_reg[ix]  | 0x8000;


                            if (opcode==0) {

                                start_transfer=true;
                                // cmd restart

                                // Transaction start interrupt
                                s->int_status=I2C_TRANS_START_INT;
                                qemu_irq_raise(irq);
                            //qemu_log_mask(LOG_GUEST_ERROR,
                            //   "%s:i2c_start_transfer 0x%x\n" , __func__,apb_data[buffer_ix-1]);

                            }

                            if (opcode==1) {
                                if (start_transfer) {
                                    start_transfer=false;
                                     unsigned int address = (apb_data[buffer_ix] >> 1);
                                    //address = 0x78;
                                    unsigned int send = apb_data[buffer_ix] & 0x01;
                                    buffer_ix++;
                                    //if (send==1) send=0; else send=1;

                                    qemu_log_mask(LOG_GUEST_ERROR,
                                                "%s: start transfer adress --------- 0x%x s=%x\n", __func__, (int)address,send);


                                    // Not NULL response should indicate missing
                                    if (i2c_start_transfer(s->bus,address,send)==0) 
                                    {
                                        s->i2c_sr_reg=s->i2c_sr_reg | 0x01;
                                    }
                                    else
                                    {

                                    }
                                } else {
                                    for (data_ix=0;data_ix<len;data_ix++) {
                                        //if (data_offset>buffer_ix) {
                                        //        qemu_log_mask(LOG_GUEST_ERROR,
                                        //        "%s: ERROR not enough data for i2c_send 0x%x next %x\n", __func__, (int)apb_data[buffer_ix],apb_data[buffer_ix+1]);
                                        //
                                        //}
                                        i2c_send(s->bus,apb_data[buffer_ix++]);
                                            qemu_log_mask(LOG_GUEST_ERROR,
                                            "%s: i2c_send ---- 0x%x ---- next %x\n", __func__, (int)apb_data[buffer_ix-1],apb_data[buffer_ix]); 
                                     }

                                    s->int_status=I2C_TXFIFO_EMPTY_INT;
                                    qemu_irq_raise(irq);                                     
                                }

                                data_offset=0;
                            }

                            if (opcode==2) {
                                // read
                                int ret = i2c_recv(s->bus);     

                                qemu_log_mask(LOG_GUEST_ERROR,
                                    "%s: i2c_recv 0x%x\n", __func__, (int)ret);

                                // TODO, put data in fifo??
                                // length??           
                            }

                            if (opcode==3) {
                                // stop               
                                i2c_end_transfer(s->bus);
                                s->int_status=I2C_TRANS_COMPLETE_INT;
                                qemu_irq_raise(irq);

                            }

                            if (opcode==4) {
                                s->int_status=I2C_TRANS_COMPLETE_INT;
                                qemu_irq_raise(irq);
                            }

                }
                s->num_out=0;
            }
        }


        s->i2c_ctr_reg=value & 0xffff;
        // Clear transmit bit
        s->i2c_ctr_reg=s->i2c_ctr_reg & (0xffdf);
        //s->i2c_ctr_reg=0;
            //qemu_log_mask(LOG_GUEST_ERROR,
            //          "%s: I2C_CTR_REG 0x%x\n", __func__, (int)value & 0xffff);

            //qemu_log_mask(LOG_GUEST_ERROR,
            //          "%s: I2C_CTR_REG 0x%x\n", __func__, (int)s->i2c_ctr_reg & 0xffff);
                      
        break;

   case I2C_FIFO_CONF_REG:

            if ((u32_value & BIT(13)) == BIT(13)) {
                //qemu_log_mask(LOG_GUEST_ERROR,
                //        "%s: I2C_FIFO_CONF_REG  RESET TX FIFO 0x%x\n", __func__, (int)value & 0xffff);
                // Intentionally ignore this buffer clear as we will be one transmission behind most of the times
                //data_offset=0;
            }

            if ((u32_value & BIT(12)) == BIT(12)) {
                //qemu_log_mask(LOG_GUEST_ERROR,
                //        "%s: I2C_FIFO_CONF_REG  RESET RX FIFO 0x%x\n", __func__, (int)value & 0xffff);
            }


            //qemu_log_mask(LOG_GUEST_ERROR,
            //          "%s: log I2C_FIFO_CONF_REG 0x%x\n", __func__, (int)value & 0xffff);
            s->i2c_fifo_conf_reg=value;
         break;

    //case I2C_COMD3_REG:
    //        qemu_log_mask(LOG_GUEST_ERROR,
    //                  "%s: I2C_COMD3_REG offset 0x%x\n", __func__, (int)offset);


    case I2C_COMD0_REG:
    case I2C_COMD1_REG:
    case I2C_COMD2_REG:
    case I2C_COMD3_REG:
    case I2C_COMD4_REG:
    case I2C_COMD5_REG:
    case I2C_COMD6_REG:
    case I2C_COMD7_REG:
    case I2C_COMD8_REG:
    case I2C_COMD9_REG:
    case I2C_COMD10_REG:
    case I2C_COMD11_REG:
    case I2C_COMD12_REG:
    case I2C_COMD13_REG:
    case I2C_COMD14_REG:
    case I2C_COMD15_REG:
     {
            char opcode= (value & 0x3800) >> 11;
            //qemu_log_mask(LOG_GUEST_ERROR,
            //          "%s:OPCODE 0x%x\n" , __func__,opcode);

            // Assume commands are filled in sequencially
            if (s->num_out<=1+(offset-I2C_COMD0_REG*4)/4)
            {
                s->num_out=1+(offset-I2C_COMD0_REG*4)/4;
            } 

            s->command_reg[(offset-(I2C_COMD0_REG*4))/4]=value& 0xffff;

            //qemu_log_mask(LOG_GUEST_ERROR,
            //       "%s: num comands 0x%x\n", __func__, (int)s->num_out);



            //qemu_log_mask(LOG_GUEST_ERROR,
            //          "%s: I2C_COMD_REG_%d 0x%x\n", __func__,(offset-(I2C_COMD0_REG*4))/4, (int)value& 0xffff);


         }
         break;
    case I2C_INT_ENA_REG:
         break;
    default:
    break;
        //qemu_log_mask(LOG_GUEST_ERROR,
        //              "%s: %s regnum 0x%x 0x%x\n", __func__,I2C_REG_NAME[offset/4], (int)offset/4,value);
    }
}

static const MemoryRegionOps esp32_i2c_ops = {
    .read = esp32_i2c_read,
    .write = esp32_i2c_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void esp32_i2c_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    Esp32I2CState *s = ESP32_I2C(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    s->i2c_ctr_reg=0;
    s->i2c_sr_reg=0xffff;

    s->bus = i2c_init_bus(dev, "i2c");
    memory_region_init_io(&s->iomem, obj, &esp32_i2c_ops, s,
                          "esp32_i2c", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const TypeInfo esp32_i2c_info = {
    .name          = TYPE_ESP32_I2C,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Esp32I2CState),
    .instance_init = esp32_i2c_init,
};

static void esp32_i2c_register_types(void)
{
    type_register_static(&esp32_i2c_info);
}

type_init(esp32_i2c_register_types)

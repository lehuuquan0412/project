#include <stdio.h>
#include <rtthread.h>
#include <mqtt.h>
#include <obd2.h>
#include <can_user.h>
#include <ppp_sp.h>




int main(int argc, char **argv) {
    while(!ppp_get_status_connect(ppp_device_get()));
    rt_thread_mdelay(6000);
    can_system_init();
    mqtt_system_init();
    obd_init();
    return RT_EOK;
}

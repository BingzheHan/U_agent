#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include "select.h"
#include "uagent.h"
#include "os.h"
#include "uagent_debug.h"
#include "common.h"
#include "server_cmd.h"
#include <sys/sysinfo.h>
static int get_ibeacon_status()
{
	return 0;
}

static int get_wifi_module_status()
{
	return 0;
}

static int get_net_type()
{
	return 0;
}
static int cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n) 
{   
    unsigned long od, nd;    
    unsigned long id, sd;
    int cpu_use = 0;   
    
    od = (unsigned long) (o->user + o->nice + o->system +o->idle);
    nd = (unsigned long) (n->user + n->nice + n->system +n->idle);
      
    id = (unsigned long) (n->user - o->user);    
    sd = (unsigned long) (n->system - o->system);
    if((nd-od) != 0)
    	cpu_use = (int)((sd+id)*10000)/(nd-od); 
    else 
		{	cpu_use = 0;
			uagent_printf(MSG_ERROR,"The old is equal to new.\n");
    	}
    return cpu_use;
}

static void  get_cpuoccupy_sample (CPU_OCCUPY *cpust) 
{   
    FILE *fd;         
    int n;            
    char buff[256]; 
    CPU_OCCUPY *cpu_occupy;
    cpu_occupy=cpust;
                                                                                                               
    fd = fopen ("/proc/stat", "r"); 
    fgets (buff, sizeof(buff), fd);
    
    sscanf (buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle);
    
    fclose(fd);     
}
static int get_cpuoccupy_status()
{
	CPU_OCCUPY cpu_stat1;
    CPU_OCCUPY cpu_stat2;
	int cpu;
	get_cpuoccupy_sample((CPU_OCCUPY *)&cpu_stat1);
	sleep(2);
	get_cpuoccupy_sample((CPU_OCCUPY *)&cpu_stat2);
	cpu = cal_cpuoccupy ((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);
	return cpu;
}

static unsigned long get_memoccupy_status()
{
	struct sysinfo s_info;
	int error;
	unsigned long mem;
	float memoccupy;
	error = sysinfo(&s_info);
	if (0 != error)
		{ 
			uagent_printf(MSG_ERROR,"Get mem info code error=%d\n",error);
			return -1;
		}
	uagent_printf(MSG_INFO,"Uptime = %ds\nLoad: 1 min%d / 5 min %d / 15 min %d\n"
           "RAM: total %d / free %d /shared%d\n"
           "Memory in buffers = %d\nSwap:total%d/free%d\n"
           "Number of processes = %d\n",
           s_info.uptime, s_info.loads[0],
           s_info.loads[1], s_info.loads[2],
           s_info.totalram, s_info.freeram,
           s_info.sharedram, s_info.bufferram,
           s_info.totalswap, s_info.freeswap,
          s_info.procs );
	memoccupy = (float)(s_info.totalram - s_info.freeram)/(float)s_info.totalram ;
	uagent_printf(MSG_INFO,"The mem occupy is %f\n",memoccupy);
	mem = memoccupy * 10000;
    return mem;

}
void dev_update()
{
	return;
}

void dev_log()
{
	return;
}
void dev_restart()
{
	return;
}

struct status_data dev_status_handle()
{
	struct status_data dev_status;
	dev_status.cpu_usage = get_cpuoccupy_status();
	dev_status.ibeacon_status = get_ibeacon_status();
	dev_status.wifi_collect_module = get_wifi_module_status();
	dev_status.net_type = get_net_type();
	dev_status.mem_usage = get_memoccupy_status();
	uagent_printf(MSG_ERROR,"The cpu occupy rate is %d, the ibeacon status" 
		"is %d, the wifi collect module status is %d, the net type is %d," 
		"the mem occupy is %d.\n",
		dev_status.cpu_usage, dev_status.ibeacon_status, dev_status.wifi_collect_module,
		dev_status.net_type,dev_status.mem_usage);
	return dev_status;	
}


struct resp_data handle_server_msg(struct server_msg server_messege,
	struct status_data *status)
{
	struct resp_data resp_server;
	switch(server_messege.srv_cmd)
	 {
		case 0:
			dev_update();
			break;
		case 1:
			dev_restart();
			break;
		case 2:
			*status = dev_status_handle();
			break;
		case 3:	
			dev_log();
			break;
		default:
			uagent_printf(MSG_ERROR,"No such server command!");			
	 }
	 resp_server.srv_cmd = server_messege.srv_cmd;
	return resp_server;
}



/********************************************************************************
1、本文件主要是描述wifi设备与服务器之间信息交互的格式。

2、wifi设备与服务器之间只要是数据与命令之间的交互，主要包括：
（1）数据通路（单向）：wifi设备将收集到的data上传到服务器端，这个socket是单向的，只有上行
（2）控制通路（双向）：
     1）wifi设备接受服务器端发过来的cmd，进行相应的操作
	 2）wifi设备将一些状态信息发给服务器
	 
3、服务器端发给wifi设备的信息主要包括（按需添加）：
（1）升级
（2）重启
（3）获取wifi设备状态（对于wifi设备来说，这个属于被动上报状态）
（4）在线导出log
********************************************************************************/

/**********************************************************************************/
/********************************Macro Definition**********************************/
/**********************************************************************************/

/**********************************************************************************/
/********************************Enum Definition***********************************/
/**********************************************************************************/

/* 服务器下发的命令类型 */
enum server_cmd
{
	UPDATE = 0,             /* 通知设备有新版本，需要升级 */
	RESTART,                /* server端远程让设备重启 */
	STATUS,                 /* server端让设备发送运行状态 */
	LOG                   /* 远程从设备导出log */
};
//typedef unsigned char server_cmd_uint8;

/* wifi设备往服务器发送数据或者状态用的网络（wifi还是3g） */
enum network_type
{
	WIFI = 0,
	WCDMA
};
//typedef unsigned char network_type_uint8;

/* wifi设备的各个组件工作是否正常 */
enum wifi_module_status
{
	OK = 0,
	UNUSUAL
	
};
//typedef unsigned char wifi_module_status_uint8;

/* wifi设备通过控制通路传给服务器的消息是属于回应服务器，还是主动上报状态 */
enum ctrl_msg_type
{
	NOTIFY = 0,
	RESP
};
//typedef unsigned char ctrl_msg_tyep_uint8;

/**********************************************************************************/
/******************************Structure Definition********************************/
/**********************************************************************************/

/* wifi设备收集到的信号按如下格式组织，发送给服务器(这种结构对齐方式可能不行，后续联调时发现问题调整) */
struct wifi_signal_data
{
	unsigned char	 user_dev_mac[6];
	unsigned char    resv[2];/* 收集到的用户wifi设备的mac地址 */
	int              rssi; /* 收集到的wifi信号的信号强度 */
	unsigned char	 wifi_dev_mac[6];    /* wifi设备的mac地址，也就是apcli0的mac地址 */
	unsigned char    resv1[2];
	unsigned int	 timestamp;          /* 收到当前wifi信号的系统时间 */
	unsigned char    hotpot_mac[6];      /* apcli0关联的路由器mac地址，用于定位wifi设备 */
	unsigned char    resv2[2];
};

/*如果通过控制通路传输的是设备工作状态，则data部分使用如下结构*/
struct status_data
{
	enum wifi_module_status		wifi_collect_module; /* wifi收集模块的工作状态是否正常，是否在收集数据 */
	enum network_type				net_type;            /* 当前往服务器推送数据是利用wifi还是3g */
	enum wifi_module_status		ibeacon_status;      /* ibeacon模块工作是否正常 */
	int							cpu_usage;           /* 当前cpu使用率 */
	unsigned long				mem_usage;			 /* 当前内存使用率 */
};

/* 如果通过控制通路传输的是wifi设备接收到服务器命令后的响应，则data部分使用如下结构 */
struct resp_data
{
	enum server_cmd				srv_cmd;			 /* 本次收到的服务器命令类型 */
	int 	      				result;				 /* 是否正确收到服务器命令，0正确   需要讨论，比如重启跟升级，是否升级成功或者重启成功给服务器一个回复 */
};

/* wifi设备将各个组件的工作状态按如下格式组织，发送给服务器 */
struct wifi_ctrl_data
{
	enum ctrl_msg_type            msg_type;			 /* 0:主动上报状态给服务器    1：回应服务器是否成功接收到服务器的命令 */
	char							data[16];            /* 根据msg_tyep的类型决定数据部分的格式 */
};

/* 服务器给wifi设备发命令的消息格式 */
struct server_msg
{
	enum server_cmd		        srv_cmd;                     /* 服务器下发给wifi设备的命令类型 */
	char					msg[256];                    /* 如果是update命令，则应该是新版本的路径，否则为0，不必解析 */
};

typedef struct PACKED         //����һ��cpu occupy�Ľṹ��
{
char name[20];      //����һ��char���͵�������name��20��Ԫ��
unsigned int user; //����һ���޷��ŵ�int���͵�user
unsigned int nice; //����һ���޷��ŵ�int���͵�nice
unsigned int system;//����һ���޷��ŵ�int���͵�system
unsigned int idle; //����һ���޷��ŵ�int���͵�idle
}CPU_OCCUPY;

struct status_data dev_status_handle();
struct resp_data handle_server_msg(struct server_msg server_messege, 
	struct status_data *status );


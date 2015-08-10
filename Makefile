all: select_server2.o select_server1.o select_uagent.o uagent_debug.o select.o os_unix.o common.o uagent.o server_cmd_handle.o
	cc -o select_uagent select_uagent.o uagent_debug.o select.o os_unix.o common.o uagent.o server_cmd_handle.o
	cc -o select_server1 select_server1.o  uagent_debug.o select.o os_unix.o common.o 
	cc -o select_server2 select_server2.o  uagent_debug.o select.o os_unix.o common.o 
select_uagent.o : select_uagent.c 
				cc -c -g select_uagent.c
select_server1.o : select_server1.c
				cc -c -g select_server1.c
select_server2.o : select_server2.c
				cc -c -g select_server2.c
select.o : select.c
				cc -c -g select.c
uagent_debug.o : uagent_debug.c 
				cc -c -g uagent_debug.c
os_unix.o : os_unix.c
				cc -c -g os_unix.c
common.o : common.c
				cc -c -g common.c
server_cmd_handle.o : server_cmd_handle.c 
				cc -c -g server_cmd_handle.c
uagent.o : uagent.c 
				cc -c -g uagent.c
clean:  
	rm -rf *.o select_server1 select_server2 select_uagent

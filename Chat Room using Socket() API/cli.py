import sys, socket, select,os

#check argurments
def chat_client():
    if(len(sys.argv) < 3) :
        print('valid format : python3 cli.py hostname port')
        sys.exit()
    #server IP and Port#
    host = sys.argv[1]
    port = int(sys.argv[2])

    #create socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # connect to remote host
    try :
        s.connect((host, port))
        
    except :
        print('Unable to connect')
        sys.exit()
    try:
        
        while 1:
                       
            # Get the list sockets which are readable
            read_sockets, write_sockets, error_sockets = select.select([sys.stdin, s] , [], [])
                    
            for sock in read_sockets:
                #if readable socket is server                    
                if sock == s:
                    # get data from server
                    data = sock.recv(4096)
                    if data:
                        #print data
                        print("\r"+data.decode())
                        sys.stdout.write('[You] '); sys.stdout.flush()
                    # terminate socket if the server is terminated
                    else:
                        print("\n server closed")
                        s.close()
                        sys.exit()
                #if readable socket is the client, user is sending message                         
                else:
                    #read message
                    message = input()
                    # user uploads a file
                    
                    if message=="I will upload a file":
                        s.send("I will upload a file".encode())
                        #get filename and compute filesize
                        command, filename = sys.stdin.readline().split() 
                        filesize = os.path.getsize(filename.replace("'",""))
                        #send command, filename, filesize to server
                        s.send((command+" "+filename+" "+str(filesize)).encode())
                        #read file and send it to server
                        f = open(filename.replace("'",""),'rb')
                        l = f.read(4096)
                        while l:
                            s.sendall(l)
                            l = f.read(4096)
                        f.close()
                        #print upload info, then go back to chatting
                        sys.stdout.write("\033[F")
                        sys.stdout.write("\r> File %s has uploaded!\n" % filename)
                        sys.stdout.write('[You] '); sys.stdout.flush()
                    else:
                        #send message to the server
                        s.send(message.encode())
                        sys.stdout.write('[You] '); sys.stdout.flush()
        s.close()
    #terminate when ctrl+c pressed
    except KeyboardInterrupt:
        print("\nexit")
        s.close()
        sys.exit()
        
chat_client()

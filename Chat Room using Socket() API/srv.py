
import sys, socket, select,os
#check arguments
if(len(sys.argv) < 3) :
    print('valid format : python3 cli.py hostname port')
    sys.exit()

#get hostname, port#, define Socket list and receving buffer
HOST = sys.argv[1]
PORT = int(sys.argv[2])
SOCKET_LIST = []
RECV_BUFFER = 4096 
#create folder
def createFolder(directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ('Error: Creating directory. ' +  directory)
#run chat server
def chat_server():
    #create socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(10)
    client_count = 0
    # add server socket object to the list of readable connections
    SOCKET_LIST.append(server_socket)
 
    print("Chat server started on port " + str(PORT) +".")
 
    #make directory
    createFolder(os.getcwd()+"/server")

    
    while 1:
        try:
            # get the list sockets by select()
            ready_to_read,ready_to_write,in_error = select.select(SOCKET_LIST,[],[])
        
            for sock in ready_to_read:
                # a new client connection
                if sock == server_socket: 
                    sockfd, addr = server_socket.accept()
                    SOCKET_LIST.append(sockfd)
                    client_count += 1
                    #print, broadcast the connection info
                    if client_count > 1:
                        print("> New user %s:%s entered " % addr + "(%d users online)" % client_count)
                        sockfd.send( ("> Connected to the chat server " + "(%d users online)" % client_count).encode())
                        broadcast(server_socket, sockfd, "> New user %s:%s entered " % addr + "(%d users online)" % client_count)
                    else:
                        print("> New user %s:%s entered " % addr + "(%d user online)" % client_count)
                        sockfd.send( ("> Connected to the chat server " + "(%d user online)" % client_count).encode())
                        broadcast(server_socket, sockfd, "> New user %s:%s entered " % addr + "(%d user online)" % client_count)
                    
                # a message from a client
                else:
                        # receive data from the socket.
                        data = sock.recv(RECV_BUFFER)
                        # print, broadcast data
                        if data:
                            #file uploading
                            if data.decode().startswith('#upload'):
                                #get filename, filesize
                                command, filename, filesize = data.decode().split()
                                #make complete filename
                                folder_path = os.getcwd() + "/server"
                                file_name = filename.replace("'","")
                                complete_name = os.path.join(folder_path,file_name)
                                #read file to the end.
                                total_bytes_read = 0
                                file_size = int(filesize)
                                with open(complete_name, "wb") as f:
                                    while total_bytes_read < file_size:
                                        data = sock.recv(4092)
                                        total_bytes_read += len(data)
                                        f.write(data)
                                f.close()

                                #send upload info
                                print("> User %s:%s has uploaded a file" % sock.getpeername())
                                broadcast(server_socket, sock, "> User %s:%s has uploaded a file" % sock.getpeername())
                            #print on the server and send message to clients
                            else:
                                print("[%s:%s] " % sock.getpeername() + data.decode() )
                                broadcast(server_socket, sock, "[%s:%s] " % sock.getpeername() + data.decode())  
                        # remove broken socket
                        else:
                            if sock in SOCKET_LIST:
                                SOCKET_LIST.remove(sock)
                            # print, broadcast the exit info
                            client_count -= 1
                            if client_count > 1:
                                print("< The user %s:%s left " % sock.getpeername() + "(%d users online)" % client_count)
                                broadcast(server_socket, sock, "< The user %s:%s left " % sock.getpeername() + "(%d users online)" % client_count) 
                            else:
                                print("< The user %s:%s left " % sock.getpeername() + "(%d user online)" % client_count)
                                broadcast(server_socket, sock, "< The user %s:%s left " % sock.getpeername() + "(%d user online)" % client_count) 

        # terminate when ctrl+c pressed 
        except KeyboardInterrupt:
            sys.stdout.write("\033[F")
            print("\nexit")
            server_socket.close()
            sys.exit()

    server_socket.close()
    
# broadcast chat messages to all connected clients
def broadcast (server_socket, sock, message):
    for socket in SOCKET_LIST:
        # send the message only to peer
        if socket != server_socket and socket != sock :
            socket.send(message.encode())
           
chat_server()
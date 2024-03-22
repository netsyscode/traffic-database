import socket

SERVER_HOST = '127.0.0.1'
SERVER_PORT = 12345
BUFFER_SIZE = 1024

def socket_connect(host,port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host,port))
    return s

def send_pkt(s, message):
    s.sendall(message.encode())
    
def recv_pkt(s):
    response = s.recv(BUFFER_SIZE).decode()
    return response

if __name__ == "__main__":
    s = socket_connect(SERVER_HOST,SERVER_PORT)
    while(True):
        message = input("Enter message for C++ program: ")

        send_pkt(s,message)

        if message == "q":
            break
            
        response = recv_pkt(s)
        
        print(response)

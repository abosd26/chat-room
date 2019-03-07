# chat-room

A chat room include server and client using TCP socket.

client: 1. Usage => ./client <server IP> <port #>  
        2. Connect to server.
        3. Handle input:
          i <Message> => 訊息傳送給同一個群組下的成員
          ii /W <Name or room> <Message> => 判斷訊息傳送的對象是人還是群組並將訊息傳給指定的人或者群組
          iii Bye => 中斷連線
server: 1. Usage => ./server <port #>
        2. Use multi-thread to handle requests from clients.
        3. List all the members and chat room online, client can choose which room to join.
        4. Handle clients request:
          i 訊息傳送給同一個群組下的成員。
          ii 判斷訊息傳送的對象是人還是群組並將訊息傳給指定的人或者群組。
          iii 有人加入聊天室或者離開聊天室時,通知所有的成員。

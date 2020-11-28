
generate:
	@echo "Generating executable."
	gcc server.c -o server
	gcc client.c -o client

server-run:
	@./server 7070

client-run:
	@./client 127.0.0.1 7070 Morteza

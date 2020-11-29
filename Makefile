
# default username
u = Morteza

generate:
	@echo "Generating executable."
	@mkdir -p ./build
	gcc server.c -o build/server
	gcc client.c -o build/client

server-run:
	@build/server 7070

client-run:
	@build/client 127.0.0.1 7070 $(u)

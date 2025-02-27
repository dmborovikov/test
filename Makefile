build_server:
	cd server && mkdir -p build && cd build && cmake .. && make

build_client:
	cd client && mkdir -p build && cd build && cmake .. && make

run_server:
	cd server/build && ./Server

run_client:
	cd client/build && ./Client

run:
	cd client && mkdir -p build && cd build && cmake .. && make
	cd server && mkdir -p build && cd build && cmake .. && make && ./Server

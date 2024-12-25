
SRC_DIR = ./src
CONTAINER_NAME = linux

all: docker-compose.yml Dockerfile
	sudo docker compose -f docker-compose.yml up --build -d 

restart:
	sudo docker compose restart

rerun: clean all

clean:
	- sudo docker compose down
	- sudo docker system prune -a

stop:
	sudo docker compose down

connect:
	sudo docker exec -it $(CONTAINER_NAME) bash
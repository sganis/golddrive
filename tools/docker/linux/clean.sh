#!/bin/bash
# Stop all containers
docker stop $(docker ps -aq)
# Delete all containers
docker rm $(docker ps -aq)
# Delete all images
#docker rmi $(docker images -q)

# list containers
docker ps -aq
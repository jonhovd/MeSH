# Bruk et lettvekt basebilde med bash-støtte
FROM ubuntu:20.04

# Oppdater og installer nødvendige pakker som bash og andre avhengigheter
RUN apt-get update && apt-get install -y \
    bash \
    curl \
    wget \
    vim \
    software-properties-common

# Kopier hele prosjektet ditt inn i containeren
WORKDIR /usr/src/app
COPY . .

# Gi eksekveringsrettigheter til Bash-skriptene dine
RUN chmod +x /usr/src/app/src/*.sh

CMD ["bash", "/usr/src/app/src/install_wt.sh"]


# Expose the port that the web service will use
EXPOSE 8080


# Bygg Docker image:
# docker build -t my-mesh-project .

# Kjør containeren:
# docker run -it my-mesh-project
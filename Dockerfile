# Use the official Debian base image
FROM debian:latest

# Set the working directory inside the container
WORKDIR /root

# Copy the startMeSHWeb.sh script into the container
COPY startMeSHWeb.sh /root/startMeSHWeb.sh

# Make the script executable
RUN chmod +x /root/startMeSHWeb.sh

# Install necessary dependencies (you may need to adjust this list based on the script's requirements)
RUN apt-get update && apt-get install -y \
    nginx \
    curl \
    && apt-get clean

# Start the web service by running the script
CMD ["./startMeSHWeb.sh"]

# Expose the port that the web service will use
EXPOSE 8080


# For å bygge konteineren kjør dette i terminal:
    # docker build -t mesh-web-service .

# Får å kjøre konteineren kjør dette:
    # docker run -d -p 8080:8080 --name my-mesh-web-service mesh-web-service

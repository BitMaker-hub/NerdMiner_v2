FROM node:18-alpine

# Create app directory
WORKDIR /app

# Install app dependencies
COPY package*.json ./
RUN npm install --only=production

# Bundle app source
COPY . .

# Expose ports
EXPOSE 3000
EXPOSE 33333/udp

CMD [ "node", "server.js" ]
